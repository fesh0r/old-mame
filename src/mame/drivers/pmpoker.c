/******************************************************************************

    PLAYMAN POKER
    -------------

    Driver by Roberto Fresca.


    Games running on this hardware:

    * PlayMan Poker (german).             1981, PlayMan.
    * Golden Poker Double Up (Big Boy).   1981, Bonanza Enterprises, Ltd.
    * Golden Poker Double Up (Mini Boy).  1981, Bonanza Enterprises, Ltd.
    * Jack Potten's Poker (set 1).        198?, Bootleg.
    * Jack Potten's Poker (set 2).        198?, Bootleg in Coinmaster H/W.
    * Jack Potten's Poker (set 3).        198?, Bootleg.
    * Jack Potten's Poker (set 4).        198?, Bootleg.
    * Jack Potten's Poker (set 5).        198?, Bootleg.
    * Jack Potten's Poker (set 6).        198?, Bootleg.
    * Good Luck.                          198?, Unknown.
    * Buena Suerte (spanish, set 1).      198?, Unknown.
    * Buena Suerte (spanish, set 2).      198?, Unknown.
    * Royale.                             198?, Unknown.
    * Witch Card.                         1991, Unknown.
    * Witch Card (spanish, set 1).        1991, Unknown.
    * Witch Card (spanish, set 2).        1991, Unknown.
    * Super Loco 93 (spanish, set 1).     1993, Unknown.
    * Super Loco 93 (spanish, set 2).     1993, Unknown.


*******************************************************************************


    I think "Diamond Poker Double Up" from Bonanza Enterprises should run on this hardware too.
    http://www.arcadeflyers.com/?page=thumbs&id=4539

    Big-Boy and Mini-Boy are different sized cabinets for Bonanza Enterprises games.
    http://www.arcadeflyers.com/?page=thumbs&id=4616
    http://www.arcadeflyers.com/?page=thumbs&id=4274


    Preliminary Notes (pmpoker):

    - This set was found as "unknown playman-poker".
    - The ROMs didn't match any currently supported set (0.108u2 romident switch).
    - All this driver was made using reverse engineering in the program roms.


    Game Notes:
    ==========

    * goldnpkr & goldnpkb:

    "How to play"... (from "Golden Poker Double Up" instruction card)

    1st GAME
    - Insert coin / bank note.
    - Push BET button, 1-10 credits multiple play.
    - Push DEAL/DRAW button.
    - Push HOLD buttons to hold cards you need.
    - Cards held can be cancelled by pushing CANCEL button.
    - Push DEAL/DRAW button to draw cards.

    2nd GAME - Double Up game
    - When you win, choose TAKE SCORE or DOUBLE UP game.
    - Bet winnings on
        "BIG (8 or more number)" or
        "SMALL (6 and less number)" of next one card dealt.
    - Over 5,000 winnings will be storaged automatically.


    ----- Learn Mode (settings) -----
    Press LEARN (F2) to enter the learn mode for settings.
    This is a timed function and after 30-40 seconds switch back to the game.

    Press DOUBLE UP to change Double Up 7 settings between 'Even' and 'Lose'.
    Press BET to adjust the maximum bet (20-200).
    Press HOLD4 for Meter Over (5000-50000).
    Press BIG to change Double Up settings (Normal-Hard).
    Press TAKE SCORE to set Half Gamble (Yes/No).
    Press SMALL to set win sound (Yes/No).
    Press HOLD1 to set coinage 1.
    Press HOLD2 to set coinage 2.
    Press HOLD3 to set coinage 3.
    Press HOLD5 to exit.

    ----- Meters Mode -----
    Press METER SW (9) to enter the Meters mode. You also can switch between interim
    and permanent meters using METER SW (only for goldnpkr).
    This is a timed function and after 30-40 seconds switch back to the game.

    To reset meters push CANCEL + SMALL buttons. HOLD5 to exit.
    In goldnpkr you can switch between Permanent/Interim Meters.
    In goldnpkb & pmpoker you can see only Permanent Meters.

    ----- Percentage Mode -----
    Press Meter SW (9), then DEAL/DRAW, to enter the percentage mode.
    Press HOLD4 to change the value following the table below (from manual). HOLD1 to exit.

    Number    Overall scoring percentage in the LONG RUN
    - - - - - - - - - - - - - - - - - - - - - - - - - - -
      0         about 85%
      1         about 30%
      2         about 40%
      3         about 50%

    ----- Test Mode -----
    Press Meter SW (9), then DEAL/DRAW, then HOLD5 to enter the test mode.
    After a RAM test, you can see an input test matrix. Press HOLD1+HOLD2+HOLD3 to exit
    entering into a video grid test. Press HOLD5 to exit.


    * Good Luck

    This hybrid runs on Witch Card hardware.
    Even when is shown on screen "Bet 1 to 10", you can bet up to 50.
    There are extra graphics for a couple of jokers, but they never are shown.
    Maybe some settings can enable the use of them...


    * Witch Card

    This game is derivated from Golden Poker.

    The hardware has a feature called BLUE KILLER.
    Using the original intensity line, the PCB has a bridge
    that allow (as default) turn the background black.

    Except goodluck, all other games running in this hardware
    were designed to wear black background.

    Settings:

    There are 12 parameters to program. All of them are unknown.
    To program them, use the HOLD keys, CANCEL key, DEAL + HOLD keys,
    and DEAL + CANCEL.


    * Super Loco 93

    I like this game!... This one has nothing to do with poker games.
    The objective is to get the higher sum with 2 or 3 cards. Is clearly
    based on a passage (Envido) of the famous argentine's game called "Truco".

    How to play?...

    Like in Truco's envido, You must add 20 to the sum of your cards.
    If you have 7 and 5 of the same kind, you have 7 + 5 + 20 = 32 points.

    "Flor", is 3 cards of the same kind.
    "Simple", is 2 cards of the same kind.

    HAND             WIN    DESCRIPTION
    ----------------------------------------------
    38  Corazones   1000    7, 6 and 5 of hearts.
    777 Loco Loco    200    3x sevens.
    38  Flor          80    7, 6 and 5 of the same kind (except hearts).
    37  Flor          20    7, 6 and 4 of the same kind.
    36  Flor          14    7, 5 and 4 of the same kind.
    35  Flor          10    6, 5 and 4 of the same kind.
    32-33-34 Flor      6    3 of the same kind that sum 32, 33 or 34.
    33  Simple         4    7 and 6 of the same kind.
    32  Simple         2    7 and 5 of the same kind.

    After bet (apuesta) some credits, Press deal (reparte) button.
    The game will deals 3 cards. You can discard up to all your 3 cards.
    Pressing the deal button again, new cards will appear in the place
    of the previously selected cards.

    Settings:

    There are 12 parameters to program.
    To program them, use the HOLD keys, CANCEL key, DEAL + HOLD keys,
    and DEAL + CANCEL.



*******************************************************************************


    Hardware Notes (pmpoker):

    - CPU:            1x M6502.
    - Video:          1x MC6845.
    - RAM:            4x uPD2114LC
    - I/O             2x 6821 PIAs.
    - prg ROMs:       3x 2732 (32Kb) or similar.
    - gfx ROMs:       4x 2716 (16Kb) or similar.
    - sound:          (discrete).
    - battery backup: 2x S8423


    PCB Layout (pmpoker): (XX) = unreadable.
     _______________________________________________________________________________
    |   _________                                                                   |
    |  |         |               -- DIP SW x8 --                                    |
    |  | Battery |   _________   _______________   _________  _________   ________  |
    |  |   055   |  | 74LS32  | |1|2|3|4|5|6|7|8| | HCF4011 || HCF4096 | | LM339N | |
    |  |_________|  |_________| |_|_|_|_|_|_|_|_| |_________||_________| |________| |
    |       _________     _________   _________   _________                         |
    |      | 74LS138 |   | S-8423  | | 74LS08N | | 74LS(XX)|                        |
    |      |_________|   |_________| |_________| |_________|                        |
    |  _______________    _________   ____________________                      ____|
    | |               |  | S-8423  | |                    |                    |
    | |     2732      |  |_________| |       6502P        |                    |
    | |_______________|   _________  |____________________|                    |
    |  _______________   |  7432   |  ____________________                     |____
    | |               |  |_________| |                    |                     ____|
    | |     2732      |   _________  |       6821P        |                     ____|
    | |_______________|  | 74LS157 | |____________________|                     ____|
    |  _______________   |_________|  ____________________                      ____|
    | |               |   _________  |                    |                     ____|
    | |     2732      |  | 74LS157 | |       6821P        |                     ____|
    | |_______________|  |_________| |____________________|                     ____|
    |  _______________    _________   ____________________                      ____|
    | |               |  | 74LS157 | |                    |                     ____|
    | |     2732      |  |_________| |       6845SP       |                     ____|
    | |_______________|   _________  |____________________|                     ____|
    |                    | 2114-LC |                                            ____| 28x2
    |                    |_________|                                            ____| connector
    |       _________     _________                                             ____|
    |      | 74LS245 |   | 2114-LC |                                            ____|
    |      |_________|   |_________|                                            ____|
    |       _________     _________               _________                     ____|
    |      | 74LS245 |   | 2114-LC |             | 74LS174 |                    ____|
    |      |_________|   |_________|             |_________|                    ____|
    |  ________________   _________   _________   _________                     ____|
    | |                | | 2114-LC | | 74LS08H | | TI (XX) | <-- socketed.      ____|
    | |      2716      | |_________| |_________| |_________|       PROM         ____|
    | |________________|              _________   _________                     ____|
    |  ________________              | 74LS04P | | 74LS174 |                    ____|
    | |                |             |_________| |_________|                    ____|
    | |      2716      |              _________   _________                     ____|
    | |________________|             | 74166P  | | 74LS86C |                    ____|
    |  ________________              |_________| |_________|                    ____|
    | |                |              _________    _______                     |
    | |      2716      |             | 74166P  |  | 555TC |                    |
    | |________________|             |_________|  |_______|                    |
    |  ________________                                                        |____
    | |                |                                                        ____|
    | |      2716      |              _________   _________      ________       ____| 5x2
    | |________________|             | 74166P  | |  7407N  |    | LM380N |      ____| connector
    |                                |_________| |_________|    |________|      ____|
    |  ________  ______               _________   _________      ___            ____|
    | | 74LS04 || osc. |             | 74LS193 | |  7407N  |    /   \          |
    | |________||10 MHz|             |_________| |_________|   | POT |         |
    |           |______|                                        \___/          |
    |__________________________________________________________________________|



    Some odds:

    - There are unused pieces of code like the following sub:

    78DE: 18         clc
    78DF: 69 07      adc  #$07
    78E1: 9D 20 10   sta  $1020,x
    78E4: A9 00      lda  #$00
    78E6: 9D 20 18   sta  $1820,x
    78E9: E8         inx

    78EA: 82         DOP        ; use of DOP (double NOP)
    78EB: A2 0A      dummy (ldx #$0A)

    78ED: AD 82 08   lda  $0882

    78F0: 82         DOP        ; use of DOP (double NOP)
    78F1: 48 08      dummy
    78F3: D0 F6      bne  $78EB ; branch to the 1st DOP dummy arguments (now ldx #$0A).
    78F5: CA         dex
    78F6: D0 F8      bne  $78F0
    78F8: 29 10      and  #$10
    78FA: 60         rts

    Offset $78EA and $78F0 contains an undocumented 6502 opcode (0x82).

    At beginning, I thought that the main processor was a 65sc816, since 0x82 is a documented opcode (BRL) for this CPU.
    Even the vector $FFF8 contain 0x09 (used to indicate "Emulation Mode" for the 65sc816).
    I dropped the idea following the code. Impossible to use BRL (branch relative long) in this structure.

    Some 6502 sources list the 0x82 mnemonic as DOP (double NOP), with 2 dummy bytes as argument.
    The above routine dynamically change the X register value using the DOP undocumented opcode.
    Since the opcode DOP in fact has only 1 dummy byte as argument, they apparently dropped this
    piece of code due to it didn't work as expected. Now all have sense.


*******************************************************************************


    -----------------------------------------------
    ***  Memory Map (pmpoker/goldnpkr hardware) ***
    -----------------------------------------------

    $0000 - $00FF   RAM     ; Zero Page (pointers and registers)

                            ; $45 to $47 - Coin settings.
                            ; $50 - Input port register.
                            ; $5C - Input port register.
                            ; $5D - Input port register.
                            ; $5E - Input port register.
                            ; $5F - Input port register.

    $0100 - $01FF   RAM     ; 6502 Stack Pointer.

    $0200 - $02FF   RAM     ; R/W. (settings)
    $0300 - $03FF   RAM     ; R/W (mainly to $0383). $0340 - $035f (settings).

    $0800 - $0801   MC6845  ; MC6845 use $0800 for register addressing and $0801 for register values.

                            *** pmpoker mc6845 init at $65B9 ***
                            *** goldnpkr mc6845 init at $5E75 ***
                            *** sloco93 mc6845 init at $D765 ***
                            register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
                            value:     0x27  0x20  0x22  0x02  0x1F  0x04  0x1D  0x1E  0x00  0x07  0x00  0x00  0x00  0x00  0x00  0x00  0x00  0x00.

                            *** goodluck mc6845 init at $527B ***
                            register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
                            value:     0x27  0x20  0x23  0x03  0x1F  0x04  0x1D  0x1F  0x00  0x00  0x26  0x00  0x20  0x22  0x58  0xA5  0x4F  0xC9.

                            *** royale mc6845 init at $6581 ***
                            register:   00    01    02    03    04    05    06    07    08    09    10    11    12    13    14    15    16    17
                            value:     0x27  0x20  0x23  0x03  0x1F  0x04  0x1D  0x1E  0x00  0x07  0x00  0x00  0x00  0x00  0x00  0x00  0x00  0x00.

    $0844 - $0847   PIA0    ; Muxed inputs and lamps. Initialized at $5000.
    $0848 - $084B   PIA1    ; Sound writes and muxed inputs selector. Initialized at $5000.

    $1000 - $13FF   Video RAM   ; Initialized in subroutine starting at $5042.
    $1800 - $1BFF   Color RAM   ; Initialized in subroutine starting at $5042.

    $4000 - $7FFF   ROM

    $8000 - $FFFF           ; Mirrored from $0000 - $7FFF due to lack of A15 line connection.



    ---------------------------------------
    ***  Memory Map (pottnpkr hardware) ***
    ---------------------------------------

    $0000 - $00FF   RAM     ; Zero Page (pointers and registers)
    $0100 - $01FF   RAM     ; 6502 Stack Pointer.
    $0200 - $02FF   RAM     ; R/W. (settings)
    $0300 - $03FF   RAM     ; R/W (mainly to $0383). $0340 - $035f (settings).

    $0800 - $0801   MC6845  ; MC6845 use $0800 for register addressing and $0801 for register values.

    $0844 - $0847   PIA0    ; Muxed inputs and lamps.
    $0848 - $084B   PIA1    ; Sound writes and muxed inputs selector.

    $1000 - $13FF   Video RAM
    $1800 - $1BFF   Color RAM

    $2000 - $3FFF   ROM space

    $4000 - $7FFF           ; Mirrored from $0000 - $3FFF due to lack of A14 & A15 lines connection.
    $8000 - $BFFF           ; Mirrored from $0000 - $3FFF due to lack of A14 & A15 lines connection.
    $C000 - $FFFF           ; Mirrored from $0000 - $3FFF due to lack of A14 & A15 lines connection.



*******************************************************************************


    Buttons/Inputs   goldnpkr goldnpkb  pmpoker  bsuerte goodluck pottnpkr potnpkra potnpkrc potnpkrb
    -------------------------------------------------------------------------------------------------

    HOLD (5 buttons)  mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped
    CANCEL            mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped
    BIG               mapped   mapped   mapped   mapped    ----     ----     ----     ----     ----
    SMALL             mapped   mapped   mapped   mapped    ----     ----     ----     ----     ----
    DOUBLE UP         mapped   mapped   mapped   mapped    ----     ----     ----     ----     ----
    TAKE SCORE        mapped   mapped   mapped   mapped    ----     ----     ----     ----     ----
    DEAL/DRAW         mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped
    BET               mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped

    Coin 1 (coins)    mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped
    Coin 2 (notes)    mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   fixed 1c-1c
    Coin 3 (coupons)  mapped   mapped   mapped   mapped    ----     ----     ----     ----     ----
    Payout            mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped
    Manual Collect    mapped   mapped   mapped    ----    mapped   mapped   mapped   mapped   mapped

    LEARN/SETTINGS    mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped
    METERS            mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped   mapped


    Inputs are different for some games. Normally each button has only one function.
    In pmpoker some buttons have different functions.


*******************************************************************************


    DRIVER UPDATES:


    [2006-09-02]

    - Initial release.


    [2006-09-06]

    - Understood the GFX banks:
        - 1 bank (1bpp) for text layer and minor graphics.
        - 1 bank (3bpp) for the undumped cards deck graphics.

    - Partially added inputs through 6821 PIAs.
        ("Bitte techniker rufen" error messages. Press 'W' to reset the machine)

    - Confirmed the CPU as 6502. (was in doubt due to use of illegal opcodes)


    [2006-09-15]

    - Confirmed the GFX banks (a complete dump appeared!).
    - Improved technical notes and added a PCB layout based on PCB picture.
    - Found and fixed the 3rd bitplane of BigBoy gfx.
    - Renamed Big-Boy to Golden Poker Double Up. (Big-Boy and Mini-Boy are names of cabinet models).
    - Added 'Joker Poker' (Golden Poker version without the 'double-up' feature).
    - Added 'Jack Potten's Poker' (same as Joker Poker, but with 'Aces or better' instead of jacks).
    - Simulated colors for all sets till color PROMs appear.
    - Fixed bit corruption in goldnpkr rom u40_4a.bin.
    - Completed inputs in all sets (except DIP switches).
    - Removed flags GAME_WRONG_COLORS and GAME_IMPERFECT_GRAPHICS in all sets.
    - Removed flag GAME_NOT_WORKING. All sets are now playable. :)


    [2006-10-09]

    - Added service/settings mode to pmpoker.
    - Added PORT_IMPULSE to manage correct timings for most inputs in all games.
      (jokerpkr still trigger more than 1 credit for coin pulse).


    [2007-02-01]

    - Crystal documented via #define.
    - CPU clock derived from #defined crystal value.
    - Replaced simulated colors with proper color prom decode.
    - Renamed "Golden Poker Double Up" to "Golden Poker Double Up (Big Boy)".
    - Added set Golden Poker Double Up (Mini Boy).
    - Cleaned up the driver a bit.
    - Updated technical notes.


    [2007-05-05]

    - Removed all inputs hacks.
    - Connected both PIAs properly.
    - Demuxed all inputs for each game.
    - Documented all outputs.
    - Added lamps support.
    - Created different layout files to cover each game.
    - Add NVRAM support to all games.
    - Corrected the color PROM status for each set.
    - Figured out most of the DIP switches.
    - Added diplocations to goldnpkb.
    - Replaced the remaining IPT_SERVICE with IPT_BUTTON for regular buttons.
    - Updated technical notes.
    - Cleaned up the driver. Now is better organized and documented.


    [2007-07-07]

    - Added set goldnpkc (Golden Poker without the double up feature).
    - Updated technical notes.


    [2008-10-12] *** REWRITE ***

    - Added discrete sound support to Golden Poker hardware games based on schematics.
    - Added discrete sound support to Potten's Poker hardware games based on PCB analysis.
    - Added discrete circuitry diagrams for both hardware types.
    - Adjusted the CPU adressing to 15 bits for pmpoker/goldenpkr hardware.
    - Adjusted the CPU adressing to 14 bits for pottnpkr hardware.
    - Rewrote all the ROM loads based on these changes.
    - Defined MASTER Xtal & CPU clock.
    - Fixed the visible area based on M6845 registers.
    - Improved the lamps layouts to be more realistic.
    - Added Good Luck (potten's poker hybrid running in goldnpkr hardware).
    - Added Buena Suerte (spanish) x 2 sets.
    - Added set Royale.
    - Added Witch Card and spanish variants.
    - Added Super Loco 93 (spanish) x 2 sets.
    - Renamed set goldnpkc to pottnpkr (parent Jack Potten's Poker set).
    - Renamed set jokerpkr to potnpkra, since is another Jack Potten's Poker set.
    - Added other 2 clones of Jack Potten's Poker.
    - Renamed/cleaned all sets based on code/hardware analysis.
    - Added intensity bit to the color system.
    - Implemented the blue killer bit for Witch Card hardware.
    - Implemented the extended graphics addressing bit for Witch Card hardware.
    - Added proper visible area to sloco93.
    - Rewrote the graphics & color decode system based on schematics. No more patched codes.
    - Changed the char gfx bank structure and rom load according to the new routines.
    - Adjusted the amount of color codes and PROM region size accordingly.
    - Updated all notes.


    TODO:

    - Analyze the code of other upcoming clones to see if worth the inclusion.
    - Inputs & lamps for Royale.
    - Final cleanup and split the driver.


*******************************************************************************/


#define MASTER_CLOCK	XTAL_10MHz
#define CPU_CLOCK		(MASTER_CLOCK/16)

#include "driver.h"
#include "video/mc6845.h"
#include "machine/6821pia.h"
#include "sound/discrete.h"

#include "pmpoker.lh"
#include "goldnpkr.lh"
#include "pottnpkr.lh"


/*************************
*     Video Hardware     *
*************************/

static tilemap *bg_tilemap;

static WRITE8_HANDLER( pmpoker_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static WRITE8_HANDLER( pmpoker_colorram_w )
{
	colorram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static TILE_GET_INFO( get_bg_tile_info )
{
/*  - bits -
    7654 3210
    --xx xx--   tiles color.
    ---- --x-   tiles bank.
    ---- ---x   tiles extended address (MSB).
    xx-- ----   unused.
*/

	int attr = colorram[tile_index];
	int code = ((attr & 1) << 8) | videoram[tile_index];
	int bank = (attr & 0x02) >> 1;	/* bit 1 switch the gfx banks */
 	int color = (attr & 0x3c) >> 2;	/* bits 2-3-4-5 for color */

	SET_TILE_INFO(bank, code, color, 0);
}

static VIDEO_START( pmpoker )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 8, 8, 32, 29);
}

static VIDEO_UPDATE( pmpoker )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}

static PALETTE_INIT( goldnpkr )
{
/*  prom bits
    7654 3210
    ---- ---x   red component.
    ---- --x-   green component.
    ---- -x--   blue component.
    ---- x---   intensity.
    xxxx ----   unused.
*/
	int i;

	/* 0000IBGR */
	if (color_prom == 0) return;

	for (i = 0;i < machine->config->total_colors;i++)
	{
		int bit0, bit1, bit2, r, g, b, inten, intenmin, intenmax;

		intenmin = 0xe0;
//      intenmin = 0xc2;    /* 2.5 Volts (75.757575% of the whole range) */
		intenmax = 0xff;	/* 3.3 Volts (the whole range) */

		/* intensity component */
		inten = (color_prom[i] >> 3) & 0x01;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		r = (bit0 * intenmin) + (inten * (bit0 * (intenmax - intenmin)));

		/* green component */
		bit1 = (color_prom[i] >> 1) & 0x01;
		g = (bit1 * intenmin) + (inten * (bit1 * (intenmax - intenmin)));

		/* blue component */
		bit2 = (color_prom[i] >> 2) & 0x01;
		b = (bit2 * intenmin) + (inten * (bit2 * (intenmax - intenmin)));


		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}
}

static PALETTE_INIT( witchcrd )
{
/*
    This hardware has a feature called BLUE KILLER.
    Using the original intensity line, the PCB has a bridge
    that allow (as default) turn the background black.

    Except goodluck, all other games running in this hardware
    were designed to wear black background.

    7654 3210
    ---- ---x   red component.
    ---- --x-   green component.
    ---- -x--   blue component.
    ---- x---   blue killer.
    xxxx ----   unused.
*/
	int i;

	/* 0000KBGR */

	if (color_prom == 0) return;

	for (i = 0;i < machine->config->total_colors;i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b, bk;

		/* blue killer (from schematics) */
        bit3 = (color_prom[i] >> 3) & 0x01;
        bk = bit3;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		r = (bit0 * 0xff);

		/* green component */
		bit1 = (color_prom[i] >> 1) & 0x01;
		g = (bit1 * 0xff);

		/* blue component */
		bit2 = (color_prom[i] >> 2) & 0x01;
		b = bk * (bit2 * 0xff);

		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}
}


/***********************
*     R/W Handlers     *
***********************/

static int input_selector = 0;

/* Inputs (buttons) are multiplexed.
   There are 4 sets of 5 bits each and are connected to PIA0, portA.
   The selector bits are located in PIA1, portB (bits 4-7).
*/
static READ8_HANDLER( mux_port_r )
{
	switch( input_selector )
	{
		case 0x10: return input_port_read(machine, "IN0-0");
		case 0x20: return input_port_read(machine, "IN0-1");
		case 0x40: return input_port_read(machine, "IN0-2");
		case 0x80: return input_port_read(machine, "IN0-3");
	}
	return 0xff;
}

static WRITE8_HANDLER( mux_port_w )
{
//  popmessage("written : %02X", data);
	input_selector = (data ^ 0xff) & 0xf0;	/* bits 4-7, inverted */
}

/***** Lamps wiring *****

    -------------------
    pmpoker
    -------------------
    L0 = Deal
    L1 = Hold3
    L2 = Hold1
    L3 = Hold5
    L4 = Hold2 & Hold4

    -----------------------
    goldnpkr sets & bsuerte
    -----------------------
    L0 = Bet
    L1 = Deal
    L2 = Holds (all)
    L3 = Double Up & Take
    L4 = Big & Small

    ----------------------------------
    pottnpkr sets, jokerpkr & goodluck
    ----------------------------------
    L0 = Bet
    L1 = Deal
    L2 = Holds (all)

    ------------------
    witchcrd & sloco93
    ------------------
    NONE. Just lack of lamps...

*/

static WRITE8_HANDLER( lamps_a_w )
{
	output_set_lamp_value(0, 1 - ((data) & 1));			/* Lamp 0 */
	output_set_lamp_value(1, 1 - ((data >> 1) & 1));	/* Lamp 1 */
	output_set_lamp_value(2, 1 - ((data >> 2) & 1));	/* Lamp 2 */
	output_set_lamp_value(3, 1 - ((data >> 3) & 1));	/* Lamp 3 */
	output_set_lamp_value(4, 1 - ((data >> 4) & 1));	/* Lamp 4 */

	coin_counter_w(0, data & 0x40);	/* Coin1 */
	coin_counter_w(1, data & 0x80);	/* Note */

/*  Counters:

    bit 5 = Coin out
    bit 6 = Coin counter
    bit 7 = Note counter (only goldnpkr use it)

    ONLY for witchcrd and sloco93 sets:

    bit3 = Coin counter (inverted).
    bit5 = Coin out (inverted).
*/
}

static WRITE8_HANDLER( sound_w )
{
	/* 555 voltage controlled */
	logerror("Sound Data: %2x\n",data & 0x0f);

	/* discrete sound is connected to PIA1, portA: bits 0-3 */
	discrete_sound_w(machine,NODE_01, data >> 3 & 0x01);
	discrete_sound_w(machine,NODE_10, data & 0x07);
}


/*************************
* Memory Map Information *
*************************/

static ADDRESS_MAP_START( pmpoker_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)	/* battery backed RAM */
	AM_RANGE(0x0800, 0x0800) AM_DEVWRITE(MC6845, "crtc", mc6845_address_w)
	AM_RANGE(0x0801, 0x0801) AM_DEVREADWRITE(MC6845, "crtc", mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x0844, 0x0847) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0848, 0x084b) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x1000, 0x13ff) AM_RAM_WRITE(pmpoker_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1800, 0x1bff) AM_RAM_WRITE(pmpoker_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pottnpkr_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)	/* battery backed RAM */
	AM_RANGE(0x0800, 0x0800) AM_DEVWRITE(MC6845, "crtc", mc6845_address_w)
	AM_RANGE(0x0801, 0x0801) AM_DEVREADWRITE(MC6845, "crtc", mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x0844, 0x0847) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0848, 0x084b) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x1000, 0x13ff) AM_RAM_WRITE(pmpoker_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1800, 0x1bff) AM_RAM_WRITE(pmpoker_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x2000, 0x3fff) AM_ROM
ADDRESS_MAP_END


/*************************
*      Input Ports       *
*************************/

static INPUT_PORTS_START( pmpoker )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Meters") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Payout") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1 / Take Score (Kasse)") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2 / Small (Tief)") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3 / Bet (Setze)") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4 / Big (Hoch)") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5 / Double Up (Dopp.)") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Settings") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_IMPULSE(3) PORT_NAME("Note 1 In")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin In")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )   PORT_NAME("Note 2 In")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Hohes Paar (Jacks or Better)" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )
	PORT_DIPSETTING(    0x40, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( goldnpkr )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Meters") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Double Up") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON13 ) PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON14 ) PORT_NAME("Off (Payout)") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Take") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON11 ) PORT_NAME("Big") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON12 ) PORT_NAME("Small") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Learn Mode") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("D-31") PORT_CODE(KEYCODE_E)	/* O.A.R? (D-31 in schematics) */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Coupon (Note In)")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin In")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )   PORT_NAME("Weight (Coupon In)")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	/* only bits 4-7 are connected here and were routed to SW1 1-4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "Jacks or Better" )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "50hz/60hz" )			PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, "50hz" )
	PORT_DIPSETTING(    0x00, "60hz" )
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )		PORT_DIPLOCATION("SW1:3")	/* listed in the manual as "Play Mode" */
	PORT_DIPSETTING(    0x40, "Manual" )			/*  listed in the manual as "Out Play" */
	PORT_DIPSETTING(    0x00, "Auto" )				/*  listed in the manual as "Credit Play" */
	PORT_DIPNAME( 0x80, 0x00, "Royal Flush" )		PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END

static INPUT_PORTS_START( bsuerte )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Apostar (Bet)") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Contabilidad (Meters)") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Doblar (Double Up)") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Dar/Virar (Deal/Draw)") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancelar (Cancel)") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON13 ) PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON14 ) PORT_NAME("Pagar (Payout)") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Cobrar (Take)") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON11 ) PORT_NAME("Mayor (Big)") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON12 ) PORT_NAME("Menor (Small)") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Configuracion (settings)") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Billetes (Note In)")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Fichas (Coin In)")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )   PORT_NAME("Cupones (Coupon In)")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	/* only bits 4-7 are connected here and were routed to SW1 1-4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "Jotas o Mejores" )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Modo de Pago" )		PORT_DIPLOCATION("SW1:3")	/* left as 'auto' */
	PORT_DIPSETTING(    0x40, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( goodluck )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Meters") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON9 )  PORT_IMPULSE(3) PORT_NAME("Manual Collect") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Payout") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Settings") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Note In")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin In")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	/* only bits 4-7 are connected here and were routed to SW1 1-4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "Jacks or Better" )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )		PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
INPUT_PORTS_END

static INPUT_PORTS_START( pottnpkr )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Meters") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Off (Payout)") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Learn Mode") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Coupon (Note In)")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin In")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	/* only bits 4-7 are connected here and were routed to SW1 1-4 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Jacks or Better" )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/* listed in the manual as "Play Mode" */
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )		PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, "Manual" )			/*  listed in the manual as "Out Play" */
	PORT_DIPSETTING(    0x00, "Auto" )				/*  listed in the manual as "Credit Play" */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( potnpkra )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin 1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Meters") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON9 )  PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Off (Payout)") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Settings") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Note in")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Jacks or Better" )	PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Royal Flush Value" )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, "250 by bet" )
	PORT_DIPSETTING(    0x00, "500 by bet" )
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )		PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( potnpkrc )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin 1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Meters") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON9 )  PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Off (Payout)") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Settings") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Note in")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Ace or Better" )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )		PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static INPUT_PORTS_START( witchcrd )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Meters") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Double Up") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Deal/Draw") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON13 ) PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON14 ) PORT_NAME("Off (Payout)") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Take") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON11 ) PORT_NAME("Big") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON12 ) PORT_NAME("Small") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Learn Mode") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("D-31") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Note In")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin In")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	/* only bits 4-7 are connected here and were routed to SW1 1-4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "Jacks or Better" )		PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Won Credits Counter" )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, "Show" )
	PORT_DIPSETTING(    0x00, "Hide" )
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )			PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )		PORT_DIPLOCATION("SW1:4")
	/* Note In is always 1 Note - 10 Credits */
	PORT_DIPSETTING(    0x00, "1 Coin - 1 Credit / 1 Note - 10 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin - 5 Credits / 1 Note - 10 Credits" )
INPUT_PORTS_END

static INPUT_PORTS_START( sloco93 )
	/* Multiplexed - 4x5bits */
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Apuesta/Bet") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bookkeeping") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Double Up") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Reparte/Deal") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Cancela/Cancel") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON13 ) PORT_IMPULSE(3) PORT_NAME("Out (Manual Collect)") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON14 ) PORT_NAME("Off (Payout)") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON10 ) PORT_NAME("Take") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON11 ) PORT_NAME("Big") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON12 ) PORT_NAME("Small") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Learn Mode") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("D-31") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )   PORT_NAME("Note In")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )   PORT_IMPULSE(3) PORT_NAME("Coin In")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("SW1")
	/* only bits 4-7 are connected here and were routed to SW1 1-4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "32 Simple" )				PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Won Credits Counter" )	PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, "Show" )
	PORT_DIPSETTING(    0x00, "Hide" )
	PORT_DIPNAME( 0x40, 0x00, "Payout Mode" )			PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, "Manual" )
	PORT_DIPSETTING(    0x00, "Auto" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )		PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, "1 Coin - 10 Credit / 1 Note - 120 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin - 100 Credits / 1 Note - 100 Credits" )
INPUT_PORTS_END

static INPUT_PORTS_START( royale )
	PORT_START("IN0-0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-1") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-3") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-4") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-5") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-6") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-7") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("0-8") PORT_CODE(KEYCODE_8)

	PORT_START("IN0-1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-1") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-2") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-3") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-4") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-5") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-6") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-7") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("1-8") PORT_CODE(KEYCODE_I)

	PORT_START("IN0-2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-1") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-2") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-3") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-4") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-5") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-6") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-7") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2-8") PORT_CODE(KEYCODE_K)

	PORT_START("IN0-3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-6") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-7") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3-8") PORT_CODE(KEYCODE_L)

	PORT_START("SW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/*************************
*    Graphics Layouts    *
*************************/

static const gfx_layout tilelayout =
{
	8, 8,
	RGN_FRAC(1,3),
	3,
	{ 0, RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


/******************************
* Graphics Decode Information *
******************************/

static GFXDECODE_START( pmpoker )
	GFXDECODE_ENTRY( "gfx1", 0, tilelayout, 0, 16 )
	GFXDECODE_ENTRY( "gfx2", 0, tilelayout, 0, 16 )
GFXDECODE_END


/***********************
*    PIA Interfaces    *
***********************/

static const pia6821_interface pia0_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	mux_port_r, 0, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */
	0, lamps_a_w, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};

static const pia6821_interface pia1_intf =
{
	/* PIA inputs: A, B, CA1, CB1, CA2, CB2 */
	input_port_4_r, 0, 0, 0, 0, 0,

	/* PIA outputs: A, B, CA2, CB2 */

	sound_w, mux_port_w, 0, 0,

	/* PIA IRQs: A, B */
	0, 0
};


/**************************************
*       Discrete Sound Routines       *
***************************************

    Golden Poker Double-Up discrete sound circuitry.
    ------------------------------------------------

           +12V                            .--------+---------------.  +5V
            |                              |        |               |   |
         .--+                              |        Z             +-------+
         |  |                             8|        Z 1K          | PC617 |
         Z  Z                         +----+----+   Z             +-------+
    330K Z  Z 47K                     |   VCC   |3  |   20K   1uF  |     |
         Z  Z             1.7uF       |        Q|---|--ZZZZZ--||---+     Z   1uF         6|\
         |  |            .-||-- GND   |         |7  |              |  1K Z<--||--+--------| \
         |  |   30K      |           4|      DIS|---+              Z     Z       |   LM380|  \  220uF
  PA0 ---|--|--ZZZZZ--.  |      .-----|R        |   |         2.2K Z     |       Z        |  8>--||----> Audio Out.
         |  |         |  |      |     |   555   |   Z              Z    -+-  10K Z     7+-|  /
         |  |   15K   |  |      |    5|         |   Z 10K          |    GND      Z     3+-| /-.1   .---> Audio Out.
  PA1 ---+--|--ZZZZZ--+--+------|-----|CV       |   Z              |             |     2+-|/  |    |
            |         |         |    2|         |6  |              |             |      |     |    |
            |  7.5K   |         |  .--|TR    THR|---+---||---------+-------------+      +-||--'    |
  PA2 ------+--ZZZZZ--'  |\     |  |  |   GND   |   |                            |      |          |
                         | \    |  |  +----+----+   |  .1uF                      |      | 10uF     |
                        9|  \   |  |      1|        |                           -+-     |          |
  PA3 -------------------|  8>--'  |      -+-       |                           GND     +----------'
                    4069 |  /      |      GND       |                                   |
                         | /       |                |                                  -+-
                         |/        '----------------'                                  GND
*/

static const discrete_555_desc goldnpkr_555_vco_desc =
	{DISC_555_OUT_DC | DISC_555_OUT_ENERGY, 5, DEFAULT_555_VALUES};

static const discrete_dac_r1_ladder dac_goldnpkr_ladder =
{
	3,									/* size of ladder */
	{RES_K(30), RES_K(15), RES_K(7.5)},	/* elements */

/*  external vBias doesn't seems to be accurate.
    using the 555 internal values sound better.
*/
	5,									/* voltage Bias resistor is tied to */
	RES_K(5),							/* additional resistor tied to vBias */
	RES_K(10),							/* resistor tied to ground */

	CAP_U(1.7)							/* filtering cap tied to ground */

//  12,                                 /* voltage Bias resistor is tied to */
//  RES_K(330),                         /* additional resistor tied to vBias */
//  0,                                  /* resistor tied to ground */
};

static DISCRETE_SOUND_START( goldnpkr )
/*
    - bits -
    76543210
    --------
    .....xxx --> sound data.
    ....x... --> enable/disable.

*/
	DISCRETE_INPUT_NOT   (NODE_01)		/* bit 3 - enable/disable */
	DISCRETE_INPUT_DATA  (NODE_10)		/* bits 0-2  - sound data */

	DISCRETE_DAC_R1(NODE_20, NODE_01, NODE_10, 5, &dac_goldnpkr_ladder)

	DISCRETE_555_ASTABLE_CV(NODE_30, NODE_01, RES_K(1), RES_K(10), CAP_U(.1), NODE_20, &goldnpkr_555_vco_desc)
	DISCRETE_OUTPUT(NODE_30, 3000)

DISCRETE_SOUND_END

/*
    Jack Potten's Poker discrete sound circuitry.
    ---------------------------------------------

                                           +5V
           +12V                             |
            |                               +--------.
         .--+                               |        Z
         |  |                              8|        Z 1K                                 +12V
         Z  Z                          .----+----.   Z                                     |
     47K Z  Z 220K                     |   VCC   |3  |    10K    1uF                     |\|
         Z  Z                          |        Q|---|---ZZZZZ---||--.             1K   7| \
         |  |                          |         |7  |               |         .--ZZZZZ--|+ \  470uF
         |  |   33K                   4|      DIS|---+               Z   10uF  |        6|  8>--||----> Audio Out.
  PA0 ---|--|--ZZZZZ--.         .------|R        |   |               Z<---||---+---------|- /
         |  |         |         |      |   555   |   Z               Z              LM380| /   .------> Audio Out.
         |  |   18K   |         |     5|         |   Z 1K            |                   |/|   |
  PA1 ---+--|--ZZZZZ--+---------|------|CV       |   Z               |                2,3,4|   |
            |         |         |     2|         |6  |  +            |                5,10 +---'
            |   10K   |         |  .---|TR    THR|---+---|(----------+                11,12|
  PA2 ------+--ZZZZZ--'  |\     |  |   |   GND   |   |               |                     |
                         | \    |  |   '----+----'   |   1uF        -+-                   -+-
                        9|  \ 8 |  |       1|        |              GND                   GND
  PA3 -------------------|   >--'  |       -+-       |
                  74LS04 |  /      |       GND       |
                         | /       |                 |
                         |/        '-----------------'
*/

static const discrete_dac_r1_ladder dac_pottnpkr_ladder =
{
	3,									/* size of ladder */
	{RES_K(33), RES_K(18), RES_K(10)},	/* elements */

/*  external vBias doesn't seems to be accurate.
    using the 555 internal values sound better.
*/
	5,									/* voltage Bias resistor is tied to */
	RES_K(5),							/* additional resistor tied to vBias */
	RES_K(10),							/* resistor tied to ground */

	0									/* no filtering cap tied to ground */
};

static DISCRETE_SOUND_START( pottnpkr )
/*
    - bits -
    76543210
    --------
    .....xxx --> sound data.
    ....x... --> enable/disable.

*/
	DISCRETE_INPUT_NOT   (NODE_01)		/* bit 3 - enable/disable */
	DISCRETE_INPUT_DATA  (NODE_10)		/* bits 0-2  - sound data */

	DISCRETE_DAC_R1(NODE_20, NODE_01, NODE_10, 5, &dac_pottnpkr_ladder)

	DISCRETE_555_ASTABLE_CV(NODE_30, NODE_01, RES_K(1), RES_K(1), CAP_U(1), NODE_20, &goldnpkr_555_vco_desc)
	DISCRETE_OUTPUT(NODE_30, 3000)

DISCRETE_SOUND_END


/*************************
*    Machine Drivers     *
*************************/

static MACHINE_DRIVER_START( pmpoker_base )

	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6502, CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(pmpoker_map, 0)
	MDRV_CPU_VBLANK_INT("main", nmi_line_pulse)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((39+1)*8, (31+1)*8)                  /* From MC6845 init, registers 00 & 04 (programmed with value-1). */
	MDRV_SCREEN_VISIBLE_AREA(1*8, 32*8-1, 0*8, 29*8-1)    /* From MC6845 init, registers 01 & 06. */

	MDRV_DEVICE_ADD("crtc", MC6845)

	MDRV_GFXDECODE(pmpoker)
	MDRV_PALETTE_INIT(goldnpkr)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(pmpoker)
	MDRV_VIDEO_UPDATE(pmpoker)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pmpoker )

	MDRV_IMPORT_FROM(pmpoker_base)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(goldnpkr)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pottnpkr )

	MDRV_IMPORT_FROM(pmpoker_base)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(pottnpkr_map, 0)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(pottnpkr)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( witchcrd )

	MDRV_IMPORT_FROM(pmpoker_base)

	/* video hardware */
	MDRV_PALETTE_INIT(witchcrd)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(goldnpkr)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sloco93 )

	MDRV_IMPORT_FROM(pmpoker_base)

	/* video hardware */
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_SIZE((39+1)*8, (31+1)*8)
	MDRV_SCREEN_VISIBLE_AREA(2*8, 32*8-1, 1*8, 29*8-1)

	MDRV_PALETTE_INIT(witchcrd)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(goldnpkr)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


/*************************
*        Rom Load        *
*************************/

ROM_START( pmpoker )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "2-5.bin",	0x5000, 0x1000, CRC(3446a643) SHA1(e67854e3322e238c17fed4e05282922028b5b5ea) )
	ROM_LOAD( "2-6.bin",	0x6000, 0x1000, CRC(50d2d026) SHA1(7f58ab176de0f0f7666d87271af69a845faec090) )
	ROM_LOAD( "2-7.bin",	0x7000, 0x1000, CRC(a9ab972e) SHA1(477441b7ff3acae3a5d5a3e4c2a428e0b3121534) )

	ROM_REGION( 0x1800, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x1000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "1-4.bin",	0x1000, 0x0800, CRC(62b9f90d) SHA1(39c61a01225027572fdb75543bb6a78ed74bb2fb) )    /* text layer */

	ROM_REGION( 0x1800, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "1-1.bin",	0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "1-2.bin",	0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "1-3.bin",	0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "tbp24sa10n.7d",		0x0000, 0x0100, BAD_DUMP CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

/*  the original goldnpkr u40_4a.bin rom is bit corrupted.
    U43_2A.bin        BADADDR      --xxxxxxxxxxx
    U38_5A.bin        1ST AND 2ND HALF IDENTICAL
    UPS39_12A.bin     0xxxxxxxxxxxxxx = 0xFF
*/
ROM_START( goldnpkr )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "ups39_12a.bin",	0x0000, 0x8000, CRC(216b45fb) SHA1(fbfcd98cc39b2e791cceb845b166ff697f584add) )

	ROM_REGION( 0x6000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x4000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "u38_5a.bin",	0x4000, 0x2000, CRC(32705e1d) SHA1(84f9305af38179985e0224ae2ea54c01dfef6e12) )    /* text layer */

	ROM_REGION( 0x6000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "u43_2a.bin",	0x0000, 0x2000, CRC(10b34856) SHA1(52e4cc81b36b4c807b1d4471c0f7bea66108d3fd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "u40_4a.bin",	0x2000, 0x2000, CRC(5fc965ef) SHA1(d9ecd7e9b4915750400e76ca604bec8152df1fe4) )    /* cards deck gfx, bitplane2 */
	ROM_COPY( "gfx1",	0x4800, 0x4000, 0x0800 )    /* cards deck gfx, bitplane3. found in the 2nd quarter of the text layer rom */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "tbp24s10n.7d",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )

/*  pmpoker                 goldnpkr
    1-4.bin                 u38_5a (1st quarter)    96.582031%  \ 1st and 2nd halves are identical.
    1-3.bin                 u38_5a (2nd quarter)    IDENTICAL   /
    1-1.bin                 u43_2a (1st quarter)    IDENTICAL   ; 4 quarters are identical.
*/
ROM_END

ROM_START( goldnpkb )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "ups31h.12a",	0x0000, 0x8000, CRC(bee5b07a) SHA1(5da60292ecbbedd963c273eac2a1fb88ad66ada8) )

	ROM_REGION( 0x6000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x4000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "u38_5a.bin",	0x4000, 0x2000, CRC(32705e1d) SHA1(84f9305af38179985e0224ae2ea54c01dfef6e12) )    /* text layer */

	ROM_REGION( 0x6000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "u43_2a.bin",	0x0000, 0x2000, CRC(10b34856) SHA1(52e4cc81b36b4c807b1d4471c0f7bea66108d3fd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "u40_4a.bin",	0x2000, 0x2000, CRC(5fc965ef) SHA1(d9ecd7e9b4915750400e76ca604bec8152df1fe4) )    /* cards deck gfx, bitplane2 */
	ROM_COPY( "gfx1",	0x4800, 0x4000, 0x0800 )    /* cards deck gfx, bitplane3. found in the 2nd quarter of the text layer rom */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "tbp24s10n.7d",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )

/*  pmpoker                 goldnpkb
    1-4.bin                 u38.5a (1st quarter)    96.582031%  \ 1st and 2nd halves are identical.
    1-3.bin                 u38.5a (2nd quarter)    IDENTICAL   /
    1-1.bin                 u43.2a (1st quarter)    IDENTICAL   ; 4 quarters are identical.
*/
ROM_END

ROM_START( pottnpkr )	/* Golden Poker game without the double up feature. Code is intended to start at $6000 */
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "ic13_3.bin",	0x2000, 0x1000, CRC(23c975cd) SHA1(1d32a9ba3aa996287a823558b9d610ab879a29e8) )
	ROM_LOAD( "ic14_4.bin",	0x3000, 0x1000, CRC(86a03aab) SHA1(0c4e8699b9fc9943de1fa0a364e043b3878636dc) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "ic7_0.bin",	0x2000, 0x1000, CRC(1090e7f0) SHA1(26a7fc8853debb9a759811d7fee39410614c3895) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "ic2_7.bin",	0x0000, 0x1000, CRC(b5a1f5a3) SHA1(a34aaaab5443c6962177a5dd35002bd09d0d2772) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "ic3_8.bin",	0x1000, 0x1000, CRC(40e426af) SHA1(7e7cb30dafc96bcb87a05d3e0ef5c2d426ed6a74) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "ic5_9.bin",	0x2000, 0x1000, CRC(232374f3) SHA1(b75907edbf769b8c46fb1ebdb301c325c556e6c2) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "tbp24s10n.7d",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )

/*  ic2_7.bin    1ST AND 2ND HALF IDENTICAL
    ic3_8.bin    1ST AND 2ND HALF IDENTICAL
    ic5_9.bin    1ST AND 2ND HALF IDENTICAL
    ic7_0.bin    1ST AND 2ND HALF IDENTICAL

    RB confirmed the dump. There are other games with double sized roms and identical halves.
*/
ROM_END

ROM_START( potnpkra )    /* a Coinmaster game?... seems to be a hack */
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "vp-5.bin",	0x2000, 0x1000, CRC(1443d0ff) SHA1(36625d24d9a871cc8c03bdeda983982ba301b385) )
	ROM_LOAD( "vp-6.bin",	0x3000, 0x1000, CRC(94f82fc1) SHA1(ce95fc429f5389eea45fec877bac992fa7ba2b3c) )

	ROM_REGION( 0x1800, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x1000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "vp-4.bin",	0x1000, 0x0800, CRC(2c53493f) SHA1(9e71db51499294bb4b16e7d8013e5daf6f1f9d18) )    /* text layer */

	ROM_REGION( 0x1800, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "vp-1.bin",	0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "vp-2.bin",	0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "vp-3.bin",	0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, BAD_DUMP CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )
ROM_END

/*
pottpok4.bin                                                 0xxxxxxxxxx = 0x00
                        517.4a                               0xxxxxxxxxx = 0x00
pottpok1.bin            517.8a                  IDENTICAL
pottpok2.bin            517.7a                  IDENTICAL
pottpok3.bin            517.6a                  IDENTICAL
pottpok4.bin            517.4a                  IDENTICAL
pottpok5.bin            517.16a                 69.335938%
pottpok6.bin            517.17a                 2.685547%
*/

ROM_START( potnpkrb )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "517.16a",	0x2000, 0x1000, CRC(8892fbd4) SHA1(22a27c0c3709ca4808a9afb8848233bc4124559f) )
	ROM_LOAD( "517.17a",	0x3000, 0x1000, CRC(75a72877) SHA1(9df8fd2c98526d20aa0fa056a7b71b5c5fb5206b) )

	ROM_REGION( 0x1800, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x1000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "517.8a",		0x1000, 0x0800, CRC(2c53493f) SHA1(9e71db51499294bb4b16e7d8013e5daf6f1f9d18) )    /* text layer */

	ROM_REGION( 0x1800, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "517.4a",		0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "517.6a",		0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "517.7a",		0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "517_mb7052.9c",	0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )
ROM_END

/* the alternative Jack Potten set is identical, but with different sized roms.

                pot1.bin                1ST AND 2ND HALF IDENTICAL
                pot2.bin                1ST AND 2ND HALF IDENTICAL
pottpok1.bin    pot34.bin (1st half)    IDENTICAL
pottpok2.bin    pot34.bin (2nd half)    IDENTICAL
pottpok3.bin    pot2.bin (1st half)     IDENTICAL
pottpok4.bin    pot1.bin (1st half)     IDENTICAL
pottpok5.bin    pot5.bin                IDENTICAL
pottpok6.bin    pot6.bin                IDENTICAL
*/

ROM_START( potnpkrc )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "pottpok5.bin",	0x2000, 0x1000, CRC(d74e50f4) SHA1(c3a8a6322a3f1622898c6759e695b4e702b79b28) )
	ROM_LOAD( "pottpok6.bin",	0x3000, 0x1000, CRC(53237873) SHA1(b640cb3db2513784c8d2d8983a17352276c11e07) )

	ROM_REGION( 0x1800, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(					0x0000, 0x1000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "pottpok1.bin",	0x1000, 0x0800, CRC(2c53493f) SHA1(9e71db51499294bb4b16e7d8013e5daf6f1f9d18) )    /* text layer */

	ROM_REGION( 0x1800, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "pottpok4.bin",	0x0000, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "pottpok3.bin",	0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "pottpok2.bin",	0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )
ROM_END


/***************** NEW SETS **************************/


ROM_START( potnpkrd )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "pot5.bin",	0x2000, 0x1000, CRC(d74e50f4) SHA1(c3a8a6322a3f1622898c6759e695b4e702b79b28) )
	ROM_LOAD( "pot6.bin",	0x3000, 0x1000, CRC(53237873) SHA1(b640cb3db2513784c8d2d8983a17352276c11e07) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "pot34.bin",	0x2000, 0x1000, CRC(52fd35d2) SHA1(ad8bf8c222ceb2e9b3b6d9033866867f1977c65f) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "pot1.bin",	0x0000, 0x1000, CRC(b5a1f5a3) SHA1(a34aaaab5443c6962177a5dd35002bd09d0d2772) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "pot2.bin",	0x1000, 0x1000, CRC(40e426af) SHA1(7e7cb30dafc96bcb87a05d3e0ef5c2d426ed6a74) )    /* cards deck gfx, bitplane2 */
	ROM_COPY( "gfx1",		0x2800, 0x2000, 0x0800 )    /* cards deck gfx, bitplane3. found in the 2nd quarter of the text layer rom */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) )
ROM_END

ROM_START( potnpkre )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "g_luck_a.bin",	0x2000, 0x1000, CRC(21d3b5e9) SHA1(32f06eb26c5232738ad7e86f1a81eb9717f9c7e0) )
	ROM_LOAD( "g_luck_b.bin",	0x3000, 0x1000, CRC(7e848e5e) SHA1(45461cfcce06f6240562761d26ba7fdb7ef4986b) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "ic7_0.bin",	0x2000, 0x1000, CRC(1090e7f0) SHA1(26a7fc8853debb9a759811d7fee39410614c3895) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "ic2_7.bin",	0x0000, 0x1000, CRC(b5a1f5a3) SHA1(a34aaaab5443c6962177a5dd35002bd09d0d2772) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "ic3_8.bin",	0x1000, 0x1000, CRC(40e426af) SHA1(7e7cb30dafc96bcb87a05d3e0ef5c2d426ed6a74) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "ic5_9.bin",	0x2000, 0x1000, CRC(232374f3) SHA1(b75907edbf769b8c46fb1ebdb301c325c556e6c2) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "tbp24s10n.7d",		0x0000, 0x0100, BAD_DUMP CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( goodluck )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "goodluck_glh6b.bin",	0x0000, 0x8000, CRC(2cfa4a2c) SHA1(720e2900f3a0ef2632aa201a63b5eba0570e6aa3) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(			0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "4.bin",	0x2000, 0x1000, CRC(41924d13) SHA1(8ab69b6efdc20858960fa5df669470ba90b5f8d7) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( bsuerte )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "ups39_12a.bin",	0x0000, 0x8000, CRC(e6b661b7) SHA1(b265f6814a168034d24bc1c25f67ece131281bc2) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "u38.bin",	0x2000, 0x1000, CRC(0a159dfa) SHA1(0a9c8e6177b36831b365917a10042aac3383983d) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( bsuertea )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "x10d4esp.16c",	0x0000, 0x8000, CRC(0606bab4) SHA1(624b0cef1a23a4e7ba2d2d256f30f73b1e455fa7) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(				0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "u38.bin",	0x2000, 0x1000, CRC(0a159dfa) SHA1(0a9c8e6177b36831b365917a10042aac3383983d) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( royale )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "royalex.bin",	0x4000, 0x4000, CRC(ef370617) SHA1(0fc5679e9787aeea3bc592b36efcaa20e859f912) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(					0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "royalechr.bin",	0x2000, 0x1000, CRC(b1f2cbb8) SHA1(8f4930038f2e21ca90b213c35b45ed14d8fad6fb) )    /* text layer */

	ROM_REGION( 0x1800, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "royale3.bin",	0x0000, 0x0800, CRC(1f41c541) SHA1(00df5079193f78db0617a6b8a613d8a0616fc8e9) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "royale2.bin",	0x0800, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "royale1.bin",	0x1000, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( witchcrd )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "wc_sbruj.256",	0x0000, 0x8000, CRC(5689ae41) SHA1(c7a624ec881204137489b147ce66cc9a9900650a) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(					0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "bs_4_wc.032",	0x2000, 0x1000, CRC(41924d13) SHA1(8ab69b6efdc20858960fa5df669470ba90b5f8d7) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( witchcda )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "w_card.256",	0x0000, 0x8000, CRC(104218a4) SHA1(191e198f5443afc80a0e1bba3ccaad4e744b15e0) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(					0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "bs_4_wc.032",	0x2000, 0x1000, CRC(41924d13) SHA1(8ab69b6efdc20858960fa5df669470ba90b5f8d7) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( witchcdb )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "w_card.128",	0x4000, 0x4000, CRC(620ac5ca) SHA1(69275683780c65a83ee6dcef3bd14ac02f4929a0) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(					0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "bs_4_wc.032",	0x2000, 0x1000, CRC(41924d13) SHA1(8ab69b6efdc20858960fa5df669470ba90b5f8d7) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( sloco93 )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "locoloco.128",	0x4000, 0x4000, CRC(f626a770) SHA1(afbd33b3f65b8a781c716a3d6e5447aa817d856c) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(					0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "confloco.032",	0x2000, 0x1000, CRC(b86f219c) SHA1(3f655a96bcf597a271a4eaaa0acbf8dd70fcdae9) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END

ROM_START( sloco93a )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "locoloco.256",	0x0000, 0x8000, CRC(ab037b0b) SHA1(16f811daaed5bf7b72549db85755c5274dfee310) )

	ROM_REGION( 0x3000, "gfx1", ROMREGION_DISPOSE )
	ROM_FILL(					0x0000, 0x2000, 0 ) /* filling the R-G bitplanes */
	ROM_LOAD( "confloco.032",	0x2000, 0x1000, CRC(b86f219c) SHA1(3f655a96bcf597a271a4eaaa0acbf8dd70fcdae9) )    /* text layer */

	ROM_REGION( 0x3000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "7.bin",	0x0000, 0x1000, CRC(28ecfaea) SHA1(19d73ed0fdb5a873447b46e250ad6e71abe257cd) )    /* cards deck gfx, bitplane1 */
	ROM_LOAD( "6.bin",	0x1000, 0x1000, CRC(eeec8862) SHA1(ae03aba1bd43c3ffd140f76770fc1c8cf89ea115) )    /* cards deck gfx, bitplane2 */
	ROM_LOAD( "5.bin",	0x2000, 0x1000, CRC(2712f297) SHA1(d3cc1469d07c3febbbe4a645cd6bdb57e09cf504) )    /* cards deck gfx, bitplane3 */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "82s129.9c",		0x0000, 0x0100, CRC(7f31066b) SHA1(15420780ec6b2870fc4539ec3afe4f0c58eedf12) ) /* PROM dump needed */
ROM_END


/*************************
*      Driver Init       *
*************************/

static DRIVER_INIT( pmpoker )
{
	pia_config(0, &pia0_intf);
	pia_config(1, &pia1_intf);
}
/*
    Golden Poker H/W sets:

    newname    oldname    gameplay  music      settings    testmode
    ===================================================================
    pmpoker    pmpoker    fast      minimal    hack        matrix/grill
    goldnpkr   goldnpkr   fast      y.doodle   excellent   matrix/grill
    goldnpkb   goldnpkb   normal    minimal    normal      matrix/grill


    Potten's Poker H/W sets:

    newname    oldname    gameplay  music      settings    testmode
    ===================================================================
    pottnpkr   goldnpkc   fast      y.doodle   normal      only grid
    potnpkra   jokerpkr   normal    normal     normal      only skill
    potnpkrb   pottnpkb   slow      y.doodle   normal      only grid
    potnpkrc   pottnpkr   normal    minimal    hack        only skill
    potnpkrd   --------   normal    minimal    hack        only skill
    potnpkre   --------   slow      y.doodle   normal      matrix/grill


    Witch Card H/W sets:

    newname    oldname    gameplay  music      settings    testmode
    ===================================================================
    bsuerte    --------   normal    minimal    only 1-10   matrix/grill
    bsuertea   --------   normal    minimal    only 1-10   matrix/grill
    goodluck   --------   fast      y.doodle   normal      matrix/grill
    royale     --------
    witchcrd   --------   normal    y.doodle   12-param    only grid
    witchcda   --------   fast      y.doodle   12-param    only grid
    witchcdb   --------   normal    y.doodle   12-param    only grid
    sloco93    --------   fast      custom     complete    only grid
    sloco93a   --------   fast      custom     complete    only grid

*/
static DRIVER_INIT( royale )
{
    /* $60bb, NOPing the ORA #$F0 (after read the PIA1 port B */

//  UINT8 *ROM = memory_region(machine, "main");

//  ROM[0x60bb] = 0xea;
//  ROM[0x60bc] = 0xea;

	pia_config(0, &pia0_intf);
	pia_config(1, &pia1_intf);
}

/*************************
*      Game Drivers      *
*************************/

/*     YEAR  NAME      PARENT    MACHINE   INPUT     INIT      ROT      COMPANY                      FULLNAME                            FLAGS                    LAYOUT  */
GAMEL( 1981, pmpoker,  0,        pmpoker,  pmpoker,  pmpoker,  ROT0,   "PlayMan",                   "PlayMan Poker (german)",            0,                       layout_pmpoker  )
GAMEL( 1981, goldnpkr, 0,        pmpoker,  goldnpkr, pmpoker,  ROT0,   "Bonanza Enterprises, Ltd",  "Golden Poker Double Up (Big Boy)",  0,                       layout_goldnpkr )
GAMEL( 1981, goldnpkb, goldnpkr, pmpoker,  goldnpkr, pmpoker,  ROT0,   "Bonanza Enterprises, Ltd",  "Golden Poker Double Up (Mini Boy)", 0,                       layout_goldnpkr )
GAMEL( 198?, pottnpkr, 0,        pottnpkr, pottnpkr, pmpoker,  ROT0,   "Bootleg",                   "Jack Potten's Poker (set 1)",       0,                       layout_pottnpkr )
GAMEL( 198?, potnpkra, pottnpkr, pottnpkr, potnpkra, pmpoker,  ROT0,   "Bootleg on Coinmaster H/W", "Jack Potten's Poker (set 2)",       0,                       layout_pottnpkr )
GAMEL( 198?, potnpkrb, pottnpkr, pottnpkr, pottnpkr, pmpoker,  ROT0,   "Bootleg",                   "Jack Potten's Poker (set 3)",       0,                       layout_pottnpkr )
GAMEL( 198?, potnpkrc, pottnpkr, pottnpkr, potnpkrc, pmpoker,  ROT0,   "Bootleg",                   "Jack Potten's Poker (set 4)",       0,                       layout_pottnpkr )

GAMEL( 198?, potnpkrd, pottnpkr, pottnpkr, potnpkrc, pmpoker,  ROT0,   "Bootleg",                   "Jack Potten's Poker (set 5)",       0,                       layout_pottnpkr )
GAMEL( 198?, potnpkre, pottnpkr, pottnpkr, pottnpkr, pmpoker,  ROT0,   "Bootleg",                   "Jack Potten's Poker (set 6)",       0,                       layout_pottnpkr )
GAMEL( 198?, goodluck, 0,        witchcrd, goodluck, pmpoker,  ROT0,   "Unknown",                   "Good Luck",                         0,                       layout_pottnpkr )
GAMEL( 198?, bsuerte,  0,        witchcrd, bsuerte,  pmpoker,  ROT0,   "Unknown",                   "Buena Suerte (spanish, set 1)",     0,                       layout_goldnpkr )
GAMEL( 198?, bsuertea, bsuerte,  witchcrd, bsuerte,  pmpoker,  ROT0,   "Unknown",                   "Buena Suerte (spanish, set 2)",     0,                       layout_goldnpkr )
GAMEL( 198?, royale,   0,        pmpoker,  royale,   royale,   ROT0,   "Unknown",                   "Royale (ver.X)",                    GAME_NOT_WORKING,        layout_goldnpkr )
GAME(  1991, witchcrd, 0,        witchcrd, witchcrd, pmpoker,  ROT0,   "Unknown",                   "Witch Card (english)",              0 )
GAME(  1991, witchcda, witchcrd, witchcrd, witchcrd, pmpoker,  ROT0,   "Unknown",                   "Witch Card (spanish, set 1)",       0 )
GAME(  1991, witchcdb, witchcrd, witchcrd, witchcrd, pmpoker,  ROT0,   "Unknown",                   "Witch Card (spanish, set 2)",       0 )
GAME(  1993, sloco93,  0,        sloco93,  sloco93,  pmpoker,  ROT0,   "Unknown",                   "Super Loco 93 (spanish, set 1)",    0 )
GAME(  1993, sloco93a, sloco93,  sloco93,  sloco93,  pmpoker,  ROT0,   "Unknown",                   "Super Loco 93 (spanish, set 2)",    0 )
