/***************************************************************************

  genesis.c

  Machine file to handle emulation of the Sega Mega Drive & Genesis.


	2008-09: Moved here cart code and custom mapper handlers. Hopefully,
		it will make painless the future merging with HazeMD
	2008-10: Fixed SRAM, matching as much as possible HazeMD. Some game is 
		not detected, however. E.g. Sonic 3. Also some games seem false
		positives (e.g. Wonderboy 5: according to its headers it should
		have 1byte of SRAM). Better detection routines would be welcome.

***************************************************************************/


#include "driver.h"
#include "cpu/m68000/m68000.h"

#include "devices/cartslot.h"

static UINT16 squirrel_king_extra = 0;
static UINT16 lion2_prot1_data, lion2_prot2_data;
static UINT16 realtek_bank_addr=0, realtek_bank_size=0, realtek_old_bank_addr;
static UINT16 g_l3alt_pdat;
static UINT16 g_l3alt_pcmd;

static int genesis_last_loaded_image_length = -1;


#define MAX_MD_CART_SIZE 0x500000

/* where a fresh copy of rom is stashed for reset and banking setup */
#define VIRGIN_COPY_GEN 0xd00000

static UINT16 *genesis_sram, *megadriv_backupram;
static int genesis_sram_start = 0;
static int genesis_sram_end = 0;
static int genesis_sram_active = 0;
static int genesis_sram_readonly = 0;
static int has_sram = 0;


enum 
{
	STANDARD = 0,
	SSF2,					/* Super Street Fighter 2 */
	LIONK3,					/* Lion King 3 */
	SKINGKONG,				/* Super King Kong 99 */
	SDK99,					/* Super Donkey Kong 99 */
	REDCLIFF,				/* Romance of the Three Kingdoms - Battle of Red Cliffs, already decoded from .mdx format */
	REDCL_EN,				/* The encoded version... */
	RADICA,					/* Radica TV games.. these probably should be a seperate driver since they are a seperate 'console' */
	KOF99,					/* King of Fighters '99 */
	SOULBLAD,				/* Soul Blade */
	MJLOVER,				/* Mahjong Lover */
	SQUIRRELK,				/* Squirrel King */
	SMOUSE,					/* Smart Mouse */
	SMB,					/* Super Mario Bros. */
	SMB2,					/* Super Mario Bros. 2 */
	KAIJU,
	CHINFIGHT3,				/* Chinese Fighters 3 */
	LIONK2,					/* Lion King 2 */
	BUGSLIFE,				/* A Bug's Life */
	ELFWOR,					/* Elf Wor */
	ROCKMANX3,				/* Rockman X3 */
	SBUBBOB,				/* Super Bubble Bobble */
	KOF98,					/* King of Fighters '98 */
	REALTEC,				/* Whac a Critter/Mallet legend, Defend the Earth, Funnyworld/Ballonboy */
	SUP19IN1,				/* Super 19 in 1 */
	SUP15IN1,				/* Super 15 in 1 */
};


static int cart_type = STANDARD;


/*************************************
 *
 *  Handlers for custom mappers
 *
 *************************************/

static WRITE16_HANDLER( genesis_ssf2_bank_w )
{
	static int lastoffset = -1,lastdata = -1;
	UINT8 *ROM = memory_region(machine, "main");

	if ((lastoffset != offset) || (lastdata != data)) {
		lastoffset = offset; lastdata = data;
		switch (offset<<1)
		{
			case 0x00: /* write protect register // this is not a write protect, but seems to do nothing useful but reset bank0 after the checksum test (red screen otherwise) */
				if (data == 2) {
					memcpy(ROM + 0x000000, ROM + 0x400000+(((data&0xf)-2)*0x080000), 0x080000);
				}
				break;
			case 0x02: /* 0x080000 - 0x0FFFFF */
				memcpy(ROM + 0x080000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
				break;
			case 0x04: /* 0x100000 - 0x17FFFF */
				memcpy(ROM + 0x100000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
				break;
			case 0x06: /* 0x180000 - 0x1FFFFF */
				memcpy(ROM + 0x180000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
				break;
			case 0x08: /* 0x200000 - 0x27FFFF */
				memcpy(ROM + 0x200000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
				break;
			case 0x0a: /* 0x280000 - 0x2FFFFF */
				memcpy(ROM + 0x280000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
				break;
			case 0x0c: /* 0x300000 - 0x37FFFF */
				memcpy(ROM + 0x300000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
				break;
			case 0x0e: /* 0x380000 - 0x3FFFFF */
				memcpy(ROM + 0x380000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
				break;
		}
	}
}

/*static WRITE16_HANDLER( g_l3alt_pdat_w )
{
	g_l3alt_pdat = data;
}

static WRITE16_HANDLER( g_l3alt_pcmd_w )
{
	g_l3alt_pcmd = data;
}
*/

static READ16_HANDLER( g_l3alt_prot_r )
{
	int retdata = 0;

	offset &=0x7;

	switch (offset)
	{

		case 2:

			switch (g_l3alt_pcmd)
			{
				case 1:
					retdata = g_l3alt_pdat>>1;
					break;

				case 2:
					retdata = g_l3alt_pdat>>4;
					retdata |= (g_l3alt_pdat&0x0f)<<4;
					break;

				default:
					/* printf("unk prot case %d\n",g_l3alt_pcmd); */
					retdata = (((g_l3alt_pdat>>7)&1)<<0);
					retdata |=(((g_l3alt_pdat>>6)&1)<<1);
					retdata |=(((g_l3alt_pdat>>5)&1)<<2);
					retdata |=(((g_l3alt_pdat>>4)&1)<<3);
					retdata |=(((g_l3alt_pdat>>3)&1)<<4);
					retdata |=(((g_l3alt_pdat>>2)&1)<<5);
					retdata |=(((g_l3alt_pdat>>1)&1)<<6);
					retdata |=(((g_l3alt_pdat>>0)&1)<<7);
					break;
			}
			break;

		default:

			printf("protection read, unknown offset\n");
			break;
	}

/*	printf("%06x: g_l3alt_pdat_w %04x g_l3alt_pcmd_w %04x return %04x\n",activecpu_get_pc(), g_l3alt_pdat,g_l3alt_pcmd,retdata); */

	return retdata;

}

static WRITE16_HANDLER( g_l3alt_protw )
{
	offset &=0x7;

	switch (offset)
	{
		case 0x0:
			g_l3alt_pdat = data;
			break;
		case 0x1:
			g_l3alt_pcmd = data;
			break;
		default:
			printf("protection write, unknown offst\n");
			break;
	}
}

static WRITE16_HANDLER( g_l3alt_bank_w )
{
	offset &=0x7;

	switch (offset)
	{
		case 0:
		{
		UINT8 *ROM = memory_region(machine, "main");
		/* printf("%06x data %04x\n",activecpu_get_pc(), data); */
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN + (data&0xffff)*0x8000], 0x8000);
		}
		break;

		default:
		printf("unk bank w\n");
		break;

	}

}


static WRITE16_HANDLER( realtec_402000_w )
{
	realtek_bank_addr = 0;
	realtek_bank_size = (data>>8)&0x1f;
}

static WRITE16_HANDLER( realtec_400000_w )
{
	int bankdata = (data >> 9) & 0x7;

	UINT8 *ROM = memory_region(machine, "main");

	realtek_old_bank_addr = realtek_bank_addr;
	realtek_bank_addr = (realtek_bank_addr & 0x7) | bankdata<<3;

	memcpy(ROM, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
	memcpy(ROM+ realtek_bank_size*0x20000, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
}

static WRITE16_HANDLER( realtec_404000_w )
{
	int bankdata = (data >> 8) & 0x3;
	UINT8 *ROM = memory_region(machine, "main");

	realtek_old_bank_addr = realtek_bank_addr;
	realtek_bank_addr = (realtek_bank_addr & 0xf8)|bankdata;

	if (realtek_old_bank_addr != realtek_bank_addr)
	{
		memcpy(ROM, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
		memcpy(ROM+ realtek_bank_size*0x20000, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
	}
}

static WRITE16_HANDLER( g_chifi3_bank_w )
{
	UINT8 *ROM = memory_region(machine, "main");

	if (data==0xf100) // *hit player
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x410000, 0x10000);
		}
	}
	else if (data==0xd700) // title screen..
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x470000, 0x10000);
		}
	}
	else if (data==0xd300) // character hits floor
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x430000, 0x10000);
		}
	}
	else if (data==0x0000)
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x400000+x, 0x10000);
		}
	}
	else
	{
		logerror("%06x chifi3, bankw? %04x %04x\n",activecpu_get_pc(), offset,data);
	}

}

static READ16_HANDLER( g_chifi3_prot_r )
{
	UINT32 retdat;

	/* not 100% correct, there may be some relationship between the reads here
	and the writes made at the start of the game.. */

	/*
	04dc10 chifi3, prot_r? 2800
	04cefa chifi3, prot_r? 65262
	*/

	if (activecpu_get_pc()==0x01782) // makes 'VS' screen appear
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x1c24) // background gfx etc.
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x10c4a) // unknown
	{
		return mame_rand(machine);
	}
	else if (activecpu_get_pc()==0x10c50) // unknown
	{
		return mame_rand(machine);
	}
	else if (activecpu_get_pc()==0x10c52) // relates to the game speed..
	{
		retdat = activecpu_get_reg(M68K_D4)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x061ae)
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x061b0)
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else
	{
		logerror("%06x chifi3, prot_r? %04x\n",activecpu_get_pc(), offset);
	}

	return 0;
}

static WRITE16_HANDLER( s19in1_bank )
{
	UINT8 *ROM = memory_region(machine, "main");
	memcpy(ROM + 0x000000, ROM + 0x400000+((offset << 1)*0x10000), 0x80000);
}

// Kaiju? (Pokemon Stadium) handler from HazeMD
static WRITE16_HANDLER( g_kaiju_bank_w )
{
	UINT8 *ROM = memory_region(machine, "main");
	memcpy(ROM + 0x000000, ROM + 0x400000+(data&0x7f)*0x8000, 0x8000);
}

// Soulblade handler from HazeMD
static READ16_HANDLER( soulb_0x400006_r )
{
	return 0xf000;
}

static READ16_HANDLER( soulb_0x400002_r )
{
	return 0x9800;
}

static READ16_HANDLER( soulb_0x400004_r )
{
	return 0xc900;
}

// Mahjong Lover handler from HazeMD
static READ16_HANDLER( mjlovr_prot_1_r )
{
	return 0x9000;
}

static READ16_HANDLER( mjlovr_prot_2_r )
{
	return 0xd300;
}

// Super Mario Bros handler from HazeMD
static READ16_HANDLER( smbro_prot_r )
{
	return 0xc;
}


// Smart Mouse handler from HazeMD
static READ16_HANDLER( smous_prot_r )
{
	switch (offset)
	{
		case 0: return 0x5500;
		case 1: return 0x0f00;
		case 2: return 0xaa00;
		case 3: return 0xf000;
	}

	return -1;
}

// Super Bubble Bobble MD handler from HazeMD
static READ16_HANDLER( sbub_extra1_r )
{
	return 0x5500;
}

static READ16_HANDLER( sbub_extra2_r )
{
	return 0x0f00;
}

// KOF99 handler from HazeMD
static READ16_HANDLER( kof99_0xA13002_r )
{
	// write 02 to a13002.. shift right 1?
	return 0x01;
}

static READ16_HANDLER( kof99_00A1303E_r )
{
	// write 3e to a1303e.. shift right 1?
	return 0x1f;
}

static READ16_HANDLER( kof99_0xA13000_r )
{
	// no write, startup check, chinese message if != 0
	return 0x0;
}

// Radica handler from HazeMD

static READ16_HANDLER( radica_bank_select )
{
	int bank = offset&0x3f;
	UINT8 *ROM = memory_region(machine, "main");
	memcpy(ROM, ROM +  (bank*0x10000)+0x400000, 0x400000);
	return 0;
}

// ROTK Red Cliff handler from HazeMD

static READ16_HANDLER( redclif_prot_r )
{
	return -0x56 << 8;
}

static READ16_HANDLER( redclif_prot2_r )
{
	return 0x55 << 8;
}

// Squirrel King handler from HazeMD, this does not give screen garbage like HazeMD compile If you reset it twice
static READ16_HANDLER( squirrel_king_extra_r )
{
	return squirrel_king_extra;

}
static WRITE16_HANDLER( squirrel_king_extra_w )
{
	squirrel_king_extra = data;
}

// Lion King 2 handler from HazeMD
static READ16_HANDLER( lion2_prot1_r )
{
	return lion2_prot1_data;
}

static READ16_HANDLER( lion2_prot2_r )
{
	return lion2_prot2_data;
}

static WRITE16_HANDLER ( lion2_prot1_w )
{
	lion2_prot1_data = data;
}

static WRITE16_HANDLER ( lion2_prot2_w )
{
	lion2_prot2_data = data;
}

// Rockman X3 handler from HazeMD
static READ16_HANDLER( rx3_extra_r )
{
	return 0xc;
}

// King of Fighters 98 handler from HazeMD
static READ16_HANDLER( g_kof98_aa_r )
{
	return 0xaa00;
}

static READ16_HANDLER( g_kof98_0a_r )
{
	return 0x0a00;
}

static READ16_HANDLER( g_kof98_00_r )
{
	return 0x0000;
}

// Super Mario Bros 2 handler from HazeMD
static READ16_HANDLER( smb2_extra_r )
{
	return 0xa;
}

// Bug's Life handler from HazeMD
static READ16_HANDLER( bugl_extra_r )
{
	return 0x28;
}

// Elf Wor handler from HazeMD (DFJustin says the title is 'Linghuan Daoshi Super Magician')
static READ16_HANDLER( elfwor_0x400000_r )
{
	return 0x5500;
}

static READ16_HANDLER( elfwor_0x400002_r )
{
	return 0x0f00;
}

static READ16_HANDLER( elfwor_0x400004_r )
{
	return 0xc900;
}

static READ16_HANDLER( elfwor_0x400006_r )
{
	return 0x1800;
}

static WRITE16_HANDLER( genesis_TMSS_bank_w )
{
	/* this probably should do more, like make Genesis V2 'die' if the SEGA string is not written promptly */
}



/*************************************
 *
 *  Handlers for SRAM
 *
 *************************************/


static READ16_HANDLER( genesis_sram_read )
{
	return genesis_sram_active ? genesis_sram[offset] : 0;
}

static WRITE16_HANDLER( genesis_sram_write )
{
	if (genesis_sram_active)
	{
		if (!genesis_sram_readonly)
			genesis_sram[offset] = data;
	}
}

static WRITE16_HANDLER( genesis_sram_toggle )
{

	/* unsure if this is actually supposed to toggle or just switch on?
	* Yet to encounter game that utilizes
	*/
	genesis_sram_active = (data & 1) ? 1 : 0;
	genesis_sram_readonly = (data & 2) ? 1 : 0;
		
	if (genesis_sram_active)
		memcpy(megadriv_backupram, genesis_sram, genesis_sram_end - genesis_sram_start);
}



/*************************************
 *
 *  Machine driver reset
 *
 *************************************/


static void setup_megadriv_custom_mappers(running_machine *machine)
{
	static int relocate = VIRGIN_COPY_GEN;
	unsigned char *ROM;
	UINT32 mirroraddr;

	ROM = memory_region(machine, "main");

	if (cart_type == SSF2)
	{
		memcpy(&ROM[0x800000], &ROM[VIRGIN_COPY_GEN + 0x400000], 0x100000);
		memcpy(&ROM[0x400000], &ROM[VIRGIN_COPY_GEN], 0x400000);
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x400000);

		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa130f0, 0xa130ff, 0, 0, genesis_ssf2_bank_w);
	}

	if (cart_type == LIONK3 || cart_type == SKINGKONG)
	{
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x200000); /* default rom */
		memcpy(&ROM[0x200000], &ROM[VIRGIN_COPY_GEN], 0x200000); /* default rom */

		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_prot_r, g_l3alt_protw);
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_l3alt_bank_w);
	}

	if (cart_type == SDK99)
	{
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x300000); /* default rom */
		memcpy(&ROM[0x300000], &ROM[VIRGIN_COPY_GEN], 0x100000); /* default rom */

		memory_install_readwrite16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_prot_r, g_l3alt_protw);
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_l3alt_bank_w);
	}

	if (cart_type == REDCLIFF)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, redclif_prot2_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, redclif_prot_r);
	}

	if (cart_type == REDCL_EN)
	{
		int x;

		for (x = VIRGIN_COPY_GEN; x < VIRGIN_COPY_GEN + 0x200005; x++)
		{
			ROM[x] ^= 0x40;
		}

		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN + 4], 0x200000); /* default rom */

		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, redclif_prot2_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, redclif_prot_r);
	}

	if (cart_type == RADICA)
	{
		memcpy(&ROM[0x400000], &ROM[VIRGIN_COPY_GEN], 0x400000); // keep a copy for later banking.. making use of huge ROM_REGION allocated to genesis driver
		memcpy(&ROM[0x800000], &ROM[VIRGIN_COPY_GEN], 0x400000); // wraparound banking (from hazemd code)
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x400000);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa1307f, 0, 0, radica_bank_select);
	}

	if (cart_type == KOF99)
	{
		//memcpy(&ROM[0x000000],&ROM[VIRGIN_COPY_GEN],0x300000);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13001, 0, 0, kof99_0xA13000_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13002, 0xa13003, 0, 0, kof99_0xA13002_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa1303e, 0xa1303f, 0, 0, kof99_00A1303E_r);
	}

	if (cart_type == SOULBLAD)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, soulb_0x400002_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, soulb_0x400004_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, soulb_0x400006_r);
	}

	if (cart_type == MJLOVER)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, mjlovr_prot_1_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x401000, 0x401001, 0, 0, mjlovr_prot_2_r);
	}

	if (cart_type == SQUIRRELK)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, squirrel_king_extra_r);
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, squirrel_king_extra_w);
	}

	if (cart_type == SMOUSE)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, smous_prot_r);
	}

	if (cart_type == SMB)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13001, 0, 0, smbro_prot_r);
	}

	if (cart_type == SMB2)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13001, 0, 0, smb2_extra_r);
	}

	if (cart_type == KAIJU)
	{
		memcpy(&ROM[0x400000], &ROM[VIRGIN_COPY_GEN], 0x200000);
		memcpy(&ROM[0x600000], &ROM[VIRGIN_COPY_GEN], 0x200000);
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x200000);

		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_kaiju_bank_w);
	}

	if (cart_type == CHINFIGHT3)
	{
		memcpy(&ROM[0x400000], &ROM[VIRGIN_COPY_GEN], 0x200000);
		memcpy(&ROM[0x600000], &ROM[VIRGIN_COPY_GEN], 0x200000);
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x200000);

		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x4fffff, 0, 0, g_chifi3_prot_r);
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_chifi3_bank_w);
	}

	if (cart_type == LIONK2)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, lion2_prot1_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, lion2_prot2_r);
		memory_install_write16_handler(machine, 0,ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, lion2_prot1_w);
		memory_install_write16_handler(machine, 0,ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, lion2_prot2_w);
	}

	if (cart_type == BUGSLIFE)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13001, 0, 0, bugl_extra_r);
	}

	if (cart_type == ELFWOR)
	{
	/* It return (0x55 @ 0x400000 OR 0xc9 @ 0x400004) AND (0x0f @ 0x400002 OR 0x18 @ 0x400006). It is probably best to add handlers for all 4 addresses. */

		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, elfwor_0x400000_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, elfwor_0x400002_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, elfwor_0x400004_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, elfwor_0x400006_r);
	}

	if (cart_type == ROCKMANX3)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13001, 0, 0, rx3_extra_r);
	}

	if (cart_type == SBUBBOB)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, sbub_extra1_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, sbub_extra2_r);
	}

	if (cart_type == KOF98)
	{
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x480000, 0x480001, 0, 0, g_kof98_aa_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4800e0, 0x4800e1, 0, 0, g_kof98_aa_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4824a0, 0x4824a1, 0, 0, g_kof98_aa_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x488880, 0x488881, 0, 0, g_kof98_aa_r);

		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4a8820, 0x4a8821, 0, 0, g_kof98_0a_r);
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4f8820, 0x4f8821, 0, 0, g_kof98_00_r);
	}

	if (cart_type == REALTEC)
	{
		realtek_old_bank_addr = -1;
		
		/* Realtec mapper!*/
		memcpy(&ROM[0x400000], &ROM[relocate], 0x80000);

		for (mirroraddr = 0; mirroraddr < 0x400000; mirroraddr += 0x2000) 
		{
			memcpy(ROM + mirroraddr, ROM + relocate + 0x7e000, 0x002000); /* copy last 8kb across the whole rom region */
		}

		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, realtec_400000_w);
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x402000, 0x402001, 0, 0, realtec_402000_w);
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x404000, 0x404001, 0, 0, realtec_404000_w);
	}

	if (cart_type == SUP19IN1)
	{
		memcpy(&ROM[0x400000], &ROM[VIRGIN_COPY_GEN], 0x400000); // allow hard reset to menu
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13039, 0, 0, s19in1_bank);
	}

	if (cart_type == SUP15IN1)
	{
		memcpy(&ROM[0x400000], &ROM[VIRGIN_COPY_GEN], 0x200000); // allow hard reset to menu
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13039, 0, 0, s19in1_bank);
	}

	if (has_sram)
	{
        memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, genesis_sram_start & 0x3fffff, genesis_sram_end & 0x3fffff, 0, 0, genesis_sram_read);
        memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, genesis_sram_start & 0x3fffff, genesis_sram_end & 0x3fffff, 0, 0, genesis_sram_write);
        memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa130f0, 0xa130f1, 0, 0, genesis_sram_toggle);
	}


	/* install NOP handler for TMSS */
	memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa14000, 0xa14003, 0, 0, genesis_TMSS_bank_w);
}


MACHINE_RESET( md_mappers )
{
	setup_megadriv_custom_mappers(machine);
}


/*************************************
 *
 *  Driver initialization
 *
 *************************************/


static void genesis_machine_stop(running_machine *machine)
{
}

DRIVER_INIT( gencommon )
{
	if (genesis_sram)
	{
		add_exit_callback(machine, genesis_machine_stop);
	}
}



/*************************************
 *
 *  Cart handling
 *
 *************************************/


/******* Memcmp for both endianness *******/

static int allendianmemcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *realbuf;

#ifdef LSB_FIRST
	unsigned char flippedbuf[64];
	unsigned int i;

	if ((n & 1) || (n > 63)) return -1 ; // don't want odd sized buffers or too large a compare
	for (i = 0; i < n; i++) flippedbuf[i] = *((char *)s2 + (i ^ 1));
	realbuf = flippedbuf;

#else

	realbuf = s2;

#endif
	return memcmp(s1,realbuf,n);
}


/******* SMD files detection *******/

/* code taken directly from GoodGEN by Cowering */
static int genesis_is_funky_SMD(unsigned char *buf,unsigned int len)
{
	/* aq quiz */
	if (!strncmp("UZ(-01  ", (const char *) &buf[0xf0], 8))
		return 1;

    /* Phelios USA redump */
	/* target earth */
	/* klax (namcot) */
	if (buf[0x2080] == ' ' && buf[0x0080] == 'S' && buf[0x2081] == 'E' && buf[0x0081] == 'G')
		return 1;

    /* jap baseball 94 */
	if (!strncmp("OL R-AEAL", (const char *) &buf[0xf0], 9))
		return 1;

    /* devilish Mahjong Tower */
    if (!strncmp("optrEtranet", (const char *) &buf[0xf3], 11))
		return 1;

	/* golden axe 2 beta */
	if (buf[0x0100] == 0x3c && buf[0x0101] == 0 && buf[0x0102] == 0 && buf[0x0103] == 0x3c)
		return 1;

    /* omega race */
	if (!strncmp("OEARC   ", (const char *) &buf[0x90], 8))
		return 1;

    /* budokan beta */
	if ((len >= 0x6708+8) && !strncmp(" NTEBDKN", (const char *) &buf[0x6708], 8))
		return 1;

    /* cdx pro 1.8 bios */
	if (!strncmp("so fCXP", (const char *) &buf[0x2c0], 7))
		return 1;

    /* ishido (hacked) */
	if (!strncmp("sio-Wyo ", (const char *) &buf[0x0090], 8))
		return 1;

    /* onslaught */
	if (!strncmp("SS  CAL ", (const char *) &buf[0x0088], 8))
		return 1;

    /* tram terror pirate */
	if ((len >= 0x3648 + 8) && !strncmp("SG NEPIE", (const char *) &buf[0x3648], 8))
		return 1;

    /* breath of fire 3 chinese */
	if (buf[0x0007] == 0x1c && buf[0x0008] == 0x0a && buf[0x0009] == 0xb8 && buf[0x000a] == 0x0a)
		return 1;

    /*tetris pirate */
	if ((len >= 0x1cbe + 5) && !strncmp("@TTI>", (const char *) &buf[0x1cbe], 5))
		return 1;

	return 0;
}



/* code taken directly from GoodGEN by Cowering */
static int genesis_is_SMD(unsigned char *buf,unsigned int len)
{
	if (buf[0x2080] == 'S' && buf[0x80] == 'E' && buf[0x2081] == 'G' && buf[0x81] == 'A')
		return 1;
	return genesis_is_funky_SMD(buf,len);
}


/******* Image loading *******/

static DEVICE_IMAGE_LOAD( genesis_cart )
{
	unsigned char *ROM, *rawROM, *tmpROMnew, *tmpROM, *secondhalf;
	int relocate, length, ptr, x;
#ifdef LSB_FIRST
	unsigned char fliptemp;
#endif

	rawROM = memory_region(image->machine, "main");
	ROM = rawROM /*+ 512 */;

	length = image_fread(image, rawROM + 0x2000, 0x600000);
	logerror("image length = 0x%x\n", length);

	/* is this a SMD file? */
	if (genesis_is_SMD(&rawROM[0x2200], (unsigned)length))
	{
		tmpROMnew = ROM;
		tmpROM = ROM + 0x2000 + 512;

		for (ptr = 0; ptr < MAX_MD_CART_SIZE / (8192); ptr += 2)
		{
			for (x = 0; x < 8192; x++)
			{
				*tmpROMnew++ = *(tmpROM + ((ptr + 1) * 8192) + x);
				*tmpROMnew++ = *(tmpROM + ((ptr + 0) * 8192) + x);
			}
		}

#ifdef LSB_FIRST
		tmpROMnew = ROM;
		for (ptr = 0; ptr < length; ptr += 2)
		{
			fliptemp = tmpROMnew[ptr];
			tmpROMnew[ptr] = tmpROMnew[ptr+1];
			tmpROMnew[ptr+1] = fliptemp;
		}
#endif
		genesis_last_loaded_image_length = length - 512;
		memcpy(&ROM[VIRGIN_COPY_GEN], &ROM[0x000000], MAX_MD_CART_SIZE);  /* store a copy of data for MACHINE_RESET processing */

		relocate = 0;

	}

	/* is this a MD file? */
	else if ((rawROM[0x2080] == 'E') && (rawROM[0x2081] == 'A') &&
				(rawROM[0x2082] == 'M' || rawROM[0x2082] == 'G'))
	{
		tmpROMnew = malloc(length);
		secondhalf = &tmpROMnew[length >> 1];

		if (!tmpROMnew)
		{
			logerror("Memory allocation failed reading roms!\n");
			return INIT_FAIL;
		}

		memcpy(tmpROMnew, ROM + 0x2000, length);
		for (ptr = 0; ptr < length; ptr += 2)
		{

			ROM[ptr] = secondhalf[ptr >> 1];
			ROM[ptr + 1] = tmpROMnew[ptr >> 1];
		}
		free(tmpROMnew);

#ifdef LSB_FIRST
		for (ptr = 0; ptr < length; ptr += 2)
		{
			fliptemp = ROM[ptr];
			ROM[ptr] = ROM[ptr+1];
			ROM[ptr+1] = fliptemp;
		}
#endif
		genesis_last_loaded_image_length = length;
		memcpy(&ROM[VIRGIN_COPY_GEN], &ROM[0x000000], MAX_MD_CART_SIZE);  /* store a copy of data for MACHINE_RESET processing */
		relocate = 0;

	}

	/* BIN it is, then */
	else
	{
		relocate = 0x2000;
		genesis_last_loaded_image_length = length;

 		for (ptr = 0; ptr < MAX_MD_CART_SIZE + relocate; ptr += 2)		/* mangle bytes for little endian machines */
		{
#ifdef LSB_FIRST
			int temp = ROM[relocate + ptr];

			ROM[ptr] = ROM[relocate + ptr + 1];
			ROM[ptr + 1] = temp;
#else
			ROM[ptr] = ROM[relocate + ptr];
			ROM[ptr + 1] = ROM[relocate + ptr + 1];
#endif
		}

		memcpy(&ROM[VIRGIN_COPY_GEN], &ROM[0x000000], MAX_MD_CART_SIZE);  /* store a copy of data for MACHINE_RESET processing */
	}


	/* Detect carts which need additional handlers */
	{
		static unsigned char smouse_sig[] = { 0x4d, 0xf9, 0x00, 0x40, 0x00, 0x02 }, 
			mjlover_sig[]	= { 0x13, 0xf9, 0x00, 0x40, 0x00, 0x00 }, // move.b  ($400000).l,($FFFF0C).l (partial)
			squir_sig[]		= { 0x26, 0x79, 0x00, 0xff, 0x00, 0xfa },
			bugsl_sig[]		= { 0x20, 0x12, 0x13, 0xc0, 0x00, 0xff },
			sbub_sig[]		= { 0x0c, 0x39, 0x00, 0x55, 0x00, 0x40 }, // cmpi.b  #$55,($400000).l
			lk3_sig[]		= { 0x0c, 0x01, 0x00, 0x30, 0x66, 0xe4 },
			sdk_sig[]		= { 0x48, 0xe7, 0xff, 0xfe, 0x52, 0x79 },
			redcliff_sig[]	= { 0x10, 0x39, 0x00, 0x40, 0x00, 0x04 }, // move.b  ($400004).l,d0
			redcl_en_sig[]	= { 0x50, 0x79, 0x40, 0x00, 0x40, 0x44 }, // move.b  ($400004).l,d0
			smb_sig[]		= { 0x20, 0x4d, 0x41, 0x52, 0x49, 0x4f },
			smb2_sig[]		= { 0x4e, 0xb9, 0x00, 0x0f, 0x25, 0x84 },
			kaiju_sig[]		= { 0x19, 0x7c, 0x00, 0x01, 0x00, 0x00 },
			chifi3_sig[]	= { 0xb6, 0x16, 0x66, 0x00, 0x00, 0x4a },
			lionk2_sig[]	= { 0x26, 0x79, 0x00, 0xff, 0x00, 0xf4 },
			rx3_sig[]		= { 0x66, 0x00, 0x00, 0x0e, 0x30, 0x3c },
			kof98_sig[]		= { 0x9b, 0xfc, 0x00, 0x00, 0x4a, 0x00 },
			s15in1_sig[]	= { 0x22, 0x3c, 0x00, 0xa1, 0x30, 0x00 },
			kof99_sig[]		= { 0x20, 0x3c, 0x30, 0x00, 0x00, 0xa1 }, // move.l  #$300000A1,d0
			radica_sig[]	= { 0x4e, 0xd0, 0x30, 0x39, 0x00, 0xa1 }, // jmp (a0) move.w ($a130xx),d0
			soulb_sig[]		= { 0x33, 0xfc, 0x00, 0x0c, 0x00, 0xff }, // move.w  #$C,($FF020A).l (what happens if check fails)
			s19in1_sig[]	= { 0x13, 0xc0, 0x00, 0xa1, 0x30, 0x38 };

		switch (genesis_last_loaded_image_length)
		{
			case 0x80000:
				if (!allendianmemcmp(&ROM[0x08c8], &smouse_sig[0], sizeof(smouse_sig)))
					cart_type = SMOUSE;

				if (!allendianmemcmp((char *)&ROM[0x7e30e], "SEGA", 4) ||
					!allendianmemcmp((char *)&ROM[0x7e100], "SEGA", 4) ||
					!allendianmemcmp((char *)&ROM[0x7e1e6], "SEGA", 4))
						cart_type = REALTEC;
				break;

			case 0x100000:
				if (!allendianmemcmp(&ROM[0x01b24], &mjlover_sig[0], sizeof(mjlover_sig)))
					cart_type = MJLOVER;

				if (!allendianmemcmp(&ROM[0x03b4], &squir_sig[0], sizeof(squir_sig)))
					cart_type = SQUIRRELK;

				if (!allendianmemcmp(&ROM[0xee0d0], &bugsl_sig[0], sizeof(bugsl_sig)))
					cart_type = BUGSLIFE;

				if (!allendianmemcmp((char *)&ROM[0x0172], "GAME : ELF WOR", 14))
					cart_type = ELFWOR;

				if (!allendianmemcmp(&ROM[0x123e4], &sbub_sig[0], sizeof(sbub_sig)))
					cart_type = SBUBBOB;
				break;

			case 0x200000:
				if (!allendianmemcmp(&ROM[0x18c6], &lk3_sig[0], sizeof(lk3_sig)))
					cart_type = LIONK3;

				if (!allendianmemcmp(&ROM[0x220], &sdk_sig[0], sizeof(sdk_sig)))
					cart_type = SKINGKONG;

				if (!allendianmemcmp(&ROM[0xce560], &redcliff_sig[0], sizeof(redcliff_sig)))
					cart_type = REDCLIFF;

				if (!allendianmemcmp(&ROM[0xc8cb0], &smb_sig[0], sizeof(smb_sig)))
					cart_type = SMB;

				if (!allendianmemcmp(&ROM[0xf24d6], &smb2_sig[0], sizeof(smb2_sig)))
					cart_type = SMB2;

				if (!allendianmemcmp(&ROM[0x674e], &kaiju_sig[0], sizeof(kaiju_sig)))
					cart_type = KAIJU;

				if (!allendianmemcmp(&ROM[0x1780], &chifi3_sig[0], sizeof(chifi3_sig)))
					cart_type = CHINFIGHT3;

				if (!allendianmemcmp(&ROM[0x03c2], &lionk2_sig[0], sizeof(lionk2_sig)))
					cart_type = LIONK2;

				if (!allendianmemcmp(&ROM[0xc8b90], &rx3_sig[0], sizeof(rx3_sig)))
					cart_type = ROCKMANX3;

				if (!allendianmemcmp(&ROM[0x56ae2], &kof98_sig[0], sizeof(kof98_sig)))
					cart_type = KOF98;

				if (!allendianmemcmp(&ROM[0x17bb2], &s15in1_sig[0], sizeof(s15in1_sig)))
					cart_type = SUP15IN1;
				break;

			case 0x200005:
				if (!allendianmemcmp(&ROM[0xce564], &redcl_en_sig[0], sizeof(redcliff_sig)))
					cart_type = REDCL_EN;
				break;

			case 0x300000:
				if (!allendianmemcmp(&ROM[0x220], &sdk_sig[0], sizeof(sdk_sig)))
					cart_type = SDK99;

				if (!allendianmemcmp(&ROM[0x1fd0d2], &kof99_sig[0], sizeof(kof99_sig)))
					cart_type = KOF99;
				break;

			case 0x400000:
				if (!allendianmemcmp(&ROM[0x3c031c], &radica_sig[0], sizeof(radica_sig)) ||
					!allendianmemcmp(&ROM[0x3f031c], &radica_sig[0], sizeof(radica_sig))) // ssf+gng + radica vol1
						cart_type = RADICA;

				if (!allendianmemcmp(&ROM[0x028460], &soulb_sig[0], sizeof(soulb_sig)))
					cart_type = SOULBLAD;

				if (!allendianmemcmp(&ROM[0x1e700], &s19in1_sig[0], sizeof(s19in1_sig)))
					cart_type = SUP19IN1;
				break;

			case 0x500000:
				if (!allendianmemcmp((char *)&ROM[0x0120], "SUPER STREET FIGHTER2 ", 22))
					cart_type = SSF2;
				break;

			default:
				break;
		}
	}

	logerror("cart type: %d\n", cart_type);

	/* check if cart has battery save */
	genesis_sram = NULL;
	
	/* This is not enough! some games (e.g. Sonic 3) are not detected */
	if (ROM[0x1b1] == 'R' && ROM[0x1b0] == 'A')
	{
		has_sram = 1;
		genesis_sram_start = (ROM[0x1b5] << 24 | ROM[0x1b4] << 16 | ROM[0x1b7] << 8 | ROM[0x1b6]);
		genesis_sram_end = (ROM[0x1b9] << 24 | ROM[0x1b8] << 16 | ROM[0x1bb] << 8 | ROM[0x1ba]);

		if ((genesis_sram_start > genesis_sram_end) || ((genesis_sram_end - genesis_sram_start) >= 0x10000))	// we assume at most 64k of SRAM (HazeMD uses at most 64k). is this correct?
			genesis_sram_end = genesis_sram_start + 0x10000;

		/* This fixes Wonderboy 5, but we need more info on the SRAM and a better detection routine! */
		if ((genesis_sram_end - genesis_sram_start) < 0x800 )
			has_sram = 0;

		if (genesis_sram_start & 1)
			genesis_sram_start -= 1;

		if (!(genesis_sram_end & 1))
			genesis_sram_end += 1;
		
		genesis_sram = malloc_or_die(genesis_sram_end - genesis_sram_start);
		image_battery_load(image_from_devtype_and_index(IO_CARTSLOT, 0), genesis_sram, genesis_sram_end - genesis_sram_start);

		megadriv_backupram = (UINT16*)ROM + ((genesis_sram_start & 0x3fffff) / 2);
		memmove(&megadriv_backupram[0], genesis_sram, genesis_sram_end - genesis_sram_start);

		if (genesis_last_loaded_image_length < genesis_sram_start)
			genesis_sram_active = 1;
	}

	logerror("SRAM detected? %s\n", has_sram ? "Yes" : "No");
	if (has_sram)
		logerror("SRAM starting location %X - SRAM Length %X\n", genesis_sram_start, genesis_sram_end - genesis_sram_start);

	return INIT_PASS;
}


/******* Image unloading (with SRAM saving) *******/

static DEVICE_IMAGE_UNLOAD( genesis_cart )
{
	/* Write out the battery file if necessary */
	if (has_sram && (genesis_sram != NULL) && (genesis_sram_end - genesis_sram_start > 0))
	{
		image_battery_save(image_from_devtype_and_index(IO_CARTSLOT, 0), genesis_sram, genesis_sram_end - genesis_sram_start);
	}
}


/******* Cart getinfo *******/

void genesis_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:						info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(genesis_cart); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(genesis_cart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "smd,bin,md,gen"); break;

		default:											cartslot_device_getinfo(devclass, state, info); break;
	}
}


void pico_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:						info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(genesis_cart); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(genesis_cart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin"); break;

		default:											cartslot_device_getinfo(devclass, state, info); break;
	}
}
