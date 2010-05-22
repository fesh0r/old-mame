/*****************************************************************************************

    NES MMC Emulation - UNIF boards

    Very preliminary support for UNIF boards

    TODO:
    - add more info
    - test more images
    - properly support mirroring, WRAM, etc.

****************************************************************************************/

/*************************************************************

    Board BMC-64IN1NOREPEAT

    Games: 64-in-1 Y2K

    In MESS: Supported

*************************************************************/

static void bmc_64in1nr_set_prg( running_machine *machine )
{
	UINT8 helper1 = (bmc_64in1nr_reg[1] & 0x1f);
	UINT8 helper2 = (helper1 << 1) | ((bmc_64in1nr_reg[1] & 0x40) >> 6);

	if (bmc_64in1nr_reg[0] & 0x80)
	{
		if (bmc_64in1nr_reg[1] & 0x80)
			prg32(machine, helper1);
		else
		{
			prg16_89ab(machine, helper2);
			prg16_cdef(machine, helper2);
		}
	}
	else
		prg16_cdef(machine, helper2);
}

static WRITE8_HANDLER( bmc_64in1nr_l_w )
{
	LOG_MMC(("bmc_64in1nr_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	switch (offset)
	{
	case 0x1000:
	case 0x1001:
	case 0x1002:
	case 0x1003:
		bmc_64in1nr_reg[offset & 0x03] = data;
		bmc_64in1nr_set_prg(space->machine);
		chr8(space->machine, ((bmc_64in1nr_reg[0] >> 1) & 0x03) | (bmc_64in1nr_reg[2] << 2), CHRROM);
		break;
	}
	if (offset == 0x1000)	/* reg[0] also sets mirroring */
		set_nt_mirroring(space->machine, BIT(data, 5) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
}

static WRITE8_HANDLER( bmc_64in1nr_w )
{
	LOG_MMC(("bmc_64in1nr_w offset: %04x, data: %02x\n", offset, data));

	bmc_64in1nr_reg[3] = data;	// reg[3] is currently unused?!?
}

/*************************************************************

    Board BMC-190IN1

    Games: 190-in-1

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_190in1_w )
{
	LOG_MMC(("bmc_190in1_w offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
	data >>= 2;
	prg16_89ab(space->machine, data);
	prg16_cdef(space->machine, data);
	chr8(space->machine, data, CHRROM);
}

/*************************************************************

    Board BMC-A65AS

    Games: 3-in-1 (N068)

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_a65as_w )
{
	UINT8 helper = (data & 0x30) >> 1;
	LOG_MMC(("bmc_a65as_w offset: %04x, data: %02x\n", offset, data));

	if (data & 0x80)
		set_nt_mirroring(space->machine, BIT(data, 5) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	else
		set_nt_mirroring(space->machine, BIT(data, 3) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);

	if (data & 0x40)
		prg32(space->machine, data >> 1);
	else
	{
		prg16_89ab(space->machine, helper | (data & 0x07));
		prg16_cdef(space->machine, helper | 0x07);
	}
}

/*************************************************************

    Board BMC-GS2004

    Games: Tetris Family 6-in-1

    In MESS: Preliminary Support. It also misses WRAM handling
       (we need reads from 0x6000-0x7fff)

*************************************************************/

static WRITE8_HANDLER( bmc_gs2004_w )
{
	LOG_MMC(("bmc_gs2004_w offset: %04x, data: %02x\n", offset, data));

	prg32(space->machine, data);
}

/*************************************************************

    Board BMC-GS2013

    Games: Tetris Family 12-in-1

    In MESS: Preliminary Support. It also misses WRAM handling
       (we need reads from 0x6000-0x7fff)

*************************************************************/

static WRITE8_HANDLER( bmc_gs2013_w )
{
	LOG_MMC(("bmc_gs2013_w offset: %04x, data: %02x\n", offset, data));

	if (data & 0x08)
		prg32(space->machine, data & 0x09);
	else
		prg32(space->machine, data & 0x07);
}

/*************************************************************

    Board BMC-SUPER24IN1SC03

    Games: Super 24-in-1

    In MESS: Supported

*************************************************************/

static void bmc_s24in1sc03_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	static const UINT8 masks[8] = {0x3f, 0x1f, 0x0f, 0x01, 0x03, 0x00, 0x00, 0x00};
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;
	int prg_base = bmc_s24in1sc03_reg[1] << 1;
	int prg_mask = masks[bmc_s24in1sc03_reg[0] & 0x07];

	prg8_89(machine, prg_base | (state->prg_bank[0 ^ prg_flip] & prg_mask));
	prg8_ab(machine, prg_base | (state->prg_bank[1] & prg_mask));
	prg8_cd(machine, prg_base | (state->prg_bank[2 ^ prg_flip] & prg_mask));
	prg8_ef(machine, prg_base | (state->prg_bank[3] & prg_mask));
}

static void bmc_s24in1sc03_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 chr = (bmc_s24in1sc03_reg[0] & 0x20) ? CHRRAM : CHRROM;
	int chr_base = (bmc_s24in1sc03_reg[2] << 3) & 0xf00;

	chr1_x(machine, chr_page ^ 0, chr_base | (state->vrom_bank[0] & ~0x01), chr);
	chr1_x(machine, chr_page ^ 1, chr_base | (state->vrom_bank[0] |  0x01), chr);
	chr1_x(machine, chr_page ^ 2, chr_base | (state->vrom_bank[1] & ~0x01), chr);
	chr1_x(machine, chr_page ^ 3, chr_base | (state->vrom_bank[1] |  0x01), chr);
	chr1_x(machine, chr_page ^ 4, chr_base | state->vrom_bank[2], chr);
	chr1_x(machine, chr_page ^ 5, chr_base | state->vrom_bank[3], chr);
	chr1_x(machine, chr_page ^ 6, chr_base | state->vrom_bank[4], chr);
	chr1_x(machine, chr_page ^ 7, chr_base | state->vrom_bank[5], chr);
}

static WRITE8_HANDLER( bmc_s24in1sc03_l_w )
{
	LOG_MMC(("bmc_s24in1sc03_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1ff0)
	{
		bmc_s24in1sc03_reg[0] = data;
		bmc_s24in1sc03_set_prg(space->machine);
		bmc_s24in1sc03_set_chr(space->machine);
	}

	if (offset == 0x1ff1)
	{
		bmc_s24in1sc03_reg[1] = data;
		bmc_s24in1sc03_set_prg(space->machine);
	}

	if (offset == 0x1ff2)
	{
		bmc_s24in1sc03_reg[2] = data;
		bmc_s24in1sc03_set_chr(space->machine);
	}
}

static WRITE8_HANDLER( bmc_s24in1sc03_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 cmd, bmc_s24in1sc03_helper;
	LOG_MMC(("bmc_s24in1sc03_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x6001)
	{
		case 0x0000:
			bmc_s24in1sc03_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (bmc_s24in1sc03_helper & 0x40)
				bmc_s24in1sc03_set_prg(space->machine);

			/* Has CHR Mode changed? */
			if (bmc_s24in1sc03_helper & 0x80)
				bmc_s24in1sc03_set_chr(space->machine);
			break;

		case 0x0001:
			cmd = state->mmc_cmd1 & 0x07;
			switch (cmd)
			{
				case 0: case 1:	// these do not need to be separated: we take care of them in set_chr!
				case 2: case 3: case 4: case 5:
					state->vrom_bank[cmd] = data;
					bmc_s24in1sc03_set_chr(space->machine);
					break;
				case 6:
				case 7:
					state->prg_bank[cmd - 6] = data;
					bmc_s24in1sc03_set_prg(space->machine);
					break;
			}
			break;

		default:
			mapper4_w(space, offset, data);
			break;
	}
}

/*************************************************************

    Board BMC-T-262

    Games: 4-in-1 (D-010), 8-in-1 (A-020)

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_t262_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 mmc_helper;
	LOG_MMC(("bmc_t262_w offset: %04x, data: %02x\n", offset, data));

	if (state->mmc_cmd2 || offset == 0)
	{
		state->mmc_cmd1 = (state->mmc_cmd1 & 0x38) | (data & 0x07);
		prg16_89ab(space->machine, state->mmc_cmd1);
	}
	else
	{
		state->mmc_cmd2 = 1;
		set_nt_mirroring(space->machine, BIT(data, 1) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		mmc_helper = ((offset >> 3) & 0x20) | ((offset >> 2) & 0x18);
		state->mmc_cmd1 = mmc_helper | (state->mmc_cmd1 & 0x07);
		prg16_89ab(space->machine, state->mmc_cmd1);
		prg16_cdef(space->machine, mmc_helper | 0x07);
	}
}

/*************************************************************

    Board BMC-WS

    Games: Super 40-in-1

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( bmc_ws_m_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 mmc_helper;
	LOG_MMC(("bmc_ws_m_w offset: %04x, data: %02x\n", offset, data));

	if (offset < 0x1000)
	{
		switch (offset & 0x01)
		{
		case 0:
			if (!state->mmc_cmd1)
			{
				state->mmc_cmd1 = data & 0x20;
				set_nt_mirroring(space->machine, BIT(data, 4) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
				mmc_helper = (~data & 0x08) >> 3;
				prg16_89ab(space->machine, data & ~mmc_helper);
				prg16_cdef(space->machine, data |  mmc_helper);
			}
			break;
		case 1:
			if (!state->mmc_cmd1)
			{
				chr8(space->machine, data, CHRROM);
			}
			break;
		}
	}
}

/*************************************************************

    Board DREAMTECH01

    Games: Korean Igo

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( dreamtech_l_w )
{
	LOG_MMC(("dreamtech_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1020)	/* 0x5020 */
		prg16_89ab(space->machine, data);
}

/*************************************************************

    Board UNL-8237

    Games: Pochahontas 2

    MMC3 clone

    In MESS: Not working

*************************************************************/

static void unl_8237_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 prg_flip = (state->mmc_cmd1 & 0x40) ? 2 : 0;

	if (!(unl_8237_reg[0] & 0x80))
	{
		prg8_89(machine, state->prg_bank[0 ^ prg_flip]);
		prg8_ab(machine, state->prg_bank[1]);
		prg8_cd(machine, state->prg_bank[2 ^ prg_flip]);
		prg8_ef(machine, state->prg_bank[3]);
	}
}

static void unl_8237_set_chr( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	UINT8 chr_page = (state->mmc_cmd1 & 0x80) >> 5;
	UINT8 bank[8];
	int i;

	for(i = 0; i < 6; i++)
		bank[i] = state->vrom_bank[i] | ((unl_8237_reg[1] << 6) & 0x100);

	chr1_x(machine, chr_page ^ 0, (bank[0] & ~0x01), CHRROM);
	chr1_x(machine, chr_page ^ 1, (bank[0] |  0x01), CHRROM);
	chr1_x(machine, chr_page ^ 2, (bank[1] & ~0x01), CHRROM);
	chr1_x(machine, chr_page ^ 3, (bank[1] |  0x01), CHRROM);
	chr1_x(machine, chr_page ^ 4, bank[2], CHRROM);
	chr1_x(machine, chr_page ^ 5, bank[3], CHRROM);
	chr1_x(machine, chr_page ^ 6, bank[4], CHRROM);
	chr1_x(machine, chr_page ^ 7, bank[5], CHRROM);
}

static WRITE8_HANDLER( unl_8237_l_w )
{
	LOG_MMC(("unl_8237_l_w offset: %04x, data: %02x\n", offset, data));
	offset += 0x100;

	if (offset == 0x1000)
	{
		unl_8237_reg[0] = data;
		if (unl_8237_reg[0] & 0x80)
		{
			if (unl_8237_reg[0] & 0x20)
				prg32(space->machine, (unl_8237_reg[0] & 0x0f) >> 1);
			else
			{
				prg16_89ab(space->machine, unl_8237_reg[0] & 0x1f);
				prg16_cdef(space->machine, unl_8237_reg[0] & 0x1f);
			}
		}
		else
			unl_8237_set_prg(space->machine);
	}

	if (offset == 0x1001)
	{
		unl_8237_reg[1] = data;
		unl_8237_set_chr(space->machine);
	}
}

static WRITE8_HANDLER( unl_8237_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 unl_8237_helper, cmd;
	static const UINT8 conv_table[8] = {0,2,6,1,7,3,4,5};
	LOG_MMC(("unl_8237_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7000)
	{
		case 0x2000:
		case 0x3000:
			unl_8237_reg[2] = 1;
			data = (data & 0xc0) | conv_table[data & 0x07];
			unl_8237_helper = state->mmc_cmd1 ^ data;
			state->mmc_cmd1 = data;

			/* Has PRG Mode changed? */
			if (unl_8237_helper & 0x40)
				unl_8237_set_prg(space->machine);

			/* Has CHR Mode changed? */
			if (unl_8237_helper & 0x80)
				unl_8237_set_chr(space->machine);
			break;

		case 0x4000:
		case 0x5000:
			if (unl_8237_reg[2])
			{
				unl_8237_reg[2] = 0;
				cmd = state->mmc_cmd1 & 0x07;
				switch (cmd)
				{
					case 0: case 1:
					case 2: case 3: case 4: case 5:
						state->vrom_bank[cmd] = data;
						unl_8237_set_chr(space->machine);
						break;
					case 6:
					case 7:
						state->prg_bank[cmd - 6] = data;
						unl_8237_set_prg(space->machine);
						break;
				}
			}
			break;

		case 0x0000:
		case 0x1000:
			set_nt_mirroring(space->machine, (data | (data >> 7)) & 0x01 ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
			break;

		case 0x6000:
			break;

		case 0x7000:
			mapper4_w(space, 0x6001, data);
			mapper4_w(space, 0x4000, data);
			mapper4_w(space, 0x4001, data);
			break;
	}
}

/*************************************************************

    Board UNL-AX5705

    Games: Super Mario Bros. Pocker Mali (Crayon Shin-chan pirate hack)

    In MESS: Supported

*************************************************************/

static void unl_ax5705_set_prg( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	prg8_89(machine, state->prg_bank[0]);
	prg8_ab(machine, state->prg_bank[1]);
}

static WRITE8_HANDLER( unl_ax5705_w )
{
	nes_state *state = (nes_state *)space->machine->driver_data;
	UINT8 bank;
	LOG_MMC(("unl_ax5705_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x700f)
	{
	case 0x0000:
		state->prg_bank[0] = (data & 0x05) | ((data & 0x08) >> 2) | ((data & 0x02) << 2);
		unl_ax5705_set_prg(space->machine);
		break;
	case 0x0008:
		set_nt_mirroring(space->machine, BIT(data, 0) ? PPU_MIRROR_HORZ : PPU_MIRROR_VERT);
		break;
	case 0x2000:
		state->prg_bank[1] = (data & 0x05) | ((data & 0x08) >> 2) | ((data & 0x02) << 2);
		unl_ax5705_set_prg(space->machine);
		break;
	/* CHR banks 0, 1, 4, 5 */
	case 0x2008:
	case 0x200a:
	case 0x4008:
	case 0x400a:
		bank = ((offset & 0x4000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->vrom_bank[bank] = (state->vrom_bank[bank] & 0xf0) | (data & 0x0f);
		chr1_x(space->machine, bank, state->vrom_bank[bank], CHRROM);
		break;
	case 0x2009:
	case 0x200b:
	case 0x4009:
	case 0x400b:
		bank = ((offset & 0x4000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->vrom_bank[bank] = (state->vrom_bank[bank] & 0x0f) | ((data & 0x04) << 3) | ((data & 0x02) << 5) | ((data & 0x09) << 4);
		chr1_x(space->machine, bank, state->vrom_bank[bank], CHRROM);
		break;
	/* CHR banks 2, 3, 6, 7 */
	case 0x4000:
	case 0x4002:
	case 0x6000:
	case 0x6002:
		bank = 2 + ((offset & 0x2000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->vrom_bank[bank] = (state->vrom_bank[bank] & 0xf0) | (data & 0x0f);
		chr1_x(space->machine, bank, state->vrom_bank[bank], CHRROM);
		break;
	case 0x4001:
	case 0x4003:
	case 0x6001:
	case 0x6003:
		bank = 2 + ((offset & 0x2000) ? 4 : 0) + ((offset & 0x0002) ? 1 : 0);
		state->vrom_bank[bank] = (state->vrom_bank[bank] & 0x0f) | ((data & 0x04) << 3) | ((data & 0x02) << 5) | ((data & 0x09) << 4);
		chr1_x(space->machine, bank, state->vrom_bank[bank], CHRROM);
		break;
	}
}

/*************************************************************

    Board UNL-CC-21

    Games: Mi Hun Che

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( unl_cc21_w )
{
	LOG_MMC(("unl_cc21_w offset: %04x, data: %02x\n", offset, data));

	set_nt_mirroring(space->machine, BIT(data, 1) ? PPU_MIRROR_HIGH : PPU_MIRROR_LOW);
	chr8(space->machine, (offset & 0x01), CHRROM);
}

/*************************************************************

    Board UNL-KOF97

    Games: King of Fighters 97 (Rex Soft)

    MMC3 clone

    In MESS: Not working

*************************************************************/

static UINT8 unl_kof97_unscramble( UINT8 data )
{
	return ((data >> 1) & 0x01) | ((data >> 4) & 0x02) | ((data << 2) & 0x04) | ((data >> 0) & 0xd8) | ((data << 3) & 0x20);
}

static WRITE8_HANDLER( unl_kof97_w )
{
	LOG_MMC(("unl_kof97_w offset: %04x, data: %02x\n", offset, data));

	/* Addresses 0x9000, 0xa000, 0xd000 & 0xf000 behaves differently than MMC3 */
	if (offset == 0x1000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x0001, data);
	}
	else if (offset == 0x2000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x0000, data);
	}
	else if (offset == 0x5000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x4001, data);
	}
	else if (offset == 0x7000)
	{
		data = unl_kof97_unscramble(data);
		mapper4_w(space, 0x6001, data);
	}
	else		/* Other addresses behaves like MMC3, up to unscrambling data */
	{
		switch (offset & 0x6001)
		{
		case 0x0000:
		case 0x0001:
		case 0x4000:
		case 0x4001:
		case 0x6000:
		case 0x6001:
		case 0x2000:	/* are these ever called?!? */
		case 0x2001:
			data = unl_kof97_unscramble(data);
			mapper4_w(space, offset, data);
			break;
		}
	}
}

/*************************************************************

    Board UNL-T-230

    Games: Dragon Ball Z IV (Unl)

    In MESS: Supported

*************************************************************/

static WRITE8_HANDLER( unl_t230_w )
{
	LOG_MMC(("unl_t230_w offset: %04x, data: %02x\n", offset, data));

	switch (offset & 0x7007)
	{
		case 0x0000:
			break;
		case 0x2000:
			data = (data << 1) & 0x1f;
			prg8_89(space->machine, data);
			prg8_ab(space->machine, data | 0x01);
			break;

		default:
			konami_vrc2b_w(space, offset, data);
			break;
	}
}


/*************************************************************

    Constants

*************************************************************/

/* CHRRAM sizes */
enum
{
CHRRAM_0 = 0,
CHRRAM_1,
CHRRAM_2,
CHRRAM_4,
CHRRAM_6,
CHRRAM_8,
CHRRAM_16,
CHRRAM_32
};

/* NT mirroring */
enum
{
NT_X = 0,
NT_HORZ,	// not hardwired, horizontal at start
NT_VERT,	// not hardwired, vertical at start
NT_Y,
NT_1SCREEN,
NT_4SCR_2K,
NT_4SCR_4K
};

/* Boards */
enum
{
STD_NROM = 0,
STD_AMROM, STD_ANROM, STD_AN1ROM, STD_AOROM,
STD_BNROM, STD_CNROM, STD_CPROM,
STD_DEROM, STD_DE1ROM, STD_DRROM,
STD_EKROM, STD_ELROM, STD_ETROM, STD_EWROM,
STD_FJROM, STD_FKROM, STD_GNROM, STD_HKROM,
STD_JLROM, STD_JSROM, STD_MHROM, STD_NTBROM,
STD_PEEOROM, STD_PNROM,
STD_SAROM, STD_SBROM, STD_SCROM, STD_SEROM,
STD_SFROM, STD_SGROM, STD_SHROM, STD_SJROM,
STD_SKROM, STD_SLROM, STD_SNROM, STD_SOROM,
STD_SUROM, STD_SXROM,
STD_TEROM, STD_TBROM, STD_TFROM, STD_TGROM,
STD_TKROM, STD_TLROM, STD_TNROM, STD_TR1ROM,
STD_TSROM, STD_TVROM,
STD_TKSROM, STD_TLSROM, STD_TQROM,
STD_UN1ROM, STD_UNROM, STD_UOROM,
HVC_FAMBASIC, NES_BTR, NES_QJ, NES_WH, NES_ZZ,
/* Active Enterprises */
ACTENT_ACT52,
/* AGCI */
AGCI_47516, AGCI_50282,
/* AVE */
AVE_MB91, AVE_NINA01, AVE_NINA02, AVE_NINA03,
AVE_NINA06, AVE_NINA07, AVE_74161,
/* Bandai */
BANDAI_74161, BANDAI_FCG1, BANDAI_FCG2, BANDAI_JUMP2,
BANDAI_LZ93D50_A, BANDAI_LZ93D50_B, BANDAI_PT554,
/* Caltron */
CALTRON_6IN1,
/* Camerica */
CAMERICA_ALGN, CAMERICA_ALGQ,
CAMERICA_9093, CAMERICA_9096, CAMERICA_9097,
/* Color Dreams */
COLORDREAMS,
/* Dreamtech */
DREAMTECH,
/* Irem */
IREM_74161, IREM_G101, IREM_G101A, IREM_G101B,
IREM_HOLYDIV,
/* Jaleco */
JALECO_JF01, JALECO_JF02, JALECO_JF03, JALECO_JF04,
JALECO_JF05, JALECO_JF06, JALECO_JF07, JALECO_JF08,
JALECO_JF09, JALECO_JF10, JALECO_JF11, JALECO_JF12,
JALECO_JF13, JALECO_JF14, JALECO_JF15, JALECO_JF16,
JALECO_JF17, JALECO_JF18, JALECO_JF19, JALECO_JF20,
JALECO_JF21, JALECO_JF22, JALECO_JF23, JALECO_JF24,
JALECO_JF25, JALECO_JF26, JALECO_JF27, JALECO_JF28,
JALECO_JF29, JALECO_JF30, JALECO_JF31, JALECO_JF32,
JALECO_JF33, JALECO_JF34, JALECO_JF35, JALECO_JF36,
JALECO_JF37, JALECO_JF38, JALECO_JF39, JALECO_JF40,
JALECO_JF41,
/* Konami */
KONAMI_74139,
KONAMI_VRC1, KONAMI_VRC2, KONAMI_VRC3,
KONAMI_VRC4, KONAMI_VRC6, KONAMI_VRC7,
/* Namcot */
NAMCOT_163,
NAMCOT_3425, NAMCOT_3433, NAMCOT_3443, NAMCOT_3446,
/* NTDEC */
NTDEC_N715062,
/* Rex Soft */
REXSOFT_SL1632,
/* Sachen */
SACHEN_8259A, SACHEN_8259B, SACHEN_8259C,
SACHEN_8259D, SACHEN_SA0036, SACHEN_SA0037,
SACHEN_SA0161M, SACHEN_SA72007, SACHEN_SA72008,
SACHEN_TCA01, SACHEN_TCU01, SACHEN_TCU02, SACHEN_74LS374,
/* Sunsoft */
SUNSOFT_1, SUNSOFT_2B, SUNSOFT_3, SUNSOFT_4,
SUNSOFT_5B, SUNSOFT_FME7,
/* Taito */
TAITO_74139, TAITO_74161,
TAITO_TC0190FMC, TAITO_TC0190FMCP, TAITO_X1005, TAITO_X1017,
/* Tengen */
TENGEN_800002, TENGEN_800004, TENGEN_800008,
TENGEN_800030, TENGEN_800032, TENGEN_800037, TENGEN_800042,
/* TXC */
TXC_22211A,
/* Multigame Carts */
BMC_64IN1NR, BMC_190IN1, BMC_A65AS, BMC_GS2004, BMC_GS2013,
BMC_HIK8IN1, BMC_NOVELDIAMOND, BMC_S24IN1SC03, BMC_T262,
BMC_WS,
/* Unlicensed */
UNL_8237, UNL_CC21, UNL_AX5705, UNL_KOF97,
UNL_N625092, UNL_SC127, UNL_SMB2J, UNL_T230
};


/*************************************************************

    unif_list

    Supported UNIF boards and corresponding handlers

*************************************************************/

static const unif unif_list[] =
{
/*       UNIF                       LOW_W, LOW_R, MED_W, HIGH_W, PPU_latch, scanline CB, hblank CB     PRG  CHR  NVW  WRAM  CRAM       NMT    IDX*/
	{ "ACCLAIM-AOROM",              NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 16,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_AOROM},
	{ "ACCLAIM-MC-ACC",             NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "ACCLAIM-TLROM",              NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "AGCI-47516",                 NULL, NULL, NULL, mapper11_w, NULL, NULL, NULL,                 8,   16,   0,    0, CHRRAM_0,  NT_X,  AGCI_47516},
	{ "AGCI-50282",                 NULL, NULL, NULL, mapper144_w, NULL, NULL, NULL,                4,    8,   0,    0, CHRRAM_0,  NT_X,  AGCI_50282},
	{ "AVE-74*161",                 NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,  256,   8,    0, CHRRAM_0,  NT_X,  AVE_74161},
	{ "AVE-MB-91",                  mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL,               4,    8,   0,    0, CHRRAM_0,  NT_X,  AVE_MB91},
	{ "AVE-NINA-01",                NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL,         4,    8,   8,    0, CHRRAM_0,  NT_X,  AVE_NINA01},
	{ "AVE-NINA-02",                NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL,         4,    8,   8,    0, CHRRAM_0,  NT_X,  AVE_NINA02},
	{ "AVE-NINA-03",                mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL,               2,    4,   0,    0, CHRRAM_0,  NT_X,  AVE_NINA03},
	{ "AVE-NINA-06",                mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL,               4,    8,   0,    0, CHRRAM_0,  NT_X,  AVE_NINA06},
	{ "AVE-NINA-07",                NULL, NULL, NULL, mapper11_w, NULL, NULL, NULL,                 8,   16,   0,    0, CHRRAM_0,  NT_X,  AVE_NINA07},
	{ "BANDAI-74*161/161/32",       NULL, NULL, NULL, mapper70_w, NULL, NULL, NULL,                 8,   16,   0,    0, CHRRAM_0,  NT_X,  BANDAI_74161},
	{ "BANDAI-CNROM",               NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,    4,   0,    0, CHRRAM_0,  NT_X,  STD_CNROM},
	{ "BANDAI-FCG-1",               NULL, NULL, mapper153_m_w, mapper153_w, NULL,NULL,  bandai_irq, 16,  32,   0,    0, CHRRAM_0,  NT_VERT,  BANDAI_FCG1},
	{ "BANDAI-FCG-2",               NULL, NULL, mapper153_m_w, mapper153_w, NULL,NULL,  bandai_irq, 16,  32,   0,    0, CHRRAM_0,  NT_VERT,  BANDAI_FCG2},
	{ "BANDAI-GNROM",               NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL,       8,    4,   0,    0, CHRRAM_0,  NT_X,  STD_GNROM},
	{ "BANDAI-JUMP2",               NULL, NULL, mapper153_m_w, mapper153_w, NULL,NULL,  bandai_irq, 32,  0,   8,    0, CHRRAM_8,  NT_VERT,  BANDAI_JUMP2},
	{ "BANDAI-LZ93D50+24C01",       NULL, NULL, mapper16_m_w, mapper16_w, NULL,NULL,  bandai_irq, 16, 32,   0,    0, CHRRAM_0,  NT_VERT,  BANDAI_LZ93D50_A},
	{ "BANDAI-LZ93D50+24C02",       NULL, NULL, mapper16_m_w, mapper16_w, NULL,NULL,  bandai_irq, 16, 32,   0,    0, CHRRAM_0,  NT_VERT,  BANDAI_LZ93D50_B},
	{ "BANDAI-NROM-128",            NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "BANDAI-NROM-256",            NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "BANDAI-PT-554",              NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,    4,   0,    0, CHRRAM_0,  NT_X,  BANDAI_PT554},
//    { "BMC-13in1JY110",             [mentioned in FCEUMM source - we need more info]}
	{ "BMC-190IN1",                 NULL, NULL, NULL, bmc_190in1_w, NULL, NULL, NULL,        8,   8,   0,   0,  CHRRAM_0, NT_VERT,  BMC_190IN1},
//  { "BMC-42IN1RESETSWITCH",       [mapper 60 - unsupported]},
	{ "BMC-64IN1NOREPEAT",          bmc_64in1nr_l_w, NULL, NULL, bmc_64in1nr_w, NULL, NULL, NULL,        64,  64,   0,   0,  CHRRAM_0, NT_VERT,  BMC_64IN1NR},		//UNIF only!
//  { "BMC-70IN1",                  [mapper 236 - unsupported]},
//  { "BMC-70IN1B",                 [mapper 236 - unsupported]},
//  { "BMC-8157",                   [unif only!!]               32,   0,   0,   0,  CHRRAM_8, NT_VERT,  BMC_8157},
	{ "BMC-A65AS",                  NULL, NULL, NULL, bmc_a65as_w, NULL, NULL, NULL,        32,   0,   0,   0,  CHRRAM_8, NT_VERT,  BMC_A65AS},		//UNIF only!
//  { "BMC-BS-5",                   [unif only!!]                8,   8,   0,   0,  CHRRAM_0, NT_VERT,  BENSHENG_BS5},
//  { "BMC-D1038",                  [mapper 60 - unsupported]},
//  { "BMC-FK23C",                  [unif only!!]               64, 128,   0,   0,  CHRRAM_0, NT_X,  BMC_FK23C},
//  { "BMC-FK23CA",                 (same as above, but different reg init)},
//  { "BMC-GHOSTBUSTERS63IN1",      [unif only!!]              128,   0,   0,   0,  CHRRAM_8, NT_HORZ,  BMC_G63IN1},
//    { "BMC-GK-192",                 [mentioned in FCEUMM source - we need more info]}
	{ "BMC-GS-2004",                NULL, NULL, NULL, bmc_gs2004_w, NULL, NULL, NULL,        32,   0,   0,   0,  CHRRAM_8, NT_X,  BMC_GS2004},		//UNIF only!
	{ "BMC-GS-2013",                NULL, NULL, NULL, bmc_gs2013_w, NULL, NULL, NULL,        32,   0,   0,   0,  CHRRAM_8, NT_X,  BMC_GS2013},		//UNIF only!
	{ "BMC-NOVELDIAMOND9999999IN1", NULL, NULL, NULL, mapper54_w, NULL, NULL, NULL,           8,   8,   0,   0,  CHRRAM_0, NT_X,  BMC_NOVELDIAMOND},
	{ "BMC-SUPER24IN1SC03",         bmc_s24in1sc03_l_w, NULL, NULL, bmc_s24in1sc03_w, NULL, NULL, mapper4_irq,     256, 256,   8,   0,  CHRRAM_8, NT_X,  BMC_S24IN1SC03},
	{ "BMC-SUPERHIK8IN1",           NULL, NULL, mapper45_m_w, mapper4_w, NULL, NULL, mapper4_irq, 256,  256,   8,    0, CHRRAM_0,  NT_X,  BMC_HIK8IN1},
//  { "BMC-SUPERVISION16IN1",       [mapper 53 - unsupported]},
	{ "BMC-T-262",                  NULL, NULL, NULL, bmc_t262_w, NULL, NULL, NULL,        64,   0,   0,   0,  CHRRAM_8, NT_VERT,  BMC_T262},		//UNIF only!
	{ "BMC-WS",                     NULL, NULL, bmc_ws_m_w, NULL, NULL, NULL, NULL,         8,   8,   0,   0,  CHRRAM_0, NT_VERT,  BMC_WS},		//UNIF only!
//  { "BTL-MARIO1-MALEE2",          [mapper 55 - unsupported]},
	{ "CAMERICA-ALGN",              NULL, NULL, NULL, mapper71_w, NULL, NULL, NULL,               16,    0,   0,    0, CHRRAM_8,  NT_X,  CAMERICA_ALGN},
	{ "CAMERICA-ALGQ",              NULL, NULL, mapper232_w, mapper232_w, NULL, NULL, NULL,       16,    0,   0,    0, CHRRAM_8,  NT_X,  CAMERICA_ALGQ},
	{ "CAMERICA-BF9093",            NULL, NULL, NULL, mapper71_w, NULL, NULL, NULL,               16,    0,   0,    0, CHRRAM_8,  NT_X,  CAMERICA_9093},
	{ "CAMERICA-BF9096",            NULL, NULL, mapper232_w, mapper232_w, NULL, NULL, NULL,       16,    0,   0,    0, CHRRAM_8,  NT_X,  CAMERICA_9096},
	{ "CAMERICA-BF9097",            NULL, NULL, NULL, mapper71_w, NULL, NULL, NULL,                8,    0,   0,    0, CHRRAM_8,  NT_Y,  CAMERICA_9097},
	{ "CAMERICA-GAMEGENIE",         NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "COLORDREAMS-74*377",         NULL, NULL, NULL, mapper11_w, NULL, NULL, NULL,                8,   16,   0,    0, CHRRAM_0,  NT_X,  COLORDREAMS},
	{ "DREAMTECH01",                dreamtech_l_w, NULL, NULL, NULL, NULL, NULL, NULL,            16,    0,   0,   0,  CHRRAM_8, NT_X,  DREAMTECH},		//UNIF only!
	{ "HVC-AMROM",                  NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 8,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_AMROM},
	{ "HVC-AN1ROM",                 NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 4,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_AN1ROM},
	{ "HVC-ANROM",                  NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 8,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_ANROM},
	{ "HVC-AOROM",                  NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                16,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_AOROM},
	{ "HVC-BNROM",                  NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL,        8,    0,   0,    0, CHRRAM_8,  NT_X,  STD_BNROM},
	{ "HVC-CNROM",                  NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,    4,   0,    0, CHRRAM_0,  NT_X,  STD_CNROM},
	{ "HVC-CPROM",                  NULL, NULL, NULL, mapper13_w, NULL, NULL, NULL,        2,    0,   0,    0, CHRRAM_16,  NT_X,  STD_CPROM},
	{ "HVC-DE1ROM",                 NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "HVC-DEROM",                  NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             4,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DEROM},
	{ "HVC-DRROM",                  NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_4SCR_2K,  STD_DRROM},
	{ "HVC-EKROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,    8,   0, CHRRAM_0,  NT_X, STD_EKROM},
	{ "HVC-ELROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,    0,   0, CHRRAM_0,  NT_X, STD_ELROM},
	{ "HVC-ETROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,    8,   8, CHRRAM_0,  NT_X, STD_ETROM},
	{ "HVC-EWROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,   32,   0, CHRRAM_0,  NT_X, STD_EWROM},
	{ "HVC-FAMILYBASIC",            NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   2,    0, CHRRAM_0,  NT_X,  HVC_FAMBASIC},
	{ "HVC-FJROM",                  NULL, NULL, NULL, mapper10_w, mapper9_latch, NULL, NULL,             8,   16,    8,    0, CHRRAM_0,  NT_VERT,  STD_FJROM},
	{ "HVC-FKROM",                  NULL, NULL, NULL, mapper10_w, mapper9_latch, NULL, NULL,            16,   16,    8,    0, CHRRAM_0,  NT_VERT,  STD_FKROM},
	{ "HVC-GNROM",                  NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL,       8,    4,   0,    0, CHRRAM_0,  NT_X,  STD_GNROM},
	{ "HVC-HKROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_VERT,  STD_HKROM},
	{ "HVC-HROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-JLROM",                  NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq,             16,  32,    0,   0, CHRRAM_0,  NT_VERT,  STD_JLROM},
	{ "HVC-JSROM",                  NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq,             16,  32,    8,   0, CHRRAM_0,  NT_VERT,  STD_JSROM},
	{ "HVC-MHROM",                  NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL,       4,    4,   0,    0, CHRRAM_0,  NT_X,  STD_MHROM},
	{ "HVC-NROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-NROM-128",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-NROM-256",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-NTBROM",                 NULL, NULL, NULL, mapper68_w, NULL, NULL, NULL,                     16,   16,   8,    0, CHRRAM_0,  NT_VERT,  STD_NTBROM},
	{ "HVC-PEEOROM",                NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL, NULL,              8,   16,   0,    0, CHRRAM_0,  NT_VERT,  STD_PEEOROM},
	{ "HVC-PNROM",                  NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL, NULL,              8,   16,   0,    0, CHRRAM_0,  NT_VERT,  STD_PNROM},
	{ "HVC-RROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-RROM-128",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-SAROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,    8,   8,    0, CHRRAM_0,  NT_HORZ,  STD_SAROM},
	{ "HVC-SBROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SBROM},
	{ "HVC-SC1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SCROM},
	{ "HVC-SCROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SCROM},
	{ "HVC-SEROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                2,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SEROM},
	{ "HVC-SF1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SFROM},
	{ "HVC-SFROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SFROM},
	{ "HVC-SGROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    0,   0,    0, CHRRAM_8,  NT_HORZ,  STD_SGROM},
	{ "HVC-SH1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                2,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SHROM},
	{ "HVC-SHROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                2,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SHROM},
	{ "HVC-SJROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    8,   8,    0, CHRRAM_0,  NT_HORZ,  STD_SJROM},
	{ "HVC-SKROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   8,    0, CHRRAM_0,  NT_HORZ,  STD_SKROM},
	{ "HVC-SL1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "HVC-SL2ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "HVC-SL3ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "HVC-SLROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "HVC-SLRROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "HVC-SNROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    0,   8,    0, CHRRAM_8,  NT_HORZ,  STD_SNROM},
	{ "HVC-SOROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    0,   8,    8, CHRRAM_8,  NT_HORZ,  STD_SOROM},
	{ "HVC-SROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-STROM",                  NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "HVC-SUROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               32,    0,   8,    0, CHRRAM_8,  NT_HORZ,  STD_SUROM},
	{ "HVC-SXROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               32,    0,  32,    0, CHRRAM_8,  NT_HORZ,  STD_SXROM},
	{ "HVC-TBROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,        4,    8,   0,    0, CHRRAM_0,  NT_X,  STD_TBROM},
	{ "HVC-TEROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,        2,    8,   0,    0, CHRRAM_0,  NT_X,  STD_TEROM},
	{ "HVC-TFROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    8,   0,    0, CHRRAM_0,  NT_X,  STD_TFROM},
	{ "HVC-TGROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    0,   0,    0, CHRRAM_8,  NT_X,  STD_TGROM},
	{ "HVC-TKROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   8,    0, CHRRAM_0,  NT_X,  STD_TKROM},
	{ "HVC-TKSROM",                 NULL, NULL, NULL, mapper118_w, NULL, NULL, mapper4_irq,     32,   16,   8,    0, CHRRAM_0,  NT_Y,  STD_TKSROM},
	{ "HVC-TL1ROM",                 NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "HVC-TL2ROM",                 NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "HVC-TLROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "HVC-TLSROM",                 NULL, NULL, NULL, mapper118_w, NULL, NULL, mapper4_irq,     32,   16,   0,    0, CHRRAM_0,  NT_Y,  STD_TLSROM},
	{ "HVC-TNROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    0,   8,    0, CHRRAM_8,  NT_X,  STD_TNROM},
	{ "HVC-TQROM",                  NULL, NULL, NULL, mapper119_w, NULL, NULL, mapper4_irq,      8,    8,   0,    0, CHRRAM_8,  NT_VERT,  STD_TQROM},
	{ "HVC-TR1ROM",                 NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    8,   0,    0, CHRRAM_0,  NT_4SCR_4K,  STD_TR1ROM},
	{ "HVC-TSROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    8, CHRRAM_0,  NT_X,  STD_TSROM},
	{ "HVC-TVROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,        4,    8,   0,    0, CHRRAM_0,  NT_4SCR_4K,  STD_TVROM},
	{ "HVC-UN1ROM",                 NULL, NULL, NULL, mapper94_w, NULL, NULL, NULL,              8,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UN1ROM},
	{ "HVC-UNROM",                  NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,               8,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UNROM},
	{ "HVC-UOROM",                  NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,              16,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UOROM},
	{ "IREM-74*161/161/21/138",     NULL, NULL, NULL, mapper77_w, NULL, NULL, NULL,                 8,    4,   0,    0, CHRRAM_6,  NT_4SCR_2K,  IREM_74161},
	{ "IREM-BNROM",                 NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL,        8,    0,   0,    0, CHRRAM_8,  NT_X,  STD_BNROM},
	{ "IREM-G101",                  NULL, NULL, NULL, mapper32_w, NULL, NULL, NULL,                16,   16,   0,    0, CHRRAM_0,  NT_VERT,  IREM_G101},
	{ "IREM-G101-A",                NULL, NULL, NULL, mapper32_w, NULL, NULL, NULL,                16,   16,   0,    0, CHRRAM_0,  NT_VERT,  IREM_G101A},
	{ "IREM-G101-B",                NULL, NULL, NULL, mapper32_w, NULL, NULL, NULL,                16,   16,   0,    0, CHRRAM_0,  NT_Y,  IREM_G101B},
	// there are also variants with 8K NVWRAM
	{ "IREM-HOLYDIVER",             NULL, NULL, NULL, mapper78_w, NULL, NULL, NULL,                 8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  IREM_HOLYDIV},
	{ "IREM-NROM-128",              NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "IREM-NROM-256",              NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "IREM-UNROM",                 NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,               8,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UNROM},
	{ "JALECO-JF-01",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            1,    1,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF01},
	{ "JALECO-JF-02",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            1,    1,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF02},
	{ "JALECO-JF-03",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            1,    1,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF03},
	{ "JALECO-JF-04",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            1,    1,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF04},
	{ "JALECO-JF-05",               NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   1,    2,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF05},
	{ "JALECO-JF-06",               NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   1,    2,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF06},
	{ "JALECO-JF-07",               NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   2,    2,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF07},
	{ "JALECO-JF-08",               NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   2,    4,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF08},
	{ "JALECO-JF-09",               NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   2,    4,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF09},
	{ "JALECO-JF-10",               NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   2,    4,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF10},
	{ "JALECO-JF-11",               NULL, NULL, mapper_140_m_w, NULL, NULL, NULL, NULL,                 8,    4,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF11},
	{ "JALECO-JF-12",               NULL, NULL, mapper_140_m_w, NULL, NULL, NULL, NULL,                 8,    4,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF12},
	{ "JALECO-JF-13",               NULL, NULL, mapper86_m_w, NULL, NULL, NULL, NULL,                 8,    4,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF13},
	{ "JALECO-JF-14",               NULL, NULL, mapper_140_m_w, NULL, NULL, NULL, NULL,                 8,    4,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF14},
	{ "JALECO-JF-15",               NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,              16,    0,    0,   0, CHRRAM_8,  NT_X,  JALECO_JF15},
	{ "JALECO-JF-16",               NULL, NULL, NULL, mapper78_w, NULL, NULL, NULL,                 8,   16,   0,    0, CHRRAM_0,  NT_Y,  JALECO_JF16},
	{ "JALECO-JF-17",               NULL, NULL, NULL, mapper72_w, NULL, NULL, NULL,              8,   16,    0,   0, CHRRAM_0,  NT_X,  JALECO_JF17},
	{ "JALECO-JF-18",               NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,              16,    0,    0,   0, CHRRAM_8,  NT_X,  JALECO_JF18},
	{ "JALECO-JF-19",               NULL, NULL, NULL, mapper92_w, NULL, NULL, NULL,                    16,   16,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF19},
	{ "JALECO-JF-20",               NULL, NULL, NULL, mapper75_w, NULL, NULL, NULL,                     8,   16,   0,    0, CHRRAM_0,  NT_VERT,  JALECO_JF20},
	{ "JALECO-JF-21",               NULL, NULL, NULL, mapper92_w, NULL, NULL, NULL,                    16,   16,   0,    0, CHRRAM_0,  NT_X,  JALECO_JF21},
	{ "JALECO-JF-22",               NULL, NULL, NULL, mapper75_w, NULL, NULL, NULL,                     8,   16,   0,    0, CHRRAM_0,  NT_VERT,  JALECO_JF22},
	{ "JALECO-JF-23",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,           16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF23},
	{ "JALECO-JF-24",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF24},
	{ "JALECO-JF-25",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF25},
	{ "JALECO-JF-26",               NULL, NULL, NULL, mapper72_w, NULL, NULL, NULL,              8,   16,    0,   0, CHRRAM_0,  NT_X,  JALECO_JF26},
	{ "JALECO-JF-27",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   8,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF27},
	{ "JALECO-JF-28",               NULL, NULL, NULL, mapper72_w, NULL, NULL, NULL,              8,   16,    0,   0, CHRRAM_0,  NT_X,  JALECO_JF28},
	{ "JALECO-JF-29",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,           16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF29},
	{ "JALECO-JF-30",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF30},
	{ "JALECO-JF-31",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF31},
	{ "JALECO-JF-32",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF32},
	{ "JALECO-JF-33",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,           16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF33},
	{ "JALECO-JF-34",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF34},
	{ "JALECO-JF-35",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF35},
	{ "JALECO-JF-36",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF36},
	{ "JALECO-JF-37",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   32,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF37},
	{ "JALECO-JF-38",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF38},
	{ "JALECO-JF-39",               NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,               8,    0,    0,   0, CHRRAM_8,  NT_X,  JALECO_JF39},
	{ "JALECO-JF-40",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,            8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF40},
	{ "JALECO-JF-41",               NULL, NULL, NULL, mapper18_w, NULL, NULL, jaleco_irq,           16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  JALECO_JF41},
	{ "KONAMI-74*139/74",           NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   2,    4,   0,    0, CHRRAM_0,  NT_X,  KONAMI_74139},
	{ "KONAMI-CNROM",               NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,    4,   0,    0, CHRRAM_0,  NT_X,  STD_CNROM},
	{ "KONAMI-NROM-128",            NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
//    { "KONAMI-QTAI",                [mentioned in FCEUMM source - we need more info]}
	{ "KONAMI-SLROM",               NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "KONAMI-TLROM",               NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "KONAMI-UNROM",               NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,               8,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UNROM},
	{ "KONAMI-VRC-1",               NULL, NULL, NULL, mapper75_w, NULL, NULL, NULL,                     8,   16,   0,    0, CHRRAM_0,  NT_VERT,  KONAMI_VRC1},
	{ "KONAMI-VRC-2",               NULL, NULL, NULL, konami_vrc2a_w, NULL, NULL, NULL,                 8,   32,   0,    0, CHRRAM_0,  NT_VERT,  KONAMI_VRC2},
	// variant 2b (mapper 23) exists! how to distinguish?!? NULL, NULL, NULL, konami_vrc2b_w, NULL, NULL, konami_irq
	{ "KONAMI-VRC-3",               NULL, NULL, NULL, mapper73_w, NULL, NULL, konami_irq,               8,    0,   8,    0, CHRRAM_8,  NT_X,  KONAMI_VRC3},
	{ "KONAMI-VRC-4",               NULL, NULL, NULL, konami_vrc4a_w, NULL, NULL, konami_irq,           16,   32,   0,    0, CHRRAM_0,  NT_VERT,  KONAMI_VRC4},
	// variant 4b (mapper 23) exists! how to distinguish?!? NULL, NULL, NULL, konami_vrc4b_w, NULL, NULL, konami_irq
	// exist also variants with 2 & 8 NVWRAM!
	{ "KONAMI-VRC-6",               NULL, NULL, NULL, konami_vrc6a_w, NULL, NULL, konami_irq,          16,   32,   0,    0, CHRRAM_0,  NT_VERT,  KONAMI_VRC6},
	// variant 6b (mapper 26) exists! how to distinguish?!? NULL, NULL, NULL, konami_vrc6b_w, NULL, NULL, konami_irq
	// exist also variants with 8 NVWRAM!
	{ "KONAMI-VRC-7",               NULL, NULL, NULL, konami_vrc7_w, NULL, NULL, konami_irq,           32,   32,   0,    0, CHRRAM_0,  NT_VERT,  KONAMI_VRC7},
	// exist also variants with 8 NVWRAM!
	{ "MLT-ACTION52",               NULL, NULL, NULL, mapper228_w, NULL, NULL, NULL,                  128,   64,   0,    0, CHRRAM_0,  NT_VERT,  ACTENT_ACT52},
	{ "MLT-CALTRON6IN1",            NULL, NULL, mapper41_m_w, mapper41_w, NULL, NULL, NULL,            16,   16,   0,    0, CHRRAM_0,  NT_VERT,  CALTRON_6IN1},
//  { "MLT-MAXI15",                 [mapper 234 - unsupported]},
	{ "NAMCOT-163",                 mapper19_l_w, mapper19_l_r, NULL, mapper19_w, NULL, NULL, namcot_irq,32, 32,   0,    0, CHRRAM_0,  NT_X,  NAMCOT_163},
	// exist also variants with 8 NVWRAM
	{ "NAMCOT-3301",                NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NAMCOT-3302",                NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NAMCOT-3303",                NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NAMCOT-3305",                NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NAMCOT-3311",                NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NAMCOT-3401",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3405",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3406",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3407",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3411",                NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NAMCOT-3413",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3414",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3415",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3416",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3417",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NAMCOT-3425",                NULL, NULL, NULL, mapper95_w, NULL, NULL, NULL,                    8,   16,   0,    0, CHRRAM_0,  NT_Y,  NAMCOT_3425},
	{ "NAMCOT-3433",                NULL, NULL, NULL, mapper88_w, NULL, NULL, NULL,                    8,   16,   0,    0, CHRRAM_0,  NT_HORZ,  NAMCOT_3433},
	{ "NAMCOT-3443",                NULL, NULL, NULL, mapper88_w, NULL, NULL, NULL,                    8,   16,   0,    0, CHRRAM_0,  NT_VERT,  NAMCOT_3443},
	{ "NAMCOT-3446",                NULL, NULL, NULL, mapper76_w, NULL, NULL, NULL,                    8,   16,   0,    0, CHRRAM_0,  NT_X,  NAMCOT_3446},
	{ "NAMCOT-3451",                NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NES-AMROM",                  NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 8,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_AMROM},
	{ "NES-AN1ROM",                 NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 4,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_AN1ROM},
	{ "NES-ANROM",                  NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 8,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_ANROM},
	{ "NES-AOROM",                  NULL, NULL, NULL, mapper7_w, NULL, NULL, NULL,                 16,    0,   0,    0, CHRRAM_8,  NT_Y,  STD_AOROM},
	{ "NES-B4",                     NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "NES-BNROM",                  NULL, NULL, mapper34_m_w, mapper34_w, NULL, NULL, NULL,        8,    0,   0,    0, CHRRAM_8,  NT_X,  STD_BNROM},
	{ "NES-BTR",                    NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq,             32,  32,    8,   0, CHRRAM_0,  NT_VERT,  NES_BTR},
	{ "NES-CNROM",                  NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,    4,   0,    0, CHRRAM_0,  NT_X,  STD_CNROM},
	{ "NES-CPROM",                  NULL, NULL, NULL, mapper13_w, NULL, NULL, NULL,        2,    0,   0,    0, CHRRAM_16,  NT_X,  STD_CPROM},
	{ "NES-DE1ROM",                 NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DE1ROM},
	{ "NES-DEROM",                  NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             4,   8,    0,    0, CHRRAM_0,  NT_X,  STD_DEROM},
	{ "NES-DRROM",                  NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_4SCR_2K,  STD_DRROM},
	{ "NES-EKROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,    8,   0, CHRRAM_0,  NT_X, STD_EKROM},
	{ "NES-ELROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,    0,   0, CHRRAM_0,  NT_X, STD_ELROM},
	{ "NES-ETROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,    8,   8, CHRRAM_0,  NT_X, STD_ETROM},
//  { "NES-EVENT",                  [mapper 105 - unsupported]},
	{ "NES-EWROM",                  mapper5_l_w, mapper5_l_r, NULL, mapper5_w, NULL, NULL, mapper5_irq,   32,  64,   32,   0, CHRRAM_0,  NT_X, STD_EWROM},
	{ "NES-FJROM",                  NULL, NULL, NULL, mapper10_w, mapper9_latch, NULL, NULL,             8,   16,    8,    0, CHRRAM_0,  NT_VERT,  STD_FJROM},
	{ "NES-FKROM",                  NULL, NULL, NULL, mapper10_w, mapper9_latch, NULL, NULL,            16,   16,    8,    0, CHRRAM_0,  NT_VERT,  STD_FKROM},
	{ "NES-GNROM",                  NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL,       8,    4,   0,    0, CHRRAM_0,  NT_X,  STD_GNROM},
	{ "NES-HKROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_VERT,  STD_HKROM},
	{ "NES-HROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-JLROM",                  NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq,             16,  32,    0,   0, CHRRAM_0,  NT_VERT,  STD_JLROM},
	{ "NES-JSROM",                  NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq,             16,  32,    8,   0, CHRRAM_0,  NT_VERT,  STD_JSROM},
	{ "NES-MHROM",                  NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL,       4,    4,   0,    0, CHRRAM_0,  NT_X,  STD_MHROM},
	{ "NES-NROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-NROM-128",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-NROM-256",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-NTBROM",                 NULL, NULL, NULL, mapper68_w, NULL, NULL, NULL,                     16,   16,   8,    0, CHRRAM_0,  NT_VERT,  STD_NTBROM},
	{ "NES-PEEOROM",                NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL, NULL,              8,   16,   0,    0, CHRRAM_0,  NT_VERT,  STD_PEEOROM},
	{ "NES-PNROM",                  NULL, NULL, NULL, mapper9_w, mapper9_latch, NULL, NULL,              8,   16,   0,    0, CHRRAM_0,  NT_VERT,  STD_PNROM},
	{ "NES-QJ",                     NULL, NULL, mapper47_m_w, mapper4_w, NULL, NULL, mapper4_irq, 16,   32,  0,    0, CHRRAM_0,  NT_VERT,  NES_QJ},
	{ "NES-RROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                     2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-RROM-128",               NULL, NULL, NULL, NULL, NULL, NULL, NULL,                     2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-SAROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,    8,   8,    0, CHRRAM_0,  NT_HORZ,  STD_SAROM},
	{ "NES-SBROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SBROM},
	{ "NES-SC1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SCROM},
	{ "NES-SCROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                4,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SCROM},
	{ "NES-SEROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                2,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SEROM},
	{ "NES-SF1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SFROM},
	{ "NES-SFROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    8,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SFROM},
	{ "NES-SGROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    0,   0,    0, CHRRAM_8,  NT_HORZ,  STD_SGROM},
	{ "NES-SH1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                2,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SHROM},
	{ "NES-SHROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,                2,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SHROM},
	{ "NES-SJROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    8,   8,    0, CHRRAM_0,  NT_HORZ,  STD_SJROM},
	{ "NES-SKROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   8,    0, CHRRAM_0,  NT_HORZ,  STD_SKROM},
	{ "NES-SL1ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "NES-SL2ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "NES-SL3ROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "NES-SLROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "NES-SLRROM",                 NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,   16,   0,    0, CHRRAM_0,  NT_HORZ,  STD_SLROM},
	{ "NES-SNROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    0,   8,    0, CHRRAM_8,  NT_HORZ,  STD_SNROM},
	{ "NES-SOROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    0,   8,    8, CHRRAM_8,  NT_HORZ,  STD_SOROM},
	{ "NES-SROM",                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-STROM",                  NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "NES-SUROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               32,    0,   8,    0, CHRRAM_8,  NT_HORZ,  STD_SUROM},
	{ "NES-SXROM",                  NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               32,    0,  32,    0, CHRRAM_8,  NT_HORZ,  STD_SXROM},
	{ "NES-TBROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,        4,    8,   0,    0, CHRRAM_0,  NT_X,  STD_TBROM},
	{ "NES-TEROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,        2,    8,   0,    0, CHRRAM_0,  NT_X,  STD_TEROM},
	{ "NES-TFROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    8,   0,    0, CHRRAM_0,  NT_X,  STD_TFROM},
	{ "NES-TGROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    0,   0,    0, CHRRAM_8,  NT_X,  STD_TGROM},
	{ "NES-TKROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   8,    0, CHRRAM_0,  NT_X,  STD_TKROM},
	{ "NES-TKSROM",                 NULL, NULL, NULL, mapper118_w, NULL, NULL, mapper4_irq,     32,   16,   8,    0, CHRRAM_0,  NT_Y,  STD_TKSROM},
	{ "NES-TL1ROM",                 NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "NES-TL2ROM",                 NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "NES-TLROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    0, CHRRAM_0,  NT_X,  STD_TLROM},
	{ "NES-TLSROM",                 NULL, NULL, NULL, mapper118_w, NULL, NULL, mapper4_irq,     32,   16,   0,    0, CHRRAM_0,  NT_Y,  STD_TLSROM},
	{ "NES-TNROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    0,   8,    0, CHRRAM_8,  NT_X,  STD_TNROM},
	{ "NES-TQROM",                  NULL, NULL, NULL, mapper119_w, NULL, NULL, mapper4_irq,      8,    8,   0,    0, CHRRAM_8,  NT_VERT,  STD_TQROM},
	{ "NES-TR1ROM",                 NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,    8,   0,    0, CHRRAM_0,  NT_4SCR_4K,  STD_TR1ROM},
	{ "NES-TSROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,       32,   32,   0,    8, CHRRAM_0,  NT_X,  STD_TSROM},
	{ "NES-TVROM",                  NULL, NULL, NULL, mapper4_w, NULL, NULL, mapper4_irq,        4,    8,   0,    0, CHRRAM_0,  NT_4SCR_4K,  STD_TVROM},
	{ "NES-UN1ROM",                 NULL, NULL, NULL, mapper94_w, NULL, NULL, NULL,              8,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UN1ROM},
	{ "NES-UNROM",                  NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,               8,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UNROM},
	{ "NES-UOROM",                  NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,              16,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UOROM},
	{ "NES-WH",                     NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               8,    8,   0,    0, CHRRAM_0,  NT_HORZ,  NES_WH},
	{ "NTDEC-N715062",              NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,                       2,   4,    0,   0, CHRRAM_0,  NT_X,  NTDEC_N715062},
	{ "PAL-MH",                     NULL, NULL, NULL, mapper66_w, NULL, NULL, NULL,       4,    4,   0,    0, CHRRAM_0,  NT_X,  STD_MHROM},
	{ "PAL-ZZ",                     NULL, NULL, mapper37_m_w, mapper4_w, NULL, NULL, mapper4_irq,       16,  32,   0,    0, CHRRAM_0,  NT_VERT,  NES_ZZ},
	{ "SACHEN-8259A",               mapper141_l_w, NULL, mapper141_m_w, NULL, NULL, NULL, NULL,         16,  32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259A},
	{ "SACHEN-8259B",               mapper138_l_w, NULL, mapper138_m_w, NULL, NULL, NULL, NULL,         16,  32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259B},
	{ "SACHEN-8259C",               mapper139_l_w, NULL, mapper139_m_w, NULL, NULL, NULL, NULL,         16,  32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259C},
	{ "SACHEN-8259D",               mapper137_l_w, NULL, mapper137_m_w, NULL, NULL, NULL, NULL,         16,  32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259D},
	{ "SACHEN-CNROM",               NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,    4,   0,    0, CHRRAM_0,  NT_X,  STD_CNROM},
	{ "SETA-NROM-128",              NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "SUNSOFT-1",                  NULL, NULL, mapper184_m_w, NULL, NULL, NULL, NULL,                   2,   4,    0,   0, CHRRAM_0,  NT_X,  SUNSOFT_1},
	{ "SUNSOFT-2",                  NULL, NULL, NULL, mapper89_w, NULL, NULL, NULL,                      8,  16,    0,   0, CHRRAM_0,  NT_Y,  SUNSOFT_2B},
	// 2a variant (mapper 93)?
	{ "SUNSOFT-3",                  NULL, NULL, NULL, mapper67_w, NULL, NULL, mapper67_irq,              8,  16,    0,   0, CHRRAM_0,  NT_VERT,  SUNSOFT_3},
	{ "SUNSOFT-4",                  NULL, NULL, NULL, mapper68_w, NULL, NULL, NULL,                      8,  32,    0,   0, CHRRAM_0,  NT_VERT,  SUNSOFT_4},
	// variant with 8 NVWRAM
	{ "SUNSOFT-5B",                 NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq,             16,  32,    0,   0, CHRRAM_0,  NT_VERT,  SUNSOFT_5B},
	// variant with 8 NVWRAM
	{ "SUNSOFT-FME-7",              NULL, NULL, NULL, mapper69_w, NULL, NULL, mapper69_irq,             16,  32,    0,   0, CHRRAM_0,  NT_VERT,  SUNSOFT_FME7},
	// variant with 8 NVWRAM
	{ "SUNSOFT-NROM-256",           NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "TAITO-74*139/74",            NULL, NULL, mapper87_m_w, NULL, NULL, NULL, NULL,                   2,    4,   0,    0, CHRRAM_0,  NT_X,  TAITO_74139},
	{ "TAITO-74*161/161/32",        NULL, NULL, NULL, mapper70_w, NULL, NULL, NULL,                     8,   16,   0,    0, CHRRAM_0,  NT_X,  TAITO_74161},
	{ "TAITO-CNROM",                NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,        2,    4,   0,    0, CHRRAM_0,  NT_X,  STD_CNROM},
	{ "TAITO-NROM-128",             NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "TAITO-NROM-256",             NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "TAITO-TC0190FMC",            NULL, NULL, NULL, mapper33_w, NULL, NULL, NULL,                    16,  32,    0,   0, CHRRAM_0,  NT_VERT,  TAITO_TC0190FMC},
	{ "TAITO-TC0190FMC+PAL16R4",    NULL, NULL, NULL, mapper48_w, NULL, NULL, mapper4_irq,             16,  32,    0,   0, CHRRAM_0,  NT_VERT,  TAITO_TC0190FMCP},
	{ "TAITO-UNROM",                NULL, NULL, NULL, mapper2_w, NULL, NULL, NULL,               8,    0,    0,   0, CHRRAM_8,  NT_X,  STD_UNROM},
	{ "TAITO-X1-005",               NULL, NULL, mapper80_m_w, NULL, NULL, NULL, NULL,                  16,  32,    0,   0, CHRRAM_0,  NT_X,  TAITO_X1005},
	{ "TAITO-X1-017",               NULL, NULL, mapper82_m_w, NULL, NULL, NULL, NULL,                  16,  32,    0,   0, CHRRAM_0,  NT_HORZ,  TAITO_X1017},
	{ "TENGEN-800002",              NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             4,   8,    0,    0, CHRRAM_0,  NT_X,  TENGEN_800002},
	{ "TENGEN-800003",              NULL, NULL, NULL, NULL, NULL, NULL, NULL,                            2,    1,   0,    0, CHRRAM_0,  NT_X,  STD_NROM},
	{ "TENGEN-800004",              NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_4SCR_2K,  TENGEN_800004},
	{ "TENGEN-800008",              NULL, NULL, NULL, mapper3_w, NULL, NULL, NULL,                      4,   8,    0,    0, CHRRAM_0,  NT_X,  TENGEN_800008},
	{ "TENGEN-800030",              NULL, NULL, NULL, mapper206_w, NULL, NULL, mapper4_irq,             8,   8,    0,    0, CHRRAM_0,  NT_X,  TENGEN_800030},
	{ "TENGEN-800032",              NULL, NULL, NULL, mapper64_w, NULL, NULL, mapper64_irq,             8,  16,    0,    0, CHRRAM_0,  NT_VERT,  TENGEN_800032},
	{ "TENGEN-800037",              NULL, NULL, NULL, mapper158_w, NULL, NULL, mapper64_irq,            8,  16,    0,    0, CHRRAM_0,  NT_Y,  TENGEN_800037},
	{ "TENGEN-800042",              NULL, NULL, NULL, mapper68_w, NULL, NULL, NULL,                     8,  32,    0,    0, CHRRAM_0,  NT_VERT,  TENGEN_800042},
	{ "UNL-22211",                  mapper132_l_w, mapper132_l_r, NULL, mapper132_w, NULL, NULL, NULL,  4,   4,    0,    0, CHRRAM_0,  NT_X,  TXC_22211A},
	// mapper 172 & 173 are variant of this one... no UNIF?
//  { "UNL-3D-BLOCK",               [mentioned in FCEUMM source - we need more info]}
//  { "UNL-603-5052",               [mapper 238 - unsupported]},
//    { "UNL-8157",                   [mentioned in FCEUMM source - we need more info]}
	{ "UNL-8237",                   unl_8237_l_w, NULL, NULL, unl_8237_w, NULL, NULL, mapper4_irq,     32, 64,   0,   0,  CHRRAM_0, NT_X,  UNL_8237},
//  { "UNL-A9746",                  [mapper 219 - unsupported]},
	{ "UNL-AX5705",                 NULL, NULL, NULL, unl_ax5705_w, NULL, NULL, NULL,                   8, 32,   0,   0,  CHRRAM_0, NT_X,  UNL_AX5705},
	{ "UNL-CC-21",                  NULL, NULL, NULL, unl_cc21_w, NULL, NULL, NULL,                     2,  2,   0,   0,  CHRRAM_0, NT_Y,  UNL_CC21},
//    { "UNL-C-N22M",                 [mentioned in FCEUMM source - we need more info]}
//    { "UNL-DANCE",                  [mentioned in FCEUMM source - we need more info]}
//  { "UNL-DRIPGAME",               [by Quietust - we need more info]}
//  { "UNL-EDU2000",                [unif only!!]              64,  0,  32,   0,  CHRRAM_8, NT_Y,  UNL_EDU2K},
//  { "UNL-H2288",                  [mapper 123 - unsupported]},
	{ "UNL-KOF97",                  NULL, NULL, NULL, unl_kof97_w, NULL, NULL, mapper4_irq,       32, 32,   0,   0,  CHRRAM_0, NT_X,  UNL_KOF97},
//  { "UNL-KS7032",                 [mapper 142 - unsupported]},
//    { "UNL-KS7017",                 [mentioned in FCEUMM source - we need more info]}
	{ "UNL-N625092",                NULL, NULL, NULL, mapper221_w, NULL, NULL, NULL,                   64,    1,   0,    0, CHRRAM_0,  NT_VERT,  UNL_N625092},
//  { "UNL-PEC-586",                [mentioned in FCEUMM source - we need more info]},
	{ "UNL-SA-002",                 mapper136_l_w, mapper136_l_r, NULL, NULL, NULL, NULL, NULL,         2,    2,   0,    0, CHRRAM_0,  NT_X,  SACHEN_TCU02},
//  { "UNL-SA-009",                 [mentioned in FCEUMM source - we need more info]},
	{ "UNL-SA-0036",                NULL, NULL, NULL, mapper149_w, NULL, NULL, NULL,                    2,    2,   0,    0, CHRRAM_0,  NT_X,  SACHEN_SA0036},
	{ "UNL-SA-0037",                NULL, NULL, NULL, mapper148_w, NULL, NULL, NULL,                    4,    8,   0,    0, CHRRAM_0,  NT_X,  SACHEN_SA0037},
	{ "UNL-SA-016-1M",              mapper79_l_w, NULL, NULL, NULL, NULL, NULL, NULL,                   4,    8,   0,    0, CHRRAM_0,  NT_X,  SACHEN_SA0161M},	// actually this is Mapper 146, but works like 79!
	{ "UNL-SA-72007",               mapper145_l_w, NULL, NULL, NULL, NULL, NULL, NULL,                  1,    2,   0,    0, CHRRAM_0,  NT_X,  SACHEN_SA72007},
	{ "UNL-SA-72008",               mapper133_l_w, NULL, mapper133_m_w, NULL, NULL, NULL, NULL,         4,    4,   0,    0, CHRRAM_0,  NT_X,  SACHEN_SA72008},
	{ "UNL-SA-NROM",                NULL, mapper143_l_r, NULL, NULL, NULL, NULL, NULL,                  2,    1,   0,    0, CHRRAM_0,  NT_X,  SACHEN_TCA01},
	{ "UNL-SACHEN-74LS374N",        mapper150_l_w, mapper150_l_r, mapper150_m_w, NULL, NULL, NULL, NULL,4,   16,   0,    0, CHRRAM_0,  NT_X,  SACHEN_74LS374},
	// mapper 243 variant exists! how to distinguish?!?  mapper243_l_w, NULL, NULL, NULL, NULL, NULL, NULL (also uses NT_VERT!)
//  { "UNL-SACHEN-74LS374NA",       [is it mapper 243?]},
	{ "UNL-SACHEN-8259A",           mapper141_l_w, NULL, mapper141_m_w, NULL, NULL, NULL, NULL,        16,   32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259A},
	{ "UNL-SACHEN-8259B",           mapper138_l_w, NULL, mapper138_m_w, NULL, NULL, NULL, NULL,        16,   32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259B},
	{ "UNL-SACHEN-8259C",           mapper139_l_w, NULL, mapper139_m_w, NULL, NULL, NULL, NULL,        16,   32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259C},
	{ "UNL-SACHEN-8259D",           mapper137_l_w, NULL, mapper137_m_w, NULL, NULL, NULL, NULL,        16,   32,   0,    0, CHRRAM_0,  NT_X,  SACHEN_8259D},
//  { "UNL-SHERO",                  [unif only!!]               32,  64,   0,   0,  CHRRAM_8, NT_4SCR_2K,  SACHEN_SHERO},
	{ "UNL-SC-127",                 NULL, NULL, NULL, mapper35_w, NULL, NULL, mapper35_irq,            32,   64,   8,    0, CHRRAM_0,  NT_X,  UNL_SC127},
	{ "UNL-SL1632",                 NULL, NULL, NULL, mapper14_w, NULL, NULL, mapper4_irq,             16,   64,   0,    0, CHRRAM_0,  NT_VERT,  REXSOFT_SL1632},
	{ "UNL-SMB2J",                  NULL, NULL, NULL, mapper43_w, NULL, NULL, NULL,                     8,    1,   8,    8, CHRRAM_0,  NT_X,  UNL_SMB2J},
	{ "UNL-T-230",                  NULL, NULL, NULL, unl_t230_w, NULL, NULL, konami_irq,              16,    0,   0,    0, CHRRAM_8,  NT_VERT,  UNL_T230},
	{ "UNL-TC-U01-1.5M",            mapper147_l_w, NULL, mapper147_m_w, mapper147_w, NULL, NULL, NULL,  8,   16,   0,    0, CHRRAM_0,  NT_X,  SACHEN_TCU01},
	// mapper 136 variant exists! how to distinguish?!?
//  { "UNL-TEK90",                  [mappers 90, 209, 211 - unsupported]},
//  { "UNL-TF1201",                 [unif only!!]               16,  32,   0,   0,  CHRRAM_0, NT_VERT,  UNL_TF1201},
	{ "VIRGIN-SNROM",               NULL, NULL, NULL, mapper1_w, NULL, NULL, NULL,               16,    0,   8,    0, CHRRAM_8,  NT_HORZ,  STD_SNROM}
};

const unif *nes_unif_lookup( const char *board )
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(unif_list); i++)
	{
		if (!mame_stricmp(unif_list[i].board, board))
			return &unif_list[i];
	}
	return NULL;
}
