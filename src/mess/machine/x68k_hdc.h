/*

    X68000 Custom SASI HD controller

*/

#include "driver.h"

enum
{
	SASI_PHASE_BUSFREE = 0,
	SASI_PHASE_ARBITRATION,
	SASI_PHASE_SELECTION,
	SASI_PHASE_RESELECTION,
	SASI_PHASE_COMMAND,
	SASI_PHASE_DATA,
	SASI_PHASE_STATUS,
	SASI_PHASE_MESSAGE,
	SASI_PHASE_READ,
	SASI_PHASE_WRITE
};

// SASI commands, based on the SASI standard
enum
{
	// Class 0 (6-byte) commands
	SASI_CMD_TEST_UNIT_READY = 0,
	SASI_CMD_REZERO_UNIT,
	SASI_CMD_RESERVED_02,
	SASI_CMD_REQUEST_SENSE,
	SASI_CMD_FORMAT_UNIT,
	SASI_CMD_RESERVED_05,
	SASI_CMD_FORMAT_UNIT_06,  // the X68000 uses command 0x06 for Format Unit, despite the SASI specs saying 0x04
	SASI_CMD_RESERVED_07,
	SASI_CMD_READ,
	SASI_CMD_RESERVED_09,
	SASI_CMD_WRITE,
	SASI_CMD_SEEK,
	SASI_CMD_RESERVED_0C,
	SASI_CMD_RESERVED_0D,
	SASI_CMD_RESERVED_0E,
	SASI_CMD_WRITE_FILE_MARK,
	SASI_CMD_INVALID_10,
	SASI_CMD_INVALID_11,
	SASI_CMD_RESERVE_UNIT,
	SASI_CMD_RELEASE_UNIT,
	SASI_CMD_INVALID_14,
	SASI_CMD_INVALID_15,
	SASI_CMD_READ_CAPACITY,
	SASI_CMD_INVALID_17,
	SASI_CMD_INVALID_18,
	SASI_CMD_INVALID_19,
	SASI_CMD_READ_DIAGNOSTIC,
	SASI_CMD_WRITE_DIAGNOSTIC,
	SASI_CMD_INVALID_1C,
	SASI_CMD_INVALID_1D,
	SASI_CMD_INVALID_1E,
	SASI_CMD_INQUIRY,
	// Class 1 commands  (yes, just the one)
	SASI_CMD_RESERVED_20,
	SASI_CMD_RESERVED_21,
	SASI_CMD_RESERVED_22,
	SASI_CMD_SET_BLOCK_LIMITS = 0x28,
	// Class 2 commands
	SASI_CMD_EXTENDED_ADDRESS_READ = 0x48,
	SASI_CMD_INVALID_49,
	SASI_CMD_EXTENDED_ADDRESS_WRITE,
	SASI_CMD_WRITE_AND_VERIFY = 0x54,
	SASI_CMD_VERIFY,
	SASI_CMD_INVALID_56,
	SASI_CMD_SEARCH_DATA_HIGH,
	SASI_CMD_SEARCH_DATA_EQUAL,
	SASI_CMD_SEARCH_DATA_LOW,
	// controller-specific commands
	SASI_CMD_SPECIFY = 0xc2
};

typedef struct _sasi_ctrl_t sasi_ctrl_t;
struct _sasi_ctrl_t
{
	int phase;
	unsigned char status_port;  // read at 0xe96003
	unsigned char status;       // status phase output
	unsigned char message;
	unsigned char command[10];
	unsigned char sense[4];
	int command_byte_count;
	int command_byte_total;
	int current_command;
	int transfer_byte_count;
	int transfer_byte_total;
	int msg;  // MSG
	int cd;   // C/D (Command/Data)
	int bsy;  // BSY
	int io;   // I/O
	int req;  // REQ
};

struct hd_state
{
	int current_block;
	int current_pos;
};

DEVICE_START( x68k_hdc );
DEVICE_GET_INFO( x68k_hdc );

DEVICE_IMAGE_CREATE( sasihd );

#define X68KHDC DEVICE_GET_INFO_NAME(x68k_hdc)

#define MDRV_X68KHDC_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, X68KHDC, 0)

WRITE16_DEVICE_HANDLER( x68k_hdc_w );
READ16_DEVICE_HANDLER( x68k_hdc_r );
