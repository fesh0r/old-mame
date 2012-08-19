
#include "machine/6821pia.h"
#include "machine/6840ptm.h"
#include "machine/nvram.h"

#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/okim6376.h"
#include "sound/2413intf.h"
#include "sound/upd7759.h"
#include "machine/steppers.h"
#include "machine/roc10937.h"
#include "machine/meters.h"


#define MPU4_MASTER_CLOCK (6880000)
#define VIDEO_MASTER_CLOCK			XTAL_10MHz


#ifdef MAME_DEBUG
#define MPU4VIDVERBOSE 1
#else
#define MPU4VIDVERBOSE 0
#endif

#define LOGSTUFF(x) do { if (MPU4VIDVERBOSE) logerror x; } while (0)



#ifdef MAME_DEBUG
#define MPU4VERBOSE 1
#else
#define MPU4VERBOSE 0
#endif

#define LOG(x)	do { if (MPU4VERBOSE) logerror x; } while (0)
#define LOG_CHR(x)	do { if (MPU4VERBOSE) logerror x; } while (0)
#define LOG_CHR_FULL(x)	do { if (MPU4VERBOSE) logerror x; } while (0)
#define LOG_IC3(x)	do { if (MPU4VERBOSE) logerror x; } while (0)
#define LOG_IC8(x)	do { if (MPU4VERBOSE) logerror x; } while (0)
#define LOG_SS(x)	do { if (MPU4VERBOSE) logerror x; } while (0)





static const UINT8 reel_mux_table[8]= {0,4,2,6,1,5,3,7};//include 7, although I don't think it's used, this is basically a wire swap
static const UINT8 reel_mux_table7[8]= {3,1,5,6,4,2,0,7};

static const UINT8 bwb_chr_table_common[10]= {0x00,0x04,0x04,0x0c,0x0c,0x1c,0x14,0x2c,0x5c,0x2c};

#define STANDARD_REEL  0	// As originally designed 3/4 reels
#define FIVE_REEL_5TO8 1	// Interfaces to meter port, allows some mechanical metering, but there is significant 'bounce' in the extra reel
#define FIVE_REEL_8TO5 2	// Mounted backwards for space reasons, but different board
#define FIVE_REEL_3TO6 3	// Connected to the centre of the meter connector, taking up meters 3 to 6
#define SIX_REEL_1TO8  4	// Two reels on the meter drives
#define SIX_REEL_5TO8  5	// Like FIVE_REEL_5TO8, but with an extra reel elsewhere
#define SEVEN_REEL     6	// Mainly club machines, significant reworking of reel hardware
#define FLUTTERBOX     7	// Will you start the fans, please!  A fan using a reel mux-like setup, but not actually a reel

#define NO_EXTENDER			0 // As originally designed
#define SMALL_CARD			1
#define LARGE_CARD_A		2 //96 Lamps
#define LARGE_CARD_B		3 //96 Lamps, 16 LEDs - as used by BwB
#define LARGE_CARD_C		4 //Identical to B, no built in LED support

#define CARD_A			1
#define CARD_B			2
#define CARD_C			3

#define TUBES				0
#define HOPPER_DUART_A		1
#define HOPPER_DUART_B		2
#define HOPPER_DUART_C		3
#define HOPPER_NONDUART_A	4
#define HOPPER_NONDUART_B	5

/* Lookup table for CHR data */

struct mpu4_chr_table
{
	UINT8 call;
	UINT8 response;
};

struct bwb_chr_table//dynamically populated table for BwB protection
{
	UINT8 response;
};


class mpu4_state : public driver_device
{
public:
	mpu4_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_vfd(*this, "vfd"),
		  m_6840ptm(*this, "ptm_ic2"),
		  m_pia3(*this, "pia_ic3"),
		  m_pia4(*this, "pia_ic4"),
		  m_pia5(*this, "pia_ic5"),
		  m_pia6(*this, "pia_ic6"),
		  m_pia7(*this, "pia_ic7"),
		  m_pia8(*this, "pia_ic8")
		   { }

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
	{
		return 0;
	}
	optional_device<roc10937_t> m_vfd;
	optional_device<ptm6840_device> m_6840ptm;
	optional_device<pia6821_device> m_pia3;
	optional_device<pia6821_device> m_pia4;
	optional_device<pia6821_device> m_pia5;
	optional_device<pia6821_device> m_pia6;
	optional_device<pia6821_device> m_pia7;
	optional_device<pia6821_device> m_pia8;
	int m_mod_number;
	int m_mmtr_data;
	int m_alpha_data_line;
	int m_alpha_clock;
	int m_ay8913_address;
	int m_serial_data;
	int m_signal_50hz;
	int m_ic4_input_b;
	int m_aux1_input;
	int m_aux2_input;
	int m_IC23G1;
	int m_IC23G2A;
	int m_IC23G2B;
	int m_IC23GC;
	int m_IC23GB;
	int m_IC23GA;
	int m_prot_col;
	int m_lamp_col;
	int m_init_col;
	int m_reel_flag;
	int m_ic23_active;
	int m_led_lamp;
	int m_link7a_connected;
	emu_timer *m_ic24_timer;
	int m_expansion_latch;
	int m_global_volume;
	int m_input_strobe;
	UINT8 m_lamp_strobe;
	UINT8 m_lamp_strobe2;
	UINT8 m_lamp_strobe_ext;
	UINT8 m_lamp_strobe_ext_persistence;
	UINT8 m_led_strobe;
	UINT8 m_ay_data;
	int m_optic_pattern;
	int m_active_reel;
	int m_remote_meter;
	int m_reel_mux;
	int m_lamp_extender;
	int m_last_b7;
	int m_last_latch;
	int m_lamp_sense;
	int m_card_live;
	int m_led_extender;
	int m_bwb_bank;
	int m_chr_state;
	int m_chr_counter;
	int m_chr_value;
	int m_bwb_return;
	int m_pageval;
	int m_pageset;
	int m_hopper;
	int m_reels;
	int m_chrdata;
	int m_t1;
	int m_t3l;
	int m_t3h;
	mpu4_chr_table* m_current_chr_table;
	const bwb_chr_table* m_bwb_chr_table1;

	DECLARE_WRITE8_MEMBER(bankswitch_w);
	DECLARE_READ8_MEMBER(bankswitch_r);
	DECLARE_WRITE8_MEMBER(bankset_w);
	DECLARE_WRITE8_MEMBER(characteriser_w);
	DECLARE_READ8_MEMBER(characteriser_r);
	DECLARE_WRITE8_MEMBER(bwb_characteriser_w);
	DECLARE_READ8_MEMBER(bwb_characteriser_r);
	DECLARE_WRITE8_MEMBER(mpu4_ym2413_w);
	DECLARE_READ8_MEMBER(mpu4_ym2413_r);
	DECLARE_READ8_MEMBER(crystal_sound_r);
	DECLARE_WRITE8_MEMBER(crystal_sound_w);
	DECLARE_WRITE8_MEMBER(ic3ss_w);
	DECLARE_WRITE_LINE_MEMBER(cpu0_irq);
	DECLARE_WRITE8_MEMBER(ic2_o1_callback);
	DECLARE_WRITE8_MEMBER(ic2_o2_callback);
	DECLARE_WRITE8_MEMBER(ic2_o3_callback);
	DECLARE_WRITE8_MEMBER(pia_ic3_porta_w);
	DECLARE_WRITE8_MEMBER(pia_ic3_portb_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic3_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic3_cb2_w);
	DECLARE_WRITE8_MEMBER(pia_ic4_porta_w);
	DECLARE_WRITE8_MEMBER(pia_ic4_portb_w);
	DECLARE_READ8_MEMBER(pia_ic4_portb_r);
	DECLARE_WRITE_LINE_MEMBER(pia_ic4_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic4_cb2_w);
	DECLARE_READ8_MEMBER(pia_ic5_porta_r);
	DECLARE_WRITE8_MEMBER(pia_ic5_porta_w);
	DECLARE_WRITE8_MEMBER(pia_ic5_portb_w);
	DECLARE_READ8_MEMBER(pia_ic5_portb_r);
	DECLARE_WRITE_LINE_MEMBER(pia_ic5_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic5_cb2_w);
	DECLARE_WRITE8_MEMBER(pia_ic6_portb_w);
	DECLARE_WRITE8_MEMBER(pia_ic6_porta_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic6_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic6_cb2_w);
	DECLARE_WRITE8_MEMBER(pia_ic7_porta_w);
	DECLARE_WRITE8_MEMBER(pia_ic7_portb_w);
	DECLARE_READ8_MEMBER(pia_ic7_portb_r);
	DECLARE_WRITE_LINE_MEMBER(pia_ic7_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic7_cb2_w);
	DECLARE_READ8_MEMBER(pia_ic8_porta_r);
	DECLARE_WRITE8_MEMBER(pia_ic8_portb_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic8_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia_ic8_cb2_w);
	DECLARE_WRITE8_MEMBER(pia_gb_porta_w);
	DECLARE_WRITE8_MEMBER(pia_gb_portb_w);
	DECLARE_READ8_MEMBER(pia_gb_portb_r);
	DECLARE_WRITE_LINE_MEMBER(pia_gb_ca2_w);
	DECLARE_WRITE_LINE_MEMBER(pia_gb_cb2_w);
	DECLARE_WRITE8_MEMBER(ic3ss_o1_callback);
	DECLARE_WRITE8_MEMBER(ic3ss_o2_callback);
	DECLARE_WRITE8_MEMBER(ic3ss_o3_callback);
	DECLARE_DRIVER_INIT(m4default_alt);
	DECLARE_DRIVER_INIT(crystali);
	DECLARE_DRIVER_INIT(m4tst2);
	DECLARE_DRIVER_INIT(crystal);
	DECLARE_DRIVER_INIT(m_frkstn);
	DECLARE_DRIVER_INIT(m4default_big);
	DECLARE_DRIVER_INIT(m4default);
	DECLARE_DRIVER_INIT(m_blsbys);
	DECLARE_DRIVER_INIT(m_oldtmr);
	DECLARE_DRIVER_INIT(m4tst);
	DECLARE_DRIVER_INIT(m_ccelbr);
	DECLARE_DRIVER_INIT(m4gambal);
	DECLARE_DRIVER_INIT(m_grtecp);
	DECLARE_DRIVER_INIT(m4debug);
	DECLARE_DRIVER_INIT(m4_showstring);
	DECLARE_DRIVER_INIT(m4_showstring_mod4yam);
	DECLARE_DRIVER_INIT(m4_debug_mod4yam);
	DECLARE_DRIVER_INIT(m4_showstring_mod2);
	DECLARE_DRIVER_INIT(m4_showstring_big);
	DECLARE_DRIVER_INIT(m_grtecpss);
	DECLARE_DRIVER_INIT(connect4);
	DECLARE_DRIVER_INIT(m4altreels);
};

/* mpu4.c, used by mpu4vid.c */
extern void mpu4_config_common(running_machine &machine);
extern void mpu4_stepper_reset(mpu4_state *state);

MACHINE_CONFIG_EXTERN( mpu4_common );
MACHINE_CONFIG_EXTERN( mpu4_common2 );

MACHINE_CONFIG_EXTERN( mod2     );

extern MACHINE_START( mod2     );
extern const ay8910_interface ay8910_config;

INPUT_PORTS_EXTERN( mpu4 );

