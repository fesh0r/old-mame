/*****************************************************************************
 *
 * video/dl1416.h
 *
 * DL1416
 *
 * 4-Digit 16-Segment Alphanumeric Intelligent Display
 * with Memory/Decoder/Driver
 *
 * See video/dl1416.c for more info
 *
 ****************************************************************************/

#ifndef DL1416_H_
#define DL1416_H_


/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	DL1416B,
	DL1416T,
	MAX_DL1416_TYPES
};


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*dl1416_update_func)(const device_config *device, int digit, int data);

typedef struct _dl1416_interface dl1416_interface;
struct _dl1416_interface
{
	int type;
	dl1416_update_func update;
};


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_DL1416_ADD(_tag, _type, _update) \
	MDRV_DEVICE_ADD(_tag, DL1416, 0) \
	MDRV_DEVICE_CONFIG_DATA32(dl1416_interface, type, _type) \
	MDRV_DEVICE_CONFIG_DATAPTR(dl1416_interface, update, _update)

#define MDRV_DL1416B_ADD(_tag, _update_) \
	MDRV_DL1416_ADD(_tag, DL1416B, _update)

#define MDRV_DL1416T_ADD(_tag, _update) \
	MDRV_DL1416_ADD(_tag, DL1416T, _update)


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* inputs */
void dl1416_set_input_w(const device_config *device, int data); /* write enable */
void dl1416_set_input_ce(const device_config *device, int data); /* chip enable */
void dl1416_set_input_cu(const device_config *device, int data); /* cursor enable */
WRITE8_DEVICE_HANDLER( dl1416_w );

/* device get info callback */
#define DL1416 DEVICE_GET_INFO_NAME(dl1416)
DEVICE_GET_INFO( dl1416 );


#endif /* DL1416_H_ */
