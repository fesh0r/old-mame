/*
This file is a set of function calls and defs required for MESS.
It doesnt do much at the moment, but its here in case anyone
needs it ;-)
*/

#include "driver.h"
#include "messwin.h"
#include <ctype.h>
#include <dirent.h>

#ifndef _MSC_VER
#include <sys/stat.h>
#include <dos.h>
#include <unistd.h>
#endif /* _MSC_VER */

/* fronthlp functions */
extern int strwildcmp(const char *sp1, const char *sp2);


/* This function contains all the -list calls from fronthlp.c for MESS */
/* Currently Supported: */
/*   -listdevices       */
/*   -listtext       	*/

void list_mess_info(char *gamename, char *arg, int listclones)
{

	int i, j;

	/* -listdevices */
	if (!stricmp(arg, "-listdevices"))
	{

		i = 0;
		j = 0;


		printf(" SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
		printf("----------  --------------------  ------------------------------------\n");

		while (drivers[i])
		{
			const struct IODevice *dev = drivers[i]->dev;

			if (!strwildcmp(gamename, drivers[i]->name))
			{
				int devcount = 1;

				printf("%-13s", drivers[i]->name);

				/* if IODevice not used, print UNKNOWN */
				if (dev->type == IO_END)
					printf("%-12s\n", "UNKNOWN");

				/* else cycle through Devices */
				while (dev->type != IO_END)
				{
					const char *src = dev->file_extensions;

					if (devcount == 1)
						printf("%-12s(%s)   ", device_typename(dev->type), device_brieftypename(dev->type));
					else
						printf("%-13s%-12s(%s)   ", "    ", device_typename(dev->type), device_brieftypename(dev->type));

					devcount++;

					while (src && *src)
					{

						printf(".%-5s", src);
						src += strlen(src) + 1;
					}
					dev++;			   /* next IODevice struct */
					printf("\n");
				}


			}
			i++;

		}

	}

	/* -listtext */
	else if (!stricmp(arg, "-listtext"))
	{
		printf("                   ==========================================\n" );
		printf("                    M.E.S.S.  -  Multi-Emulator Super System\n"  );
		printf("                             Copyright (C) 1998-2001\n");
		printf("                                by the MESS team\n"    );
		printf("                    Official Page at: http://mess.emuverse.com\n");
		printf("                   ==========================================\n\n" );

		printf("This document is generated for MESS %s\n\n",build_version);

		printf("Please note that many people helped with this project, either directly or by\n"
		       "releasing source code which was used to write the drivers. We are not trying to\n"
		       "appropriate merit which isn't ours. See the acknowledgents section for a list\n"
			   "of contributors, however please note that the list is largely incomplete. See\n"
			   "also the CREDITS section in the emulator to see the people who contributed to a\n"
			   "specific driver. Again, that list might be incomplete. We apologize in advance\n"
			   "for any omission.\n\n"

			   "All trademarks cited in this document are property of their respective owners.\n"

			   "Especially, the MESS team would like to thank Nicola Salmoria and the MAME team\n"
			   "for letting us play with their code and, in fact, incorporating MESS specific\n"
			   "code into MAME.  Without it, MESS would be substantially less than what it is\n"
			   "right now! ;-)\n\n"

			   "Usage and Distribution Licence:\n"
			   "===============================\n"
			   "- MESS usage and distribution follows that of MAME.  Please read the MAME\n"
			   "  readme.txt file distributed with MESS for further information.\n\n"

			   "How to Contact The MESS Team\n"
			   "============================\n"
			   "Visit the web page at http://mess.emuverse.com to see a list of contributers\n"
			   "If you have comments, suggestions or bug reports about an existing driver, check\n"
			   "the page contacts section to find who has worked on it, and send comments to that \n"
			   "person. If you are not sure who to contact, write to Ben (ben@mame.net)\n"
			   "who is the current coordinator of the MESS project [Win Console]. \n\n"

			   "PLEASE DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST! \n"

			   "THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses\n"
			   "*will* be ignored. Please understand that this is a *free* project, mostly\n"
			   "targeted at experienced users. We don't have the resources to provide end user\n"
			   "support. Basically, if you can't get the emulator to work, you are on your own.\n"
			   "First of all, read this doc carefully. If you still can't find an answer to\n"
			   "your question, try checking the beginner's sections that many emulation pages\n"
			   "have, or ask on the appropriate Usenet newsgroups (e.g. comp.emulators.misc)\n"
			   "or on the many emulation message boards.  The official MESS message board is at:\n"
			   "   http://mess.emuverse.com\n\n");


		printf("Also, please DO NOT SEND REQUESTS FOR NEW SYSTEMS TO ADD, unless you have some original\n");
		printf("info on the hardware or, even better, have the technical expertise needed to\n");
		printf("help us. Please don't send us information widely available on the Internet -\n");
		printf("we are perfectly capable of finding it ourselves, thank you.\n\n\n");


		printf("Complete Emulated System List\n");
		printf("=============================\n");
		printf("Here is the list of systems supported by MESS %s\n",build_version);
		if (!listclones)
			printf("Variants of the same system are not included, you can use the -listclones command\n"
				"to get a list of the alternate versions of a given system.\n");
		printf("\n"
			   "The meanings of the columns are as follows:\n"
			   "Working - \"No\" means that the emulation has shortcomings that cause the system\n"
			   "  not to work correctly. This can be anywhere from just showing a black screen\n"
			   "  to not being playable with major problems.\n"
			   "Correct Colors - \"Yes\" means that colors should be identical to the original,\n"
			   "  \"Close\" that they are very similar but wrong in places, \"No\" that they are\n"
			   "  completely wrong. \n"
			   "Sound - \"Partial\" means that sound support is either incomplete or not entirely\n"
			   "  accurate. \n"
			   "Internal Name - This is the unique name that should be specified on the command\n"
			   "  line to run the system. ROMs must be placed in the ROM path, either in a .zip\n"
			   "  file or in a subdirectory of the same name. The former is suggested, because\n"
			   "  the files will be identified by their CRC instead of requiring specific\n"
			   "  names.  NOTE! that as well as required ROM files to emulate the system, you may\n"
			   "  also attach IMAGES of files created for system specific devices (some examples of \n"
			   "  devices are cartridges, floppydisks, harddisks, etc).  See below for a complete list\n"
			   "  of a systems supported devices and common file formats used for that device\n\n");

		printf("System Information can be obtained from the SysInfo.dat file (online in the MESS UI\n"
			   "from the Machine history) or sysinfo.htm.  To generate sysinfo.htm, execute \n"
			   "dat2html.exe.\n\n\n");

		printf("+-----------------------------------------+-------+-------+-------+----------+\n");
		printf("|                                         |       |Correct|       | Internal |\n");
		printf("| System Name                             |Working|Colors | Sound |   Name   |\n");
		printf("+-----------------------------------------+-------+-------+-------+----------+\n");



			/* Generate the System List */

			 i = 0;
			while (drivers[i])
			{

				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					char name[200],name_ref[200];

					strcpy(name,drivers[i]->description);

					/* Move leading "The" to the end */
					if (strstr(name," (")) *strstr(name," (") = 0;
					if (strncmp(name,"The ",4) == 0)
					{
						sprintf(name_ref,"%s, The ",name+4);
					}
					else
						sprintf(name_ref,"%s ",name);

					/* print the additional description only if we are listing clones */
					if (listclones)
					{
						if (strchr(drivers[i]->description,'('))
							strcat(name_ref,strchr(drivers[i]->description,'('));
					}

					//printf("| %-33.33s",name_ref);
					printf("| %-40.40s",name_ref);

					if (drivers[i]->flags & GAME_NOT_WORKING)
					{
						const struct GameDriver *maindrv;
						int foundworking;

						if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
							maindrv = drivers[i]->clone_of;
						else maindrv = drivers[i];

						foundworking = 0;
						j = 0;
						while (drivers[j])
						{
							if (drivers[j] == maindrv || drivers[j]->clone_of == maindrv)
							{
								if ((drivers[j]->flags & GAME_NOT_WORKING) == 0)
								{
									foundworking = 1;
									break;
								}
							}
							j++;
						}

						if (foundworking)
							printf("| No(1) ");
						else
							printf("|   No  ");
					}
					else
						printf("|  Yes  ");

					if (drivers[i]->flags & GAME_WRONG_COLORS)
						printf("|   No  ");
					else if (drivers[i]->flags & GAME_IMPERFECT_COLORS)
						printf("| Close ");
					else
						printf("|  Yes  ");

					{
						const char **samplenames = 0;
						for (j = 0;drivers[i]->drv->sound[j].sound_type && j < MAX_SOUND; j++)
						{

						#if (HAS_SAMPLES)
							if (drivers[i]->drv->sound[j].sound_type == SOUND_SAMPLES)
							{
								samplenames = ((struct Samplesinterface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
								break;
							}
						#endif
						}
						if (drivers[i]->flags & GAME_NO_SOUND)
							printf("|   No  ");
						else if (drivers[i]->flags & GAME_IMPERFECT_SOUND)
						{
							if (samplenames)
								printf("|Part(2)");
							else
								printf("|Partial");
						}
						else
						{
							if (samplenames)
								printf("| Yes(2)");
							else
								printf("|  Yes  ");
						}
					}

					printf("| %-8s |\n",drivers[i]->name);
				}
				i++;
			}

			printf("+-----------------------------------------+-------+-------+-------+----------+\n");
			printf("(1) There are variants of the system that work correctly\n");
			printf("(2) Needs samples provided separately\n\n\n");


		printf("QUICK MESS USAGE GUIDE!\n"
		       "=======================\n"
		       "In order to use MESS, you must at least specify at the command line\n\n"
               "      MESS <system>\n\n"
			   "This will emulate the system requested.  Note that most systems require the BIOS for\n"
			   "emulation.  These system BIOS ROM files are copyright and ARE NOT supplied with MESS.\n\n"
			   "To use files created for the system emulated (SOFTWARE), MESS works by attaching these\n"
			   "files created for the particular device of that system, for example, a cartridge,\n"
               "floppydisk, harddisk, cassette, software etc.  Therefore, in order to attach software to the\n"
			   "system, you must specify at the command line:\n\n"
               "      MESS <system> <device> <software_name>\n\n"
			   "To manually manipulate the emulation options, you must specify:\n\n"
               "      MESS <system> <device> <software_name> <options>\n\n");
		printf("*For a complete list of systems emulated,  use: MESS -listfull\n"
			   "*For system files (BIOS) required by each system, use: MESS <system> -listroms\n"
			   "*See below for valid device names and usage.\n"
			   "*See the MAME readme.txt and below for a detailed list of options.\n\n"
			   "Make sure you have BIOS and SOFTWARE in a subdirectory specified in mess.cfg\n\n\n");
		printf("Examples:\n\n"
			   "    MESS nes -cart zelda.nes\n"
			   "        will attach zelda.nes to the cartridge device and run MESS in\n"
			   "        the following way:\n"
			   "        <system>        = nes             (Nintendo Entertainment System)\n"
			   "        <device>        = CARTRIDGE\n"
			   "        <software_name> = zelda.nes       (Zelda cartridge)\n"
			   "        <options>       = none specified, so default options (see mess.cfg)\n\n"
			   "    MESS coleco -cart dkong -soundcard 0\n"
			   "        will run MESS in the following way:\n"
			   "        <system>        = coleco          (ColecoVision)\n"
			   "        <device>        = CARTRIDGE\n"
			   "        <software_name> = dkong.rom       (Donkey Kong cartridge)\n"
			   "        <options>       = default options without sound (see mess.cfg)\n\n"
			   "    MESS trs80 -flop boot.dsk -flop arcade1.dsk\n"
			   "        will run MESS in the following way:\n"
			   "        <system>         = trs80           (TRs-80 model 1)\n"
			   "        <device1>        = FLOPPYDISK\n"
			   "        <software_name1> = boot.dsk        (The Trs80 boot floppy diskl)\n"
			   "        <device2>        = FLOPPYDISK\n"
			   "        <software_name2> = arcade1.dsk     (floppy Disk which contains games)\n"
			   "        <options>        = default options (all listed in mess.cfg)\n\n"
			   "    MESS cgenie -flop games1\n"
			   "        will run the system Colour Genie with one disk loaded,\n"
			   "        automatically appending the file extension .dsk.\n\n\n");


		printf("To EXIT the emulator, press ESC.  If the emulated system is a computer, \n"
		       "you may need to toggle the UI mode (use the SCR_LOCK key).\n"
		       "The on-screen display shows the current UI mode when SCR_LOCK is pressed.\n\n");


		printf("To automatically create the individual system directories in the \n"
		       "SOFTWARE folder, use:\n"
		       "    MESS -createdir\n\n\n");

		printf("DEVICE support list\n");
		printf("===================\n");
		printf("As mentioned, in order to fully utilise MESS, you will need to attach software files\n"
			   "to the system devices.  To obtain a full list for all the devices and software \n"
			   "file extensions currently supported by a system in MESS, Use:\n"
			   "    MESS -listdevices\n\n\n");

		printf("KEYS: see readme.txt\n"
		       "====================\n"
		       "      ESC     - Exit emulator (providing UI is enabled - see below)\n"
		       "      ScrLOCK - Toggle UI for computer systems\n");


	}

    else if (!stricmp(arg, "-createdir"))
	{
	/***************************************************/
	/* To create the SOFTWARE directories */
		char* sys_rom_path  = "SOFTWARE";
		char buf[128];
		int d=0;

			/* ensure the roms directory exists! */
			sprintf(buf,"%s %s","md",sys_rom_path);
			printf("%s\n",buf);
			system(buf);

			/* create directory for all currently supported drivers */
			while(drivers[d])
			{
				/* create the systems directory */
				sprintf(buf,"%s %s\\%s","md",sys_rom_path,drivers[d]->name);
				printf("%s\n",buf);
				system(buf);
				d++;
			}
	}


}

int osd_select_file(int sel, char *filename)
{
	return 0;
}


