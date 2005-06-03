/***************************************************************************

	pc_vga.h

	PC standard VGA adaptor

***************************************************************************/

#ifndef PC_VGA_H
#define PC_VGA_H

#include "osdepend.h"

MACHINE_DRIVER_EXTERN( pcvideo_vga );
MACHINE_DRIVER_EXTERN( pcvideo_pc1640 );

struct pc_vga_interface
{
	read8_handler read_dipswitch;

	/* where the ports go */
	int port_addressspace;
	offs_t port_offset;
};


void pc_vga_init(const struct pc_vga_interface *intf);
void pc_vga_reset(void);

READ8_HANDLER( ega_port_03c0_r );
READ8_HANDLER( ega_port_03d0_r );

READ8_HANDLER( paradise_ega_03c0_r );

READ8_HANDLER( vga_port_03b0_r );
READ8_HANDLER( vga_port_03c0_r );
READ8_HANDLER( vga_port_03d0_r );

WRITE8_HANDLER( vga_port_03b0_w );
WRITE8_HANDLER( vga_port_03c0_w );
WRITE8_HANDLER( vga_port_03d0_w );

/*
  pega notes (paradise)
  build in amstrad pc1640

  ROM_LOAD("40100", 0xc0000, 0x8000, CRC(d2d1f1ae))

  4 additional dipswitches
  seems to have emulation modes at register level
  (mda/hgc lines bit 8 not identical to ega/vga)

  standard ega/vga dipswitches
  00000000	320x200
  00000001	640x200 hanging
  00000010	640x200 hanging
  00000011	640x200 hanging

  00000100	640x350 hanging
  00000101	640x350 hanging EGA mono
  00000110	320x200
  00000111	640x200

  00001000	640x200
  00001001	640x200
  00001010	720x350 partial visible
  00001011	720x350 partial visible

  00001100	320x200
  00001101	320x200
  00001110	320x200
  00001111	320x200

*/

/*
  oak vga (oti 037 chip)
  (below bios patch needed for running)

  ROM_LOAD("oakvga.bin", 0xc0000, 0x8000, CRC(318c5f43))
*/
#if 0
        int i; 
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

		/* oak vga */
        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;
#endif

#endif /* PC_VGA_H */

