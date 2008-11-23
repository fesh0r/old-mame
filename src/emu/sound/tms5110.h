#pragma once

#ifndef __TMS5110_H__
#define __TMS5110_H__


/* TMS5110 commands */
                                     /* CTL8  CTL4  CTL2  CTL1  |   PDC's  */
                                     /* (MSB)             (LSB) | required */
#define TMS5110_CMD_RESET        (0) /*    0     0     0     x  |     1    */
#define TMS5110_CMD_LOAD_ADDRESS (2) /*    0     0     1     x  |     2    */
#define TMS5110_CMD_OUTPUT       (4) /*    0     1     0     x  |     3    */
#define TMS5110_CMD_READ_BIT     (8) /*    1     0     0     x  |     1    */
#define TMS5110_CMD_SPEAK       (10) /*    1     0     1     x  |     1    */
#define TMS5110_CMD_READ_BRANCH (12) /*    1     1     0     x  |     1    */
#define TMS5110_CMD_TEST_TALK   (14) /*    1     1     1     x  |     3    */

/* Variants */

#define TMS5110_IS_5110A	(1)
#define TMS5110_IS_5100		(2)
#define TMS5110_IS_5110		(3)

#define TMS5110_IS_CD2801	TMS5110_IS_5100
#define TMS5110_IS_TMC0281	TMS5110_IS_5100

#define TMS5110_IS_CD2802	TMS5110_IS_5110
#define TMS5110_IS_M58817	TMS5110_IS_5110

void *tms5110_create(const char *tag, int variant);
void tms5110_destroy(void *chip);

void tms5110_set_variant(void *chip, int variant);

void tms5110_reset_chip(void *chip);
void tms5110_set_M0_callback(void *chip, int (*func)(void));
void tms5110_set_load_address(void *chip, void (*func)(int));

void tms5110_CTL_set(void *chip, int data);
void tms5110_PDC_set(void *chip, int data);

int tms5110_status_read(void *chip);
int tms5110_ready_read(void *chip);

void tms5110_process(void *chip, INT16 *buffer, unsigned int size);

#endif /* __TMS5110_H__ */
