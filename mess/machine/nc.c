/***************************************************************************

  nc.c

  TODO:

        - Allow different card sizes
***************************************************************************/

#include "driver.h"
#include "includes/nc.h"

unsigned char *nc_card_ram;

/* load image */
int nc_load(int type, int id, unsigned char **ptr)
{
	void *file;

	file = image_fopen(type, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				/* read whole file */
				osd_fread(file, data, datasize);

				*ptr = data;

				/* close file */
				osd_fclose(file);

				logerror("File loaded!\r\n");

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}

/* load pcmcia card */
int nc_pcmcia_card_load(int id)
{
        if (nc_load(IO_CARTSLOT,id,&nc_card_ram))
	{

                nc_set_card_present_state(1);

		return INIT_OK;
	}

	return INIT_FAILED;
}

/* check if pcmcia card is valid  */
int nc_pcmcia_card_id(int id)
{
        /* for now it's valid */
        return 1;
}

void nc_pcmcia_card_exit(int id)
{
        if (nc_card_ram!=NULL)
        {
                free(nc_card_ram);
                nc_card_ram = NULL;
        }

        nc_set_card_present_state(0);
}
