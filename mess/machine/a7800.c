/***************************************************************************

  a7800.c

  Machine file to handle emulation of the Atari 7800.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiasound.h"
#include "cpuintrf.h"
#include "zlib.h"

#include "includes/a7800.h"

unsigned char *a7800_cart_f000 = NULL;
unsigned char *a7800_bios_f000 = NULL;
int a7800_ctrl_lock;
int a7800_ctrl_reg;
int maria_flag;

/* local */
unsigned char *a7800_ram;
unsigned char *a7800_cartridge_rom;
unsigned char a7800_cart_type;
unsigned char a7800_stick_type;
static UINT8 *ROM;

void a7800_init_machine(void) {
    a7800_ctrl_lock = 0;
    a7800_ctrl_reg = 0;
    maria_flag=0;
    if (a7800_cart_type & 0x01){
        install_mem_write_handler(0,0x4000,0x7FFF,pokey1_w);
    }
}

void a7800_stop_machine(void)
{

	if (a7800_bios_f000) free(a7800_bios_f000);
	a7800_bios_f000 = NULL;
	if (a7800_cart_f000) free(a7800_cart_f000);
	a7800_cart_f000 = NULL;
}

/*    Header format
0      Header version     - 1 byte
1..16  "ATARI7800      "  - 16 bytes
17..48 Cart title         - 32 bytes
49..52 data length        - 4 bytes
53..54 cart type          - 2 bytes
    bit 0 - Pokey cart
    bit 1 - Supercart bank switched
    bit 2 - Supercart RAM at $4000
    bit 3 - ROM at $4000
    bit 4 - Bank 6 at $4000
    bit 8-15 - Special
        0 = Normal cart

55     controller 1 type  - 1 byte
56     controller 2 type  - 1 byte
    0 = None
    1 = Joystick
    2 = Light Gun
57  0 = NTSC/1 = PAL

100..127 "ACTUAL CART DATA STARTS HERE" - 28 bytes

Versions:
	Version 0: Initial release
	Version 1: Added PAL/NTSC bit. Added Special cart byte.
               Changed 53 bit 2, added bit 3

*/

UINT32 a7800_partialcrc(const unsigned char *buf,unsigned int size)
{
UINT32 crc;
if (size < 129) return 0;
crc = (UINT32) crc32(0L,&buf[128],size-128);
logerror("A7800 Partial CRC: %08lx %d [%s]\n",crc,size,&buf[1]);
return crc;
}


void a7800_exit_rom (int id)
{
	if (a7800_bios_f000) free(a7800_bios_f000);
	a7800_bios_f000 = NULL;
	if (a7800_cart_f000) free(a7800_cart_f000);
    a7800_cart_f000 = NULL;
}

static int a7800_verify_cart (char header[128])
{
    char tag[] = "ATARI7800";

	if (strncmp(&tag[0], &header[1],9)) {
		logerror("Not a valid A7800 image\n");
		return IMAGE_VERIFY_FAIL;
	}
	logerror("returning ID_OK\n");
    return IMAGE_VERIFY_PASS;
}


int a7800_init_cart (int id)
{

    FILE *cartfile =NULL;
    long len,start;
    unsigned char header[128];

    ROM = memory_region(REGION_CPU1);

    /* A cartridge is mandatory, since it doesnt do much without one */
	if (device_filename(IO_CARTSLOT,id) == NULL ||
		strlen(device_filename(IO_CARTSLOT,id)) == 0)
    {
        logerror("A7800 - no cartridge specified!\n");
        return INIT_FAIL;
    }

	if (!(cartfile = (FILE*)image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0)))
    {
		logerror("A7800 - Unable to locate cartridge: %s\n",device_filename(IO_CARTSLOT,id) == NULL);
        return INIT_FAIL;
    }

    /* Allocate memory for BIOS bank switching */
    a7800_bios_f000 = (UINT8*)malloc(0x1000);
    if (!a7800_bios_f000) {
        logerror("Could not allocate ROM memory\n");
        return INIT_FAIL;
    }
    a7800_cart_f000 = (UINT8*)malloc(0x1000);
    if (!a7800_cart_f000) {
        logerror("Could not allocate ROM memory\n");
        free(a7800_bios_f000);
        return INIT_FAIL;
    }

    /* save the BIOS so we can switch it in and out */
    memcpy(a7800_bios_f000,&(ROM[0xF000]),0x1000);

    /* No cart, exit */
    if (cartfile == NULL)
    	return INIT_FAIL;

    /* Load and decode the header */
    osd_fread(cartfile,header,128);

    /* Check the cart */
    if (a7800_verify_cart((char *)header) == IMAGE_VERIFY_FAIL)
    {
		osd_fclose(cartfile);
		return INIT_FAIL;
	}

    len = (header[49] << 24) | (header[50] << 16) | (header[51] << 8) | header[52];
    a7800_cart_type = (header[53] << 8) | header[54];
    a7800_stick_type = header[55];
    logerror("Cart type: %x\n",a7800_cart_type);

    /* For now, if game support stick and gun, set it to stick */
    if (a7800_stick_type == 3) a7800_stick_type = 1;

    /* Normal Cart */
    if  (a7800_cart_type == 0 || a7800_cart_type == 1) {
        start = 0x10000 - len;
        a7800_cartridge_rom = &(ROM[start]);
        osd_fread (cartfile, a7800_cartridge_rom, len);
        osd_fclose (cartfile);
    }
    /* Super Cart */
    else {
        /* Extra ROM at $4000 */
        if (a7800_cart_type & 0x08) {
            osd_fread(cartfile,&(ROM[0x4000]),0x4000);
            len -= 0x4000;
        }
        a7800_cartridge_rom = &(ROM[0x10000]);
        osd_fread (cartfile, a7800_cartridge_rom, len);
        osd_fclose (cartfile);
        memcpy(&(ROM[0x8000]),&(ROM[0x10000]), 0x4000);
        memcpy(&(ROM[0xC000]),&(ROM[0x2C000]), 0x4000);
        if (!(a7800_cart_type & 0x08)) {
            memcpy(&(ROM[0x4000]),&(ROM[0x28000]), 0x4000);
        }
    }

    memcpy(a7800_cart_f000,&(ROM[0xF000]),0x1000);
    memcpy(&(ROM[0xF000]),a7800_bios_f000,0x1000);
    cpu_setbank(1,memory_region(REGION_CPU1) + 0x8000);

    return 0;
}

/******  TIA  *****************************************/

READ_HANDLER( a7800_TIA_r ) {
    switch (offset) {
        case 0x08:
              return ((input_port_1_r(0) & 0x02) << 6);
        case 0x09:
              return ((input_port_1_r(0) & 0x08) << 4);
        case 0x0A:
              return ((input_port_1_r(0) & 0x01) << 7);
        case 0x0B:
              return ((input_port_1_r(0) & 0x04) << 5);
        case 0x0c:
            if ((input_port_1_r(0) & 0x08) || (input_port_1_r(0) & 0x02))
                return 0x00;
            else
                return 0x80;
        case 0x0d:
            if ((input_port_1_r(0) & 0x01) || (input_port_1_r(0) & 0x04))
                return 0x00;
            else
                return 0x80;
        default:
            logerror("undefined TIA read %x\n",offset);

    }
    return 0xFF;
}

WRITE_HANDLER( a7800_TIA_w ) {
    switch(offset) {
        case 0x01:
            if (data & 0x01) {
                maria_flag=1;
            }
            if (!a7800_ctrl_lock) {
                a7800_ctrl_lock = data & 0x01;
                a7800_ctrl_reg = data;
                if (data & 0x04)
               		memcpy(&(ROM[0xF000]),a7800_cart_f000,0x1000);
                else
               		memcpy(&(ROM[0xF000]),a7800_bios_f000,0x1000);
            }
        break;
    }
    tia_w(offset,data);
    ROM[offset] = data;
}

/****** RIOT ****************************************/

READ_HANDLER( a7800_RIOT_r ) {
    switch (offset) {
        case 0:
            if (a7800_stick_type == 0x01)
                return input_port_0_r(0);
            else
                return ((input_port_1_r(0) & 0x02) << 3);

        case 2:
            return input_port_3_r(0);
        default:
            logerror("undefined RIOT read %x\n",offset);
    }
    return 0xFF;
}

WRITE_HANDLER( a7800_RIOT_w ) {
}


/****** RAM Mirroring ******************************/

READ_HANDLER( a7800_MAINRAM_r ) {
    return ROM[0x2000 + offset];
}

WRITE_HANDLER( a7800_MAINRAM_w ) {
    ROM[0x2000 + offset] = data;
}

READ_HANDLER( a7800_RAM0_r ) {
    return ROM[0x2040 + offset];
}

WRITE_HANDLER( a7800_RAM0_w ) {
    ROM[0x2040 + offset] = data;
    ROM[0x40 + offset] = data;
}

READ_HANDLER( a7800_RAM1_r ) {
    return ROM[0x2140 + offset];
}

WRITE_HANDLER( a7800_RAM1_w ) {
    ROM[0x2140 + offset] = data;
}


WRITE_HANDLER( a7800_cart_w ) {

    if (offset < 0x4000) {
        if (a7800_cart_type & 0x04) {
            ROM[0x4000 + offset] = data;
        }
        else if (a7800_cart_type & 0x01) {
            pokey1_w(offset,data);
        }
        else {
            logerror("Undefined write A: %x",offset + 0x4000);
        }
    }
    if (offset > 0x3FFF) {
        if (a7800_cart_type & 0x02) {
            data &= 0x07;
            cpu_setbank(1,memory_region(REGION_CPU1) + 0x10000 + (data << 14));
/*            logerror("BANK SEL: %d\n",data); */
       }
    }
}

