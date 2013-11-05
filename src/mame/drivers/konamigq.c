/***************************************************************************

  Konami GQ System - Arcade PSX Hardware
  ======================================
  Driver by R. Belmont & smf

  Crypt Killer
  Konami, 1995

  PCB Layout
  ----------

  GQ420  PWB354905B
  |----------------------------------------------------------|
  |CN14     420A01.2G  420A02.3M           CN6  CN7   CN8    |
  |056602    6264                                            |
  |LA4705    6264         058141  424800                     |
  |          68000                                           |
  |  18.432MHz      PAL   056800  424800                     |
  |  32MHz    48MHz PAL           424800                     |
  |  RESET_SW             TMS57002                           |
  |                               M514256                    |
  |                               M514256                    |
  |J  000180                                                 |
  |A                              MACH110                    |
  |M         53.693175MHz         (000619A)                  |
  |M         KM4216V256                                      |
  |A         KM4216V256     CXD8538Q        PAL (000613)     |
  |          KM4216V256                     PAL (000612)     |
  | TEST_SW  KM4216V256                     PAL (000618)     |
  |   CXD2923AR   67.7376MHz                                 |
  | DIPSW(8)                   93C46       NCR 53CF96-2      |
  | KM48V514  KM48V514         420B03.27P    HD_LED          |
  | KM48V514  KM48V514                                       |
  | CN3                CXD8530BQ      TOSHIBA MK1924FBV      |
  | KM48V514  KM48V514                (U420UAA04)            |
  | KM48V514  KM48V514                                       |
  |----------------------------------------------------------|

  Notes:
        CN6, CN7, CN8: For connection of guns.
        CN3 : For connection of extra controls/buttons.
        CN14: For connection of additional speaker for stereo output.
        68000 clock: 8.000MHz
        Vsync: 60Hz
*/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "cpu/psx/psx.h"
#include "video/psx.h"
#include "machine/am53cf96.h"
#include "machine/eepromser.h"
#include "machine/mb89371.h"
#include "machine/scsibus.h"
#include "machine/scsihd.h"
#include "sound/k054539.h"

class konamigq_state : public driver_device
{
public:
	konamigq_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_am53cf96(*this, "scsi:am53cf96"),
		m_maincpu(*this, "maincpu"),
		m_soundcpu(*this, "soundcpu")
	{
	}

	required_device<am53cf96_device> m_am53cf96;

	UINT8 m_sndto000[ 16 ];
	UINT8 m_sndtor3k[ 16 ];
	UINT8 *m_p_n_pcmram;
	UINT8 m_sector_buffer[ 512 ];
	DECLARE_WRITE16_MEMBER(soundr3k_w);
	DECLARE_READ16_MEMBER(soundr3k_r);
	DECLARE_WRITE16_MEMBER(eeprom_w);
	DECLARE_WRITE8_MEMBER(pcmram_w);
	DECLARE_READ8_MEMBER(pcmram_r);
	DECLARE_READ16_MEMBER(sndcomm68k_r);
	DECLARE_WRITE16_MEMBER(sndcomm68k_w);
	DECLARE_READ16_MEMBER(tms57002_data_word_r);
	DECLARE_WRITE16_MEMBER(tms57002_data_word_w);
	DECLARE_READ16_MEMBER(tms57002_status_word_r);
	DECLARE_WRITE16_MEMBER(tms57002_control_word_w);
	DECLARE_DRIVER_INIT(konamigq);
	DECLARE_MACHINE_START(konamigq);
	DECLARE_MACHINE_RESET(konamigq);
	void scsi_dma_read( UINT32 *p_n_psxram, UINT32 n_address, INT32 n_size );
	void scsi_dma_write( UINT32 *p_n_psxram, UINT32 n_address, INT32 n_size );
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_soundcpu;
};

/* Sound */

WRITE16_MEMBER(konamigq_state::soundr3k_w)
{
	m_sndto000[ offset ] = data;
	if( offset == 7 )
	{
		m_soundcpu->set_input_line(1, HOLD_LINE );
	}
}

READ16_MEMBER(konamigq_state::soundr3k_r)
{
	/* hack to help the main program start up */
	if( offset == 2 || offset == 3 )
	{
		return 0;
	}

	return m_sndtor3k[ offset ];
}

/* EEPROM */

static const UINT16 konamigq_def_eeprom[64] =
{
	0x292b, 0x5256, 0x2094, 0x4155, 0x0041, 0x1414, 0x0003, 0x0101,
	0x0103, 0x0000, 0x0707, 0x0001, 0xaa00, 0xaaaa, 0xaaaa, 0xaaaa,
	0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
	0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
	0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
	0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
	0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
	0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa,
};

WRITE16_MEMBER(konamigq_state::eeprom_w)
{
	ioport("EEPROMOUT")->write(data & 0x07, 0xff);
	m_soundcpu->set_input_line(INPUT_LINE_RESET, ( data & 0x40 ) ? CLEAR_LINE : ASSERT_LINE );
}


/* PCM RAM */

WRITE8_MEMBER(konamigq_state::pcmram_w)
{
	m_p_n_pcmram[ offset ] = data;
}

READ8_MEMBER(konamigq_state::pcmram_r)
{
	return m_p_n_pcmram[ offset ];
}

/* Video */

static ADDRESS_MAP_START( konamigq_map, AS_PROGRAM, 32, konamigq_state )
	AM_RANGE(0x1f000000, 0x1f00001f) AM_DEVREADWRITE8("scsi:am53cf96", am53cf96_device, read, write, 0x00ff00ff)
	AM_RANGE(0x1f100000, 0x1f10000f) AM_WRITE16(soundr3k_w, 0xffffffff)
	AM_RANGE(0x1f100010, 0x1f10001f) AM_READ16(soundr3k_r, 0xffffffff)
	AM_RANGE(0x1f180000, 0x1f180003) AM_WRITE16(eeprom_w, 0x0000ffff)
	AM_RANGE(0x1f198000, 0x1f198003) AM_WRITENOP            /* cabinet lamps? */
	AM_RANGE(0x1f1a0000, 0x1f1a0003) AM_WRITENOP            /* indicates gun trigger */
	AM_RANGE(0x1f200000, 0x1f200003) AM_READ_PORT("GUNX1")
	AM_RANGE(0x1f208000, 0x1f208003) AM_READ_PORT("GUNY1")
	AM_RANGE(0x1f210000, 0x1f210003) AM_READ_PORT("GUNX2")
	AM_RANGE(0x1f218000, 0x1f218003) AM_READ_PORT("GUNY2")
	AM_RANGE(0x1f220000, 0x1f220003) AM_READ_PORT("GUNX3")
	AM_RANGE(0x1f228000, 0x1f228003) AM_READ_PORT("GUNY3")
	AM_RANGE(0x1f230000, 0x1f230003) AM_READ_PORT("P1_P2")
	AM_RANGE(0x1f230004, 0x1f230007) AM_READ_PORT("P3_SERVICE")
	AM_RANGE(0x1f238000, 0x1f238003) AM_READ_PORT("DSW")
	AM_RANGE(0x1f300000, 0x1f5fffff) AM_READWRITE8(pcmram_r, pcmram_w, 0x00ff00ff)
	AM_RANGE(0x1f680000, 0x1f68001f) AM_DEVREADWRITE8("mb89371", mb89371_device, read, write, 0x00ff00ff)
	AM_RANGE(0x1f780000, 0x1f780003) AM_WRITENOP /* watchdog? */
ADDRESS_MAP_END

/* SOUND CPU */

READ16_MEMBER(konamigq_state::sndcomm68k_r)
{
	return m_sndto000[ offset ];
}

WRITE16_MEMBER(konamigq_state::sndcomm68k_w)
{
	m_sndtor3k[ offset ] = data;
}

READ16_MEMBER(konamigq_state::tms57002_data_word_r)
{
	return 0;
}

WRITE16_MEMBER(konamigq_state::tms57002_data_word_w)
{
}

READ16_MEMBER(konamigq_state::tms57002_status_word_r)
{
	return 0;
}

WRITE16_MEMBER(konamigq_state::tms57002_control_word_w)
{
}

/* 68000 memory handling */
static ADDRESS_MAP_START( konamigq_sound_map, AS_PROGRAM, 16, konamigq_state )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x200000, 0x2004ff) AM_DEVREADWRITE8("k054539_1", k054539_device, read, write, 0xff00)
	AM_RANGE(0x200000, 0x2004ff) AM_DEVREADWRITE8("k054539_2", k054539_device, read, write, 0x00ff)
	AM_RANGE(0x300000, 0x300001) AM_READWRITE(tms57002_data_word_r,tms57002_data_word_w)
	AM_RANGE(0x400000, 0x40000f) AM_WRITE(sndcomm68k_w)
	AM_RANGE(0x400010, 0x40001f) AM_READ(sndcomm68k_r)
	AM_RANGE(0x500000, 0x500001) AM_READWRITE(tms57002_status_word_r,tms57002_control_word_w)
	AM_RANGE(0x580000, 0x580001) AM_WRITENOP /* ?? */
ADDRESS_MAP_END


static const k054539_interface k054539_config =
{
	"shared"
};

/* SCSI */

void konamigq_state::scsi_dma_read( UINT32 *p_n_psxram, UINT32 n_address, INT32 n_size )
{
	UINT8 *sector_buffer = m_sector_buffer;
	int i;
	int n_this;

	while( n_size > 0 )
	{
		if( n_size > sizeof( m_sector_buffer ) / 4 )
		{
			n_this = sizeof( m_sector_buffer ) / 4;
		}
		else
		{
			n_this = n_size;
		}
		m_am53cf96->dma_read_data( n_this * 4, sector_buffer );
		n_size -= n_this;

		i = 0;
		while( n_this > 0 )
		{
			p_n_psxram[ n_address / 4 ] =
				( sector_buffer[ i + 0 ] << 0 ) |
				( sector_buffer[ i + 1 ] << 8 ) |
				( sector_buffer[ i + 2 ] << 16 ) |
				( sector_buffer[ i + 3 ] << 24 );
			n_address += 4;
			i += 4;
			n_this--;
		}
	}
}

void konamigq_state::scsi_dma_write( UINT32 *p_n_psxram, UINT32 n_address, INT32 n_size )
{
}

DRIVER_INIT_MEMBER(konamigq_state,konamigq)
{
	m_p_n_pcmram = memregion( "shared" )->base() + 0x80000;
}

MACHINE_START_MEMBER(konamigq_state,konamigq)
{
	save_pointer(NAME(m_p_n_pcmram), 0x380000);
	save_item(NAME(m_sndto000));
	save_item(NAME(m_sndtor3k));
	save_item(NAME(m_sector_buffer));
}

MACHINE_RESET_MEMBER(konamigq_state,konamigq)
{
}

static MACHINE_CONFIG_START( konamigq, konamigq_state )
	/* basic machine hardware */
	MCFG_CPU_ADD( "maincpu", CXD8530BQ, XTAL_67_7376MHz )
	MCFG_CPU_PROGRAM_MAP( konamigq_map )

	MCFG_RAM_MODIFY("maincpu:ram")
	MCFG_RAM_DEFAULT_SIZE("4M")

	MCFG_PSX_DMA_CHANNEL_READ( "maincpu", 5, psx_dma_read_delegate( FUNC( konamigq_state::scsi_dma_read ), (konamigq_state *) owner ) )
	MCFG_PSX_DMA_CHANNEL_WRITE( "maincpu", 5, psx_dma_write_delegate( FUNC( konamigq_state::scsi_dma_write ), (konamigq_state *) owner ) )

	MCFG_CPU_ADD( "soundcpu", M68000, 8000000 )
	MCFG_CPU_PROGRAM_MAP( konamigq_sound_map)
	MCFG_CPU_PERIODIC_INT_DRIVER(konamigq_state, irq2_line_hold,  480)

	MCFG_MACHINE_START_OVERRIDE(konamigq_state, konamigq )
	MCFG_MACHINE_RESET_OVERRIDE(konamigq_state, konamigq )

	MCFG_DEVICE_ADD("mb89371", MB89371, 0)
	MCFG_EEPROM_SERIAL_93C46_ADD("eeprom")
	MCFG_EEPROM_SERIAL_DATA(konamigq_def_eeprom, 128)

	MCFG_SCSIBUS_ADD("scsi")
	MCFG_SCSIDEV_ADD("scsi:disk", SCSIHD, SCSI_ID_0)
	MCFG_AM53CF96_ADD("scsi:am53cf96")
	MCFG_AM53CF96_IRQ_HANDLER(DEVWRITELINE("^maincpu:irq", psxirq_device, intin10))

	/* video hardware */
	MCFG_PSXGPU_ADD( "maincpu", "gpu", CXD8538Q, 0x200000, XTAL_53_693175MHz )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_K054539_ADD("k054539_1", 48000, k054539_config)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	MCFG_K054539_ADD("k054539_2", 48000, k054539_config)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END

static INPUT_PORTS_START( konamigq )
	PORT_START("GUNX1")
	PORT_BIT( 0x01ff, 0x011d, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_MINMAX( 0x007d, 0x01bc ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START("GUNY1")
	PORT_BIT( 0x00ff, 0x0078, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX( 0x0000, 0x00ef ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	PORT_START("GUNX2")
	PORT_BIT( 0x01ff, 0x011d, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_MINMAX( 0x007d, 0x01bc ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START("GUNY2")
	PORT_BIT( 0x00ff, 0x0078, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX( 0x0000, 0x00ef ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(2)

	PORT_START("GUNX3")
	PORT_BIT( 0x01ff, 0x011d, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_MINMAX( 0x007d, 0x01bc ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(3)

	PORT_START("GUNY3")
	PORT_BIT( 0x00ff, 0x0078, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX( 0x0000, 0x00ef ) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(3)

	PORT_START("P1_P2")
	PORT_BIT( 0x000001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x000002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x000004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x000008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* trigger */
	PORT_BIT( 0x000010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) /* reload */
	PORT_BIT( 0x000020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x000040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x000080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x010000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x020000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x040000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x080000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) /* trigger */
	PORT_BIT( 0x100000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) /* reload */
	PORT_BIT( 0x200000, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x400000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x800000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("P3_SERVICE")
	PORT_BIT( 0x000001, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x000002, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x000004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x000008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3) /* trigger */
	PORT_BIT( 0x000010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3) /* reload */
	PORT_BIT( 0x000020, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x000040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x000080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x010000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x020000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x040000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x080000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x100000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE( 0x200000, IP_ACTIVE_LOW )
	PORT_BIT( 0x400000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x800000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("DSW")
	PORT_DIPNAME( 0x00000001, 0x01, DEF_STR( Stereo ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Stereo ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Mono ) )
	PORT_DIPNAME( 0x00000002, 0x00, "Stage Set" )
	PORT_DIPSETTING(    0x02, "Endless" )
	PORT_DIPSETTING(    0x00, "6st End" )
	PORT_DIPNAME( 0x00000004, 0x04, "Mirror" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x00000008, 0x08, "Woofer" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x00000010, 0x10, "Number of Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x00000020, 0x20, "Coin Mechanism (2p only)" )
	PORT_DIPSETTING(    0x20, "Common" )
	PORT_DIPSETTING(    0x00, "Independent" )
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_93cxx_device, do_read)

	PORT_START( "EEPROMOUT" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_93cxx_device, di_write)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_93cxx_device, cs_write)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_93cxx_device, clk_write)
INPUT_PORTS_END

ROM_START( cryptklr )
	ROM_REGION( 0x80000, "soundcpu", 0 ) /* 68000 sound program */
	ROM_LOAD16_WORD_SWAP( "420a01.2g", 0x000000, 0x080000, CRC(84fc2613) SHA1(e06f4284614d33c76529eb43b168d095200a9eac) )

	ROM_REGION( 0x400000, "shared", 0 )
	ROM_LOAD( "420a02.3m",    0x000000, 0x080000, CRC(2169c3c4) SHA1(6d525f10385791e19eb1897d18f0bab319640162) )

	ROM_REGION32_LE( 0x080000, "maincpu:rom", 0 ) /* bios */
	ROM_LOAD( "420b03.27p",   0x0000000, 0x080000, CRC(aab391b1) SHA1(bf9dc7c0c8168c22a4be266fe6a66d3738df916b) )

	DISK_REGION( "scsi:disk:image" )
	DISK_IMAGE( "420uaa04", 0, SHA1(67cb1418fc0de2a89fc61847dc9efb9f1bebb347) )
ROM_END

GAME( 1995, cryptklr, 0, konamigq, konamigq, konamigq_state, konamigq, ROT0, "Konami", "Crypt Killer (GQ420 UAA)", GAME_IMPERFECT_GRAPHICS )
