/*****************************************************************************
 *
 *	Programmable Interval Timer 8253/8254
 *
 *****************************************************************************/

#ifndef PIT8253_H
#define PIT8253_H

typedef enum { TYPE8253, TYPE8254 } PIT8253_TYPE;

struct pit8253_config
{
    PIT8253_TYPE type;
	struct
	{
		double clockin;
		void (*irq_callback)(int state);
		void (*clock_callback)(double clockout);
	} timer[3];
};



int pit8253_init(int count);
void pit8253_config(int which, struct pit8253_config *config);
void pit8253_reset(int which);

READ8_HANDLER ( pit8253_0_r );
READ8_HANDLER ( pit8253_1_r );
WRITE8_HANDLER ( pit8253_0_w );
WRITE8_HANDLER ( pit8253_1_w );

READ32_HANDLER ( pit8253_32_0_r );
READ32_HANDLER ( pit8253_32_1_r );
WRITE32_HANDLER ( pit8253_32_0_w );
WRITE32_HANDLER ( pit8253_32_1_w );

WRITE_HANDLER ( pit8253_0_gate_w );
WRITE_HANDLER ( pit8253_1_gate_w );

int pit8253_get_frequency(int which, int timer);
int pit8253_get_output(int which, int timer);
void pit8253_set_clockin(int which, int timer, double new_clockin);


#endif	/* PIT8253_H */

