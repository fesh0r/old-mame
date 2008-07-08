/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "driver.h"
#include "includes/lynx.h"
#include "cpu/m6502/m6502.h"


UINT16 lynx_granularity = 1;
static int lynx_line;

typedef struct
{
    union
	{
		UINT8 data[0x100];
		struct
		{
			struct { UINT8 l, h; }
			// eng used by the blitter engine
			// scb written by the blitter scb blocks to engine expander
			//     used by the engine
			//     might be used directly by software!
			eng1, eng2,
			h_offset, v_offset, vidbas, colbas,
			eng3, eng4,
			scb1, scb2, scb3, scb4, scb5, scb6,
			eng5, eng6, eng7, eng8,
			colloff,
			eng9,
			hsizoff, vsizoff,
			eng10, eng11;
			UINT8 res[0x20];
			UINT8 used[2];
			UINT8 D,C,B,A,P,N;
			UINT8 res1[8];
			UINT8 H,G,F,E;
			UINT8 res2[8];
			UINT8 M,L,K,J;
			UINT8 res3[0x21];
			UINT8 SPRG0;
			UINT8 SPRSYS;
		} s;
    } u;
    int accumulate_overflow;
    UINT8 high;
    int low;
} SUZY;

static SUZY suzy;

static struct
{
    UINT8 *mem;
    // global
    UINT16 screen;
    UINT16 colbuf;
    UINT16 colpos; // byte where value of collision is written
    UINT16 xoff, yoff;
    // in command
    int mode;
    UINT16 cmd;
    UINT8 spritenr;
    int x,y;
    UINT16 width, height; // uint16 important for blue lightning
    UINT16 stretch, tilt; // uint16 important
    UINT8 color[16]; // or stored
    void (*line_function)(const int y, const int xdir);
    UINT16 bitmap;

    int everon;
    int memory_accesses;
    attotime time;
} blitter;

UINT8 *lynx_mem_0000;
UINT8 *lynx_mem_fc00;
UINT8 *lynx_mem_fd00;
UINT8 *lynx_mem_fe00;
UINT8 *lynx_mem_fffa;
size_t lynx_mem_fe00_size;

static UINT8 lynx_memory_config;

#define GET_WORD(mem, index) ((mem)[(index)]|((mem)[(index)+1]<<8))

/*
mode from blitter command
#define SHADOW         (0x07)
#define XORSHADOW      (0x06)
#define NONCOLLIDABLE  (0x05)
#define NORMAL         (0x04)
#define BOUNDARY       (0x03)
#define BOUNDARYSHADOW (0x02)
#define BKGRNDNOCOL    (0x01)
#define BKGRND         (0x00)
*/

INLINE void lynx_plot_pixel(const int mode, const int x, const int y, const int color)
{
    int back;
    UINT8 *screen;
    UINT8 *colbuf;

    blitter.everon=TRUE;
    screen=blitter.mem+blitter.screen+y*80+x/2;
    colbuf=blitter.mem+blitter.colbuf+y*80+x/2;
    switch (mode) {
    case 0x00: // background (shadow bug!)
	if (!(x&1)) {
	    *screen=(*screen&0x0f)|(color<<4);
	} else {
	    *screen=(*screen&0xf0)|color;
	}
	blitter.memory_accesses++;
	break;
    case 0x10:
	if (!(x&1)) {
	    if (color!=0xe) {
		*colbuf=(*colbuf&~0xf0)|(blitter.spritenr<<4);
		blitter.memory_accesses++;
	    }
	    *screen=(*screen&0x0f)|(color<<4);
	} else {
	    if (color!=0xe) {
		*colbuf=(*colbuf&~0xf)|(blitter.spritenr);
		blitter.memory_accesses++;
	    }
	    *screen=(*screen&0xf0)|color;
	}
	blitter.memory_accesses++;
	break;
    case 0x01: // background, no colliding
    case 0x11:
	if (!(x&1)) {
	    *screen=(*screen&0x0f)|(color<<4);
	} else {
	    *screen=(*screen&0xf0)|color;
	}
	blitter.memory_accesses++;
	break;
    case 0x03: // boundary: pen P? transparent, but collides
    case 0x02: // boundary, shadow
	if ((color==0)||(color==0xf)) break;
	if (!(x&1)) {
	    *screen=(*screen&0x0f)|(color<<4);
	} else {
	    *screen=(*screen&0xf0)|color;
	}
	blitter.memory_accesses++;
	break;
    case 0x13:
	if (color==0) break;
	if (!(x&1)) {
	    back=*colbuf;
	    if (back>>4>blitter.mem[blitter.colpos])
		blitter.mem[blitter.colpos]=back>>4;
	    *colbuf=(back&~0xf0)|(blitter.spritenr<<4);
	    if (color!=0xf) {
		*screen=(*screen&0x0f)|(color<<4);
		blitter.memory_accesses++;
	    }
	} else {
	    back=*colbuf;
	    if ((back&0xf)>blitter.mem[blitter.colpos])
		blitter.mem[blitter.colpos]=back&0xf;
	    *colbuf=(back&~0xf)|(blitter.spritenr);
	    if (color!=0xf) {
		*screen=(*screen&0xf0)|color;
		blitter.memory_accesses++;
	    }
	}
	blitter.memory_accesses+=2;
	break;
    case 0x12:
	if (color==0) break;
	if (!(x&1)) {
	    if (color!=0xe) {
		back=*colbuf;
		if (back>>4>blitter.mem[blitter.colpos])
		    blitter.mem[blitter.colpos]=back>>4;
		*colbuf=(back&~0xf0)|(blitter.spritenr<<4);
	    }
	    if (color!=0xf) {
		*screen=(*screen&0x0f)|(color<<4);
		blitter.memory_accesses++;
	    }
	} else {
	    if (color!=0xe) {
		back=*colbuf;
		if ((back&0xf)>blitter.mem[blitter.colpos])
		    blitter.mem[blitter.colpos]=back&0xf;
		*colbuf=(back&~0xf)|(blitter.spritenr);
	    }
	    if (color!=0xf) {
		*screen=(*screen&0xf0)|color;
		blitter.memory_accesses++;
	    }
	}
	blitter.memory_accesses+=2;
	break;
    case 0x04: // pen 0 transparent,
    case 0x07: // shadow: pen e doesn't collide
    case 0x05: // non collidable sprite
    case 0x15:
	if (color==0) break;
	if (!(x&1)) {
	    *screen=(*screen&0x0f)|(color<<4);
	} else {
	    *screen=(*screen&0xf0)|color;
	}
	blitter.memory_accesses++;
	break;
    case 0x14:
	if (color==0) break;
	if (!(x&1)) {
	    back=*colbuf;
	    *colbuf=(back&~0xf0)|(blitter.spritenr<<4);
	    if (back>>4>blitter.mem[blitter.colpos])
		blitter.mem[blitter.colpos]=back>>4;
	    *screen=(*screen&0x0f)|(color<<4);
	} else {
	    back=*colbuf;
	    *colbuf=(back&~0xf)|(blitter.spritenr);
	    if ((back&0xf)>blitter.mem[blitter.colpos])
		blitter.mem[blitter.colpos]=back&0xf;
	    *screen=(*screen&0xf0)|color;
	}
	blitter.memory_accesses+=3;
	break;
    case 0x17:
	if (color==0) break;
	if (!(x&1)) {
	    if (color!=0xe) {
		back=*colbuf;
		if (back>>4>blitter.mem[blitter.colpos])
		    blitter.mem[blitter.colpos]=back>>4;
		*colbuf=(back&~0xf0)|(blitter.spritenr<<4);
		blitter.memory_accesses+=2;
	    }
	    *screen=(*screen&0x0f)|(color<<4);
	} else {
	    if (color!=0xe) {
		back=*colbuf;
		if ((back&0xf)>blitter.mem[blitter.colpos])
		    blitter.mem[blitter.colpos]=back&0xf;
		*colbuf=(back&~0xf)|(blitter.spritenr);
		blitter.memory_accesses+=2;
	    }
	    *screen=(*screen&0xf0)|color;
	}
	blitter.memory_accesses++;
	break;
    case 0x06: // xor sprite (shadow bug!)
	if (!(x&1)) {
	    *screen=(*screen&0x0f)^(color<<4);
	} else {
	    *screen=(*screen&0xf0)^color;
	}
	blitter.memory_accesses++;
	break;
    case 0x16:
	if (!(x&1)) {
	    if (color!=0xe) {
		back=*colbuf;
		if (back>>4>blitter.mem[blitter.colpos])
		    blitter.mem[blitter.colpos]=back>>4;
		*colbuf=(back&~0xf0)|(blitter.spritenr<<4);
		blitter.memory_accesses+=2;
	    }
	    *screen=(*screen&0x0f)^(color<<4);
	} else {
	    if (color!=0xe) {
		back=*colbuf;
		if ((back&0xf)>blitter.mem[blitter.colpos])
		    blitter.mem[blitter.colpos]=back&0xf;
		*colbuf=(back&~0xf)|(blitter.spritenr);
		blitter.memory_accesses+=2;
	    }
	    *screen=(*screen&0xf0)^color;
	}
	blitter.memory_accesses++;
	break;
    }
}

#define INCLUDE_LYNX_LINE_FUNCTION
static void lynx_blit_2color_line(const int y, const int xdir)
{
	const int bits=1;
	const int mask=0x1;
#include "includes/lynx.h"
}
static void lynx_blit_4color_line(const int y, const int xdir)
{
	const int bits=2;
	const int mask=0x3;
#include "includes/lynx.h"
}
static void lynx_blit_8color_line(const int y, const int xdir)
{
	const int bits=3;
	const int mask=0x7;
#include "includes/lynx.h"
}
static void lynx_blit_16color_line(const int y, const int xdir)
{
	const int bits=4;
	const int mask=0xf;
#include "includes/lynx.h"
}
#undef INCLUDE_LYNX_LINE_FUNCTION

/*
2 color rle: ??
 0, 4 bit repeat count-1, 1 bit color
 1, 4 bit count of values-1, 1 bit color, ....
*/

/*
4 color rle:
 0, 4 bit repeat count-1, 2 bit color
 1, 4 bit count of values-1, 2 bit color, ....
*/

/*
8 color rle:
 0, 4 bit repeat count-1, 3 bit color
 1, 4 bit count of values-1, 3 bit color, ....
*/

/*
16 color rle:
 0, 4 bit repeat count-1, 4 bit color
 1, 4 bit count of values-1, 4 bit color, ....
*/
#define INCLUDE_LYNX_LINE_RLE_FUNCTION
static void lynx_blit_2color_rle_line(const int y, const int xdir)
{
	const int bits=1;
	const int mask=0x1;
#include "includes/lynx.h"
}
static void lynx_blit_4color_rle_line(const int y, const int xdir)
{
	const int bits=2;
	const int mask=0x3;
#include "includes/lynx.h"
}
static void lynx_blit_8color_rle_line(const int y, const int xdir)
{
	const int bits=3;
	const int mask=0x7;
#include "includes/lynx.h"
}
static void lynx_blit_16color_rle_line(const int y, const int xdir)
{
	const int bits=4;
	const int mask=0xf;
#include "includes/lynx.h"
}
#undef INCLUDE_LYNX_LINE_RLE_FUNCTION


static void lynx_blit_lines(void)
{
	int i, hi, y;
	int ydir=0, xdir=0;
	int flip=blitter.mem[blitter.cmd+1]&3;

	blitter.everon=FALSE;

	// flipping sprdemo3
	// fat bobby 0x10
	// mirror the sprite in gameplay?
	xdir=1;
	if (blitter.mem[blitter.cmd]&0x20) { xdir=-1;blitter.x--;/*?*/ }
	ydir=1;
	if (blitter.mem[blitter.cmd]&0x10) { ydir=-1;blitter.y--;/*?*/ }
	switch (blitter.mem[blitter.cmd+1]&3) {
	case 0:
		flip =0;
		break;
	case 1: // blockout
		xdir*=-1;
		flip=1;
		break;
	case 2: // fat bobby horicontal
		ydir*=-1;
		flip=1;
		break;
	case 3:
		xdir*=-1;
		ydir*=-1;
		flip=3;
		break;
	}

	for ( y=blitter.y, hi=0; (blitter.memory_accesses++,i=blitter.mem[blitter.bitmap]); blitter.bitmap+=i ) {
		if (i==1) {
		    // centered sprites sprdemo3, fat bobby, blockout
		    hi=0;
		    switch (flip&3) {
		    case 0:
		    case 2:
			ydir*=-1;
			blitter.y+=ydir;
			break;
		    case 1:
		    case 3:
			xdir*=-1;
			blitter.x+=xdir;
			break;
		    }
		    y=blitter.y;
		    flip++;
		    continue;
		}
		for (;(hi<blitter.height); hi+=0x100, y+=ydir) {
		    if ( y>=0 && y<102 )
			blitter.line_function(y,xdir);
		    blitter.width+=blitter.stretch;
		    blitter.x+=blitter.tilt;
		}
		hi-=blitter.height;
	}
	switch (blitter.mode) {
	case 0x12: case 0x13: case 0x14: case 0x16: case 0x17:
	    if (suzy.u.s.SPRG0&0x20 && blitter.everon) {
		blitter.mem[blitter.colpos]|=0x80;
	    }
	}
}

static TIMER_CALLBACK(lynx_blitter_timer)
{
    suzy.u.s.SPRSYS&=~1; //blitter finished
}

/*
  control 0
   bit 7,6: 00 2 color
            01 4 color
            11 8 colors?
            11 16 color
   bit 5,4: 00 right down
            01 right up
            10 left down
            11 left up

#define SHADOW         (0x07)
#define XORSHADOW      (0x06)
#define NONCOLLIDABLE  (0x05)
#define NORMAL         (0x04)
#define BOUNDARY       (0x03)
#define BOUNDARYSHADOW (0x02)
#define BKGRNDNOCOL    (0x01)
#define BKGRND         (0x00)

  control 1
   bit 7: 0 bitmap rle encoded
          1 not encoded
   bit 3: 0 color info with command
          1 no color info with command

#define RELHVST        (0x30)
#define RELHVS         (0x20)
#define RELHV          (0x10)

#define SKIPSPRITE     (0x04)

#define DUP            (0x02)
#define DDOWN          (0x00)
#define DLEFT          (0x01)
#define DRIGHT         (0x00)


  coll
#define DONTCOLLIDE    (0x20)

  word next
  word data
  word x
  word y
  word width
  word height

  pixel c0 90 20 0000 datapointer x y 0100 0100 color (8 colorbytes)
  4 bit direct?
  datapointer 2 10 0
  98 (0 colorbytes)

  box c0 90 20 0000 datapointer x y width height color
  datapointer 2 10 0

  c1 98 00 4 bit direct without color bytes (raycast)

  40 10 20 4 bit rle (sprdemo2)

  line c1 b0 20 0000 datapointer x y 0100 0100 stretch tilt:x/y color (8 color bytes)
  or
  line c0 b0 20 0000 datapointer x y 0100 0100 stretch tilt:x/y color
  datapointer 2 11 0

  text ?(04) 90 20 0000 datapointer x y width height color
  datapointer 2 10 0

  stretch: hsize adder
  tilt: hpos adder

*/

static void lynx_blitter(void)
{
    static const int lynx_colors[4]={2,4,8,16};

    static void (* const blit_line[4])(const int y, const int xdir)= {
	lynx_blit_2color_line,
	lynx_blit_4color_line,
	lynx_blit_8color_line,
	lynx_blit_16color_line
    };

    static void (* const blit_rle_line[4])(const int y, const int xdir)= {
	lynx_blit_2color_rle_line,
	lynx_blit_4color_rle_line,
	lynx_blit_8color_rle_line,
	lynx_blit_16color_rle_line
    };
    int i; int o;int colors;

    blitter.memory_accesses=0;
    blitter.mem = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, 0x0000);
    blitter.colbuf=GET_WORD(suzy.u.data, 0xa);
    blitter.screen=GET_WORD(suzy.u.data, 8);
    blitter.xoff=GET_WORD(suzy.u.data,4);
    blitter.yoff=GET_WORD(suzy.u.data,6);
    // hsizeoff GET_WORD(suzy.u.data, 0x28)
    // vsizeoff GET_WORD(suzy.u.data, 0x2a)

    // these might be never set by the blitter hardware
    blitter.width=0x100;
    blitter.height=0x100;
    blitter.stretch=0;
    blitter.tilt=0;

    blitter.memory_accesses+=2;
    for (blitter.cmd=GET_WORD(suzy.u.data, 0x10); blitter.cmd; ) {

	blitter.memory_accesses+=1;
	if (!(blitter.mem[blitter.cmd+1]&4)) {

	    blitter.colpos=GET_WORD(suzy.u.data, 0x24)+blitter.cmd;

	    blitter.bitmap=GET_WORD(blitter.mem,blitter.cmd+5);
	    blitter.x=(INT16)GET_WORD(blitter.mem, blitter.cmd+7)-blitter.xoff;
	    blitter.y=(INT16)GET_WORD(blitter.mem, blitter.cmd+9)-blitter.yoff;
	    blitter.memory_accesses+=6;

	    blitter.mode=blitter.mem[blitter.cmd]&07;
	    if (blitter.mem[blitter.cmd+1]&0x80) {
		blitter.line_function=blit_line[blitter.mem[blitter.cmd]>>6];
	    } else {
		blitter.line_function=blit_rle_line[blitter.mem[blitter.cmd]>>6];
	    }

	    if (!(blitter.mem[blitter.cmd+2]&0x20) && !( suzy.u.s.SPRSYS&0x20) ) {
		switch (blitter.mode) {
		case 0: case 2: case 3: case 4: case 6: case 7:
		    blitter.mode|=0x10;
		    blitter.mem[blitter.colpos]=0;
		    blitter.spritenr=blitter.mem[blitter.cmd+2]&0xf;
		}
	    }

	    o=0xb;
	    if (blitter.mem[blitter.cmd+1]&0x30) {
		blitter.width=GET_WORD(blitter.mem, blitter.cmd+11);
		blitter.height=GET_WORD(blitter.mem, blitter.cmd+13);
		blitter.memory_accesses+=4;
		o+=4;
	    }

	    if (blitter.mem[blitter.cmd+1]&0x20) {
		blitter.stretch=GET_WORD(blitter.mem, blitter.cmd+o);
		blitter.memory_accesses+=2;
		o+=2;
		if (blitter.mem[blitter.cmd+1]&0x10) {
		    blitter.tilt=GET_WORD(blitter.mem, blitter.cmd+o);
		    blitter.memory_accesses+=2;
		    o+=2;
		}
	    }
	    colors=lynx_colors[blitter.mem[blitter.cmd]>>6];

	    if (!(blitter.mem[blitter.cmd+1]&8)) {
		for (i=0; i<colors/2; i++) {
		    blitter.color[i*2]=blitter.mem[blitter.cmd+o+i]>>4;
		    blitter.color[i*2+1]=blitter.mem[blitter.cmd+o+i]&0xf;
		    blitter.memory_accesses++;
		}
	    }
	    lynx_blit_lines();
	}
	blitter.cmd=GET_WORD(blitter.mem, blitter.cmd+3);
	blitter.memory_accesses+=2;
	if (!(blitter.cmd&0xff00)) break;
    }

	if (0)
		timer_set(ATTOTIME_IN_CYCLES(blitter.memory_accesses*20,0), NULL, 0, lynx_blitter_timer);
}

static void lynx_divide(void)
{
	UINT32 left=suzy.u.s.H|(suzy.u.s.G<<8)|(suzy.u.s.F<<16)|(suzy.u.s.E<<24);
	UINT16 right=suzy.u.s.P|(suzy.u.s.N<<8);
	UINT32 res, mod;
	suzy.accumulate_overflow=FALSE;
	if (right==0) {
	    suzy.accumulate_overflow=TRUE;
	    res=0xffffffff;
	    mod=0; //?
	} else {
	    res=left/right;
	    mod=left%right;
	}
//	logerror("coprocessor %8x / %8x = %4x\n", left, right, res);
	suzy.u.s.D=res&0xff;
	suzy.u.s.C=res>>8;
	suzy.u.s.B=res>>16;
	suzy.u.s.A=res>>24;
	suzy.u.s.M=mod&0xff;
	suzy.u.s.L=mod>>8;
	suzy.u.s.K=mod>>16;
	suzy.u.s.J=mod>>24;
}

static void lynx_multiply(void)
{
	UINT16 left, right;
	UINT32 res, accu;
	left=suzy.u.s.B|(suzy.u.s.A<<8);
	right=suzy.u.s.D|(suzy.u.s.C<<8);
	if (suzy.u.s.SPRSYS&0x80) {
	    // to do
	    res=(INT16)left*(INT16)right;
	} else res=left*right;
//	logerror("coprocessor %4x * %4x = %4x\n", left, right, res);
	suzy.u.s.H=res&0xff;
	suzy.u.s.G=res>>8;
	suzy.u.s.F=res>>16;
	suzy.u.s.E=res>>24;
	if (suzy.u.s.SPRSYS&0x40) {
	    accu=suzy.u.s.M|suzy.u.s.L<<8|suzy.u.s.K<<16|suzy.u.s.J<<24;
	    accu+=res;
	    if (accu<res) suzy.accumulate_overflow=TRUE;
	    suzy.u.s.M=accu;
	    suzy.u.s.L=accu>>8;
	    suzy.u.s.K=accu>>16;
	    suzy.u.s.J=accu>>24;
	}
}

 READ8_HANDLER(suzy_read)
{
	UINT8 data=0, input;
	switch (offset) {
	case 0x88:
		data=1; // must not be 0 for correct power up
		break;
	case 0x92:
		if (!attotime_compare(blitter.time, attotime_zero))
		{
			if ( ATTOTIME_TO_CYCLES(0,attotime_sub(timer_get_time(), blitter.time)) > blitter.memory_accesses*20)
			{
				suzy.u.data[offset]&=~1; //blitter finished
				blitter.time = attotime_zero;
			}
		}
		data=suzy.u.data[offset];
		data&=~0x80; // math finished
		data&=~0x40;
		if (suzy.accumulate_overflow) data|=0x40;
		break;
	case 0xb0:
		input=input_port_read(machine, "JOY");
		switch (lynx_rotate) {
		case 1:
			data=input;
			input&=0xf;
			if (data&PAD_UP) input|=PAD_LEFT;
			if (data&PAD_LEFT) input|=PAD_DOWN;
			if (data&PAD_DOWN) input|=PAD_RIGHT;
			if (data&PAD_RIGHT) input|=PAD_UP;
			break;
		case 2:
			data=input;
			input&=0xf;
			if (data&PAD_UP) input|=PAD_RIGHT;
			if (data&PAD_RIGHT) input|=PAD_DOWN;
			if (data&PAD_DOWN) input|=PAD_LEFT;
			if (data&PAD_LEFT) input|=PAD_UP;
			break;
		}
		if (suzy.u.s.SPRSYS&8) {
			data=input&0xf;
			if (input&PAD_UP) data|=PAD_DOWN;
			if (input&PAD_DOWN) data|=PAD_UP;
			if (input&PAD_LEFT) data|=PAD_RIGHT;
			if (input&PAD_RIGHT) data|=PAD_LEFT;
		} else {
			data=input;
		}
		break;
	case 0xb1: data=input_port_read(machine, "PAUSE");break;
	case 0xb2:
		data=*(memory_region(machine, REGION_USER1)+(suzy.high*lynx_granularity)+suzy.low);
		suzy.low=(suzy.low+1)&(lynx_granularity-1);
		break;
	default:
		data=suzy.u.data[offset];
	}
//	logerror("suzy read %.2x %.2x\n",offset,data);
	return data;
}

WRITE8_HANDLER(suzy_write)
{
	suzy.u.data[offset]=data;
	switch(offset) {
	case 0x52: case 0x54: case 0x56:
	case 0x60: case 0x62:
	case 0x6e:
	    suzy.u.data[offset+1]=0;
	    break;
	case 0x6c:
	    suzy.u.data[offset+1]=0;
	    suzy.accumulate_overflow=FALSE;
	    break;
	case 0x55: lynx_multiply();break;
	case 0x63: lynx_divide();break;
	case 0x91:
	    if (data&1) {
		blitter.time=timer_get_time();
		lynx_blitter();
	    }
//	    logerror("suzy write %.2x %.2x\n",offset,data);
	    break;
//	default:
//	    logerror("suzy write %.2x %.2x\n",offset,data);
	}
}

/*
 0xfd0a r sync signal?
 0xfd81 r interrupt source bit 2 vertical refresh
 0xfd80 w interrupt quit
 0xfd87 w bit 1 !clr bit 0 blocknumber clk
 0xfd8b w bit 1 blocknumber hi B
 0xfd94 w 0
 0xfd95 w 4
 0xfda0-f rw farben 0..15
 0xfdb0-f rw bit0..3 farben 0..15
*/
MIKEY mikey={ { 0 } };


/*
HCOUNTER        EQU TIMER0
VCOUNTER        EQU TIMER2
SERIALRATE      EQU TIMER4

TIM_BAKUP       EQU 0   ; backup-value (count+1)
TIM_CNTRL1      EQU 1   ; timer-control register
TIM_CNT EQU 2   ; current counter
TIM_CNTRL2      EQU 3   ; dynamic control

; TIM_CNTRL1
TIM_IRQ EQU %10000000   ; enable interrupt (not TIMER4 !)
TIM_RESETDONE   EQU %01000000   ; reset timer done
TIM_MAGMODE     EQU %00100000   ; nonsense in Lynx !!
TIM_RELOAD      EQU %00010000   ; enable reload
TIM_COUNT       EQU %00001000   ; enable counter
TIM_LINK        EQU %00000111
; link timers (0->2->4 / 1->3->5->7->Aud0->Aud1->Aud2->Aud3->1
TIM_64us        EQU %00000110
TIM_32us        EQU %00000101
TIM_16us        EQU %00000100
TIM_8us EQU %00000011
TIM_4us EQU %00000010
TIM_2us EQU %00000001
TIM_1us EQU %00000000

;TIM_CNTRL2 (read-only)
; B7..B4 unused
TIM_DONE        EQU %00001000   ; set if timer's done; reset with TIM_RESETDONE
TIM_LAST        EQU %00000100   ; last clock (??)
TIM_BORROWIN    EQU %00000010
TIM_BORROWOUT   EQU %00000001
*/
typedef struct {
	UINT8	bakup;
	UINT8	cntrl1;
	UINT8	cntrl2;
	int		counter;
	void	*timer;
	int		timer_active;
} LYNX_TIMER;

#define NR_LYNX_TIMERS	8

static LYNX_TIMER lynx_timer[NR_LYNX_TIMERS];

static TIMER_CALLBACK(lynx_timer_shot);

static void lynx_timer_init(int which)
{
	memset( &lynx_timer[which], 0, sizeof(LYNX_TIMER) );
	lynx_timer[which].timer = timer_alloc( lynx_timer_shot , NULL);
}

static void lynx_timer_signal_irq(running_machine *machine, int which)
{
    if ( ( lynx_timer[which].cntrl1 & 0x80 ) && ( which != 4 ) ) { // irq flag handling later
		mikey.data[0x81] |= ( 1 << which );
		cpunum_set_input_line(machine, 0, M65SC02_IRQ_LINE, ASSERT_LINE);
    }
    switch ( which ) {
    case 0:
		lynx_timer_count_down( machine, 2 );
		lynx_line++;
		break;
    case 2:
		lynx_timer_count_down( machine, 4 );
		lynx_draw_lines( machine, -1 );
		lynx_line=0;
		break;
    case 1:
		lynx_timer_count_down( machine, 3 );
		break;
    case 3:
		lynx_timer_count_down( machine, 5 );
		break;
    case 5:
		lynx_timer_count_down( machine, 7 );
		break;
    case 7:
		lynx_audio_count_down( machine, 0 );
		break;
    }
}

void lynx_timer_count_down(running_machine *machine, int which)
{
    if ( ( lynx_timer[which].cntrl1 & 0x0f ) == 0x0f ) {
		if ( lynx_timer[which].counter > 0 ) {
		    lynx_timer[which].counter--;
		    return;
		}
		if ( lynx_timer[which].counter == 0 ) {
			lynx_timer[which].cntrl2 |= 8;
		    lynx_timer_signal_irq(machine, which);
		    if ( lynx_timer[which].cntrl1 & 0x10 ) {
				lynx_timer[which].counter = lynx_timer[which].bakup;
		    } else {
				lynx_timer[which].counter--;
		    }
		    return;
		}
    }
}

static TIMER_CALLBACK(lynx_timer_shot)
{
	lynx_timer[param].cntrl2 |= 8;
    lynx_timer_signal_irq( machine, param );
    if ( ! ( lynx_timer[param].cntrl1 & 0x10 ) )
		lynx_timer[param].timer_active = 0;
}

static UINT32 lynx_time_factor(int val)
{
	switch(val)
	{
		case 0:	return 1000000;
		case 1: return 500000;
		case 2: return 250000;
		case 3: return 125000;
		case 4: return 62500;
		case 5: return 31250;
		case 6: return 15625;
		default: fatalerror("invalid value");
	}
}

static UINT8 lynx_timer_read(int which, int offset)
{
	UINT8 data = 0;
	switch (offset) {
	case 0:
		data = lynx_timer[which].bakup;
		break;
	case 1:
		data = lynx_timer[which].cntrl1;
		break;
	case 2:
		if ( ( lynx_timer[which].cntrl1 & 7 ) == 7 )
		{
			data = lynx_timer[which].counter;
		}
		else
		{
			if ( lynx_timer[which].timer_active )
				data = (UINT8) ( lynx_timer[which].bakup - attotime_mul(timer_timeleft(lynx_timer[which].timer), lynx_time_factor( lynx_timer[which].cntrl1 & 7 )).seconds);
		}
		break;

	case 3:
		data = lynx_timer[which].cntrl2;
		break;
	}
	logerror("timer %d read %x %.2x\n", which, offset, data);
	return data;
}

static void lynx_timer_write(int which, int offset, UINT8 data)
{
	logerror("timer %d write %x %.2x\n", which, offset, data);

	switch (offset) {
	case 0:
		lynx_timer[which].bakup = data;
		break;
	case 1:
		lynx_timer[which].cntrl1 = data;
		if ( data & 0x40 )
			lynx_timer[which].cntrl2 &= ~8;
		break;
	case 2:
//		lynx_timer[which].counter = data;
		break;
	case 3:
		lynx_timer[which].cntrl2 = ( lynx_timer[which].cntrl2 & 8 ) | ( data & ~8 );
		break;
	}

	/* Update timers */
	if ( offset < 3 ) {
		timer_reset( lynx_timer[which].timer, attotime_never);
		lynx_timer[which].timer_active = 0;
		if ( ( lynx_timer[which].cntrl1 & 0x08 ) )
		{
			if ( ( lynx_timer[which].cntrl1 & 7 ) != 7 )
			{
				attotime t = attotime_mul(ATTOTIME_IN_HZ( lynx_time_factor( lynx_timer[which].cntrl1 & 7 ) ), lynx_timer[which].bakup + 1 );
				if ( lynx_timer[which].cntrl1 & 0x10 )
					timer_adjust_periodic(lynx_timer[which].timer, attotime_zero, which, t);
				else
					timer_adjust_oneshot(lynx_timer[which].timer, t, which);
				lynx_timer[which].timer_active = 1;
			}
		}
	}
}

static struct {
    UINT8 serctl;
    UINT8 data_received, data_to_send, buffer;

    int received;
    int sending;
    int buffer_loaded;
} uart;

static void lynx_uart_reset(void)
{
    memset(&uart, 0, sizeof(uart));
}

static TIMER_CALLBACK(lynx_uart_timer)
{
    if (uart.buffer_loaded) {
	uart.data_to_send=uart.buffer;
	uart.buffer_loaded=FALSE;
	timer_set(ATTOTIME_IN_USEC(11), NULL, 0, lynx_uart_timer);
    } else {
	uart.sending=FALSE;
    }
//    mikey.data[0x80]|=0x10;
    if (uart.serctl&0x80) {
	mikey.data[0x81]|=0x10;
	cpunum_set_input_line(machine, 0, M65SC02_IRQ_LINE, ASSERT_LINE);
    }
}

static  READ8_HANDLER(lynx_uart_r)
{
    UINT8 data=0;
    switch (offset) {
    case 0x8c:
	if (!uart.buffer_loaded) data|=0x80;
	if (uart.received) data|=0x40;
	if (!uart.sending) data|=0x20;
	break;
    case 0x8d:
	data=uart.data_received;
	break;
    }
    logerror("uart read %.2x %.2x\n",offset,data);
    return data;
}

static WRITE8_HANDLER(lynx_uart_w)
{
    logerror("uart write %.2x %.2x\n",offset,data);
    switch (offset) {
    case 0x8c:
	uart.serctl=data;
	break;
    case 0x8d:
	if (uart.sending) {
	    uart.buffer=data;
	    uart.buffer_loaded=TRUE;
	} else {
	    uart.sending=TRUE;
	    uart.data_to_send=data;
	    timer_set(ATTOTIME_IN_USEC(11), NULL, 0, lynx_uart_timer);
	}
	break;
    }
}

 READ8_HANDLER(mikey_read)
{
    UINT8 data=0;
    switch (offset) {
    case 0: case 1: case 2: case 3:
    case 4: case 5: case 6: case 7:
    case 8: case 9: case 0xa: case 0xb:
    case 0xc: case 0xd: case 0xe: case 0xf:
    case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x14: case 0x15: case 0x16: case 0x17:
    case 0x18: case 0x19: case 0x1a: case 0x1b:
    case 0x1c: case 0x1d: case 0x1e: case 0x1f:
		data = lynx_timer_read( offset >> 2, offset & 3 );
		return data;
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
    case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x50:
		data = lynx_audio_read( offset );
		return data;
    case 0x81:
		data = mikey.data[offset];
		logerror( "mikey read %.2x %.2x\n", offset, data );
		break;
    case 0x8b:
		data = mikey.data[offset];
		data |= 4; // no comlynx adapter
		break;
    case 0x8c: case 0x8d:
		data = lynx_uart_r(machine, offset);
		break;
    default:
		data = mikey.data[offset];
		logerror( "mikey read %.2x %.2x\n", offset, data );
    }
    return data;
}

WRITE8_HANDLER(mikey_write)
{
	switch (offset) {
	case 0: case 1: case 2: case 3:
	case 4: case 5: case 6: case 7:
	case 8: case 9: case 0xa: case 0xb:
	case 0xc: case 0xd: case 0xe: case 0xf:
	case 0x10: case 0x11: case 0x12: case 0x13:
	case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b:
	case 0x1c: case 0x1d: case 0x1e: case 0x1f:
		lynx_timer_write( offset >> 2, offset & 3, data );
		return;

	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x50:
		lynx_audio_write(offset, data);
		return;

	case 0x80:
		mikey.data[0x81]&=~data; // clear interrupt source
		logerror("mikey write %.2x %.2x\n",offset,data);
		if (!mikey.data[0x81])
			cpunum_set_input_line(machine, 0, M65SC02_IRQ_LINE, CLEAR_LINE);
		break;

	case 0x87:
		mikey.data[offset]=data; //?
		if (data&2)
		{
			if (data&1)
			{
				suzy.high<<=1;
				if (mikey.data[0x8b]&2)
					suzy.high|=1;
				suzy.low=0;
			}
		}
		else
		{
			suzy.high=0;
			suzy.low=0;
		}
		break;

	case 0x8c: case 0x8d:
		lynx_uart_w(machine, offset, data);
		break;

	case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
	case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
	case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
	case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
		mikey.data[offset]=data;
		lynx_draw_lines(machine, lynx_line);
#if 0
		palette_set_color_rgb(offset&0xf,
					(mikey.data[0xb0+(offset&0xf)]&0xf)<<4,
					(mikey.data[0xa0+(offset&0xf)]&0xf)<<4,
					mikey.data[0xb0+(offset&0xf)]&0xf0 );
#else
		lynx_palette[offset&0xf]=machine->pens[((mikey.data[0xb0+(offset&0xf)]&0xf))
			|((mikey.data[0xa0+(offset&0xf)]&0xf)<<4)
			|((mikey.data[0xb0+(offset&0xf)]&0xf0)<<4)];
#endif
		break;

    case 0x8b:case 0x90:case 0x91:
		mikey.data[offset]=data;
		break;

    default:
		mikey.data[offset]=data;
		logerror("mikey write %.2x %.2x\n",offset,data);
		break;
    }
}

READ8_HANDLER( lynx_memory_config_r )
{
	return lynx_memory_config;
}

WRITE8_HANDLER( lynx_memory_config_w )
{
    /* bit 7: hispeed, uses page mode accesses (4 instead of 5 cycles )
     * when these are safe in the cpu */
    lynx_memory_config = data;

	memory_install_read8_handler(machine, 0,  ADDRESS_SPACE_PROGRAM, 0xfc00, 0xfcff, 0, 0, (data & 1) ? SMH_BANK1 : suzy_read);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfc00, 0xfcff, 0, 0, (data & 1) ? SMH_BANK1 : suzy_write);
	memory_install_read8_handler(machine, 0,  ADDRESS_SPACE_PROGRAM, 0xfd00, 0xfdff, 0, 0, (data & 2) ? SMH_BANK2 : mikey_read);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfd00, 0xfdff, 0, 0, (data & 2) ? SMH_BANK2 : mikey_write);

	if (data & 1)
		memory_set_bankptr(1, lynx_mem_fc00);
	if (data & 2)
		memory_set_bankptr(2, lynx_mem_fd00);
	memory_set_bank(3, (data & 4) ? 1 : 0);
	memory_set_bank(4, (data & 8) ? 1 : 0);
}

static void lynx_reset(running_machine *machine)
{
	int i;
	lynx_memory_config_w(machine, 0, 0);

	cpunum_set_input_line(machine, 0, M65SC02_IRQ_LINE, CLEAR_LINE);

	memset(&suzy, 0, sizeof(suzy));
	memset(&mikey, 0, sizeof(mikey));

	mikey.data[0x80] = 0;
	mikey.data[0x81] = 0;

	lynx_uart_reset();

	for (i = 0; i < NR_LYNX_TIMERS; i++)
		lynx_timer_init( i );

	lynx_audio_reset();

	// hack to allow current object loading to work
#if 1
	lynx_timer_write( 0, 0, 160 );
	lynx_timer_write( 0, 1, 0x10 | 0x8 | 0 );
	lynx_timer_write( 2, 0, 102 );
	lynx_timer_write( 2, 1, 0x10 | 0x8 | 7 );
#endif
}

static STATE_POSTLOAD( lynx_postload )
{
	lynx_memory_config_w(machine, 0, lynx_memory_config);
}

MACHINE_START( lynx )
{
	state_save_register_global(lynx_memory_config);
	state_save_register_global_pointer(lynx_mem_fe00, lynx_mem_fe00_size);
	state_save_register_postload(machine, lynx_postload, NULL);

	memory_configure_bank(3, 0, 1, memory_region(machine, REGION_CPU1) + 0x0000, 0);
	memory_configure_bank(3, 1, 1, lynx_mem_fe00, 0);
	memory_configure_bank(4, 0, 1, memory_region(machine, REGION_CPU1) + 0x01fa, 0);
	memory_configure_bank(4, 1, 1, lynx_mem_fffa, 0);

	memset(&suzy, 0, sizeof(suzy));

	add_reset_callback(machine, lynx_reset);
}
