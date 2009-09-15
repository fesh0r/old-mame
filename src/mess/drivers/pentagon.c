#include "driver.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"
#include "formats/tzx_cas.h"
#include "machine/wd17xx.h"
#include "machine/beta.h"


static int ROMSelection;
static const device_config* beta;

static DIRECT_UPDATE_HANDLER( pentagon_direct )
{	
	UINT16 pc = cpu_get_reg(cputag_get_cpu(space->machine, "maincpu"), REG_GENPCBASE);
	
	if (beta->started && betadisk_is_active(beta)) 
	{
		if (pc >= 0x4000) 
		{
			ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) ? 1 : 0;
			betadisk_disable(beta);
			memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
			memory_set_bankptr(space->machine, 1, memory_region(space->machine, "maincpu") + 0x010000 + (ROMSelection<<14));
		} 	
	} else if (((pc & 0xff00) == 0x3d00) && (ROMSelection==1))
	{
		ROMSelection = 3;
		if (beta->started) 
			betadisk_enable(beta);
		
	} 
	if((address>=0x0000) && (address<=0x3fff)) 
	{	
		memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
		if (ROMSelection == 3) {			
			if (beta->started) 
				direct->raw = direct->decrypted =  memory_region(space->machine, "beta:beta");
		} else {
			direct->raw = direct->decrypted =  memory_region(space->machine, "maincpu") + 0x010000 + (ROMSelection<<14);
		}
		memory_set_bankptr(space->machine, 1, direct->raw);
		return ~0;
	}
	return address;
}

static void pentagon_update_memory(running_machine *machine)
{	
	spectrum_screen_location = mess_ram + ((spectrum_128_port_7ffd_data & 8) ? (7<<14) : (5<<14));

	memory_set_bankptr(machine, 4, mess_ram + ((spectrum_128_port_7ffd_data & 0x07) * 0x4000));

	if (beta->started && betadisk_is_active(beta) && !( spectrum_128_port_7ffd_data & 0x10 ) ) 
	{    
		/* GLUK */
		if (strcmp(machine->gamedrv->name, "pent1024")==0) {
			ROMSelection = 2;
		} else {
			ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) ;
		}
	}
	else {	
		/* ROM switching */
		ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) ;
	}
	/* rom 0 is 128K rom, rom 1 is 48 BASIC */
	memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + 0x010000 + (ROMSelection<<14));
}

static WRITE8_HANDLER(pentagon_port_7ffd_w)
{
	/* disable paging */
	if (spectrum_128_port_7ffd_data & 0x20)
		return;

	/* store new state */
	spectrum_128_port_7ffd_data = data;

	/* update memory */
	pentagon_update_memory(space->machine);
}

static ADDRESS_MAP_START (pentagon_io, ADDRESS_SPACE_IO, 8)	
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x001f, 0x001f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_status_r,betadisk_command_w) AM_MIRROR(0xff00)
	AM_RANGE(0x003f, 0x003f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_track_r,betadisk_track_w) AM_MIRROR(0xff00)
	AM_RANGE(0x005f, 0x005f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_sector_r,betadisk_sector_w) AM_MIRROR(0xff00)
	AM_RANGE(0x007f, 0x007f) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_data_r,betadisk_data_w) AM_MIRROR(0xff00)
	AM_RANGE(0x00fe, 0x00fe) AM_READWRITE(spectrum_port_fe_r,spectrum_port_fe_w) AM_MIRROR(0xff00) AM_MASK(0xffff) 
	AM_RANGE(0x00ff, 0x00ff) AM_DEVREADWRITE(BETA_DISK_TAG, betadisk_state_r, betadisk_param_w) AM_MIRROR(0xff00)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(pentagon_port_7ffd_w)  AM_MIRROR(0x3ffd)
	AM_RANGE(0x8000, 0x8000) AM_DEVWRITE("ay8912", ay8910_data_w) AM_MIRROR(0x3ffd)
	AM_RANGE(0xc000, 0xc000) AM_DEVREADWRITE("ay8912", ay8910_r, ay8910_address_w) AM_MIRROR(0x3ffd)
ADDRESS_MAP_END

static MACHINE_RESET( pentagon )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	beta = devtag_get_device(machine, BETA_DISK_TAG);
	
	memory_install_read8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_BANK(1));
	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);

	if (beta->started)  {
		betadisk_enable(beta);
		betadisk_clear_status(beta);
	}

	memory_set_direct_update_handler( space, pentagon_direct ); 
	
	memset(mess_ram,0,128*1024);
	
	/* Bank 5 is always in 0x4000 - 0x7fff */
	memory_set_bankptr(machine, 2, mess_ram + (5<<14));

	/* Bank 2 is always in 0x8000 - 0xbfff */
	memory_set_bankptr(machine, 3, mess_ram + (2<<14));

	spectrum_128_port_7ffd_data = 0;

	pentagon_update_memory(machine);	
}

static MACHINE_DRIVER_START( pentagon )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_IO_MAP(pentagon_io)
	MDRV_MACHINE_RESET( pentagon )
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(pentagon)
	ROM_REGION(0x01c000, "maincpu", ROMREGION_ERASEFF)
	ROM_SYSTEM_BIOS(0, "v1", "Pentagon 128K")
	ROMX_LOAD("128p-0.rom", 0x010000, 0x4000, CRC(124ad9e0) SHA1(d07fcdeca892ee80494d286ea9ea5bf3928a1aca), ROM_BIOS(1))
	ROMX_LOAD("128p-1.rom", 0x014000, 0x4000, CRC(b96a36be) SHA1(80080644289ed93d71a1103992a154cc9802b2fa), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v2", "Pentagon 128K 93")
	ROMX_LOAD("128tr93.rom",0x010000, 0x4000, CRC(08ad241c) SHA1(16daba547c644ef01ce76d2686ccfbff72e13dbe), ROM_BIOS(2))
	ROMX_LOAD("128p-1.rom", 0x014000, 0x4000, CRC(b96a36be) SHA1(80080644289ed93d71a1103992a154cc9802b2fa), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v3", "Pentagon 128K (joined)")	
	ROMX_LOAD( "pentagon.rom", 0x010000, 0x8000, CRC(aa1ce4bd) SHA1(a584272f21dc82c14b7d4f1ed440e23a976e71f0), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "v4", "Pentagon 128K Spanish")	
	ROMX_LOAD( "pent-es.rom", 0x010000, 0x8000, CRC(34d04bae) SHA1(6782c8c0ee77c40d6d3170a254894dae44ddc93e), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "v5", "Pentagon 128K SOS89R Monitor")	
	ROMX_LOAD("128p-0.rom", 0x010000, 0x4000, CRC(124ad9e0) SHA1(d07fcdeca892ee80494d286ea9ea5bf3928a1aca), ROM_BIOS(5))	
	ROMX_LOAD( "sos89r.rom",0x014000, 0x4000, CRC(09c9e7e1) SHA1(29c567921abd377d2f9c088352c392a5a0858651), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "v6", "Pentagon 128K 1990 Monitor")	
	ROMX_LOAD("128p-0.rom",  0x010000, 0x4000, CRC(124ad9e0) SHA1(d07fcdeca892ee80494d286ea9ea5bf3928a1aca), ROM_BIOS(6))
	ROMX_LOAD( "basic90.rom",0x014000, 0x4000, CRC(a41575ba) SHA1(44c5de86e765172b0af154fe3934643ce40bf378), ROM_BIOS(6))
	ROM_SYSTEM_BIOS(6, "v7", "Pentagon 128K RaK(c) 1991 Monitor")	
	ROMX_LOAD("128p-0.rom", 0x010000, 0x4000, CRC(124ad9e0) SHA1(d07fcdeca892ee80494d286ea9ea5bf3928a1aca), ROM_BIOS(7))
	ROMX_LOAD( "sos48.rom", 0x014000, 0x4000, CRC(ceb4005d) SHA1(d56c01ea7abdca178efb2b1c6b2866a9a38274ee), ROM_BIOS(7))
	ROM_SYSTEM_BIOS(7, "v8", "Pentagon 128K Dynaelectronics 1989")	
	ROMX_LOAD("128p-0.rom", 0x010000, 0x4000, CRC(124ad9e0) SHA1(d07fcdeca892ee80494d286ea9ea5bf3928a1aca), ROM_BIOS(8))
	ROMX_LOAD( "m48a.rom",  0x014000, 0x4000, CRC(a3b4def6) SHA1(7ad59ca373876d452b0cf0ed5edb0e93c3176f1a), ROM_BIOS(8))
	ROM_SYSTEM_BIOS(8, "v9", "ZXVGS v0.22 by Yarek")	
    ROMX_LOAD( "zxvgs-22-0.rom", 0x010000, 0x4000, CRC(63041c61) SHA1(f6718097d939afa8881b4436741a5a23d7e93d78), ROM_BIOS(9))
    ROMX_LOAD( "zxvgs-22-1.rom", 0x014000, 0x4000, CRC(f3736047) SHA1(f3739bf460a57e3f10e8dfb1e7120842938d27ea), ROM_BIOS(9))    
	ROM_SYSTEM_BIOS(9, "v10", "ZXVGS v0.29 by Yarek")	
  	ROMX_LOAD( "zxvg-29-0.rom", 0x010000, 0x4000, CRC(3b66f433) SHA1(d21df9e7f1ee99d8b38c2e6a32727aac0f1d5dc6), ROM_BIOS(10))
  	ROMX_LOAD( "zxvg-1.rom", 0x014000, 0x4000, CRC(a8baca3e) SHA1(f2f131eaa4de832eda76290e48f86e465d28ded7), ROM_BIOS(10))
	ROM_SYSTEM_BIOS(10, "v11", "ZXVGS v0.30 by Yarek")	
  	ROMX_LOAD( "zxvg-30-0.rom", 0x010000, 0x4000, CRC(533e0f26) SHA1(b5f157c5d0da414ec77e445fdc40b78450129709), ROM_BIOS(11))
  	ROMX_LOAD( "zxvg-1.rom", 0x014000, 0x4000, CRC(a8baca3e) SHA1(f2f131eaa4de832eda76290e48f86e465d28ded7), ROM_BIOS(11))
	ROM_SYSTEM_BIOS(11, "v12", "ZXVGS v0.31 by Yarek")	
  	ROMX_LOAD( "zxvg-31-0.rom", 0x010000, 0x4000, CRC(76f43500) SHA1(1c7cd52894847668418876d55b93b213d89d92ee), ROM_BIOS(12))
  	ROMX_LOAD( "zxvg-1.rom", 0x014000, 0x4000, CRC(a8baca3e) SHA1(f2f131eaa4de832eda76290e48f86e465d28ded7), ROM_BIOS(12))
	ROM_SYSTEM_BIOS(12, "v13", "ZXVGS v0.35 by Yarek")	
  	ROMX_LOAD( "zxvg-35-0.rom", 0x010000, 0x4000, CRC(5cc8b3b1) SHA1(6c6d0ef1b65d7dc4f607d17204488264575ce48c), ROM_BIOS(13))
  	ROMX_LOAD( "zxvg-1.rom", 0x014000, 0x4000, CRC(a8baca3e) SHA1(f2f131eaa4de832eda76290e48f86e465d28ded7), ROM_BIOS(13))    
	ROM_SYSTEM_BIOS(13, "v14", "NeOS 512")	  	
  	ROMX_LOAD("neos_512.rom", 0x010000, 0x4000, CRC(1657fa43) SHA1(647545f06257bce9b1919fcb86b2a49a21c851a7), ROM_BIOS(14))
	ROMX_LOAD("128p-1.rom",   0x014000, 0x4000, CRC(b96a36be) SHA1(80080644289ed93d71a1103992a154cc9802b2fa), ROM_BIOS(14))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(pent1024)
	ROM_REGION(0x01c000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD("128p-0.rom", 0x010000, 0x4000, CRC(124ad9e0) SHA1(d07fcdeca892ee80494d286ea9ea5bf3928a1aca))
	ROM_LOAD("128p-1.rom", 0x014000, 0x4000, CRC(b96a36be) SHA1(80080644289ed93d71a1103992a154cc9802b2fa))	
	ROM_SYSTEM_BIOS(0, "v1", "Gluk 6.3r")
	ROMX_LOAD( "gluk63r.rom",0x018000, 0x4000, CRC(ca321d79) SHA1(015eb96dafb273d4f4512c467e9b43c305fd1bc4), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v2", "Gluk 5.2i")
	ROMX_LOAD( "gluk52i.rom", 0x018000, 0x4000, CRC(fe44b86a) SHA1(9099d8a0f99a818849ca67ae1a8d3e7eacf06e65), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "v3", "Gluk 5.3")
	ROMX_LOAD( "gluk53.rom",  0x018000, 0x4000, CRC(479515ef) SHA1(ed656cd4faa36de2e31b38102bcbd8cee12e7976), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "v4", "Gluk 5.4")
	ROMX_LOAD( "gluk54r.rom", 0x018000, 0x4000, CRC(f4c1e975) SHA1(7e9e116750e1398572695b9cf8a120e47066256e), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "v5", "Gluk 5.5r")
	ROMX_LOAD( "gluk55r.rom", 0x018000, 0x4000, CRC(3658c1ee) SHA1(4a5c8ca1e090cfb0168796f0d695310fa5c955d3), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "v6", "Gluk 5.5rr")
	ROMX_LOAD( "gluk55rr.rom",0x018000, 0x4000, CRC(6b60b818) SHA1(9d606275d17770c9341b33b43f40aee227078827), ROM_BIOS(6))
	ROM_SYSTEM_BIOS(6, "v7", "Gluk 6.0r")
	ROMX_LOAD( "gluk60r.rom", 0x018000, 0x4000, CRC(d114a032) SHA1(5db3462ce7a51b473a3a7056e67c11a62cc1cc2a), ROM_BIOS(7))  	
	ROM_SYSTEM_BIOS(7, "v8", "Gluk 6.0-1r")
	ROMX_LOAD( "gluk601r.rom", 0x018000, 0x4000, CRC(daf6310b) SHA1(b8945168d4d136b731b33ec4758f8510c47fb8c4), ROM_BIOS(8))  	
	ROM_SYSTEM_BIOS(8, "v9", "Gluk 5.1")
	ROMX_LOAD( "gluk51.rom",   0x018000, 0x4000, CRC(ea8c760b) SHA1(adaab28066ca46fbcdcf084c3b53d5a1b82d94a9), ROM_BIOS(9)) 
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END
static SYSTEM_CONFIG_START(pentagon)
	CONFIG_RAM_DEFAULT(128 * 1024)
	CONFIG_DEVICE(beta_floppy_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(pent1024)
	CONFIG_RAM_DEFAULT(1024 * 1024)
	CONFIG_DEVICE(beta_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP( ????, pentagon, spec128,	0,		pentagon,	spec_plus,	0,		pentagon,	"???",		"Pentagon", GAME_NOT_WORKING)
COMP( ????, pent1024, spec128,	0,		pentagon,	spec_plus,	0,		pent1024,	"???",		"Pentagon 1024", GAME_NOT_WORKING)
