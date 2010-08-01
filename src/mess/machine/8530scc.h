/*********************************************************************

    8530scc.h

    Zilog 8530 SCC (Serial Control Chip) code

*********************************************************************/

#ifndef __8530SCC_H__
#define __8530SCC_H__


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(SCC8530, scc8530);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _scc8530_interface scc8530_interface;
struct _scc8530_interface
{
	void (*irq)(running_device *device, int state);
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_SCC8530_ADD(_tag, _clock) \
	MDRV_DEVICE_ADD(_tag, SCC8530, _clock)

#define MDRV_SCC8530_IRQ(_irq) \
	MDRV_DEVICE_CONFIG_DATAPTR(scc8530_interface, irq, _irq)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void scc8530_set_status(running_device *device, int status);

UINT8 scc8530_get_reg_a(running_device *device, int reg);
UINT8 scc8530_get_reg_b(running_device *device, int reg);
void scc8530_set_reg_a(running_device *device, int reg, UINT8 data);
void scc8530_set_reg_b(running_device *device, int reg, UINT8 data);

READ8_DEVICE_HANDLER(scc8530_r);
WRITE8_DEVICE_HANDLER(scc8530_w);

#endif /* __8530SCC_H__ */
