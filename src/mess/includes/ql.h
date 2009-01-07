#ifndef __QL__
#define __QL__

#define SCREEN_TAG	"main"

#define ZX8301_TAG	"ic22"
#define ZX8302_TAG	"ic23"

#define MDV1_TAG	"mdv1"
#define MDV2_TAG	"mdv2"

#define X1 XTAL_15MHz
#define X2 XTAL_32_768kHz
#define X3 XTAL_4_436MHz
#define X4 XTAL_11MHz


typedef struct _ql_state ql_state;
struct _ql_state
{
	/* IPC state */
	UINT8 keylatch;
	int ser1_txd;
	int ser1_dtr;
	int ser2_rxd;
	int ser2_cts;
	int ipl;
	int comctl;
	int comdata;
	int baudx4;

	/* devices */
	const device_config *zx8301;
	const device_config *zx8302;
};

#endif
