/*****************************************************************************
 *
 *   ops4510.h
 *   Addressing mode and opcode macros for 4510 CPU
 *
 *   Copyright Peter Trauner, all rights reserved.
 *   documentation preliminary databook
 *   documentation by michael steil mist@c64.org
 *   available at ftp://ftp.funet.fi/pub/cbm/c65
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     pullmoll@t-online.de
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/


/* 65ce02 ********************************************************
 * TXS  Transfer index X to stack LSB
 * no flags changed (sic!)
 * txs tys not interruptable
 ***************************************************************/
#undef TXS
#define TXS								\
	SPL = X;							\
	if (PEEK_OP() == 0x2b /*TYS*/ ) {	            \
		UINT8 op = RDOP();				\
		(*m4510->insn[op])(m4510);			\
	}

#undef NOP
#define NOP       \
  m4510->interrupt_inhibit = 0;

/* c65 docu says transfer of axyz to the mapper register
   so no readback!? */
#define MAP 						\
  m4510->interrupt_inhibit = 1;			\
  m4510->low=m4510->a|(m4510->x<<8); \
  m4510->high=m4510->y|(m4510->z<<8); \
  m4510->mem[0]=(m4510->low&0x1000) ?  (m4510->low&0xfff)<<8:0;  \
  m4510->mem[1]=(m4510->low&0x2000) ?  (m4510->low&0xfff)<<8:0;  \
  m4510->mem[2]=(m4510->low&0x4000) ?  (m4510->low&0xfff)<<8:0;  \
  m4510->mem[3]=(m4510->low&0x8000) ?  (m4510->low&0xfff)<<8:0;  \
  m4510->mem[4]=(m4510->high&0x1000) ? (m4510->high&0xfff)<<8:0; \
  m4510->mem[5]=(m4510->high&0x2000) ? (m4510->high&0xfff)<<8:0; \
  m4510->mem[6]=(m4510->high&0x4000) ? (m4510->high&0xfff)<<8:0; \
  m4510->mem[7]=(m4510->high&0x8000) ? (m4510->high&0xfff)<<8:0; \
  CHANGE_PC; \
  m4510->icount -= 3; \
  { \
				UINT8 op = RDOP();								\
				(*m4510->insn[op])(m4510);							\
  }
