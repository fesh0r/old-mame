/*

GNET Motherboard
Taito, 1998

The Taito GNET System comprises the following main parts....
- Sony ZN-2 Motherboard (Main CPU/GPU/SPU, RAM, BIOS, EEPROM & peripheral interfaces)
- Taito FC PCB (Sound hardware & FLASHROMs for storage of PCMCIA cart contents)
- Taito CD PCB (PCMCIA cart interface)

Also available are...
- Optional Communication Interface PCB
- Optional Save PCB

On power-up, the system checks for a PCMCIA cart. If the cart matches the contents of the flashROMs,
the game boots immediately with no delay. If the cart doesn't match, it re-flashes the flashROMs with _some_
of the information contained in the cart, which takes approximately 2-3 minutes. The game then resets
and boots up.

If no cart is present on power-up, the Taito GNET logo is displayed, then a message 'SYSTEM ERROR'
Since the logo is shown on boot even without a cart, there must be another sub-BIOS for the initial booting,
which I suspect is one of the flashROMs that is acting like a standard ROM and is not flashed at all.
Upon inspecting the GNET top board, it appears flash.u30 is the sub-BIOS and perhaps U27 is something sound related.
The flashROMs at U55, U56 & U29 appear to be the ones that are re-flashed when swapping game carts.

PCB Layouts
-----------
(Standard ZN2 Motherboard)

ZN-2 COH-3000 (sticker says COH-3002T denoting Taito GNET BIOS version)
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       S301           COH3002T.353                      |
|                                                        |
|                                                        |
|J                                                       |
|                                                        |
|A              814260    CXD2925Q     EPM7064           |
|                                                        |
|M                                     67.73MHz          |
|                                                        |
|M                                                       |
|            S551    KM4132G271BQ-8                      |
|A                                                       |
|                                CXD8654Q    CXD8661R    |
|                    KM4132G271BQ-8                      |
|CN505  CN506                   53.693MHz    100MHz      |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|            MC44200FT                                   |
|  NEC_78081G503        KM416V1204BT-L5  KM416V1204BT-L5 |
|                                                        |
|CN651  CN652                 *                 *        |
|                CN654                                   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      S301  - Slide switch for stereo or mono sound output
      S551  - Dip switch (4 position, defaults all OFF)

      COH3002T.353   - GNET BIOS 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM
      814260-70      - 256K x16 (4MBit) DRAM
      KM4132G271BQ-8 - 128K x 32Bit x 2 Banks SGRAM
      KM416V1204BT-L5- 1M x16 EDO DRAM
      EPM7064        - Altera EPM7064QC100 CPLD (QFP100)
      CAT702         - Protection chip labelled 'TT10' (DIP20)
      *              - Unpopulated position for additional KM416V1204BT-L5 RAMs


FC PCB  K91X0721B  M43X0337B
|--------------------------------------------|
|   |---------------------------|            |
|   |---------------------------|            |
| NJM2100  NJM2100                           |
| MB87078                                    |
| *MB3773     XC95108         DIP40   CAT702 |
| *ADM708AR                                  |
| *UPD6379GR                                 |
|             FLASH.U30                      |
|                                            |
| DIP24                                      |
|                  *RF5C296                  |
| -------CD-PCB------- _                     |
| |                   | |                    |
| |                   | |                    |
| |                   | |                    |
| |                   | |                    |
| |                   | |                    |
| |                   | |                    |
| |                   | |                    |
| |                   |-|                    |
| --------------------                       |
|          M66220FP   FLASH.U55   FLASH16.U29|
|      FLASH.U27             FLASH.U56       |
|*LC321664                                   |
| TMS57002DPHA                *ZSG-2         |
|           LH52B256      25MHz              |
|   MN1020012A                               |
|--------------------------------------------|
Notes:
      DIP40           - Unpopulated socket for 8MBit DIP40 EPROM type AM27C800
      DIP24           - Unpopulated position for FM1208 DIP24 IC
      FLASH.U30       - Intel TE28F160 16MBit FLASHROM (TSOP56)
      FLASH.U29/55/56 - Intel TE28F160 16MBit FLASHROM (TSOP56)
      FLASH.U27       - Intel E28F400 4MBit FLASHROM (TSOP48)
      LH52B256        - Sharp 32K x8 SRAM (SOP28)
      LC321664        - Sanyo 64K x16 EDO DRAM (SOP40)
      XC95108         - XILINX XC95108 CPLD labelled 'E65-01' (QFP100)
      MN1020012A      - Panasonic MN1020012A Sound CPU (QFP128)
      ZSG-2           - Zoom Corp ZSG-2 Sound DSP (QFP100)
      TMS57002DPHA    - Texas Instruments TMS57002DPHA Sound DSP (QFP80)
      RF5C296         - Ricoh RF5C296 PCMCIA controller (TQFP144)
      M66220FP        - 256 x8bit Mail-Box (Inter-MPU data transfer)
      CAT702          - Protection chip labelled 'TT16' (DIP20)
      CD PCB          - A PCMCIA cart slot connector mounted onto a small daughterboard
      *               - These parts located under the PCB


Taito G-Net card info
---------------------

The G-Net system uses a custom PCMCIA card for game software storage. The card is
locked with a password and can't be read by conventional means.
Some of the cards are made in separate pieces and can be opened. However some
are encased in a single-piece steel shell and opening it up destroys the card.
Some of the later games came packaged as a Compact Flash card and a PCMCIA to CF
adapter, however these cards were also locked the same as the older type.
The game uses an analog wheel (5k potentiometer) in the shape of a hand-held
control unit and a trigger (another 5k potentiometer) used for acceleration
and brake. The trigger and wheel are self centering. If the trigger is pulled back
(like firing a gun) the car goes faster. If the trigger is pushed forward the car
slows down. The controller looks a lot like the old Scalextric controllers (remember those?  :-)

The controller is connected to the ZN2 main board to the 10 pin connector labelled
'ANALOG'. Using two 5k-Ohm potentiometers, power (+5V) and ground are taken from the JAMMA
edge connector or directly from the power supply. The output of the steering pot is
connected to pin 2 and the output of the acceleration pot is connected to pin 3.
The ANALOG connector output pins are tied directly to a chip next to the connector marked
'NEC 78081G503 9810KX189'. This is a NEC 8-bit 78K0-family microcontroller with on-chip 8k ROM,
256 bytes RAM, 33 I/O ports, 8-bit resolution 8-channel A/D converter, 3-channel timer, 1-channel
3-wire serial interface interrupt control (USART) and other peripheral hardware.


Card PCB Layouts
----------------

Type 1 (standard 'Taito' type, as found on most G-Net games)
------ (This type has separate top and bottom pieces which are glued together and
       (can be opened if done 'carefully'. But be careful the edges of the top lid are SHARP!)

Top
---

RO-055A AI AM-1
|-|-------------------------------------|
| |        |---------|     |--------|   |
| |        |S2812A150|     |CXK58257|   |
| |        |---------|     |--------|   |
| |                                     |
| |        |-------|     |-----------|  |
| |        |       |     |           |  |
| |        |ML-101 |     |           |  |
| |        |       |     |  F1PACK   |  |
| |        |-------|     |           |  |
| |                      |           |  |
| |        |-----|       |-----------|  |
| |        |ROM1 |                      |
| |        |-----|                      |
| |                                     |
|-|-------------------------------------|
Notes:
      F1PACK    - TEL F1PACK(tm) TE6350B 9744 E0B (TQFP176)
      S2812A150 - Seiko Instruments 2k x8 parallel EEPROM (TSOP28)
      ML-101    - ML-101 24942-6420 9833 Z03 JAPAN (TQFP100, NOTE! This chip looks like it is Fujitsu-manufactured)
      CXK58257  - SONY CXK58257 32k x8 SRAM (TSOP28)
      ROM1      - TOSHIBA TC58V32FT 4M x8 (32MBit) CMOS NAND Flash EEPROM 3.3Volt (TSOP44)
                  The ROMs use a non-standard format and can not be read by conventional methods.

Bottom
------

|-|-------------------------------------|
| |                                     |
| |  |-----|      |-----|     |-----|   |
| |  |ROM2 |      |ROM3 |     |ROM4 |   |
| |  |-----|      |-----|     |-----|   |
| |                                     |
| |                                     |
| |  |-----|      |-----|     |-----|   |
| |  |ROM5 |      |ROM6 |     |ROM7 |   |
| |  |-----|      |-----|     |-----|   |
| |                                     |
| |                                     |
| |  |-----|      |-----|     |-----|   |
| |  |ROM8 |      |ROM9 |     |ROM10|   |
| |  |-----|      |-----|     |-----|   |
| |                                     |
|-|-------------------------------------|
Notes:
      ROM2-10   - TOSHIBA TC58V32FT 4M x8 (32MBit) CMOS NAND Flash EEPROM 3.3Volt (TSOP44)
                  The ROMs use a non-standard format and can not be read by conventional methods.

      Note: All ROMs have no markings (except part number) and no labels. There are also
      no PCB location marks. The numbers I've assigned to the ROMs are made up for
      simplicity. The actual cards have been dumped as a single storage device.

      Confirmed usage on.... (not all games listed)
      Chaos Heat
      Flip Maze
      Kollon
      Mahjong OH
      Nightraid
      Otenki Kororin / Weather Tales
      Psyvariar Medium Unit
      Psyvariar Revision
      Ray Crisis
      RC de Go
      Shanghai Shoryu Sairin
      Shikigami no Shiro
      Souten Ryu
      Space Invaders Anniversary
      Super Puzzle Bobble (English)
      XIIStag
      Zoku Otenami Haiken
      Zooo


Type 2 (3rd party type 'sealed' cards)
------

Note only 1 card was sacrificed and opened.

Top
---

|-|-------------------------------------|
| |             18.00         |-----|   |
| |                           |U15  |   |
| |        |---------|        |-----|   |
| |        |20H2877  |                  |
| |        |IBM0398  |        |-----|   |
| |        |1B37001TQ|        |U13  |   |
| |        |KOREA    |        |-----|   |
| |        |---------|                  |
| |                                     |
| |                                     |
| |   |-----|     |-------|             |
| |   |U10  |     |D431000|             |
| |   |-----|     |-------|             |
| |                                     |
|-|-------------------------------------|
Notes:
      IBM0398 - Custom IC marked 20H2877 IBM0398 1B37001TQ KOREA (TQFP176)
      D431000 - NEC D431000 128k x8 SRAM (TSOP32)
      18.00   - Small square white 'thing', may be an oscillator at 18MHz?
      U*      - TOSHIBA TC58V32FT 4M x8 (32MBit) CMOS NAND Flash EEPROM 3.3Volt (TSOP44)
                The ROMs use a non-standard format and can not be read by conventional methods.

Bottom
------

|-|-------------------------------------|
| |                                     |
| |  |-----|      |-----|     |-----|   |
| |  |U1   |      |U2   |     |U3   |   |
| |  |-----|      |-----|     |-----|   |
| |                                     |
| |                                     |
| |  |-----|      |-----|     |-----|   |
| |  |U4   |      |U5   |     |U6   |   |
| |  |-----|      |-----|     |-----|   |
| |                                     |
| |                                     |
| |               |-----|     |-----|   |
| |               |U7   |     |U8   |   |
| |               |-----|     |-----|   |
| |                                     |
|-|-------------------------------------|
Notes:
      U*  - TOSHIBA TC58V32FT 4M x8 (32MBit) CMOS NAND Flash EEPROM 3.3Volt (TSOP44)
            The ROMs use a non-standard format and can not be read by conventional methods.
            U8 not populated in Nightraid card, but may be populated in other cards.

      Note: All ROMs have no markings (except part number) and no labels. There are PCB
            location marks. The actual cards have been dumped as a single storage device.

      Confirmed usage on.... (not all games listed)
      Nightraid (another one, not the same as listed above)

      Based on card type (made with single sealed steel shell) these are also using the same PCB...
      XIIStag (another one, not the same as listed above)
      Go By RC
      Space Invaders Anniversary (another one, not the same as listed above)
      Super Puzzle Bobble (Japan)
      Usagi

Type 3 (PCMCIA Compact Flash Adaptor + Compact Flash card, sealed together with the game?s label)
------

       The Compact Flash card is read protected, it is a custom Sandisk SDCFB-64 Card (64MByte)

       Confirmed usage on.... (not all games listed)
       Otenami Haiken Final
       Kollon
       Zooo
*/

#include "emu.h"
#include "audio/taito_zm.h"
#include "cpu/psx/psx.h"
#include "machine/at28c16.h"
#include "machine/ataflash.h"
#include "machine/bankdev.h"
#include "machine/intelfsh.h"
#include "machine/mb3773.h"
#include "machine/rf5c296.h"
#include "machine/znsec.h"
#include "machine/zndip.h"
#include "sound/spu.h"
#include "video/psx.h"

class taitogn_state : public driver_device
{
public:
	taitogn_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_znsec0(*this,"maincpu:sio0:znsec0"),
		m_znsec1(*this,"maincpu:sio0:znsec1"),
		m_zndip(*this,"maincpu:sio0:zndip"),
		m_maincpu(*this, "maincpu"),
		m_mn10200(*this, "mn10200"),
		m_flashbank(*this, "flashbank"),
		m_mb3773(*this, "mb3773")
	{
	}

	DECLARE_READ8_MEMBER(control_r);
	DECLARE_WRITE8_MEMBER(control_w);
	DECLARE_WRITE16_MEMBER(control2_w);
	DECLARE_READ8_MEMBER(control3_r);
	DECLARE_WRITE8_MEMBER(control3_w);
	DECLARE_READ16_MEMBER(gn_1fb70000_r);
	DECLARE_WRITE16_MEMBER(gn_1fb70000_w);
	DECLARE_READ16_MEMBER(hack1_r);
	DECLARE_READ8_MEMBER(znsecsel_r);
	DECLARE_WRITE8_MEMBER(znsecsel_w);
	DECLARE_READ8_MEMBER(boardconfig_r);
	DECLARE_WRITE8_MEMBER(coin_w);
	DECLARE_READ8_MEMBER(coin_r);
	DECLARE_READ8_MEMBER(gnet_mahjong_panel_r);
	DECLARE_MACHINE_RESET(coh3002t);

protected:
	virtual void driver_start();

private:
	UINT8 m_control;
	UINT16 m_control2;
	UINT8 m_control3;
	int m_v;

	UINT8 m_n_znsecsel;

	UINT8 m_coin_info;

	required_device<znsec_device> m_znsec0;
	required_device<znsec_device> m_znsec1;
	required_device<zndip_device> m_zndip;
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_mn10200;
	required_device<address_map_bank_device> m_flashbank;
	required_device<mb3773_device> m_mb3773;
};


// Misc. controls

READ8_MEMBER(taitogn_state::control_r)
{
	//      fprintf(stderr, "gn_r %08x @ %08x (%s)\n", 0x1fb00000+4*offset, mem_mask, machine().describe_context());
	return m_control;
}

WRITE8_MEMBER(taitogn_state::control_w)
{
	// 20 = watchdog
	// 04 = select bank

	COMBINE_DATA(&m_control);

	m_mb3773->write_line_ck((data & 0x20) >> 5);

#if 0
	if((p ^ control) & ~0x20)
		fprintf(stderr, "control = %c%c.%c %c%c%c%c (%s)\n",
				control & 0x80 ? '1' : '0',
				control & 0x40 ? '1' : '0',
				control & 0x10 ? '1' : '0',
				control & 0x08 ? '1' : '0',
				control & 0x04 ? 'f' : '-',
				control & 0x02 ? '1' : '0',
				control & 0x01 ? '1' : '0',
				machine().describe_context());
#endif

	// According to the rom code, bits 1-0 may be part of the bank
	// selection too, but they're always 0.
	m_flashbank->set_bank(m_control & 4);
}

WRITE16_MEMBER(taitogn_state::control2_w)
{
	COMBINE_DATA(&m_control2);
}

READ8_MEMBER(taitogn_state::control3_r)
{
	return m_control3;
}

WRITE8_MEMBER(taitogn_state::control3_w)
{
	COMBINE_DATA(&m_control3);
}

READ16_MEMBER(taitogn_state::gn_1fb70000_r)
{
	// (1328) 1348 tests mask 0002, 8 times.
	// Called by 1434, exit at 143c
	// f -> 4/1
	// end with 4x1 -> ok
	// end with 4x0 -> configid error
	// so returning 2 always works, strange.

	return 2;
}

WRITE16_MEMBER(taitogn_state::gn_1fb70000_w)
{
	// Writes 0 or 1 all the time, it *may* have somthing to do with
	// i/o port width, but then maybe not
}

READ16_MEMBER(taitogn_state::hack1_r)
{
	switch(offset)
	{
	case 0:
		m_v = m_v ^ 8;
		// Probably something to do with sound
		return m_v;
	}

	return 0;
}



// Lifted from zn.c

static const UINT8 tt10[ 8 ] = { 0x80, 0x20, 0x38, 0x08, 0xf1, 0x03, 0xfe, 0xfc };
static const UINT8 tt16[ 8 ] = { 0xc0, 0x04, 0xf9, 0xe1, 0x60, 0x70, 0xf2, 0x02 };

READ8_MEMBER(taitogn_state::znsecsel_r)
{
	return m_n_znsecsel;
}

WRITE8_MEMBER(taitogn_state::znsecsel_w)
{
	COMBINE_DATA( &m_n_znsecsel );

	m_znsec0->select( ( m_n_znsecsel >> 2 ) & 1 );
	m_znsec1->select( ( m_n_znsecsel >> 3 ) & 1 );
	m_zndip->select( ( m_n_znsecsel & 0x8c ) != 0x8c );
}

READ8_MEMBER(taitogn_state::boardconfig_r)
{
	/*
	------00 mem=4M
	------01 mem=4M
	------10 mem=8M
	------11 mem=16M
	-----0-- smem=hM
	-----1-- smem=2M
	----0--- vmem=1M
	----1--- vmem=2M
	000----- rev=-2
	001----- rev=-1
	010----- rev=0
	011----- rev=1
	100----- rev=2
	101----- rev=3
	110----- rev=4
	111----- rev=5
	*/

	return 64|32|8;
}


WRITE8_MEMBER(taitogn_state::coin_w)
{
	/* 0x01=counter
	   0x02=coin lock 1
	   0x08=??
	   0x20=coin lock 2
	   0x80=??
	*/
	m_coin_info = data;
}

READ8_MEMBER(taitogn_state::coin_r)
{
	return m_coin_info;
}

/* mahjong panel handler (for Usagi & Mahjong Oh) */
READ8_MEMBER(taitogn_state::gnet_mahjong_panel_r)
{
	switch(m_coin_info & 0xcc)
	{
		case 0x04: return ioport("KEY0")->read();
		case 0x08: return ioport("KEY1")->read();
		case 0x40: return ioport("KEY2")->read();
		case 0x80: return ioport("KEY3")->read();
	}

	/* mux disabled */
	return ioport("P4")->read();
}

// Init and reset

void taitogn_state::driver_start()
{
	m_znsec0->init(tt10);
	m_znsec1->init(tt16);
}

MACHINE_RESET_MEMBER(taitogn_state,coh3002t)
{
	m_control = 0;

	// halt sound CPU since it has no valid program at start
	m_mn10200->set_input_line(INPUT_LINE_RESET,ASSERT_LINE); /* MCU */
}

static ADDRESS_MAP_START( taitogn_map, AS_PROGRAM, 32, taitogn_state )
	AM_RANGE(0x1f000000, 0x1f7fffff) AM_DEVICE16("flashbank", address_map_bank_device, amap16, 0xffffffff)
	AM_RANGE(0x1fa00000, 0x1fa00003) AM_READ_PORT("P1")
	AM_RANGE(0x1fa00100, 0x1fa00103) AM_READ_PORT("P2")
	AM_RANGE(0x1fa00200, 0x1fa00203) AM_READ_PORT("SERVICE")
	AM_RANGE(0x1fa00300, 0x1fa00303) AM_READ_PORT("SYSTEM")
	AM_RANGE(0x1fa10000, 0x1fa10003) AM_READ_PORT("P3")
	AM_RANGE(0x1fa10100, 0x1fa10103) AM_READ_PORT("P4")
	AM_RANGE(0x1fa10200, 0x1fa10203) AM_READ8(boardconfig_r, 0x000000ff)
	AM_RANGE(0x1fa10300, 0x1fa10303) AM_READWRITE8(znsecsel_r, znsecsel_w, 0x000000ff)
	AM_RANGE(0x1fa20000, 0x1fa20003) AM_READWRITE8(coin_r, coin_w, 0x000000ff)
	AM_RANGE(0x1fa30000, 0x1fa30003) AM_READWRITE8(control3_r, control3_w, 0x000000ff)
	AM_RANGE(0x1fa51c00, 0x1fa51dff) AM_READNOP // systematic read at spu_address + 250000, result dropped, maybe other accesses
	AM_RANGE(0x1fa60000, 0x1fa60003) AM_READ16(hack1_r, 0xffffffff)
	AM_RANGE(0x1faf0000, 0x1faf07ff) AM_DEVREADWRITE8("at28c16", at28c16_device, read, write, 0xffffffff) /* eeprom */
	AM_RANGE(0x1fb00000, 0x1fb0ffff) AM_DEVREADWRITE16("rf5c296", rf5c296_device, io_r, io_w, 0xffffffff)
	AM_RANGE(0x1fb40000, 0x1fb40003) AM_READWRITE8(control_r, control_w, 0x000000ff)
	AM_RANGE(0x1fb60000, 0x1fb60003) AM_WRITE16(control2_w, 0x0000ffff)
	AM_RANGE(0x1fb70000, 0x1fb70003) AM_READWRITE16(gn_1fb70000_r, gn_1fb70000_w, 0x0000ffff)
	AM_RANGE(0x1fbe0000, 0x1fbe01ff) AM_RAM // 256 bytes com zone with the mn102, low bytes of words only, with additional comm at 1fb80000
ADDRESS_MAP_END

static ADDRESS_MAP_START( flashbank_map, AS_PROGRAM, 16, taitogn_state )
	// Bank 0 has access to the subbios, the mn102 flash and the rf5c296 mem zone
	AM_RANGE(0x00000000, 0x001fffff) AM_DEVREADWRITE("biosflash", intelfsh16_device, read, write)
	AM_RANGE(0x00200000, 0x002fffff) AM_DEVREADWRITE("rf5c296", rf5c296_device, mem_r, mem_w )
	AM_RANGE(0x00300000, 0x0037ffff) AM_DEVREADWRITE("pgmflash", intelfsh16_device, read, write)

	// Bank 1 has access to the 3 samples flashes
	AM_RANGE(0x08000000, 0x081fffff) AM_DEVREADWRITE("sndflash0", intelfsh16_device, read, write)
	AM_RANGE(0x08200000, 0x083fffff) AM_DEVREADWRITE("sndflash1", intelfsh16_device, read, write)
	AM_RANGE(0x08400000, 0x085fffff) AM_DEVREADWRITE("sndflash2", intelfsh16_device, read, write)
ADDRESS_MAP_END

static ADDRESS_MAP_START( taitogn_mp_map, AS_PROGRAM, 32, taitogn_state )
	AM_RANGE(0x1fa10100, 0x1fa10103) AM_READ8(gnet_mahjong_panel_r, 0x000000ff)
	AM_IMPORT_FROM(taitogn_map)
ADDRESS_MAP_END

SLOT_INTERFACE_START(slot_ataflash)
	SLOT_INTERFACE("ataflash", ATA_FLASH_PCCARD)
SLOT_INTERFACE_END

static MACHINE_CONFIG_START( coh3002t, taitogn_state )
	/* basic machine hardware */
	MCFG_CPU_ADD( "maincpu", CXD8661R, XTAL_100MHz )
	MCFG_CPU_PROGRAM_MAP(taitogn_map)

	MCFG_RAM_MODIFY("maincpu:ram")
	MCFG_RAM_DEFAULT_SIZE("4M")

	MCFG_DEVICE_ADD("maincpu:sio0:znsec0", ZNSEC, 0)
	MCFG_DEVICE_ADD("maincpu:sio0:znsec1", ZNSEC, 0)
	MCFG_DEVICE_ADD("maincpu:sio0:zndip", ZNDIP, 0)
	MCFG_ZNDIP_DATA_HANDLER(IOPORT(":DSW"))

	/* video hardware */
	MCFG_PSXGPU_ADD( "maincpu", "gpu", CXD8654Q, 0x200000, XTAL_53_693175MHz )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_SPU_ADD( "spu", XTAL_67_7376MHz/2 )
	MCFG_SOUND_ROUTE(0, "lspeaker", 0.35)
	MCFG_SOUND_ROUTE(1, "rspeaker", 0.35)

	MCFG_MACHINE_RESET_OVERRIDE(taitogn_state, coh3002t )

	MCFG_AT28C16_ADD( "at28c16", 0 )
	MCFG_DEVICE_ADD("rf5c296", RF5C296, 0)
	MCFG_RF5C296_SLOT(":pccard")

	MCFG_DEVICE_ADD("pccard", PCCARD_SLOT, 0)
	MCFG_DEVICE_SLOT_INTERFACE(slot_ataflash, "ataflash", false)

	MCFG_MB3773_ADD("mb3773")

	MCFG_INTEL_TE28F160_ADD("biosflash")
	MCFG_INTEL_E28F400_ADD("pgmflash")
	MCFG_INTEL_TE28F160_ADD("sndflash0")
	MCFG_INTEL_TE28F160_ADD("sndflash1")
	MCFG_INTEL_TE28F160_ADD("sndflash2")

	MCFG_DEVICE_ADD("flashbank", ADDRESS_MAP_BANK, 0)
	MCFG_DEVICE_PROGRAM_MAP(flashbank_map)
	MCFG_ADDRESS_MAP_BANK_ENDIANNESS(ENDIANNESS_LITTLE)
	MCFG_ADDRESS_MAP_BANK_DATABUS_WIDTH(16)
	MCFG_ADDRESS_MAP_BANK_STRIDE(0x2000000)

	MCFG_FRAGMENT_ADD( taito_zoom_sound )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( coh3002t_mp, coh3002t )
	MCFG_CPU_MODIFY( "maincpu" )
	MCFG_CPU_PROGRAM_MAP(taitogn_mp_map)
MACHINE_CONFIG_END

static INPUT_PORTS_START( coh3002t )
	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SERVICE")
	PORT_SERVICE_NO_TOGGLE( 0x01, IP_ACTIVE_LOW )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START("P3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("P4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Freeze" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* input port define for the mahjong panel (standard type) */
static INPUT_PORTS_START( coh3002t_mp )
	PORT_INCLUDE( coh3002t )

	PORT_START("KEY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) //rate button
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


//

#define ROM_LOAD16_WORD_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(bios+1)) /* Note '+1' */

#define TAITOGNET_BIOS \
	ROM_REGION32_LE( 0x080000, "maincpu:rom", 0 ) \
	ROM_LOAD( "coh-3002t.353", 0x000000, 0x080000, CRC(03967fa7) SHA1(0e17fec2286e4e25deb23d40e41ce0986f373d49) ) \
	ROM_REGION( 0x200000, "biosflash", 0 ) \
	ROM_SYSTEM_BIOS( 0, "v1",   "G-NET Bios v1" ) \
		ROM_LOAD16_WORD_BIOS(0, "flash.u30", 0x000000, 0x200000, CRC(c48c8236) SHA1(c6dad60266ce2ff635696bc0d91903c543273559) ) \
	ROM_SYSTEM_BIOS( 1, "v2",   "G-NET Bios v2" ) \
		ROM_LOAD16_WORD_BIOS(1, "flashv2.u30", 0x000000, 0x200000, CRC(CAE462D3) SHA1(f1b10846a8423d9fe021191c5876190857c3d2a4) ) \
	ROM_REGION32_LE( 0x80000,  "mn10200", 0) \
	ROM_FILL( 0, 0x80000, 0xff) \
	ROM_REGION32_LE( 0x600000, "zsg1", 0) \
	ROM_FILL( 0, 0x600000, 0xff)

ROM_START( taitogn )
	TAITOGNET_BIOS
ROM_END

/* Taito */

ROM_START(raycris)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "raycris", 0, SHA1(015cb0e6c4421cc38809de28c4793b4491386aee))
ROM_END


ROM_START(gobyrc)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "gobyrc", 0, SHA1(0bee1f495fc8b033fd56aad9260ae94abb35eb58))
ROM_END

ROM_START(rcdego)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "rcdego", 0, SHA1(9e177f2a3954cfea0c8c5a288e116324d10f5dd1))
ROM_END

ROM_START(chaoshea)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "chaosheat", 0, SHA1(c13b7d7025eee05f1f696d108801c7bafb3f1356))
ROM_END

ROM_START(chaosheaj)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "chaosheatj", 0, SHA1(2f211ac08675ea8ec33c7659a13951db94eaa627))
ROM_END


ROM_START(flipmaze)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "flipmaze", 0, SHA1(423b6c06f4f2d9a608ce20b61a3ac11687d22c40) )
ROM_END


ROM_START(spuzbobl)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "spuzbobl", 0, SHA1(1b1c72fb7e5656021485fefaef8f2ba48e2b4ea8))
ROM_END

ROM_START(spuzboblj)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "spuzbobj", 0, SHA1(dac433cf88543d2499bf797d7406b82ae4338726))
ROM_END

ROM_START(soutenry)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "soutenry", 0, SHA1(9204d0be833d29f37b8cd3fbdf09da69b622254b))
ROM_END

ROM_START(shanghss)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "shanghss", 0, SHA1(7964f71ec5c81d2120d83b63a82f97fbad5a8e6d))
ROM_END

ROM_START(sianniv)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "sianniv", 0, SHA1(1e08b813190a9e1baf29bc16884172d6c8da7ae3))
ROM_END

ROM_START(kollon)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "kollon", 0, SHA1(d8ea5b5b0ee99004b16ef89883e23de6c7ddd7ce))
ROM_END

ROM_START(kollonc)
	TAITOGNET_BIOS
	ROM_DEFAULT_BIOS( "v2" )

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "kollonc", 0, SHA1(ce62181659701cfb8f7c564870ab902be4d8e060)) /* Original Taito Compact Flash version */
ROM_END

ROM_START(shikigam)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "shikigam", 0, SHA1(fa49a0bc47f5cb7c30d7e49e2c3696b21bafb840))
ROM_END


/* Success */

ROM_START(otenamih)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "otenamih", 0, SHA1(b3babe3a1876c43745616ee1e7d87276ce7dad0b) )
ROM_END


ROM_START(psyvaria)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "psyvaria", 0,  SHA1(b981a42a10069322b77f7a268beae1d409b4156d))
ROM_END

ROM_START(psyvarrv)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "psyvarrv", 0, SHA1(277c4f52502bcd7acc1889840962ec80d56465f3))
ROM_END

ROM_START(zooo)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "zooo", 0, SHA1(e275b3141b2bc49142990e6b497a5394a314a30b))
ROM_END

ROM_START(zokuoten)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "zokuoten", 0, SHA1(5ce13db00518f96af64935176c71ec68d2a51938))
ROM_END

ROM_START(otenamhf)
	TAITOGNET_BIOS
	ROM_DEFAULT_BIOS( "v2" )

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "otenamhf", 0, SHA1(5b15c33bf401e5546d78e905f538513d6ffcf562)) /* Original Taito Compact Flash version */
ROM_END




/* Takumi */

ROM_START(nightrai)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "nightrai", 0, SHA1(74d0458f851cbcf10453c5cc4c47bb4388244cdf))
ROM_END

ROM_START(otenki)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "otenki", 0, SHA1(7e745ca4c4570215f452fd09cdd56a42c39caeba))
ROM_END

/* Warashi */

ROM_START(usagi)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "usagi", 0, SHA1(edf9dd271957f6cb06feed238ae21100514bef8e))
ROM_END

ROM_START(mahjngoh)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "mahjngoh", 0, SHA1(3ef1110d15582d7c0187438d7ad61765dd121cff))
ROM_END

ROM_START(shangtou)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "shanghaito", 0, SHA1(9901db5a9aae77e3af4157aa2c601eaab5b7ca85) )
ROM_END


/* Triangle Service */

ROM_START(xiistag)
	TAITOGNET_BIOS

	DISK_REGION( "pccard:ataflash:drive_0" )
	DISK_IMAGE( "xiistag", 0, SHA1(586e37c8d926293b2bd928e5f0d693910cfb05a2))
ROM_END


/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the coh-3002t.353 file, so that we do not have to include */
/* it in every zip file */
GAME( 1997, taitogn,  0,        coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Taito GNET", GAME_IS_BIOS_ROOT )

GAME( 1998, chaoshea, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Chaos Heat (V2.09O)", GAME_IMPERFECT_SOUND )
GAME( 1998, chaosheaj,chaoshea, coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Chaos Heat (V2.08J)", GAME_IMPERFECT_SOUND )
GAME( 1998, raycris,  taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Ray Crisis (V2.03J)", GAME_IMPERFECT_SOUND )
GAME( 1999, spuzbobl, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Super Puzzle Bobble (V2.05O)", GAME_IMPERFECT_SOUND )
GAME( 1999, spuzboblj,spuzbobl, coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Super Puzzle Bobble (V2.04J)", GAME_IMPERFECT_SOUND )
GAME( 1999, gobyrc,   taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Go By RC (V2.03O)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND ) // custom inputs need calibrating
GAME( 1999, rcdego,   gobyrc,   coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "RC De Go (V2.03J)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND ) // custom inputs need calibrating
GAME( 1999, flipmaze, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito / Moss", "Flip Maze (V2.04J)", GAME_IMPERFECT_SOUND )
GAME( 2001, shikigam, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT270, "Alfa System / Taito", "Shikigami no Shiro (V2.03J)", GAME_IMPERFECT_SOUND )
GAME( 2003, sianniv,  taitogn,  coh3002t, coh3002t, driver_device, 0, ROT270, "Taito", "Space Invaders Anniversary (V2.02J)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND ) // IRQ at the wrong time
GAME( 2003, kollon,   taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Kollon (V2.04J)", GAME_IMPERFECT_SOUND )
GAME( 2003, kollonc,  kollon,   coh3002t, coh3002t, driver_device, 0, ROT0,   "Taito", "Kollon (V2.04JC)", GAME_IMPERFECT_SOUND )

GAME( 1999, otenamih, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Success", "Otenami Haiken (V2.04J)", GAME_IMPERFECT_SOUND )
GAME( 2005, otenamhf, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Success / Warashi", "Otenami Haiken Final (V2.07JC)", GAME_IMPERFECT_SOUND )
GAME( 2000, psyvaria, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT270, "Success", "Psyvariar -Medium Unit- (V2.04J)", GAME_IMPERFECT_SOUND )
GAME( 2000, psyvarrv, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT270, "Success", "Psyvariar -Revision- (V2.04J)", GAME_IMPERFECT_SOUND )
GAME( 2000, zokuoten, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Success", "Zoku Otenamihaiken (V2.03J)", GAME_IMPERFECT_SOUND )
GAME( 2004, zooo,     taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Success", "Zooo (V2.01J)", GAME_IMPERFECT_SOUND )

GAME( 1999, mahjngoh, taitogn,  coh3002t_mp, coh3002t_mp, driver_device, 0, ROT0, "Warashi / Mahjong Kobo / Taito", "Mahjong Oh (V2.06J)", GAME_IMPERFECT_SOUND )
GAME( 2001, usagi,    taitogn,  coh3002t_mp, coh3002t_mp, driver_device, 0, ROT0, "Warashi / Mahjong Kobo / Taito", "Usagi (V2.02J)", GAME_IMPERFECT_SOUND )
GAME( 2000, soutenry, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Warashi", "Soutenryu (V2.07J)", GAME_IMPERFECT_SOUND )
GAME( 2000, shanghss, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Warashi", "Shanghai Shoryu Sairin (V2.03J)", GAME_IMPERFECT_SOUND )
GAME( 2002, shangtou, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Warashi / Sunsoft / Taito", "Shanghai Sangokuhai Tougi (Ver 2.01J)", GAME_IMPERFECT_SOUND )

GAME( 2001, nightrai, taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Takumi", "Night Raid (V2.03J)", GAME_IMPERFECT_SOUND )
GAME( 2001, otenki,   taitogn,  coh3002t, coh3002t, driver_device, 0, ROT0,   "Takumi", "Otenki Kororin (V2.01J)", GAME_IMPERFECT_SOUND )

GAME( 2002, xiistag,  taitogn,  coh3002t, coh3002t, driver_device, 0, ROT270, "Triangle Service", "XII Stag (V2.01J)", GAME_IMPERFECT_SOUND )
