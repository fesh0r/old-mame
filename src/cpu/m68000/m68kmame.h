/* HJB 01/10/99
 * Adaption to MAME's naming conventions, structures
 * and changes for the new interrupt system.
 * To be included at the end of the new m68000.h
 */

/* make the register structs look the same as the
 * old and in the assembler core (at least names ;)
 */
typedef struct {
   unsigned int  cpu_type;     /* CPU Type being emulated */
   unsigned int  dr[8];        /* Data Registers */
   unsigned int  ar[8];        /* Address Registers */
   unsigned int  pc;           /* Program Counter */
   unsigned int  usp;          /* User Stack Pointer */
   unsigned int  isp;          /* Interrupt Stack Pointer */
   unsigned int  vbr;          /* Vector Base Register.  Used in 68010+ */
   unsigned int  sfc;          /* Source Function Code.  Used in 68010+ */
   unsigned int  dfc;          /* Destination Function Code.  Used in 68010+ */
   unsigned int  sr;           /* Status Register */
   unsigned int  stopped;      /* Stopped state: only interrupt can restart */
   unsigned int  halted;       /* Halted state: only reset can restart */
   unsigned int  ints_pending; /* Interrupt levels pending */
#if NEW_INTERRUPT_SYSTEM
	int (*irq_callback)(int irqline);
#endif
}   regstruct;

/* well, another hack to make the names equal */
typedef struct {
	regstruct regs;
}	MC68000_Regs;

/* map some function names to MAME style */
#define MC68000_Reset m68000_input_reset
#define MC68000_GetPC m68000_peek_pc
#define MC68000_ICount m68000_clks_left
#define MC68000_Execute m68000_execute

/* m68000_disassemble does not use/need the memory base!? */
#define Dasm68000(membase,buffer,pc) m68000_disassemble(buffer,pc)

#if NEW_INTERRUPT_SYSTEM

/* dirty hack: declare the actual irq_callback static here
 * it might be included (and thus defined) more than once,
 * but only the copy included from cpuintrf.c should be used
 */
static int (*MC68000_irq_callback)(int irqline);

/* set/get the registers and our local irq_callback */
INLINE void MC68000_SetRegs(MC68000_Regs *regs)
{
	m68000_set_context((m68000_cpu_context *)&regs->regs);
	MC68000_irq_callback = regs->regs.irq_callback;
}

INLINE void MC68000_GetRegs(MC68000_Regs *regs)
{
	m68000_get_context((m68000_cpu_context *)&regs->regs);
	regs->regs.irq_callback = MC68000_irq_callback;
}

INLINE void MC68000_set_nmi_line(int state)
{
	/* does not apply */
}

/* there seems to be no need to call the irq_callback when the
 * interrupt is actually taken, so we do it right here.
 */
INLINE void MC68000_set_irq_line(int irqline, int state)
{
	if (state != CLEAR_LINE)
		m68000_input_interrupt((*MC68000_irq_callback)(irqline));
}

INLINE void MC68000_set_irq_callback(int (*callback)(int irqline))
{
	MC68000_irq_callback = callback;
}

#else

#define MC68000_SetRegs(R) m68000_set_context((m68000_cpu_context *)&R->regs)
#define MC68000_GetRegs(R) m68000_get_context((m68000_cpu_context *)&R->regs)
#define MC68000_CauseInterupt m68000_input_interrupt
#define MC68000_ClearPendingInterrupts m68000_clear_interrupts

#endif

