#ifndef SSP1610_H
#define SSP1610_H

#include "cpuintrf.h"

/* Functions */

#if (HAS_SSP1610)
void ssp1610_get_info(UINT32 state, cpuinfo *info);
#endif

#ifdef ENABLE_DEBUGGER
extern unsigned dasm_ssp1610(char *buffer, unsigned pc, const UINT8 *oprom);
#endif

#endif /* SSP1610_H */
