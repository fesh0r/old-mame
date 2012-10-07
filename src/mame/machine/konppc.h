#ifndef _KONPPC_H
#define _KONPPC_H

#define CGBOARD_TYPE_ZR107		0
#define CGBOARD_TYPE_GTICLUB	1
#define CGBOARD_TYPE_NWKTR		2
#define CGBOARD_TYPE_HORNET		3
#define CGBOARD_TYPE_HANGPLT	4

void init_konami_cgboard(running_machine &machine, int board_id, int type);
void set_cgboard_id(int board_id);
int get_cgboard_id(void);
void set_cgboard_texture_bank(running_machine &machine, int board, const char *bank, UINT8 *rom);

DECLARE_READ32_HANDLER( cgboard_dsp_comm_r_ppc );
DECLARE_WRITE32_HANDLER( cgboard_dsp_comm_w_ppc );
DECLARE_READ32_HANDLER( cgboard_dsp_shared_r_ppc );
DECLARE_WRITE32_HANDLER( cgboard_dsp_shared_w_ppc );

DECLARE_READ32_HANDLER( cgboard_0_comm_sharc_r );
DECLARE_WRITE32_HANDLER( cgboard_0_comm_sharc_w );
DECLARE_READ32_HANDLER( cgboard_0_shared_sharc_r );
DECLARE_WRITE32_HANDLER( cgboard_0_shared_sharc_w );
DECLARE_READ32_HANDLER( cgboard_1_comm_sharc_r );
DECLARE_WRITE32_HANDLER( cgboard_1_comm_sharc_w );
DECLARE_READ32_HANDLER( cgboard_1_shared_sharc_r );
DECLARE_WRITE32_HANDLER( cgboard_1_shared_sharc_w );

DECLARE_READ32_HANDLER(K033906_0_r);
DECLARE_WRITE32_HANDLER(K033906_0_w);
DECLARE_READ32_HANDLER(K033906_1_r);
DECLARE_WRITE32_HANDLER(K033906_1_w);

DECLARE_WRITE32_DEVICE_HANDLER(nwk_fifo_0_w);
DECLARE_WRITE32_DEVICE_HANDLER(nwk_fifo_1_w);
DECLARE_READ32_DEVICE_HANDLER(nwk_voodoo_0_r);
DECLARE_READ32_DEVICE_HANDLER(nwk_voodoo_1_r);
DECLARE_WRITE32_DEVICE_HANDLER(nwk_voodoo_0_w);
DECLARE_WRITE32_DEVICE_HANDLER(nwk_voodoo_1_w);

void draw_7segment_led(bitmap_rgb32 &bitmap, int x, int y, UINT8 value);

#endif
