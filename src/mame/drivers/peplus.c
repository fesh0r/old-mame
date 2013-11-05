/**********************************************************************************


    PLAYER'S EDGE PLUS (PE+)

    Driver by Jim Stolis.
    Layouts by Stephh.

    Special thanks to smf for I2C EEPROM support.

    --- Technical Notes ---

    Name:    Player's Edge Plus (PP0516) Double Bonus Draw Poker.
    Company: IGT - International Gaming Technology
    Year:    1987

    Hardware:

    CPU =  INTEL 83c02       ; I8052 compatible
    VIDEO = ROCKWELL 6545    ; CRTC6845 compatible
    SND =  AY-3-8912         ; AY8910 compatible

    History:

    This form of video poker machine has the ability to use different game roms.  The operator
    changes the game by placing the rom at U68 on the motherboard.  This driver currently supports
    several PE+ game roms, but should work with all other compatible game roms as far as cpu, video,
    sound, and inputs are concerned.  Some games can share the same color prom and graphic roms,
    but this is not always the case.  It is best to confirm the game, color and graphic combinations.

    The game code runs in two different modes, game mode and operator mode.  Game mode is what a
    normal player would see when playing.  Operator mode is for the machine operator to adjust
    machine settings and view coin counts.  The upper door must be open in order to enter operator
    mode and so it should be mapped to an input bank if you wish to support it.  The operator
    has two additional inputs (jackpot reset and self-test) to navigate with, along with the
    normal buttons available to the player.

    A normal machine keeps all coin counts and settings in a battery-backed ram, and will
    periodically update an external eeprom for an even more secure backup.  This eeprom
    also holds the current game state in order to recover the player from a full power failure.


Additional notes
================

1) What are "set chips" ?

    They are meant to be used after you have already sucessfully put a new game in your machine.
    Lets say you have 'pepp0516' installed and you go through the setup. In a real machine,
    you may want to add a bill validator. The only way to do that is to un-socket the 'pepp0516'
    chip and put in the 'peset038' chip and then reboot the machine. Then this chip's program
    runs and you set the options and put the 'pepp0516' chip back in.

    The only way to simulate this is to fire up the 'pepp0516' game and set it up. Then exit the
    game and copy the pepp0516.nv file to peset038.nv, and then run the 'peset038' program.
    This is because they have to have the same eeprom and cmos data in memory to work. When you
    are done with the 'peset038' program, you copy the peset038.nv file back over the pepp0516.nv .
    'peset038' is just a utility program with one screen and 3 tested inputs.


2) Initialization

  - Method 1 :
      * be sure the door is opened (if not, press 'O' by default)
      * "CMOS DATA" will be displayed
      * press the self-test button (default is 'K')
      * be sure the door is opened (if not, press 'O' by default)
      * "EEPROM DATA" will be displayed
      * press the self-test button (default is 'K')
      * be sure the door is closed (if not, press 'O' by default)

  - Method 2 :
      * be sure the door is opened (if not, press 'O' by default)
      * "CMOS DATA" will be displayed
      * press the self-test button (default is 'K') until a "beep" is heard
      * be sure the door is closed (if not, press 'O' by default)
      * press the jackpot reset button (default is 'L')
      * be sure the door is opened (if not, press 'O' by default)
      * "EEPROM DATA" will be displayed
      * press the self-test button (default is 'K')
      * be sure the door is closed (if not, press 'O' by default)


3) Configuration

  - To configure a game :
      * be sure the door is opened (if not, press 'O' by default)
      * press the self-test button (default is 'K')
      * cycle through the screens with the self-test button (default is 'K')
      * close the door (default is 'O') to go back to the game and save the settings

3a) About the "autohold" feature

    Depending on laws which vary from cities/country, this feature can available or not in the
    "operator mode". By default, it isn't available. To have this feature available in the
    "operator mode", a new chip has to be burnt with a bit set and a new checksum (game ID
    doesn't change though). Each program rom requires a specific address to be set to 0x01
    to enable this option. It is beyond the scope of this driver to provide that information
    for each set that is supported. Only PP0197 & PP0419 have support for Autohold enabled.


Stephh's log (2007.11.28) :
  - Renamed sets :
      * 'peplus'   -> 'pepp0516' (so we get the game ID as for the other games)
  - added generic/default layout, inputs and outputs
  - for each kind of game (poker, bjack, keno, slots) :
      * added two layouts (default is the "Bezel Lamps" for players, the other is "Debug Lamps")
      * added one INPUT_PORT definition
  - for "set chips" :
      * added one fake layout
      * added one fake INPUT_PORT definition

******************************************************************************************************************

Generally speaking for standard PE+ boards:

    Program roms are 64K and read as 27C512  (Jumper E15 is for 64K, E14 is for 32K)
CG Graphics roms are 32K and read as 27C256
      Color PROM are 256 bytes and read as N82S135N (or compatible, IE: DM74LS471)

Board type with program type

Standard PE+
 Program Types:
  BEnnnn Black Jack / 21 games
  KEnnnn Keno
  PPnnnn Poker games. Several different types of poker require specific CG graphics + CAP color prom
  PSnnnn Slot games. Each slot game requires specific CG graphics + CAP color prom

Super PE+
 Program Types
  XPnnnnnn  Poker Programs. Different options for each set, but all use the same XnnnnnnP data roms
   XnnnnnnP Poker Data. Contains poker game + paytable percentages
             Data roms will not work with every Program rom. Incompatible combos report: Incompatible Data EPROM
             X000055P is a good example, it works with 19 XP000xxx Program roms. Others may be as few as 2.
  XMPnnnnn  Multi-Poker Programs. Different options for each set, but all use the same XMnnnnnP data roms
             XMP00002 through XMP00006 & XMP00024 Use the XM000xxP Poker Data
             XMP00014, XMP00017 & XMP00030 Use the WING Board add-on and use the XnnnnnnP Poker Data (Not all are compatible!)
   XMnnnnnP Poker Data. Contains poker game + paytable percentages: Requires specific CG graphics + CAP color prom
  XKnnnnnn  Spot Keno Programs. Different options for each set, but all use the same XnnnnnnK data roms
   XnnnnnnK Spot Keno Data. Uses CG2120 with CAP1267
  XSnnnnnn  Slot Programs. Different options for each set, but all use the same XnnnnnnS data roms
  XnnnnnnT  Slot Program. For Slant (table top) machines??
   XnnnnnnS Slot Data. Each set requires specific CG graphics + CAP color prom

The CG graphics + CAP color proms along with which program sets they belong to is a closely guarded secret by IGT.
 The only public information is from collectors who document and share such information.

NOTE:  Do NOT use the CG+CAP combos listed below as THE definitive absolute reference. There are other combos that
       worked to produce correct card graphics plus paytable information for many sets. So the combos listed below
       are not the "official" combo and a better or more correct combo may exist.

NOTE: XP000035 supports a Tournament mode.  You can toggle back and forth between standard and Tournament mode by
      pressing and holding Jackpot Reset (L key) and pressing Change Request (Y key)

***********************************************************************************/

#include "emu.h"
#include "sound/ay8910.h"
#include "machine/nvram.h"
#include "cpu/mcs51/mcs51.h"
#include "machine/i2cmem.h"
#include "video/mc6845.h"

#include "peplus.lh"
#include "pe_schip.lh"
#include "pe_poker.lh"
#include "pe_bjack.lh"
#include "pe_keno.lh"
#include "pe_slots.lh"


class peplus_state : public driver_device
{
public:
	enum
	{
		TIMER_ASSERT_LP
	};

	peplus_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_cmos_ram(*this, "cmos") ,
		m_program_ram(*this, "prograram"),
		m_s3000_ram(*this, "s3000_ram"),
		m_s5000_ram(*this, "s5000_ram"),
		m_videoram(*this, "videoram"),
		m_s7000_ram(*this, "s7000_ram"),
		m_sb000_ram(*this, "sb000_ram"),
		m_sd000_ram(*this, "sd000_ram"),
		m_sf000_ram(*this, "sf000_ram"),
		m_io_port(*this, "io_port"),
		m_maincpu(*this, "maincpu") { }

	required_shared_ptr<UINT8> m_cmos_ram;
	required_shared_ptr<UINT8> m_program_ram;
	required_shared_ptr<UINT8> m_s3000_ram;
	required_shared_ptr<UINT8> m_s5000_ram;
	required_shared_ptr<UINT8> m_videoram;
	required_shared_ptr<UINT8> m_s7000_ram;
	required_shared_ptr<UINT8> m_sb000_ram;
	required_shared_ptr<UINT8> m_sd000_ram;
	required_shared_ptr<UINT8> m_sf000_ram;
	required_shared_ptr<UINT8> m_io_port;
	tilemap_t *m_bg_tilemap;
	UINT8 m_wingboard;
	UINT8 m_jumper_e16_e17;
	UINT16 m_vid_address;
	UINT8 *m_palette_ram;
	UINT8 *m_palette_ram2;
	UINT64 m_last_cycles;
	UINT8 m_coin_state;
	UINT64 m_last_door;
	UINT8 m_door_open;
	UINT64 m_last_coin_out;
	UINT8 m_coin_out_state;
	int m_sda_dir;
	UINT8 m_bv_state;
	UINT8 m_bv_busy;
	UINT8 m_bv_pulse;
	UINT8 m_bv_denomination;
	UINT8 m_bv_protocol;
	UINT64 m_bv_cycles;
	UINT8 m_bv_last_enable_state;
	UINT8 m_bv_enable_state;
	UINT8 m_bv_enable_count;
	UINT8 m_bv_data_bit;
	UINT8 m_bv_loop_count;
	UINT16 id023_data;
	DECLARE_WRITE8_MEMBER(peplus_bgcolor_w);
	DECLARE_WRITE8_MEMBER(peplus_crtc_display_w);
	DECLARE_WRITE8_MEMBER(peplus_io_w);
	DECLARE_WRITE8_MEMBER(peplus_duart_w);
	DECLARE_WRITE8_MEMBER(peplus_cmos_w);
	DECLARE_WRITE8_MEMBER(peplus_s3000_w);
	DECLARE_WRITE8_MEMBER(peplus_s5000_w);
	DECLARE_WRITE8_MEMBER(peplus_s7000_w);
	DECLARE_WRITE8_MEMBER(peplus_sb000_w);
	DECLARE_WRITE8_MEMBER(peplus_sd000_w);
	DECLARE_WRITE8_MEMBER(peplus_sf000_w);
	DECLARE_WRITE8_MEMBER(peplus_output_bank_a_w);
	DECLARE_WRITE8_MEMBER(peplus_output_bank_b_w);
	DECLARE_WRITE8_MEMBER(peplus_output_bank_c_w);
	DECLARE_READ8_MEMBER(peplus_io_r);
	DECLARE_READ8_MEMBER(peplus_duart_r);
	DECLARE_READ8_MEMBER(peplus_cmos_r);
	DECLARE_READ8_MEMBER(peplus_s3000_r);
	DECLARE_READ8_MEMBER(peplus_s5000_r);
	DECLARE_READ8_MEMBER(peplus_s7000_r);
	DECLARE_READ8_MEMBER(peplus_sb000_r);
	DECLARE_READ8_MEMBER(peplus_sd000_r);
	DECLARE_READ8_MEMBER(peplus_sf000_r);
	DECLARE_READ8_MEMBER(peplus_bgcolor_r);
	DECLARE_READ8_MEMBER(peplus_dropdoor_r);
	DECLARE_READ8_MEMBER(peplus_watchdog_r);
	DECLARE_CUSTOM_INPUT_MEMBER(peplus_input_r);
	DECLARE_WRITE8_MEMBER(peplus_crtc_mode_w);
	DECLARE_WRITE_LINE_MEMBER(crtc_vsync);
	DECLARE_WRITE8_MEMBER(i2c_nvram_w);
	DECLARE_READ8_MEMBER(peplus_input_bank_a_r);
	DECLARE_READ8_MEMBER(peplus_input0_r);
	DECLARE_DRIVER_INIT(peplus);
	DECLARE_DRIVER_INIT(peplussb);
	DECLARE_DRIVER_INIT(peplussbw);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	virtual void machine_reset();
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_peplus(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void peplus_load_superdata(const char *bank_name);
	void peplus_init();
	required_device<cpu_device> m_maincpu;

protected:
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
};

static const UINT8  id_022[8] = { 0x00, 0x01, 0x04, 0x09, 0x13, 0x16, 0x18, 0x00 };
static const UINT16 id_023[8] = { 0x4a6c, 0x4a7b, 0x4a4b, 0x4a5a, 0x4a2b, 0x4a0a, 0x4a19, 0x4a3a };

#define MASTER_CLOCK        XTAL_20MHz
#define CPU_CLOCK           ((MASTER_CLOCK)/2)      /* divided by 2 - 7474 */
#define MC6845_CLOCK        ((MASTER_CLOCK)/8/3)
#define SOUND_CLOCK         ((MASTER_CLOCK)/12)


#define eeprom_NVRAM_SIZE   0x200 // 4k Bit

/* EEPROM is a X2404P 4K-bit Serial I2C Bus */
static const i2cmem_interface i2cmem_interface =
{
	I2CMEM_SLAVE_ADDRESS, 8, eeprom_NVRAM_SIZE
};

/* prototypes */

static MC6845_ON_UPDATE_ADDR_CHANGED(crtc_addr);

static MC6845_INTERFACE( mc6845_intf )
{
	false,                  /* show border area */
	8,                      /* number of pixels per video memory address */
	NULL,                   /* before pixel update callback */
	NULL,                   /* row update callback */
	NULL,                   /* after pixel update callback */
	DEVCB_NULL,             /* callback for display state changes */
	DEVCB_NULL,             /* callback for cursor state changes */
	DEVCB_NULL,             /* HSYNC callback */
	DEVCB_DRIVER_LINE_MEMBER(peplus_state,crtc_vsync),  /* VSYNC callback */
	crtc_addr               /* update address callback */
};


/**************
* Memory Copy *
***************/

void peplus_state::peplus_load_superdata(const char *bank_name)
{
	UINT8 *super_data = memregion(bank_name)->base();

	/* Distribute Superboard Data */
	memcpy(m_s3000_ram, &super_data[0x3000], 0x1000);
	memcpy(m_s5000_ram, &super_data[0x5000], 0x1000);
	memcpy(m_s7000_ram, &super_data[0x7000], 0x1000);
	memcpy(m_sb000_ram, &super_data[0xb000], 0x1000);
	memcpy(m_sd000_ram, &super_data[0xd000], 0x1000);
	memcpy(m_sf000_ram, &super_data[0xf000], 0x1000);
}


/*****************
* Write Handlers *
******************/

WRITE8_MEMBER(peplus_state::peplus_bgcolor_w)
{
	int i;

	for (i = 0; i < machine().total_colors(); i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (~data >> 0) & 0x01;
		bit1 = (~data >> 1) & 0x01;
		bit2 = (~data >> 2) & 0x01;
		r = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* green component */
		bit0 = (~data >> 3) & 0x01;
		bit1 = (~data >> 4) & 0x01;
		bit2 = (~data >> 5) & 0x01;
		g = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* blue component */
		bit0 = (~data >> 6) & 0x01;
		bit1 = (~data >> 7) & 0x01;
		bit2 = 0;
		b = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		palette_set_color(machine(), (15 + (i*16)), MAKE_RGB(r, g, b));
	}
}


/* ROCKWELL 6545 - Transparent Memory Addressing */

static MC6845_ON_UPDATE_ADDR_CHANGED(crtc_addr)
{
	peplus_state *state = device->machine().driver_data<peplus_state>();
	state->m_vid_address = address;
}

WRITE8_MEMBER(peplus_state::peplus_crtc_mode_w)
{
	/* Reset timing logic */
}

void peplus_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_ASSERT_LP:
		downcast<mc6845_device *>((device_t*)ptr)->assert_light_pen_input();
		break;
	default:
		assert_always(FALSE, "Unknown id in peplus_state::device_timer");
	}
}


static void handle_lightpen( mc6845_device *device )
{
	peplus_state *state = device->machine().driver_data<peplus_state>();
	int x_val = device->machine().root_device().ioport("TOUCH_X")->read_safe(0x00);
	int y_val = device->machine().root_device().ioport("TOUCH_Y")->read_safe(0x00);
	const rectangle &vis_area = device->screen().visible_area();
	int xt, yt;

	xt = x_val * vis_area.width() / 1024 + vis_area.min_x;
	yt = y_val * vis_area.height() / 1024 + vis_area.min_y;

	state->timer_set(device->screen().time_until_pos(yt, xt), peplus_state::TIMER_ASSERT_LP, 0, device);
}

WRITE_LINE_MEMBER(peplus_state::crtc_vsync)
{
	mc6845_device *device = machine().device<mc6845_device>("crtc");
	m_maincpu->set_input_line(0, state ? ASSERT_LINE : CLEAR_LINE);
	handle_lightpen(device);
}

WRITE8_MEMBER(peplus_state::peplus_crtc_display_w)
{
	UINT8 *videoram = m_videoram;
	videoram[m_vid_address] = data;
	m_palette_ram[m_vid_address] = m_io_port[1];
	m_palette_ram2[m_vid_address] = m_io_port[3];

	m_bg_tilemap->mark_tile_dirty(m_vid_address);

	/* An access here triggers a device read !*/
	machine().device<mc6845_device>("crtc")->register_r(space, 0);
}

WRITE8_MEMBER(peplus_state::peplus_io_w)
{
	m_io_port[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_duart_w)
{
	// Used for Slot Accounting System Communication
}

WRITE8_MEMBER(peplus_state::peplus_cmos_w)
{
	char bank_name[6];

	/* Test for Wingboard PAL Trigger Condition */
	if (offset == 0x1fff && m_wingboard && data < 5)
	{
		sprintf(bank_name, "user%d", data + 1);
		peplus_load_superdata(bank_name);
	}

	m_cmos_ram[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_s3000_w)
{
	m_s3000_ram[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_s5000_w)
{
	m_s5000_ram[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_s7000_w)
{
	m_s7000_ram[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_sb000_w)
{
	m_sb000_ram[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_sd000_w)
{
	m_sd000_ram[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_sf000_w)
{
	m_sf000_ram[offset] = data;
}

WRITE8_MEMBER(peplus_state::peplus_output_bank_a_w)
{
	output_set_value("pe_bnka0",(data >> 0) & 1); /* Coin Lockout */
	output_set_value("pe_bnka1",(data >> 1) & 1); /* Diverter */
	output_set_value("pe_bnka2",(data >> 2) & 1); /* Bell */
	output_set_value("pe_bnka3",(data >> 3) & 1); /* N/A */
	output_set_value("pe_bnka4",(data >> 4) & 1); /* Hopper 1 */
	output_set_value("pe_bnka5",(data >> 5) & 1); /* Hopper 2 */
	output_set_value("pe_bnka6",(data >> 6) & 1); /* specific to a kind of machine */
	output_set_value("pe_bnka7",(data >> 7) & 1); /* specific to a kind of machine */

	m_coin_out_state = 0;
	if(((data >> 4) & 1) || ((data >> 5) & 1))
		m_coin_out_state = 3;
}

WRITE8_MEMBER(peplus_state::peplus_output_bank_b_w)
{
	output_set_value("pe_bnkb0",(data >> 0) & 1); /* specific to a kind of machine */
	output_set_value("pe_bnkb1",(data >> 1) & 1); /* Deal Spin Start */
	output_set_value("pe_bnkb2",(data >> 2) & 1); /* Cash Out */
	output_set_value("pe_bnkb3",(data >> 3) & 1); /* specific to a kind of machine */
	output_set_value("pe_bnkb4",(data >> 4) & 1); /* Bet 1 / Bet Max */
	output_set_value("pe_bnkb5",(data >> 5) & 1); /* Change Request */
	output_set_value("pe_bnkb6",(data >> 6) & 1); /* Door Open */
	output_set_value("pe_bnkb7",(data >> 7) & 1); /* specific to a kind of machine */
}

WRITE8_MEMBER(peplus_state::peplus_output_bank_c_w)
{
	output_set_value("pe_bnkc0",(data >> 0) & 1); /* Coin In Meter */
	output_set_value("pe_bnkc1",(data >> 1) & 1); /* Coin Out Meter */
	output_set_value("pe_bnkc2",(data >> 2) & 1); /* Coin Drop Meter */
	output_set_value("pe_bnkc3",(data >> 3) & 1); /* Jackpot Meter */
	output_set_value("pe_bnkc4",(data >> 4) & 1); /* Bill Acceptor Enabled */
	output_set_value("pe_bnkc5",(data >> 5) & 1); /* SDS Out */
	output_set_value("pe_bnkc6",(data >> 6) & 1); /* N/A */
	output_set_value("pe_bnkc7",(data >> 7) & 1); /* Game Meter */

	m_bv_enable_state = (data >> 4) & 1;
}

WRITE8_MEMBER(peplus_state::i2c_nvram_w)
{
	device_t *device = machine().device("i2cmem");
	i2cmem_scl_write(device,BIT(data, 2));
	m_sda_dir = BIT(data, 1);
	i2cmem_sda_write(device,BIT(data, 0));
}


/****************
* Read Handlers *
****************/

READ8_MEMBER(peplus_state::peplus_io_r)
{
	return m_io_port[offset];
}

READ8_MEMBER(peplus_state::peplus_duart_r)
{
	// Used for Slot Accounting System Communication
	return 0x00;
}

READ8_MEMBER(peplus_state::peplus_cmos_r)
{
	return m_cmos_ram[offset];
}

READ8_MEMBER(peplus_state::peplus_s3000_r)
{
	return m_s3000_ram[offset];
}

READ8_MEMBER(peplus_state::peplus_s5000_r)
{
	return m_s5000_ram[offset];
}

READ8_MEMBER(peplus_state::peplus_s7000_r)
{
	return m_s7000_ram[offset];
}

READ8_MEMBER(peplus_state::peplus_sb000_r)
{
	return m_sb000_ram[offset];
}

READ8_MEMBER(peplus_state::peplus_sd000_r)
{
	return m_sd000_ram[offset];
}

READ8_MEMBER(peplus_state::peplus_sf000_r)
{
	return m_sf000_ram[offset];
}

/* Last Color in Every Palette is bgcolor */
READ8_MEMBER(peplus_state::peplus_bgcolor_r)
{
	return palette_get_color(machine(), 15); // Return bgcolor from First Palette
}

READ8_MEMBER(peplus_state::peplus_dropdoor_r)
{
	return 0x00; // Drop Door 0x00=Closed 0x02=Open
}

READ8_MEMBER(peplus_state::peplus_watchdog_r)
{
	return 0x00; // Watchdog
}

READ8_MEMBER(peplus_state::peplus_input0_r)
{
/*
        PE+ bill validators have a dip switch setting to switch between ID-022 and ID-023 protocols.

        Emulating IGT IDO22 Pulse Protocol (IGT Smoke 2.2)
        ID022 protocol requires a 20ms on/off pulse x times for denomination followed by a 50ms stop pulse.
        The DBV then waits for at least 3 toggling (ACK) pulses of alternating 20ms each from the game.
        If no toggling received within 200ms, the bill was rejected by the game (e.g. Max Credits reached).
        Once toggling received, the DBV stacks the bill and sends a 10ms stacked pulses.

        Emulating IGT IDO23 Pulse Protocol (IGT 2.5)
        ID023 protocol requires a start pulse of 50ms ON followed by a 20ms pause.  Next a 15-bit data stream
        is sent based on the country code and denomination (see table below).  And finally a 90ms stop pulse.
        There is then a 200ms pause and the entire sequence is transmitted again two more times.
        The DBV then waits for the toggling much like the ID-022 protocol above, however ends with two 10ms
        stack pulses instead of one.

        Ticket handling has not been emulated.

        IDO23 Country Codes
        -------------------
        0x07 = Canada
        0x25 = USA

        IDO23 USA 15-bit Data Samples:
        ---------+--------------+--------------+-----------+
        Bill Amt | Country Code |  Denom Code  |  Checksum |
        ---------+--------------+--------------+-----------+
        $1       | 1 0 0 1 0 1  |  0 0 1 1 0   |  1 1 0 0  |
        $2       | 1 0 0 1 0 1  |  0 0 1 1 1   |  1 0 1 1  |
        $5       | 1 0 0 1 0 1  |  0 0 1 0 0   |  1 0 1 1  |
        $10      | 1 0 0 1 0 1  |  0 0 1 0 1   |  1 0 1 0  |
        $20      | 1 0 0 1 0 1  |  0 0 0 1 0   |  1 0 1 1  |
        $50      | 1 0 0 1 0 1  |  0 0 0 0 0   |  1 0 1 0  |
        $100     | 1 0 0 1 0 1  |  0 0 0 0 1   |  1 0 0 1  |
        Ticket   | 1 0 0 1 0 1  |  0 0 0 1 1   |  1 0 1 0  |
        ---------+--------------+--------------+-----------+

        Direction Data
        --------------
        A (FA) <-- [FRONT OF BILL] --> (FB) B
        D (BB) <-- [BACK OF BILL ] --> (BA) C

        Pulses are currently time via cpu cycles.
        833.3 cycles per millisecond
        10 ms = 8333 cycles
*/
	UINT64 curr_cycles = machine().firstcpu->total_cycles();

	// Allow Bill Insert if DBV Enabled
	if (m_bv_enable_state == 0x01 && ((ioport("DBV")->read_safe(0xff) & 0x01) == 0x00)) {
		// If not busy
		if (m_bv_busy == 0) {
			m_bv_busy = 1;

			// Fetch Current Denomination and Protocol
			m_bv_denomination = ioport("BC")->read();
			m_bv_protocol = ioport("BP")->read();

			if (m_bv_protocol == 0) {
				// ID-022
				m_bv_denomination = id_022[m_bv_denomination];

				if (m_bv_denomination == 0)
					m_bv_state = 0x03; // $1 So Skip Credit Pulse
				else
					m_bv_state = 0x01; // Greater than $1 Needs Credit Pulse
			} else {
				// ID-023
				id023_data = id_023[m_bv_denomination];

				m_bv_data_bit = 14;
				m_bv_loop_count = 0;

				m_bv_state = 0x11;
			}

			m_bv_cycles = curr_cycles;
			m_bv_pulse = 1;
			m_bv_enable_count = 0;
		}
	}

	switch (m_bv_state)
	{
		case 0x00: // Not Active
			m_bv_busy = 0;
			break;
		case 0x01: // Credit Pulse 20ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 20) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 0;
				m_bv_state++;
			}
			break;
		case 0x02: // Credit Pulse 20ms OFF
			if (curr_cycles - m_bv_cycles >= 833.3 * 20) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 1;

				m_bv_denomination--;

				if (m_bv_denomination == 0)
					m_bv_state++; // Done with Credit Pulse
				else
					m_bv_state = 0x01; // Continue Pulsing Denomination
			}
			break;
		case 0x03: // Stop Pulse 50ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 50) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 0;

				// Reset Toggle Details
				m_bv_last_enable_state = m_bv_enable_state;
				m_bv_enable_count = 0;

				m_bv_state++;
			}
			break;
		case 0x04: // Begin Toggle Polling
			if (m_bv_enable_state != m_bv_last_enable_state) {
				m_bv_enable_count++;
				m_bv_last_enable_state = m_bv_enable_state;

				// Got Enough Toggles, Advance to Stacking
				if (m_bv_enable_count == 0x03) {
					m_bv_cycles = curr_cycles;
					m_bv_pulse = 1;
					m_bv_state++;
				}
			} else {
				// No Toggling Found, Game Rejected Bill
				if (curr_cycles - m_bv_cycles >= 833.3 * 200) {
					m_bv_pulse = 0;
					m_bv_state = 0x00;
				}
			}
			break;
		case 0x05: // Stacked Pulse 10ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 10) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 0;
				m_bv_state = 0x00;
			}
			break;
		case 0x11: // Start Pulse 50ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 50) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 0;
				m_bv_state++;
			}
			break;
		case 0x12: // Start Pulse 20ms OFF
			if (curr_cycles - m_bv_cycles >= 833.3 * 20) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 1;
				m_bv_state++;
			}
			break;
		case 0x13: // Data Sync Pulse 20ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 20) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 1 - ((id023_data >> m_bv_data_bit) & 0x01);
				m_bv_state++;
			}
			break;
		case 0x14: // Data Value Pulse 20ms OFF
			if (curr_cycles - m_bv_cycles >= 833.3 * 20) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 1;

				if (m_bv_data_bit == 0) {
					m_bv_data_bit = 14; // Done with Data Stream
					m_bv_state++;
				} else {
					m_bv_data_bit--;
					m_bv_state = 0x13; // More Data Yet
				}
			}
			break;
		case 0x15: // Stop Pulse 90ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 90) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 0;

				if (m_bv_loop_count >= 2) {
					m_bv_state = 0x17; // Done, Ready for Toggling
				} else {
					m_bv_loop_count++;
					m_bv_state++;
				}
			}
			break;
		case 0x16: // Repeat Pulse 200ms OFF
			if (curr_cycles - m_bv_cycles >= 833.3 * 200) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 1;
				m_bv_state = 0x11; // Repeat from Start
			}
			break;
		case 0x17: // Begin Toggle Polling
			if (m_bv_enable_state != m_bv_last_enable_state) {
				m_bv_enable_count++;
				m_bv_last_enable_state = m_bv_enable_state;

				// Got Enough Toggles, Advance to Stacking
				if (m_bv_enable_count == 0x03) {
					m_bv_cycles = curr_cycles;
					m_bv_pulse = 1;
					m_bv_state++;
				}
			} else {
				// No Toggling Found, Game Rejected Bill
				if (curr_cycles - m_bv_cycles >= 833.3 * 200) {
					m_bv_pulse = 0;
					m_bv_state = 0x00;
				}
			}
			break;
		case 0x18: // Stacked Pulse 10ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 10) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 0;
				m_bv_state++;
			}
			break;
		case 0x19: // Stacked Pulse 10ms OFF
			if (curr_cycles - m_bv_cycles >= 833.3 * 10) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 1;
				m_bv_state++;
			}
			break;
		case 0x1a: // Stacked Pulse 10ms ON
			if (curr_cycles - m_bv_cycles >= 833.3 * 10) {
				m_bv_cycles = curr_cycles;
				m_bv_pulse = 0;
				m_bv_state = 0x00;
			}
			break;
	}

	if (m_bv_pulse == 1) {
		return (0x70 || ioport("IN0")->read()); // Add Bill Validator Credit Pulse
	} else {
		return ioport("IN0")->read();
	}
}

READ8_MEMBER(peplus_state::peplus_input_bank_a_r)
{
	device_t *device = machine().device("i2cmem");
/*
        Bit 0 = COIN DETECTOR A
        Bit 1 = COIN DETECTOR B
        Bit 2 = COIN DETECTOR C
        Bit 3 = COIN OUT
        Bit 4 = HOPPER FULL
        Bit 5 = DOOR OPEN
        Bit 6 = LOW BATTERY
        Bit 7 = I2C EEPROM SDA
*/
	UINT8 bank_a = 0x50; // Turn Off Low Battery and Hopper Full Statuses
	UINT8 coin_optics = 0x00;
	UINT8 coin_out = 0x00;
	UINT64 curr_cycles = machine().firstcpu->total_cycles();
	UINT16 door_wait = 500;

	UINT8 sda = 0;
	if(!m_sda_dir)
	{
		sda = i2cmem_sda_read(device);
	}

	if ((ioport("SENSOR")->read_safe(0x00) & 0x01) == 0x01 && m_coin_state == 0) {
		m_coin_state = 1; // Start Coin Cycle
		m_last_cycles = machine().firstcpu->total_cycles();
	} else {
		/* Process Next Coin Optic State */
		if (curr_cycles - m_last_cycles > 600000/6 && m_coin_state != 0) {
			m_coin_state++;
			if (m_coin_state > 5)
				m_coin_state = 0;
			m_last_cycles = machine().firstcpu->total_cycles();
		}
	}

	switch (m_coin_state)
	{
		case 0x00: // No Coin
			coin_optics = 0x00;
			break;
		case 0x01: // Optic A
			coin_optics = 0x01;
			break;
		case 0x02: // Optic AB
			coin_optics = 0x03;
			break;
		case 0x03: // Optic ABC
			coin_optics = 0x07;
			break;
		case 0x04: // Optic BC
			coin_optics = 0x06;
			break;
		case 0x05: // Optic C
			coin_optics = 0x04;
			break;
	}

	if (m_wingboard)
		door_wait = 12345;

	if (curr_cycles - m_last_door > door_wait) {
		if ((ioport("DOOR")->read_safe(0xff) & 0x01) == 0x01) {
			m_door_open = (m_door_open ^ 0x01) & 0x01;
		} else {
			m_door_open = 1;
		}
		m_last_door = machine().firstcpu->total_cycles();
	}

	if (curr_cycles - m_last_coin_out > 600000/12 && m_coin_out_state != 0) { // Guessing with 600000
		if (m_coin_out_state != 2) {
			m_coin_out_state = 2; // Coin-Out Off
		} else {
			m_coin_out_state = 3; // Coin-Out On
		}

		m_last_coin_out = machine().firstcpu->total_cycles();
	}

	switch (m_coin_out_state)
	{
		case 0x00: // No Coin-Out
			coin_out = 0x00;
			break;
		case 0x01: // First Coin-Out On
			coin_out = 0x08;
			break;
		case 0x02: // Coin-Out Off
			coin_out = 0x00;
			break;
		case 0x03: // Additional Coin-Out On
			coin_out = 0x08;
			break;
	}

	bank_a = (sda<<7) | bank_a | (m_door_open<<5) | coin_optics | coin_out;

	return bank_a;
}


/****************************
* Video/Character functions *
****************************/

TILE_GET_INFO_MEMBER(peplus_state::get_bg_tile_info)
{
	UINT8 *videoram = m_videoram;
	int pr = m_palette_ram[tile_index];
	int pr2 = m_palette_ram2[tile_index];
	int vr = videoram[tile_index];

	int code = ((pr & 0x0f)*256) | vr;
	int color = (pr>>4) & 0x0f;

	// Access 2nd Half of CGs and CAP
	if (m_jumper_e16_e17 && (pr2 & 0x10) == 0x10)
	{
		code += 0x1000;
		color += 0x10;
	}

	SET_TILE_INFO_MEMBER(0, code, color, 0);
}

void peplus_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(peplus_state::get_bg_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 40, 25);
	m_palette_ram = auto_alloc_array(machine(), UINT8, 0x3000);
	memset(m_palette_ram, 0, 0x3000);
	m_palette_ram2 = auto_alloc_array(machine(), UINT8, 0x3000);
	memset(m_palette_ram2, 0, 0x3000);
}

UINT32 peplus_state::screen_update_peplus(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);

	return 0;
}

void peplus_state::palette_init()
{
	const UINT8 *color_prom = memregion("proms")->base();
/*  prom bits
    7654 3210
    ---- -xxx   red component.
    --xx x---   green component.
    xx-- ----   blue component.
*/
	int i;

	for (i = 0;i < machine().total_colors();i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = (~color_prom[i] >> 0) & 0x01;
		bit1 = (~color_prom[i] >> 1) & 0x01;
		bit2 = (~color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* green component */
		bit0 = (~color_prom[i] >> 3) & 0x01;
		bit1 = (~color_prom[i] >> 4) & 0x01;
		bit2 = (~color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		/* blue component */
		bit0 = (~color_prom[i] >> 6) & 0x01;
		bit1 = (~color_prom[i] >> 7) & 0x01;
		bit2 = 0;
		b = 0x21 * bit2 + 0x47 * bit1 + 0x97 * bit0;

		palette_set_color(machine(), i, MAKE_RGB(r, g, b));
	}
}


/******************************
* Graphics Decode Information *
******************************/

static GFXDECODE_START( peplus )
			GFXDECODE_ENTRY( "gfx1", 0x00000, gfx_8x8x4_planar, 0, 16 )
GFXDECODE_END


/*************************
* Memory map information *
*************************/

static ADDRESS_MAP_START( peplus_map, AS_PROGRAM, 8, peplus_state )
	AM_RANGE(0x0000, 0xffff) AM_ROM AM_SHARE("prograram")
ADDRESS_MAP_END

static ADDRESS_MAP_START( peplus_iomap, AS_IO, 8, peplus_state )
	// Battery-backed RAM (0x1000-0x01fff Extended RAM for Superboards Only)
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(peplus_cmos_r, peplus_cmos_w) AM_SHARE("cmos")

	// CRT Controller
	AM_RANGE(0x2008, 0x2008) AM_WRITE(peplus_crtc_mode_w)
	AM_RANGE(0x2080, 0x2080) AM_DEVREADWRITE("crtc", mc6845_device, status_r, address_w)
	AM_RANGE(0x2081, 0x2081) AM_DEVREADWRITE("crtc", mc6845_device, register_r, register_w)
	AM_RANGE(0x2083, 0x2083) AM_DEVREAD("crtc", mc6845_device, register_r) AM_WRITE(peplus_crtc_display_w)

	// Superboard Data
	AM_RANGE(0x3000, 0x3fff) AM_READWRITE(peplus_s3000_r, peplus_s3000_w) AM_SHARE("s3000_ram")

	// Sound and Dipswitches
	AM_RANGE(0x4000, 0x4000) AM_DEVWRITE("aysnd", ay8910_device, address_w)
	AM_RANGE(0x4004, 0x4004) AM_READ_PORT("SW1")/* likely ay8910 input port, not direct */ AM_DEVWRITE("aysnd", ay8910_device, data_w)

	// Superboard Data
	AM_RANGE(0x5000, 0x5fff) AM_READWRITE(peplus_s5000_r, peplus_s5000_w) AM_SHARE("s5000_ram")

	// Background Color Latch
	AM_RANGE(0x6000, 0x6000) AM_READ(peplus_bgcolor_r) AM_WRITE(peplus_bgcolor_w)

	// Bogus Location for Video RAM
	AM_RANGE(0x06001, 0x06400) AM_RAM AM_SHARE("videoram")

	// Superboard Data
	AM_RANGE(0x7000, 0x7fff) AM_READWRITE(peplus_s7000_r, peplus_s7000_w) AM_SHARE("s7000_ram")

	// Input Bank A, Output Bank C
	AM_RANGE(0x8000, 0x8000) AM_READ(peplus_input_bank_a_r) AM_WRITE(peplus_output_bank_c_w)

	// Drop Door, I2C EEPROM Writes
	AM_RANGE(0x9000, 0x9000) AM_READ(peplus_dropdoor_r) AM_WRITE(i2c_nvram_w)

	// Input Banks B & C, Output Bank B
	AM_RANGE(0xa000, 0xa000) AM_READ(peplus_input0_r) AM_WRITE(peplus_output_bank_b_w)

	// Superboard Data
	AM_RANGE(0xb000, 0xbfff) AM_READWRITE(peplus_sb000_r, peplus_sb000_w) AM_SHARE("sb000_ram")

	// Output Bank A
	AM_RANGE(0xc000, 0xc000) AM_READ(peplus_watchdog_r) AM_WRITE(peplus_output_bank_a_w)

	// Superboard Data
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(peplus_sd000_r, peplus_sd000_w) AM_SHARE("sd000_ram")

	// DUART
	AM_RANGE(0xe000, 0xe00f) AM_READWRITE(peplus_duart_r, peplus_duart_w)

	// Superboard Data
	AM_RANGE(0xf000, 0xffff) AM_READWRITE(peplus_sf000_r, peplus_sf000_w) AM_SHARE("sf000_ram")

	/* Ports start here */
	AM_RANGE(MCS51_PORT_P0, MCS51_PORT_P3) AM_READ(peplus_io_r) AM_WRITE(peplus_io_w) AM_SHARE("io_port")
ADDRESS_MAP_END


/*************************
*      Input ports       *
*************************/

CUSTOM_INPUT_MEMBER(peplus_state::peplus_input_r)
{
	UINT8 inp_ret = 0x00;
	UINT8 inp_read = ioport((const char *)param)->read();

	if (inp_read & 0x01) inp_ret = 0x01;
	if (inp_read & 0x02) inp_ret = 0x02;
	if (inp_read & 0x04) inp_ret = 0x03;
	if (inp_read & 0x08) inp_ret = 0x04;
	if (inp_read & 0x10) inp_ret = 0x05;
	if (inp_read & 0x20) inp_ret = 0x06;
	if (inp_read & 0x40) inp_ret = 0x07;

	return inp_ret;
}

static INPUT_PORTS_START( peplus )
	/* IN0 has to be defined for each kind of game */
	PORT_START("DOOR")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Upper Door") PORT_CODE(KEYCODE_O) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Lower Door") PORT_CODE(KEYCODE_I)

	PORT_START("SENSOR")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_NAME("Coin In") PORT_IMPULSE(1)

	PORT_START("DBV")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("Bill In") PORT_IMPULSE(1)

	PORT_START("BC")
	PORT_CONFNAME( 0x1f, 0x00, "Bill Choices" )
	PORT_CONFSETTING( 0x00, "$1" )
	PORT_CONFSETTING( 0x01, "$2" )
	PORT_CONFSETTING( 0x02, "$5" )
	PORT_CONFSETTING( 0x03, "$10" )
	PORT_CONFSETTING( 0x04, "$20" )
	PORT_CONFSETTING( 0x05, "$50" )
	PORT_CONFSETTING( 0x06, "$100" )

	PORT_START("BP")
	PORT_CONFNAME( 0x1f, 0x00, "Bill Protocol" )
	PORT_CONFSETTING( 0x00, "ID-022" )
	PORT_CONFSETTING( 0x01, "ID-023" )

	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, "Line Frequency" )
	PORT_DIPSETTING(    0x01, "60Hz" )
	PORT_DIPSETTING(    0x00, "50Hz" )
	PORT_DIPUNUSED( 0x02, IP_ACTIVE_LOW )
	PORT_DIPUNUSED( 0x04, IP_ACTIVE_LOW )
	PORT_DIPUNUSED( 0x08, IP_ACTIVE_LOW )
	PORT_DIPUNUSED( 0x10, IP_ACTIVE_LOW )
	PORT_DIPUNUSED( 0x20, IP_ACTIVE_LOW )
	PORT_DIPUNUSED( 0x40, IP_ACTIVE_LOW )
	PORT_DIPUNUSED( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

static INPUT_PORTS_START( peplus_schip )
	PORT_INCLUDE(peplus)
	PORT_START("IN_BANK1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("IN_BANK2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("IN0")
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK1")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( peplus_poker )
	PORT_INCLUDE(peplus)

	PORT_START("IN_BANK1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_POKER_HOLD1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_POKER_HOLD2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_POKER_HOLD3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_POKER_HOLD4 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_POKER_HOLD5 )

	PORT_START("IN_BANK2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Max Bet") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Play Credit") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Cashout") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Change Request") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) // Bill Acceptor

	PORT_START("IN0")
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK1")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( peplus_bjack )
	PORT_INCLUDE(peplus)

	PORT_START("IN_BANK1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Surrender") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Stand") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Insurance") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Double Down") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Split") PORT_CODE(KEYCODE_B)

	PORT_START("IN_BANK2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Max Bet") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON12 ) PORT_NAME("Play Credit") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON13 ) PORT_NAME("Cashout") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON14 ) PORT_NAME("Change Request") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON15 ) // Bill Acceptor

	PORT_START("IN0")
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK1")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( peplus_keno )
	PORT_INCLUDE(peplus)

	PORT_START("IN_BANK1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Erase") PORT_CODE(KEYCODE_B)

	PORT_START("IN_BANK2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Max Bet") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON12 ) PORT_NAME("Play Credit") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON13 ) PORT_NAME("Cashout") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON14 ) PORT_NAME("Change Request") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON15 ) // Bill Acceptor

	PORT_START("TOUCH_X")
	PORT_BIT( 0xffff, 0x200, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00, 1024) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(13)
	PORT_START("TOUCH_Y")
	PORT_BIT( 0xffff, 0x200, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00, 1024) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(25) PORT_KEYDELTA(13)

	PORT_START("IN0")
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK1")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Light Pen") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( peplus_slots )
	PORT_INCLUDE(peplus)

	PORT_START("IN_BANK1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Jackpot Reset") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Self Test") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("IN_BANK2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Deal-Spin-Start") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Max Bet") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON12 ) PORT_NAME("Play Credit") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON13 ) PORT_NAME("Cashout") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON14 ) PORT_NAME("Change Request") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON15 ) // Bill Acceptor

	PORT_START("IN0")
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK1")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, peplus_state,peplus_input_r, "IN_BANK2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/*************************
*     Machine Reset      *
*************************/

void peplus_state::machine_reset()
{
}

/*************************
*     Machine Driver     *
*************************/

static MACHINE_CONFIG_START( peplus, peplus_state )
	// basic machine hardware
	MCFG_CPU_ADD("maincpu", I80C32, CPU_CLOCK)
	MCFG_CPU_PROGRAM_MAP(peplus_map)
	MCFG_CPU_IO_MAP(peplus_iomap)

	MCFG_NVRAM_ADD_0FILL("cmos")

	// video hardware
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_SIZE((52+1)*8, (31+1)*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 25*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(peplus_state, screen_update_peplus)

	MCFG_GFXDECODE(peplus)
	MCFG_PALETTE_LENGTH(16*16*2)

	MCFG_MC6845_ADD("crtc", R6545_1, "screen", MC6845_CLOCK, mc6845_intf)
	MCFG_I2CMEM_ADD("i2cmem", i2cmem_interface)


	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("aysnd", AY8912, SOUND_CLOCK)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_CONFIG_END



/*************************
*      Driver Init       *
*************************/

/* Normal board */
DRIVER_INIT_MEMBER(peplus_state,peplus)
{
	m_wingboard = FALSE;
	m_jumper_e16_e17 = FALSE;
}

/* Superboard */
DRIVER_INIT_MEMBER(peplus_state,peplussb)
{
	m_wingboard = FALSE;
	m_jumper_e16_e17 = FALSE;
	peplus_load_superdata("user1");
}

/* Superboard with Attached Wingboard */
DRIVER_INIT_MEMBER(peplus_state,peplussbw)
{
	m_wingboard = TRUE;
	m_jumper_e16_e17 = TRUE;
	peplus_load_superdata("user1");
}


/*************************
*        Rom Load        *
*************************/

ROM_START( peset001 ) /* Normal board : Set Chip (Set001) - Use for PP0542 */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "set001.u68",   0x00000, 0x10000, CRC(03397ced) SHA1(89d8ba7e6706e6d34ae9aae09a8a631fff06a36f) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( peset038 ) /* Normal board : Set Chip (Set038) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "set038.u68",   0x00000, 0x10000, CRC(9c4b1d1a) SHA1(8a65cd1d8e2d74c7b66f4dfc73e7afca8458e979) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0043 ) /* Normal board : 10's or Better (PP0043) */
/*
PayTable  10s+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P7B      1     1    3    4    6   9  25  50 300    800
  % Range: 87.2-89.2%  Optimum: 91.2%  Hit Frequency: 49.1%
     Programs Available: PP0043
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0043_a45-a75.u68",   0x00000, 0x10000, CRC(04051a88) SHA1(e7a9ec2ab7f6f575245d47ee10a03f39c887d1b3) ) /* Game Version: A45, Library Version: A74 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0055 ) /* Normal board : Deuces Wild Poker (PP0055) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0055_961-984.u68",   0x00000, 0x10000, CRC(c6b897cc) SHA1(9ba200652db58e602f388c21aaf9b3f837412385) ) /* Game Version: 961, Library Version: 984 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1215.u72",   0x00000, 0x8000, CRC(425f57be) SHA1(6d53ae86bec7189a35671a7f691e101a2ed4d8c4) ) /*  06/09/93   @ IGT  L93-1585  */
	ROM_LOAD( "mgo-cg1215.u73",   0x08000, 0x8000, CRC(0f66cd94) SHA1(9ac0cd01aca87e045c4fd6045ed907a092d6b2ee) )
	ROM_LOAD( "mbo-cg1215.u74",   0x10000, 0x8000, CRC(10f89e44) SHA1(cdc34970b0325a24cfd5c187a4b4dbf42be8fc93) )
	ROM_LOAD( "mxo-cg1215.u75",   0x18000, 0x8000, CRC(73c24e43) SHA1(f09beaf374ad371db2701767ce6ac5bdb13c445a) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0065 ) /* Normal board : Jokers Wild Poker (PP0065) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0065_944-a00.u68",   0x00000, 0x10000, CRC(76c1a367) SHA1(ea8be9241e9925b5a4206db6875e1572f85fa5fe) ) /* Game Version: 944, Library Version: A00 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0103 ) /* Normal board : Deuces Wild Poker 1-100 Coin (PP0103) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0103_600-550.u68",   0x00000, 0x10000, CRC(ba8bb1bf) SHA1(e6f13dd8922696f7ff94dce33fba8dc5bafa9bc6) ) /* Game Version: 600, Library Version: 550, Video Lib Ver: 550 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg773.u72",   0x00000, 0x8000, CRC(73827e49) SHA1(f2b3f58aeac62b36ba60a408cf04c691b0564ace) )
	ROM_LOAD( "mgo-cg773.u73",   0x08000, 0x8000, CRC(af569952) SHA1(d28ae1c216a99bedc4315e61151934f53b932ef4) )
	ROM_LOAD( "mbo-cg773.u74",   0x10000, 0x8000, CRC(3b59799b) SHA1(b6da6e719f5cc475f2f7112d6a8fe346ea5d511e) )
	ROM_LOAD( "mxo-cg773.u75",   0x18000, 0x8000, CRC(75da0cd8) SHA1(4fb4eda9ae8e59884201368c7d8e4ff8b9967a4f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap773.u50", 0x0000, 0x0100, CRC(0b496da9) SHA1(612363cb78f2ab5e100ca4bd0bfae6f7c3f80381) )
ROM_END

ROM_START( pepp0126 ) /* Normal board : Deuces Wild Poker (PP0126) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P63A     1     2    2   3   5   9  12  20 200 250    800
  % Range: 94.9-96.9%  Optimum: 98.9%  Hit Frequency: 45.4%
     Programs Available: PP0126, X000126P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0126_961-984.u68",   0x00000, 0x10000, CRC(aadb62ef) SHA1(85979d5932ef254241c363414c2093cc943e88a5) ) /* Game Version: 961, Library Version: 984 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1215.u72",   0x00000, 0x8000, CRC(425f57be) SHA1(6d53ae86bec7189a35671a7f691e101a2ed4d8c4) ) /*  06/09/93   @ IGT  L93-1585  */
	ROM_LOAD( "mgo-cg1215.u73",   0x08000, 0x8000, CRC(0f66cd94) SHA1(9ac0cd01aca87e045c4fd6045ed907a092d6b2ee) )
	ROM_LOAD( "mbo-cg1215.u74",   0x10000, 0x8000, CRC(10f89e44) SHA1(cdc34970b0325a24cfd5c187a4b4dbf42be8fc93) )
	ROM_LOAD( "mxo-cg1215.u75",   0x18000, 0x8000, CRC(73c24e43) SHA1(f09beaf374ad371db2701767ce6ac5bdb13c445a) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0127 ) /* Normal board : Deuces Joker Wild Poker (PP0127) */
/*
                                         With  w/o  w/o  With
                                         Wild  JKR  Wild JKR
PayTable   3K   STR  FL  FH  4K  SF  5K   RF    4D   RF   4D  (Bonus)
--------------------------------------------------------------------
  P65N     1     2    3   3   3   6   9   12    25  250  1000  2000
  % Range: 95.1-97.1%  Optimum: 99.1%  Hit Frequency: 50.4%
     Programs Available: PP0127
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0127_a47-a76.u68",   0x00000, 0x10000, CRC(2997aaac) SHA1(b52525154f4ae39a341ecf829c33449f31a8ce07) ) /* Game Version: A47, Library Version: A76 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1215.u72",   0x00000, 0x8000, CRC(425f57be) SHA1(6d53ae86bec7189a35671a7f691e101a2ed4d8c4) ) /*  06/09/93   @ IGT  L93-1585  */
	ROM_LOAD( "mgo-cg1215.u73",   0x08000, 0x8000, CRC(0f66cd94) SHA1(9ac0cd01aca87e045c4fd6045ed907a092d6b2ee) )
	ROM_LOAD( "mbo-cg1215.u74",   0x10000, 0x8000, CRC(10f89e44) SHA1(cdc34970b0325a24cfd5c187a4b4dbf42be8fc93) )
	ROM_LOAD( "mxo-cg1215.u75",   0x18000, 0x8000, CRC(73c24e43) SHA1(f09beaf374ad371db2701767ce6ac5bdb13c445a) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0158 ) /* Normal board : 4 of a Kind Bonus Poker (PP0158) - 10/23/95   @ IGT  L95-2438 */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P77A      1    2    3    4    5   8  25  40  80  50 250    800
  % Range: 95.2-97.2%  Optimum: 99.2%  Hit Frequency: 45.5%
     Programs Available: PP0158, X000158P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0158_a46-a75.u68",   0x00000, 0x10000, CRC(5976cd19) SHA1(6a461ea9ddf78dffa3cf8b65903ebf3127f23d45) ) /* Game Version: A46, Library Version: A75, Video Lib ver A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0178 ) /* Normal board : 4 of a Kind Bonus Poker w/ operator selectable special 4 of a Kind (PP0178) */
/*
                                          Aor?
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K*  SF  RF  (Bonus)
-----------------------------------------------------------------
   ??       1    2    3    4    6   9  25  25   50 250    800

* Operator selectable Special 4 of a Kind. Maxbet payout is 250 same as SF at Maxbet

*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0178_645-627.u68",   0x00000, 0x10000, CRC(96cb3b13) SHA1(d8b0621e48b20142d25bf81d07d6716d9858f33e) ) /* Game Version: 645, Library Version: 627 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg954.u72",   0x00000, 0x8000, CRC(1f9cd61e) SHA1(92a2ca3765ad4eeb7ab96538d4278e0a99d16638) )
	ROM_LOAD( "mgo-cg954.u73",   0x08000, 0x8000, CRC(ac1dd15d) SHA1(41ddbb05edc3d274a27d4938c29e6a1e7c785cd7) )
	ROM_LOAD( "mbo-cg954.u74",   0x10000, 0x8000, CRC(cfcb5740) SHA1(8a94536f3b4315c1e6a16a8e5043a80205a3aabe) )
	ROM_LOAD( "mxo-cg954.u75",   0x18000, 0x8000, CRC(94f63723) SHA1(f53867cc1c07235ff2aba1854459085dd0643c89) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap953.u50", 0x0000, 0x0100, CRC(6ece50ad) SHA1(bc5761303b09625850ba50263607d11871ea3ed3) )
ROM_END

ROM_START( pepp0188 ) /* Normal board : Standard Draw Poker (PP0188) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P40A     1     2    3    4    6   9  15  40 250    800
  % Range: 86.5-88.5%  Optimum: 92.5%  Hit Frequency: 45.5%
     Programs Available: PP0188, X000188P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0188_986-a0u.u68",   0x00000, 0x10000, CRC(cf36a53c) SHA1(99b578538ab24d9ff91971b1f77599272d1dbfc6) ) /* Game Version: 986, Library Version: A0U */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0188a ) /* Normal board : Standard Draw Poker (PP0188) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P40A     1     2    3    4    6   9  15  40 250    800
  % Range: 86.5-88.5%  Optimum: 92.5%  Hit Frequency: 45.5%
     Programs Available: PP0188, X000188P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0188_a66-a8h.u68",   0x00000, 0x10000, CRC(72740894) SHA1(5b7109b6cbe67e0a951fd48b4daf09875abb75fc) ) /* Game Version: A66, Library Version: A8H */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0190 ) /* Normal board : Deuces Wild Poker - System Progressive/Non Progressive (No Double-up) (PP0190) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P57A     1     2    3   4   4   8  10  20 200 250    800
  % Range: 92.0-94.0%  Optimum: 96.0%  Hit Frequency: 44.5%
     Programs Available: PP0417, X000417P, X000190P & PP0190 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0190_642-623.u68",   0x00000, 0x10000, CRC(3b8a3203) SHA1(33bd285def754df34f4815cd713e2ff599c74f11) ) /* Game Version: 642, Library Version: 623 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg773.u72",   0x00000, 0x8000, CRC(73827e49) SHA1(f2b3f58aeac62b36ba60a408cf04c691b0564ace) ) /* Should be MxO-CG2023.U7x & CAP773 (or CAP2023)?? */
	ROM_LOAD( "mgo-cg773.u73",   0x08000, 0x8000, CRC(af569952) SHA1(d28ae1c216a99bedc4315e61151934f53b932ef4) )
	ROM_LOAD( "mbo-cg773.u74",   0x10000, 0x8000, CRC(3b59799b) SHA1(b6da6e719f5cc475f2f7112d6a8fe346ea5d511e) )
	ROM_LOAD( "mxo-cg773.u75",   0x18000, 0x8000, CRC(75da0cd8) SHA1(4fb4eda9ae8e59884201368c7d8e4ff8b9967a4f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap773.u50", 0x0000, 0x0100, CRC(0b496da9) SHA1(612363cb78f2ab5e100ca4bd0bfae6f7c3f80381) )
ROM_END

ROM_START( pepp0197 ) /* Normal board : Standard Draw Poker (Auto Hold in options) (PP0197) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   BA      1     2    3    4    5   8  25  50 250   1000
  % Range: 93.8-95.8%  Optimum: 97.8%  Hit Frequency: 45.3%
     Programs Available: PP0197, X000197P & PP0419 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0197_689-654.u68",   0x00000, 0x10000, CRC(84a2fbde) SHA1(0515f2693d388e29cdf0de4d29708250c671e0a4) ) /* Game Version: 689, Library Version: 654 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )


	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0197a ) /* Normal board : Standard Draw Poker (PP0197) - 10/23/95   @ IGT  L95-2452 */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   BA      1     2    3    4    5   8  25  50 250   1000
  % Range: 93.8-95.8%  Optimum: 97.8%  Hit Frequency: 45.3%
     Programs Available: PP0197, X000197P & PP0419 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0197_a45-a75.u68",   0x00000, 0x10000, CRC(6b5b3108) SHA1(2fafcf979db92d4f9f1fb0a2e9645fd71d8dd5c2) ) /* Game Version: A45, Library Version: A75 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )


	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0203 ) /* Normal board : 4 of a Kind Bonus Poker (PP0203) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P90A      1    2    3    4    5   7  25  40  80  50 250    800
  % Range: 94.0-96.0%  Optimum: 98.0%  Hit Frequency: 45.5%
     Programs Available: PP0203, X000203P, PP0590 & PP0409 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0203_813-824.u68",   0x00000, 0x10000, CRC(49ea6fb9) SHA1(ca595e30d786d28397eeef10786a811af1a5c74d) ) /* Game Version: 813, Library Version: 824 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0203a ) /* Normal board : 4 of a Kind Bonus Poker (PP0203) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P90A      1    2    3    4    5   7  25  40  80  50 250    800
  % Range: 94.0-96.0%  Optimum: 98.0%  Hit Frequency: 45.5%
     Programs Available: PP0203, X000203P, PP0590 & PP0409 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0203_a46-a75.u68",   0x00000, 0x10000, CRC(2955eeb5) SHA1(ac2483dbb92de84ab64d0d7e55acff196966ea1b) ) /* Game Version: A46, Library Version: A75, Video Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )


	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0221 ) /* Normal board : Standard Draw Poker (No Double-up) (PP0221) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P11A     1     2    3    4    5   9  25  50 250    800
  % Range: 92.1-94.1%  Optimum: 96.1%  Hit Frequency: 45.5%
     Programs Available: PP0449, X000449P & PP0221 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0221_979_a0c.u68",   0x00000, 0x10000, CRC(c45fb8f1) SHA1(fba8dce2954beb168624ba94b2a4fdd3b260da46) ) /* Game Version: 979, Library Version: A0C */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0224 ) /* Normal board : Deuces Wild Poker (No Double-up) (PP0224) */
	ROM_REGION( 0x10000, "maincpu", 0 )
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P56A     1     2    3   3   4   8  10  20 200 250    800
  % Range: 89.4-91.4%  Optimum: 93.4%  Hit Frequency: 45.1%
     Programs Available: PP0242
*/
	ROM_LOAD( "pp0224_a47-a76.u68",   0x00000, 0x10000, CRC(5d6881ad) SHA1(38953ffadea04df614b14c70177736039495c408) ) /* Game Version: A47, Library Version: A76 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1215.u72",   0x00000, 0x8000, CRC(425f57be) SHA1(6d53ae86bec7189a35671a7f691e101a2ed4d8c4) ) /*  06/09/93   @ IGT  L93-1585  */
	ROM_LOAD( "mgo-cg1215.u73",   0x08000, 0x8000, CRC(0f66cd94) SHA1(9ac0cd01aca87e045c4fd6045ed907a092d6b2ee) )
	ROM_LOAD( "mbo-cg1215.u74",   0x10000, 0x8000, CRC(10f89e44) SHA1(cdc34970b0325a24cfd5c187a4b4dbf42be8fc93) )
	ROM_LOAD( "mxo-cg1215.u75",   0x18000, 0x8000, CRC(73c24e43) SHA1(f09beaf374ad371db2701767ce6ac5bdb13c445a) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0224a ) /* Normal board : Deuces Wild Poker (No Double-up) (PP0224) */
	ROM_REGION( 0x10000, "maincpu", 0 )
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P56A     1     2    3   3   4   8  10  20 200 250    800
  % Range: 89.4-91.4%  Optimum: 93.4%  Hit Frequency: 45.1%
     Programs Available: PP0242
*/

	ROM_LOAD( "pp0224_961-984.u68",   0x00000, 0x10000, CRC(71d5e112) SHA1(528f06ad7ea7e1e297939c7b3ca0bb7faa8ce8c1) ) /* Game Version: 961, Library Version: 984 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1215.u72",   0x00000, 0x8000, CRC(425f57be) SHA1(6d53ae86bec7189a35671a7f691e101a2ed4d8c4) ) /*  06/09/93   @ IGT  L93-1585  */
	ROM_LOAD( "mgo-cg1215.u73",   0x08000, 0x8000, CRC(0f66cd94) SHA1(9ac0cd01aca87e045c4fd6045ed907a092d6b2ee) )
	ROM_LOAD( "mbo-cg1215.u74",   0x10000, 0x8000, CRC(10f89e44) SHA1(cdc34970b0325a24cfd5c187a4b4dbf42be8fc93) )
	ROM_LOAD( "mxo-cg1215.u75",   0x18000, 0x8000, CRC(73c24e43) SHA1(f09beaf374ad371db2701767ce6ac5bdb13c445a) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0230 ) /* Normal board : Standard Draw Poker (PP0230) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0230_715-702.u68",   0x00000, 0x10000, CRC(0272d8d6) SHA1(659f9149ab5e26283eaccd31588183c21c291adb) ) /* Game Version: 715, Library Version: 702 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0235 ) /* Normal board : 4 of a Kind Bonus Poker (PP0235) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0235_a46-a75.u68",   0x00000, 0x10000, CRC(dccb5e2f) SHA1(4c1ff0f79d9441d0b7e8b31f95def1056945eb96) ) /* Game Version: A46, Library Version: A75, Video Lib ver A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0250 ) /* Normal board : Double Down Stud Poker (PP0250) */
/*
PayTable  6s-10s  Js+  2PR  3K   STR  FL  FH  4K  SF   RF  (Bonus)
-----------------------------------------------------------------
  ??        1      2    3    4    6    6  12  50 200  1000   ???
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0250_733-778.u68",   0x00000, 0x10000, CRC(4c919598) SHA1(fe73503c6ccb3c5746fb96be58cd5b740c819713) ) /* Game Version: 733, Library Version: 778 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1019.u72",   0x00000, 0x8000, CRC(9086dc3c) SHA1(639baef8a9b347015d21817d69265700ff205774) ) /* Verified CG set for PP0250 set */
	ROM_LOAD( "mgo-cg1019.u73",   0x08000, 0x8000, CRC(fb538a19) SHA1(1ed480ebdf3ad210511e7e0a0dd4e28466219ae9) )
	ROM_LOAD( "mbo-cg1019.u74",   0x10000, 0x8000, CRC(493bf604) SHA1(9cbce26ed328e6878ec5f6531ea140e1c17e6753) )
	ROM_LOAD( "mxo-cg1019.u75",   0x18000, 0x8000, CRC(064a5c80) SHA1(4d21a7a424258f74d4a1e78c123288799e316228) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0265 ) /* Normal board : 4 of a Kind Bonus Poker (PP0265) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
 P101A      1    2    3    4    5   6  25  40  80  50 250    800
  % Range: 92.5-94.5%  Optimum: 96.9%  Hit Frequency: 45.5%
     Programs Available: PP0265, X000265P, PP0403 & PP0410 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0265_a0n-a23.u68",   0x00000, 0x10000, CRC(7dad6907) SHA1(890bdafb8bf272e513e94c8016c9866ab39f8fa2) ) /* Game Version: A0N, Library Version: A23, Viedo Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0274 ) /* Normal board : Standard Draw Poker (PP0274) */

/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   ??       1    2    3    4    6   9  25  50 250    800
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0274_741-728.u68",   0x00000, 0x10000, CRC(cd0b2890) SHA1(5f859dbc8d747a198c735a2bff42279c874928b0) ) /* Game Version: 741, Library Version: 728, Viedo Lib ver: 728 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0290 ) /* Normal board : Deuces Wild Poker (PP0290) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  13  16  22 200 250    800
  % Range: 92.8-94.8%  Optimum: 96.8%  Hit Frequency: 44.9%
     Programs Available: PP0290
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0290_a0v-a2d.u68",   0x00000, 0x10000, CRC(e2e6451c) SHA1(fa83b7a6d6c4ba1b3ee95b48ab6c3594477e536d) ) /* Game Version: A0V, Library Version: A2D */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1215.u72",   0x00000, 0x8000, CRC(425f57be) SHA1(6d53ae86bec7189a35671a7f691e101a2ed4d8c4) ) /*  06/09/93   @ IGT  L93-1585  */
	ROM_LOAD( "mgo-cg1215.u73",   0x08000, 0x8000, CRC(0f66cd94) SHA1(9ac0cd01aca87e045c4fd6045ed907a092d6b2ee) )
	ROM_LOAD( "mbo-cg1215.u74",   0x10000, 0x8000, CRC(10f89e44) SHA1(cdc34970b0325a24cfd5c187a4b4dbf42be8fc93) )
	ROM_LOAD( "mxo-cg1215.u75",   0x18000, 0x8000, CRC(73c24e43) SHA1(f09beaf374ad371db2701767ce6ac5bdb13c445a) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0291 ) /* Normal board : Deuces Wild Poker (No Double-up) (PP0291) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P62A     1     2    3   4   4   9  15  25 200 250    800
  % Range: 94.9-96.9%  Optimum: 98.9%  Hit Frequency: 44.4%
     Programs Available: PP0418, X000291P & PP0291 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0291_a0v-a2d.u68",   0x00000, 0x10000, CRC(4eabac97) SHA1(dc849bca8ac90536c361cd576ee81c50afd7071b) ) /* Game Version: A0V, Library Version: A2D */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1215.u72",   0x00000, 0x8000, CRC(425f57be) SHA1(6d53ae86bec7189a35671a7f691e101a2ed4d8c4) ) /*  06/09/93   @ IGT  L93-1585  */
	ROM_LOAD( "mgo-cg1215.u73",   0x08000, 0x8000, CRC(0f66cd94) SHA1(9ac0cd01aca87e045c4fd6045ed907a092d6b2ee) )
	ROM_LOAD( "mbo-cg1215.u74",   0x10000, 0x8000, CRC(10f89e44) SHA1(cdc34970b0325a24cfd5c187a4b4dbf42be8fc93) )
	ROM_LOAD( "mxo-cg1215.u75",   0x18000, 0x8000, CRC(73c24e43) SHA1(f09beaf374ad371db2701767ce6ac5bdb13c445a) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0409 ) /* Normal board : 4 of a Kind Bonus Poker (No Double-up) (PP0409) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P90A      1    2    3    4    5   7  25  40  80  50 250    800
  % Range: 94.0-96.0%  Optimum: 98.0%  Hit Frequency: 45.5%
     Programs Available: PP0203, X000203P, PP0590 & PP0409 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0409_a45-a75.u68",   0x00000, 0x10000, CRC(a71fdad9) SHA1(a719787895ad035b2b930d2692930466a9bfb19d) ) /* Game Version: A46, Library Version: A75, Game Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0410 ) /* Normal board : 4 of a Kind Bonus Poker (No Double-up) (PP0410) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
 P101A      1    2    3    4    5   6  25  40  80  50 250    800
  % Range: 92.5-94.5%  Optimum: 96.9%  Hit Frequency: 45.5%
     Programs Available: PP0265, X000265P, PP0403 & PP0410 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0410_a0n-a23.u68",   0x00000, 0x10000, CRC(474f4809) SHA1(8d88647ab4719fdcf6ec0800386f32574b1da66d) ) /* Game Version: A0N, Library Version: A23, Game Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0417 ) /* Normal board : Deuces Wild Poker - System Progressive/Non Progressive (No Double-up) (PP0417) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P57A     1     2    3   4   4   8  10  20 200 250    800
  % Range: 92.0-94.0%  Optimum: 96.0%  Hit Frequency: 44.5%
     Programs Available: PP0417, X000417P & PP0190 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0417_961-984.u68",   0x00000, 0x10000, CRC(4a1e7899) SHA1(b6f243d275da70841482843e05c1be22fd80c25c) ) /* Game Version: 961, Library Version: 984 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2133.u72",   0x00000, 0x8000, CRC(b21a789f) SHA1(c49f9b5f51c29bbc0e1392e86d6602bd44e46380) ) /*  02/02/95   @ IGT  L95-0276  */
	ROM_LOAD( "mgo-cg2133.u73",   0x08000, 0x8000, CRC(2b7db148) SHA1(d5ff5dde3589d28937d13dc5c4c38caa1ebf2d56) )
	ROM_LOAD( "mbo-cg2133.u74",   0x10000, 0x8000, CRC(6ed455b7) SHA1(e4f223606c19d09be501461f38520f423599e0a2) ) /* Supersedes CG1215 graphics set */
	ROM_LOAD( "mxo-cg2133.u75",   0x18000, 0x8000, CRC(095ea26d) SHA1(9bdd8afe67da2370c4ca2d8418f3afdaf7b557ff) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1215.u50", 0x0000, 0x0100, CRC(294b7b10) SHA1(a405a4b8547b713c5c02dacb19e7354095a7b584) )
ROM_END

ROM_START( pepp0419 ) /* Normal board : Standard Draw Poker - Progressive Local/System - Auto Hold in Options (No Double-up) (PP0419) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   BA       1    2    3    4    5   8  25  50 250    800
  % Range: 93.3-95.3%  Optimum: 97.3%  Hit Frequency: 45.5%
     Programs Available: PP0197, X000197P & PP0419 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0419_768-761.u68",   0x00000, 0x10000, CRC(204f9ffe) SHA1(adfdc8b22ba5cc69234ad88ab2f92393a5fb3aff) ) /* Game Version: 768, Library Version: 761, Game Lib ver: 761 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0420 ) /* Normal board : Standard Draw Poker - System Progressive/Non Progressive (No Double-up) (PP0420) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   GA       1    2    3    4    5   6  25  50 250    800
  % Range: 91.0-93.0%  Optimum: 95.0%  Hit Frequency: 45.5%
     Programs Available: PP0060, X000060P & PP0420 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0420_896-914.u68",   0x00000, 0x10000, CRC(cdd923d2) SHA1(f7548159ea3c36c3fce481156ab0293d00f0fd0f) ) /* Game Version: 896, Library Version: 914 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0423 ) /* Normal board : Standard Draw Poker (PP0423) - Progressive Local/System (No Double-up) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P11A      1    2    3    4    5   9  25  50 250    800
  % Range: 92.1-94.1%  Optimum: 96.1%  Hit Frequency: 45.5%
     Programs Available: PP0423
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0423_836-847.u68",   0x00000, 0x10000, CRC(12eec557) SHA1(3721d068223262260ad22698e7dca0440becc53e) ) /* Game Version: 836, Library Version: 847 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg740.u72",   0x00000, 0x8000, CRC(72667f6c) SHA1(89843f472cc0329317cfc643c63bdfd11234b194) )
	ROM_LOAD( "mgo-cg740.u73",   0x08000, 0x8000, CRC(7437254a) SHA1(bba166dece8af58da217796f81117d0b05752b87) )
	ROM_LOAD( "mbo-cg740.u74",   0x10000, 0x8000, CRC(92e8c33e) SHA1(05344664d6fdd3f4205c50fa4ca76fc46c18cf8f) )
	ROM_LOAD( "mxo-cg740.u75",   0x18000, 0x8000, CRC(ce4cbe0b) SHA1(4bafcd68be94a5deaae9661584fa0fc940b834bb) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0426 ) /* Normal board : Joker Poker (No Double-up) (PP0426) */
/*
                                            w/J     w/oJ
PayTable   Ks+  2P  3K  STR  FL  FH  4K  SF  RF  5K  RF  (Bonus)
----------------------------------------------------------------
   YD       1    1   2   3    5   7  15  50 100 200 400    800
  % Range: 92.7-94.7%  Optimum: 96.7%  Hit Frequency: 44.1%
     Programs Available: PP0568 & PP0426 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0426_979-a0c.u68",   0x00000, 0x10000, CRC(0dca4bf1) SHA1(f2cfe250e78fc0995dda653d231f75090ff5c394) ) /* Game Version: 979, Library Version: A0C */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0428 ) /* Normal board : Joker Poker (No Double-up) (PP0428) */
/*
                                            w/J     w/oJ
PayTable   Ks+  2P  3K  STR  FL  FH  4K  SF  RF  5K  RF  (Bonus)
----------------------------------------------------------------
  P17A      1    1   2   3    4   5  20  40 100 200 500    800
  % Range: 91.5-92.5%  Optimum: 995.5%  Hit Frequency: 44.7%
     Programs Available: PP0459, X000459P & PP0428 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0428_979-a0c.u68",   0x00000, 0x10000, CRC(a206a3bd) SHA1(48e1386027308684daee09370b5ee09d9eb645a8) ) /* Game Version: 979, Library Version: A0c */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0429 ) /* Normal board : Joker Poker (No Double-up) (PP0429) */
/*
                                            w/J     w/oJ
PayTable   Ks+  2P  3K  STR  FL  FH  4K  SF  RF  5K  RF  (Bonus)
----------------------------------------------------------------
  P18A      1    1   2   3    5   6  20  50 100 200 500    800
  % Range: 91.5-92.5%  Optimum: 995.5%  Hit Frequency: 44.7%
     Programs Available: PP0458, X000458P & PP0429 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0429_979-a0c.u68",   0x00000, 0x10000, CRC(453484e7) SHA1(6ba80b72cdd8b3bd43c656400591f4d543a9d94f) ) /* Game Version: 979, Library Version: A0C */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0434 ) /* Normal board : Bonus Poker Deluxe (PP0434) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P200A     1    1    3    4    6   8  80  50 250    800
  % Range: 94.5-96.5%  Optimum: 98.5%  Hit Frequency: 45.2%
     Programs Available: PP0434, X000434P & PP0713 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0434_a45-a75.u68",   0x00000, 0x10000, CRC(e5c9ba19) SHA1(9a01457a54a0445a0f32affe2038366e681cada1) ) /* Game Version: A45, Library Version: A74 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0447 ) /* Normal board : Standard Draw Poker (PP0447) */
/*
PayTable   Js+  2PR  3K  STR  FL  FH  4K  SF  RF  (Bonus)
---------------------------------------------------------
  CA        1    2    3   4    6   9  25  50 250    800
  % Range: 95.5-97.5%  Optimum: 99.5%  Hit Frequency: 45.5%
     Programs Available: PP0447, X000447P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0447_a45-a74.u68",   0x00000, 0x10000, CRC(0ef0bb6c) SHA1(d0ef7a83417054f05d32d0a93ed0d5d618f4dfb9) ) /* Game Version: A45, Library Version: A74 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0447a ) /* Normal board : Standard Draw Poker (PP0447) */
/*
PayTable   Js+  2PR  3K  STR  FL  FH  4K  SF  RF  (Bonus)
---------------------------------------------------------
  CA        1    2    3   4    6   9  25  50 250    800
  % Range: 95.5-97.5%  Optimum: 99.5%  Hit Frequency: 45.5%
     Programs Available: PP0447, X000447P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0447_979-a0c.u68",   0x00000, 0x10000, CRC(a62748ea) SHA1(585049160f7983047dd13a1715d1edbf3b9778ea) ) /* Game Version: 979, Library Version: A0C */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0449 ) /* Normal board : Standard Draw Poker (PP0449) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P11A     1     2    3    4    5   9  25  50 250    800
  % Range: 92.1-94.1%  Optimum: 96.1%  Hit Frequency: 45.5%
     Programs Available: PP0449, X000449P & PP0221 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0449_a45-a74.u68",   0x00000, 0x10000, CRC(44699ad7) SHA1(89d9d3107384a6b474c51a34bd34936c36c4b3f5) ) /* Game Version: A45, Library Version: A75 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap740.u50", 0x0000, 0x0100, CRC(6fe619c4) SHA1(49e43dafd010ce0fe9b2a63b96a4ddedcb933c6d) ) /* BPROM type DM74LS471 (compatible with N82S135N) verified */
ROM_END

ROM_START( pepp0452 ) /* Normal board : Double Deuces Wild Poker (PP0452) */
/*
                                        w/D     wo/D
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P236A     1    2    2   3   4  11  16  25 400 250    800
  % Range: 95.6-97.6%  Optimum: 99.6%  Hit Frequency: 45.1%
     Programs Available: PP0452, X000452P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0452_726-706.u68",   0x00000, 0x10000, CRC(26413947) SHA1(66bc2fb3dd62aa9d8ab125665747d331a55e1868) ) /* Game Version: 726, Library Version: 706 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg773.u72",   0x00000, 0x8000, CRC(73827e49) SHA1(f2b3f58aeac62b36ba60a408cf04c691b0564ace) )
	ROM_LOAD( "mgo-cg773.u73",   0x08000, 0x8000, CRC(af569952) SHA1(d28ae1c216a99bedc4315e61151934f53b932ef4) )
	ROM_LOAD( "mbo-cg773.u74",   0x10000, 0x8000, CRC(3b59799b) SHA1(b6da6e719f5cc475f2f7112d6a8fe346ea5d511e) )
	ROM_LOAD( "mxo-cg773.u75",   0x18000, 0x8000, CRC(75da0cd8) SHA1(4fb4eda9ae8e59884201368c7d8e4ff8b9967a4f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap773.u50", 0x0000, 0x0100, CRC(0b496da9) SHA1(612363cb78f2ab5e100ca4bd0bfae6f7c3f80381) )
ROM_END

ROM_START( pepp0454 ) /* Normal board : Bonus Poker Deluxe (PP0454) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P253A     1    1    3    4    5   7  80  50 250    800
  % Range: 92.3-94.3%  Optimum: 96.3%  Hit Frequency: 45.2%
     Programs Available: PP0454, X000454P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0454_a45-a75.u68",   0x00000, 0x10000, CRC(f15f751d) SHA1(6e73625bf3fe0461a171f72ab5478439207516b3) ) /* Game Version: A45, Library Version: A74 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0488 ) /* Normal board : Standard Draw Poker (PP0488) - 01/12/95   @ IGT  L95-0175 */
/*
PayTable   Js+  TP  3K  STR  FL  FH  4K  SF  RF  (Bonus)
--------------------------------------------------------
   ??       1    1   2   4   5    8  ??  50 250    800

NOTE: Will work with the standard CG740 + CAP740 graphics for a non-localized game.
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0488_a30-a4v.u68",   0x00000, 0x10000, CRC(99849f5d) SHA1(643a303beda1c4a4619803071df5e612ab922eb9) ) /* Game Version: A30, Library Version: A4V */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2153.u72",   0x00000, 0x8000, CRC(004c9c8e) SHA1(ec3fa9d2c658de59e722d9979513d6b0c71d5742) ) /*  05/01/95   @ IGT  L95-1123  */
	ROM_LOAD( "mgo-cg2153.u73",   0x08000, 0x8000, CRC(e6843b35) SHA1(2d5219a3cb054ce8b470797c0496c7e24e94ed81) )
	ROM_LOAD( "mbo-cg2153.u74",   0x10000, 0x8000, CRC(e3e28611) SHA1(d040f1df6203dc0bd6a79a391fb90fb930f8dd1a) ) /* Custom Arizona Charlie's Casino graphics */
	ROM_LOAD( "mxo-cg2153.u75",   0x18000, 0x8000, CRC(3ae44f7e) SHA1(00d625b60bffef6ce622cb50a3aa93b92131f578) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap2153.u50", 0x0000, 0x0100, CRC(d02fca7e) SHA1(4384b4238d487b4c763983b27381f4c6c08eb605) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0508 ) /* Normal board : Loose Deuce Poker (PP0508) */
/*
                                       w/D     W/oD
PayTable   3K  STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
-----------------------------------------------------------------
  P313A     1    2   2   3   4   8  12  25 500 250    800
  % Range: 95.2-97.2%  Optimum: 99.2%  Hit Frequency: 45.2%
     Programs Available: PP0508, X000508P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0508_872-888.u68",   0x00000, 0x10000, CRC(41da6c1e) SHA1(75dc178bc48a58ccf7e87d91419c5dcd99af2d58) ) /* Game Version: 872, Library Version: 888 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg773.u72",   0x00000, 0x8000, CRC(73827e49) SHA1(f2b3f58aeac62b36ba60a408cf04c691b0564ace) )
	ROM_LOAD( "mgo-cg773.u73",   0x08000, 0x8000, CRC(af569952) SHA1(d28ae1c216a99bedc4315e61151934f53b932ef4) )
	ROM_LOAD( "mbo-cg773.u74",   0x10000, 0x8000, CRC(3b59799b) SHA1(b6da6e719f5cc475f2f7112d6a8fe346ea5d511e) )
	ROM_LOAD( "mxo-cg773.u75",   0x18000, 0x8000, CRC(75da0cd8) SHA1(4fb4eda9ae8e59884201368c7d8e4ff8b9967a4f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap773.u50", 0x0000, 0x0100, CRC(0b496da9) SHA1(612363cb78f2ab5e100ca4bd0bfae6f7c3f80381) )
ROM_END

ROM_START( pepp0509 ) /* Normal board : Standard Draw Poker (No Double-up) (PP0509) */
/*
PayTable   Js+  TP  3K  STR  FL  FH  4K  SF  RF  (Bonus)
--------------------------------------------------------
   ??       1    2   3    4   6  10  25  50 250    800
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0509_783-779.u68",   0x00000, 0x10000, CRC(a7c9b166) SHA1(3565070b9beba9aa50662253cafafa00f4f5abfa) ) /* Game Version: 782, Library Version: 779 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0514 ) /* Normal board : Double Bonus Poker (PP0514) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P323A     1    1    3   5    7   9  50  80 160  50 250    800
  % Range: 95.1-97.1%  Optimum: 99.1%  Hit Frequency: 43.2%
     Programs Available: PP0514, X000514P & PP0538 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0514_a46-a75.u68",   0x00000, 0x10000, CRC(53ca68c7) SHA1(2c46c89c6347bb8cf80b0ff85daabd0e925c87ec) ) /* Game Version: A46, Library Version: A75, Video Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0514a ) /* Normal board : Double Bonus Poker (PP0514) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P323A     1    1    3   5    7   9  50  80 160  50 250    800
  % Range: 95.1-97.1%  Optimum: 99.1%  Hit Frequency: 43.2%
     Programs Available: PP0514, X000514P & PP0538 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0514_a0n-a0y.u68",   0x00000, 0x10000, CRC(2d54260d) SHA1(cf891c5624331f9b2ef7fbb1e084cfc00f407691) ) /* Game Version: A0N, Library Version: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0515 ) /* Normal board : Double Bonus Poker (PP0515) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P324A     1    1    3   5    7  10  50  80 160  50 250    800
  % Range: 96.2-98.2%  Optimum: 100.2%  Hit Frequency: 43.3%
     Programs Available: PP0515, X000515P & PP0539 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0515_989-974.u68",   0x00000, 0x10000, CRC(bc9654ab) SHA1(ecf2401bfbfcfa22354cde517cf425a8db8ea961) ) /* Game Version: 989, Library Version: 974 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0515a ) /* Normal board : Double Bonus Poker (PP0515) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P324A     1    1    3   5    7  10  50  80 160  50 250    800
  % Range: 96.2-98.2%  Optimum: 100.2%  Hit Frequency: 43.3%
     Programs Available: PP0515, X000515P & PP0539 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0515_813-824.u68",   0x00000, 0x10000, CRC(6c06b0ea) SHA1(7907d10d7290080c33e46c9644f24d1d2fc6b3ff) ) /* Game Version: 813, Library Version: 824 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0516 ) /* Normal board : Double Bonus Poker (PP0516) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P325A     1    2    3   4    5   8  50  80 160  50 250    800
  % Range: 93.8-95.8%  Optimum: 97.8%  Hit Frequency: 44.5%
     Programs Available: PP0516, X000516P & PP0540 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0516_974-a0y.u68",   0x00000, 0x10000, CRC(d9da6e13) SHA1(421678d9cb42daaf5b21074cc3900db914dd26cf) ) /* Game Version: 974, Library Version: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0516a ) /* Normal board : Double Bonus Poker (PP0516) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P325A     1    2    3   4    5   8  50  80 160  50 250    800
  % Range: 93.8-95.8%  Optimum: 97.8%  Hit Frequency: 44.5%
     Programs Available: PP0516, X000516P & PP0540 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0516_a46-a75.u68",   0x00000, 0x10000, CRC(6e226711) SHA1(71930ec43c4b75ca50971242be79459976882546) ) /* Game Version: A46, Library Version: A75 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0538 ) /* Normal board : Double Bonus Poker (No Double-up) (PP0538) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P323A     1    1    3   5    7   9  50  80 160  50 250    800
  % Range: 95.1-97.1%  Optimum: 99.1%  Hit Frequency: 43.2%
     Programs Available: PP0514, X000514P & PP0538 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0538_a46-a75.u68",   0x00000, 0x10000, CRC(f9e8dbe7) SHA1(dd745a48764f7da7314236016bf9c7fa67a78fad) ) /* Game Version: A46, Library Version: A75, Video Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0540 ) /* Normal board : Double Bonus Poker (No Double-up) (PP0540) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P325A     1    2    3   4    5   8  50  80 160  50 250    800
  % Range: 93.8-95.8%  Optimum: 97.8%  Hit Frequency: 44.5%
     Programs Available: PP0516, X000516P & PP0540 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0540_974-a0y.u68",   0x00000, 0x10000, CRC(b39e0c13) SHA1(a0dd05eb2927792efb1f432dbbd4a9a4fd6ad4c9) ) /* Game Version: 974, Library Version: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0542 ) /* Normal board : One Eyed Jacks (PP0542) Use SET001 to set Denomination for this game */
/*
                                             With    w/o
                                             Wild    Wild
PayTable    As  2PR  3K  STR  FL  FH  4K  SF  RF  5K  RF  (Bonus)
-----------------------------------------------------------------
   ??        1   1    1   2    4   5  10  ??  ??  ?? 250    800
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0542_905-923.u68",   0x00000, 0x10000, CRC(f4fe3db5) SHA1(18521a569aae8d89e82f9709edc03badae153dd4) ) /* Game Version: 905, Library Version: 923 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2243.u72",  0x00000, 0x8000, CRC(9c81a78e) SHA1(0c5f91443f051bfab4b1e6b0fde443fbb94aa4ec) )
	ROM_LOAD( "mgo-cg2243.u73",  0x08000, 0x8000, CRC(53ccbf75) SHA1(092406f74b8604f19800c473dd9ec46fe7fc77b2) )
	ROM_LOAD( "mbo-cg2243.u74",  0x10000, 0x8000, CRC(2ae69415) SHA1(275245ffed10fb6ec2ff9dd433c3f41ff07fe5ad) )
	ROM_LOAD( "mxo-cg2243.u75",  0x18000, 0x8000, CRC(fa6f300b) SHA1(ba72c3004e6fd4e78d8385a52ede566cf5143d10) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0568 ) /* Normal board : Joker Poker (PP0568) */
/*
                                            w/J     w/oJ
PayTable   Ks+  2P  3K  STR  FL  FH  4K  SF  RF  5K  RF  (Bonus)
----------------------------------------------------------------
   YD       1    1   1   3    5   7  15  50 100 200 400    800
  % Range: 92.7-94.7%  Optimum: 96.7%  Hit Frequency: 44.1%
     Programs Available: PP0568 & PP0426 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0568_a45-a74.u68",   0x00000, 0x10000, CRC(a1015eef) SHA1(074dcd966a5da6867532c6e90e2bc98404c2247b) ) /* Game Version: A45, Library Version: A74 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0585 ) /* Normal board : Standard Draw Poker (No Double-up) (PP0585) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   ??       1    2    3    4    5   7  25  50 250    800
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0585_956-979.u68",   0x00000, 0x10000, CRC(419633df) SHA1(f755e7b6cdd95444a02d2172789d9d73f31f56c4) ) /* Game Version: 956, Library Version: 979 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2004.u72",  0x00000, 0x8000, CRC(e5e40ea5) SHA1(e0d9e50b30cc0c25c932b2bf444990df1fb2c38c) ) /*  08/31/94   @ IGT  L95-0146  */
	ROM_LOAD( "mgo-cg2004.u73",  0x08000, 0x8000, CRC(12607f1e) SHA1(248e1ecee4e735f5943c50f8c350ca95b81509a7) )
	ROM_LOAD( "mbo-cg2004.u74",  0x10000, 0x8000, CRC(78c3fb9f) SHA1(2b9847c511888de507a008dec981778ca4dbcd6c) ) /* Supersedes CG740 */
	ROM_LOAD( "mxo-cg2004.u75",  0x18000, 0x8000, CRC(5aaa4480) SHA1(353c4ce566c944406fce21f2c5045c856ef7a609) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0725 ) /* Normal board : Double Bonus Poker (PP0725) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P434A     1    1    3   4    6   9  50  80 160  50 250    800
  % Range: 92.4-94.4%  Optimum: 96.4%  Hit Frequency: 44.9%
     Programs Available: PP0725, X000725P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0725_a46-a75.u68",   0x00000, 0x10000, CRC(6679f095) SHA1(4cca3103610a75c8e515957ebea0cd75052a1100) )/* Game Version: A46, Library Version: A75, Video Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0728 ) /* Normal board : Double Bonus Poker (PP0728) - 11/30/95   @ IGT  L96-0247 */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
P437A/4K/5  1    1    3   4    5   6  50  80 160  50 250    800
  % Range: 87.1-89.1%  Optimum: 91.1%  Hit Frequency: 45.0%
     Programs Available: PP0728
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0728_a46-a75.u68",   0x00000, 0x10000, CRC(ecfbdb77) SHA1(072dc5d59b6a705591045162612b5101e85e1b10) )/* Game Version: A46, Library Version: A75, Video Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0760 ) /* Normal board : Double Down Stud Poker (PP0760) */
/*
PayTable  8s-10s  Js+  2PR  3K   STR  FL  FH  4K  SF   RF  (Bonus)
-----------------------------------------------------------------
  ??        1      2    3    4    6    9  12  50 200  1000   ???
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0760_a4v-a6d.u68",   0x00000, 0x10000, CRC(1c26076c) SHA1(612ac66bbb0827b81dc9c6bc23fa7558445481bc) ) /* Game Version: A4V, Library Version: A6D */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2015.u72",   0x00000, 0x8000, CRC(7f73ee5c) SHA1(b6c5d423c8176555c1f32605c328ffbfcf94b656) ) /* Verified CG set for PP0760 set */
	ROM_LOAD( "mgo-cg2015.u73",   0x08000, 0x8000, CRC(de270e0e) SHA1(41b207f9380f623ab64dc42224275cccd43417ee) )
	ROM_LOAD( "mbo-cg2015.u74",   0x10000, 0x8000, CRC(02e623d9) SHA1(4c689293f5c5a8eb0b17861cf433ae1e01d83545) )
	ROM_LOAD( "mxo-cg2015.u75",   0x18000, 0x8000, CRC(0c96b7fc) SHA1(adde93f08db0b957daf77d57a7ab60af3b667f25) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0763 ) /* Normal board : 4 of a Kind Bonus Poker (PP0763) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
 P597A      1    1    3    5    8  10  25  40  80  50 250    800
  % Range: 90.2-92.2%  Optimum: 94.2%  Hit Frequency: 42.7%
     Programs Available: PP0763, X000763P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0763_a46-a75.u68",   0x00000, 0x10000, CRC(8e329f30) SHA1(bf271164a4e90e11630a236fb55c70639bdb3e11) ) /* Game Version: A46, Library Version: A75, Game Lib ver: A0Y */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2003.u72",  0x00000, 0x8000, CRC(0d425f48) SHA1(b60aaf3f4bd76f75f72f6e8dda724bdf795cb521) ) /*  08/30/94   @ IGT  L95-0145  */
	ROM_LOAD( "mgo-cg2003.u73",  0x08000, 0x8000, CRC(add0afc4) SHA1(0519bf2f36cb67140933b2c533e625544f27d16b) )
	ROM_LOAD( "mbo-cg2003.u74",  0x10000, 0x8000, CRC(8649dec0) SHA1(0024d3a8fd85279552910b14b69b225bda93957f) )
	ROM_LOAD( "mxo-cg2003.u75",  0x18000, 0x8000, CRC(904631cd) SHA1(d280a2f16b51a04b3f601db3535980a765c60e6f) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap904.u50", 0x0000, 0x0100, CRC(0eec8336) SHA1(a6585c978dbc2f4f3818e3a5b92f8c28be23c4c0) ) /* BPROM type N82S135N verified */
ROM_END

ROM_START( pepp0775 ) /* Normal board : Unknown Poker (PP0775) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pp0775_a44-a73.u68",   0x00000, 0x10000, CRC(79a56642) SHA1(dfde6c12551e4f12a59e31c14fbfb9edb57e4fac) ) /* Game Version: A44, Library Version: A73 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2129.u72",   0x00000, 0x8000, CRC(f168f9ad) SHA1(acf98a155ccc892ac7c095bbd5538193c444ceb3) ) /* Most Likely WRONG!! But sort of works */
	ROM_LOAD( "mgo-cg2129.u73",   0x08000, 0x8000, CRC(40b6df21) SHA1(7de6ff24db09facae040b889612222a6b1ce1b4e) ) /* use until the correct set is verified! */
	ROM_LOAD( "mbo-cg2129.u74",   0x10000, 0x8000, CRC(508eab4b) SHA1(ba7920c3190302be924ef110b8a5e4c4c38ea535) ) /* CG2003 & CG954 give same results */
	ROM_LOAD( "mxo-cg2129.u75",   0x18000, 0x8000, CRC(e98a0efd) SHA1(eed9229f904a38435cd34e06b2c22fd323d73e2d) ) /* Others are clearly wrong */

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pebe0014 ) /* Normal board : Blackjack (BE0014) */
/* Known to exist:
BE0013 508-544 (Non Double-up)
BE0013 528-A22 (Non Double-up)
BE0014 526-906
BE0014 527-936
BE0017 532-A22
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "be0014_522-a22.u68",   0x00000, 0x10000, CRC(232b32b7) SHA1(a3af9414577642fedc23b4c1911901cd31e9d6e0) ) /* Game Version: 528, Library Version: A22 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2036.u72",  0x00000, 0x8000, CRC(0a168d06) SHA1(7ed4fb5c7bcacab077bcec030f0465c6eaf3ce1c) )
	ROM_LOAD( "mgo-cg2036.u73",  0x08000, 0x8000, CRC(826b4090) SHA1(34390484c0faffe9340fd93d273b9292d09f97fd) )
	ROM_LOAD( "mbo-cg2036.u74",  0x10000, 0x8000, CRC(46aac851) SHA1(28d84b49c6cebcf2894b5a15d935618f84093caa) )
	ROM_LOAD( "mxo-cg2036.u75",  0x18000, 0x8000, CRC(60204a56) SHA1(2e3420da9e79ba304ca866d124788f84861380a7) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap707.u50", 0x0000, 0x0200, CRC(5bfeed62) SHA1(df47a2723a70a7c16fbf03b9f614e9b98751a59e) )
ROM_END

ROM_START( peke1012 ) /* Normal board : Keno 1-10 Spot (KE1012) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ke1012.u68",   0x00000, 0x10000, CRC(470e8c10) SHA1(f8a65a3a73477e9e9d2f582eeefa93b470497dfa) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1267.u72",  0x00000, 0x8000, CRC(16498b57) SHA1(9c22726299af7204c4be1c6d8afc4c1b512ad918) )
	ROM_LOAD( "mgo-cg1267.u73",  0x08000, 0x8000, CRC(80847c5a) SHA1(8422cd13a91c3c462af5efcfca8615e7eeaa2e52) )
	ROM_LOAD( "mbo-cg1267.u74",  0x10000, 0x8000, CRC(ce7af8a7) SHA1(38675122c764b8fa9260246ea99ac0f0750da277) )
	ROM_LOAD( "mxo-cg1267.u75",  0x18000, 0x8000, CRC(a4394303) SHA1(30a07028de35f74cc4fb776b0505ca743c8d7b5b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap1267.u50", 0x0000, 0x0200, CRC(3dac264f) SHA1(e9c9de42ffd64d4463bee6fa10886a53bc062ff8) )
ROM_END

ROM_START( peke1013 ) /* Normal board : Keno 2-10 Spot (KE1013) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ke1013.u68",   0x00000, 0x10000, CRC(3b178f94) SHA1(c601150a728d750b73f949ba6e2d2979c4c4be2e) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1267.u72",  0x00000, 0x8000, CRC(16498b57) SHA1(9c22726299af7204c4be1c6d8afc4c1b512ad918) )
	ROM_LOAD( "mgo-cg1267.u73",  0x08000, 0x8000, CRC(80847c5a) SHA1(8422cd13a91c3c462af5efcfca8615e7eeaa2e52) )
	ROM_LOAD( "mbo-cg1267.u74",  0x10000, 0x8000, CRC(ce7af8a7) SHA1(38675122c764b8fa9260246ea99ac0f0750da277) )
	ROM_LOAD( "mxo-cg1267.u75",  0x18000, 0x8000, CRC(a4394303) SHA1(30a07028de35f74cc4fb776b0505ca743c8d7b5b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap1267.u50", 0x0000, 0x0200, CRC(3dac264f) SHA1(e9c9de42ffd64d4463bee6fa10886a53bc062ff8) )
ROM_END

ROM_START( peps0014 ) /* Normal board : Super Joker Slots (PS0014) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0014.u68",   0x00000, 0x10000, CRC(368c3f58) SHA1(ebefcefbb5386659680719936bff72ad61087343) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0916.u72",  0x00000, 0x8000, CRC(d97049d9) SHA1(78f7bb33866ca92922a8b83d5f9ac459edd39176) )
	ROM_LOAD( "mgo-cg0916.u73",  0x08000, 0x8000, CRC(6e075788) SHA1(e8e9d8b7943d62e31d1d58f870bc765cba65c203) )
	ROM_LOAD( "mbo-cg0916.u74",  0x10000, 0x8000, CRC(a5cdf0f3) SHA1(23b2749fd2cb5b8462ce7c912005779b611f32f9) )
	ROM_LOAD( "mxo-cg0916.u75",  0x18000, 0x8000, CRC(1f3a2d72) SHA1(8e07324d436980b628e007d30a835757c1f70f6d) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0916.u50", 0x0000, 0x0200, CRC(c9a4f87c) SHA1(3c7c53fbf7573f07b334e0529bfd7ccf8d5339b5) )
ROM_END

ROM_START( peps0021 ) /* Normal board : Red White & Blue Slots (PS0021) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0021.u68",   0x00000, 0x10000, CRC(e87d5040) SHA1(e7478e845c888d97190f0398da4bfb043222a3c1) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0022 ) /* Normal board : Red White & Blue Slots (PS0022) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0022.u68",   0x00000, 0x10000, CRC(d65c0939) SHA1(d91f472a43f77f9df8845e97561540f988e522e3) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0042 ) /* Normal board : Double Diamond Slots (PS0042) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0042.u68",   0x00000, 0x10000, CRC(b891f04b) SHA1(e735de918e6d91fd87cc85ff40f187dc421a8cf2) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1003.u72",  0x00000, 0x8000, CRC(41ce0395) SHA1(ae90dbae30e4efed33f83ee7038fb2e5171c1945) )
	ROM_LOAD( "mgo-cg1003.u73",  0x08000, 0x8000, CRC(5a383fa1) SHA1(27b1febbdda7332e8d474fc0cca683f451a07090) )
	ROM_LOAD( "mbo-cg1003.u74",  0x10000, 0x8000, CRC(5ec00224) SHA1(bb70a4326cd1810b200e193a449061df62085f37) )
	ROM_LOAD( "mxo-cg1003.u75",  0x18000, 0x8000, CRC(2ffacd52) SHA1(38126ac4998806a1ddd55e6aa1942044240d41d0) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap1003.u50", 0x0000, 0x0200, CRC(1fb7b69f) SHA1(cdb609f39ef1ca0ddf389a599f799c269c7163f9) )
ROM_END

ROM_START( peps0043 ) /* Normal board : Double Diamond Slots (PS0043) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0043.u68",   0x00000, 0x10000, CRC(d612429c) SHA1(95eb4774482a930066456d517fb2e4f67d4df4cb) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1003.u72",  0x00000, 0x8000, CRC(41ce0395) SHA1(ae90dbae30e4efed33f83ee7038fb2e5171c1945) )
	ROM_LOAD( "mgo-cg1003.u73",  0x08000, 0x8000, CRC(5a383fa1) SHA1(27b1febbdda7332e8d474fc0cca683f451a07090) )
	ROM_LOAD( "mbo-cg1003.u74",  0x10000, 0x8000, CRC(5ec00224) SHA1(bb70a4326cd1810b200e193a449061df62085f37) )
	ROM_LOAD( "mxo-cg1003.u75",  0x18000, 0x8000, CRC(2ffacd52) SHA1(38126ac4998806a1ddd55e6aa1942044240d41d0) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap1003.u50", 0x0000, 0x0200, CRC(1fb7b69f) SHA1(cdb609f39ef1ca0ddf389a599f799c269c7163f9) )
ROM_END

ROM_START( peps0045 ) /* Normal board : Red White & Blue Slots (PS0045) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0045.u68",   0x00000, 0x10000, CRC(de180b84) SHA1(0d592d7d535b0aacbd62c18ac222da770fab7b85) ) /* 3 Coins Max / 3 Lines */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0047 ) /* Normal board : Wild Cherry (PS0047) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0047.u68",   0x00000, 0x10000, CRC(b7df1cf8) SHA1(5c5392b7b3a387ccb45fe96310b47078215f2ea0) ) /* 2 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1004.u72",  0x00000, 0x3000, BAD_DUMP CRC(631ca70e) SHA1(c4d9c4ebc9e90bd1704f154f1bf9b0ce91af35b4) ) /* These should each be 0x8000 bytes */
	ROM_LOAD( "mgo-cg1004.u73",  0x08000, 0x1000, BAD_DUMP CRC(28b4f718) SHA1(91ca3ebf288bb60f43fb0e7aace1f2ada2e978ba) ) /* Needs to redumped as standard 27C256 roms */
	ROM_LOAD( "mbo-cg1004.u74",  0x10000, 0x1000, BAD_DUMP CRC(542a3a45) SHA1(13569e5bac44c2cffd647c27cf40456494d4612e) )
	ROM_LOAD( "mxo-cg1004.u75",  0x18000, 0x1000, BAD_DUMP CRC(20242083) SHA1(f9c9bbe559516f1d02cd4f0bab69f0f7765780ca) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1004.u50", 0x0000, 0x0100, CRC(5eced808) SHA1(b40b8efa8cbc76cff7560c36939275eb360c6f11) )
ROM_END

ROM_START( peps0092 ) /* Normal board : Wild Cherry (PS0092) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0092.u68",   0x00000, 0x10000, CRC(d533f6d5) SHA1(9c470f7c474022445aeb45ee8c5757d1b6957a91) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1004.u72",  0x00000, 0x3000, BAD_DUMP CRC(631ca70e) SHA1(c4d9c4ebc9e90bd1704f154f1bf9b0ce91af35b4) ) /* These should each be 0x8000 bytes */
	ROM_LOAD( "mgo-cg1004.u73",  0x08000, 0x1000, BAD_DUMP CRC(28b4f718) SHA1(91ca3ebf288bb60f43fb0e7aace1f2ada2e978ba) ) /* Needs to redumped as standard 27C256 roms */
	ROM_LOAD( "mbo-cg1004.u74",  0x10000, 0x1000, BAD_DUMP CRC(542a3a45) SHA1(13569e5bac44c2cffd647c27cf40456494d4612e) )
	ROM_LOAD( "mxo-cg1004.u75",  0x18000, 0x1000, BAD_DUMP CRC(20242083) SHA1(f9c9bbe559516f1d02cd4f0bab69f0f7765780ca) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "cap1004.u50", 0x0000, 0x0100, CRC(5eced808) SHA1(b40b8efa8cbc76cff7560c36939275eb360c6f11) )
ROM_END

ROM_START( peps0206 ) /* Normal board : Red White & Blue Slots (PS0206) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0206.u68",   0x00000, 0x10000, CRC(e165efc0) SHA1(170f917740c63b0b00f424ce02bfd04dc48a1397) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0207 ) /* Normal board : Red White & Blue Slots (PS0207) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0207.u68",   0x00000, 0x10000, CRC(e7c5f103) SHA1(6e420c151e07863b21a423f8743da360d6389cde) ) /* 3 Coins Max / 3 Lines */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0298 ) /* Normal board : Double Diamond Slots (PS0298) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0298.u68",   0x00000, 0x10000, CRC(3af2eb50) SHA1(1b2e1036f78658da3821bcf88a48b5068b2421b2) ) /* 5 Coins Max / 5 Lines */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg1003.u72",  0x00000, 0x8000, CRC(41ce0395) SHA1(ae90dbae30e4efed33f83ee7038fb2e5171c1945) )
	ROM_LOAD( "mgo-cg1003.u73",  0x08000, 0x8000, CRC(5a383fa1) SHA1(27b1febbdda7332e8d474fc0cca683f451a07090) )
	ROM_LOAD( "mbo-cg1003.u74",  0x10000, 0x8000, CRC(5ec00224) SHA1(bb70a4326cd1810b200e193a449061df62085f37) )
	ROM_LOAD( "mxo-cg1003.u75",  0x18000, 0x8000, CRC(2ffacd52) SHA1(38126ac4998806a1ddd55e6aa1942044240d41d0) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap1003.u50", 0x0000, 0x0200, CRC(1fb7b69f) SHA1(cdb609f39ef1ca0ddf389a599f799c269c7163f9) )
ROM_END

ROM_START( peps0308 ) /* Normal board : Double Jackpot Slots (PS0308) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0308.u68",   0x00000, 0x10000, CRC(fe30e081) SHA1(d216cbc6336727caf359e6b178c856ab2659cabd) ) /* 5 Coins Max / 5 Lines */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0911.u72",  0x00000, 0x8000, CRC(48491b50) SHA1(9ec6d3ff34a08d40082a1347a46635838fd31afc) )
	ROM_LOAD( "mgo-cg0911.u73",  0x08000, 0x8000, CRC(c1ff7d97) SHA1(78ab138ae9c7f9b3352f9b1ef5fbc473993bb8c8) )
	ROM_LOAD( "mbo-cg0911.u74",  0x10000, 0x8000, CRC(202e0f9e) SHA1(51421dfd1b00a9e3b1e938d5bffaa3b7cd4c2b5e) )
	ROM_LOAD( "mxo-cg0911.u75",  0x18000, 0x8000, CRC(d97740a2) SHA1(d76926d7fbbc24d2384a1079cb97e654600b134b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0911.u50", 0x0000, 0x0200, CRC(79dc19c0) SHA1(9ebf998b73c3390cbb957b3dd3fec57b3c70a06d) )
ROM_END

ROM_START( peps0364 ) /* Normal board : Red White & Blue Slots (PS0364) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0364.u68",   0x00000, 0x10000, CRC(596c4ae4) SHA1(a06626fb7d17fd12c7514d435031924973e4ba55) ) /* 3 Coins Max / 1 Line (show alt graphics??) */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0581 ) /* Normal board : Red White & Blue Slots (PS0581) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0581.u68",   0x00000, 0x10000, CRC(8730cbf3) SHA1(2e81aec6982909511a9782f60ff506215f9aac7c) ) /* 5 Coins Max / 5 Lines */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0615 ) /* Normal board : Chaos Slots (PS0615) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0615.u68",   0x00000, 0x10000, CRC(d27dd6ab) SHA1(b3f065f507191682edbd93b07b72ed87bf6ae9b1) ) /* 3 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2246.u72",  0x00000, 0x8000, CRC(7c08c355) SHA1(2a154b81c6d9671cea55a924bffb7f5461747142) )
	ROM_LOAD( "mgo-cg2246.u73",  0x08000, 0x8000, CRC(b3c16487) SHA1(c97232fadd086f604eaeb3cd3c2d1c8fe0dcfa70) )
	ROM_LOAD( "mbo-cg2246.u74",  0x10000, 0x8000, CRC(e61331f5) SHA1(4364edc625d64151cbae40780b54cb1981086647) )
	ROM_LOAD( "mxo-cg2246.u75",  0x18000, 0x8000, CRC(f0f4a27d) SHA1(3a10ab196aeaa5b50d47b9d3c5b378cfadd6fe96) )

	ROM_REGION( 0x200, "proms", 0 ) // WRONG CAP
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) ) /* WRONG!! - Should be CAP2246 here */
ROM_END

ROM_START( peps0631 ) /* Normal board : Red White & Blue Slots (PS0631) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0631.u68",   0x00000, 0x10000, CRC(3d4c52dd) SHA1(f6b31a77de52c0d6402b51349f34bdb687b27178) ) /* 3 Coins Max / 1 Line (show alt graphics??) */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg0960.u72",  0x00000, 0x8000, CRC(8c38c6fd) SHA1(5d6e9ac18b9b3f1253bba080bef1c067b2fdd7a8) )
	ROM_LOAD( "mgo-cg0960.u73",  0x08000, 0x8000, CRC(b4f44163) SHA1(1bc635a5160fdff2882c8362644aebf983a1a427) )
	ROM_LOAD( "mbo-cg0960.u74",  0x10000, 0x8000, CRC(8057e3a8) SHA1(5510872b1607daaf890603e76a8a47680e639e8e) )
	ROM_LOAD( "mxo-cg0960.u75",  0x18000, 0x8000, CRC(d57b4c25) SHA1(6ddfbaae87f9958642ddb95e581ac31e1dd55608) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap0960.u50", 0x0000, 0x0200, CRC(83d67070) SHA1(4c50abbe750dbd4a461084b0bfc51e38df97e421) )
ROM_END

ROM_START( peps0716 ) /* Normal board : River Gambler Slots (PS0716) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ps0716.u68",   0x00000, 0x10000, CRC(7615d7b6) SHA1(91fe62eec720a0dc2ebf48835065148f19499d16) ) /* 2 Coins Max / 1 Line */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2266.u72",  0x00000, 0x8000, CRC(590accd8) SHA1(4e1c963c50799eaa49970e25ecf9cb01eb6b09e1) )
	ROM_LOAD( "mgo-cg2266.u73",  0x08000, 0x8000, CRC(b87ffa05) SHA1(92126b670b9cabeb5e2cc35b6e9c458088b18eea) )
	ROM_LOAD( "mbo-cg2266.u74",  0x10000, 0x8000, CRC(e3df30e1) SHA1(c7d2ae9a7c7e53bfb6197b635efcb5dc231e4fe0) )
	ROM_LOAD( "mxo-cg2266.u75",  0x18000, 0x8000, CRC(56271442) SHA1(61ad0756b9f6412516e46ef6625a4c3899104d4e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap2266.u50", 0x0000, 0x0200, CRC(ae8b52ac) SHA1(f58d40ee77d7f432dfe5f37954e43cab654c9a4c) )
ROM_END

ROM_START( pex0055p ) /* Superboard : Deuces Wild Poker (X000055P+XP000019) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000019.u67",   0x00000, 0x10000, CRC(8ac876eb) SHA1(105b4aee2692ccb20795586ccbdf722c59db66cf) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pa ) /* Superboard : Deuces Wild Poker (X000055P+XP000022) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000022.u67",   0x00000, 0x10000, CRC(7930741b) SHA1(d3b83fd08a458cc794301ef612f8c7b13d4b2050) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pb ) /* Superboard : Deuces Wild Poker (X000055P+XP000023) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000023.u67",   0x00000, 0x10000, CRC(d2ad7dd3) SHA1(9c5fe5ca5a5a682566e96c6802b7164730cda919) ) /* No Double-up */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pc ) /* Superboard : Deuces Wild Poker (X000055P+XP000028) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000028.u67",   0x00000, 0x10000, CRC(1407fe54) SHA1(4615efbba9a58698e2cfd53c93fa133678101441) ) /* Works */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pd ) /* Superboard : Deuces Wild Poker (X000055P+XP000035) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000035.u67",   0x00000, 0x10000, CRC(aa16e53b) SHA1(5a37c7af2c09be26e8734b36da765fd408754771) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pe ) /* Superboard : Deuces Wild Poker (X000055P+XP000038) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pf ) /* Superboard : Deuces Wild Poker (X000055P+XP000040) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000040.u67",   0x00000, 0x10000, CRC(7b30b1d5) SHA1(394c964cf6269a4cd9b9debe8f4a5a0c96db06a7) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pg ) /* Superboard : Deuces Wild Poker (X000055P+XP000053) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000053.u67",   0x00000, 0x10000, CRC(f4f1f986) SHA1(84cfc2c4a10ed24d3a971fe75041a4108ec1d7f2) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055ph ) /* Superboard : Deuces Wild Poker (X000055P+XP000055) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000055.u67",   0x00000, 0x10000, CRC(339821e0) SHA1(127d4eff01136feaf1e3242d57433349afb7b6ca) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pi ) /* Superboard : Deuces Wild Poker (X000055P+XP000063) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000063.u67",   0x00000, 0x10000, CRC(008dcaf9) SHA1(34203f602a531c1a58febdf31fe7a94c2c09fcb4) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pj ) /* Superboard : Deuces Wild Poker (X000055P+XP000075) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000075.u67",   0x00000, 0x10000, CRC(79b3013f) SHA1(98e6f2c7756643bc9c2371c015cba7ed2c314a60) ) /* English / Spanish */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) ) /* Contains needed English / Spanish graphics */
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pk ) /* Superboard : Deuces Wild Poker (X000055P+XP000079) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000079.u67",   0x00000, 0x10000, CRC(fe9757b7) SHA1(8547f00f23e2e3cd4b36d006b760eca6a19f0710) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pl ) /* Superboard : Deuces Wild Poker (X000055P+XP000094) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000094.u67",   0x00000, 0x10000, CRC(97ff8171) SHA1(ca714f201a7425df81b830698f65640570ac5935) ) /* Coupon Compatible */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pm ) /* Superboard : Deuces Wild Poker (X000055P+XP000095) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000095.u67",   0x00000, 0x10000, CRC(6a1679ea) SHA1(421e8c9eacc8e397267a48cad7ae96f541b1c19a) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pn ) /* Superboard : Deuces Wild Poker (X000055P+XP000098) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000098.u67",   0x00000, 0x10000, CRC(12257ad8) SHA1(8f613377519850f8f711ccb827685dece018c735) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055po ) /* Superboard : Deuces Wild Poker (X000055P+XP000102) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000102.u67",   0x00000, 0x10000, CRC(76d37639) SHA1(c7190ee3bff135b39ce42428eadef3ca067508b4) ) /* English / Spanish */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) ) /* Contains needed English / Spanish graphics */
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pp ) /* Superboard : Deuces Wild Poker (X000055P+XP000104) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000104.u67",   0x00000, 0x10000, CRC(53dbef8a) SHA1(0be0d0f0ac33ae86e79ba7a1151a281774f80af9) ) /* Works */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pq ) /* Superboard : Deuces Wild Poker (X000055P+XP000112) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) ) /* No Double-up */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0055pr ) /* Superboard : Deuces Wild Poker (X000055P+XP000126) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P32A     1     2    2   3   4  10  15  25 200 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 45.1%
     Programs Available: PP0055, X000055P, PP0723
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000126.u67",   0x00000, 0x10000, CRC(e41685ac) SHA1(a81ad3f352eebcc0684fc20c73a6935288207215) ) /* English / Spanish */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x010000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) ) /* Contains needed English / Spanish graphics */
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0158p ) /* Superboard : 4 of a Kind Bonus Poker (X000158P+XP000038) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P77A      1    2    3    4    5   8  25  40  80  50 250    800
  % Range: 95.2-97.2%  Optimum: 99.2%  Hit Frequency: 45.5%
     Programs Available: PP0158, X000158P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000158p.u66",   0x00000, 0x010000, CRC(51a8a294) SHA1(f76992729ceaca18af82ab2fb3403dc5a48b7e8a) ) /* 4 of a Kind Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0188p ) /* Superboard : Standard Draw Poker (X000188P+XP000038) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P40A     1     2    3    4    6   9  15  40 250    800
  % Range: 86.5-88.5%  Optimum: 92.5%  Hit Frequency: 45.5%
     Programs Available: PP0188, X000188P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000188p.u66",   0x00000, 0x010000, CRC(3eb7580e) SHA1(86f2280542fb8a55767efd391d0fb04a12ed9408) ) /* Standard Draw Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0190p ) /* Superboard : Deuces Wild Poker (X000190P+XP000053) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P57A     1     2    3   4   4   8  10  20 200 250    800
  % Range: 92.0-94.0%  Optimum: 96.0%  Hit Frequency: 44.5%
     Programs Available: PP0417, X000417P, X000190P & PP0190 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000053.u67",   0x00000, 0x10000, CRC(f4f1f986) SHA1(84cfc2c4a10ed24d3a971fe75041a4108ec1d7f2) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000190p.u66",   0x00000, 0x010000, CRC(17cc52f6) SHA1(90818df05c6bba3ffcbc2047c7eedea31abdc05a) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0197p ) /* Superboard : Standard Draw Poker (X000197P+XP000038) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   BA      1     2    3    4    5   8  25  50 250   1000
  % Range: 93.8-95.8%  Optimum: 97.8%  Hit Frequency: 45.3%
     Programs Available: PP0197, X000197P & PP0419 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000197p.u66",   0x00000, 0x010000, CRC(13394826) SHA1(c9325b2ed47267b2918a3bf365b90338134ce9c7) ) /* Standard Draw Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0203p ) /* Superboard : 4 of a Kind Bonus Poker (X000203P+XP000038) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P90A      1    2    3    4    5   7  25  40  80  50 250    800
  % Range: 94.0-96.0%  Optimum: 98.0%  Hit Frequency: 45.5%
     Programs Available: PP0203, X000203P, PP0590 & PP0409 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000203p.u66",   0x00000, 0x010000, CRC(8e5dc66e) SHA1(e3030598e42e7a76e608d5edfaf263aadc2caf85) ) /* 4 of a Kind Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0225p ) /* Superboard : Dueces Joker Wild Poker 1-100 Coins (X000225P+XP000079) */
/*
                                         With  w/o  w/o  With
                                         Wild  JKR  Wild JKR
PayTable   3K   STR  FL  FH  4K  SF  5K   RF    4D   RF   4D  (Bonus)
--------------------------------------------------------------------
  P76N     1     1    3   3   3   6   9   12    25  800  1000  2000
  % Range: 95.1-97.1%  Optimum: 99.1%  Hit Frequency: 50.4%
     Programs Available: PP0431, PP0812, PP0813, X000225P & PP0225 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000079.u67",   0x00000, 0x10000, CRC(fe9757b7) SHA1(8547f00f23e2e3cd4b36d006b760eca6a19f0710) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000225p.u66",   0x00000, 0x010000, CRC(d965dd5e) SHA1(1f3e3acb9319e26fa8563f57d1c75940a4445959) ) /* Dueces Joker Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2242.u77",  0x00000, 0x8000, CRC(963a7e7d) SHA1(ebb159f6c731a3f912382745ef9a9c6d4fa2fc99) )
	ROM_LOAD( "mgo-cg2242.u78",  0x08000, 0x8000, CRC(53eed56f) SHA1(e79f31c5c817b8b96b4970c1a702d1892961d441) )
	ROM_LOAD( "mbo-cg2242.u79",  0x10000, 0x8000, CRC(af092f50) SHA1(53a3536593bb14c4072e8a5ee9e05af332feceb1) )
	ROM_LOAD( "mxo-cg2242.u80",  0x18000, 0x8000, CRC(ecacb6b2) SHA1(32660adcc266fbbb3702a0cd30e25d11b953d23d) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0291p ) /* Superboard : Deuces Wild Poker (X000291P+XP000053) */
/*
                                        w/D     w/oD
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P62A     1     2    3   4   4   9  15  25 200 250    800
  % Range: 94.9-96.9%  Optimum: 98.9%  Hit Frequency: 44.4%
     Programs Available: PP0418, X000291P & PP0291 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000053.u67",   0x00000, 0x10000, CRC(f4f1f986) SHA1(84cfc2c4a10ed24d3a971fe75041a4108ec1d7f2) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000291p.u66",   0x00000, 0x010000, CRC(7aa72bbd) SHA1(f754b85579720abd2efd57efa31091bca3c01425) ) /* Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0430p ) /* Superboard : Dueces Joker Wild Poker (X000430P+XP000079) */
/*
                                         With  w/o  w/o  With
                                         Wild  JKR  Wild JKR
PayTable   3K   STR  FL  FH  4K  SF  5K   RF    4D   RF   4D  (Bonus)
--------------------------------------------------------------------
  P73N     1     2    3   3   3   5   8   10    25  800  1000  2000
  % Range: 93.2-95.2%  Optimum: 97.2%  Hit Frequency: 50.5%
     Programs Available: PP0430, X000430P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000079.u67",   0x00000, 0x10000, CRC(fe9757b7) SHA1(8547f00f23e2e3cd4b36d006b760eca6a19f0710) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000430p.u66",   0x00000, 0x010000, CRC(905571e3) SHA1(fd506516fed22842df8e9dbb3683dcb4c459719b) ) /* Dueces Joker Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2242.u77",  0x00000, 0x8000, CRC(963a7e7d) SHA1(ebb159f6c731a3f912382745ef9a9c6d4fa2fc99) )
	ROM_LOAD( "mgo-cg2242.u78",  0x08000, 0x8000, CRC(53eed56f) SHA1(e79f31c5c817b8b96b4970c1a702d1892961d441) )
	ROM_LOAD( "mbo-cg2242.u79",  0x10000, 0x8000, CRC(af092f50) SHA1(53a3536593bb14c4072e8a5ee9e05af332feceb1) )
	ROM_LOAD( "mxo-cg2242.u80",  0x18000, 0x8000, CRC(ecacb6b2) SHA1(32660adcc266fbbb3702a0cd30e25d11b953d23d) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0434p ) /* Superboard : Bonus Poker Deluxe (X000434P+XP000038) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P200A     1    1    3    4    6   8  80  50 250    800
  % Range: 94.5-96.5%  Optimum: 98.5%  Hit Frequency: 45.2%
     Programs Available: PP0434, X000434P & PP0713 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000434p.u66",   0x00000, 0x010000, CRC(5a3ad28b) SHA1(d4f103c7ce3c4f72728450ab015aca8ef10cd79c) ) /* Bonus Poker Deluxe */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0447p ) /* Superboard : Standard Draw Poker (X000447P+XP000038) */
/*
PayTable   Js+  2PR  3K  STR  FL  FH  4K  SF  RF  (Bonus)
---------------------------------------------------------
  CA        1    2    3   4    6   9  25  50 250    800
  % Range: 95.5-97.5%  Optimum: 99.5%  Hit Frequency: 45.5%
     Programs Available: PP0447, X000447P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000447p.u66",   0x00000, 0x010000, CRC(4d3ab095) SHA1(337255658242816dc552432aee328fa52e556793) ) /* Standard Draw Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0449p ) /* Superboard : Standard Draw Poker (X000449P+XP000038) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P11A     1     2    3    4    5   9  25  50 250    800
  % Range: 92.1-94.1%  Optimum: 96.1%  Hit Frequency: 45.5%
     Programs Available: PP0449, X000449P & PP0221 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000449p.u66",   0x00000, 0x010000, CRC(6c22b0b3) SHA1(47cb1b8edb3e1d5d2055a7a31a1dfb46b4fd6391) ) /* Standard Draw Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0451p ) /* Superboard : Bonus Poker Deluxe (X000451P+XP000038) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P253A     1    1    3    4    5   8  80  50 250    800
  % Range: 93.4-95.4%  Optimum: 97.4%  Hit Frequency: 45.2%
     Programs Available: PP0451, X000451P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000451p.u66",   0x00000, 0x010000, CRC(4f11e26c) SHA1(6cea3cbef530ef4ece2a4351cbd9ead5b66bb359) ) /* Bonus Poker Deluxe */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0452p ) /* Superboard : Double Deuces Wild Poker (X000452P+XP000038) */
/*
                                        w/D     wo/D
PayTable   3K   STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
----------------------------------------------------------
  P236A     1    2    2   3   4  11  16  25 400 250    800
  % Range: 95.6-97.6%  Optimum: 99.6%  Hit Frequency: 45.1%
     Programs Available: PP0452, X000452P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000452p.u66",   0x00000, 0x010000, CRC(a96c7a71) SHA1(6be1012e68035fbc9aa5e0e6ea3a6c54c1864b1b) ) /* Double Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0454p ) /* Superboard : Bonus Poker Deluxe (X000454P+XP000038) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
  P253A     1    1    3    4    5   7  80  50 250    800
  % Range: 92.3-94.3%  Optimum: 96.3%  Hit Frequency: 45.2%
     Programs Available: PP0454, X000454P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000454p.u66",   0x00000, 0x010000, CRC(93934ae4) SHA1(f243c66e23269e5509bf1306e9e37a579b08fda4) ) /* Bonus Poker Deluxe */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0458p ) /* Superboard : Joker Poker (X000458P+XP000038) */
/*
                                            w/J     w/oJ
PayTable   Ks+  2P  3K  STR  FL  FH  4K  SF  RF  5K  RF  (Bonus)
----------------------------------------------------------------
  P18A      1    1   2   3    5   6  20  50 100 200 500    800
  % Range: 91.5-92.5%  Optimum: 995.5%  Hit Frequency: 44.7%
     Programs Available: PP0458, X000458P & PP0429 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000458p.u66",   0x00000, 0x010000, CRC(dcd20558) SHA1(22c99a265431b0ef8199d3cb69fbbc4aff822dc0) ) /* Joker Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0508p ) /* Superboard : Loose Deuce Deuces Wild Poker (X000508P+XP000038) */
/*
                                       w/D     W/oD
PayTable   3K  STR  FL  FH  4K  SF  5K  RF  4D  RF  (Bonus)
-----------------------------------------------------------------
  P313A     1    2   2   3   4   8  12  25 500 250    800
  % Range: 95.2-97.2%  Optimum: 99.2%  Hit Frequency: 45.2%
     Programs Available: PP0508, X000508P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000508p.u66",   0x00000, 0x010000, CRC(5efde4b4) SHA1(ead7448464aecc03748f04e4d6e9f346d262cd96) ) /* Loose Deuce Deuces Wild Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0514p ) /* Superboard : Double Double Bonus Poker (X000514P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P323A     1    1    3   5    7   9  50  80 160  50 250    800
  % Range: 95.1-97.1%  Optimum: 99.1%  Hit Frequency: 43.2%
     Programs Available: PP0514, X000514P & PP0538 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000514p.u66",   0x00000, 0x010000, CRC(32cf8696) SHA1(83992695d3af4de10d0e53e01558faad18cdc221) ) /* Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0515p ) /* Superboard : Double Double Bonus Poker (X000515P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P324A     1    1    3   5    7  10  50  80 160  50 250    800
  % Range: 96.2-98.2%  Optimum: 100.2%  Hit Frequency: 43.3%
     Programs Available: PP0515, X000515P & PP0539 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000515p.u66",   0x00000, 0x010000, CRC(4311224a) SHA1(69e6657dacd6e09c2d1514417994adc561f63a83) ) /* Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0581p ) /* Superboard : 4 of a Kind Bonus Poker (X000581P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P373A     1    2    2   4    5   8  25  40  80  50 250    800
  % Range: 87.7-89.7%  Optimum: 91.7%  Hit Frequency: 45.2%
     Programs Available: X000581P, PP0581 - Non Double-up Only
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000581p.u66",   0x00000, 0x010000, CRC(a4cfecc3) SHA1(b2c805781ba43bda9e208d8c16578dc96b6f58f7) ) /* 4 of a Kind Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0725p ) /* Superboard : Double Bonus Poker (X000725P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P434A     1    1    3   4    6   9  50  80 160  50 250    800
  % Range: 92.4-94.4%  Optimum: 96.4%  Hit Frequency: 44.9%
     Programs Available: PP0725, X000725P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000725p.u66",   0x00000, 0x010000, CRC(a56f3910) SHA1(06d0d4a8722e033ff1fbe0947135952ce8274725) ) /* Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0726p ) /* Superboard : Double Bonus Poker (X000726P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P435A     1    1    3   4    5   8  50  80 160  50 250    800
  % Range: 90.2-92.2%  Optimum: 94.2%  Hit Frequency: 45.1%
     Programs Available: PP0726, X000726P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000726p.u66",   0x00000, 0x010000, CRC(800eb7e5) SHA1(cb4c2749d025ab093f26967909d5f366f1cc9cba) ) /* Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0727p ) /* Superboard : Double Bonus Poker (X000727P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P436A     1    1    3   4    5   9  50  80 160  50 250    800
  % Range: 91.3-93.3%  Optimum: 95.3%  Hit Frequency: 45.0%
     Programs Available: PP0727, X000727P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000727p.u66",   0x00000, 0x010000, CRC(4828474c) SHA1(9836b76113a71802df30ca15f7c9a5790e6f1c5b) ) /* Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex0763p ) /* Superboard : 4 of a Kind Bonus Poker (X000763P+XP000038) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
 P597A      1    1    3    5    8  10  25  40  80  50 250    800
  % Range: 90.2-92.2%  Optimum: 94.2%  Hit Frequency: 42.7%
     Programs Available: PP0763, X000763P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000763p.u66",   0x00000, 0x010000, CRC(bf7dda7d) SHA1(1a6089d1159c199199e608f3dd2ba7b45a29b31c) ) /* 4 of a Kind Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2018p ) /* Superboard : Fullhouse Bonus Poker (X002018P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P532A     1    1    3   5    6  12  25  40 200 100 250    800
  % Range: 92.4-94.4%  Optimum: 96.4%  Hit Frequency: 44.1%
     Programs Available: X002018P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002018p.u66",   0x00000, 0x010000, CRC(a7b79cfa) SHA1(89216fafffc64fda22a016a906483b76174c3f02) ) /* Full House Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2025p ) /* Superboard : Deuces Wild Bonus Poker (X002025P+XP000019) */
/*
                                   w/D 6-K 3-5         w/A w/oD
PayTable   3K  STR  FL  FH  4K  SF  RF  5K  5K  5A  4D  4D  RF  (Bonus)
-----------------------------------------------------------------------
  P552A     1   1    3   4   4   9  25  20  40  80 200 400 250    800
  % Range: 95.5-97.5%  Optimum: 99.5%  Hit Frequency: 41.1%
     Programs Available: X002025P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000019.u67",   0x00000, 0x10000, CRC(8ac876eb) SHA1(105b4aee2692ccb20795586ccbdf722c59db66cf) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002025p.u66",   0x00000, 0x10000, CRC(f3dac423) SHA1(e9394d330deb3b8a1001e57e72a506cd9098f161) ) /* Deuces Wild Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2026p ) /* Superboard : Deuces Wild Bonus Poker (X002026P+XP000019) */
/*
                                   w/D 6-K 3-5         w/A w/oD
PayTable   3K  STR  FL  FH  4K  SF  RF  5K  5K  5A  4D  4D  RF  (Bonus)
-----------------------------------------------------------------------
  P553A     1   1    3   3   4  13  25  20  40  80 200 400 250    800
  % Range: 94.8-96.8%  Optimum: 98.8%  Hit Frequency: 44.6%
     Programs Available: X002026P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000019.u67",   0x00000, 0x10000, CRC(8ac876eb) SHA1(105b4aee2692ccb20795586ccbdf722c59db66cf) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002026p.u66",   0x00000, 0x10000, CRC(7fcbc10a) SHA1(5d50b356ae1a3461a5916b469f85b690b086e675) ) /* Deuces Wild Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2027p ) /* Superboard : Deuces Wild Bonus Poker (X002027P+XP000019) */
/*
                                   w/D 6-K 3-5         w/A w/oD
PayTable   3K  STR  FL  FH  4K  SF  RF  5K  5K  5A  4D  4D  RF  (Bonus)
-----------------------------------------------------------------------
  P554A     1   1    3   3   4  10  25  20  40  80 200 400 250    800
  % Range: 93.4-95.4%  Optimum: 97.4%  Hit Frequency: 44.6%
     Programs Available: X002027P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000019.u67",   0x00000, 0x10000, CRC(8ac876eb) SHA1(105b4aee2692ccb20795586ccbdf722c59db66cf) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002027p.u66",   0x00000, 0x10000, CRC(40dbc35a) SHA1(56bf79738e35a22d1f23d76cd6197c8949eba3fb) ) /* Deuces Wild Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2029p ) /* Superboard : Deuces Wild Bonus Poker (X002029P+XP000019) */
/*
                                   w/D 6-K 3-5         w/A w/oD
PayTable   3K  STR  FL  FH  4K  SF  RF  5K  5K  5A  4D  4D  RF  (Bonus)
-----------------------------------------------------------------------
  P556A     1   1    2   3   4  10  25  20  40  80 200 400 250    800
  % Range: 91.3-93.3%  Optimum: 95.3%  Hit Frequency: 45.0%
     Programs Available: X002029P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000019.u67",   0x00000, 0x10000, CRC(8ac876eb) SHA1(105b4aee2692ccb20795586ccbdf722c59db66cf) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002029p.u66",   0x00000, 0x10000, CRC(e2f6fb89) SHA1(4b60b580b00b4268d1cb9065ffe0d21f8fa6a931) ) /* Deuces Wild Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2031p ) /* Superboard : Lucky Deal Poker (X002031P+XP000112) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P567AL    1    1    3   4    6  10  25  40  80  50 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 44.9%
     Programs Available: X002031P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002031p.u66",   0x00000, 0x010000, CRC(c883fbdd) SHA1(c0444ffdac1ffe542d0d6a65f3c810346e2b2e05) ) /* Lucky Deal Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) )
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pex2035p ) /* Superboard : White Hot Aces Poker (X002035P+XP000112) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P581A     1    1    3   4    5   7  50 120 240  80 250    800
  % Range: 93.4-95.4%  Optimum: 97.4%  Hit Frequency: 44.7%
     Programs Available: X002035P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002035p.u66",   0x00000, 0x10000, CRC(dc3f0742) SHA1(d57cf3353b81f41895458019e47203f98645f17a) ) /* White Hot Aces Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) )
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pex2036p ) /* Superboard : White Hot Aces Poker (X002036P+XP000112) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P582A     1    1    3   4    5   6  50 120 240  80 250    800
  % Range: 92.4-94.4%  Optimum: 96.4%  Hit Frequency: 44.7%
     Programs Available: X002036P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002036p.u66",   0x00000, 0x10000, CRC(69207baf) SHA1(fe038b969106ae5cdc8dde1c06497be9c7b5b8bf) ) /* White Hot Aces Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) )
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pex2040p ) /* Superboard : Nevada Bonus Poker (X002040P+XP000038) */
/*
                                         2-K
PayTable   Js+  2PR  3K  3A  STR  FL  FH  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P578A     1    1    3   6   5    6   9  25 200 100 250    800
  % Range: 90.8-92.8%  Optimum: 94.8%  Hit Frequency: 44.2%
     Programs Available: X002040P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002040p.u66",   0x00000, 0x010000, CRC(38acb477) SHA1(894f5861ac84323e50e8972602251f2873988e6c) ) /* Nevada Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2042p ) /* Superboard : Triple Bonus Poker (X002042P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P604A     1    1    3   4    7  11  75 120 240  50 250    800
  % Range: 95.6-97.6%  Optimum: 99.6%  Hit Frequency: 34.7%
     Programs Available: X002042P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002042p.u66",   0x00000, 0x010000, CRC(12787a36) SHA1(4ef478cc5c23694ffa6733e2fc47c2b6f545c30a) ) /* Triple Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2045p ) /* Superboard : Triple Bonus Poker (X002045P+XP000038) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P607A     1    1    3   4    6   9  75 120 240  50 250    800
  % Range: 91.9-93.9%  Optimum: 95.9%  Hit Frequency: 32.5%
     Programs Available: X002045P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002045p.u66",   0x00000, 0x010000, CRC(75fe81db) SHA1(980bcc06b54a1ef78e3beac1db83b73e17a04818) ) /* Triple Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2067p ) /* Superboard : Double Double Bonus Poker (X002067P+XP000038) */
/*
                                                  2-4
                                                   4K    4A
                                      5-K 2-4     with   with
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  A,2-4  2-4  SF  RF  (Bonus)
-----------------------------------------------------------------------------
  P505A     1    1    3   4    5   9  50  80 160   160    400  50 250    800
  % Range: 93.9-95.9%  Optimum: 97.9%  Hit Frequency: 44.8%
     Programs Available: X002067P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002067p.u66",   0x00000, 0x010000, CRC(c62adf21) SHA1(25e037f91ef1860d1d45cae12f21bdd1c39ba264) ) /* Double Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2068p ) /* Superboard : Double Double Bonus Poker (X002068P+XP000038) */
/*
                                                   2-4    4A
                                      5-K 2-4      4K    with
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  with A  2-4  SF  RF  (Bonus)
-----------------------------------------------------------------------------
  P506A     1    1    3   4    5   8  50  80 160   160    400  50 250    800
  % Range: 92.8-94.8%  Optimum: 96.8%  Hit Frequency: 44.8%
     Programs Available: X002068P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002068p.u66",   0x00000, 0x010000, CRC(a5e4279d) SHA1(c4dfd1e77a03a94d9911d1754b5874200bfe6c71) ) /* Double Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2069p ) /* Superboard : Double Double Bonus Poker (X002069P+XP000038) */
/*
                                                  2-4
                                                   4K    4A
                                      5-K 2-4     with   with
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  A,2-4  2-4  SF  RF  (Bonus)
-----------------------------------------------------------------------------
  P507A     1    1    3   4    5   7  50  80 160   160   400  50 250    800
  % Range: 91.6-93.6%  Optimum: 95.6%  Hit Frequency: 45.0%
     Programs Available: X002069P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002069p.u66",   0x00000, 0x10000, CRC(875ecfcf) SHA1(80472cb43b36e518216e60a9b4883a81163718a2) ) /* Double Double Bonus Poker - 08/17/95   @ IGT  L95-2088 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2070p ) /* Superboard : Double Double Bonus Poker (X002070P+XP000038) */
/*
                                                  2-4
                                                   4K    4A
                                      5-K 2-4     with   with
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  A,2-4  2-4  SF  RF  (Bonus)
-----------------------------------------------------------------------------
  P508A     1    1    3   4    5   6  50  80 160   160    400  50 250    800
  % Range: 90.5-92.5%  Optimum: 94.5%  Hit Frequency: 45.0%
     Programs Available: X002070P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002070p.u66",   0x00000, 0x010000, CRC(e7732ff9) SHA1(de1a68040d46e78831a6c806f26f253b4ab014b5) ) /* Double Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2172p ) /* Superboard : Ace$ Bonus Poker (X002172P+XP000112) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P903A     1    2    3   4    5   7  25  40  80  50 250    800
  % Range: 94.3-96.3%  Optimum: 98.3%  Hit Frequency: 45.7%
     Programs Available: X002172P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002172p.u66",   0x00000, 0x010000, CRC(d4c44409) SHA1(8082b76a51fa131751b8c2c446cb29fb04f531dc) ) /* Ace$ Bonus Poker - 05/09/96   @ IGT  L96-1119 */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2172.u77",  0x00000, 0x8000, NO_DUMP ) /*  06/05/95   @ IGT  L95-1494  */
	ROM_LOAD( "mgo-cg2172.u78",  0x08000, 0x8000, NO_DUMP )
	ROM_LOAD( "mbo-cg2172.u79",  0x10000, 0x8000, NO_DUMP ) /* Original version came with CG2172 + CAPX2172 */
	ROM_LOAD( "mxo-cg2172.u80",  0x18000, 0x8000, NO_DUMP )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) ) /* WRONG!! */
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2172.u43", 0x0000, 0x0200, NO_DUMP ) /* Original version came with CG2271 + CAPX2271 */
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) ) /* WRONG!! */
ROM_END

ROM_START( pex2241p ) /* Superboard : 4 of a Kind Bonus Poker 1-100 Coins (X002241P+XP000079) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  ???       1    2    3    4    5   8  25  40  80  50 800   1000
  % Range: 95.2-97.2%  Optimum: 99.2%  Hit Frequency: ??.?%
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000079.u67",   0x00000, 0x10000, CRC(fe9757b7) SHA1(8547f00f23e2e3cd4b36d006b760eca6a19f0710) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002241p.u66",   0x00000, 0x10000, CRC(c6b45cf4) SHA1(3d47e8ff5c915c4b35e4a2ffe20fcc950e838454) ) /* 4 of a Kind Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2242.u77",  0x00000, 0x8000, CRC(963a7e7d) SHA1(ebb159f6c731a3f912382745ef9a9c6d4fa2fc99) )
	ROM_LOAD( "mgo-cg2242.u78",  0x08000, 0x8000, CRC(53eed56f) SHA1(e79f31c5c817b8b96b4970c1a702d1892961d441) )
	ROM_LOAD( "mbo-cg2242.u79",  0x10000, 0x8000, CRC(af092f50) SHA1(53a3536593bb14c4072e8a5ee9e05af332feceb1) )
	ROM_LOAD( "mxo-cg2242.u80",  0x18000, 0x8000, CRC(ecacb6b2) SHA1(32660adcc266fbbb3702a0cd30e25d11b953d23d) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2244p ) /* Superboard : Double Bonus Poker 1-100 Coins (X002244P+XP000079) */
/*
                                       5-K 2-4
PayTable   Js+  2PR  3K   STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  ???       1    1    3    5    7   9  50  80 160  50 800   1000
  % Range: 91.2-97.1%  Optimum: 99.1%  Hit Frequency: ??.?%
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000079.u67",   0x00000, 0x10000, CRC(fe9757b7) SHA1(8547f00f23e2e3cd4b36d006b760eca6a19f0710) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002244p.u66",   0x00000, 0x010000, CRC(06a3e60d) SHA1(8abeafab589406ff68898e8f90431c1a5f8d2de5) ) /* Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2242.u77",  0x00000, 0x8000, CRC(963a7e7d) SHA1(ebb159f6c731a3f912382745ef9a9c6d4fa2fc99) )
	ROM_LOAD( "mgo-cg2242.u78",  0x08000, 0x8000, CRC(53eed56f) SHA1(e79f31c5c817b8b96b4970c1a702d1892961d441) )
	ROM_LOAD( "mbo-cg2242.u79",  0x10000, 0x8000, CRC(af092f50) SHA1(53a3536593bb14c4072e8a5ee9e05af332feceb1) )
	ROM_LOAD( "mxo-cg2242.u80",  0x18000, 0x8000, CRC(ecacb6b2) SHA1(32660adcc266fbbb3702a0cd30e25d11b953d23d) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2245p ) /* Superboard : Standard Draw Poker (X002245P+XP000055) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   ??       1    2    3    4    5   6  25  50 250    800
  % Range: 91.0-93.0%  Optimum: 95.0%  Hit Frequency: ??.?%
     Programs Available: X002245P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000055.u67",   0x00000, 0x10000, CRC(339821e0) SHA1(127d4eff01136feaf1e3242d57433349afb7b6ca) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002245p.u66",   0x00000, 0x10000, CRC(df49a54c) SHA1(1c89d0a114e27e27117120c9e2fc36b124fe7761) ) /* Standard Draw Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2245pa ) /* Superboard : Standard Draw Poker (X002245P+XP000079) */
/*
PayTable   Js+  2PR  3K   STR  FL  FH  4K  SF  RF  (Bonus)
----------------------------------------------------------
   ??       1    2    3    4    5   6  25  50 250    800
  % Range: 91.0-93.0%  Optimum: 95.0%  Hit Frequency: ??.?%
     Programs Available: X002245P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000079.u67",   0x00000, 0x10000, CRC(fe9757b7) SHA1(8547f00f23e2e3cd4b36d006b760eca6a19f0710) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002245p.u66",   0x00000, 0x10000, CRC(df49a54c) SHA1(1c89d0a114e27e27117120c9e2fc36b124fe7761) ) /* Standard Draw Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2275.u77",  0x00000, 0x8000, CRC(15d5d6b8) SHA1(61b6821d4cc059732bc3831bf19bf01aa3910b31) )
	ROM_LOAD( "mgo-cg2275.u78",  0x08000, 0x8000, CRC(bcb49579) SHA1(d5d9f523304582fa6f0a0c69aade77629bdec006) )
	ROM_LOAD( "mbo-cg2275.u79",  0x10000, 0x8000, CRC(9f893787) SHA1(0b79d5cbac920394d5f5c04d0d9d3727e0060366) )
	ROM_LOAD( "mxo-cg2275.u80",  0x18000, 0x8000, CRC(6187c68b) SHA1(7777b141fd1379d37d93a228b2e2159476c2b89e) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2250p ) /* Superboard : Shockwave Poker (X002250P+XP000050) */
/*
PayTable   Js+  2PR  3K  STR  FL  FH  4K  SF  RF  (Bonus)
-----------------------------------------------------------------
 P598BA     1    1    3   5    8  11  25 100 250    800
  % Range: 94.5-96.5%  Optimum: 98.5%  Hit Frequency: 42.6%
     Programs Available: X002250P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000050.u67",   0x00000, 0x10000, CRC(cf9e72d6) SHA1(fc5c679aae43df0bd563fbcc3e00a3274af1ed11) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002250p.u66",   0x00000, 0x10000, CRC(8d8810f9) SHA1(14262d83cf5f2511c3de7777336ac9df7270dab2) ) /* Shockwave Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2309.u77",  0x00000, 0x8000, CRC(fdef322c) SHA1(8024cb6fcba18b56168e853173b9856c4d011831) )
	ROM_LOAD( "mgo-cg2309.u78",  0x08000, 0x8000, CRC(f70b30c0) SHA1(e4acd0060b3d68b9f385cb60ed43a0988fca66a8) )
	ROM_LOAD( "mbo-cg2309.u79",  0x10000, 0x8000, CRC(1843eec7) SHA1(0d0b80cd4d458081394c2943023b2440c2c2e42c) )
	ROM_LOAD( "mxo-cg2309.u80",  0x18000, 0x8000, CRC(5c73d095) SHA1(078c6c815e8c48988f631d9d37018ea0b4bbfa19) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2309.u43", 0x0000, 0x0200, CRC(5da912cc) SHA1(6294f8be682e70e9052c9ae5f6865467e9dba2e3) )
ROM_END

ROM_START( pex2251p ) /* Superboard : Shockwave Poker (X002251P+XP000050) */
/*
PayTable   Js+  2PR  3K  STR  FL  FH  4K  SF  RF  (Bonus)
-----------------------------------------------------------------
  P719A     1    1    3   5    8  12  25 100 250    800
  % Range: 95.6-97.6%  Optimum: 99.6%  Hit Frequency: 42.6%
     Programs Available: X002251P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000050.u67",   0x00000, 0x10000, CRC(cf9e72d6) SHA1(fc5c679aae43df0bd563fbcc3e00a3274af1ed11) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002251p.u66",   0x00000, 0x10000, CRC(9069aa23) SHA1(299d5befce817e8334d4ac53470ff678775546ff) ) /* Shockwave Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2309.u77",  0x00000, 0x8000, CRC(fdef322c) SHA1(8024cb6fcba18b56168e853173b9856c4d011831) )
	ROM_LOAD( "mgo-cg2309.u78",  0x08000, 0x8000, CRC(f70b30c0) SHA1(e4acd0060b3d68b9f385cb60ed43a0988fca66a8) )
	ROM_LOAD( "mbo-cg2309.u79",  0x10000, 0x8000, CRC(1843eec7) SHA1(0d0b80cd4d458081394c2943023b2440c2c2e42c) )
	ROM_LOAD( "mxo-cg2309.u80",  0x18000, 0x8000, CRC(5c73d095) SHA1(078c6c815e8c48988f631d9d37018ea0b4bbfa19) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2309.u43", 0x0000, 0x0200, CRC(5da912cc) SHA1(6294f8be682e70e9052c9ae5f6865467e9dba2e3) )
ROM_END

ROM_START( pex2302p ) /* Superboard : Bonus Poker Deluxe (X002302P+XP000038) */
/*
PayTable   Js+  2PR  3K  STR  FL  FH  4K  SF  RF  (Bonus)
-----------------------------------------------------------------
  P902A     1    1    3   4    5   6  80  50 250    800
  % Range: 91.4-93.4%  Optimum: 95.4%  Hit Frequency: 45.2%
     Programs Available: X002302P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000038.u67",   0x00000, 0x10000, CRC(8707ab9e) SHA1(3e00a2ad8017e1495c6d6fe900d0efa68a1772b8) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002302p.u66",   0x00000, 0x010000, CRC(8e52646e) SHA1(f6722778eb7e2981a00f8e4e5ea32f71a35e20e5) ) /* Bonus Poker Deluxe */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2185.u77",  0x00000, 0x8000, CRC(7e64bd1a) SHA1(e988a380ee58078bf5bdc7747e83aed1393cfad8) ) /*  07/10/95   @ IGT  L95-1506  */
	ROM_LOAD( "mgo-cg2185.u78",  0x08000, 0x8000, CRC(d4127893) SHA1(75039c45ba6fd171a66876c91abc3191c7feddfc) )
	ROM_LOAD( "mbo-cg2185.u79",  0x10000, 0x8000, CRC(17dba955) SHA1(5f77379c88839b3a04e235e4fb0120c77e17b60e) )
	ROM_LOAD( "mxo-cg2185.u80",  0x18000, 0x8000, CRC(583eb3b1) SHA1(4a2952424969917fb1594698a779fe5a1e99bff5) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx1321.u43", 0x0000, 0x0200, CRC(4b57569f) SHA1(fa29c0f627e7ce79951ec6dadec114864144f37d) )
ROM_END

ROM_START( pex2303p ) /* Superboard : White Hot Aces Poker (X002303P+XP000112) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P903A     1    1    3   4    5   5  50 120 240  80 250    800
  % Range: 91.5-93.5%  Optimum: 95.5%  Hit Frequency: 44.7%
     Programs Available: X002303P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002303p.u66",   0x00000, 0x010000, CRC(81cfd71b) SHA1(485a45412cad705d050b369c4cd1472a438374e8) ) /* White Hot Aces Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) )
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pex2307p ) /* Superboard : Triple Double Bonus Poker (X002307P+XP000112) */
/*
                                                  2-4
                                                   4K    4A
                                      5-K 2-4     with   with
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  A,2-4  2-4  SF  RF  (Bonus)
-----------------------------------------------------------------------------
 P908BM     1    1    3   4    6   9  50  80 160   400   400  50 400    800
  % Range: 94.2-96.2%  Optimum: 98.2%  Hit Frequency: 44.1%
     Programs Available: X002307P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002307p.u66",   0x00000, 0x010000, CRC(c6d5db70) SHA1(017e1e382fb789e4cd8b410362ad5e82b61f61db) ) /* Triple Double Bonus Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) )
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pex2314p ) /* Superboard : Triple Bonus Poker Plus (X002314P+XP000112) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
  P903A     1    1    3   4    5   6  50 120 240 100 250    800
  % Range: 92.6-94.6%  Optimum: 96.6%  Hit Frequency: 44.7%
     Programs Available: X002314P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002314p.u66",   0x00000, 0x010000, CRC(bfc0acf0) SHA1(a6b7c228a84d0ea224ad945964c53de2d44e4a8d) ) /* Triple Bonus Poker Plus */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2324.u77",  0x00000, 0x8000, CRC(6eceef42) SHA1(a2ddd2a3290c41e510f483c6b633fe0002694d0b) )
	ROM_LOAD( "mgo-cg2324.u78",  0x08000, 0x8000, CRC(26d0acbe) SHA1(09a9127deb88185cd5b748bac657461eadb2f48f) )
	ROM_LOAD( "mbo-cg2324.u79",  0x10000, 0x8000, CRC(47baee32) SHA1(d8af09022ccb5fc06aa3aa4c200a734b66cbee00) )
	ROM_LOAD( "mxo-cg2324.u80",  0x18000, 0x8000, CRC(60449fc0) SHA1(251d1e04786b70c1d2bc7b02f3b69cd58ac76398) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pex2374p ) /* Superboard : Super Aces Poker (X002374P+XP000112) */
/*
                                      5-K 2-4
PayTable   Js+  2PR  3K  STR  FL  FH  4K  4K  4A  SF  RF  (Bonus)
-----------------------------------------------------------------
 P956A      1    1    3   4    5   6  50  80 400  60 250    800
  % Range: 93.7-95.7%  Optimum: 97.8%  Hit Frequency: 44.8%
     Programs Available: X002374P
*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xp000112.u67",   0x00000, 0x10000, CRC(c1ae96ad) SHA1(da109602f0fbe9b225cdcd60be0613fd41014864) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x002374p.u66",   0x00000, 0x010000, CRC(fc4b6c8d) SHA1(b101f9042bd54dbfdeed4c7a3acf3798096f6857) ) /* Super Aces Poker */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2374.u72",  0x00000, 0x8000, CRC(ceeb714d) SHA1(6de908d04bcaa243195943affa9ad0d725de5c81) ) /* Custom Horseshoe Casino card backs */
	ROM_LOAD( "mgo-cg2374.u73",  0x08000, 0x8000, CRC(d0fabad5) SHA1(438ebe074fa3eaa3073ef042f481449f416d0665) )
	ROM_LOAD( "mbo-cg2374.u74",  0x10000, 0x8000, CRC(9a0fbc8d) SHA1(aa39f47cbeaf8218fd2d753c9a350e9eab5df5d3) )
	ROM_LOAD( "mxo-cg2374.u75",  0x18000, 0x8000, CRC(99814562) SHA1(2d8e132f4cc4edd06332c0327927219513b22832) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "cap2374.u50", 0x0000, 0x0200, CRC(f922e1b8) SHA1(4aa5291c59431c022dc0561a6e3b38209f60286a) )
ROM_END

ROM_START( pexs0006 ) /* Superboard : Triple Triple Diamond Slots (XS000006) */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xs000006.u67",   0x00000, 0x10000, CRC(4b11ca18) SHA1(f64a1fbd089c01bc35a5484e60b8834a2db4a79f) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000998s.u66",   0x00000, 0x10000, CRC(e29d4346) SHA1(93901ff65c8973e34ac1f0dd68bb4c4973da5621) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2361.u77",  0x00000, 0x8000, CRC(c0eba866) SHA1(8f217aeb3e8028a5633d95e5612f1b55e601650f) )
	ROM_LOAD( "mgo-cg2361.u78",  0x08000, 0x8000, CRC(345eaea2) SHA1(18ebb94a323e1cf671201db8b9f85d4f30d8b5ec) )
	ROM_LOAD( "mbo-cg2361.u79",  0x10000, 0x8000, CRC(fa130af6) SHA1(aca5e52e00bc75a4801fd3f6c564e62ed4251d8e) )
	ROM_LOAD( "mxo-cg2361.u80",  0x18000, 0x8000, CRC(7de1812c) SHA1(c7e23a10f20fc8b618df21dd33ac456e1d2cfe33) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2361.u43", 0x0000, 0x0200, CRC(93057296) SHA1(534bbf8ee80a22822d577f6685501f4c929987ef) )
ROM_END

ROM_START( pexmp002 ) /* Superboard : Multi-Poker (XMP00002) - Dbl Dbl Bonus Poker, Nevada Bonus Poker, Joker Poker, Dbl Bonus Poker & Dbl Deuces Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00002.u67",   0x00000, 0x10000, CRC(d5624ac8) SHA1(6b778b0e7ddb81123c6038920b3447e05a0556b2) ) /* Linkable Progressive */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00004p.u66",   0x00000, 0x10000, CRC(bafd160f) SHA1(7454fbf992d4d0668ef375b76ce2cae3324a5f75) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2196.u77",  0x00000, 0x8000, CRC(f2f95ad9) SHA1(92c105147d4cdcebb4c784d771b9cebc982a742f) )
	ROM_LOAD( "mgo-cg2196.u78",  0x08000, 0x8000, CRC(95980a94) SHA1(40b84b2f3b77584739f2eb8df49b64533c60e1e7) )
	ROM_LOAD( "mbo-cg2196.u79",  0x10000, 0x8000, CRC(38151131) SHA1(7730a342bcfab2c2acd84f93ce280eb5dc9666f3) )
	ROM_LOAD( "mxo-cg2196.u80",  0x18000, 0x8000, CRC(60f748b8) SHA1(61af0bac1d6c23f8e1aa3f0094fd56185aa6ae86) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp003 ) /* Superboard : Multi-Poker (XMP00003) - Bonus Poker, Bonus Poker Dlx, Deuces Wild Poker, Jacks or Better & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00003.u67",   0x00000, 0x10000, CRC(41e33b3e) SHA1(cd5debfb59c4f0cc5d700a1c592a0dc203abcb66) ) /* Linkable Progressive, No Double Up */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00001p.u66",   0x00000, 0x10000, CRC(b1569f05) SHA1(c94409ad74c4585288780cc2f96957592554a250) )
	/* Can be found with any of the following Payout Table roms: XM00001P, XM00002P, XM00003P or XM00006P */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2174.u77",  0x00000, 0x8000, CRC(bb666733) SHA1(dcaa1980b051a554cb0f443b1183a680edc9ad3f) )
	ROM_LOAD( "mgo-cg2174.u78",  0x08000, 0x8000, CRC(cc46adb0) SHA1(6065aa5dcb9091ad80e499c7ee6dc629e79c865a) )
	ROM_LOAD( "mbo-cg2174.u79",  0x10000, 0x8000, CRC(7291a0c8) SHA1(1068f35e6ef5fd88c584922860231840a90fb623) )
	ROM_LOAD( "mxo-cg2174.u80",  0x18000, 0x8000, CRC(14f9480c) SHA1(59323f9fc5995277aea86d088893b6eb95b4e89b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp003a ) /* Superboard : Multi-Poker (XMP00003) - Bonus Poker, Bonus Poker Dlx, Deuces Wild Poker, Jacks or Better & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00003.u67",   0x00000, 0x10000, CRC(41e33b3e) SHA1(cd5debfb59c4f0cc5d700a1c592a0dc203abcb66) ) /* Linkable Progressive, No Double Up */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00002p.u66",   0x00000, 0x10000, CRC(96cf471c) SHA1(9597bf6a80c392ee22dc4606db610fdaf032377f) )
	/* Can be found with any of the following Payout Table roms: XM00001P, XM00002P, XM00003P or XM00006P */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2174.u77",  0x00000, 0x8000, CRC(bb666733) SHA1(dcaa1980b051a554cb0f443b1183a680edc9ad3f) )
	ROM_LOAD( "mgo-cg2174.u78",  0x08000, 0x8000, CRC(cc46adb0) SHA1(6065aa5dcb9091ad80e499c7ee6dc629e79c865a) )
	ROM_LOAD( "mbo-cg2174.u79",  0x10000, 0x8000, CRC(7291a0c8) SHA1(1068f35e6ef5fd88c584922860231840a90fb623) )
	ROM_LOAD( "mxo-cg2174.u80",  0x18000, 0x8000, CRC(14f9480c) SHA1(59323f9fc5995277aea86d088893b6eb95b4e89b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp003b ) /* Superboard : Multi-Poker (XMP00003) - Bonus Poker, Bonus Poker Dlx, Deuces Wild Poker, Jacks or Better & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00003.u67",   0x00000, 0x10000, CRC(41e33b3e) SHA1(cd5debfb59c4f0cc5d700a1c592a0dc203abcb66) ) /* Linkable Progressive, No Double Up */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00003p.u66",   0x00000, 0x10000, CRC(55b44732) SHA1(8e0bbad3aaa7deca85ae641c444be3a513bdce50) )
	/* Can be found with any of the following Payout Table roms: XM00001P, XM00002P, XM00003P or XM00006P */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2174.u77",  0x00000, 0x8000, CRC(bb666733) SHA1(dcaa1980b051a554cb0f443b1183a680edc9ad3f) )
	ROM_LOAD( "mgo-cg2174.u78",  0x08000, 0x8000, CRC(cc46adb0) SHA1(6065aa5dcb9091ad80e499c7ee6dc629e79c865a) )
	ROM_LOAD( "mbo-cg2174.u79",  0x10000, 0x8000, CRC(7291a0c8) SHA1(1068f35e6ef5fd88c584922860231840a90fb623) )
	ROM_LOAD( "mxo-cg2174.u80",  0x18000, 0x8000, CRC(14f9480c) SHA1(59323f9fc5995277aea86d088893b6eb95b4e89b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp004 ) /* Superboard : Multi-Poker (XMP00004) - Bonus Poker, Dbl Dbl Bonus Poker, Joker Poker, Dbl Joker Poker & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00004.u67",   0x00000, 0x10000, CRC(83184999) SHA1(b8483917b338be4fd3641b3990eea37072d36885) ) /* Linkable Progressive */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00005p.u66",   0x00000, 0x10000, CRC(c832eac7) SHA1(747d57de602b44ae1276fe1009db1b6de0d2c64c) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2240.u77",  0x00000, 0x8000, CRC(eedef2d4) SHA1(419a90e1f4a840625e6ac7afc2c24d13c908156d) )
	ROM_LOAD( "mgo-cg2240.u78",  0x08000, 0x8000, CRC(c596b058) SHA1(d53824f869bceeda482e434cba9a77ba8ce2015f) )
	ROM_LOAD( "mbo-cg2240.u79",  0x10000, 0x8000, CRC(ab1a58ee) SHA1(44963f27d5f5d8f9415d88c12b2d40f0ef55c559) )
	ROM_LOAD( "mxo-cg2240.u80",  0x18000, 0x8000, CRC(75488ff7) SHA1(a34ae53847b5643b8c4dc182dc59b1fccf22d557) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp006 ) /* Superboard : Multi-Poker (XMP00006) - Bonus Poker, Bonus Poker Dlx, Deuces Wild Poker, Jacks or Better & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00006.u67",   0x00000, 0x10000, CRC(d61f1677) SHA1(2eca1315d6aa310a54de2dfa369e443a07495b76) ) /* Linkable Progressive */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00001p.u66",   0x00000, 0x10000, CRC(b1569f05) SHA1(c94409ad74c4585288780cc2f96957592554a250) )
	/* Can be found with any of the following Payout Table roms: XM00001P, XM00002P, XM00003P or XM00006P */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2174.u77",  0x00000, 0x8000, CRC(bb666733) SHA1(dcaa1980b051a554cb0f443b1183a680edc9ad3f) )
	ROM_LOAD( "mgo-cg2174.u78",  0x08000, 0x8000, CRC(cc46adb0) SHA1(6065aa5dcb9091ad80e499c7ee6dc629e79c865a) )
	ROM_LOAD( "mbo-cg2174.u79",  0x10000, 0x8000, CRC(7291a0c8) SHA1(1068f35e6ef5fd88c584922860231840a90fb623) )
	ROM_LOAD( "mxo-cg2174.u80",  0x18000, 0x8000, CRC(14f9480c) SHA1(59323f9fc5995277aea86d088893b6eb95b4e89b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp006a ) /* Superboard : Multi-Poker (XMP00006) - Bonus Poker, Bonus Poker Dlx, Deuces Wild Poker, Jacks or Better & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00006.u67",   0x00000, 0x10000, CRC(d61f1677) SHA1(2eca1315d6aa310a54de2dfa369e443a07495b76) ) /* Linkable Progressive */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00002p.u66",   0x00000, 0x10000, CRC(96cf471c) SHA1(9597bf6a80c392ee22dc4606db610fdaf032377f) )
	/* Can be found with any of the following Payout Table roms: XM00001P, XM00002P, XM00003P or XM00006P */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2174.u77",  0x00000, 0x8000, CRC(bb666733) SHA1(dcaa1980b051a554cb0f443b1183a680edc9ad3f) )
	ROM_LOAD( "mgo-cg2174.u78",  0x08000, 0x8000, CRC(cc46adb0) SHA1(6065aa5dcb9091ad80e499c7ee6dc629e79c865a) )
	ROM_LOAD( "mbo-cg2174.u79",  0x10000, 0x8000, CRC(7291a0c8) SHA1(1068f35e6ef5fd88c584922860231840a90fb623) )
	ROM_LOAD( "mxo-cg2174.u80",  0x18000, 0x8000, CRC(14f9480c) SHA1(59323f9fc5995277aea86d088893b6eb95b4e89b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp006b ) /* Superboard : Multi-Poker (XMP00006) - Bonus Poker, Bonus Poker Dlx, Deuces Wild Poker, Jacks or Better & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00006.u67",   0x00000, 0x10000, CRC(d61f1677) SHA1(2eca1315d6aa310a54de2dfa369e443a07495b76) ) /* Linkable Progressive */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00003p.u66",   0x00000, 0x10000, CRC(55b44732) SHA1(8e0bbad3aaa7deca85ae641c444be3a513bdce50) )
	/* Can be found with any of the following Payout Table roms: XM00001P, XM00002P, XM00003P or XM00006P */

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2174.u77",  0x00000, 0x8000, CRC(bb666733) SHA1(dcaa1980b051a554cb0f443b1183a680edc9ad3f) )
	ROM_LOAD( "mgo-cg2174.u78",  0x08000, 0x8000, CRC(cc46adb0) SHA1(6065aa5dcb9091ad80e499c7ee6dc629e79c865a) )
	ROM_LOAD( "mbo-cg2174.u79",  0x10000, 0x8000, CRC(7291a0c8) SHA1(1068f35e6ef5fd88c584922860231840a90fb623) )
	ROM_LOAD( "mxo-cg2174.u80",  0x18000, 0x8000, CRC(14f9480c) SHA1(59323f9fc5995277aea86d088893b6eb95b4e89b) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp024 ) /* Superboard : Multi-Poker (XMP00024) - Bonus Poker, Dbl Dbl Bonus Poker, Joker Poker, Dbl Joker Poker & Dbl Bonus Poker */
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00024.u67",   0x00000, 0x10000, CRC(f2df8870) SHA1(bc7fa1d79da07093cf3d3508e226a9c490990e04) ) /* Standalone Progressive */

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "xm00005p.u66",   0x00000, 0x10000, CRC(c832eac7) SHA1(747d57de602b44ae1276fe1009db1b6de0d2c64c) )

	ROM_REGION( 0x020000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2240.u77",  0x00000, 0x8000, CRC(eedef2d4) SHA1(419a90e1f4a840625e6ac7afc2c24d13c908156d) )
	ROM_LOAD( "mgo-cg2240.u78",  0x08000, 0x8000, CRC(c596b058) SHA1(d53824f869bceeda482e434cba9a77ba8ce2015f) )
	ROM_LOAD( "mbo-cg2240.u79",  0x10000, 0x8000, CRC(ab1a58ee) SHA1(44963f27d5f5d8f9415d88c12b2d40f0ef55c559) )
	ROM_LOAD( "mxo-cg2240.u80",  0x18000, 0x8000, CRC(75488ff7) SHA1(a34ae53847b5643b8c4dc182dc59b1fccf22d557) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2174.u43", 0x0000, 0x0200, CRC(50bdad55) SHA1(958d463c7effb3457c1f9c44c9b7822339c04e8b) )
ROM_END

ROM_START( pexmp017 ) /* Superboard : 5-in-1 Wingboard (XMP00017) */
/*
The CG2298 graphics can support the following XnnnnnnP Data game types:

  Bonus Poker, Bonus Poker Deluxe, Double Bonus Poker, Double Double Bonus Poker, Triple Bonus Poker
  Deuces Wild Poker, Deluxe Deuces Wild, Loose Deuces, Deuces Bonus, Double Deuces, Royal Deuces Poker
  Joker Poker, Double Joker Poker, Deuces Joker Wild Poker, Sevens or Better, Tens or Better, Jacks or Better
  Nevada Draw Poker, Nevada Bonus Poker, White Hot Aces, Double Double Aces & Faces, Odds & Ends Poker
  Two Pair, Crazy Eights and Full House Bonus

*/
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "xmp00017.u67",   0x00000, 0x10000, CRC(129e6eaa) SHA1(1dd2b83a672a618f338b553a6cbd598b6d4ce672) )

	ROM_REGION( 0x10000, "user1", 0 )
	ROM_LOAD( "x000055p.u66",   0x00000, 0x10000, CRC(e06819df) SHA1(36590c4588b8036908e63714fbb3e77d23e60eae) ) /* Deuces Wild Poker */

	ROM_REGION( 0x10000, "user2", 0 )
	ROM_LOAD( "x000188p.u66",   0x00000, 0x10000, CRC(3eb7580e) SHA1(86f2280542fb8a55767efd391d0fb04a12ed9408) ) /* Jacks or Better */

	ROM_REGION( 0x10000, "user3", 0 )
	ROM_LOAD( "x000581p.u66",   0x00000, 0x10000, CRC(a4cfecc3) SHA1(b2c805781ba43bda9e208d8c16578dc96b6f58f7) ) /* Four of a Kind Bonus Poker */

	ROM_REGION( 0x10000, "user4", 0 )
	ROM_LOAD( "x000727p.u66",   0x00000, 0x10000, CRC(4828474c) SHA1(9836b76113a71802df30ca15f7c9a5790e6f1c5b) ) /* Double Bonus Poker */

	ROM_REGION( 0x10000, "user5", 0 )
	ROM_LOAD( "x002036p.u66",   0x00000, 0x10000, CRC(69207baf) SHA1(fe038b969106ae5cdc8dde1c06497be9c7b5b8bf) ) /* White Hot Aces */

	ROM_REGION( 0x040000, "gfx1", 0 )
	ROM_LOAD( "mro-cg2298.u77",  0x00000, 0x10000, CRC(8c35dc7f) SHA1(90e9566e816287e6248d7cab318dee3ad6fac871) )
	ROM_LOAD( "mgo-cg2298.u78",  0x10000, 0x10000, CRC(3663174a) SHA1(c203a4a59f6bc1625d47f35426ffc5b4d279251a) )
	ROM_LOAD( "mbo-cg2298.u79",  0x20000, 0x10000, CRC(9088cdbe) SHA1(dc62951c584463a1e795a774f5752f890d8e3f65) )
	ROM_LOAD( "mxo-cg2298.u80",  0x30000, 0x10000, CRC(8d3aafc8) SHA1(931bc82398b94c63ed9f6f1bd95723aa801894cc) )

	ROM_REGION( 0x200, "proms", 0 )
	ROM_LOAD( "capx2298.u43", 0x0000, 0x0200, CRC(77856036) SHA1(820487c8494965408402ddee6a54511906218e66) )
ROM_END


/*************************
*      Game Drivers      *
*************************/

/*    YEAR  NAME      PARENT  MACHINE   INPUT         INIT      ROT    COMPANY                                  FULLNAME                                                  FLAGS   LAYOUT */

/* Set chips */
GAMEL(1987, peset001, 0,      peplus,  peplus_schip, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (Set001) Set Chip",                      0,   layout_pe_schip )
GAMEL(1987, peset038, 0,      peplus,  peplus_schip, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (Set038) Set Chip",                      0,   layout_pe_schip )

/* Normal board : poker */
GAMEL(1987, pepp0043,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0043) 10's or Better",                0, layout_pe_poker )
GAMEL(1987, pepp0055,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0055) Deuces Wild Poker",             0, layout_pe_poker )
GAMEL(1987, pepp0065,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0065) Joker Poker",                  0, layout_pe_poker )
GAMEL(1987, pepp0103,  pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0103) Deuces Wild Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0126,  pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0126) Deuces Wild Poker",             0, layout_pe_poker )
GAMEL(1987, pepp0127,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0127) Deuces Joker Wild Poker",       0, layout_pe_poker )
GAMEL(1987, pepp0158,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0158) 4 of a Kind Bonus Poker",       0, layout_pe_poker )
GAMEL(1987, pepp0178,  pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0178) 4 of a Kind Bonus Poker (Operator selectable special 4 of a Kind)", 0, layout_pe_poker )
GAMEL(1987, pepp0188,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0188) Standard Draw Poker (set 1)",   0, layout_pe_poker )
GAMEL(1987, pepp0188a, pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0188) Standard Draw Poker (set 2)",   0, layout_pe_poker )
GAMEL(1987, pepp0190,  pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0190) Deuces Wild Poker",             0, layout_pe_poker )
GAMEL(1987, pepp0197,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0197) Standard Draw Poker (set 1)",    0, layout_pe_poker )
GAMEL(1987, pepp0197a, pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0197) Standard Draw Poker (set 2)",    0, layout_pe_poker )
GAMEL(1987, pepp0203,  pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0203) 4 of a Kind Bonus Poker (set 1)", 0, layout_pe_poker )
GAMEL(1987, pepp0203a, pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0203) 4 of a Kind Bonus Poker (set 2)", 0, layout_pe_poker )
GAMEL(1987, pepp0221,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0221) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0224,  pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0224) Deuces Wild Poker (set 1)",     0, layout_pe_poker )
GAMEL(1987, pepp0224a, pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0224) Deuces Wild Poker (set 2)",     0, layout_pe_poker )
GAMEL(1987, pepp0230,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0230) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0235,  pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0235) 4 of a Kind Bonus Poker",       0, layout_pe_poker )
GAMEL(1987, pepp0250,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0250) Double Down Stud Poker",        0, layout_pe_poker )
GAMEL(1987, pepp0265,  pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0265) 4 of a Kind Bonus Poker",       0, layout_pe_poker )
GAMEL(1987, pepp0274,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0274) Standard Draw Poker",           0, layout_pe_poker )
GAMEL(1987, pepp0290,  pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0290) Deuces Wild Poker",             0, layout_pe_poker )
GAMEL(1987, pepp0291,  pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0291) Deuces Wild Poker",             0, layout_pe_poker )
GAMEL(1987, pepp0409,  pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0409) 4 of a Kind Bonus Poker",       0, layout_pe_poker )
GAMEL(1987, pepp0410,  pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0410) 4 of a Kind Bonus Poker",       0, layout_pe_poker )
GAMEL(1987, pepp0417,  pepp0055, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0417) Deuces Wild Poker",             0, layout_pe_poker )
GAMEL(1987, pepp0419,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0419) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0420,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0420) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0423,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0423) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0426,  pepp0065, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0426) Joker Poker",                  0, layout_pe_poker )
GAMEL(1987, pepp0428,  pepp0065, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0428) Joker Poker",                  0, layout_pe_poker )
GAMEL(1987, pepp0429,  pepp0065, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0429) Joker Poker",                  0, layout_pe_poker )
GAMEL(1987, pepp0434,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0434) Bonus Poker Deluxe",             0, layout_pe_poker )
GAMEL(1987, pepp0447,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0447) Standard Draw Poker (set 1)",   0, layout_pe_poker )
GAMEL(1987, pepp0447a, pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0447) Standard Draw Poker (set 2)",   0, layout_pe_poker )
GAMEL(1987, pepp0449,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0449) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0452,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0452) Double Deuces Wild Poker",    0, layout_pe_poker )
GAMEL(1987, pepp0454,  pepp0434, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0454) Bonus Poker Deluxe",            0, layout_pe_poker )
GAMEL(1985, pepp0488,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0488) Standard Draw Poker (Arizona Charlie's)", 0, layout_pe_poker )
GAMEL(1987, pepp0508,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0508) Loose Deuce Deuces Wild! Poker", 0, layout_pe_poker )
GAMEL(1987, pepp0509,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0509) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0514,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0514) Double Bonus Poker (set 1)",    0, layout_pe_poker )
GAMEL(1987, pepp0514a, pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0514) Double Bonus Poker (set 2)",    0, layout_pe_poker )
GAMEL(1987, pepp0515,  pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0515) Double Bonus Poker (set 1)",    0, layout_pe_poker )
GAMEL(1987, pepp0515a, pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0515) Double Bonus Poker (set 2)",    0, layout_pe_poker )
GAMEL(1987, pepp0516,  pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0516) Double Bonus Poker (set 1)",    0, layout_pe_poker )
GAMEL(1987, pepp0516a, pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0516) Double Bonus Poker (set 2)",    0, layout_pe_poker )
GAMEL(1987, pepp0538,  pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0538) Double Bonus Poker",           0, layout_pe_poker )
GAMEL(1987, pepp0540,  pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0540) Double Bonus Poker",           0, layout_pe_poker )
GAMEL(1987, pepp0542,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0542) One Eyed Jacks Wild Poker",    0, layout_pe_poker )
GAMEL(1987, pepp0568,  pepp0065, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0568) Joker Poker",                  0, layout_pe_poker )
GAMEL(1987, pepp0585,  pepp0188, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0585) Standard Draw Poker",          0, layout_pe_poker )
GAMEL(1987, pepp0725,  pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0725) Double Bonus Poker",           0, layout_pe_poker )
GAMEL(1987, pepp0728,  pepp0514, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0728) Double Bonus Poker",           0, layout_pe_poker )
GAMEL(1987, pepp0760,  pepp0250, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0760) Double Down Stud Poker",         0, layout_pe_poker )
GAMEL(1987, pepp0763,  pepp0158, peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0763) 4 of a Kind Bonus Poker",       0, layout_pe_poker )
GAMEL(1987, pepp0775,  0,        peplus,  peplus_poker, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PP0775) Unknown Draw Poker",          GAME_IMPERFECT_GRAPHICS, layout_pe_poker ) /* Wrong CG graphics & CAP */

/* Normal board : blackjack */
GAMEL(1994, pebe0014, 0,      peplus,  peplus_bjack, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (BE0014) Blackjack",                     0,   layout_pe_bjack )

/* Normal board : keno */
GAMEL(1994, peke1012,  0,        peplus, peplus_keno, peplus_state, peplus, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (KE1012) Keno",                          0, layout_pe_keno )
GAMEL(1994, peke1013,  peke1012, peplus, peplus_keno, peplus_state, peplus, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (KE1013) Keno",                          0, layout_pe_keno )

/* Normal board : slots machine */
GAMEL(1996, peps0014, 0,        peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0014) Super Joker Slots",             0, layout_pe_slots )
GAMEL(1996, peps0021, 0,        peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0021) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0022, peps0021, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0022) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0042, 0,        peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0042) Double Diamond Slots",          0, layout_pe_slots )
GAMEL(1996, peps0043, peps0042, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0043) Double Diamond Slots",          0, layout_pe_slots )
GAMEL(1996, peps0045, peps0021, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0045) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0047, 0,        peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0047) Wild Cherry Slots",             GAME_NOT_WORKING, layout_pe_slots ) /* Needs MxO-CG1004.Uxx graphics roms redumped */
GAMEL(1996, peps0092, peps0047, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0092) Wild Cherry Slots",             GAME_NOT_WORKING, layout_pe_slots ) /* Needs MxO-CG1004.Uxx graphics roms redumped */
GAMEL(1996, peps0206, peps0021, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0206) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0207, peps0021, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0207) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0298, peps0042, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0298) Double Diamond Slots",          0, layout_pe_slots )
GAMEL(1996, peps0308, 0,        peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0308) Double Jackpot Slots",          0, layout_pe_slots )
GAMEL(1996, peps0364, peps0021, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0364) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0581, peps0021, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0581) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0615, 0,        peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0615) Chaos Slots",                   0, layout_pe_slots )
GAMEL(1996, peps0631, peps0021, peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0631) Red White & Blue Slots",        0, layout_pe_slots )
GAMEL(1996, peps0716, 0,        peplus, peplus_slots, peplus_state, peplus,   ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (PS0716) River Gambler Slots",           0, layout_pe_slots )

/* Superboard : poker */
GAMEL(1995, pex0055p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000019) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pa, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000022) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pb, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000023) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pc, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000028) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pd, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000035) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pe, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000038) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pf, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000040) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pg, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000053) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055ph, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000055) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pi, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000063) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pj, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000075) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pk, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000079) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pl, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000094) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pm, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000095) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pn, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000098) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055po, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000102) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pp, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000104) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pq, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000112) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0055pr, pex0055p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000055P+XP000126) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0158p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000158P+XP000038) 4 of a Kind Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0188p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000188P+XP000038) Standard Draw Poker", 0, layout_pe_poker )
GAMEL(1995, pex0190p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000190P+XP000053) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0197p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000197P+XP000038) Standard Draw Poker", 0, layout_pe_poker )
GAMEL(1995, pex0203p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000203P+XP000038) 4 of a Kind Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0225p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000225P+XP000079) Dueces Joker Wild Poker", 0,layout_pe_poker )
GAMEL(1995, pex0291p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000291P+XP000053) Deuces Wild Poker",  0, layout_pe_poker )
GAMEL(1995, pex0430p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000430P+XP000079) Dueces Joker Wild Poker", 0,layout_pe_poker )
GAMEL(1995, pex0434p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000434P+XP000038) Bonus Poker Deluxe", 0, layout_pe_poker )
GAMEL(1995, pex0447p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000447P+XP000038) Standard Draw Poker", 0, layout_pe_poker )
GAMEL(1995, pex0449p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000449P+XP000038) Standard Draw Poker", 0, layout_pe_poker )
GAMEL(1995, pex0451p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000451P+XP000038) Bonus Poker Deluxe", 0, layout_pe_poker )
GAMEL(1995, pex0452p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000452P+XP000038) Double Deuces Wild Poker", 0, layout_pe_poker )
GAMEL(1995, pex0454p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000454P+XP000038) Bonus Poker Deluxe", 0, layout_pe_poker )
GAMEL(1995, pex0458p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000458P+XP000038) Joker Poker", 0, layout_pe_poker )
GAMEL(1995, pex0508p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000508P+XP000038) Loose Deuce Deuces Wild! Poker", 0, layout_pe_poker )
GAMEL(1995, pex0514p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000514P+XP000038) Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0515p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000515P+XP000038) Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0581p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000581P+XP000038) 4 of a Kind Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0725p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000725P+XP000038) Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0726p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000726P+XP000038) Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0727p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000727P+XP000038) Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex0763p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X000763P+XP000038) 4 of a Kind Bonus Poker", 0,layout_pe_poker )
GAMEL(1995, pex2018p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002018P+XP000038) Full House Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2025p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002025P+XP000019) Deuces Wild Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2026p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002026P+XP000019) Deuces Wild Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2027p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002027P+XP000019) Deuces Wild Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2029p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002029P+XP000019) Deuces Wild Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2031p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002031P+XP000112) Lucky Deal Poker", 0, layout_pe_poker )
GAMEL(1995, pex2035p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002035P+XP000112) White Hot Aces Poker", 0, layout_pe_poker )
GAMEL(1995, pex2036p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002036P+XP000112) White Hot Aces Poker", 0, layout_pe_poker )
GAMEL(1995, pex2040p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002040P+XP000038) Nevada Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2042p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002042P+XP000038) Triple Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2045p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002045P+XP000038) Triple Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2067p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002067P+XP000038) Double Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2068p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002068P+XP000038) Double Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2069p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002069P+XP000038) Double Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2070p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002070P+XP000038) Double Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2172p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002172P+XP000112) Ace$ Bonus Poker", GAME_IMPERFECT_GRAPHICS, layout_pe_poker ) /* Wrong CG graphics & CAP */
GAMEL(1995, pex2241p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002241P+XP000079) 4 of a Kind Bonus Poker", 0,layout_pe_poker )
GAMEL(1995, pex2244p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002244P+XP000079) Double Bonus Poker", 0,layout_pe_poker )
GAMEL(1995, pex2245p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002245P+XP000055) Standard Draw Poker", 0,layout_pe_poker )
GAMEL(1995, pex2245pa, pex2245p,  peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002245P+XP000079) Standard Draw Poker", 0,layout_pe_poker )
GAMEL(1995, pex2250p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002250P+XP000050) Shockwave Poker",   0,layout_pe_poker )
GAMEL(1995, pex2251p,  0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002251P+XP000050) Shockwave Poker",   0,layout_pe_poker )
GAMEL(1995, pex2302p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002302P+XP000038) Bonus Poker Deluxe", 0, layout_pe_poker )
GAMEL(1995, pex2303p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002303P+XP000112) White Hot Aces Poker", 0, layout_pe_poker )
GAMEL(1995, pex2307p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002307P+XP000112) Triple Double Bonus Poker", 0, layout_pe_poker )
GAMEL(1995, pex2314p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002314P+XP000112) Triple Bonus Poker Plus", 0, layout_pe_poker )
GAMEL(1995, pex2374p, 0,          peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (X002374P+XP000112) Super Aces Poker (Horseshoe)", 0, layout_pe_poker )

/* Superboard : multi-poker */
GAMEL(1995, pexmp002,  0,        peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00002+XM00004P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp003,  0,        peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00003+XM00001P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp003a, pexmp003, peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00003+XM00002P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp003b, pexmp003, peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00003+XM00003P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp004,  0,        peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00004+XM00005P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp006,  0,        peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00006+XM00001P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp006a, pexmp006, peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00006+XM00002P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp006b, pexmp006, peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00006+XM00003P) Multi-Poker",         0,   layout_pe_poker )
GAMEL(1995, pexmp024, 0,         peplus,  peplus_poker, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00024+XM00005P) Multi-Poker",         0,   layout_pe_poker )

/* Superboard : multi-poker (wingboard) */
GAMEL(1995, pexmp017, 0,      peplus,  peplus_poker, peplus_state, peplussbw,ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XMP00017) 5-in-1 Wingboard",            0,   layout_pe_poker )

/* Superboard : slots machine */
GAMEL(1997, pexs0006, 0,      peplus,  peplus_slots, peplus_state, peplussb, ROT0,  "IGT - International Gaming Technology", "Player's Edge Plus (XS000006) Triple Triple Diamond Slots", 0,   layout_pe_slots )
