#ifndef YM2413INTF_H
#define YM2413INTF_H

#define MAX_2413 	(4)

#define YM2413interface YM3812interface

WRITE_HANDLER( YM2413_register_port_0_w );
WRITE_HANDLER( YM2413_register_port_1_w );
WRITE_HANDLER( YM2413_register_port_2_w );
WRITE_HANDLER( YM2413_register_port_3_w );
WRITE_HANDLER( YM2413_data_port_0_w );
WRITE_HANDLER( YM2413_data_port_1_w );
WRITE_HANDLER( YM2413_data_port_2_w );
WRITE_HANDLER( YM2413_data_port_3_w );

WRITE16_HANDLER( YM2413_register_port_0_lsb_w );
WRITE16_HANDLER( YM2413_register_port_1_lsb_w );
WRITE16_HANDLER( YM2413_register_port_2_lsb_w );
WRITE16_HANDLER( YM2413_register_port_3_lsb_w );
WRITE16_HANDLER( YM2413_data_port_0_lsb_w );
WRITE16_HANDLER( YM2413_data_port_1_lsb_w );
WRITE16_HANDLER( YM2413_data_port_2_lsb_w );
WRITE16_HANDLER( YM2413_data_port_3_lsb_w );

void YM2413DAC_update(int num, INT16 *buffer, int length);
int  YM2413_sh_start(const struct MachineSound *msound);
void YM2413_sh_stop(void);

#endif

