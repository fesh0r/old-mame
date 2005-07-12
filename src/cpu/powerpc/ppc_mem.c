INLINE UINT8 READ8(UINT32 a)
{
	return ppc.read8(a);
}

INLINE UINT16 READ16(UINT32 a)
{
	if( a & 0x1 )
		return ppc.read16_unaligned(a);
	else
		return ppc.read16(a);
}

INLINE UINT32 READ32(UINT32 a)
{
	if( a & 0x3 )
		return ppc.read32_unaligned(a);
	else
		return ppc.read32(a);
}

INLINE UINT64 READ64(UINT32 a)
{
	if( a & 0x7 )
		return ppc.read64_unaligned(a);
	else
		return ppc.read64(a);
}

INLINE void WRITE8(UINT32 a, UINT8 d)
{
	ppc.write8(a, d);
}

INLINE void WRITE16(UINT32 a, UINT16 d)
{
	if( a & 0x1 )
		ppc.write16_unaligned(a, d);
	else
		ppc.write16(a, d);
}

INLINE void WRITE32(UINT32 a, UINT32 d)
{
	if( ppc.reserved ) {
		if( a == ppc.reserved_address ) {
			ppc.reserved = 0;
		}
	}

	if( a & 0x3 )
		ppc.write32_unaligned(a, d);
	else
		ppc.write32(a, d);
}

INLINE void WRITE64(UINT32 a, UINT64 d)
{
	if( a & 0x7 )
		ppc.write64_unaligned(a, d);
	else
		ppc.write64(a, d);
}

/***********************************************************************/

static UINT16 ppc_read16_unaligned(UINT32 a)
{
	return ((UINT16)ppc.read8(a+0) << 8) | ((UINT16)ppc.read8(a+1) << 0);
}

static UINT32 ppc_read32_unaligned(UINT32 a)
{
	return ((UINT32)ppc.read8(a+0) << 24) | ((UINT32)ppc.read8(a+1) << 16) |
				   ((UINT32)ppc.read8(a+2) << 8) | ((UINT32)ppc.read8(a+3) << 0);
}

static UINT64 ppc_read64_unaligned(UINT32 a)
{
	return ((UINT64)READ32(a+0) << 32) | (UINT64)(READ32(a+4));
}

static void ppc_write16_unaligned(UINT32 a, UINT16 d)
{
	ppc.write8(a+0, (UINT8)(d >> 8));
	ppc.write8(a+1, (UINT8)(d));
}

static void ppc_write32_unaligned(UINT32 a, UINT32 d)
{
	ppc.write8(a+0, (UINT8)(d >> 24));
	ppc.write8(a+1, (UINT8)(d >> 16));
	ppc.write8(a+2, (UINT8)(d >> 8));
	ppc.write8(a+3, (UINT8)(d >> 0));
}

static void ppc_write64_unaligned(UINT32 a, UINT64 d)
{
	ppc.write32(a+0, (UINT32)(d >> 32));
	ppc.write32(a+4, (UINT32)(d));
}

/***********************************************************************/

enum
{
	PPC_TRANSLATE_DATA	= 0x0000,
	PPC_TRANSLATE_CODE	= 0x0001,

	PPC_TRANSLATE_READ	= 0x0000,
	PPC_TRANSLATE_WRITE	= 0x0002,

	PPC_TRANSLATE_NOEXCEPTION = 0x0004
};

static int ppc_translate_address(offs_t *addr_ptr, int flags)
{
	/* placeholder; not yet implemented */
	return 1;
}

static data8_t ppc_read8_translated(offs_t address)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_READ);
	return program_read_byte_64be(address);
}

static data16_t ppc_read16_translated(offs_t address)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_READ);
	return program_read_word_64be(address);
}

static data32_t ppc_read32_translated(offs_t address)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_READ);
	return program_read_dword_64be(address);
}

static data64_t ppc_read64_translated(offs_t address)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_READ);
	return program_read_qword_64be(address);
}

static void ppc_write8_translated(offs_t address, data8_t data)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_WRITE);
	program_write_byte_64be(address, data);
}

static void ppc_write16_translated(offs_t address, data16_t data)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_WRITE);
	program_write_word_64be(address, data);
}

static void ppc_write32_translated(offs_t address, data32_t data)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_WRITE);
	program_write_dword_64be(address, data);
}

static void ppc_write64_translated(offs_t address, data64_t data)
{
	ppc_translate_address(&address, PPC_TRANSLATE_DATA | PPC_TRANSLATE_WRITE);
	program_write_qword_64be(address, data);
}

static data32_t ppc_readop_translated(offs_t address)
{
	ppc_translate_address(&address, PPC_TRANSLATE_CODE | PPC_TRANSLATE_READ);
	return program_read_dword_64be(address);
}

/***********************************************************************/


static offs_t ppc_dasm(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	UINT32 op, translated_pc;

	if (MSR & MSR_IR)
	{
		translated_pc = pc;
		if (!ppc_translate_address(&translated_pc, PPC_TRANSLATE_CODE | PPC_TRANSLATE_READ | PPC_TRANSLATE_NOEXCEPTION))
		{
			sprintf(buffer, "<not executable>");
			return 4;
		}
		op = program_read_dword_32be(translated_pc);
	}
	else
		op = ROPCODE(pc);
	return ppc_dasm_one(buffer, pc, op);
#else
	sprintf(buffer, "$%08X", ROPCODE(pc));
	return 4;
#endif
}

static offs_t ppc_dasm64(char *buffer, offs_t pc)
{
#ifdef MAME_DEBUG
	UINT32 op, translated_pc;

	if (MSR & MSR_IR)
	{
		translated_pc = pc;
		if (!ppc_translate_address(&translated_pc, PPC_TRANSLATE_CODE | PPC_TRANSLATE_READ | PPC_TRANSLATE_NOEXCEPTION))
		{
			sprintf(buffer, "<not executable>");
			return 4;
		}
		op = program_read_dword_64be(translated_pc);
	}
	else
		op = ROPCODE64(pc);
	return ppc_dasm_one(buffer, pc, op);
#else
	sprintf(buffer, "$%08X", (UINT32)ROPCODE64(pc));
	return 4;
#endif
}

/***********************************************************************/

static int ppc_readop(UINT32 offset, int size, UINT64 *value)
{
	if (!(ppc.msr & MSR_IR))
		return 0;

	*value = 0;

	if (ppc_translate_address(&offset, PPC_TRANSLATE_CODE | PPC_TRANSLATE_READ | PPC_TRANSLATE_NOEXCEPTION))
	{
		switch(size)
		{
			case 1:	*value = program_read_byte(offset);	break;
			case 2:	*value = program_read_word(offset);	break;
			case 4:	*value = program_read_dword(offset);	break;
			case 8:	*value = program_read_qword(offset);	break;
		}
	}

	return 1;
}

static int ppc_read(int space, UINT32 offset, int size, UINT64 *value)
{
	if (!(ppc.msr & MSR_DR))
		return 0;

	*value = 0;

	if (ppc_translate_address(&offset, PPC_TRANSLATE_DATA | PPC_TRANSLATE_READ | PPC_TRANSLATE_NOEXCEPTION))
	{
		switch(size)
		{
			case 1:	*value = program_read_byte(offset);	break;
			case 2:	*value = program_read_word(offset);	break;
			case 4:	*value = program_read_dword(offset);	break;
			case 8:	*value = program_read_qword(offset);	break;
		}
	}

	return 1;
}

static int ppc_write(int space, UINT32 offset, int size, UINT64 value)
{
	if (!(ppc.msr & MSR_DR))
		return 0;

	if (ppc_translate_address(&offset, PPC_TRANSLATE_DATA | PPC_TRANSLATE_WRITE | PPC_TRANSLATE_NOEXCEPTION))
	{
		switch(size)
		{
			case 1:	program_write_byte(offset, value);	break;
			case 2:	program_write_word(offset, value);	break;
			case 4:	program_write_dword(offset, value);	break;
			case 8:	program_write_qword(offset, value);	break;
		}
	}

	return 1;
}
