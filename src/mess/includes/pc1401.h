/*****************************************************************************
 *
 * includes/pc1401.h
 *
 * Pocket Computer 1401
 *
 ****************************************************************************/

#ifndef PC1401_H_
#define PC1401_H_

#define CONTRAST (input_port_read(machine, "DSW0") & 0x07)


/*----------- defined in machine/pc1401.c -----------*/

extern UINT8 pc1401_portc;
int pc1401_reset(const device_config *device);
int pc1401_brk(const device_config *device);
void pc1401_outa(const device_config *device, int data);
void pc1401_outb(const device_config *device, int data);
void pc1401_outc(const device_config *device, int data);
int pc1401_ina(const device_config *device);
int pc1401_inb(const device_config *device);

DRIVER_INIT( pc1401 );
NVRAM_HANDLER( pc1401 );


/*----------- defined in video/pc1401.c -----------*/

READ8_HANDLER(pc1401_lcd_read);
WRITE8_HANDLER(pc1401_lcd_write);
VIDEO_UPDATE( pc1401 );


#endif /* PC1401_H_ */
