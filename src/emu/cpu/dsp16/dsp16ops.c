#include "dsp16.h"


// The YL register is the lower half of the 32 bit Y register
void* dsp16_device::addressYL()
{
	return (void*)(((UINT8*)&m_y) + 2);
}


void dsp16_device::writeRegister(void* reg, const UINT16 &value)
{
	// Make sure you're not attempting to write somewhere this function doesn't support.
	if (reg == &m_p || reg == &m_a0 || reg == &m_a1)
	{
		logerror("dsp16::writeRegister called on invalid register at PC 0x%04x.\n", m_pc);
		return;
	}

	if (reg == &m_auc || reg == &m_c0 || reg == &m_c1 || reg == &m_c2)
	{
		// 8 bit registers
		*(UINT8*)reg = value & 0x00ff;
	}
	else if (reg == &m_i)
	{
		// 12 bit register
		m_i = value & 0x0fff;
	}
	else if (reg == &m_y)
	{
		// Y register [[TODO - check a flag to see if clearing yl is necessary]]
		m_y = (value << 16) | (m_y & 0x0000ffff);
	}
	else if (reg == addressYL())
	{
		// Yl register
		m_y = value | (m_y & 0xffff0000);
	}
	else
	{
		// Everything else
		*(UINT16*)reg = value;
	}
}


void* dsp16_device::registerFromRImmediateField(const UINT8& R)
{
	switch (R)
	{
		case 0x00: return (void*)&m_j;
		case 0x01: return (void*)&m_k;
		case 0x02: return (void*)&m_rb;
		case 0x03: return (void*)&m_re;
		case 0x04: return (void*)&m_r0;
		case 0x05: return (void*)&m_r1;
		case 0x06: return (void*)&m_r2;
		case 0x07: return (void*)&m_r3;

		default: return NULL;
	}
	return NULL;
}


void* dsp16_device::registerFromRTable(const UINT8 &R)
{
	switch (R)
	{
		case 0x00: return (void*)&m_r0;
		case 0x01: return (void*)&m_r1;
		case 0x02: return (void*)&m_r2;
		case 0x03: return (void*)&m_r3;
		case 0x04: return (void*)&m_j;
		case 0x05: return (void*)&m_k;
		case 0x06: return (void*)&m_rb;
		case 0x07: return (void*)&m_re;
		case 0x08: return (void*)&m_pt;
		case 0x09: return (void*)&m_pr;
		case 0x0a: return (void*)&m_pi;
		case 0x0b: return (void*)&m_i;

		case 0x10: return (void*)&m_x;
		case 0x11: return (void*)&m_y;
		case 0x12: return (void*)addressYL();
		case 0x13: return (void*)&m_auc;
		case 0x14: return (void*)&m_psw;
		case 0x15: return (void*)&m_c0;
		case 0x16: return (void*)&m_c1;
		case 0x17: return (void*)&m_c2;
		case 0x18: return (void*)&m_sioc;
		//case 0x19: return (void*)&m_srta;
		//case 0x1a: return (void*)&m_sdx;
		//case 0x1b: return (void*)&m_tdms;
		case 0x1c: return (void*)&m_pioc;
		//case 0x1d: return (void*)&m_pdx0;
		//case 0x1e: return (void*)&m_pdx1;

		default: return NULL;
	}
	return NULL;
}


void dsp16_device::executeF1Field(const UINT8& F1, const UINT8& D, const UINT8& S)
{
	// Where is the first operation being written?
	//UINT64* destinationReg = NULL;
	//switch (D)
	//{
	//  case 0x00: destinationReg = &m_a0;
	//  case 0x01: destinationReg = &m_a1;
	//  default: break;
	//}

	// Which source is being used?
	//UINT64* sourceReg = NULL;
	//switch (S)
	//{
	//  case 0x00: sourceReg = &m_a0;
	//  case 0x01: sourceReg = &m_a1;
	//  default: break;
	//}

	switch (F1)
	{
		case 0x00: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x01: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x02: m_p = (INT32)((INT16)m_x * (INT16)((m_y & 0xffff0000) >> 16)); break;
		case 0x03: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x04: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x05: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x06: /* nop */ break;
		case 0x07: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x08: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x09: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x0a: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x0b: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x0c: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x0d: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x0e: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
		case 0x0f: printf("UNIMPLEMENTED F1 operation @ PC 0x%04x\n", m_pc); break;
	}
}


void* dsp16_device::registerFromYFieldUpper(const UINT8& Y)
{
	UINT16* destinationReg = NULL;
	const UINT8 N = (Y & 0x0c) >> 2;
	switch (N)
	{
		case 0x00: destinationReg = &m_r0; break;
		case 0x01: destinationReg = &m_r1; break;
		case 0x02: destinationReg = &m_r2; break;
		case 0x03: destinationReg = &m_r3; break;
		default: break;
	}
	return destinationReg;
}


void dsp16_device::executeYFieldPost(const UINT8& Y)
{
	UINT16* opReg = NULL;
	const UINT8 N = (Y & 0x0c) >> 2;
	switch (N)
	{
		case 0x00: opReg = &m_r0; break;
		case 0x01: opReg = &m_r1; break;
		case 0x02: opReg = &m_r2; break;
		case 0x03: opReg = &m_r3; break;
		default: break;
	}

	const UINT8 lower = Y & 0x03;
	switch (lower)
	{
		case 0x00: /* nop */ break;
		case 0x01: (*opReg)++; break;
		case 0x02: (*opReg)--; break;
		case 0x03: (*opReg) += m_j; break;
	}
}


void dsp16_device::execute_one(const UINT16& op, UINT8& cycles, UINT8& pcAdvance)
{
	cycles = 1;
	pcAdvance = 0;

	const UINT8 opcode = (op >> 11) & 0x1f;
	switch(opcode)
	{
		// Format 1: Multiply/ALU Read/Write Group
		case 0x06:
		{
			// F1, Y  :  (page 3-38)
			const UINT8 Y = (op & 0x000f);
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			executeF1Field(F1, D, S);
			executeYFieldPost(Y);
			cycles = 1;
			pcAdvance = 1;
			break;
		}
		case 0x04: case 0x1c:
		{
			// F1 Y=a0[1] | F1 Y=a1[1]
			//const UINT8 Y = (op & 0x000f);
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 D = (op & 0x0400) >> 10;
			//const UINT8 F1 = (op & 0x01e0) >> 5;
			break;
		}
		case 0x16:
		{
			// F1, x = Y
			//const UINT8 Y = (op & 0x000f);
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 D = (op & 0x0400) >> 10;
			//const UINT8 F1 = (op & 0x01e0) >> 5;
			break;
		}
		case 0x17:
		{
			// F1, y[l] = Y  :  (page 3-44)
			const UINT8 Y = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			executeF1Field(F1, D, S);
			UINT16* sourceReg = (UINT16*)registerFromYFieldUpper(Y);
			UINT16 sourceValue = data_read(*sourceReg);
			switch (X)
			{
				case 0x00: writeRegister(addressYL(), sourceValue); break;
				case 0x01: writeRegister(&m_y, sourceValue); break;
				default: break;
			}
			executeYFieldPost(Y);
			cycles = 1;
			pcAdvance = 1;
			break;
		}
		case 0x1f:
		{
			// F1, y = Y, x = *pt++[i]
			//const UINT8 Y = (op & 0x000f);
			//const UINT8 X = (op & 0x0010) >> 4;
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 D = (op & 0x0400) >> 10;
			//const UINT8 F1 = (op & 0x01e0) >> 5;
			break;
		}
		case 0x19: case 0x1b:
		{
			// F1, y = a0|1, x = *pt++[i]
			//const UINT8 Y = (op & 0x000f);
			//const UINT8 X = (op & 0x0010) >> 4;
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 D = (op & 0x0400) >> 10;
			//const UINT8 F1 = (op & 0x01e0) >> 5;
			break;
		}
		case 0x14:
		{
			// F1, Y = y[1]  :  (page 3-53)
			const UINT8 Y = (op & 0x000f);
			const UINT8 X = (op & 0x0010) >> 4;
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 D = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			executeF1Field(F1, D, S);
			UINT16* destinationReg = (UINT16*)registerFromYFieldUpper(Y);
			UINT16 yRegValue = 0x0000;
			switch (X)
			{
				case 0x00: yRegValue = (m_y & 0x0000ffff); break;
				case 0x01: yRegValue = (m_y & 0xffff0000) >> 16; break;
				default: break;
			}
			data_write(*destinationReg, yRegValue);
			executeYFieldPost(Y);
			cycles = 2;
			pcAdvance = 1;
			break;
		}

		// Format 1a: Multiply/ALU Read/Write Group (TODO: Figure out major typo in docs on p3-51)
		case 0x07:
		{
			// F1, At[1] = Y  :  (page 3-50)
			const UINT8 Y = (op & 0x000f);
			const UINT8 S = (op & 0x0200) >> 9;
			const UINT8 aT = (op & 0x0400) >> 10;
			const UINT8 F1 = (op & 0x01e0) >> 5;
			executeF1Field(F1, !aT, S);
			UINT64* destinationReg = NULL;
			switch(aT)
			{
				case 0: destinationReg = &m_a1; break;
				case 1: destinationReg = &m_a0; break;
				default: break;
			}
			UINT16 sourceAddress = *((UINT16*)registerFromYFieldUpper(Y));
			INT64 sourceValueSigned = (INT16)data_read(sourceAddress);
			*destinationReg = sourceValueSigned & U64(0xffffffffff);
			executeYFieldPost(Y);
			cycles = 1;
			pcAdvance = 1;
			break;
		}

		// Format 2: Multiply/ALU Read/Write Group
		case 0x15:
		{
			// F1, Z : y[1]
			//const UINT8 Z = (op & 0x000f);
			//const UINT8 X = (op & 0x0010) >> 4;
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 D = (op & 0x0400) >> 10;
			//const UINT8 F1 = (op & 0x01e0) >> 5;
			break;
		}
		case 0x1d:
		{
			// F1, Z : y, x=*pt++[i]
			//const UINT8 Z = (op & 0x000f);
			//const UINT8 X = (op & 0x0010) >> 4;
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 D = (op & 0x0400) >> 10;
			//const UINT8 F1 = (op & 0x01e0) >> 5;
			break;
		}

		// Format 2a: Multiply/ALU Read/Write Group
		case 0x05:
		{
			// F1, Z : aT[1]
			//const UINT8 Z = (op & 0x000f);
			//const UINT8 X = (op & 0x0010) >> 4;
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 aT = (op & 0x0400) >> 10;
			//const UINT8 F1 = (op & 0x01e0) >> 5;
			break;
		}

		// Format 3: Special Functions
		case 0x12:
		case 0x13:
		{
			// if|ifc CON F2
			//const UINT8 CON = (op & 0x001f);
			//const UINT8 S = (op & 0x0200) >> 9;
			//const UINT8 D = (op & 0x0400) >> 10;
			//const UINT8 F2 = (op & 0x01e0) >> 5;
			break;
		}

		// Format 4: Branch Direct Group
		case 0x00: case 0x01:
		{
			// goto JA  :  (page 3-20)
			const UINT16 JA = (op & 0x0fff) | (m_pc & 0xf000);
			m_pc = JA;
			cycles = 2;
			pcAdvance = 0;
			break;
		}

		case 0x10: case 0x11:
		{
			// call JA  :  (page 3-23)
			const UINT16 JA = (op & 0x0fff) | (m_pc & 0xf000);
			m_pr = m_pc + 1;
			m_pc = JA;
			cycles = 2;
			pcAdvance = 0;
			break;
		}

		// Format 5: Branch Indirect Group
		case 0x18:
		{
			// goto B
			//const UINT8 B = (op & 0x0700) >> 8;
			break;
		}

		// Format 6: Contitional Branch Qualifier/Software Interrupt (icall)
		case 0x1a:
		{
			// if CON [goto/call/return]
			//const UINT8 CON = (op & 0x001f);
			break;
		}

		// Format 7: Data Move Group
		case 0x09: case 0x0b:
		{
			// R = aS
			//const UINT8 R = (op & 0x03f0) >> 4;
			//const UINT8 S = (op & 0x1000) >> 12;
			break;
		}
		case 0x08:
		{
			// aT = R
			//const UINT8 R  = (op & 0x03f0) >> 4;
			//const UINT8 aT = (op & 0x0400) >> 10;
			break;
		}
		case 0x0f:
		{
			// R = Y
			//const UINT8 Y = (op & 0x000f);
			//const UINT8 R = (op & 0x03f0) >> 4;
			break;
		}
		case 0x0c:
		{
			// Y = R  :  (page 3-33)
			const UINT8 Y = (op & 0x000f);
			const UINT8 R = (op & 0x03f0) >> 4;
			UINT16* destinationReg = (UINT16*)registerFromYFieldUpper(Y);
			UINT16* sourceReg = (UINT16*)registerFromRTable(R);
			data_write(*destinationReg, *sourceReg);
			executeYFieldPost(Y);
			cycles = 2;
			pcAdvance = 1;
			break;
		}
		case 0x0d:
		{
			// Z : R
			//const UINT8 Z = (op & 0x000f);
			//const UINT8 R = (op & 0x03f0) >> 4;
			break;
		}

		// Format 8: Data Move (immediate operand - 2 words)
		case 0x0a:
		{
			// R = N  :  (page 3-28)
			const UINT8 R = (op & 0x03f0) >> 4;
			const UINT16 iVal = opcode_read(1);
			void* reg = registerFromRTable(R);
			writeRegister(reg, iVal);
			cycles = 2;
			pcAdvance = 2;
			break;
		}

		// Format 9: Short Immediate Group
		case 0x02: case 0x03:
		{
			// R = M  :  (page 3-27)
			const INT8 M = (op & 0x00ff);
			const UINT8 R = (op & 0x0e00) >> 9;
			void* reg = registerFromRImmediateField(R);
			writeRegister(reg, (INT16)M);   // Sign extend 8 bit int
			cycles = 1;
			pcAdvance = 1;
			break;
		}

		// Format 10: do - redo
		case 0x0e:
		{
			// do|redo K  :  (pages 3-25 & 3-26)
			const UINT8 K = (op & 0x007f);
			const UINT8 NI = (op & 0x0780) >> 7;
			if (NI != 0)
			{
				// Do
				m_cacheStart = m_pc + 1;
				m_cacheEnd = m_pc + NI + 1;
				m_cacheIterations = K-1;    // -1 because we check the counter below
				cycles = 1;
				pcAdvance = 1;
			}
			else
			{
				// Redo
				m_cacheIterations = K-1;    // -1 because we check the counter below
				m_cacheRedoNextPC = m_pc + 1;
				m_pc = m_cacheStart;
				pcAdvance = 0;
				cycles = 2;
				pcAdvance = 1;
			}
			break;
		}

		// RESERVED
		case 0x1e:
		{
			break;
		}

		// UNKNOWN
		default:
		{
			break;
		}
	}

	// Handle end-of-cache conditions for do|redos
	if (m_cacheIterations == 0 && m_cacheRedoNextPC != CACHE_INVALID)
	{
		// You've reached the end of a cache loop after a redo opcode.
		m_pc = m_cacheRedoNextPC;
		m_cacheRedoNextPC = CACHE_INVALID;
		pcAdvance = 0;
	}
	if (m_cacheIterations > 0 && (m_pc+pcAdvance == m_cacheEnd))
	{
		// A regular iteration on a cached loop.
		m_cacheIterations--;
		m_pc = m_cacheStart;
		pcAdvance = 0;
	}
}
