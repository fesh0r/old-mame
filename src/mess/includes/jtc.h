#ifndef __JTC__
#define __JTC__

#define SCREEN_TAG		"screen"
#define UB8830D_TAG		"ub8830d"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"

#define JTC_ES40_VIDEORAM_SIZE	0x2000

typedef struct _jtc_state jtc_state;
struct _jtc_state
{
	/* video state */
	UINT8 video_bank;
	UINT8 *video_ram;
	UINT8 *color_ram_r;
	UINT8 *color_ram_g;
	UINT8 *color_ram_b;

	/* devices */
	const device_config *cassette;
	const device_config *speaker;
	const device_config *centronics;
};

#endif
