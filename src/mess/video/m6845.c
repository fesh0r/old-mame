/************************************************************************
    crct6845

    MESS Driver By:

    Gordon Jefferyes
    mess_bbc@gjeffery.dircon.co.uk

 ************************************************************************/

#include "emu.h"
#include "video/m6845.h"

#define True 1
#define False 0

#define Cursor_Start_Delay_Flag 1
#define Cursor_On_Flag 2
#define Display_Enabled_Delay_Flag 4
#define Display_Disable_Delay_Flag 8

/* total number of chr -1 */
#define R0_horizontal_total		crtc.registers[0]
/* total number of displayed chr */
#define R1_horizontal_displayed	crtc.registers[1]
/* position of horizontal Sync pulse */
#define R2_horizontal_sync_position crtc.registers[2]
/* HSYNC & VSYNC width */
#define R3_sync_width crtc.registers[3]
/* total number of character rows -1 */
#define R4_vertical_total crtc.registers[4]
/* *** Not implemented yet ***
R5 Vertical total adjust
This 5 bit write only register is programmed with the fraction
for use in conjunction with register R4. It is programmed with
a number of scan lines. If can be varied
slightly in conjunction with R4 to move the whole display area
up or down a little on the screen.
BBC Emulator: It is usually set to 0 except
when using mode 3,6 and 7 in which it is set to 2
*/
#define R5_vertical_total_adjust crtc.registers[5]
/* total number of displayed chr rows */
#define R6_vertical_displayed crtc.registers[6]
/* position of vertical sync pulse */
#define R7_vertical_sync_position crtc.registers[7]
/* *** Part not implemented ***
R8 interlace settings
Interlace mode (bits 0,1)
Bit 1   Bit 0   Description
0       0       Normal (non-interlaced) sync mode
1       0       Normal (non-interlaced) sync mode
0       1       Interlace sync mode
1       1       Interlace sync and video
*/
#define R8_interlace_display_enabled crtc.registers[8]
/* scan lines per character -1 */
#define R9_scan_lines_per_character crtc.registers[9]
/* *** Part not implemented yet ***
R10 The cursor start register
Bit 6   Bit 5
0       0       Solid cursor
0       1       No cursor (This no cursor setting is working)
1       0       slow flashing cursor
1       1       fast flashing cursor
*/
#define R10_cursor_start crtc.registers[10]
/* cursor end row */
#define R11_cursor_end crtc.registers[11]
/* screen start high */
#define R12_screen_start_address_H crtc.registers[12]
/* screen start low */
#define R13_screen_start_address_L crtc.registers[13]
/* Cursor address high */
#define R14_cursor_address_H crtc.registers[14]
/* Cursor address low */
#define R15_cursor_address_L crtc.registers[15]
/* *** Not implemented yet *** */
#define R16_light_pen_address_H crtc.registers[16]
/* *** Not implemented yet *** */
#define R17_light_pen_address_L crtc.registers[17]

typedef struct m6845_state
{
	const struct m6845_interface *intf;
	m6845_personality_t personality;

	/* Register Select */
	int address_register;
	/* register data */
	int registers[32];
	/* vertical and horizontal sync widths */
	int vertical_sync_width, horizontal_sync_width;

	int screen_start_address;         /* = R12<<8 + R13 */
	int cursor_address;				  /* = R14<<8 + R15 */
	int light_pen_address;			  /* = R16<<8 + R17 */

	int scan_lines_increment;

	int Horizontal_Counter;
	int Horizontal_Counter_Reset;

	int Scan_Line_Counter;
	int Scan_Line_Counter_Reset;

	int Character_Row_Counter;
	int Character_Row_Counter_Reset;

	int Horizontal_Sync_Width_Counter;
	int Vertical_Sync_Width_Counter;

	int HSYNC;
	int VSYNC;

	int Vertical_Total_Adjust_Active;
	int Vertical_Total_Adjust_Counter;

	int Memory_Address;
	int Memory_Address_of_next_Character_Row;
	int Memory_Address_of_this_Character_Row;

	int Horizontal_Display_Enabled;
	int Vertical_Display_Enabled;
	int Display_Enabled;
	int Display_Delayed_Enabled;

	int Cursor_Delayed_Status;

	int Cursor_Flash_Count;

	int Delay_Flags;
	int Cursor_Start_Delay;
	int Display_Enabled_Delay;
	int Display_Disable_Delay;

	int	Vertical_Adjust_Done;

#if 0
	/* timer to set vsync */
	emu_timer *vsync_set_timer;
	/* timer to reset vsync */
	emu_timer *vsync_clear_timer;

	int cycles_to_vsync_start;
	int cycles_to_vsync_end;
#endif
} m6845_state;


static m6845_state crtc;

#if 0
/* VSYNC functions */

static TIMER_CALLBACK( m6845_vsync_set_timer_callback );
static TIMER_CALLBACK( m6845_vsync_clear_timer_callback );
static void m6845_remove_vsync_set_timer(void);
static void m6845_remove_vsync_clear_timer(void);
static void m6845_set_new_vsync_set_time(int);
static void m6845_set_new_vsync_clear_time(int);
static void m6845_recalc_cycles_to_vsync_start(void);
static void m6845_recalc_cycles_to_vsync_end(void);

#endif

/* set up the local copy of the 6845 external procedure calls */
void m6845_config(const struct m6845_interface *intf)
{
	memset(&crtc, 0, sizeof(crtc));
	crtc.intf = intf;
	crtc.personality = M6845_PERSONALITY_GENUINE;
	m6845_reset();
}

/* 6845 registers */

/* functions to set the 6845 registers */
void m6845_address_w(int offset, int data)
{
	crtc.address_register=data & 0x1f;
}

void m6845_set_address(int address)
{
	crtc.Memory_Address_of_next_Character_Row = address;
	crtc.Memory_Address_of_this_Character_Row = address;
	crtc.Memory_Address = address;
}


void m6845_register_w(int offset, int data)
{
	switch (crtc.address_register)
	{
		case 0:
                        R0_horizontal_total=data & 0x0ff;
//                        m6845_recalc_cycles_to_vsync_end();
  //                      m6845_recalc_cycles_to_vsync_start();
			break;
		case 1:
                        R1_horizontal_displayed=data & 0x0ff;
			break;
		case 2:
                        R2_horizontal_sync_position=data & 0x0ff;
			break;
		case 3:
                {
                        /* if 0 is programmed, vertical sync width is 16 */
                        crtc.vertical_sync_width = (data>>4) & 0x0f;

                        if (crtc.vertical_sync_width == 0)
                           crtc.vertical_sync_width = 16;

                        R3_sync_width=data;

                        crtc.horizontal_sync_width = data & 0x0f;

    //                                            m6845_recalc_cycles_to_vsync_end();
				}
                break;

        case 4:
                        R4_vertical_total=data&0x7f;
          //              m6845_recalc_cycles_to_vsync_start();
			break;
		case 5:
			R5_vertical_total_adjust=data&0x1f;


        //                m6845_recalc_cycles_to_vsync_start();

			break;
		case 6:
			R6_vertical_displayed=data&0x7f;
			break;
		case 7:
			R7_vertical_sync_position=data&0x7f;
      //                  m6845_recalc_cycles_to_vsync_start();
			break;
		case 8:
			R8_interlace_display_enabled=data&0xf3;
			crtc.scan_lines_increment=((R8_interlace_display_enabled&0x03)==3)?2:1;
			break;
		case 9:
			R9_scan_lines_per_character=data&0x1f;
            //            m6845_recalc_cycles_to_vsync_start();
			break;
		case 10:
			R10_cursor_start=data&0x7f;
			break;
		case 11:
			R11_cursor_end=data&0x1f;
			break;
		case 12:
			R12_screen_start_address_H=data&0x3f;
			crtc.screen_start_address=(R12_screen_start_address_H<<8)+R13_screen_start_address_L;
			break;
		case 13:
			R13_screen_start_address_L=data;
			crtc.screen_start_address=(R12_screen_start_address_H<<8)+R13_screen_start_address_L;
			break;
		case 14:
			R14_cursor_address_H=data&0x3f;
			crtc.cursor_address=(R14_cursor_address_H<<8)+R15_cursor_address_L;
			break;
		case 15:
			R15_cursor_address_L=data;
			crtc.cursor_address=(R14_cursor_address_H<<8)+R15_cursor_address_L;
			break;
		case 16:
			/* light pen H  (read only) */
			break;
		case 17:
			/* light pen L  (read only) */
			break;
		default:
			break;
	}
}


int m6845_register_r(int offset)
{
	int retval;

	switch (crtc.address_register)
	{
		case 12:
			switch(crtc.personality)
			{
				case M6845_PERSONALITY_UM6845:
				case M6845_PERSONALITY_HD6845S:
				case M6845_PERSONALITY_AMS40489:
				case M6845_PERSONALITY_PREASIC:
					/* not sure about M6845_PERSONALITY_PREASIC */
					retval=R12_screen_start_address_H;
					break;

				default:
					retval = 0;
					break;
			}
			break;
		case 13:
			switch(crtc.personality)
			{
				case M6845_PERSONALITY_UM6845:
				case M6845_PERSONALITY_HD6845S:
				case M6845_PERSONALITY_AMS40489:
				case M6845_PERSONALITY_PREASIC:
					/* not sure about M6845_PERSONALITY_PREASIC */
					retval=R13_screen_start_address_L;
					break;

				default:
					retval = 0;
					break;
			}
			break;
		case 14:
			retval=R14_cursor_address_H;
			break;
		case 15:
			retval=R15_cursor_address_L;
			break;
		case 16:
			retval=R16_light_pen_address_H;
			break;
		case 17:
			retval=R17_light_pen_address_L;
			break;
		default:
			retval = 0;
			break;
	}
	return retval;
}


void m6845_reset(void)
{
	memset(&crtc.registers[0], 0, sizeof(int)*32);
	crtc.address_register = 0;
	crtc.scan_lines_increment = 1;
	crtc.Horizontal_Counter = 0;
	crtc.Horizontal_Counter_Reset = True;
	crtc.Scan_Line_Counter = 0;
	crtc.Scan_Line_Counter_Reset = True;
	crtc.Character_Row_Counter = 0;
	crtc.Character_Row_Counter_Reset = True;
	crtc.Horizontal_Sync_Width_Counter=0;
	crtc.Vertical_Sync_Width_Counter=0;
	crtc.HSYNC=False;
	crtc.VSYNC=False;
	crtc.vertical_sync_width = 0;
	crtc.horizontal_sync_width = 0;
	crtc.Memory_Address=0;
	crtc.Memory_Address_of_next_Character_Row=0;
	crtc.Memory_Address_of_this_Character_Row=0;
	crtc.Horizontal_Display_Enabled=False;
	crtc.Vertical_Display_Enabled=False;
	crtc.Display_Enabled=False;
	crtc.Display_Delayed_Enabled=False;
	crtc.Cursor_Delayed_Status=False;
	crtc.Cursor_Flash_Count=0;
	crtc.Delay_Flags=0;
	crtc.Cursor_Start_Delay=0;
	crtc.Display_Enabled_Delay=0;
	crtc.Display_Disable_Delay=0;
	crtc.cursor_address =0 ;
	crtc.Vertical_Total_Adjust_Active = False;
	crtc.Vertical_Total_Adjust_Counter = 0;
	crtc.Vertical_Adjust_Done = False;
}

/* called when the internal horizontal display enabled or the
vertical display enabled changed to set up the real
display enabled output (which may be delayed 0,1 or 2 characters */
static void check_display_enabled(void)
{
	int Next_Display_Enabled;


	Next_Display_Enabled=crtc.Horizontal_Display_Enabled&crtc.Vertical_Display_Enabled;
	if ((Next_Display_Enabled) && (!crtc.Display_Enabled))
	{
		crtc.Display_Enabled_Delay=(R8_interlace_display_enabled>>4)&0x03;
		if (crtc.Display_Enabled_Delay<3)
		{
			crtc.Delay_Flags=crtc.Delay_Flags | Display_Enabled_Delay_Flag;
		}
	}
	if ((!Next_Display_Enabled) && (crtc.Display_Enabled))
	{
		crtc.Display_Disable_Delay=(R8_interlace_display_enabled>>4)&0x03;
		crtc.Delay_Flags=crtc.Delay_Flags | Display_Disable_Delay_Flag;
	}
	crtc.Display_Enabled=Next_Display_Enabled;
}

static void m6845_restart_frame(void)
{
	/* no restart frame */
	/* End of All Vertical Character rows */
	crtc.Scan_Line_Counter = 0;
	crtc.Character_Row_Counter=0;
	crtc.Vertical_Display_Enabled=True;
	check_display_enabled();

	/* KT - As it stands it emulates the UM6845R well */
	crtc.Memory_Address=(crtc.Memory_Address_of_this_Character_Row=crtc.screen_start_address);
	/* HD6845S/MC6845 */
	crtc.Memory_Address_of_next_Character_Row = crtc.Memory_Address;
}

void m6845_frameclock(void)
{
	crtc.Cursor_Flash_Count=(crtc.Cursor_Flash_Count+1)%50;
}

/* clock the 6845 */
void m6845_clock(running_machine *machine)
{
	/* KT - I think the compiler might generate bogus code when using "%" operator! */
	/*crtc.Memory_Address=(crtc.Memory_Address+1)%0x4000;*/
	crtc.Memory_Address=(crtc.Memory_Address+1)&0x03fff;

	/*crtc.Horizontal_Counter=(crtc.Horizontal_Counter+1)%256;*/
	crtc.Horizontal_Counter=(crtc.Horizontal_Counter+1)&0x0ff;

	if (crtc.Horizontal_Counter_Reset)
	{
		/* End of a Horizontal scan line */
		crtc.Horizontal_Counter=0;
		crtc.Horizontal_Counter_Reset=False;
		crtc.Horizontal_Display_Enabled=True;
		check_display_enabled();

		crtc.Memory_Address=crtc.Memory_Address_of_this_Character_Row;

		/* Vertical clock pulse (R0 CO out) */
		/*crtc.Scan_Line_Counter=(crtc.Scan_Line_Counter+crtc.scan_lines_increment)%32;*/
		crtc.Scan_Line_Counter=(crtc.Scan_Line_Counter+crtc.scan_lines_increment)&0x01f;

		if (crtc.Vertical_Total_Adjust_Active)
		{
			/* update counter */
			crtc.Vertical_Total_Adjust_Counter = (crtc.Vertical_Total_Adjust_Counter+1) & 0x01f;
		}


                /* Vertical Sync Clock Pulse (In Vertical control) */
                if (crtc.VSYNC)
                {
                        crtc.Vertical_Sync_Width_Counter=(crtc.Vertical_Sync_Width_Counter+1);	// & 0x0f;
                }

		if (crtc.Scan_Line_Counter_Reset)
		{
			/* End of a Vertical Character row */
			crtc.Scan_Line_Counter=0;
			crtc.Scan_Line_Counter_Reset=False;
			crtc.Memory_Address=(crtc.Memory_Address_of_this_Character_Row=crtc.Memory_Address_of_next_Character_Row);

			/* Character row clock pulse (R9 CO out) */
/*          crtc.Character_Row_Counter=(crtc.Character_Row_Counter+1)%128;*/
			crtc.Character_Row_Counter=(crtc.Character_Row_Counter+1)&0x07f;
			if (crtc.Character_Row_Counter_Reset)
			{
				crtc.Character_Row_Counter_Reset=False;

				/* if vertical adjust is set, the first time it will do the vertical, adjust, the
                next time, it will not do it and complete the frame */

				/* vertical adjust set? */
				if (R5_vertical_total_adjust!=0)
				{
					/* it's active */
					//crtc.Vertical_Adjust_Done = TRUE;

					crtc.Vertical_Total_Adjust_Active = TRUE;
					crtc.Vertical_Total_Adjust_Counter = 0;
				}
				else
				{
					m6845_restart_frame();
				}

	        }

			/* Check for end of All Vertical Character rows */
			if (crtc.Character_Row_Counter==R4_vertical_total)
			{
				if (!(crtc.Vertical_Total_Adjust_Active))
				{
					crtc.Character_Row_Counter_Reset=True;
				}
			}

			/* Check for end of Displayed Vertical Character rows */
			if (crtc.Character_Row_Counter==R6_vertical_displayed)
			{
				crtc.Vertical_Display_Enabled=False;
				check_display_enabled();
			}


			/* Check for start of Vertical Sync Pulse */
			if (crtc.Character_Row_Counter==R7_vertical_sync_position)
			{
				crtc.VSYNC=True;
				if (crtc.intf->out_VS_func) (crtc.intf->out_VS_func)(machine, 0,crtc.VSYNC); /* call VS update */
			}


		}

                /* KT - Moved here because VSYNC length is in scanlines */
                if (crtc.VSYNC)
                {
                        /* Check for end of Vertical Sync Pulse */
                        if (crtc.Vertical_Sync_Width_Counter==crtc.vertical_sync_width)
                        {
                                crtc.Vertical_Sync_Width_Counter=0;
                                crtc.VSYNC=False;
                                if (crtc.intf->out_VS_func) (crtc.intf->out_VS_func)(machine, 0,crtc.VSYNC); /* call VS update */
                        }
                }


		/* vertical total adjust active? */
		if (crtc.Vertical_Total_Adjust_Active)
		{
			/* equals r5? */
			if (crtc.Vertical_Total_Adjust_Counter==R5_vertical_total_adjust)
			{
				/* not active, clear counter and restart frame */
				crtc.Vertical_Total_Adjust_Active = False;
				crtc.Vertical_Total_Adjust_Counter = 0;
	//          /* cause a scan-line counter reset, and a character row counter reset.
	//          i.e. restart frame */
	//          crtc.Scan_Line_Counter_Reset = TRUE;
	//          crtc.Character_Row_Counter_Reset = TRUE;

				// KT this caused problems when R7 == 0 and R5 was set!
				m6845_restart_frame();

				/* Check for start of Vertical Sync Pulse */
				if (crtc.Character_Row_Counter==R7_vertical_sync_position)
				{
					crtc.VSYNC=True;
					if (crtc.intf->out_VS_func) (crtc.intf->out_VS_func)(machine,0,crtc.VSYNC); /* call VS update */
				}
			}
		}


		/* Check for end of Vertical Character Row */
		if (crtc.Scan_Line_Counter==R9_scan_lines_per_character)
		{
			crtc.Scan_Line_Counter_Reset=True;
		}
		if (crtc.intf->out_RA_func) (crtc.intf->out_RA_func)(machine,0,crtc.Scan_Line_Counter); /* call RA update */
	}
	/* end of vertical clock pulse */

	/* Check for end of Horizontal Scan line */
	if (crtc.Horizontal_Counter==R0_horizontal_total)
	{
		crtc.Horizontal_Counter_Reset=True;
	}

	/* Check for end of Display Horizontal Scan line */
	if (crtc.Horizontal_Counter==R1_horizontal_displayed)
	{
		crtc.Memory_Address_of_next_Character_Row=crtc.Memory_Address;
		crtc.Horizontal_Display_Enabled=False;
		check_display_enabled();
	}

	/* Horizontal Sync Clock Pulse (Clk) */
	if (crtc.HSYNC)
	{
		crtc.Horizontal_Sync_Width_Counter=(crtc.Horizontal_Sync_Width_Counter+1) & 0x0f;
	}

	/* Check for start of Horizontal Sync Pulse */
	if (crtc.Horizontal_Counter==R2_horizontal_sync_position)
	{
                /* KT - If horizontal sync width is 0, on UM6845R/HD6845S
                no hsync is generated */
                if (crtc.horizontal_sync_width!=0)
                {
                        crtc.HSYNC=True;
                        if (crtc.intf->out_HS_func) (crtc.intf->out_HS_func)(machine, 0,crtc.HSYNC); /* call HS update */
                }
	}

        if (crtc.HSYNC)
        {
                /* Check for end of Horizontal Sync Pulse */
                if (crtc.Horizontal_Sync_Width_Counter==crtc.horizontal_sync_width)
                {

                        crtc.Horizontal_Sync_Width_Counter=0;
                        crtc.HSYNC=False;
                        if (crtc.intf->out_HS_func) (crtc.intf->out_HS_func)(machine, 0,crtc.HSYNC); /* call HS update */
                }
        }
	if (crtc.intf->out_MA_func) (crtc.intf->out_MA_func)(machine,0,crtc.Memory_Address);	/* call MA update */



	/* *** cursor checks still to be done *** */
	if (crtc.Memory_Address==crtc.cursor_address)
	{
		if ((crtc.Scan_Line_Counter>=(R10_cursor_start&0x1f)) && (crtc.Scan_Line_Counter<=R11_cursor_end) && (crtc.Display_Enabled))
		{
			crtc.Cursor_Start_Delay=(R8_interlace_display_enabled>>6)&0x03;
			if (crtc.Cursor_Start_Delay<3) crtc.Delay_Flags=crtc.Delay_Flags | Cursor_Start_Delay_Flag;
		}
	}


    /* all the cursor and delay flags are stored in one byte so that we can very quickly (for speed) check if anything
       needs doing with them, if any are on then we need to do more longer test to find which ones */
	if (crtc.Delay_Flags)
	{
        /* if the cursor is on, then turn it off on the next clock */
		if (crtc.Delay_Flags & Cursor_On_Flag)
		{
			crtc.Delay_Flags=crtc.Delay_Flags^Cursor_On_Flag;
			crtc.Cursor_Delayed_Status=False;
			if (crtc.intf->out_CR_func) (crtc.intf->out_CR_func)(machine,0,crtc.Cursor_Delayed_Status); /* call CR update */
			if (crtc.intf->out_CRE_func) (crtc.intf->out_CRE_func)(machine,0,0); /* call CRE update */
		}

		/* cursor enabled delay */
		if (crtc.Delay_Flags & Cursor_Start_Delay_Flag)
		{
			crtc.Cursor_Start_Delay-=1;
			if (crtc.Cursor_Start_Delay<0)
			{
				if ((R10_cursor_start&0x60)!=0x20)
				{
					crtc.Delay_Flags=(crtc.Delay_Flags^Cursor_Start_Delay_Flag)|Cursor_On_Flag;
					crtc.Cursor_Delayed_Status=True;
					if ((crtc.intf->out_CR_func)&&(crtc.Cursor_Flash_Count>25)) (crtc.intf->out_CR_func)(machine,0,crtc.Cursor_Delayed_Status); /* call CR update */
					if (crtc.intf->out_CRE_func) (crtc.intf->out_CRE_func)(machine,0,2|((crtc.Cursor_Flash_Count>25)&1)); /* call CR update */
				}
			}
		}

    	/* display enabled delay */
		if (crtc.Delay_Flags & Display_Enabled_Delay_Flag)
		{
			crtc.Display_Enabled_Delay-=1;
			if (crtc.Display_Enabled_Delay<0)
			{
				crtc.Delay_Flags=crtc.Delay_Flags^Display_Enabled_Delay_Flag;
				crtc.Display_Delayed_Enabled=True;
				if (crtc.intf->out_DE_func) (crtc.intf->out_DE_func)(machine,0,crtc.Display_Delayed_Enabled); /* call DE update */
			}
		}

		/* display disable delay */
		if (crtc.Delay_Flags & Display_Disable_Delay_Flag)
		{
			crtc.Display_Disable_Delay-=1;
			if (crtc.Display_Disable_Delay<0)
			{
				crtc.Delay_Flags=crtc.Delay_Flags^Display_Disable_Delay_Flag;
				crtc.Display_Delayed_Enabled=False;
				if (crtc.intf->out_DE_func) (crtc.intf->out_DE_func)(machine,0,crtc.Display_Delayed_Enabled); /* call DE update */
			}
		}
	}

}

/* functions to read the 6845 outputs */

int m6845_memory_address_r(int offset)  { return crtc.Memory_Address; }    /* MA = Memory Address output */
int m6845_row_address_r(int offset)     { return crtc.Scan_Line_Counter; } /* RA = Row Address output */
int m6845_horizontal_sync_r(int offset) { return crtc.HSYNC; }             /* HS = Horizontal Sync */
int m6845_vertical_sync_r(int offset)   { return crtc.VSYNC; }             /* VS = Vertical Sync */
int m6845_display_enabled_r(int offset) { return crtc.Display_Delayed_Enabled; }   /* DE = Display Enabled */
int m6845_cursor_enabled_r(int offset)  { return crtc.Cursor_Delayed_Status; }             /* CR = Cursor Enabled */

void m6845_set_personality(m6845_personality_t p)
{
	crtc.personality = p;
}

// A few helper functions to help avoid heavy use of m6845_get/set_state()
int m6845_get_register(int reg)
{
	return crtc.registers[reg];
}

int m6845_get_scanline_counter()
{
	return crtc.Scan_Line_Counter;
}

int m6845_get_row_counter()
{
	return crtc.Character_Row_Counter;
}


#if 0

/* KT:

  The following bit of code uses timers to set/reset the vsync output of the 6845.
  If a function has been setup in the interface it will be executed with the new vsync
  value.

  */

/* calculate the number of cycles to the next vsync */
/* ignores Reg 5! - to be completed */
int     m6845_cycles_to_vsync(void)
{
	/* passed vertical sync position */
	int cycles_per_frame;

	int cycles_into_frame;
	int scans_per_character = R9_scan_lines_per_character+1;
	int chars_per_line = R0_horizontal_total+1;
	int cycles_to_vsync_start;

	/* calculate current position into frame as char cycles */
	/* scans into frames */
	cycles_into_frame  = crtc.Character_Row_Counter*scans_per_character;
	cycles_into_frame += crtc.Scan_Line_Counter;
	/* scans into frames as char cycles */
	cycles_into_frame *= chars_per_line;
	/* total cycles into frame */
	cycles_into_frame += crtc.Horizontal_Counter;

	/* cycles to vsync start as char cycles */
	cycles_to_vsync_start = R7_vertical_sync_position*scans_per_character;
	cycles_to_vsync_start *= chars_per_line;


	if (cycles_into_frame<cycles_to_vsync_start)
	{
		/* not gone past vertical sync yet! */
		return cycles_to_vsync_start - cycles_into_frame;
	}

	cycles_per_frame = (R4_vertical_total+1)*scans_per_character*chars_per_line;

	return (cycles_per_frame - cycles_into_frame) + cycles_to_vsync_start;
}

/* calculate the number of CRTC cycles for VSYNC */
int     m6845_cycles_to_vsync_end(void)
{
        /* if we are in vsync */
        if (crtc.VSYNC)
        {
           return (R0_horizontal_total+1)*(crtc.vertical_sync_width-crtc.Vertical_Sync_Width_Counter);
        }

        return m6845_cycles_to_vsync() + ((R0_horizontal_total+1)*crtc.vertical_sync_width);
}


/* number of crtc cycles for whole frame */
static int m6845_cycles_per_frame(void)
{
	int scan_lines_per_character = R9_scan_lines_per_character+1;
	int chars_per_line = R0_horizontal_total+1;

	return /* time for all rows */
			((R4_vertical_total+1)*chars_per_line*scan_lines_per_character)
			/* add time for scanlines in vertical adjust */
			+(R5_vertical_total_adjust*chars_per_line);
}

/* number of crtc cycles for vsync */
static int m6845_vsync_length_in_cycles(void)
{
	int length;

	/* if length is programmed as 0, actual length for hd6845s is 16 */
	length = crtc.vertical_sync_width;

	if (length==0)
	{
		length = 16;
	}

	/* cycles for vsync */
	return (R0_horizontal_total+1)*length;
}


/* remove "vsync set" timer */
static void m6845_remove_vsync_set_timer(void)
{
	if (crtc.vsync_set_timer!=NULL)
	{
		timer_adjust_oneshot(crtc.vsync_set_timer, attotime_never, 0);
	}
}

/* remove "vsync clear" timer */
static void m6845_remove_vsync_clear_timer(void)
{
	if (crtc.vsync_clear_timer != NULL)
	{
		timer_adjust_oneshot(crtc.vsync_clear_timer, attotime_never, 0);
	}
}

/* setup new time for "vsync set" */
static void m6845_set_new_vsync_set_time(int cycles)
{
	int crtc_cycles_to_vsync_start;

	/* get cycles to vsync start, or if vsync cannot be reached
    cycles will be -1 */
	crtc_cycles_to_vsync_start = cycles;

	m6845_remove_vsync_set_timer();

	//crtc.vsync_set_timer = timer_alloc(machine, m6845_vsync_set_timer_callback, NULL);
	if (crtc_cycles_to_vsync_start!=-1)
	{
		timer_adjust_oneshot(crtc.vsync_set_timer, ATTOTIME_IN_USEC(crtc_cycles_to_vsync_start), 0);
	}
}

/* setup new time for "vsync clear" */
static void m6845_set_new_vsync_clear_time(int cycles)
{
	int crtc_cycles_to_vsync_end;

	m6845_remove_vsync_clear_timer();

	/* get number of cycles to end of vsync */
	crtc_cycles_to_vsync_end = cycles;

	//crtc.vsync_clear_timer = timer_alloc(machine, m6845_vsync_clear_timer_callback, NULL);
	if (crtc_cycles_to_vsync_end!=-1)
	{
		timer_adjust_oneshot(crtc.vsync_clear_timer, ATTOTIME_IN_USEC(crtc_cycles_to_vsync_end), 0);
	}
}

/* for these two below, might be better to record the current cpu time, and use that
to recalculate where the start/end of the vsync will next occur! */

static TIMER_CALLBACK( m6845_vsync_clear_timer_callback )
{
	/* clear vsync */
	crtc.VSYNC = 0;

	/* call function to let emulation "know" */
	if (crtc.intf->out_VS_func) (crtc.intf->out_VS_func)(machine,0,crtc.VSYNC); /* call VS update */


	/* if we got to here the vsync has just ended */
	/* the next vsync will occur in cycles per frame - vsync length in cycles */
	/* this will work as long as the vsync length has not been reprogrammed while the vsync was active! */
	/* setup time for vsync set timer */
	m6845_set_new_vsync_set_time(m6845_cycles_per_frame()-m6845_vsync_length_in_cycles());

	/* prevent timer from being free'd and don't let it trigger again */
	timer_adjust_oneshot(crtc.vsync_clear_timer, attotime_never, 0);
}

/* called when vsync is set */
static TIMER_CALLBACK( m6845_vsync_set_timer_callback )
{
	/* set vsync */
	crtc.VSYNC = 1;

	/* call function to let emulation "know" */
	if (crtc.intf->out_VS_func) (crtc.intf->out_VS_func)(machine,0,crtc.VSYNC); /* call VS update */


	/* if we got to here the vsync has just been set, and has just started */
	/* the next timer will be in vsync length cycles unless it is reprogrammed as the VSYNC
    is active, in this case, the new vsync end will be re-calculated */

	/* setup time for vsync clear timer */
	m6845_set_new_vsync_clear_time( m6845_vsync_length_in_cycles());

	/* prevent timer from being free'd and don't let it trigger again */
	timer_reset(crtc.vsync_set_timer, attotime_never);
}
static void m6845_recalc_cycles_to_vsync_end(void)
{
	int cycles_to_vsync_end = m6845_cycles_to_vsync_end();

	/* if we're in vsync, the end is important, otherwise we are waiting for the next vsync
    to start. The start is not affected by the length of the vsync */
	if (crtc.VSYNC)
	{
		m6845_set_new_vsync_clear_time(cycles_to_vsync_end);
	}
}

static void m6845_recalc_cycles_to_vsync_start(void)
{
	int cycles_to_vsync_start = m6845_cycles_to_vsync();

	/* if we're not in vsync, the end is important, otherwise we are waiting for the end
    of the vsync */
	if (!crtc.VSYNC)
	{
		m6845_set_new_vsync_set_time(cycles_to_vsync_start);
	}
}

/* ignores r5 ! to be completed */
void m6845_recalc(int offset,int num_cycles)
{
	int cycles;
	int num_scan_lines;
	int scans_into_frame;
	int scan_lines_per_char = (R9_scan_lines_per_character+1);
	int scan_lines_per_frame = (R4_vertical_total+1)*scan_lines_per_char;
	cycles = num_cycles + crtc.Horizontal_Counter;

	/* calculate number of scan-lines */
	num_scan_lines = cycles/(R0_horizontal_total+1);
	num_scan_lines+=crtc.Scan_Line_Counter;
	/* set new horizontal counter */
	crtc.Horizontal_Counter = cycles % (R0_horizontal_total+1);

	scans_into_frame = crtc.Character_Row_Counter*scan_lines_per_char;

	while ((scans_into_frame+num_scan_lines)>=scan_lines_per_frame)
	{
		int scans_to_end_of_frame;

		scans_to_end_of_frame = scan_lines_per_frame - scans_into_frame;
		num_scan_lines -= scans_to_end_of_frame;
		scans_into_frame = 0;
		/* subtract R5 lines here! */

	}

	/* update row position */
	crtc.Character_Row_Counter = num_scan_lines/scan_lines_per_char;
	/* remainder is the scan line counter */
	crtc.Scan_Line_Counter = num_scan_lines % scan_lines_per_char;
}

#endif
