
#include <stdio.h>
#include "driver.h"
#include "machine/sms.h"

unsigned char sms_page_count;
unsigned char sms_fm_detect;
unsigned char sms_version;

WRITE_HANDLER ( sms_fm_detect_w )
{
    sms_fm_detect = (data & 1);
}

READ_HANDLER ( sms_fm_detect_r )
{
    return ( readinputport(3) & 1 ? sms_fm_detect : 0x00 );
}

WRITE_HANDLER ( sms_version_w )
{
    sms_version = (data & 0xA0);
}

READ_HANDLER ( sms_version_r )
{
    unsigned char temp;

    /* Move bits 7,5 of port 3F into bits 7, 6 */
    temp = (sms_version & 0x80) | (sms_version & 0x20) << 1;

    /* Inverse version detect value for Japanese machines */
    if(readinputport(3) & 2) temp ^= 0xC0;

    /* Merge version data with input port #2 data */
    temp = (temp & 0xC0) | (readinputport(1) & 0x3F);

    return (temp);
}

WRITE_HANDLER ( sms_mapper_w )
{
    unsigned char *RAM = memory_region(REGION_CPU1);

    offset &= 3;
    data %= sms_page_count;

    RAM[0xDFFC + offset] = data;
    RAM[0xFFFC + offset] = data;

    switch(offset)
    {
        case 0: /* Control */
            break;

        case 1: /* Select 16k ROM bank for 0000-3FFF */
            memcpy(&RAM[0x0000], &RAM[0x10000 + (data * 0x4000)], 0x3C00);
            break;

        case 2: /* Select 16k ROM bank for 4000-7FFF */
            memcpy(&RAM[0x4000], &RAM[0x10000 + (data * 0x4000)], 0x4000);
            break;

        case 3: /* Select 16k ROM bank for 8000-BFFF */
            memcpy(&RAM[0x8000], &RAM[0x10000 + (data * 0x4000)], 0x4000);
            break;
    }
}

WRITE_HANDLER ( sms_cartram_w )
{
}

WRITE_HANDLER ( sms_ram_w )
{
    unsigned char *RAM = memory_region(REGION_CPU1);
    RAM[0xC000 + (offset & 0x1FFF)] = data;
    RAM[0xE000 + (offset & 0x1FFF)] = data;
}

WRITE_HANDLER ( gg_sio_w )
{
    logerror("*** write %02X to SIO register #%d\n", data, offset);

    switch(offset & 7)
    {
        case 0x00: /* Parallel Data */
            break;

        case 0x01: /* Data Direction/ NMI Enable */
            break;

        case 0x02: /* Serial Output */
            break;

        case 0x03: /* Serial Input */
            break;

        case 0x04: /* Serial Control / Status */
            break;
    }
}

READ_HANDLER ( gg_sio_r )
{
    logerror("*** read SIO register #%d\n", offset);

    switch(offset & 7)
    {
        case 0x00: /* Parallel Data */
            break;

        case 0x01: /* Data Direction/ NMI Enable */
            break;

        case 0x02: /* Serial Output */
            break;

        case 0x03: /* Serial Input */
            break;

        case 0x04: /* Serial Control / Status */
            break;
    }

    return (0x00);
}

WRITE_HANDLER ( gg_psg_w )
{
    /* D7 = Noise Left */
    /* D6 = Tone3 Left */
    /* D5 = Tone2 Left */
    /* D4 = Tone1 Left */

    /* D3 = Noise Right */
    /* D2 = Tone3 Right */
    /* D1 = Tone2 Right */
    /* D0 = Tone1 Right */
}


/****************************************************************************/
static int sms_verify_cart (char * magic)
{
	//const char *sysname;
	int retval;

	//sysname = Machine->gamedrv->name;
	//printf ("Driver is %s\n",sysname);


	retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	if (!strncmp(&magic[0x7ff0],"TMR SEGA",8))
	{
		/* Technically, it should be this, but remove for now until verified:
		if (!strcmp(sysname, "gamegear"))
		{
			if ((unsigned char)magic[0x7ffd] < 0x50)
				retval = IMAGE_VERIFY_PASS;
		}
		if (!strcmp(sysname, "sms"))
		{
			if ((unsigned char)magic[0x7ffd] >= 0x50)
				retval = IMAGE_VERIFY_PASS;
		}
		*/
		retval = IMAGE_VERIFY_PASS;
	}

	/* Check at $81f0 also */
	if (!retval)
	{
		if (!strncmp(&magic[0x81f0],"TMR SEGA",8))
		{
			/* Technically, it should be this, but remove for now until verified:
			if (!strcmp(sysname, "gamegear"))
			{
				if ((unsigned char)magic[0x81fd] < 0x50)
					retval = IMAGE_VERIFY_PASS;
			}
			if (!strcmp(sysname, "sms"))
			{
				if ((unsigned char)magic[0x81fd] >= 0x50)
					retval = IMAGE_VERIFY_PASS;
			}
			*/
			retval = IMAGE_VERIFY_PASS;
		}
	}

	return retval;
}

int sms_init_cart(int id)
{
    int size, ret;
    FILE *handle;
    unsigned char *RAM;

    /* Ensure filename was specified */
    if(device_filename(IO_CARTSLOT,id) == NULL)
    {
        printf("Cartridge Name Required!\n");
        return (INIT_FAIL);
	}

    /* Ensure filename was specified */
    handle = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0);
    if(handle == NULL)
    {
		printf("Cartridge Name Required!\n");
        return (INIT_FAIL);
	}

    /* Get file size */
    size = osd_fsize(handle);

    /* Check for 512-byte header */
    if((size / 512) & 1)
    {
        osd_fseek(handle, 512, SEEK_SET);
        size -= 512;
    }

    /* Allocate memory */
    ret = new_memory_region(REGION_CPU1, size,0);

    /* Oops.. couldn't do it */
    if(ret)
    {
        printf("Error allocating %d bytes.\n", size);
        return INIT_FAIL;
    }

    /* Get base of CPU1 memory region */
    RAM = memory_region(REGION_CPU1);

    /* Load ROM banks */
    size = osd_fread(handle, &RAM[0x10000], size);

    /* Close file */
    osd_fclose(handle);

	/* check the image */
	if (sms_verify_cart((char*)&RAM[0x10000])==IMAGE_VERIFY_FAIL)
	{
		logerror("Invalid Image\n");
		return INIT_FAIL;
	}

    /* Get 16K page count */
    sms_page_count = (size / 0x4000);

    /* Load up first 32K of image */
    memcpy(&RAM[0x0000], &RAM[0x10000], 0x4000);
    memcpy(&RAM[0x4000], &RAM[0x14000], 0x4000);
    memcpy(&RAM[0x8000], &RAM[0x10000], 0x4000);

    return (INIT_PASS);
}

void sms_init_machine (void)
{
    sms_fm_detect = 0;
}


