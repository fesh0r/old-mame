/*********************************************************************

	8530scc.c

	Zilog 8530 SCC (Serial Control Chip) code

*********************************************************************/


#include "driver.h"
#include "8530scc.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG_SCC	0



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _scc8530_t scc8530_t;
struct _scc8530_t
{
	int mode;
	int reg;
	int status;

	UINT8 reg_val_a[16];
	UINT8 reg_val_b[16];
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE scc8530_t *get_token(const device_config *device)
{
	assert(device->type == SCC8530);
	return (scc8530_t *) device->token;
}


INLINE scc8530_interface *get_interface(const device_config *device)
{
	assert(device->type == SCC8530);
	return (scc8530_interface *) device->static_config;
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START( scc8530 )
-------------------------------------------------*/

static DEVICE_START( scc8530 )
{
	scc8530_t *scc = get_token(device);
	memset(scc, 0, sizeof(*scc));
	return DEVICE_START_OK;
}



/*-------------------------------------------------
    scc_set_status
-------------------------------------------------*/

void scc_set_status(const device_config *device, int status)
{
	scc8530_t *scc = get_token(device);
	scc->status = status;
}



/*-------------------------------------------------
    scc_acknowledge
-------------------------------------------------*/

static void scc_acknowledge(const device_config *device)
{
	scc8530_interface *intf = get_interface(device);
	if ((intf != NULL) && (intf->acknowledge != NULL))
		(*intf->acknowledge)(device);
}



/*-------------------------------------------------
    scc_getareg
-------------------------------------------------*/

static int scc_getareg(const device_config *device)
{
	scc8530_t *scc = get_token(device);

	/* Not yet implemented */
	logerror("SCC: port A reg %i read 0x%02x\n", scc->reg, scc->reg_val_a[scc->reg]);
	return scc->reg_val_a[scc->reg];
}



/*-------------------------------------------------
    scc_getareg
-------------------------------------------------*/

static int scc_getbreg(const device_config *device)
{
	scc8530_t *scc = get_token(device);

	logerror("SCC: port B reg %i read 0x%02x\n", scc->reg, scc->reg_val_b[scc->reg]);

	if (scc->reg == 2)
	{
		/* HACK! but lets the Mac Plus mouse move again.  Needs further investigation. */
		scc_acknowledge(device);

		return scc->status;
	}

	return scc->reg_val_b[scc->reg];
}



/*-------------------------------------------------
    scc_putareg
-------------------------------------------------*/

static void scc_putareg(const device_config *device, int data)
{
	scc8530_t *scc = get_token(device);

	if (scc->reg == 0)
	{
		if (data & 0x10)
			scc_acknowledge(device);
	}
	scc->reg_val_a[scc->reg] = data;
	logerror("SCC: port A reg %i write 0x%02x\n", scc->reg, data);
}



/*-------------------------------------------------
    scc_putbreg
-------------------------------------------------*/

static void scc_putbreg(const device_config *device, int data)
{
	scc8530_t *scc = get_token(device);

	if (scc->reg == 0)
	{
		if (data & 0x10)
			scc_acknowledge(device);
	}
	scc->reg_val_b[scc->reg] = data;
	logerror("SCC: port B reg %i write 0x%02x\n", scc->reg, data);
}



/*-------------------------------------------------
    scc_get_reg_a
-------------------------------------------------*/

UINT8 scc_get_reg_a(const device_config *device, int reg)
{
	scc8530_t *scc = get_token(device);
	return scc->reg_val_a[reg];
}



/*-------------------------------------------------
    scc_get_reg_b
-------------------------------------------------*/

UINT8 scc_get_reg_b(const device_config *device, int reg)
{
	scc8530_t *scc = get_token(device);
	return scc->reg_val_b[reg];
}



/*-------------------------------------------------
    scc_set_reg_a
-------------------------------------------------*/

void scc_set_reg_a(const device_config *device, int reg, UINT8 data)
{
	scc8530_t *scc = get_token(device);
	scc->reg_val_a[reg] = data;
}



/*-------------------------------------------------
    scc_set_reg_a
-------------------------------------------------*/

void scc_set_reg_b(const device_config *device, int reg, UINT8 data)
{
	scc8530_t *scc = get_token(device);
	scc->reg_val_b[reg] = data;
}



/*-------------------------------------------------
    scc_r
-------------------------------------------------*/

READ8_DEVICE_HANDLER(scc_r)
{
	scc8530_t *scc = get_token(device);
	UINT8 result = 0;

	offset %= 4;

	if (LOG_SCC)
		logerror("scc_r: offset=%u\n", (unsigned int) offset);

	switch(offset)
	{
		case 0:
			/* Channel B (Printer Port) Control */
			if (scc->mode == 1)
				scc->mode = 0;
			else
				scc->reg = 0;

			result = scc_getbreg(device);
			break;

		case 1:
			/* Channel A (Modem Port) Control */
			if (scc->mode == 1)
				scc->mode = 0;
			else
				scc->reg = 0;

			result = scc_getareg(device);
			break;

		case 2:
			/* Channel B (Printer Port) Data */
			/* Not yet implemented */
			break;

		case 3:
			/* Channel A (Modem Port) Data */
			/* Not yet implemented */
			break;
	}
	return result;
}



/*-------------------------------------------------
    scc_w
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(scc_w)
{
	scc8530_t *scc = get_token(device);

	offset &= 3;

	switch(offset)
	{
		case 0:
			/* Channel B (Printer Port) Control */
			if (scc->mode == 0)
			{
				if((data & 0xf0) == 0)  // not a reset command
				{
					scc->mode = 1;
					scc->reg = data & 0x0f;
					logerror("SCC: Port B Reg select - %i\n",scc->reg);
//					scc_putbreg(device, data & 0xf0);
				}
			}
			else
			{
				scc->mode = 0;
				scc_putbreg(device, data);
			}
			break;

		case 1:
			/* Channel A (Modem Port) Control */
			if (scc->mode == 0)
			{
				if((data & 0xf0) == 0)  // not a reset command
				{
					scc->mode = 1;
					scc->reg = data & 0x0f;
					logerror("SCC: Port A Reg select - %i\n", scc->reg);
//					scc_putareg(device, data & 0xf0);
				}
			}
			else
			{
				scc->mode = 0;
				scc_putareg(device, data);
			}
			break;

		case 2:
			/* Channel B (Printer Port) Data */
			/* Not yet implemented */
			break;

		case 3:
			/* Channel A (Modem Port) Data */
			/* Not yet implemented */
			break;
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO( scc8530 )
-------------------------------------------------*/

static DEVICE_SET_INFO( scc8530 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO( scc8530 )
-------------------------------------------------*/

DEVICE_GET_INFO( scc8530 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(scc8530_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(scc8530); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(scc8530);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Zilog 8530 SCC";					break;
		case DEVINFO_STR_FAMILY:						info->s = "Zilog 8530 SCC";					break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}
