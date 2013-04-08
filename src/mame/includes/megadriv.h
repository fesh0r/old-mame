
#include "emu.h"
#include "coreutil.h"
#include "cpu/m68000/m68000.h"
#include "cpu/sh2/sh2.h"
#include "cpu/sh2/sh2comn.h"
#include "cpu/z80/z80.h"
#include "sound/2612intf.h"
#include "sound/cdda.h"
#include "sound/dac.h"
#include "sound/rf5c68.h"
#include "sound/sn76496.h"

#include "machine/nvram.h"
#include "cpu/ssp1601/ssp1601.h"

#include "machine/megavdp.h"
#include "machine/mega32x.h"
#include "machine/megacd.h"
#include "video/315_5124.h"

#define MASTER_CLOCK_NTSC 53693175
#define MASTER_CLOCK_PAL  53203424
#define SEGACD_CLOCK      12500000

#define MD_CPU_REGION_SIZE 0x800000


/*----------- defined in machine/megadriv.c -----------*/

INPUT_PORTS_EXTERN( md_common );
INPUT_PORTS_EXTERN( megadriv );
INPUT_PORTS_EXTERN( megadri6 );
INPUT_PORTS_EXTERN( ssf2mdb );
INPUT_PORTS_EXTERN( mk3mdb );

MACHINE_CONFIG_EXTERN( megadriv_timers );
MACHINE_CONFIG_EXTERN( md_ntsc );
MACHINE_CONFIG_EXTERN( md_pal );
MACHINE_CONFIG_EXTERN( md_bootleg );    // for topshoot.c & hshavoc.c

/* Megaplay - Megatech specific */
/* It might be possible to move the following structs in the drivers */

#define MP_ROM  0x10
#define MP_GAME 0


struct genesis_z80_vars
{
	int z80_is_reset;
	int z80_has_bus;
	UINT32 z80_bank_addr;
	UINT8* z80_prgram;
};


class md_base_state : public driver_device
{
public:
	md_base_state(const machine_config &mconfig, device_type type, const char *tag)
	: driver_device(mconfig, type, tag),
		m_maincpu(*this,"maincpu"),
		m_z80snd(*this,"genesis_snd_z80"),
		m_ymsnd(*this,"ymsnd"),
		m_vdp(*this,"gen_vdp"),
		m_32x(*this,"sega32x"),
		m_segacd(*this,"segacd"),
		m_megadrive_ram(*this,"megadrive_ram")
	{ }
	required_device<cpu_device> m_maincpu;
	optional_device<cpu_device> m_z80snd;
	optional_device<device_t> m_ymsnd;
	required_device<sega_genesis_vdp_device> m_vdp;
	optional_device<sega_32x_device> m_32x;
	optional_device<sega_segacd_device> m_segacd;
	optional_shared_ptr<UINT16> m_megadrive_ram;

	ioport_port *m_io_reset;
	ioport_port *m_io_pad_3b[4];
	ioport_port *m_io_pad_6b[4];

	int m_other_hacks;  // misc hacks
	genesis_z80_vars m_genz80;
	int m_pal;
	int m_export;

	DECLARE_DRIVER_INIT(megadriv_c2);
	DECLARE_DRIVER_INIT(megadrie);
	DECLARE_DRIVER_INIT(megadriv);
	DECLARE_DRIVER_INIT(megadrij);
	DECLARE_DRIVER_INIT(mpnew);

	DECLARE_READ8_MEMBER(megadriv_68k_YM2612_read);
	DECLARE_WRITE8_MEMBER(megadriv_68k_YM2612_write);
	IRQ_CALLBACK_MEMBER(genesis_int_callback);
	void megadriv_init_common();

	DECLARE_DEVICE_IMAGE_LOAD_MEMBER( _32x_cart );

	void megadriv_z80_bank_w(UINT16 data);
	DECLARE_WRITE16_MEMBER( megadriv_68k_z80_bank_write );
	DECLARE_WRITE8_MEMBER(megadriv_z80_z80_bank_w);
	DECLARE_READ16_MEMBER( megadriv_68k_io_read );
	DECLARE_WRITE16_MEMBER( megadriv_68k_io_write );
	DECLARE_READ16_MEMBER( megadriv_68k_read_z80_ram );
	DECLARE_WRITE16_MEMBER( megadriv_68k_write_z80_ram );
	DECLARE_READ16_MEMBER( megadriv_68k_check_z80_bus );
	DECLARE_WRITE16_MEMBER( megadriv_68k_req_z80_bus );
	DECLARE_WRITE16_MEMBER ( megadriv_68k_req_z80_reset );
	DECLARE_READ8_MEMBER( z80_read_68k_banked_data );
	DECLARE_WRITE8_MEMBER( z80_write_68k_banked_data );
	DECLARE_WRITE8_MEMBER( megadriv_z80_vdp_write );
	DECLARE_READ8_MEMBER( megadriv_z80_vdp_read );
	DECLARE_READ8_MEMBER( megadriv_z80_unmapped_read );
	DECLARE_READ8_MEMBER( z80_unmapped_port_r );
	DECLARE_WRITE8_MEMBER( z80_unmapped_port_w );
	DECLARE_READ8_MEMBER( z80_unmapped_r );
	DECLARE_WRITE8_MEMBER( z80_unmapped_w );
	TIMER_CALLBACK_MEMBER(megadriv_z80_run_state);

	/* Megadrive / Genesis has 3 I/O ports */
	emu_timer *m_io_timeout[3];
	int m_io_stage[3];
	UINT8 m_megadrive_io_data_regs[3];
	UINT8 m_megadrive_io_ctrl_regs[3];
	UINT8 m_megadrive_io_tx_regs[3];
	read8_delegate m_megadrive_io_read_data_port_ptr;
	write16_delegate m_megadrive_io_write_data_port_ptr;


	TIMER_CALLBACK_MEMBER( io_timeout_timer_callback );
	void megadrive_reset_io();
	DECLARE_READ8_MEMBER(megadrive_io_read_data_port_6button);
	DECLARE_READ8_MEMBER(megadrive_io_read_data_port_3button);
	UINT8 megatech_bios_port_cc_dc_r(int offset, int ctrl);
	UINT8 megadrive_io_read_ctrl_port(int portnum);
	UINT8 megadrive_io_read_tx_port(int portnum);
	UINT8 megadrive_io_read_rx_port(int portnum);
	UINT8 megadrive_io_read_sctrl_port(int portnum);

	DECLARE_WRITE16_MEMBER(megadrive_io_write_data_port_3button);
	DECLARE_WRITE16_MEMBER(megadrive_io_write_data_port_6button);
	void megadrive_io_write_ctrl_port(int portnum, UINT16 data);
	void megadrive_io_write_tx_port(int portnum, UINT16 data);
	void megadrive_io_write_rx_port(int portnum, UINT16 data);
	void megadrive_io_write_sctrl_port(int portnum, UINT16 data);

	void megadriv_stop_scanline_timer();

	DECLARE_MACHINE_START( megadriv );
	DECLARE_MACHINE_RESET( megadriv );
	DECLARE_VIDEO_START( megadriv );
	UINT32 screen_update_megadriv(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void screen_eof_megadriv(screen_device &screen, bool state);

};

class md_boot_state : public md_base_state
{
public:
	md_boot_state(const machine_config &mconfig, device_type type, const char *tag)
	: md_base_state(mconfig, type, tag) { m_protcount = 0;}

	// bootleg specific
	int m_aladmdb_mcu_port;

	int m_protcount;

	DECLARE_DRIVER_INIT(aladmdb);
	DECLARE_DRIVER_INIT(mk3mdb);
	DECLARE_DRIVER_INIT(ssf2mdb);
	DECLARE_DRIVER_INIT(srmdb);
	DECLARE_DRIVER_INIT(topshoot);
	DECLARE_DRIVER_INIT(puckpkmn);
	DECLARE_DRIVER_INIT(hshavoc);
	DECLARE_WRITE16_MEMBER( bl_710000_w );
	DECLARE_READ16_MEMBER( bl_710000_r );
	DECLARE_WRITE16_MEMBER( aladmdb_w );
	DECLARE_READ16_MEMBER( aladmdb_r );
	DECLARE_READ16_MEMBER( mk3mdb_dsw_r );
	DECLARE_READ16_MEMBER( ssf2mdb_dsw_r );
	DECLARE_READ16_MEMBER( srmdb_dsw_r );
	DECLARE_READ16_MEMBER( topshoot_200051_r );
	DECLARE_READ16_MEMBER(puckpkmna_70001c_r);
	DECLARE_READ16_MEMBER(puckpkmna_4b2476_r);

	DECLARE_MACHINE_START( md_6button );
};


class segac2_state : public md_base_state
{
public:
	segac2_state(const machine_config &mconfig, device_type type, const char *tag)
	: md_base_state(mconfig, type, tag),
		m_paletteram(*this, "paletteram") { }

	// for Print Club only
	int m_cam_data;

	int m_segac2_enable_display;

	required_shared_ptr<UINT16> m_paletteram;

	/* internal states */
	UINT8       m_misc_io_data[0x10];   /* holds values written to the I/O chip */

	/* protection-related tracking */
	int (*m_prot_func)(int in);     /* emulation of protection chip */
	UINT8       m_prot_write_buf;       /* remembers what was written */
	UINT8       m_prot_read_buf;        /* remembers what was returned */

	/* palette-related variables */
	UINT8       m_segac2_alt_palette_mode;
	UINT8       m_palbank;
	UINT8       m_bg_palbase;
	UINT8       m_sp_palbase;

	/* sound-related variables */
	UINT8       m_sound_banks;      /* number of sound banks */

	DECLARE_DRIVER_INIT(c2boot);
	DECLARE_DRIVER_INIT(bloxeedc);
	DECLARE_DRIVER_INIT(columns);
	DECLARE_DRIVER_INIT(columns2);
	DECLARE_DRIVER_INIT(tfrceac);
	DECLARE_DRIVER_INIT(tfrceacb);
	DECLARE_DRIVER_INIT(borench);
	DECLARE_DRIVER_INIT(twinsqua);
	DECLARE_DRIVER_INIT(ribbit);
	DECLARE_DRIVER_INIT(puyo);
	DECLARE_DRIVER_INIT(tantr);
	DECLARE_DRIVER_INIT(tantrkor);
	DECLARE_DRIVER_INIT(potopoto);
	DECLARE_DRIVER_INIT(stkclmns);
	DECLARE_DRIVER_INIT(stkclmnj);
	DECLARE_DRIVER_INIT(ichir);
	DECLARE_DRIVER_INIT(ichirk);
	DECLARE_DRIVER_INIT(ichirj);
	DECLARE_DRIVER_INIT(ichirjbl);
	DECLARE_DRIVER_INIT(puyopuy2);
	DECLARE_DRIVER_INIT(zunkyou);
	DECLARE_DRIVER_INIT(pclub);
	DECLARE_DRIVER_INIT(pclubjv2);
	DECLARE_DRIVER_INIT(pclubjv4);
	DECLARE_DRIVER_INIT(pclubjv5);
	void segac2_common_init(int (*func)(int in));
	DECLARE_VIDEO_START(segac2_new);
	DECLARE_MACHINE_START(segac2);
	DECLARE_MACHINE_RESET(segac2);

	UINT32 screen_update_segac2_new(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	int m_segac2_bg_pal_lookup[4];
	int m_segac2_sp_pal_lookup[4];
	void recompute_palette_tables();

	DECLARE_WRITE16_MEMBER( segac2_upd7759_w );
	DECLARE_READ16_MEMBER( palette_r );
	DECLARE_WRITE16_MEMBER( palette_w );
	DECLARE_READ16_MEMBER( io_chip_r );
	DECLARE_WRITE16_MEMBER( io_chip_w );
	DECLARE_WRITE16_MEMBER( control_w );
	DECLARE_READ16_MEMBER( prot_r );
	DECLARE_WRITE16_MEMBER( prot_w );
	DECLARE_WRITE16_MEMBER( counter_timer_w );
	DECLARE_READ16_MEMBER( printer_r );
	DECLARE_WRITE16_MEMBER( print_club_camera_w );
	DECLARE_READ16_MEMBER(ichirjbl_prot_r);
	DECLARE_WRITE_LINE_MEMBER(segac2_irq2_interrupt);
};

class mplay_state : public md_base_state
{
public:
	mplay_state(const machine_config &mconfig, device_type type, const char *tag)
	: md_base_state(mconfig, type, tag),
	m_ic3_ram(*this, "ic3_ram"),
	m_vdp1(*this, "vdp1"),
	m_bioscpu(*this, "mtbios")


	{ }

	UINT32 m_bios_mode;  // determines whether ROM banks or Game data
	// is to read from 0x8000-0xffff

	UINT32 m_bios_bank; // ROM bank selection
	UINT16 m_game_banksel;  // Game bank selection
	UINT32 m_readpos;  // serial bank selection position (9-bit)
	UINT32 m_mp_bios_bank_addr;

	UINT32 m_bios_width;  // determines the way the game info ROM is read
	UINT8 m_bios_ctrl[6];
	UINT8 m_bios_6600;
	UINT8 m_bios_6403;
	UINT8 m_bios_6404;

	UINT16 *m_genesis_io_ram;
	required_shared_ptr<UINT8> m_ic3_ram;
	optional_device<sega315_5124_device> m_vdp1;
	required_device<cpu_device>          m_bioscpu;
	UINT8* m_ic37_ram;
	UINT16 *m_ic36_ram;
	DECLARE_WRITE_LINE_MEMBER( int_callback );
	DECLARE_DRIVER_INIT(megaplay);
	DECLARE_VIDEO_START(megplay);
	DECLARE_MACHINE_RESET(megaplay);

	UINT32 screen_update_megplay(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void screen_eof_megaplay(screen_device &screen, bool state);

	DECLARE_READ8_MEMBER( megaplay_bios_banksel_r );
	DECLARE_WRITE8_MEMBER( megaplay_bios_banksel_w );
	DECLARE_READ8_MEMBER( megaplay_bios_gamesel_r );
	DECLARE_WRITE8_MEMBER( megaplay_bios_gamesel_w );
	DECLARE_WRITE16_MEMBER( megaplay_io_write );
	DECLARE_READ16_MEMBER( megaplay_io_read );
	DECLARE_READ8_MEMBER( bank_r );
	DECLARE_WRITE8_MEMBER( bank_w );
	DECLARE_READ8_MEMBER( megaplay_bios_6402_r );
	DECLARE_WRITE8_MEMBER( megaplay_bios_6402_w );
	DECLARE_READ8_MEMBER( megaplay_bios_6204_r );
	DECLARE_WRITE8_MEMBER( megaplay_bios_width_w );
	DECLARE_READ8_MEMBER( megaplay_bios_6404_r );
	DECLARE_WRITE8_MEMBER( megaplay_bios_6404_w );
	DECLARE_READ8_MEMBER( megaplay_bios_6600_r );
	DECLARE_WRITE8_MEMBER( megaplay_bios_6600_w );
	DECLARE_WRITE8_MEMBER( megaplay_game_w );
	void mplay_start();
	DECLARE_READ16_MEMBER( megadriv_68k_read_z80_extra_ram );
	DECLARE_WRITE16_MEMBER( megadriv_68k_write_z80_extra_ram );
};

class mtech_state : public md_base_state
{
public:
	mtech_state(const machine_config &mconfig, device_type type, const char *tag)
	: md_base_state(mconfig, type, tag),
		m_vdp1(*this, "vdp1"),
		m_bioscpu(*this, "mtbios")
	{ }


	required_device<sega315_5124_device> m_vdp1;
	required_device<cpu_device>          m_bioscpu;


	DECLARE_WRITE_LINE_MEMBER( int_callback );


	UINT8 m_mt_cart_select_reg;
	UINT32 m_bios_port_ctrl;
	int m_current_game_is_sms; // is the current game SMS based (running on genesis z80, in VDP compatibility mode)
	UINT32 m_bios_ctrl_inputs;
	UINT8 m_bios_ctrl[6];

	int m_mt_bank_addr;

	int m_cart_is_genesis[8];

	DECLARE_DEVICE_IMAGE_LOAD_MEMBER( megatech_cart );

	/* Megatech BIOS specific */
	UINT8* m_megatech_banked_ram;
	DECLARE_DRIVER_INIT(mt_crt);
	DECLARE_DRIVER_INIT(mt_slot);
	DECLARE_VIDEO_START(mtnew);
	DECLARE_MACHINE_RESET(mtnew);
	UINT32 screen_update_mtnew(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void screen_eof_mtnew(screen_device &screen, bool state);
	UINT32 screen_update_megatech_menu(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	void megatech_set_megadrive_z80_as_megadrive_z80(const char* tag);

	DECLARE_READ8_MEMBER( megatech_cart_select_r );
	TIMER_CALLBACK_MEMBER( megatech_z80_run_state );
	TIMER_CALLBACK_MEMBER( megatech_z80_stop_state );
	void megatech_select_game(int gameno);
	DECLARE_WRITE8_MEMBER( megatech_cart_select_w );
	DECLARE_READ8_MEMBER( bios_ctrl_r );
	DECLARE_WRITE8_MEMBER( bios_ctrl_w );
	DECLARE_READ8_MEMBER( megatech_z80_read_68k_banked_data );
	DECLARE_WRITE8_MEMBER( megatech_z80_write_68k_banked_data );
	void megatech_z80_bank_w(UINT16 data);
	DECLARE_WRITE8_MEMBER( mt_z80_bank_w );
	DECLARE_READ8_MEMBER( megatech_banked_ram_r );
	DECLARE_WRITE8_MEMBER( megatech_banked_ram_w );
	DECLARE_WRITE8_MEMBER( megatech_bios_port_ctrl_w );
	DECLARE_READ8_MEMBER( megatech_bios_joypad_r );
	DECLARE_WRITE8_MEMBER (megatech_bios_port_7f_w);
};

class _32x_state : public md_base_state
{
public:
	_32x_state(const machine_config &mconfig, device_type type, const char *tag)
	: md_base_state(mconfig, type, tag) { }

};

class segacd_state : public _32x_state  // use _32x_state as base to make easier the combo 32X + SCD
{
public:
	segacd_state(const machine_config &mconfig, device_type type, const char *tag)
	: _32x_state(mconfig, type, tag)
		{ }
};


/* machine/megavdp.c */
extern UINT16 (*vdp_get_word_from_68k_mem)(running_machine &machine, UINT32 source, address_space& space);
extern UINT16 vdp_get_word_from_68k_mem_default(running_machine &machine, UINT32 source, address_space& space);
extern int megadrive_total_scanlines;
extern int megadrive_vblank_flag;
extern UINT16* megadrive_vdp_palette_lookup;
extern TIMER_DEVICE_CALLBACK( megadriv_scanline_timer_callback );
extern timer_device* megadriv_scanline_timer;
