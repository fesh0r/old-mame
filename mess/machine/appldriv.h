/*********************************************************************

	appldriv.h
	
	Apple 5.25" floppy drive emulation (to be interfaced with applefdc.c)

*********************************************************************/

#ifndef APPLDRIV_H
#define APPLDRIV_H

#include "image.h"

void apple525_device_getinfo(struct IODevice *dev, int spinfract_dividend, int spinfract_divisor);

void apple525_set_lines(UINT8 lines);
void apple525_set_enable_lines(int enable_mask);
void apple525_set_sel_line(int sel);

UINT8 apple525_read_data(void);
void apple525_write_data(UINT8 data);
int apple525_read_status(void);

#endif /* APPLDRIV_H */

