/*****************************************************************************
 *
 * includes/c16.h
 *
 ****************************************************************************/

#ifndef C16_H_
#define C16_H_


/*----------- defined in machine/c16.c -----------*/

UINT8 c16_m7501_port_read(void);
void c16_m7501_port_write(UINT8 data);

extern WRITE8_HANDLER(c16_6551_port_w);
extern  READ8_HANDLER(c16_6551_port_r);

extern  READ8_HANDLER(c16_fd1x_r);
extern WRITE8_HANDLER(plus4_6529_port_w);
extern  READ8_HANDLER(plus4_6529_port_r);

extern WRITE8_HANDLER(c16_6529_port_w);
extern  READ8_HANDLER(c16_6529_port_r);

#if 0
extern WRITE8_HANDLER(c16_iec9_port_w);
extern  READ8_HANDLER(c16_iec9_port_r);

extern WRITE8_HANDLER(c16_iec8_port_w);
extern  READ8_HANDLER(c16_iec8_port_r);

#endif

extern WRITE8_HANDLER(c16_select_roms);
extern WRITE8_HANDLER(c16_switch_to_rom);
extern WRITE8_HANDLER(c16_switch_to_ram);

/* ted reads */
extern int c16_read_keyboard (int databus);
extern void c16_interrupt (running_machine *machine, int);

extern void c16_driver_init(running_machine *machine);
extern MACHINE_RESET( c16 );
extern INTERRUPT_GEN( c16_frame_interrupt );

extern DEVICE_IMAGE_LOAD(c16_rom);


/*----------- defined in audio/t6721.c -----------*/

extern WRITE8_HANDLER(c364_speech_w);
extern  READ8_HANDLER(c364_speech_r);

extern void c364_speech_init(void);


#endif /* C16_H_ */
