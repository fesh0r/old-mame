
#include "machine/v3021.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"
#include "cpu/arm7/arm7.h"
#include "sound/ics2115.h"
#include "cpu/arm7/arm7core.h"
#include "machine/nvram.h"
#include "machine/pgmcrypt.h"

#define PGMARM7LOGERROR 0

class pgm_state : public driver_device
{
public:
	pgm_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_videoregs(*this, "videoregs"),
			m_videoram(*this, "videoram"),
			m_z80_mainram(*this, "z80_mainram"),
			m_mainram(*this, "sram"),
			m_maincpu(*this, "maincpu"),
			m_soundcpu(*this, "soundcpu")
		{
			m_irq4_disabled = 0;
		}

	/* memory pointers */
	required_shared_ptr<UINT16> m_videoregs;
	required_shared_ptr<UINT16> m_videoram;
	required_shared_ptr<UINT8> m_z80_mainram;
	required_shared_ptr<UINT16> m_mainram;
	UINT16 *      m_bg_videoram;
	UINT16 *      m_tx_videoram;
	UINT16 *      m_rowscrollram;
	UINT8  *      m_sprite_a_region;
	size_t        m_sprite_a_region_size;
	UINT16 *      m_spritebufferram; // buffered spriteram
//  UINT16 *      m_paletteram;    // currently this uses generic palette handling

	/* video-related */
	tilemap_t       *m_bg_tilemap;
	tilemap_t     *m_tx_tilemap;
	UINT16        *m_sprite_temp_render;
	bitmap_rgb32      m_tmppgmbitmap;

	/* devices */
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_soundcpu;
	device_t *m_ics;

	/* used by rendering */
	UINT8 *m_bdata;
	size_t  m_bdatasize;
	int m_aoffset;
	int m_boffset;

	/* hack */
	int m_irq4_disabled;

	/* calendar */
	UINT8        m_cal_val;
	UINT8        m_cal_mask;
	UINT8        m_cal_com;
	UINT8        m_cal_cnt;
	system_time  m_systime;


	DECLARE_READ16_MEMBER(pgm_videoram_r);
	DECLARE_WRITE16_MEMBER(pgm_videoram_w);
	DECLARE_WRITE16_MEMBER(pgm_coin_counter_w);
	DECLARE_READ16_MEMBER(z80_ram_r);
	DECLARE_WRITE16_MEMBER(z80_ram_w);
	DECLARE_WRITE16_MEMBER(z80_reset_w);
	DECLARE_WRITE16_MEMBER(z80_ctrl_w);
	DECLARE_WRITE16_MEMBER(m68k_l1_w);
	DECLARE_WRITE8_MEMBER(z80_l3_w);
	DECLARE_WRITE16_MEMBER(pgm_tx_videoram_w);
	DECLARE_WRITE16_MEMBER(pgm_bg_videoram_w);
	DECLARE_DRIVER_INIT(ket);
	DECLARE_DRIVER_INIT(killbld);
	DECLARE_DRIVER_INIT(pgm);
	DECLARE_DRIVER_INIT(kovsh);
	DECLARE_DRIVER_INIT(killbldp);
	DECLARE_DRIVER_INIT(ddp2);
	DECLARE_DRIVER_INIT(drgw2j);
	DECLARE_DRIVER_INIT(kov2);
	DECLARE_DRIVER_INIT(puzzli2);
	DECLARE_DRIVER_INIT(dw2001);
	DECLARE_DRIVER_INIT(martmast);
	DECLARE_DRIVER_INIT(kovlsqh2);
	DECLARE_DRIVER_INIT(espgal);
	DECLARE_DRIVER_INIT(happy6);
	DECLARE_DRIVER_INIT(oldsplus);
	DECLARE_DRIVER_INIT(kovboot);
	DECLARE_DRIVER_INIT(kovshp);
	DECLARE_DRIVER_INIT(kovshxas);
	DECLARE_DRIVER_INIT(theglad);
	DECLARE_DRIVER_INIT(kov2p);
	DECLARE_DRIVER_INIT(olds);
	DECLARE_DRIVER_INIT(svg);
	DECLARE_DRIVER_INIT(photoy2k);
	DECLARE_DRIVER_INIT(svgpcb);
	DECLARE_DRIVER_INIT(dmnfrnt);
	DECLARE_DRIVER_INIT(dw2v100x);
	DECLARE_DRIVER_INIT(kovqhsgs);
	DECLARE_DRIVER_INIT(ddp3);
	DECLARE_DRIVER_INIT(drgw2c);
	DECLARE_DRIVER_INIT(dwpc);
	DECLARE_DRIVER_INIT(kov);
	DECLARE_DRIVER_INIT(py2k2);
	DECLARE_DRIVER_INIT(drgw2);
	DECLARE_DRIVER_INIT(drgw3);
	DECLARE_DRIVER_INIT(orlegend);
	DECLARE_DRIVER_INIT(pstar);
	TILE_GET_INFO_MEMBER(get_pgm_tx_tilemap_tile_info);
	TILE_GET_INFO_MEMBER(get_pgm_bg_tilemap_tile_info);
	DECLARE_VIDEO_START(pgm);
	DECLARE_MACHINE_START(pgm);
	DECLARE_MACHINE_RESET(pgm);
	UINT32 screen_update_pgm(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void screen_eof_pgm(screen_device &screen, bool state);
	TIMER_DEVICE_CALLBACK_MEMBER(pgm_interrupt);

	// from pgmprot5.c
	int protection_address;

	UINT16 dw2_asic_reg[2];
	UINT8 dw2_asic_z;
	UINT8 dw2_asic_y;
	UINT16 dw2_asic_hold;

	DECLARE_READ16_MEMBER( dw2_d80000_r );
	DECLARE_WRITE16_MEMBER( dw2_d80000_w );
	DECLARE_WRITE16_MEMBER(dw2_unk_w);
	void pgm_dw2_decrypt();
	void drgwld2_common_init();
	inline void pgm_draw_pix( int xdrawpos, int pri, UINT16* dest, UINT8* destpri, UINT16 srcdat);
	inline void pgm_draw_pix_nopri( int xdrawpos, UINT16* dest, UINT8* destpri, UINT16 srcdat);
	inline void pgm_draw_pix_pri( int xdrawpos, UINT16* dest, UINT8* destpri, UINT16 srcdat);
	void draw_sprite_line( int wide, UINT16* dest, UINT8* destpri, int xzoom, int xgrow, int flip, int xpos, int pri, int realxsize, int palt, int draw );
	void draw_sprite_new_zoomed( int wide, int high, int xpos, int ypos, int palt, int flip, bitmap_ind16 &bitmap, bitmap_ind8 &priority_bitmap, UINT32 xzoom, int xgrow, UINT32 yzoom, int ygrow, int pri );
	void draw_sprite_line_basic( int wide, UINT16* dest, UINT8* destpri, int flip, int xpos, int pri, int realxsize, int palt, int draw );
	void draw_sprite_new_basic( int wide, int high, int xpos, int ypos, int palt, int flip, bitmap_ind16 &bitmap, bitmap_ind8 &priority_bitmap, int pri );
	void draw_sprites( bitmap_ind16& spritebitmap, UINT16 *sprite_source, bitmap_ind8& priority_bitmap );
	void expand_32x32x5bpp();
	void expand_colourdata(  );
	void pgm_basic_init( bool set_bank = true);
};


/* for machine/pgmprot.c type games */
class pgm_asic3_state : public pgm_state
{
public:
	pgm_asic3_state(const machine_config &mconfig, device_type type, const char *tag)
		: pgm_state(mconfig, type, tag) {
	}

	// ASIC 3 (oriental legends protection)
	UINT8         m_asic3_reg;
	UINT8         m_asic3_latch[3];
	UINT8         m_asic3_x;
	UINT8         m_asic3_y;
	UINT8         m_asic3_z;
	UINT8         m_asic3_h1;
	UINT8         m_asic3_h2;
	UINT16        m_asic3_hold;

	DECLARE_DRIVER_INIT(orlegend);
	void asic3_compute_hold();
	DECLARE_READ16_MEMBER( pgm_asic3_r );
	DECLARE_WRITE16_MEMBER( pgm_asic3_w );
	DECLARE_WRITE16_MEMBER( pgm_asic3_reg_w );
};


/* for machine/pgmprot1.c type games */
class pgm_arm_type1_state : public pgm_state
{
public:
	pgm_arm_type1_state(const machine_config &mconfig, device_type type, const char *tag)
		: pgm_state(mconfig, type, tag),
			m_arm7_shareram(*this, "arm7_shareram"),
			m_prot(*this, "prot") {
		m_curslots = 0;
		m_puzzli_54_trigger = 0;
	}

	/////////////// simulations
	UINT16 m_value0;
	UINT16 m_value1;
	UINT16 m_valuekey;
	UINT16 m_ddp3lastcommand;
	UINT32 m_valueresponse;
	int m_curslots;
	UINT32 m_slots[0x100];

	// pstars / oldsplus / kov
	UINT16 m_pstar_e7_value;
	UINT16 m_pstar_b1_value;
	UINT16 m_pstar_ce_value;
	UINT16 m_kov_c0_value;
	UINT16 m_kov_cb_value;
	UINT16 m_kov_fe_value;
	UINT16 m_extra_ram[0x100];
	// puzzli2
	INT32 m_puzzli_54_trigger;

	typedef void (pgm_arm_type1_state::*pgm_arm_sim_command_handler)(int pc);

	pgm_arm_sim_command_handler arm_sim_handler;

	/////////////// emulation
	UINT16        m_pgm_arm_type1_highlatch_arm_w;
	UINT16        m_pgm_arm_type1_lowlatch_arm_w;
	UINT16        m_pgm_arm_type1_highlatch_68k_w;
	UINT16        m_pgm_arm_type1_lowlatch_68k_w;
	UINT32        m_pgm_arm_type1_counter;
	optional_shared_ptr<UINT32> m_arm7_shareram;

	optional_device<cpu_device> m_prot;

	DECLARE_DRIVER_INIT(photoy2k);
	DECLARE_DRIVER_INIT(kovsh);
	DECLARE_DRIVER_INIT(kovshp);
	DECLARE_DRIVER_INIT(kovshxas);
	DECLARE_DRIVER_INIT(kovlsqh2);
	DECLARE_DRIVER_INIT(kovqhsgs);
	DECLARE_DRIVER_INIT(ddp3);
	DECLARE_DRIVER_INIT(ket);
	DECLARE_DRIVER_INIT(espgal);
	DECLARE_DRIVER_INIT(puzzli2);
	DECLARE_DRIVER_INIT(py2k2);
	DECLARE_DRIVER_INIT(pgm3in1);
	DECLARE_DRIVER_INIT(pstar);
	DECLARE_DRIVER_INIT(kov);
	DECLARE_DRIVER_INIT(kovboot);
	DECLARE_DRIVER_INIT(oldsplus);
	DECLARE_MACHINE_START(pgm_arm_type1);

	DECLARE_READ32_MEMBER( pgm_arm7_type1_protlatch_r );
	DECLARE_WRITE32_MEMBER( pgm_arm7_type1_protlatch_w );
	DECLARE_READ16_MEMBER( pgm_arm7_type1_68k_protlatch_r );
	DECLARE_WRITE16_MEMBER( pgm_arm7_type1_68k_protlatch_w );
	DECLARE_READ16_MEMBER( pgm_arm7_type1_ram_r );
	DECLARE_WRITE16_MEMBER( pgm_arm7_type1_ram_w );
	DECLARE_READ32_MEMBER( pgm_arm7_type1_unk_r );
	DECLARE_READ32_MEMBER( pgm_arm7_type1_exrom_r );
	DECLARE_READ32_MEMBER( pgm_arm7_type1_shareram_r );
	DECLARE_WRITE32_MEMBER( pgm_arm7_type1_shareram_w );
	void pgm_arm7_type1_latch_init();
	DECLARE_READ16_MEMBER( kovsh_fake_region_r );
	DECLARE_WRITE16_MEMBER( kovshp_asic27a_write_word );
	void pgm_decode_kovlsqh2_tiles();
	void pgm_decode_kovlsqh2_sprites(UINT8 *src );
	void pgm_decode_kovlsqh2_samples();
	void pgm_decode_kovqhsgs_program();
	void pgm_decode_kovqhsgs2_program();
	DECLARE_READ16_MEMBER( pgm_arm7_type1_sim_r );
	void command_handler_ddp3(int pc);
	void command_handler_puzzli2(int pc);
	void command_handler_py2k2(int pc);
	void command_handler_pstars(int pc);
	void command_handler_kov(int pc);
	void command_handler_oldsplus(int pc);
	DECLARE_WRITE16_MEMBER( pgm_arm7_type1_sim_w );
	DECLARE_READ16_MEMBER( pgm_arm7_type1_sim_protram_r );
	DECLARE_READ16_MEMBER( pstars_arm7_type1_sim_protram_r );
};

/* for machine/pgmprot2.c type games */
class pgm_arm_type2_state : public pgm_state
{
public:
	pgm_arm_type2_state(const machine_config &mconfig, device_type type, const char *tag)
		: pgm_state(mconfig, type, tag),
			m_arm_ram(*this, "arm_ram"),
			m_arm7_shareram(*this, "arm7_shareram"),
			m_prot(*this, "prot") {
	}
	// kov2
	UINT32        m_kov2_latchdata_68k_w;
	UINT32        m_kov2_latchdata_arm_w;

	required_shared_ptr<UINT32> m_arm_ram;
	required_shared_ptr<UINT32> m_arm7_shareram;

	optional_device<cpu_device> m_prot;

	DECLARE_DRIVER_INIT(kov2);
	DECLARE_DRIVER_INIT(kov2p);
	DECLARE_DRIVER_INIT(martmast);
	DECLARE_DRIVER_INIT(ddp2);
	DECLARE_DRIVER_INIT(dw2001);
	DECLARE_DRIVER_INIT(dwpc);
	DECLARE_MACHINE_START(pgm_arm_type2);
	DECLARE_READ32_MEMBER( arm7_latch_arm_r );
	DECLARE_WRITE32_MEMBER( arm7_latch_arm_w );
	DECLARE_READ32_MEMBER( arm7_shareram_r );
	DECLARE_WRITE32_MEMBER( arm7_shareram_w );
	DECLARE_READ16_MEMBER( arm7_latch_68k_r );
	DECLARE_WRITE16_MEMBER( arm7_latch_68k_w );
	DECLARE_READ16_MEMBER( arm7_ram_r );
	DECLARE_WRITE16_MEMBER( arm7_ram_w );
	void kov2_latch_init();
	DECLARE_WRITE32_MEMBER( martmast_arm_region_w );
	DECLARE_WRITE32_MEMBER( kov2_arm_region_w );
	DECLARE_READ32_MEMBER( ddp2_speedup_r );
	DECLARE_READ16_MEMBER( ddp2_main_speedup_r );
};



/* for machine/pgmprot3.c type games */
class pgm_arm_type3_state : public pgm_state
{
public:
	pgm_arm_type3_state(const machine_config &mconfig, device_type type, const char *tag)
		: pgm_state(mconfig, type, tag),
			m_arm_ram(*this, "arm_ram"),
			m_prot(*this, "prot") {
	}
	// svg
	int           m_svg_ram_sel;
	UINT32 *      m_svg_shareram[2];    //for 5585G MACHINE

	UINT32        m_svg_latchdata_68k_w;
	UINT32        m_svg_latchdata_arm_w;
	required_shared_ptr<UINT32> m_arm_ram;

	optional_device<cpu_device> m_prot;

	DECLARE_DRIVER_INIT(theglad);
	DECLARE_DRIVER_INIT(svg);
	DECLARE_DRIVER_INIT(svgpcb);
	DECLARE_DRIVER_INIT(killbldp);
	DECLARE_DRIVER_INIT(dmnfrnt);
	DECLARE_DRIVER_INIT(happy6);
	DECLARE_MACHINE_START(pgm_arm_type3);
	DECLARE_WRITE32_MEMBER( svg_arm7_ram_sel_w );
	DECLARE_READ32_MEMBER( svg_arm7_shareram_r );
	DECLARE_WRITE32_MEMBER( svg_arm7_shareram_w );
	DECLARE_READ16_MEMBER( svg_m68k_ram_r );
	DECLARE_WRITE16_MEMBER( svg_m68k_ram_w );
	DECLARE_READ16_MEMBER( svg_68k_nmi_r );
	DECLARE_WRITE16_MEMBER( svg_68k_nmi_w );
	DECLARE_WRITE16_MEMBER( svg_latch_68k_w );
	DECLARE_READ16_MEMBER( svg_latch_68k_r );
	DECLARE_READ32_MEMBER( svg_latch_arm_r );
	DECLARE_WRITE32_MEMBER( svg_latch_arm_w );
	void svg_basic_init();
	void pgm_create_dummy_internal_arm_region();
	void svg_latch_init();
	DECLARE_READ32_MEMBER( dmnfrnt_speedup_r );
	DECLARE_READ16_MEMBER( dmnfrnt_main_speedup_r );
};


/* for machine/pgmprot4.c type games */
class pgm_022_025_state : public pgm_state
{
public:
	pgm_022_025_state(const machine_config &mconfig, device_type type, const char *tag)
		: pgm_state(mconfig, type, tag),
			m_sharedprotram(*this, "sharedprotram") {
	}

	const UINT8 (*m_kb_source_data)[0xec];
	INT32 m_kb_source_data_offset;

	DECLARE_WRITE16_MEMBER( killbld_mainram_w ); // debug

	UINT32 m_kb_game_id;

	UINT16        m_kb_prot_hold;
	UINT16        m_kb_prot_hilo;
	UINT16        m_kb_prot_hilo_select;

	int           m_kb_cmd;
	int           m_kb_reg;
	int           m_kb_ptr;
	UINT8         m_kb_swap;
	UINT32        m_kb_regs[0x10];

	required_shared_ptr<UINT16> m_sharedprotram;

	DECLARE_DRIVER_INIT(killbld);
	DECLARE_DRIVER_INIT(drgw3);
	DECLARE_MACHINE_RESET(killbld);
	DECLARE_MACHINE_RESET(dw3);
	void pgm_dw3_decrypt();
	void pgm_killbld_decrypt();
	void killbld_protection_calculate_hilo();
	void killbld_protection_calculate_hold(int y, int z);
	void IGS022_do_dma(UINT16 src, UINT16 dst, UINT16 size, UINT16 mode);
	void IGS022_reset();
	void IGS022_handle_command();

	DECLARE_WRITE16_MEMBER( killbld_igs025_prot_w );
	DECLARE_READ16_MEMBER( killbld_igs025_prot_r );
};

/* for machine/pgmprot6.c type games */
class pgm_028_025_state : public pgm_state
{
public:
	pgm_028_025_state(const machine_config &mconfig, device_type type, const char *tag)
		: pgm_state(mconfig, type, tag),
			m_sharedprotram(*this, "sharedprotram") {
	}

	int           m_olds_cmd;
	int           m_olds_reg;
	int           m_olds_ptr;
	UINT16        m_olds_bs;
	UINT16        m_olds_cmd3;
	UINT16        m_olds_prot_hold;
	UINT16        m_olds_prot_hilo;
	UINT16        m_olds_prot_hilo_select;
	const UINT8  *m_olds_prot_hilo_source2;
	required_shared_ptr<UINT16> m_sharedprotram;

	DECLARE_DRIVER_INIT(olds);
	DECLARE_MACHINE_RESET(olds);

	UINT32 olds_prot_addr( UINT16 addr );
	UINT32 olds_read_reg( UINT16 addr );
	void olds_write_reg( UINT16 addr, UINT32 val );
	DECLARE_READ16_MEMBER( olds_r );
	DECLARE_WRITE16_MEMBER( olds_w );
	DECLARE_READ16_MEMBER( olds_prot_swap_r );
	void IGS028_do_dma(UINT16 src, UINT16 dst, UINT16 size, UINT16 mode);
	void olds_protection_calculate_hilo();
	void olds_protection_calculate_hold(int y, int z);
};




/*----------- defined in drivers/pgm.c -----------*/

INPUT_PORTS_EXTERN( pgm );

GFXDECODE_EXTERN( pgm );

MACHINE_CONFIG_EXTERN( pgm );
MACHINE_CONFIG_EXTERN( pgmbase );

ADDRESS_MAP_EXTERN( pgm_z80_mem, 8 );
ADDRESS_MAP_EXTERN( pgm_z80_io, 8 );
void pgm_sound_irq( device_t *device, int level );

ADDRESS_MAP_EXTERN( pgm_mem, 16 );
ADDRESS_MAP_EXTERN( pgm_basic_mem, 16 );
ADDRESS_MAP_EXTERN( pgm_base_mem, 16 );




/*----------- defined in machine/pgmprot.c -----------*/


INPUT_PORTS_EXTERN( orlegend );
INPUT_PORTS_EXTERN( orld105k );

MACHINE_CONFIG_EXTERN( pgm_asic3 );

/*----------- defined in machine/pgmprot1.c -----------*/

/* emulations */

/* simulations */

MACHINE_CONFIG_EXTERN( pgm_arm_type1 );
MACHINE_CONFIG_EXTERN( pgm_arm_type1_sim );
MACHINE_CONFIG_EXTERN( pgm_arm_type1_cave );

INPUT_PORTS_EXTERN( sango );
INPUT_PORTS_EXTERN( sango_ch );
INPUT_PORTS_EXTERN( photoy2k );
INPUT_PORTS_EXTERN( oldsplus );
INPUT_PORTS_EXTERN( pstar );
INPUT_PORTS_EXTERN( py2k2 );
INPUT_PORTS_EXTERN( puzzli2 );
INPUT_PORTS_EXTERN( kovsh );

/*----------- defined in machine/pgmprot2.c -----------*/

/* emulations */
MACHINE_CONFIG_EXTERN( pgm_arm_type2 );

/* simulations (or missing) */
INPUT_PORTS_EXTERN( ddp2 );
INPUT_PORTS_EXTERN( kov2 );
INPUT_PORTS_EXTERN( martmast );
INPUT_PORTS_EXTERN( dw2001 );

/*----------- defined in machine/pgmprot3.c -----------*/

MACHINE_CONFIG_EXTERN( pgm_arm_type3 );

/*----------- defined in machine/pgmprot4.c -----------*/

MACHINE_CONFIG_EXTERN( pgm_022_025_kb );
MACHINE_CONFIG_EXTERN( pgm_022_025_dw );

INPUT_PORTS_EXTERN( killbld );
INPUT_PORTS_EXTERN( dw3 );

/*----------- defined in machine/pgmprot6.c -----------*/

MACHINE_CONFIG_EXTERN( pgm_028_025_ol );
INPUT_PORTS_EXTERN( olds );
