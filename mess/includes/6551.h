/* 6551 ACIA */

#include "driver.h"

WRITE_HANDLER(acia_6551_w);
READ_HANDLER(acia_6551_r);

void    acia_6551_init(void);
void	acia_6551_set_irq_callback(void (*callback)(int));
void    acia_6551_stop(void);

void	acia_6551_connect_to_serial_device(int id);

