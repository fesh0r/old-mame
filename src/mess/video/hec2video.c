/////////////////////////////////////////////////////////////////////////////////
///// Hector vid??o
/////////////////////////////////////////////////////////////////////////////////
/*      Hector 2HR+
        Victor
        Hector 2HR
        Hector HRX
        Hector MX40c
        Hector MX80c
        Hector 1
        Interact

        12/05/2009 Skeleton driver - Micko : mmicko@gmail.com
        31/06/2009 Video - Robbbert

        29/10/2009 Update skeleton to functional machine
                          by yo_fr       (jj.stac@aliceadsl.fr)

               => add Keyboard,
               => add color,
               => add cassette,
               => add sn76477 sound and 1bit sound,
               => add joysticks (stick, pot, fire)
               => add BR/HR switching
               => add bank switch for HRX
               => add device MX80c and bank switching for the ROM
    Importante note : the keyboard function add been piked from
                      DChector project : http://dchector.free.fr/ made by DanielCoulom
                      (thank's Daniel)
    TODO : Add the cartridge function,
           Adjust the one shot and A/D timing (sn76477)
*/

#include "emu.h"
#include "sound/sn76477.h"   // for sn sound

#include "includes/hec2hrp.h"

// Color status
UINT8 hector_color[4];  // For actual colors
// Ram video BR :
UINT8 hector_videoram[0x04000];
// Status for screen definition
UINT8 hector_flag_hr;
UINT8 hector_flag_80c;



static void Init_Hector_Palette( running_machine  *machine)
{
	// basic colors !
	hector_color[0] = 0;  // fond (noir)
	hector_color[1] = 1;  // HECTOR HRX (rouge)
	hector_color[2] = 7; // Point interrogation (Blanc)
	hector_color[3] = 3; // Ecriture de choix (jaune)

	// Color initialisation : full luminosit??
	palette_set_color( machine, 0,MAKE_RGB(000,000,000));//Noir
	palette_set_color( machine, 1,MAKE_RGB(255,000,000));//Rouge
	palette_set_color( machine, 2,MAKE_RGB(000,255,000));//Vert
	palette_set_color( machine, 3,MAKE_RGB(255,255,000));//Jaune
	palette_set_color( machine, 4,MAKE_RGB(000,000,255));//Bleu
	palette_set_color( machine, 5,MAKE_RGB(255,000,255));//Magneta
	palette_set_color( machine, 6,MAKE_RGB(000,255,255));//Cyan
	palette_set_color( machine, 7,MAKE_RGB(255,255,255));//Blanc
	// 1/2 luminosit??
	palette_set_color( machine, 8,MAKE_RGB(000,000,000));//Noir
	palette_set_color( machine, 9,MAKE_RGB(128,000,000));//Rouge
	palette_set_color( machine,10,MAKE_RGB(000,128,000));//Vert
	palette_set_color( machine,11,MAKE_RGB(128,128,000));//Jaune
	palette_set_color( machine,12,MAKE_RGB(000,000,128));//Bleu
	palette_set_color( machine,13,MAKE_RGB(128,000,128));//Magneta
	palette_set_color( machine,14,MAKE_RGB(000,128,128));//Cyan
	palette_set_color( machine,15,MAKE_RGB(128,128,128));//Blanc
}

void hector_hr(bitmap_t *bitmap, UINT8 *page, int ymax, int yram)
{
	UINT8 gfx,y;
	UINT16 sy=0,ma=0,x;
	for (y = 0; y <= ymax; y++) {  //224
		UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);
		for (x = ma; x < ma + yram; x++) {  // 64
			gfx = *(page+x);
			/* Display a scanline of a character (4 pixels !) */
			switch (gfx & 0x03) {
			     case 0x00 : *p=hector_color[0]; break;
			     case 0x01 : *p=hector_color[1]; break;
			     case 0x02 : *p=hector_color[2]; break;
			     default   : *p=hector_color[3]; break;}
			p++;

			switch (gfx & 0x0c)	{
			     case 0x00 : *p=hector_color[0]; break;
			     case 0x04 : *p=hector_color[1]; break;
			     case 0x08 : *p=hector_color[2]; break;
			     default   : *p=hector_color[3]; break;}
			p++;

			switch (gfx & 0x30)	{
			     case 0x00 : *p=hector_color[0]; break;
			     case 0x10 : *p=hector_color[1]; break;
			     case 0x20 : *p=hector_color[2]; break;
			     default   : *p=hector_color[3]; break;}
			p++;

			switch (gfx & 0xc0) {
			     case 0x00 : *p=hector_color[0]; break;
			     case 0x40 : *p=hector_color[1]; break;
			     case 0x80 : *p=hector_color[2]; break;
			     default   : *p=hector_color[3]; break;}
			p++;
		}
		ma+=yram;
	}
}

void hector_80c(bitmap_t *bitmap, UINT8 *page, int ymax, int yram)
{
	UINT8 gfx,y;
	UINT16 sy=0,ma=0,x;
	for (y = 0; y <= ymax; y++) {  //224
		UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);
		for (x = ma; x < ma + yram; x++) {  // 64
			gfx = *(page+x);
			/* Display a scanline of a character (8 pixels !) */
			*p= (gfx &0x01 ) ? 7 : 0 ;
			p++;
			*p= (gfx &0x02 ) ? 7 : 0 ;
			p++;
			*p= (gfx &0x04 ) ? 7 : 0 ;
			p++;
			*p= (gfx &0x08 ) ? 7 : 0 ;
			p++;
			*p= (gfx &0x10 ) ? 7 : 0 ;
			p++;
			*p= (gfx &0x20 ) ? 7 : 0 ;
			p++;
			*p= (gfx &0x40 ) ? 7 : 0 ;
			p++;
			*p= (gfx &0x80 ) ? 7 : 0 ;
			p++;
		}
		ma+=yram;
	}
}

VIDEO_START( hec2hrp )
{
    Init_Hector_Palette(machine);
}

VIDEO_UPDATE( hec2hrp )
{
    if (hector_flag_hr==1)
        {
           if (hector_flag_80c==0)
           {
              video_screen_set_visarea(screen, 0, 243, 0, 227);
              hector_hr( bitmap , &hector_videoram[0], 227, 64);
           }
           else
           {
               video_screen_set_visarea(screen, 0, 243*2, 0, 227);
               hector_80c( bitmap , &hector_videoram[0], 227, 64);
           }
        }
	else
        {
            video_screen_set_visarea(screen, 0, 113, 0, 75);
            hector_hr( bitmap, screen->machine->generic.videoram.u8,  77, 32);
        }

    return 0;
}

