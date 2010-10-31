#ifndef __KYOCERA__
#define __KYOCERA__

#include "video/hd61830.h"

#define SCREEN_TAG		"screen"
#define I8085_TAG		"m19"
#define I8155_TAG		"m25"
#define UPD1990A_TAG	"m18"
#define IM6402_TAG		"m22"
#define MC14412_TAG		"m31"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"
#define SPEAKER_TAG		"speaker"

//#define I8085_TAG     "m19"
//#define I8155_TAG     "m12"
//#define MC14412_TAG   "m8"
#define RP5C01A_TAG		"m301"
#define TCM5089_TAG		"m11"
#define MSM8251_TAG		"m20"
#define HD61830_TAG		"m18"

class kc85_state : public driver_device
{
public:
	kc85_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* memory state */
	UINT8 bank;				/* memory bank selection */

	/* keyboard state */
	UINT16 keylatch;		/* keyboard latch */

	/* sound state */
	int buzzer;				/* buzzer select */
	int bell;				/* bell output */

	/* peripheral state */
	int iosel;				/* serial interface select */

	running_device *hd44102[10];
	running_device *im6042;
	running_device *upd1990a;
	running_device *mc14412;
	running_device *centronics;
	running_device *speaker;
	running_device *cassette;
};

class tandy200_state : public driver_device
{
public:
	tandy200_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* memory state */
	UINT8 bank;				/* memory bank selection */

	/* keyboard state */
	UINT16 keylatch;		/* keyboard latch */
	int tp;					/* timing pulse */

	/* sound state */
	int buzzer;				/* buzzer select */
	int bell;				/* bell output */

	hd61830_device *hd61830;
	running_device *im6042;
	running_device *mc14412;
	running_device *tcm5089;
	running_device *centronics;
	running_device *speaker;
	running_device *cassette;
};

/* ---------- defined in video/kyocera.c ---------- */

MACHINE_CONFIG_EXTERN( kc85_video );
MACHINE_CONFIG_EXTERN( tandy200_video );

#endif
