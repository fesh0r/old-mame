/***********************************************************************
 IGS28 + IGS025 PGM protection emulation

 these are simulations of the IGS 012 and 025 protection combination
 used on the following PGM games

 Oriental Legend Super

 ----

 IGS28 is some kind of encrypted DMA device, works with data in an
 external ROM, more advaned version of IGS022?

 IGS025 is some kind of state machine, bitswaps etc.

 Simulation is incomplete

 ***********************************************************************/

#include "emu.h"
#include "includes/pgm.h"

UINT32 pgm_028_025_state::olds_prot_addr( UINT16 addr )
{
	UINT32 mode = addr & 0xff;
	UINT32 offset = addr >> 8;
	UINT32 realaddr;

	switch(mode)
	{
		case 0x0:
		case 0x5:
		case 0xa:
			realaddr = 0x402a00 + (offset << 2);
			break;

		case 0x2:
		case 0x8:
			realaddr = 0x402e00 + (offset << 2);
			break;

		case 0x1:
			realaddr = 0x40307e;
			break;

		case 0x3:
			realaddr = 0x403090;
			break;

		case 0x4:
			realaddr = 0x40309a;
			break;

		case 0x6:
			realaddr = 0x4030a4;
			break;

		case 0x7:
			realaddr = 0x403000;
			break;

		case 0x9:
			realaddr = 0x40306e;
			break;

		default:
			realaddr = 0;
	}
	return realaddr;
}

UINT32 pgm_028_025_state::olds_read_reg(UINT16 addr )
{
	UINT32 protaddr = (olds_prot_addr(addr) - 0x400000) / 2;
	return m_sharedprotram[protaddr] << 16 | m_sharedprotram[protaddr + 1];
}

void pgm_028_025_state::olds_write_reg( UINT16 addr, UINT32 val )
{
	m_sharedprotram[(olds_prot_addr(addr) - 0x400000) / 2]     = val >> 16;
	m_sharedprotram[(olds_prot_addr(addr) - 0x400000) / 2 + 1] = val & 0xffff;
}

MACHINE_RESET_MEMBER(pgm_028_025_state,olds)
{
	UINT16 *mem16 = (UINT16 *)memregion("user2")->base();
	int i;

	MACHINE_RESET_CALL_MEMBER(pgm);

	/* populate shared protection ram with data read from pcb .. */
	for (i = 0; i < 0x4000 / 2; i++)
	{
		m_sharedprotram[i] = mem16[i];
	}

	//ROM:004008B4                 .word 0xFBA5
	for(i = 0; i < 0x4000 / 2; i++)
	{
		if (m_sharedprotram[i] == (0xffff - i))
			m_sharedprotram[i] = 0x4e75;
	}
}

READ16_MEMBER(pgm_028_025_state::olds_r )
{
	UINT16 res = 0;

	if (offset == 1)
	{
		if (m_kb_cmd == 1)
			res = m_kb_reg & 0x7f;
		if (m_kb_cmd == 2)
			res = m_olds_bs | 0x80;
		if (m_kb_cmd == 3)
			res = m_olds_cmd3;
		else if (m_kb_cmd == 5)
		{
			UINT32 protvalue = 0x900000 | ioport("Region")->read(); // region from protection device.
			res = (protvalue >> (8 * (m_kb_ptr - 1))) & 0xff; // includes region 1 = taiwan , 2 = china, 3 = japan (title = orlegend special), 4 = korea, 5 = hongkong, 6 = world

		}
	}
	logerror("%06X: ASIC25 R CMD %X  VAL %X\n", space.device().safe_pc(), m_kb_cmd, res);
	return res;
}

WRITE16_MEMBER(pgm_028_025_state::olds_w )
{
	if (offset == 0)
		m_kb_cmd = data;
	else //offset==2
	{
		logerror("%06X: ASIC25 W CMD %X  VAL %X\n",space.device().safe_pc(), m_kb_cmd, data);
		if (m_kb_cmd == 0)
			m_kb_reg = data;
		else if(m_kb_cmd == 2)   //a bitswap=
		{
			int reg = 0;
			if (data & 0x01)
				reg |= 0x40;
			if (data & 0x02)
				reg |= 0x80;
			if (data & 0x04)
				reg |= 0x20;
			if (data & 0x08)
				reg |= 0x10;
			m_olds_bs = reg;
		}
		else if (m_kb_cmd == 3)
		{
			UINT16 cmd = m_sharedprotram[0x3026 / 2];
			switch (cmd)
			{
				case 0x11:
				case 0x12:
						break;
				case 0x64:
					{
						UINT16 cmd0 = m_sharedprotram[0x3082 / 2];
						UINT16 val0 = m_sharedprotram[0x3050 / 2];   //CMD_FORMAT
						{
							if ((cmd0 & 0xff) == 0x2)
								olds_write_reg(val0, olds_read_reg(val0) + 0x10000);
						}
						break;
					}

				default:
						break;
			}
			m_olds_cmd3 = ((data >> 4) + 1) & 0x3;
		}
		else if (m_kb_cmd == 4)
			m_kb_ptr = data;
		else if(m_kb_cmd == 0x20)
			m_kb_ptr++;
	}
}

READ16_MEMBER(pgm_028_025_state::olds_prot_swap_r )
{
	if (space.device().safe_pc() < 0x100000)        //bios
		return m_mainram[0x178f4 / 2];
	else                        //game
		return m_mainram[0x178d8 / 2];

}

DRIVER_INIT_MEMBER(pgm_028_025_state,olds)
{
	pgm_basic_init();

	machine().device("maincpu")->memory().space(AS_PROGRAM).install_readwrite_handler(0xdcb400, 0xdcb403, read16_delegate(FUNC(pgm_028_025_state::olds_r),this), write16_delegate(FUNC(pgm_028_025_state::olds_w),this));
	machine().device("maincpu")->memory().space(AS_PROGRAM).install_read_handler(0x8178f4, 0x8178f5, read16_delegate(FUNC(pgm_028_025_state::olds_prot_swap_r),this));

	m_kb_cmd = 0;
	m_kb_reg = 0;
	m_kb_ptr = 0;
	m_olds_bs = 0;
	m_olds_cmd3 = 0;

	save_item(NAME(m_kb_cmd));
	save_item(NAME(m_kb_reg));
	save_item(NAME(m_kb_ptr));
	save_item(NAME(m_olds_bs));
	save_item(NAME(m_olds_cmd3));
}

static ADDRESS_MAP_START( olds_mem, AS_PROGRAM, 16, pgm_028_025_state )
	AM_IMPORT_FROM(pgm_mem)
	AM_RANGE(0x100000, 0x3fffff) AM_ROMBANK("bank1") /* Game ROM */
	AM_RANGE(0x400000, 0x403fff) AM_RAM AM_SHARE("sharedprotram") // Shared with protection device
ADDRESS_MAP_END


MACHINE_CONFIG_START( pgm_028_025_ol, pgm_028_025_state )
	MCFG_FRAGMENT_ADD(pgmbase)

	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(olds_mem)

	MCFG_MACHINE_RESET_OVERRIDE(pgm_028_025_state,olds)
MACHINE_CONFIG_END


INPUT_PORTS_START( olds )
	PORT_INCLUDE ( pgm )

	PORT_MODIFY("Region")   /* Region - supplied by protection device */
	PORT_CONFNAME( 0x000f, 0x0006, DEF_STR( Region ) )
	/* includes the following regions:
	1 = taiwan, 2 = china, 3 = japan (title = orlegend special),
	4 = korea, 5 = hong kong, 6 = world */
	PORT_CONFSETTING(      0x0001, DEF_STR( Taiwan ) )
	PORT_CONFSETTING(      0x0002, DEF_STR( China ) )
	PORT_CONFSETTING(      0x0003, DEF_STR( Japan ) )
	PORT_CONFSETTING(      0x0004, DEF_STR( Korea ) )
	PORT_CONFSETTING(      0x0005, DEF_STR( Hong_Kong ) )
	PORT_CONFSETTING(      0x0006, DEF_STR( World ) )
INPUT_PORTS_END
