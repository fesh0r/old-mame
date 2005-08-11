/***************************************************************************

    hiscore.h

    Manages the hiscore system.

***************************************************************************/

#pragma once

#ifndef __HISCORE_H__
#define __HISCORE_H__

void hs_open( const char *name );
void hs_init( void );
void hs_update( void );
void hs_close( void );

void computer_writemem_byte(int cpu, int addr, int value);
int computer_readmem_byte(int cpu, int addr);

#endif	/* __HISCORE_H__ */
