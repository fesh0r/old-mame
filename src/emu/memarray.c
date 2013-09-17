/***************************************************************************

    memarray.c

    Generic memory array accessor helper.

****************************************************************************

    Copyright Aaron Giles
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "emu.h"


//**************************************************************************
//  MEMORY ARRAY HELPER
//**************************************************************************

//-------------------------------------------------
//  memory_array - constructor
//-------------------------------------------------

memory_array::memory_array()
	: m_base(NULL),
		m_bytes(0),
		m_membits(0),
		m_endianness(ENDIANNESS_LITTLE),
		m_bytes_per_entry(0),
		m_reader(NULL),
		m_writer(NULL)
{
}


//-------------------------------------------------
//  set - configure the parameters
//-------------------------------------------------

void memory_array::set(void *base, UINT32 bytes, int membits, endianness_t endianness, int bpe)
{
	// validate inputs
	assert(base != NULL);
	assert(bytes > 0);
	assert(membits == 8 || membits == 16 || membits == 32 || membits == 64);
	assert(bpe == 1 || bpe == 2 || bpe == 4);

	// populate direct data
	m_base = base;
	m_bytes = bytes;
	m_membits = membits;
	m_endianness = endianness;
	m_bytes_per_entry = bpe;

	// derive data
	switch (bpe*1000 + membits*10 + endianness)
	{
		case 1*1000 + 8*10 + ENDIANNESS_LITTLE:     m_reader = &memory_array::read8_from_8;       m_writer = &memory_array::write8_to_8;        break;
		case 1*1000 + 8*10 + ENDIANNESS_BIG:        m_reader = &memory_array::read8_from_8;       m_writer = &memory_array::write8_to_8;        break;
		case 1*1000 + 16*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read8_from_16le;    m_writer = &memory_array::write8_to_16le;     break;
		case 1*1000 + 16*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read8_from_16be;    m_writer = &memory_array::write8_to_16be;     break;
		case 1*1000 + 32*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read8_from_32le;    m_writer = &memory_array::write8_to_32le;     break;
		case 1*1000 + 32*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read8_from_32be;    m_writer = &memory_array::write8_to_32be;     break;
		case 1*1000 + 64*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read8_from_64le;    m_writer = &memory_array::write8_to_64le;     break;
		case 1*1000 + 64*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read8_from_64be;    m_writer = &memory_array::write8_to_64be;     break;

		case 2*1000 + 8*10 + ENDIANNESS_LITTLE:     m_reader = &memory_array::read16_from_8le;    m_writer = &memory_array::write16_to_8le;     break;
		case 2*1000 + 8*10 + ENDIANNESS_BIG:        m_reader = &memory_array::read16_from_8be;    m_writer = &memory_array::write16_to_8be;     break;
		case 2*1000 + 16*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read16_from_16;     m_writer = &memory_array::write16_to_16;      break;
		case 2*1000 + 16*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read16_from_16;     m_writer = &memory_array::write16_to_16;      break;
		case 2*1000 + 32*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read16_from_32le;   m_writer = &memory_array::write16_to_32le;    break;
		case 2*1000 + 32*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read16_from_32be;   m_writer = &memory_array::write16_to_32be;    break;
		case 2*1000 + 64*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read16_from_64le;   m_writer = &memory_array::write16_to_64le;    break;
		case 2*1000 + 64*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read16_from_64be;   m_writer = &memory_array::write16_to_64be;    break;

		case 4*1000 + 8*10 + ENDIANNESS_LITTLE:     m_reader = &memory_array::read32_from_8le;    m_writer = &memory_array::write32_to_8le;     break;
		case 4*1000 + 8*10 + ENDIANNESS_BIG:        m_reader = &memory_array::read32_from_8be;    m_writer = &memory_array::write32_to_8be;     break;
		case 4*1000 + 16*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read32_from_16le;   m_writer = &memory_array::write32_to_16le;    break;
		case 4*1000 + 16*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read32_from_16be;   m_writer = &memory_array::write32_to_16be;    break;
		case 4*1000 + 32*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read32_from_32;     m_writer = &memory_array::write32_to_32;      break;
		case 4*1000 + 32*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read32_from_32;     m_writer = &memory_array::write32_to_32;      break;
		case 4*1000 + 64*10 + ENDIANNESS_LITTLE:    m_reader = &memory_array::read32_from_64le;   m_writer = &memory_array::write32_to_64le;    break;
		case 4*1000 + 64*10 + ENDIANNESS_BIG:       m_reader = &memory_array::read32_from_64be;   m_writer = &memory_array::write32_to_64be;    break;

		default:    throw emu_fatalerror("Illegal memory bits/bus width combo in memory_array");
	}
}


//-------------------------------------------------
//  set - additional setter variants
//-------------------------------------------------

void memory_array::set(const address_space &space, void *base, UINT32 bytes, int bpe)
{
	set(base, bytes, space.data_width(), space.endianness(), bpe);
}

void memory_array::set(const memory_share &share, int bpe)
{
	set(share.ptr(), share.bytes(), share.width(), share.endianness(), bpe);
}

void memory_array::set(const memory_array &helper)
{
	set(helper.base(), helper.bytes(), helper.membits(), helper.endianness(), helper.bytes_per_entry());
}


//-------------------------------------------------
//  read8_from_*/write8_to_* - entry read/write
//  heleprs for 1 byte-per-entry
//-------------------------------------------------

UINT32 memory_array::read8_from_8(int index) { return reinterpret_cast<UINT8 *>(m_base)[index]; }
void memory_array::write8_to_8(int index, UINT32 data) { reinterpret_cast<UINT8 *>(m_base)[index] = data; }

UINT32 memory_array::read8_from_16le(int index) { return reinterpret_cast<UINT8 *>(m_base)[BYTE_XOR_LE(index)]; }
void memory_array::write8_to_16le(int index, UINT32 data) { reinterpret_cast<UINT8 *>(m_base)[BYTE_XOR_LE(index)] = data; }
UINT32 memory_array::read8_from_16be(int index) { return reinterpret_cast<UINT8 *>(m_base)[BYTE_XOR_BE(index)]; }
void memory_array::write8_to_16be(int index, UINT32 data) { reinterpret_cast<UINT8 *>(m_base)[BYTE_XOR_BE(index)] = data; }

UINT32 memory_array::read8_from_32le(int index) { return reinterpret_cast<UINT8 *>(m_base)[BYTE4_XOR_BE(index)]; }
void memory_array::write8_to_32le(int index, UINT32 data) { reinterpret_cast<UINT8 *>(m_base)[BYTE4_XOR_BE(index)] = data; }
UINT32 memory_array::read8_from_32be(int index) { return reinterpret_cast<UINT8 *>(m_base)[BYTE4_XOR_BE(index)]; }
void memory_array::write8_to_32be(int index, UINT32 data) { reinterpret_cast<UINT8 *>(m_base)[BYTE4_XOR_BE(index)] = data; }

UINT32 memory_array::read8_from_64le(int index) { return reinterpret_cast<UINT8 *>(m_base)[BYTE8_XOR_BE(index)]; }
void memory_array::write8_to_64le(int index, UINT32 data) { reinterpret_cast<UINT8 *>(m_base)[BYTE8_XOR_BE(index)] = data; }
UINT32 memory_array::read8_from_64be(int index) { return reinterpret_cast<UINT8 *>(m_base)[BYTE8_XOR_BE(index)]; }
void memory_array::write8_to_64be(int index, UINT32 data) { reinterpret_cast<UINT8 *>(m_base)[BYTE8_XOR_BE(index)] = data; }


//-------------------------------------------------
//  read16_from_*/write16_to_* - entry read/write
//  heleprs for 2 bytes-per-entry
//-------------------------------------------------

UINT32 memory_array::read16_from_8le(int index) { return read8_from_8(index*2) | (read8_from_8(index*2+1) << 8); }
void memory_array::write16_to_8le(int index, UINT32 data) { write8_to_8(index*2, data); write8_to_8(index*2+1, data >> 8); }
UINT32 memory_array::read16_from_8be(int index) { return (read8_from_8(index*2) << 8) | read8_from_8(index*2+1); }
void memory_array::write16_to_8be(int index, UINT32 data) { write8_to_8(index*2, data >> 8); write8_to_8(index*2+1, data); }

UINT32 memory_array::read16_from_16(int index) { return reinterpret_cast<UINT16 *>(m_base)[index]; }
void memory_array::write16_to_16(int index, UINT32 data) { reinterpret_cast<UINT16 *>(m_base)[index] = data; }

UINT32 memory_array::read16_from_32le(int index) { return reinterpret_cast<UINT16 *>(m_base)[BYTE_XOR_LE(index)]; }
void memory_array::write16_to_32le(int index, UINT32 data) { reinterpret_cast<UINT16 *>(m_base)[BYTE_XOR_LE(index)] = data; }
UINT32 memory_array::read16_from_32be(int index) { return reinterpret_cast<UINT16 *>(m_base)[BYTE_XOR_BE(index)]; }
void memory_array::write16_to_32be(int index, UINT32 data) { reinterpret_cast<UINT16 *>(m_base)[BYTE_XOR_BE(index)] = data; }

UINT32 memory_array::read16_from_64le(int index) { return reinterpret_cast<UINT16 *>(m_base)[BYTE4_XOR_LE(index)]; }
void memory_array::write16_to_64le(int index, UINT32 data) { reinterpret_cast<UINT16 *>(m_base)[BYTE4_XOR_LE(index)] = data; }
UINT32 memory_array::read16_from_64be(int index) { return reinterpret_cast<UINT16 *>(m_base)[BYTE4_XOR_BE(index)]; }
void memory_array::write16_to_64be(int index, UINT32 data) { reinterpret_cast<UINT16 *>(m_base)[BYTE4_XOR_BE(index)] = data; }


//-------------------------------------------------
//  read32_from_*/write32_to_* - entry read/write
//  heleprs for 4 bytes-per-entry
//-------------------------------------------------

UINT32 memory_array::read32_from_8le(int index) { return read16_from_8le(index*2) | (read16_from_8le(index*2+1) << 16); }
void memory_array::write32_to_8le(int index, UINT32 data) { write16_to_8le(index*2, data); write16_to_8le(index*2+1, data >> 16); }
UINT32 memory_array::read32_from_8be(int index) { return (read16_from_8be(index*2) << 16) | read16_from_8be(index*2+1); }
void memory_array::write32_to_8be(int index, UINT32 data) { write16_to_8be(index*2, data >> 16); write16_to_8be(index*2+1, data); }

UINT32 memory_array::read32_from_16le(int index) { return read16_from_16(index*2) | (read16_from_16(index*2+1) << 16); }
void memory_array::write32_to_16le(int index, UINT32 data) { write16_to_16(index*2, data); write16_to_16(index*2+1, data >> 16); }
UINT32 memory_array::read32_from_16be(int index) { return (read16_from_16(index*2) << 16) | read16_from_16(index*2+1); }
void memory_array::write32_to_16be(int index, UINT32 data) { write16_to_16(index*2, data >> 16); write16_to_16(index*2+1, data); }

UINT32 memory_array::read32_from_32(int index) { return reinterpret_cast<UINT32 *>(m_base)[index]; }
void memory_array::write32_to_32(int index, UINT32 data) { reinterpret_cast<UINT32 *>(m_base)[index] = data; }

UINT32 memory_array::read32_from_64le(int index) { return reinterpret_cast<UINT32 *>(m_base)[BYTE_XOR_LE(index)]; }
void memory_array::write32_to_64le(int index, UINT32 data) { reinterpret_cast<UINT32 *>(m_base)[BYTE_XOR_LE(index)] = data; }
UINT32 memory_array::read32_from_64be(int index) { return reinterpret_cast<UINT32 *>(m_base)[BYTE_XOR_BE(index)]; }
void memory_array::write32_to_64be(int index, UINT32 data) { reinterpret_cast<UINT32 *>(m_base)[BYTE_XOR_BE(index)] = data; }
