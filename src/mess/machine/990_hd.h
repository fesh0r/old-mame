/*
	990_hd.h: include file for 990_hd.c
*/

DEVICE_START( ti990_hd );
DEVICE_IMAGE_LOAD( ti990_hd );
DEVICE_IMAGE_UNLOAD( ti990_hd );

void ti990_hdc_init(void (*interrupt_callback)(int state));

extern READ16_HANDLER(ti990_hdc_r);
extern WRITE16_HANDLER(ti990_hdc_w);

