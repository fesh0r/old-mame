/*
    dc.c - Dreamcast video emulation

*/

#include "driver.h"
#include "dc.h"
#include "cpu/sh4/sh4.h"
#include "render.h"
#include "rendutil.h"

static int vblc=0;
#define DEBUG_FIFO_POLY (0)
#define DEBUG_PVRCTRL	(0)
#define DEBUG_PVRTA	(0)
#define DEBUG_PVRTA_REGS (0)
#define DEBUG_PVRDLIST	(1)
#define DEBUG_PALRAM (1)

#define NUM_BUFFERS 4

static UINT32 pvrctrl_regs[0x100/4];
static UINT32 pvrta_regs[0x2000/4];
static const int pvr_parconfseq[] = {1,2,3,2,3,4,5,6,5,6,7,8,9,10,11,12,13,14,13,14,15,16,17,16,17,0,0,0,0,0,18,19,20,19,20,21,22,23,22,23};
static const int pvr_wordsvertex[24]  = {8,8,8,8,8,16,16,8,8,8, 8, 8,8,8,8,8,16,16, 8,16,16,8,16,16};
static const int pvr_wordspolygon[24] = {8,8,8,8,8, 8, 8,8,8,8,16,16,8,8,8,8, 8, 8,16,16,16,8, 8, 8};
static int pvr_parameterconfig[64];
static UINT32 dilated0[15][1024];
static UINT32 dilated1[15][1024];
static int dilatechose[64];
static float wbuffer[480][640];

UINT64 *dc_texture_ram;
static UINT32 tafifo_buff[32];

static emu_timer *vbout_timer;
static emu_timer *hbin_timer;
static emu_timer *endofrender_timer_isp;
static emu_timer *endofrender_timer_tsp;
static emu_timer *endofrender_timer_video;

static int scanline;
static bitmap_t *fakeframebuffer_bitmap;
static void testdrawscreen(const running_machine *machine,bitmap_t *bitmap,const rectangle *cliprect);

typedef struct texinfo {
	UINT32 address, vqbase;
	int sizex, sizey, sizes, pf, palette, mode, mipmapped;

	UINT32 (*r)(struct texinfo *t, float x, float y);
	int palbase, cd;
} texinfo;

typedef	struct
{
	float x, y, w, u, v;
} vert;

typedef struct
{
	int svert, evert;
	texinfo ti;
} strip;

typedef struct {
	vert verts[65536];
	strip strips[65536];

	int verts_size, strips_size;
	UINT32 ispbase;
	UINT32 fbwsof1;
	UINT32 fbwsof2;
	int busy;
	int valid;
} receiveddata;

typedef struct {
	int tafifo_pos, tafifo_mask, tafifo_vertexwords, tafifo_listtype;
	int start_render_received;
	int renderselect;
	int listtype_used;
	int alloc_ctrl_OPB_Mode, alloc_ctrl_PT_OPB, alloc_ctrl_TM_OPB, alloc_ctrl_T_OPB, alloc_ctrl_OM_OPB, alloc_ctrl_O_OPB;
	receiveddata grab[NUM_BUFFERS];
	int grabsel;
	int grabsellast;
	UINT32 paracontrol,paratype,endofstrip,listtype,global_paratype,parameterconfig;
	UINT32 groupcontrol,groupen,striplen,userclip;
	UINT32 objcontrol,shadow,volume,coltype,texture,offfset,gouraud,uv16bit;
	UINT32 textureusize,texturevsize,texturesizes,textureaddress,scanorder,pixelformat;
	UINT32 srcalphainstr,dstalphainstr,srcselect,dstselect,fogcontrol,colorclamp,usealpha;
	UINT32 ignoretexalpha,flipuv,clampuv,filtermode,sstexture,mmdadjust,tsinstruction;
	UINT32 depthcomparemode,cullingmode,zwritedisable,cachebypass,dcalcctrl,volumeinstruction,mipmapped,vqcompressed,strideselect,paletteselector;
} pvrta_state;

static pvrta_state state_ta;


INLINE UINT32 cv_1555(UINT16 c)
{
	return
		(c & 0x8000 ? 0xff000000 : 0) |
		((c << 9) & 0x00f80000) | ((c << 4) & 0x00070000) |
		((c << 6) & 0x0000f800) | ((c << 1) & 0x00000700) |
		((c << 3) & 0x000000f8) | ((c >> 2) & 0x00000007);
}

INLINE UINT32 cv_1555z(UINT16 c)
{
	return
		(c & 0x8000 ? 0xff000000 : 0) |
		((c << 9) & 0x00f80000) |
		((c << 6) & 0x0000f800) |
		((c << 3) & 0x000000f8);
}

INLINE UINT32 cv_565(UINT16 c)
{
	return
		0xff000000 |
		((c << 8) & 0x00f80000) | ((c << 3) & 0x00070000) |
		((c << 5) & 0x0000fc00) | ((c >> 1) & 0x00000300) |
		((c << 3) & 0x000000f8) | ((c >> 2) & 0x00000007);
}

INLINE UINT32 cv_565z(UINT16 c)
{
	return
		0xff000000 |
		((c << 8) & 0x00f80000) |
		((c << 5) & 0x0000fc00) |
		((c << 3) & 0x000000f8);
}

INLINE UINT32 cv_4444(UINT16 c)
{
	return
 		((c << 16) & 0xf0000000) | ((c << 12) & 0x0f000000) |
		((c << 12) & 0x00f00000) | ((c <<  8) & 0x000f0000) |
		((c <<  8) & 0x0000f000) | ((c <<  4) & 0x00000f00) |
		((c <<  4) & 0x000000f0) | ((c      ) & 0x0000000f);
}

INLINE UINT32 cv_4444z(UINT16 c)
{
	return
 		((c << 16) & 0xf0000000) |
		((c << 12) & 0x00f00000) |
		((c <<  8) & 0x0000f000) |
		((c <<  4) & 0x000000f0);
}

INLINE UINT32 cv_yuv(UINT16 c1, UINT16 c2, int x)
{
	int u = 11*((c1 & 0xff) - 128);
	int v = 11*((c2 & 0xff) - 128);
	int y = (x & 1 ? c2 : c1) >> 8;
	int r = y + v/8;
	int g = y - u/32 - v/16;
	int b = y + (3*u)/16;
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	return 0xff000000 | (r << 16) | (g << 8) | b;
}


static UINT32 tex_r_yuv_n(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + (t->sizex*yt + (xt & ~1))*2;
	UINT16 c1 = *(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp));
	UINT16 c2 = *(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp+2));
	return cv_yuv(c1, c2, xt);
}

static UINT32 tex_r_1555_n(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + (t->sizex*yt + xt)*2;
	return cv_1555z(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_1555_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + (dilated1[t->cd][xt] + dilated0[t->cd][yt])*2;
	return cv_1555(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_1555_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + (dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 1])*2;
	return cv_1555(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_565_n(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + (t->sizex*yt + xt)*2;
	return cv_565z(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_565_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + (dilated1[t->cd][xt] + dilated0[t->cd][yt])*2;
	return cv_565(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_565_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + (dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 1])*2;
	return cv_565(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_4444_n(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + (t->sizex*yt + xt)*2;
	return cv_4444z(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_4444_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + (dilated1[t->cd][xt] + dilated0[t->cd][yt])*2;
	return cv_4444(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_4444_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + (dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 1])*2;
	return cv_4444(*(UINT16 *)(((UINT8 *)dc_texture_ram) + WORD_XOR_LE(addrp)));
}

static UINT32 tex_r_p4_1555_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int off = dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int addrp = t->address + (off >> 1);
	int c = (((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] >> ((off & 1) << 2)) & 0xf;
	return cv_1555(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p4_1555_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] & 0xf;
	return cv_1555(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p4_565_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int off = dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int addrp = t->address + (off >> 1);
	int c = (((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] >> ((off & 1) << 2)) & 0xf;
	return cv_565(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p4_565_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] & 0xf;
	return cv_565(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p4_4444_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int off = dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int addrp = t->address + (off >> 1);
	int c = (((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] >> ((off & 1) << 2)) & 0xf;
	return cv_4444(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p4_4444_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] & 0xf;
	return cv_4444(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p4_8888_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int off = dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int addrp = t->address + (off >> 1);
	int c = (((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] >> ((off & 1) << 2)) & 0xf;
	return pvrta_regs[t->palbase + c];
}

static UINT32 tex_r_p4_8888_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)] & 0xf;
	return pvrta_regs[t->palbase + c];
}

static UINT32 tex_r_p8_1555_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return cv_1555(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p8_1555_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return cv_1555(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p8_565_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return cv_565(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p8_565_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return cv_565(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p8_4444_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return cv_4444(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p8_4444_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return cv_4444(pvrta_regs[t->palbase + c]);
}

static UINT32 tex_r_p8_8888_tw(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int addrp = t->address + dilated1[t->cd][xt] + dilated0[t->cd][yt];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return pvrta_regs[t->palbase + c];
}

static UINT32 tex_r_p8_8888_vq(texinfo *t, float x, float y)
{
	int xt = ((int)x) & (t->sizex-1);
	int yt = ((int)y) & (t->sizey-1);
	int idx = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(t->address + dilated1[t->cd][xt >> 1] + dilated0[t->cd][yt >> 1])];
	int addrp = t->vqbase + 8*idx + dilated1[t->cd][xt & 1] + dilated0[t->cd][yt & 3];
	int c = ((UINT8 *)dc_texture_ram)[BYTE_XOR_LE(addrp)];
	return pvrta_regs[t->palbase + c];
}


static UINT32 tex_r_default(texinfo *t, float x, float y)
{
	return ((int)x ^ (int)y) & 4 ? 0xffffff00 : 0xff0000ff;
}

static void tex_prepare(texinfo *t)
{
	int miptype = 0;

	t->r = tex_r_default;
	t->cd = dilatechose[t->sizes];
	t->palbase = 0;
	t->vqbase = t->address;

	//  fprintf(stderr, "tex %d %d %d %d\n", t->pf, t->mode, pvrta_regs[PAL_RAM_CTRL], t->mipmapped);

	switch(t->pf) {
	case 0: // 1555
		switch(t->mode) {
		case 0:  t->r = tex_r_1555_tw; miptype = 2; break;
		case 1:  t->r = tex_r_1555_n;  miptype = 2; break;
		default: t->r = tex_r_1555_vq; miptype = 3; t->address += 0x800; break;
		}
		break;

	case 1: // 565
		switch(t->mode) {
		case 0:  t->r = tex_r_565_tw; miptype = 2; break;
		case 1:  t->r = tex_r_565_n;  miptype = 2; break;
		default: t->r = tex_r_565_vq; miptype = 3; t->address += 0x800; break;
		}
		break;

	case 2: // 4444
		switch(t->mode) {
		case 0:  t->r = tex_r_4444_tw; miptype = 2; break;
		case 1:  t->r = tex_r_4444_n;  miptype = 2; break;
		default: t->r = tex_r_4444_vq; miptype = 3; t->address += 0x800; break;
		}
		break;

	case 3: // yuv422
		switch(t->mode) {
		case 0:  /*t->r = tex_r_yuv_tw*/; miptype = -1; break;
		case 1:  t->r = tex_r_yuv_n; miptype = -1; break;
		default: /*t->r = tex_r_yuv_vq*/; miptype = -1; break;
		}
		break;

	case 4: // bumpmap
		break;

	case 5: // 4bpp palette
		t->palbase = 0x400 | ((t->palette & 0x3f) << 4);
		switch(t->mode) {
		case 0: case 1:
			miptype = 0;

			switch(pvrta_regs[PAL_RAM_CTRL]) {
			case 0: t->r = tex_r_p4_1555_tw; break;
			case 1: t->r = tex_r_p4_565_tw;  break;
			case 2: t->r = tex_r_p4_4444_tw; break;
			case 3: t->r = tex_r_p4_8888_tw; break;
			}
			break;
		default:
			miptype = 3; // ?

			switch(pvrta_regs[PAL_RAM_CTRL]) {
			case 0: t->r = tex_r_p4_1555_vq; t->address += 0x800; break;
			case 1: t->r = tex_r_p4_565_vq;  t->address += 0x800; break;
			case 2: t->r = tex_r_p4_4444_vq; t->address += 0x800; break;
			case 3: t->r = tex_r_p4_8888_vq; t->address += 0x800; break;
			}
			break;
		}
		break;

	case 6: // 8bpp palette
		t->palbase = 0x400 | ((t->palette & 0x30) << 4);
		switch(t->mode) {
		case 0: case 1:
			miptype = 1;

			switch(pvrta_regs[PAL_RAM_CTRL]) {
			case 0: t->r = tex_r_p8_1555_tw; break;
			case 1: t->r = tex_r_p8_565_tw; break;
			case 2: t->r = tex_r_p8_4444_tw; break;
			case 3: t->r = tex_r_p8_8888_tw; break;
			}
			break;
		default:
			miptype = 3; // ?

			switch(pvrta_regs[PAL_RAM_CTRL]) {
			case 0: t->r = tex_r_p8_1555_vq; t->address += 0x800; break;
			case 1: t->r = tex_r_p8_565_vq;  t->address += 0x800; break;
			case 2: t->r = tex_r_p8_4444_vq; t->address += 0x800; break;
			case 3: t->r = tex_r_p8_8888_vq; t->address += 0x800; break;
			}
			break;
		}
		break;

	case 9: // reserved
		break;
	}

	if (t->mipmapped)
	{
		// full offset tables for reference,
		// we don't do mipmapping, so don't use anything < 8x8
		// first table is half-bytes

		// 4BPP palette textures
		// Texture size _4-bit_ offset value for starting address
		// 1x1          0x00003
		// 2x2          0x00004
		// 4x4          0x00008
		// 8x8          0x00018
		// 16x16        0x00058
		// 32x32        0x00158
		// 64x64        0x00558
		// 128x128      0x01558
		// 256x256      0x05558
		// 512x512      0x15558
		// 1024x1024    0x55558

		// 8BPP palette textures
		// Texture size Byte offset value for starting address
		// 1x1          0x00003
		// 2x2          0x00004
		// 4x4          0x00008
		// 8x8          0x00018
		// 16x16        0x00058
		// 32x32        0x00158
		// 64x64        0x00558
		// 128x128      0x01558
		// 256x256      0x05558
		// 512x512      0x15558
		// 1024x1024    0x55558

		// Non-palette textures
		// Texture size Byte offset value for starting address
		// 1x1          0x00006
		// 2x2          0x00008
		// 4x4          0x00010
		// 8x8          0x00030
		// 16x16        0x000B0
		// 32x32        0x002B0
		// 64x64        0x00AB0
		// 128x128      0x02AB0
		// 256x256      0x0AAB0
		// 512x512      0x2AAB0
		// 1024x1024    0xAAAB0

		// VQ textures
		// Texture size Byte offset value for starting address
		// 1x1          0x00000
		// 2x2          0x00001
		// 4x4          0x00002
		// 8x8          0x00006
		// 16x16        0x00016
		// 32x32        0x00056
		// 64x64        0x00156
		// 128x128      0x00556
		// 256x256      0x01556
		// 512x512      0x05556
		// 1024x1024    0x15556

		static const int mipmap_4_8_offset[8] = { 0x00018, 0x00058, 0x00158, 0x00558, 0x01558, 0x05558, 0x15558, 0x55558 };  // 4bpp (4bit offset) / 8bpp (8bit offset)
		static const int mipmap_np_offset[8] =  { 0x00030, 0x000B0, 0x002B0, 0x00AB0, 0x02AB0, 0x0AAB0, 0x2AAB0, 0xAAAB0 };  // nonpalette textures
		static const int mipmap_vq_offset[8] =  { 0x00006, 0x00016, 0x00056, 0x00156, 0x00556, 0x01556, 0x05556, 0x15556 }; // vq textures

		switch (miptype)
		{

			case 0: // 4bpp
				//printf("4bpp\n");
				t->address += mipmap_4_8_offset[t->sizes&7]>>1;
				break;

			case 1: // 8bpp
				//printf("8bpp\n");
				t->address += mipmap_4_8_offset[t->sizes&7];
				break;

			case 2: // nonpalette
				//printf("np\n");
				t->address += mipmap_np_offset[t->sizes&7];
				break;

			case 3: // vq
				//printf("vq\n");
				t->address += mipmap_vq_offset[t->sizes&7];
				break;
		}
	}

}

static void tex_get_info(texinfo *ti, pvrta_state *sa)
{
	ti->address = sa->textureaddress;
	ti->sizex   = sa->textureusize;
	ti->sizey   = sa->texturevsize;
	ti->mode    = sa->scanorder + sa->vqcompressed*2;
	ti->sizes   = sa->texturesizes;
	ti->pf      = sa->pixelformat;
	ti->mipmapped  = sa->mipmapped;
	ti->palette = sa->paletteselector;
	tex_prepare(ti);
}

// register decode helper
INLINE int decode_reg_64(UINT32 offset, UINT64 mem_mask, UINT64 *shift)
{
	int reg = offset * 2;

	*shift = 0;

	// non 32-bit accesses have not yet been seen here, we need to know when they are
	if ((mem_mask != U64(0xffffffff00000000)) && (mem_mask != U64(0x00000000ffffffff)))
	{
		/*assume to return the lower 32-bits ONLY*/
		return reg & 0xffffffff;
	}

	if (mem_mask == U64(0xffffffff00000000))
	{
		reg++;
		*shift = 32;
 	}

	return reg;
}

READ64_HANDLER( pvr_ctrl_r )
{
	int reg;
	UINT64 shift;

	reg = decode_reg_64(offset, mem_mask, &shift);

	#if DEBUG_PVRCTRL
	mame_printf_verbose("PVRCTRL: [%08x] read %x @ %x (reg %x), mask %llx (PC=%x)\n", 0x5f7c00+reg*4, pvrctrl_regs[reg], offset, reg, mem_mask, cpu_get_pc(space->cpu));
	#endif

	return (UINT64)pvrctrl_regs[reg] << shift;
}

WRITE64_HANDLER( pvr_ctrl_w )
{
	int reg;
	UINT64 shift;
	UINT32 dat;
	static struct {
		UINT32 pvr_addr;
		UINT32 sys_addr;
		UINT32 size;
		UINT8 sel;
		UINT8 dir;
		UINT8 flag;
		UINT8 start;
	}pvr_dma;

	reg = decode_reg_64(offset, mem_mask, &shift);
	dat = (UINT32)(data >> shift);

	switch (reg)
	{
		case SB_PDSTAP: pvr_dma.pvr_addr = dat; break;
		case SB_PDSTAR: pvr_dma.sys_addr = dat; break;
		case SB_PDLEN: pvr_dma.size = dat; break;
		case SB_PDDIR: pvr_dma.dir = dat & 1; break;
		case SB_PDTSEL:
			pvr_dma.sel = dat & 1;
			if(pvr_dma.sel & 1)
				printf("Warning: Unsupported irq mode trigger PVR-DMA\n");
			break;
		case SB_PDEN: pvr_dma.flag = dat & 1; break;
		case SB_PDST:
			pvr_dma.start = dat & 1;

			if(pvr_dma.flag && pvr_dma.start)
			{
				UINT32 src,dst,size;
				dst = pvr_dma.pvr_addr;
				src = pvr_dma.sys_addr;
				size = 0;

				/* used by usagui and sprtjam*/
				printf("PVR-DMA start\n");
				printf("%08x %08x %08x\n",pvr_dma.pvr_addr,pvr_dma.sys_addr,pvr_dma.size);
				printf("src %s dst %08x\n",pvr_dma.dir ? "->" : "<-",pvr_dma.sel);

				/* 0 rounding size = 16 Mbytes */
				if(pvr_dma.size == 0) { pvr_dma.size = 0x100000; }

				if(pvr_dma.dir == 0)
				{
					for(;size<pvr_dma.size;size+=4)
					{
						memory_write_dword_64le(space,dst,memory_read_dword(space,src));
						src+=4;
						dst+=4;
					}
				}
				else
				{
					for(;size<pvr_dma.size;size+=4)
					{
						memory_write_dword_64le(space,src,memory_read_dword(space,dst));
						src+=4;
						dst+=4;
					}
				}
				/*Note: do not update the params, since this DMA type doesn't support it. */

				dc_sysctrl_regs[SB_ISTNRM] |= IST_DMA_PVR;
				dc_update_interrupt_status(space->machine);
			}
			break;
	}

	#if DEBUG_PVRCTRL
	mame_printf_verbose("PVRCTRL: [%08x=%x] write %llx to %x (reg %x), mask %llx\n", 0x5f7c00+reg*4, dat, data>>shift, offset, reg, mem_mask);
	#endif

	pvrctrl_regs[reg] |= dat;
}

READ64_HANDLER( pvr_ta_r )
{
	int reg;
	UINT64 shift;

	reg = decode_reg_64(offset, mem_mask, &shift);

	switch (reg)
	{
	case SPG_STATUS:
		pvrta_regs[reg] = (video_screen_get_vblank(space->machine->primary_screen) << 13) | (video_screen_get_hblank(space->machine->primary_screen) << 12) | (video_screen_get_vpos(space->machine->primary_screen) & 0x3ff);
		break;
	}

	#if DEBUG_PVRTA_REGS
	if (reg != 0x43)
		mame_printf_verbose("PVRTA: [%08x] read %x @ %x (reg %x), mask %llx (PC=%x)\n", 0x5f8000+reg*4, pvrta_regs[reg], offset, reg, mem_mask, cpu_get_pc(space->cpu));
	#endif
	return (UINT64)pvrta_regs[reg] << shift;
}

WRITE64_HANDLER( pvr_ta_w )
{
	int reg;
	UINT64 shift;
	UINT32 old,dat;
	#if DEBUG_PVRTA
	UINT32 sizera,offsetra,v;
	#endif
	int a;

	reg = decode_reg_64(offset, mem_mask, &shift);
	dat = (UINT32)(data >> shift);
	old = pvrta_regs[reg];
	pvrta_regs[reg] = dat; // 5f8000+reg*4=dat

	switch (reg)
	{
	case SOFTRESET:
		if (dat & 1)
		{
			#if DEBUG_PVRTA
			mame_printf_verbose("pvr_ta_w:  TA soft reset\n");
			#endif
			state_ta.listtype_used=0;
		}
		if (dat & 2)
		{

			#if DEBUG_PVRTA
			mame_printf_verbose("pvr_ta_w:  Core Pipeline soft reset\n");
			#endif
			if (state_ta.start_render_received == 1)
			{
				for (a=0;a < NUM_BUFFERS;a++)
					if (state_ta.grab[a].busy == 1)
						state_ta.grab[a].busy = 0;
				state_ta.start_render_received = 0;
			}
		}
		if (dat & 4)
		{
			#if DEBUG_PVRTA
			mame_printf_verbose("pvr_ta_w:  sdram I/F soft reset\n");
			#endif
		}
		break;
	case STARTRENDER:
		#if DEBUG_PVRTA
		mame_printf_verbose("Start Render Received:\n");
		mame_printf_verbose("  Region Array at %08x\n",pvrta_regs[REGION_BASE]);
		mame_printf_verbose("  ISP/TSP Parameters at %08x\n",pvrta_regs[PARAM_BASE]);
		if (pvrta_regs[FPU_PARAM_CFG] & 0x200000)
			sizera=6;
		else
			sizera=5;
		offsetra=pvrta_regs[REGION_BASE];
		for (;;)
		{
			v=memory_read_dword(space,0x05000000+offsetra);
			mame_printf_verbose("Tile X:%d Y:%d\n  ", (v >> 2) & 0x3f, (v >> 8) & 0x3f);
			offsetra = offsetra+4;
			v=memory_read_dword(space,0x05000000+offsetra);
			if (!(v & 0x80000000))
				mame_printf_verbose("OLP %d ",v & 0xFFFFFC);
			offsetra = offsetra+4;
			v=memory_read_dword(space,0x05000000+offsetra);
			if (!(v & 0x80000000))
				mame_printf_verbose("OMVLP %d ",v & 0xFFFFFC);
			offsetra = offsetra+4;
			v=memory_read_dword(space,0x05000000+offsetra);
			if (!(v & 0x80000000))
				mame_printf_verbose("TLP %d ",v & 0xFFFFFC);
			offsetra = offsetra+4;
			v=memory_read_dword(space,0x05000000+offsetra);
			if (!(v & 0x80000000))
				mame_printf_verbose("TMVLP %d ",v & 0xFFFFFC);
			if (sizera == 6)
			{
				offsetra = offsetra+4;
				v=memory_read_dword(space,0x05000000+offsetra);
				if (!(v & 0x80000000))
					mame_printf_verbose("PTLP %d ",v & 0xFFFFFC);
			}
			mame_printf_verbose("\n");
			if (v & 0x80000000)
				break;
		}
		#endif
		// select buffer to draw using PARAM_BASE
		for (a=0;a < NUM_BUFFERS;a++)
		{
			if ((state_ta.grab[a].ispbase == pvrta_regs[PARAM_BASE]) && (state_ta.grab[a].valid == 1) && (state_ta.grab[a].busy == 0))
			{
				rectangle clip;

				state_ta.grab[a].busy = 1;
				state_ta.renderselect = a;
				state_ta.start_render_received=1;


				state_ta.grab[a].fbwsof1=pvrta_regs[FB_W_SOF1];
				state_ta.grab[a].fbwsof2=pvrta_regs[FB_W_SOF2];

				clip.min_x = 0;
				clip.max_x = 1023;
				clip.min_y = 0;
				clip.max_y = 1023;

				// we've got a request to draw, so, draw to the fake fraembuffer!
				testdrawscreen(space->machine,fakeframebuffer_bitmap,&clip);

				timer_adjust_oneshot(endofrender_timer_isp, ATTOTIME_IN_USEC(4000) , 0); // hack, make sure render takes some amount of time

				break;
			}
		}
		if (a != NUM_BUFFERS)
			break;
		assert_always(0, "TA grabber error A!\n");
		break;
	case TA_LIST_INIT:
		state_ta.tafifo_pos=0;
		state_ta.tafifo_mask=7;
		state_ta.tafifo_vertexwords=8;
		state_ta.tafifo_listtype= -1;
	#if DEBUG_PVRTA
		mame_printf_verbose("TA_OL_BASE       %08x TA_OL_LIMIT  %08x\n", pvrta_regs[TA_OL_BASE], pvrta_regs[TA_OL_LIMIT]);
		mame_printf_verbose("TA_ISP_BASE      %08x TA_ISP_LIMIT %08x\n", pvrta_regs[TA_ISP_BASE], pvrta_regs[TA_ISP_LIMIT]);
		mame_printf_verbose("TA_ALLOC_CTRL    %08x\n", pvrta_regs[TA_ALLOC_CTRL]);
		mame_printf_verbose("TA_NEXT_OPB_INIT %08x\n", pvrta_regs[TA_NEXT_OPB_INIT]);
	#endif
		pvrta_regs[TA_NEXT_OPB] = pvrta_regs[TA_NEXT_OPB_INIT];
		pvrta_regs[TA_ITP_CURRENT] = pvrta_regs[TA_ISP_BASE];
		state_ta.alloc_ctrl_OPB_Mode = pvrta_regs[TA_ALLOC_CTRL] & 0x100000; // 0 up 1 down
		state_ta.alloc_ctrl_PT_OPB = (4 << ((pvrta_regs[TA_ALLOC_CTRL] >> 16) & 3)) & 0x38; // number of 32 bit words (0,8,16,32)
		state_ta.alloc_ctrl_TM_OPB = (4 << ((pvrta_regs[TA_ALLOC_CTRL] >> 12) & 3)) & 0x38;
		state_ta.alloc_ctrl_T_OPB = (4 << ((pvrta_regs[TA_ALLOC_CTRL] >> 8) & 3)) & 0x38;
		state_ta.alloc_ctrl_OM_OPB = (4 << ((pvrta_regs[TA_ALLOC_CTRL] >> 4) & 3)) & 0x38;
		state_ta.alloc_ctrl_O_OPB = (4 << ((pvrta_regs[TA_ALLOC_CTRL] >> 0) & 3)) & 0x38;
		state_ta.listtype_used |= (1+4);
		// use TA_ISP_BASE and select buffer for grab data
		state_ta.grabsel = -1;
		// try to find already used buffer but not busy
		for (a=0;a < NUM_BUFFERS;a++)
		{
			if ((state_ta.grab[a].ispbase == pvrta_regs[TA_ISP_BASE]) && (state_ta.grab[a].busy == 0) && (state_ta.grab[a].valid == 1))
			{
				state_ta.grabsel=a;
				break;
			}
		}
		// try a buffer not used yet
		if (state_ta.grabsel < 0)
		{
			for (a=0;a < NUM_BUFFERS;a++)
			{
				if (state_ta.grab[a].valid == 0)
				{
					state_ta.grabsel=a;
					break;
				}
			}
		}
		// find a non busy buffer starting from the last one used
		if (state_ta.grabsel < 0)
		{
			for (a=0;a < 3;a++)
			{
				if (state_ta.grab[(state_ta.grabsellast+1+a) & 3].busy == 0)
				{
					state_ta.grabsel=a;
					break;
				}
			}
		}
		if (state_ta.grabsel < 0)
			assert_always(0, "TA grabber error B!\n");
		state_ta.grabsellast=state_ta.grabsel;
		state_ta.grab[state_ta.grabsel].ispbase=pvrta_regs[TA_ISP_BASE];
		state_ta.grab[state_ta.grabsel].busy=0;
		state_ta.grab[state_ta.grabsel].valid=1;
		state_ta.grab[state_ta.grabsel].verts_size=0;
		state_ta.grab[state_ta.grabsel].strips_size=0;
		break;
	case TA_LIST_CONT:
	#if DEBUG_PVRTA
		mame_printf_verbose("List continuation processing\n");
	#endif
		state_ta.tafifo_listtype= -1; // no list being received
		state_ta.listtype_used |= (1+4);
		break;
	}

	#if DEBUG_PVRTA_REGS
	if ((reg != 0x14) && (reg != 0x15))
		mame_printf_verbose("PVRTA: [%08x=%x] write %llx to %x (reg %x %x), mask %llx\n", 0x5f8000+reg*4, dat, data>>shift, offset, reg, (reg*4)+0x8000, mem_mask);
	#endif
}
void process_ta_fifo(running_machine* machine)
{
	UINT32 a;

	/* first byte in the buffer is the Parameter Control Word

     pppp pppp gggg gggg oooo oooo oooo oooo

     p = para control
     g = group control
     o = object control

    */

	receiveddata *rd = &state_ta.grab[state_ta.grabsel];

	// Para Control
	state_ta.paracontrol=(tafifo_buff[0] >> 24) & 0xff;
	// 0 end of list
	// 1 user tile clip
	// 2 object list set
	// 3 reserved
	// 4 polygon/modifier volume
	// 5 sprite
	// 6 reserved
	// 7 vertex
	state_ta.paratype=(state_ta.paracontrol >> 5) & 7;
	state_ta.endofstrip=(state_ta.paracontrol >> 4) & 1;
	state_ta.listtype=(state_ta.paracontrol >> 0) & 7;
	if ((state_ta.paratype >= 4) && (state_ta.paratype <= 6))
	{
		state_ta.global_paratype = state_ta.paratype;
		// Group Control
		state_ta.groupcontrol=(tafifo_buff[0] >> 16) & 0xff;
		state_ta.groupen=(state_ta.groupcontrol >> 7) & 1;
		state_ta.striplen=(state_ta.groupcontrol >> 2) & 3;
		state_ta.userclip=(state_ta.groupcontrol >> 0) & 3;
		// Obj Control
		state_ta.objcontrol=(tafifo_buff[0] >> 0) & 0xffff;
		state_ta.shadow=(state_ta.objcontrol >> 7) & 1;
		state_ta.volume=(state_ta.objcontrol >> 6) & 1;
		state_ta.coltype=(state_ta.objcontrol >> 4) & 3;
		state_ta.texture=(state_ta.objcontrol >> 3) & 1;
		state_ta.offfset=(state_ta.objcontrol >> 2) & 1;
		state_ta.gouraud=(state_ta.objcontrol >> 1) & 1;
		state_ta.uv16bit=(state_ta.objcontrol >> 0) & 1;
	}

	// check if we need 8 words more
	if (state_ta.tafifo_mask == 7)
	{
		state_ta.parameterconfig = pvr_parameterconfig[state_ta.objcontrol & 0x3d];
		// decide number of words per vertex
		if (state_ta.paratype == 7)
		{
			if ((state_ta.global_paratype == 5) || (state_ta.tafifo_listtype == 1) || (state_ta.tafifo_listtype == 3))
				state_ta.tafifo_vertexwords = 16;
			if (state_ta.tafifo_vertexwords == 16)
			{
				state_ta.tafifo_mask = 15;
				state_ta.tafifo_pos = 8;
				return;
			}
		}
		// decide number of words when not a vertex
		state_ta.tafifo_vertexwords=pvr_wordsvertex[state_ta.parameterconfig];
		if ((state_ta.paratype == 4) && ((state_ta.listtype != 1) && (state_ta.listtype != 3)))
			if (pvr_wordspolygon[state_ta.parameterconfig] == 16)
			{
				state_ta.tafifo_mask = 15;
				state_ta.tafifo_pos = 8;
				return;
			}
	}
	state_ta.tafifo_mask = 7;

	// now we heve all the needed words
	// here we should generate the data for the various tiles
	// for now, just interpret their meaning
	if (state_ta.paratype == 0)
	{ // end of list
		#if DEBUG_PVRDLIST
		mame_printf_verbose("Para Type 0 End of List\n");
		#endif
		a=0; // 6-10 0-3
		switch (state_ta.tafifo_listtype)
		{
		case 0:
			a = 1 << 7;
			break;
		case 1:
			a = 1 << 8;
			break;
		case 2:
			a = 1 << 9;
			break;
		case 3:
			a = 1 << 10;
			break;
		case 4:
			a = 1 << 21;
			break;
		}

		dc_sysctrl_regs[SB_ISTNRM] |= a;
		dc_update_interrupt_status(machine);
		state_ta.tafifo_listtype= -1; // no list being received
		state_ta.listtype_used |= (2+8);
	}
	else if (state_ta.paratype == 1)
	{ // user tile clip
		#if DEBUG_PVRDLIST
		mame_printf_verbose("Para Type 1 User Tile Clip\n");
		mame_printf_verbose(" (%d , %d)-(%d , %d)\n", tafifo_buff[4], tafifo_buff[5], tafifo_buff[6], tafifo_buff[7]);
		#endif
	}
	else if (state_ta.paratype == 2)
	{ // object list set
		#if DEBUG_PVRDLIST
		mame_printf_verbose("Para Type 2 Object List Set at %08x\n", tafifo_buff[1]);
		mame_printf_verbose(" (%d , %d)-(%d , %d)\n", tafifo_buff[4], tafifo_buff[5], tafifo_buff[6], tafifo_buff[7]);
		#endif
	}
	else if (state_ta.paratype == 3)
	{
		#if DEBUG_PVRDLIST
		mame_printf_verbose("Para Type %x Unknown!\n", tafifo_buff[0]);
		#endif
	}
	else
	{ // global parameter or vertex parameter
		#if DEBUG_PVRDLIST
		mame_printf_verbose("Para Type %d", state_ta.paratype);
		if (state_ta.paratype == 7)
			mame_printf_verbose(" End of Strip %d", state_ta.endofstrip);
		if (state_ta.listtype_used & 3)
			mame_printf_verbose(" List Type %d", state_ta.listtype);
		mame_printf_verbose("\n");
		#endif

		// set type of list currently being recieved
		if ((state_ta.paratype == 4) || (state_ta.paratype == 5) || (state_ta.paratype == 6))
		{
			if (state_ta.tafifo_listtype < 0)
			{
				state_ta.tafifo_listtype = state_ta.listtype;
			}
		}
		state_ta.listtype_used = state_ta.listtype_used ^ (state_ta.listtype_used & 3);

		if ((state_ta.paratype == 4) || (state_ta.paratype == 5))
		{ // quad or polygon
			state_ta.depthcomparemode=(tafifo_buff[1] >> 29) & 7;
			state_ta.cullingmode=(tafifo_buff[1] >> 27) & 3;
			state_ta.zwritedisable=(tafifo_buff[1] >> 26) & 1;
			state_ta.cachebypass=(tafifo_buff[1] >> 21) & 1;
			state_ta.dcalcctrl=(tafifo_buff[1] >> 20) & 1;
			state_ta.volumeinstruction=(tafifo_buff[1] >> 29) & 7;
			state_ta.textureusize=1 << (3+((tafifo_buff[2] >> 3) & 7));
			state_ta.texturevsize=1 << (3+(tafifo_buff[2] & 7));
			state_ta.texturesizes=tafifo_buff[2] & 0x3f;
			state_ta.srcalphainstr=(tafifo_buff[2] >> 29) & 7;
			state_ta.dstalphainstr=(tafifo_buff[2] >> 26) & 7;
			state_ta.srcselect=(tafifo_buff[2] >> 25) & 1;
			state_ta.dstselect=(tafifo_buff[2] >> 24) & 1;
			state_ta.fogcontrol=(tafifo_buff[2] >> 22) & 3;
			state_ta.colorclamp=(tafifo_buff[2] >> 21) & 1;
			state_ta.usealpha=(tafifo_buff[2] >> 20) & 1;
			state_ta.ignoretexalpha=(tafifo_buff[2] >> 19) & 1;
			state_ta.flipuv=(tafifo_buff[2] >> 17) & 3;
			state_ta.clampuv=(tafifo_buff[2] >> 15) & 3;
			state_ta.filtermode=(tafifo_buff[2] >> 13) & 3;
			state_ta.sstexture=(tafifo_buff[2] >> 12) & 1;
			state_ta.mmdadjust=(tafifo_buff[2] >> 8) & 1;
			state_ta.tsinstruction=(tafifo_buff[2] >> 6) & 3;
			if (state_ta.texture == 1)
			{
				state_ta.textureaddress=(tafifo_buff[3] & 0x1FFFFF) << 3;
				state_ta.scanorder=(tafifo_buff[3] >> 26) & 1;
				state_ta.pixelformat=(tafifo_buff[3] >> 27) & 7;
				state_ta.mipmapped=(tafifo_buff[3] >> 31) & 1;
				state_ta.vqcompressed=(tafifo_buff[3] >> 30) & 1;
				state_ta.strideselect=(tafifo_buff[3] >> 25) & 1;
				state_ta.paletteselector=(tafifo_buff[3] >> 21) & 0x3F;
				#if DEBUG_PVRDLIST
				mame_printf_verbose(" Texture %d x %d at %08x format %d\n", state_ta.textureusize, state_ta.texturevsize, (tafifo_buff[3] & 0x1FFFFF) << 3, state_ta.pixelformat);
				#endif
			}
			if (state_ta.paratype == 4)
			{ // polygon or mv
				if ((state_ta.tafifo_listtype == 1) || (state_ta.tafifo_listtype == 3))
				{
				#if DEBUG_PVRDLIST
					mame_printf_verbose(" Modifier Volume\n");
				#endif
				}
				else
				{
				#if DEBUG_PVRDLIST
					mame_printf_verbose(" Polygon\n");
				#endif
				}
			}
			if (state_ta.paratype == 5)
			{ // quad
				#if DEBUG_PVRDLIST
				mame_printf_verbose(" Sprite\n");
				#endif
			}
		}

		if (state_ta.paratype == 7)
		{ // vertex
			if ((state_ta.tafifo_listtype == 1) || (state_ta.tafifo_listtype == 3))
			{
				#if DEBUG_PVRDLIST
				mame_printf_verbose(" Vertex modifier volume");
				mame_printf_verbose(" A(%f,%f,%f) B(%f,%f,%f) C(%f,%f,%f)", u2f(tafifo_buff[1]), u2f(tafifo_buff[2]),
					u2f(tafifo_buff[3]), u2f(tafifo_buff[4]), u2f(tafifo_buff[5]), u2f(tafifo_buff[6]), u2f(tafifo_buff[7]),
					u2f(tafifo_buff[8]), u2f(tafifo_buff[9]));
				mame_printf_verbose("\n");
				#endif
			}
			else if (state_ta.global_paratype == 5)
			{
				#if DEBUG_PVRDLIST
				mame_printf_verbose(" Vertex sprite");
				mame_printf_verbose(" A(%f,%f,%f) B(%f,%f,%f) C(%f,%f,%f) D(%f,%f,)", u2f(tafifo_buff[1]), u2f(tafifo_buff[2]),
					u2f(tafifo_buff[3]), u2f(tafifo_buff[4]), u2f(tafifo_buff[5]), u2f(tafifo_buff[6]), u2f(tafifo_buff[7]),
					u2f(tafifo_buff[8]), u2f(tafifo_buff[9]), u2f(tafifo_buff[10]), u2f(tafifo_buff[11]));
				mame_printf_verbose("\n");
				#endif
				if (state_ta.texture == 1)
				{
					if (rd->verts_size <= 65530)
					{
						strip *ts;
						vert *tv = &rd->verts[rd->verts_size];
						tv[0].x = u2f(tafifo_buff[0x1]);
						tv[0].y = u2f(tafifo_buff[0x2]);
						tv[0].w = u2f(tafifo_buff[0x3]);
						tv[1].x = u2f(tafifo_buff[0x4]);
						tv[1].y = u2f(tafifo_buff[0x5]);
						tv[1].w = u2f(tafifo_buff[0x6]);
						tv[3].x = u2f(tafifo_buff[0x7]);
						tv[3].y = u2f(tafifo_buff[0x8]);
						tv[3].w = u2f(tafifo_buff[0x9]);
						tv[2].x = u2f(tafifo_buff[0xa]);
						tv[2].y = u2f(tafifo_buff[0xb]);
						tv[2].w = tv[0].w+tv[3].w-tv[1].w;
						tv[0].u = u2f(tafifo_buff[0xd] & 0xffff0000);
						tv[0].v = u2f(tafifo_buff[0xd] << 16);
						tv[1].u = u2f(tafifo_buff[0xe] & 0xffff0000);
						tv[1].v = u2f(tafifo_buff[0xe] << 16);
						tv[3].u = u2f(tafifo_buff[0xf] & 0xffff0000);
						tv[3].v = u2f(tafifo_buff[0xf] << 16);
						tv[2].u = tv[0].u+tv[3].u-tv[1].u;
						tv[2].v = tv[0].v+tv[3].v-tv[1].v;

						ts = &rd->strips[rd->strips_size++];
						tex_get_info(&ts->ti, &state_ta);
						ts->svert = rd->verts_size;
						ts->evert = rd->verts_size + 3;

						rd->verts_size += 4;
					}
				}
			}
			else if (state_ta.global_paratype == 4)
			{
				#if DEBUG_PVRDLIST
				mame_printf_verbose(" Vertex polygon");
				mame_printf_verbose(" V(%f,%f,%f) T(%f,%f)", u2f(tafifo_buff[1]), u2f(tafifo_buff[2]), u2f(tafifo_buff[3]), u2f(tafifo_buff[4]), u2f(tafifo_buff[5]));
				mame_printf_verbose("\n");
				#endif
				if (rd->verts_size <= 65530)
				{
					/* add a vertex to our list */
					/* this is used for 3d stuff, ie most of the graphics (see guilty gear, confidential mission, maze of the kings etc.) */
					/* -- this is also wildly inaccurate! */
					vert *tv = &rd->verts[rd->verts_size];

					tv->x=u2f(tafifo_buff[1]);
					tv->y=u2f(tafifo_buff[2]);
					tv->w=u2f(tafifo_buff[3]);
					tv->u=u2f(tafifo_buff[4]);
					tv->v=u2f(tafifo_buff[5]);

					if((!rd->strips_size) ||
					   rd->strips[rd->strips_size-1].evert != -1)
					{
						strip *ts = &rd->strips[rd->strips_size++];
						tex_get_info(&ts->ti, &state_ta);
						ts->svert = rd->verts_size;
						ts->evert = -1;
					}
					if(state_ta.endofstrip)
						rd->strips[rd->strips_size-1].evert = rd->verts_size;
					rd->verts_size++;
				}
			}
		}
	}
}

WRITE64_HANDLER( ta_fifo_poly_w )
{

	if (mem_mask == U64(0xffffffffffffffff)) 	// 64 bit
	{
		tafifo_buff[state_ta.tafifo_pos]=(UINT32)data;
		tafifo_buff[state_ta.tafifo_pos+1]=(UINT32)(data >> 32);
		#if DEBUG_FIFO_POLY
		mame_printf_debug("ta_fifo_poly_w:  Unmapped write64 %08x = %llx -> %08x %08x\n", 0x10000000+offset*8, data, tafifo_buff[state_ta.tafifo_pos], tafifo_buff[state_ta.tafifo_pos+1]);
		#endif
		state_ta.tafifo_pos += 2;
	}
	else
	{
		fatalerror("ta_fifo_poly_w:  Only 64 bit writes supported!\n");
	}

	state_ta.tafifo_pos &= state_ta.tafifo_mask;

	// if the command is complete, process it
	if (state_ta.tafifo_pos == 0)
		process_ta_fifo(space->machine);

}

WRITE64_HANDLER( ta_fifo_yuv_w )
{
	int reg;
	UINT64 shift;
	UINT32 dat;

	reg = decode_reg_64(offset, mem_mask, &shift);
	dat = (UINT32)(data >> shift);

	mame_printf_verbose("YUV FIFO: [%08x=%x] write %llx to %x, mask %llx\n", 0x10800000+reg*4, dat, data, offset, mem_mask);
}

/* test video start */
static UINT32 dilate0(UINT32 value,int bits) // dilate first "bits" bits in "value"
{
	UINT32 x,m1,m2,m3;
	int a;

	x = value;
	for (a=0;a < bits;a++)
	{
		m2 = 1 << (a << 1);
		m1 = m2 - 1;
		m3 = (~m1) << 1;
		x = (x & m1) + (x & m2) + ((x & m3) << 1);
	}
	return x;
}

static UINT32 dilate1(UINT32 value,int bits) // dilate first "bits" bits in "value"
{
	UINT32 x,m1,m2,m3;
	int a;

	x = value;
	for (a=0;a < bits;a++)
	{
		m2 = 1 << (a << 1);
		m1 = m2 - 1;
		m3 = (~m1) << 1;
		x = (x & m1) + ((x & m2) << 1) + ((x & m3) << 1);
	}
	return x;
}

static void computedilated(void)
{
	int a,b;

	for (b=0;b <= 14;b++)
		for (a=0;a < 1024;a++) {
			dilated0[b][a]=dilate0(a,b);
			dilated1[b][a]=dilate1(a,b);
		}
	for (b=0;b <= 7;b++)
		for (a=0;a < 7;a++)
			dilatechose[(b << 3) + a]=3+(a < b ? a : b);
}

void render_hline(bitmap_t *bitmap, texinfo *ti, int y, float xl, float xr, float ul, float ur, float vl, float vr, float wl, float wr)
{
	int xxl, xxr;
	float dx, dudx, dvdx, dwdx;
	UINT32 *tdata;
	float *wbufline;

	if(xr < 0 || xl >= 640)
		return;

	xxl = (int)xl;
	xxr = (int)xr;

	if(xxl == xxr)
		return;

	dx = xr-xl;
	dudx = (ur-ul)/dx;
	dvdx = (vr-vl)/dx;
	dwdx = (wr-wl)/dx;

	if(xl < 0) {
		ul += -dudx*xl;
		vl += -dvdx*xl;
		wl += -dwdx*xl;
		xxl = 0;
	} else {
		float dt = xxl - xl;
		ul += dudx*dt;
		vl += dvdx*dt;
		wl += dwdx*dt;
	}

	// Target the pixel center
	ul += 0.5*dudx;
	vl += 0.5*dvdx;
	wl += 0.5*dwdx;

	if(xxr > 640)
		xxr = 640;

	tdata = BITMAP_ADDR32(bitmap, y, xxl);
	wbufline = &wbuffer[y][xxl];

	while(xxl < xxr) {
		if((wl > *wbufline)) {
			UINT32 c;
			float u = ul/wl;
			float v = vl/wl;

			c = ti->r(ti, u, v);

			if((c & 0xff000000) == 0xff000000) {
				*wbufline = wl;
				*tdata = c;
			} else if(c & 0xff000000) {
				int a = (c >> 24)+1;
				int ca = 256-a;
				UINT32 c2 = *tdata;
				*tdata = ((((c & 0xff00ff)*a + (c2 & 0xff00ff)*ca) & 0xff00ff00) |
						  (((c & 0xff00)*a + (c2 & 0xff00)*ca) & 0xff0000)) >> 8;
			}
		}
		wbufline++;
		tdata++;

		ul += dudx;
		vl += dvdx;
		wl += dwdx;
		xxl ++;
	}
}

void render_span(bitmap_t *bitmap, texinfo *ti,
                 int y0, int y1, float adj,
                 float *xl, float *xr,
                 float *ul, float *ur,
                 float *vl, float *vr,
                 float *wl, float *wr,
                 float dxldy, float dxrdy,
                 float duldy, float durdy,
                 float dvldy, float dvrdy,
                 float dwldy, float dwrdy)
{
	if(y1 > 480)
		y1 = 480;
	if(y1 <= 0) {
		*xl += dxldy*(y1-y0);
		*xr += dxrdy*(y1-y0);
		*ul += duldy*(y1-y0);
		*ur += durdy*(y1-y0);
		*vl += dvldy*(y1-y0);
		*vr += dvrdy*(y1-y0);
		*wl += dwldy*(y1-y0);
		*wr += dwrdy*(y1-y0);
		return;
	}
	if(y0 < 0) {
		*xl += -dxldy*y0;
		*xr += -dxrdy*y0;
		*ul += -duldy*y0;
		*ur += -durdy*y0;
		*vl += -dvldy*y0;
		*vr += -dvrdy*y0;
		*wl += -dwldy*y0;
		*wr += -dwrdy*y0;
		y0 = 0;
	}

	if(adj) {
		*xl += adj*dxldy;
		*xr += adj*dxrdy;
		*ul += adj*duldy;
		*ur += adj*durdy;
		*vl += adj*dvldy;
		*vr += adj*dvrdy;
		*wl += adj*dwldy;
		*wr += adj*dwrdy;
	}
	while(y0 < y1) {
		render_hline(bitmap, ti, y0, *xl, *xr, *ul, *ur, *vl, *vr, *wl, *wr);

		*xl += dxldy;
		*xr += dxrdy;
		*ul += duldy;
		*ur += durdy;
		*vl += dvldy;
		*vr += dvrdy;
		*wl += dwldy;
		*wr += dwrdy;
		y0 ++;
	}
}

static void sort_vertices(const vert *v, int *i0, int *i1, int *i2)
{
	float miny, maxy;
	int imin, imax, imid;
	miny = maxy = v[0].y;
	imin = imax = 0;

	if(miny > v[1].y) {
		miny = v[1].y;
		imin = 1;
	} else if(maxy < v[1].y) {
		maxy = v[1].y;
		imax = 1;
	}

	if(miny > v[2].y) {
		miny = v[2].y;
		imin = 2;
	} else if(maxy < v[2].y) {
		maxy = v[2].y;
		imax = 2;
	}

	imid = (imin == 0 || imax == 0) ? (imin == 1 || imax == 1) ? 2 : 1 : 0;

	*i0 = imin;
	*i1 = imid;
	*i2 = imax;
}


static void render_tri_sorted(bitmap_t *bitmap, texinfo *ti, const vert *v0, const vert *v1, const vert *v2)
{
	int y0, y1, y2;
	float dy01, dy02, dy12;
	float dy;

	float dx01dy, dx02dy, dx12dy, du01dy, du02dy, du12dy, dv01dy, dv02dy, dv12dy, dw01dy, dw02dy, dw12dy;

	float xl, xr, ul, ur, vl, vr, wl, wr;

	if(v0->y >= 480 || v2->y < 0)
		return;

	y0 = (int)(v0->y);
	y1 = (int)(v1->y);
	y2 = (int)(v2->y);

	dy01 = y1-y0;
	dy02 = y2-y0;
	dy12 = y2-y1;

	dx01dy = dy01 ? (v1->x-v0->x)/dy01 : 0;
	dx02dy = dy02 ? (v2->x-v0->x)/dy02 : 0;
	dx12dy = dy12 ? (v2->x-v1->x)/dy12 : 0;

	du01dy = dy01 ? (v1->u-v0->u)/dy01 : 0;
	du02dy = dy02 ? (v2->u-v0->u)/dy02 : 0;
	du12dy = dy12 ? (v2->u-v1->u)/dy12 : 0;

	dv01dy = dy01 ? (v1->v-v0->v)/dy01 : 0;
	dv02dy = dy02 ? (v2->v-v0->v)/dy02 : 0;
	dv12dy = dy12 ? (v2->v-v1->v)/dy12 : 0;

	dw01dy = dy01 ? (v1->w-v0->w)/dy01 : 0;
	dw02dy = dy02 ? (v2->w-v0->w)/dy02 : 0;
	dw12dy = dy12 ? (v2->w-v1->w)/dy12 : 0;

	// Target the pixel center
	dy = (y0-v0->y) + 0.5;

	xl = v0->x;
	xr = v0->x;
	ul = v0->u;
	ur = v0->u;
	vl = v0->v;
	vr = v0->v;
	wl = v0->w;
	wr = v0->w;

	if(!dy01) {
		if(!dy12)
			return;
		if(v1->x > v0->x) {
			xr = v1->x;
			ur = v1->u;
			vr = v1->v;
			wr = v1->w;
			render_span(bitmap, ti, y1, y2, dy, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx02dy, dx12dy, du02dy, du12dy, dv02dy, dv12dy, dw02dy, dw12dy);
		} else {
			xl = v1->x;
			ul = v1->u;
			vl = v1->v;
			wl = v1->w;
			render_span(bitmap, ti, y1, y2, dy, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx12dy, dx02dy, du12dy, du02dy, dv12dy, dv02dy, dw12dy, dw02dy);
		}
	} else if(!dy12) {
		if(v2->x > v1->x)
			render_span(bitmap, ti, y0, y1, dy, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx01dy, dx02dy, du01dy, du02dy, dv01dy, dv02dy, dw01dy, dw02dy);
		else
			render_span(bitmap, ti, y0, y1, dy, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx02dy, dx01dy, du02dy, du01dy, dv02dy, dv01dy, dw02dy, dw01dy);
	} else {
		if(dx01dy < dx02dy) {
			render_span(bitmap, ti, y0, y1, dy, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx01dy, dx02dy, du01dy, du02dy, dv01dy, dv02dy, dw01dy, dw02dy);
			render_span(bitmap, ti, y1, y2,  0, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx12dy, dx02dy, du12dy, du02dy, dv12dy, dv02dy, dw12dy, dw02dy);
		} else {
			render_span(bitmap, ti, y0, y1, dy, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx02dy, dx01dy, du02dy, du01dy, dv02dy, dv01dy, dw02dy, dw01dy);
			render_span(bitmap, ti, y1, y2,  0, &xl, &xr, &ul, &ur, &vl, &vr, &wl, &wr, dx02dy, dx12dy, du02dy, du12dy, dv02dy, dv12dy, dw02dy, dw12dy);
		}
	}
}

static void render_tri(bitmap_t *bitmap, texinfo *ti, const vert *v)
{
	int i0, i1, i2;

	sort_vertices(v, &i0, &i1, &i2);
	render_tri_sorted(bitmap, ti, v+i0, v+i1, v+i2);
}

static void testdrawscreen(const running_machine *machine,bitmap_t *bitmap,const rectangle *cliprect)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int cs,rs,ns;
	UINT32 c;
#if 0
	int stride;
	UINT16 *bmpaddr16;
	UINT32 k;
#endif


	if (state_ta.renderselect < 0)
		return;

	//printf("drawtest!\n");

	rs=state_ta.renderselect;
	c=pvrta_regs[ISP_BACKGND_T];
	c=memory_read_dword(space,0x05000000+((c&0xfffff8)>>1)+(3+3)*4);
	bitmap_fill(bitmap,cliprect,c);
#if 0
	stride=pvrta_regs[FB_W_LINESTRIDE] << 3;
	c=pvrta_regs[ISP_BACKGND_T];
	a=(c & 0xfffff8) >> 1;
	cs=(c >> 24) & 7;
	cs=cs+3; // cs*2+3
	c=memory_read_dword(space,0x05000000+a+3*4+(cs-1)*4);
	dx=(int)u2f(memory_read_dword(space,0x05000000+a+3*4+cs*4+0));
	dy=(int)u2f(memory_read_dword(space,0x05000000+a+3*4+2*cs*4+4));
	for (y=0;y < dy;y++)
	{
		addrp=state_ta.grab[rs].fbwsof1+y*stride;
		for (x=0;x < dx;x++)
		{
			bmpaddr16=((UINT16 *)dc_texture_ram) + (WORD2_XOR_LE(addrp) >> 1);
			*bmpaddr16=((c & 0xf80000) >> 8) | ((c & 0xfc00) >> 5) | ((c & 0xf8) >> 3);
			addrp+=2;
		}
	}
#endif

	ns=state_ta.grab[rs].strips_size;
	if(ns)
		memset(wbuffer, 0x00, sizeof(wbuffer));

	for (cs=0;cs < ns;cs++)
	{
		strip *ts = &state_ta.grab[rs].strips[cs];
		int sv = ts->svert;
		int ev = ts->evert;
		int i;
		if(ev == -1)
			continue;

		for(i=sv; i <= ev; i++)
		{
			vert *tv = state_ta.grab[rs].verts + i;
			tv->u = tv->u * ts->ti.sizex * tv->w;
			tv->v = tv->v * ts->ti.sizey * tv->w;
		}

		for(i=sv; i <= ev-2; i++)
			render_tri(bitmap, &ts->ti, state_ta.grab[rs].verts + i);
	}
	state_ta.grab[rs].busy=0;
}

#if 0
static void testdrawscreenframebuffer(bitmap_t *bitmap,const rectangle *cliprect)
{
	int x,y,dy,xi;
	UINT32 addrp;
	UINT32 *fbaddr;
	UINT32 c;
	UINT32 r,g,b;

	// only for rgb565 framebuffer
	xi=((pvrta_regs[FB_R_SIZE] & 0x3ff)+1) << 1;
	dy=((pvrta_regs[FB_R_SIZE] >> 10) & 0x3ff)+1;
	for (y=0;y < dy;y++)
	{
		addrp=pvrta_regs[FB_R_SOF1]+y*xi*2;
		for (x=0;x < xi;x++)
		{
			fbaddr=BITMAP_ADDR32(bitmap,y,x);
			c=*(((UINT16 *)dc_texture_ram) + (WORD2_XOR_LE(addrp) >> 1));
			b = (c & 0x001f) << 3;
			g = (c & 0x07e0) >> 3;
			r = (c & 0xf800) >> 8;
			*fbaddr = b | (g<<8) | (r<<16);
			addrp+=2;
		}
	}
}
#endif

#if DEBUG_PALRAM
static void debug_paletteram(running_machine *machine)
{
	UINT64 pal;
	UINT32 r,g,b;
	int i;

	//popmessage("%02x",pvrta_regs[PAL_RAM_CTRL]);

	for(i=0;i<0x400;i++)
	{
		pal = pvrta_regs[((0x005F9000-0x005F8000)/4)+i];
		switch(pvrta_regs[PAL_RAM_CTRL])
		{
			case 0: //argb1555 <- guilty gear uses this mode
			{
				//a = (pal & 0x8000)>>15;
				r = (pal & 0x7c00)>>10;
				g = (pal & 0x03e0)>>5;
				b = (pal & 0x001f)>>0;
				//a = a ? 0xff : 0x00;
				palette_set_color_rgb(machine, i, pal5bit(r), pal5bit(g), pal5bit(b));
			}
			break;
			case 1: //rgb565
			{
				//a = 0xff;
				r = (pal & 0xf800)>>11;
				g = (pal & 0x07e0)>>5;
				b = (pal & 0x001f)>>0;
				palette_set_color_rgb(machine, i, pal5bit(r), pal6bit(g), pal5bit(b));
			}
			break;
			case 2: //argb4444
			{
				//a = (pal & 0xf000)>>12;
				r = (pal & 0x0f00)>>8;
				g = (pal & 0x00f0)>>4;
				b = (pal & 0x000f)>>0;
				palette_set_color_rgb(machine, i, pal4bit(r), pal4bit(g), pal4bit(b));
			}
			break;
			case 3: //argb8888
			{
				//a = (pal & 0xff000000)>>20;
				r = (pal & 0x00ff0000)>>16;
				g = (pal & 0x0000ff00)>>8;
				b = (pal & 0x000000ff)>>0;
				palette_set_color_rgb(machine, i, r, g, b);
			}
			break;
		}
	}
}
#endif

/* test video end */

static void pvr_build_parameterconfig(void)
{
	int a,b,c,d,e,p;

	for (a = 0;a <= 63;a++)
		pvr_parameterconfig[a] = -1;
	p=0;
	// volume,col_type,texture,offset,16bit_uv
	for (a = 0;a <= 1;a++)
		for (b = 0;b <= 3;b++)
			for (c = 0;c <= 1;c++)
				if (c == 0)
				{
					for (d = 0;d <= 1;d++)
						for (e = 0;e <= 1;e++)
							pvr_parameterconfig[(a << 6) | (b << 4) | (c << 3) | (d << 2) | (e << 0)] = pvr_parconfseq[p];
					p++;
				}
				else
					for (d = 0;d <= 1;d++)
						for (e = 0;e <= 1;e++)
						{
							pvr_parameterconfig[(a << 6) | (b << 4) | (c << 3) | (d << 2) | (e << 0)] = pvr_parconfseq[p];
							p++;
						}
	for (a = 1;a <= 63;a++)
		if (pvr_parameterconfig[a] < 0)
			pvr_parameterconfig[a] = pvr_parameterconfig[a-1];
}

static TIMER_CALLBACK(vbout)
{
	UINT32 a;
	//printf("vbout\n");

	a=dc_sysctrl_regs[SB_ISTNRM] | IST_VBL_OUT;
	dc_sysctrl_regs[SB_ISTNRM] = a; // V Blank-out interrupt
	dc_update_interrupt_status(machine);

	scanline = 0;
	timer_adjust_oneshot(vbout_timer, attotime_never, 0);
	timer_adjust_oneshot(hbin_timer, video_screen_get_time_until_pos(machine->primary_screen, 0, 640), 0);
}

static TIMER_CALLBACK(hbin)
{
	dc_sysctrl_regs[SB_ISTNRM] |= IST_HBL_IN; // H Blank-in interrupt
	dc_update_interrupt_status(machine);

//  printf("hbin on scanline %d\n",scanline);

	scanline++;

	timer_adjust_oneshot(hbin_timer, video_screen_get_time_until_pos(machine->primary_screen, scanline, 640), 0);
}



static TIMER_CALLBACK(endofrender_video)
{
	dc_sysctrl_regs[SB_ISTNRM] |= IST_EOR_VIDEO;// VIDEO end of render
	dc_update_interrupt_status(machine);
	timer_adjust_oneshot(endofrender_timer_video, attotime_never, 0);
}

static TIMER_CALLBACK(endofrender_tsp)
{
	dc_sysctrl_regs[SB_ISTNRM] |= IST_EOR_TSP;	// TSP end of render
	dc_update_interrupt_status(machine);

	timer_adjust_oneshot(endofrender_timer_tsp, attotime_never, 0);
	timer_adjust_oneshot(endofrender_timer_video, ATTOTIME_IN_USEC(500) , 0);
}

static TIMER_CALLBACK(endofrender_isp)
{
	dc_sysctrl_regs[SB_ISTNRM] |= IST_EOR_ISP;	// ISP end of render
	dc_update_interrupt_status(machine);

	timer_adjust_oneshot(endofrender_timer_isp, attotime_never, 0);
	timer_adjust_oneshot(endofrender_timer_tsp, ATTOTIME_IN_USEC(500) , 0);
}


VIDEO_START(dc)
{
	memset(pvrctrl_regs, 0, sizeof(pvrctrl_regs));
	memset(pvrta_regs, 0, sizeof(pvrta_regs));
	memset(state_ta.grab, 0, sizeof(state_ta.grab));
	pvr_build_parameterconfig();

	// if the next 2 registers do not have the correct values, the naomi bios will hang
	pvrta_regs[PVRID]=0x17fd11db;
	pvrta_regs[REVISION]=0x11;
	pvrta_regs[VO_CONTROL]=0xC;
	pvrta_regs[SOFTRESET]=0x7;
	state_ta.tafifo_pos=0;
	state_ta.tafifo_mask=7;
	state_ta.tafifo_vertexwords=8;
	state_ta.tafifo_listtype= -1;
	state_ta.start_render_received=0;
	state_ta.renderselect= -1;
	state_ta.grabsel=0;

	computedilated();

	vbout_timer = timer_alloc(machine, vbout, 0);
	timer_adjust_oneshot(vbout_timer, attotime_never, 0);

	hbin_timer = timer_alloc(machine, hbin, 0);
	timer_adjust_oneshot(hbin_timer, attotime_never, 0);
	scanline = 0;

	endofrender_timer_isp = timer_alloc(machine, endofrender_isp, 0);
	endofrender_timer_tsp = timer_alloc(machine, endofrender_tsp, 0);
	endofrender_timer_video = timer_alloc(machine, endofrender_video, 0);

	timer_adjust_oneshot(endofrender_timer_isp, attotime_never, 0);
	timer_adjust_oneshot(endofrender_timer_tsp, attotime_never, 0);
	timer_adjust_oneshot(endofrender_timer_video, attotime_never, 0);

	fakeframebuffer_bitmap = auto_bitmap_alloc(machine,1024,1024,BITMAP_FORMAT_RGB32);

}

VIDEO_UPDATE(dc)
{
	/******************
      MAME note
    *******************

    The video update function should NOT be generating interrupts, setting timers or doing _anything_ the game might be able to detect
    as it will be called at different times depending on frameskip etc.

    Rendering should happen when the hardware requests it, to the framebuffer(s)

    Everything else should depend on timers.

    ******************/

//  static int useframebuffer=1;
	const rectangle *visarea = video_screen_get_visible_area(screen);
	int y,x;
	//printf("videoupdate\n");

#if DEBUG_PALRAM
	debug_paletteram(screen->machine);
#endif

	// copy our fake framebuffer bitmap (where things have been rendered) to the screen
	for (y = visarea->min_y ; y <= visarea->max_y ; y++)
	{
		for (x = visarea->min_x ; x <= visarea->max_x ; x++)
		{
			UINT32* src = BITMAP_ADDR32(fakeframebuffer_bitmap, y, x);
			UINT32* dst = BITMAP_ADDR32(bitmap, y, x);
			dst[0] = src[0];
		}
	}

	return 0;
}

void dc_vblank(running_machine *machine)
{
	//printf("vblankin\n");

	dc_sysctrl_regs[SB_ISTNRM] |= IST_VBL_IN; // V Blank-in interrupt
	dc_update_interrupt_status(machine);
	vblc++;

	timer_adjust_oneshot(vbout_timer, video_screen_get_time_until_pos(machine->primary_screen, 0, 0), 0);
}
