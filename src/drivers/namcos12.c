/***************************************************************************

  Namco System 12 - Arcade PSX Hardware
  =====================================
  Driver by smf
  H8/3002 and Golgo13 support by R. Belmont based on work by The_Author and DynaChicken

  Issues:
    not all games work due to either banking, dma or protection issues.
    graphics are glitchy in some games.

    - day, date, and year from the RTC appear to be ignored (hour/min/sec are fine). H8 core bug or BIOS doesn't care?
    - golgo13 assumes the test switch is a switch, not a button - must hold down F2 to stay in test mode

Game & software revision                 Company/Year              CPU board    Mother board          Daughter board     Keycus
-------------------------------------------------------------------------------------------------------------------------------
*Aerosmith - Quest for Fame              (C) Namco,        2001
Aqua Rush (AQ1/VER.A1)                   (C) Namco,        1999    COH 700      SYSTEM12 MOTHER(C)    SYSTEM12 M5F2      KC053
*Attack Pla-Rail                         (C) Tomy/Namco    199?
Derby Quiz My Dream Horse (MDH1/VER.A2)  (C) Namco,        1998    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M10X64    KC035
Ehrgeiz (EG2/VER.A)                      (C) Square/Namco, 1998    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M4F6      KC021
Ehrgeiz (EG3/VER.A)                      (C) Square/Namco, 1998    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M4F6      KC021
Fighting Layer (FTL0/VER.A)              (C) Arika/Namco,  1998    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M5F4      KC037
*Fighting Spirit Biography 3             (C) Namco,        1997
*Ghoul Panic                             (C) Namco/Raizing 1999
Golgo 13 (GLG1/VER.A)                    (C) Raizing/Namco,1999    COH 700      SYSTEM12 MOTHER(C)    SYSTEM12 M8F6      KC054
*Kaiun Quiz                              (C) Namco,        199?
*Kart Duel                               (C) Namco,        2000
Libero Grande (LG2/VER.A)                (C) Namco,        1997    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M8F2F     KC014
Mr Driller (DRI1/VER.A2)                 (C) Namco,        1999    COH 700      SYSTEM12 MOTHER(C)    SYSTEM12 M8F2F     KC048
*Oh Bakyuuun                             (C) Namco/Raizing 1999
*Over Kuhn                               (C) Namco         1999
Paca Paca Passion (PPP1/VER.A2)          (C) Produce/Namco,1999    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M8F2F     KC038
*Paca Paca Passion Special               (C) Produce/Namco,1999
*Paca Paca Passion 2                     (C) Produce/Namco,1999
Soul Calibur (SOC11/VER.A2)              (C) Namco,        1998    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M5F2      KC020
Soul Calibur (SOC11/VER.B)               (C) Namco,        1998    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M5F2      KC020
Soul Calibur (SOC14/VER.C)               (C) Namco,        1998    COH 700      SYSTEM12 MOTHER       JO 11-04-98        none
*Soul Calibur Ver.B                      (C) Namco,        199?
*Submarines                              (C) Namco,        199?
*Super World Stadium '98                 (C) Namco,        1998
Super World Stadium '99 (SS91/VER.A3)    (C) Namco,        1999    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M5F4      KC043
*Super World Stadium 2000                (C) Namco,        2000
*Super World Stadium 2001                (C) Namco,        2001
*Techno Drive                            (C) Namco,        1998
*TeknoWerk                               (C) Namco,        1999
*Tenkomori Shooting                      (C) Namco,        1998
Tekken 3 (TET1/VER.E1)                   (C) Namco,        1996    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M8F2F     KC006
Tekken 3 (TET2/VER.A)                    (C) Namco,        1996    COH 700      SYSTEM12 MOTHER       SYSTEM12 M8F2F     KC006
Tekken 3 (TET3/VER.A)                    (C) Namco,        1996    COH 700      SYSTEM12 MOTHER       SYSTEM12 M8F2F     KC006
Tekken Tag Tournament (TEG3/VER.B)       (C) Namco,        1999    COH 700      SYSTEM12 MOTHER(B)    SYSTEM12 M8F4      KC044
*Toukon Retsuden 3                       (C) Namco,        199?
*Truck Kyosokyoku                        (C) Namco,        2000
*Um Jammer Lammy                         (C) Namco,        1999

* - denotes not dumped yet. If you can help with the remaining undumped S12 games,
    please contact http://unemulated.emuunlim.com or http://targets.mame.net


The Namco System 12 system comprises 4 PCB's....
MOTHER PCB  - This is the main PCB. It holds all sound circuitry, sound ROMs, program ROMs, shared RAM, bank-switching
              logic, controller/input logic (including sound CPU) and some video output circuitry.
              There are three known revisions of this PCB. The 3rd revision has some extra circuitry for analog controls,
              some of the PALs are different and it has the capability to use 1x TSOP56 ROM in the program location,
              instead of the regular TSOP40/48 ROMs. The first 2 revisions appear to be identical.
CPU PCB     - All games use the exact same PCB. Contains main CPU/RAM and GPU/Video RAM
ROM PCB     - There are eight known revisions of this PCB (so far). They're mostly identical except for the type and
              number of ROMs used. Most have a PAL and a CPLD known as a 'KEYCUS'. Some also have an additional CPLD for
              protection.
NETWORK PCB - Used to connect 2 PCBs together using standard USB cables. The board plugs in where the CPU board would
              normally be, and the CPU board plugs into another connector on the Network PCB. Seems to be only used on
              Libero Grande, Tekken 3 and Ehrgeiz (so far?)

Each game has a 3 or 4 digit letter code assigned to it which is printed on a small sticker and placed on the underside
of the main PCB.
The 4 digit code is then proceeded by a number (generally 1, 2 or 3), then 'Rev.' then A/B/C/D/E which denotes the software
revision, and in some cases a sub-revision such as 1 or 2 (usually only listed in the test mode).
The first 1 denotes a Japanese version. 2 (and maybe 3) denotes a World version. So far there are no other numbers used
other than 1, 2, or 3.
For World versions, usually only the main program uses a '2' or '3', the rest of the ROMs use the Japanese region code '1'.
See the Main PCB and ROM Daughterboard PCB texts below for more details.

Main PCB
--------
1st Revision
SYSTEM12 MOTHER PCB 8661960105 (8661970105)
SYSTEM12 MOTHER PCB 8661960106 (8661970106)

2nd Revision
SYSTEM12 MOTHER (B) PCB 8661961000 (8661971000)

3rd Revision
SYSTEM12 MOTHER (C) PCB (System11 Plus) 8661961101 (8661971101)
Rev. (C) Main Board is only slightly different to the other revs. Mostly some PALs are different and the
PCB is wired for 1x 32M TSOP56 FlashROM (replaces 2E & 2J) and 2x 16M TSOP48 FlashROMs (replaces 2R/2P/2N/2L).
One other difference is that the motherboard has a serial number sticker on it and the program ROMs also contain
that same serial number, so all the games running on the Rev. C mother board have serialised ROMs. I'm not sure why,
they're not exactly _easily_ tracable and no one cares either way ;-)
  |--------------------------------------------------------------|
|-|     SOUND.11S       |--------------|               AT28C16   |
|                       |--------------|                         |
|                     |------|J103                               |
|          |-----|    | C352 |                     %PRG.2R       |
|          |H8/  |    |------|                                   |
|J         |3002 |                2061ASC-1         PRG.2P       |
|A         |-----|                   14.7456MHz                  |
|M                                                 %PRG.2N       |
|M                     |-------|                                 |
|A                     |       |                    PRG.2L       |
|                      | C416  |                                 |
|    J110 N341256      |       |                                 |
|         N341256      |-------|                   %PRG.2J       |
|      DSW(2)                                 MOT1 *PRG.2F R4543 |
|-|                                                %PRG.2E       |
  |                                                              |
  |                           MOT6    MOT5    MOT2   MOT4        |
|-|                                                       3V_BATT|
|N   *D6345     6734 6358 MB88347                                |
|A                                            MOT7   MOT3        |
|M                                                               |
|C    LC78832M      CXA1779P                                     |
|O        3414A                        J111            PQ30RV21  |
|4                          |--------------------------|         |
|8                          |--------------------------|         |
|-|                                                              |
  |LA4705  J109     ADM485                                       |
  |        VOL   J108     J107           J106   J105      J104   |
  |--------------------------------------------------------------|
(logic chips omitted from the PCB layout)

Notes:
      *            : These parts only on MOTHER (C) PCB
      %            : These parts not on MOTHER (C) PCB
      J103         : Custom Namco connector for plug-in ROM PCB
      J104         : JAMMA2 power connector (only 5V/12V/0V used)
      J105/J106    : JAMMA2 15 PIN VGA D-Type connector (x2) for video output (sync can be 15kHz or 31kHz)
      J107         : JAMMA2 RCA connectors (x4) for twin stereo audio output
      J108         : JAMMA2 USB connector for controls
      J109         : 3-pin connector (populated only on MOTHER (C) PCB, labelled on PCB 'AUDIO OUT')
      J110         : 4-pin connector (purpose unknown, 2 outer pins labelled on PCB 'VCC' & 'GND')
      J111         : Custom Namco connector for plug-in CPU PCB
      MB88347      : Fujitsu MB88347 8-bit 8-channel D/A converter with OP AMP output buffers (SOIC16)
      6358         : Toshiba TD6358N Frequency Synthesizer for TV/CATV (SOIC8)
      6734         : MAX734 +12V 120mA Flash Memory Programming Supply Switching Regulator (SOIC8)
      3414A        : NJM3414A 70mA Dual Op Amp (SOIC8)
      LC78832M     : Sanyo LM78832M 2-Channel 16-Bit D/A Converter LSI with 2 On-Chip Digital Filters (SOIC20)
      2061ASC-1    : IC Designs 2061ASC-1 programmable clock generator (SOIC16)
      R4543        : EPSON Real Time Clock Module (SOIC14)
      DSW(2)       : 2 position DIP Switch (All OFF)
      N341256      : NKK N341256 32K x8 SRAM (x2, SOJ28)
      H8/3002      : Sound CPU, Hitachi H8/3002 HD6413002F17 (QFP100), clocked at 14.7456MHz.
      C416         : Namco custom C416 (QFP176)
      C352         : Namco custom C352 PCM sound chip (QFP100)
      MOT1         : PALCE 22V10H (PLCC28, labelled 'S12MOT1A')
                     *Replaced by PALCE 22V10H (PLCC28, labelled 'S12MOT1C') on MOTHER (C) PCB
      MOT2         : PALCE 22V10H (PLCC28, labelled 'S12MOT2A')
                     *Replaced by PALCE 22V10H (PLCC28, labelled 'S12MOT2C') on MOTHER (C) PCB
      MOT3         : PALCE 16V8H  (PLCC20, labelled 'S12MOT3A')
                     *Replaced by PALCE 16V8H (PLCC20, labelled 'S12MOT3C') on MOTHER (C) PCB
      MOT4         : PALCE 22V10H (PLCC28, labelled 'S12MOT4A')
      MOT5         : PALCE 22V10H (PLCC28, labelled 'S12MOT5A')
      MOT6         : PALCE 22V10H (PLCC28, labelled 'S12MOT6A')
      MOT7         : PALCE 16V8H  (PLCC20, labelled 'S12MOT7A')
      AT28C16      : Atmel AT28C16 2K x8 EEPROM (SOIC24)
      ADM485       : Analog Devices ADM485JR 5V Low Power EIA RS-485 Transceiver (SOIC8)
      CXA1179P     : Sony CXA1779P TV/Video circuit RGB Pre-Driver (DIP28)
      LA4705       : Sanyo LA4705 15W 2-Channel Power Amplifier (SIP18)
      VOL          : Master volume potentiometer
      PQ30RV21     : Sharp PQ30RV21 5V to 3.3V Voltage Regulator
      3V_BATT      : Sony CR2032 3 volt coin battery
      D6345        : NEC uPD6345 Serial 8-bit shift register IC (SOIC16)

      PRG.2L/PRG.2P: Main program ROMs, Intel 28F016S5 2M x8 FLASHROM (both TSOP40)
                     These ROMs are populated on the following games....

                                                            Test Mode S/W
                     Game                      Sticker      Revision         MOTHER PCB
                     --------------------------------------------------------------------
                     Aqua Rush                 AQ1 Ver.A    AQ1/VER.A1       MOTHER(C) (ROMs serialised)
                     Derby Quiz My Dream Horse MDH1 Ver.A   MDH1/VER.A2      MOTHER(B)
                     Ehrgeiz                   EG3 Ver.A    EG3/VER.A        MOTHER(B)
                     Golgo 13                  GLG1 Ver.A   GLG1/VER.A       MOTHER(C) (ROMs serialised)
                     Libero Grande             LG2 Ver.A    LG2/VER.A        MOTHER(B)
                     Mr Driller                DRI1 Ver.A   DRI1/VER.A2      MOTHER(C) (ROMs serialised)
                     Paca Paca Passion         PPP1 Ver.A   PPP1/VER.A2      MOTHER(B)
                     Soul Calibur              SOC1 Ver.A   SOC11/VER.A2     MOTHER(B)
                     Tekken Tag Tournament     TEG3 Ver.B   TEG3/VER.B       MOTHER(B)

      PRG.2N/PRG.2R: Main program ROMs \ Intel 28F016S5 2M x8 FLASHROM (for 2N, TSOP40)
                                       / Intel 28F008SA 1M x8 FLASHROM (for 2R, TSOP40)
                     These ROMs are not populated on any System12 PCB dumped so far. Probably they are completely
                     unused on any game since they were phased out with the Rev (C) main board.

             PRG.2F: Main program ROM (only on MOTHER (C) PCB) - Intel E28F0320 4M x8 FLASHROM (TSOP56)
                     This ROM is not populated on any System12 PCB dumped so far.

      PRG.2J/PRG.2E: Main program ROMs, Fujitsu 29F016 2M x8 FLASHROM (both TSOP48)
                     These ROMs are populated on the following games....

                                                            Test Mode S/W
                     Game                      Sticker      Revision         MOTHER PCB
                     --------------------------------------------------------------------
                     Ehrgeiz                   EG2 Ver.A    EG2/VER.A        MOTHER(B)
                     Fighting Layer            FTL1 Ver.A   FTL0/VER.A       MOTHER(B)
                     Soul Calibur              SOC1 Ver.B   SOC11/VER.B      MOTHER(B)
                     Soul Calibur              TET1 Ver.C   SOC14/VER.C      MOTHER  (factory upgraded from Tekken 3 to Soul Calibur)
                     Super World Stadium '99   SS91 Ver.A   SS91/VER.A3      MOTHER(B)
                     Tekken 3                  TET1 Ver.A   TET1/VER.E1      MOTHER(B)
                     Tekken 3                  TET2 Ver.A   TET2/VER.A       MOTHER
                     Tekken 3                  TET3 Ver.A   TET3/VER.A       MOTHER

      SOUND.11S    : Fujitsu 29F400TA-90 512K x8 EEPROM (holds H8/3002 program, SOP44)
                     This ROM is not labelled with any other markings except the manufacturer and chip type. The ROM labels
                     in the archives are simply made up for convenience.



ROM Daughterboard PCB
----------------------
This PCB holds the remainder of the ROMs, used for graphics and the 3D geometry.
There are 8 known types of ROM daughterboards used on S12 games (so far).
All of the PCBs are the same size (approx 2" x 7") containing one custom connector and some MASKROMs/FlashROMs, a PLCC
PAL and a KEYCUS (which is a PLCC CPLD) and in some cases an extra TQFP CPLD.
The PCBs are named with a special coding. First a letter M, denoting MASKROM (always SOP44), then a number denoting how
many ROMs of that type, then another letter F, denoting FLASHROM (always TSOP40/48/56), then a number denoting how many
ROMs of that type.

********
*Type 1*
********
SYSTEM 12 M4F6 PCB 8661960901 (8661970901)
|----------------------------------------------------------|
|WAVE0.IC2      IC4  FL3L.IC5                              |
|                       FL3U.IC6                           |
|                          FL2L.IC7             ROM0L.IC12 |
|                             FL2U.IC8                     |
|                                FL1L.IC9                  |
|WAVE1.IC1                          FL1U.IC10   ROM0U.IC11 |
|             IC3  |-------------------|                   |
|                  |-------------------|                   |
|----------------------------------------------------------|
Notes:
      This PCB has zero Ohm resistor jumpers to set the ROM sizes.
      Note X means 'not populated', O means 'populated'
      |----------|----|----|   |----------|----|
      |  MASK    | R6 | R7 |   |  FLASH   | R8 |
      |----------|----|----|   |----------|----|
      | WAVE 32M | X  | O  |   |  NORMAL  | X  |
      |      64M | O  | X  |   |32M FLASH | O  |
      |----------|----|----|   |----------|----|

      WAVEx: 64M SOP44 MASKROM (R6 populated)
      FLx  : Intel E28F016 TSOP40 16M FlashROM (R8 not populated)
      ROM0x: 64M SOP44 MASKROM (size fixed at 64M, no configure options)
      IC3  : MACH211 CPLD (PLCC44, labelled 'KEYCUS' and stamped with 'KC' & a 3-digit number which is different per game.)
      IC4  : PALCE 16V8H (PLCC20, PCB labelled 'A_DECO', chip stamped 'A DECO')

This PCB is used on:

              Software
Game          Revision     PCB                                            KEYCUS   ROMs Populated
------------------------------------------------------------------------------------------------------------------
Ehrgeiz       EG2/VER.A    SYSTEM 12 M4F6 PCB 8661960901 (8661970901)     KC021    EG1 WAVE0
                                                                                   EG1 ROM0U, EG1ROM0L.12
                                                                                   EG1 FL1U, EG1 FL1L, EG1 FL2U
                                                                                   EG1 FL2L, EG1 FL3U, EG1 FL3L

Ehrgeiz       EG3/VER.A    SYSTEM 12 M4F6 PCB 8661960901 (8661970901)     KC021    EG1 WAVE0
                                                                                   EG1 ROM0U, EG1ROM0L.12
                                                                                   EG1 FL1U, EG1 FL1L, EG1 FL2U
                                                                                   EG1 FL2L, EG1 FL3U, EG1 FL3L

********
*Type 2*
********
SYSTEM 12 M5F2 PCB 8661961300 (8661971300)
|----------------------------------------------------------|
|WAVE0.IC2      IC4     FL4.IC5   FL3.IC6                  |
|                                                          |
|                                        ROM0.IC7          |
|                                                          |
|                                            ROM1.IC8      |
|WAVE1.IC1                                                 |
|             IC3  |-------------------|          ROM2.IC9 |
|                  |-------------------|                   |
|----------------------------------------------------------|
Notes:
      This PCB has zero Ohm resistor jumpers to set the ROM sizes.
      Note X means 'not populated', O means 'populated'
      |----------|----|----|
      |  MASK    | R6 | R7 |
      |----------|----|----|
      | WAVE 32M | X  | O  |
      |      64M | O  | X  |
      |----------|----|----|

      |----------|----|    |------|---|---|---|---|---|----|   The Flash ROMs can be configured to 3.3V 64M Strata Flash
      |  FLASH   | R8 |    |FLASH |R11|R12|C1 |C2 |D1 |REG1|   when used with some extra components (voltage regulator/diode
      |----------|----|    |------|---|---|---|---|---|----|   /resistors/caps etc). However, the extra component positions
      |  NORMAL  | X  |    | 5V   | O | O | X | X | X | X  |   are not populated on any of the games dumped so far.
      |64M STRATA| O  |    | 3.3V | X | X | O | O | O | O  |
      |----------|----|    |------|---|---|---|---|---|----|

      WAVEx: 64M SOP44 MASKROM (R6 populated)
      FLx  : Intel E28F320J5 TSOP56 32M FlashROM (R11 & R12 populated)
      ROMx : 64M SOP44 MASKROM (size fixed at 64M, no configure options)
      IC3  : MACH211 CPLD or Cypress CY37064 CPLD (PLCC44, labelled 'KEYCUS' and stamped with 'KC' & a 3-digit number which
             is different per game.)
      IC4  : PALCE 16V8H (PLCC20, PCB labelled 'A_DECO', chip stamped 'M5F4')

This PCB is used on:

              Software
Game          Revision     PCB                                            KEYCUS   ROMs Populated
------------------------------------------------------------------------------------------------------------------
Soul Calibur  SOC11/VER.A2 SYSTEM 12 M5F2 PCB 8661961300 (8661971300)     KC020    SOC1 WAVE0
                                                                                   SOC1 ROM0, SOC1 ROM1, SOC1 ROM2
                                                                                   SOC1 FL3, SOC1 FL4
Soul Calibur  SOC11/VER.B  SYSTEM 12 M5F2 PCB 8661961300 (8661971300)     KC020    SOC1 WAVE0
                                                                                   SOC1 ROM0, SOC1 ROM1, SOC1 ROM2
                                                                                   SOC1 FL3, SOC1 FL4
Aqua Rush     AQ1/VER.A1   Same PCB but sticker says....                  KC053    AQ1 WAVE0
                           'SYSTEM 12 M5F0 PCB 8661962200'                         AQ1 ROM0, AQ1 ROM1
                           (i.e no FlashROMs)

********
*Type 3*
********
SYSTEM 12 M5F4 PCB 8661961200 (8661971200)
|----------------------------------------------------------|
|WAVE0.IC2      IC4                                        |
|                    FL4L.IC5              ROM0.IC9        |
|                         FL4H.IC6                         |
|                             FL3L.IC7        ROM1.IC10    |
|                                 FL3H.IC8                 |
|WAVE1.IC1                                       ROM2.IC11 |
|             IC3  |-------------------|                   |
|                  |-------------------|                   |
|----------------------------------------------------------|
Notes:
      This PCB has zero Ohm resistor jumpers to set the ROM sizes.
      Note X means 'not populated', O means 'populated'
      |----------|----|----|   |----------|----|
      |  MASK    | R6 | R7 |   |  FLASH   | R8 |
      |----------|----|----|   |----------|----|
      | WAVE 32M | X  | O  |   |  NORMAL  | X  |
      |      64M | O  | X  |   |64M STRATA| O  |
      |----------|----|----|   |----------|----|

      WAVEx: 64M SOP44 MASKROM (R6 populated)
      FLx  : Fujitsu 29F016 TSOP48 16M FlashROM (R8 not populated)
      ROMx : 64M SOP44 MASKROM (size fixed at 64M, no configure options)
      IC3  : MACH211 CPLD (PLCC44, labelled 'KEYCUS' and stamped with 'KC' & a 3-digit number which is different per game.)
      IC4  : PALCE 16V8H (PLCC20, PCB labelled 'A_DECO', chip stamped 'M5F4')

This PCB is used on:

               Software
Game           Revision     PCB                                            KEYCUS   ROMs Populated
-------------------------------------------------------------------------------------------------------------------
Fighting Layer FTL0/VER.A  SYSTEM 12 M5F4 PCB 8661961200 (8661971200)     KC037    FTL1 WAVE0, FTL1 WAVE1
                                                                                    FTL1 ROM0, FTL1 ROM1, FTL1 ROM2
                                                                                    FTL1 FL4L, FTL1 FL4H
                                                                                    FTL1 FL3L, FTL1 FL3H

Super World    SS91/VER.A3 SYSTEM 12 M5F4 PCB 8661961200 (8661971200)     KC027    SS91 WAVE0, SS91 WAVE1
Stadium '99                                                         (sticker KC043) SS91 ROM0, SS91 ROM1
                                                                                    SS91 FL4L, SS91 FL4H
                                                                                    SS91 FL3L, SS91 FL3H

********
*Type 4*
********
SYSTEM 12 M8F2F PCB 8661960301 (8661970301)
|----------------------------------------------------------|
|WAVE0.IC5   ROM0L.IC6        ROM0U.IC9                    |
|                                                          |
|                 ROM1L.IC7        ROM1U.IC10    FL3U.IC13 |
|                                                          |
|                      ROM2L.IC8        ROM2U.IC11         |
|WAVE1.IC4                IC2         IC3        FL3L.IC12 |
|             IC1  |-------------------|                   |
|                  |-------------------|                   |
|----------------------------------------------------------|
Notes:
      This PCB has zero Ohm resistor jumpers to set the ROM sizes.

      |-------|----|
      | W128M | R3 | Wave ROMs are 128M if R3 is populated
      |  W32M | R4 | Wave ROMs are 32M if R4 is populated
      |  W64M | R5 | Wave ROMs are 64M if R5 is populated
      |  M64M | R6 | MASK ROMs are 64M if R6 is populated
      |  M32M | R7 | MASK ROMs are 32M if R7 is populated
      |-------|----|

      WAVEx: 32M/64M SOP44 MASKROM
      FLx  : Fujitsu 29F016 TSOP48 16M FlashROM (size fixed at 16M, no configure options)
      ROMx : 32M/64M SOP44 MASKROM
      IC1  : MACH211 CPLD or Cypress CY37064 CPLD (PLCC44, labelled 'KEYCUS' and stamped with 'KC' & a 3-digit number which
             is different per game.)
      IC2  : 74F139 logic IC
      IC3  : 74HC157 logic IC

This PCB is used on:

               Software
Game           Revision      PCB                                            KEYCUS   ROMs Populated           Jumpers
---------------------------------------------------------------------------------------------------------------------
Tekken 3       TET1/VER.E1   SYSTEM 12 M8F2F PCB 8661960301 (8661970301)    KC006    TET1 WAVE0, TET1 WAVE1   R4, R7
                                                                                     TET1 ROM0U, TET1 ROM1U
                                                                                     TET1 ROM2U, TET1 ROM0L
                                                                                     TET1 ROM1L, TET1 ROM2L
                                                                                     TET1 FL3L, TET1 FL3U

Tekken 3       TET2/VER.A    SYSTEM 12 M8F2F PCB 8661960301 (8661970301)    KC006    TET1 WAVE0, TET1 WAVE1   R4, R7
                                                                                     TET1 ROM0U, TET1 ROM1U
                                                                                     TET1 ROM2U, TET1 ROM0L
                                                                                     TET1 ROM1L, TET1 ROM2L
                                                                                     TET1 FL3L, TET1 FL3U

Tekken 3       TET3/VER.A    SYSTEM 12 M8F2F PCB 8661960301 (8661970301)    KC006    TET1 WAVE0, TET1 WAVE1   R4, R7
                                                                                     TET1 ROM0U, TET1 ROM1U
                                                                                     TET1 ROM2U, TET1 ROM0L
                                                                                     TET1 ROM1L, TET1 ROM2L
                                                                                     TET1 FL3L, TET1 FL3U

Paca Paca      PPP1/VER.A2   Same PCB but sticker says....                  KC038    PPP1 WAVE0, PPP1 WAVE1   R5, R7
Passion                      'SYSTEM 12 M8F0 PCB 8661961700'                         PPP1 ROM0U, PPP1 ROM1U
                             (i.e no FlashROMs)                                      PPP1 ROM2U, PPP1 ROM0L
                                                                                     PPP1 ROM1L, PPP1 ROM2L

Mr Driller     DRI1/VER.A2   Same as Paca Paca Passion                      KC048    DRI1 WAVE0               R5, R7
                                                                                     DRI1 ROM0U, DRI1 ROM0L

Libero Grande  LG2/VER.A     SYSTEM 12 M8F2F PCB 8661960301 (8661970301)    KC014    LG1 WAVE0                R4, R7
                                                                                     LG1 ROM0U, LG1 ROM0L
                                                                                     LG1 FL3L, LG1 FL3U


********
*Type 5*
********
SYSTEM 12 M8F4 PCB 8661961901 (8661971901)
|----------------------------------------------------------|
|          IC3                              *ROM0O.IC13    |
|               FLEL.IC4    FLOL.IC6         ROM0E.IC9     |
|                                                          |
| WAVE0.IC1         FLEU.IC5    FLOU.IC7    *ROM1O.IC14    |
|*WAVE1.IC12                                 ROM1E.IC10    |
|                                       IC8                |
|             IC2  |-------------------|    *ROM2O.IC15    |
|                  |-------------------|     ROM2E.IC11    |
|----------------------------------------------------------|
Notes:
      This PCB has zero Ohm resistor jumpers to set the ROM sizes.
      Note X means 'not populated', O means 'populated'
      |----------|----|----|
      |   MASK   | R2 | R3 |
      |----------|----|----|
      | WAVE 32M | O  | X  |
      |      64M | X  | O  |
      |----------|----|----|

      *    : These parts on other side of PCB
      WAVEx: 32M/64M SOP44 MASKROM
      FLx  : Fujitsu 29F016 TSOP48 16M FlashROM (size fixed at 16M, no configure options)
      ROMx : 64M SOP44 MASKROM (size fixed at 64M, no configure options)
      IC2  : MACH211 CPLD (PLCC44, labelled 'KEYCUS' and stamped with 'KC' & a 3-digit number which is different per game.)
      IC3  : PALCE 22V10H (PLCC28, labelled 'S12M840A')
      IC8  : ALTERA MAX EPM7128STC100-10 (TQFP100, labelled 'S12M841')

This PCB is used on:

               Software
Game           Revision      PCB                                            KEYCUS   ROMs Populated           Jumper
---------------------------------------------------------------------------------------------------------------------
Tekken Tag     TEG3/VER.B    SYSTEM 12 M8F4 PCB 8661961901 (8661971901)     KC044    TEG1 WAVE0, TEG1 WAVE1   R3
Tournament                                                                           TEG1 ROM0E, TEG1 ROM1E
                                                                                     TEG1 ROM2E, TEG1 ROM0O
                                                                                     TEG1 ROM1O, TEG1 ROM2O
                                                                                     TEG1 FLEL, TEG1 FLEU
                                                                                     TEG1 FLOL, TEG1 FLOU

********
*Type 6*
********
SYSTEM 12 M8F6 PCB 8661961800 (8661971800)
|----------------------------------------------------------|
|          IC3                              *ROM0U.IC14    |
|               FL3L.IC4    FL4U.IC7         ROM0L.IC10    |
|                                                          |
| WAVE0.IC1       FL3U.IC5    FL5L.IC8      *ROM1U.IC15    |
|*WAVE1.IC13                                 ROM1L.IC11    |
|                   FL4L.IC6    FL5U.IC9                   |
|             IC2  |-------------------|    *ROM2U.IC16    |
|                  |-------------------|     ROM2L.IC12    |
|----------------------------------------------------------|
Notes:
      This PCB has zero Ohm resistor jumpers to set the ROM sizes.
      Note X means 'not populated', O means 'populated'
      |----------|----|----|
      |   MASK   | R2 | R3 |
      |----------|----|----|
      | WAVE 32M | O  | X  |
      |      64M | X  | O  |
      |----------|----|----|

      *    : These parts on other side of PCB
      WAVEx: 32M/64M SOP44 MASKROM
      FLx  : Fujitsu 29F016 TSOP48 16M FlashROM (size fixed at 16M, no configure options)
      ROMx : 64M SOP44 MASKROM (size fixed at 64M, no configure options)
      IC2  : Cypress CY37064 CPLD (PLCC44, labelled 'KEYCUS' and stamped with 'KC' & a 3-digit number which is different
             per game.)
      IC3  : PALCE 22V10H (PLCC28, PCB labelled 'A_DECO', chip stamped 'S12M8F6')

This PCB is used on:

               Software
Game           Revision      PCB                                            KEYCUS   ROMs Populated           Jumper
---------------------------------------------------------------------------------------------------------------------
Golgo 13       GLG1/VER.A    SYSTEM 12 M8F6 PCB 8661961800 (8661971800)     KC054    GLG1 WAVE0, GLG1 WAVE1   R3
                                                                                     GLG1 ROM0L, GLG1 ROM1L
                                                                                     GLG1 ROM2L, GLG1 ROM0U
                                                                                     GLG1 ROM1U, GLG1 ROM2U
                                                                                     GLG1 FL3L, GLG1 FL3U
                                                                                     GLG1 FL4L, GLG1 FL4U
                                                                                     GLG1 FL5L, GLG1 FL5U

********
*Type 7*
********
SYSTEM 12 M10X64 PCB ?????????? (??????????)
|----------------------------------------------------------|
|WAVE0                                                     |
|                ROM0L    ROM0U     ROM2L   ROM3L    ROM4L |
|                                                          |
|                ROM1L    ROM1U     ROM2U   ROM3U    ROM4U |
|                                                          |
|WAVE1                                                     |
|           KEYCUS |-------------------|                   |
|                  |-------------------|                   |
|----------------------------------------------------------|
Notes:
      This PCB probably has zero Ohm resistor jumpers to set the ROM sizes,
      however that information was not documented by the dumper. The locations
      of the ROMs and KEYCUS are also unknown, for the same reason. The PCB number
      is also unknown for the same reason. The presence or absence of a PAL is also
      unknown for the same reason.

      WAVEx : 64M SOP44 MASKROM
      ROMx  : 64M SOP44 MASKROM
      KEYCUS: MACH211 CPLD (PLCC44, labelled 'KEYCUS' and stamped with 'KC' & a 3-digit number which is different per game.)

This PCB is used on:

               Software
Game           Revision      PCB                                            KEYCUS   ROMs Populated
-----------------------------------------------------------------------------------------------------------
Derby Quiz My  MDH1/VER.A2   SYSTEM 12 M10X64 PCB ?????????? (??????????)   KC035    MDH1 WAVE0
Dream Horse                                                                          MDH1 ROM0L, MDH1 ROM1L
                                                                                     MDH1 ROM2L, MDH1 ROM0U
                                                                                     MDH1 ROM1U, MDH1 ROM2U

********
*Type 8*
********
JO 11-04-98
|----------------------------------------------------------|
|WAVE0.IC2 IC4                                             |
|           FL4.IC5  ROM0-1.IC9A ROM1-1.IC10A  ROM2-1.IC11A|
|                                                          |
|           FL3.IC6  ROM0.IC9    ROM1.IC10     ROM2.IC11   |
|                                                          |
|WAVE1.IC1                                                 |
|           KEYCUS |-------------------|                   |
|                  |-------------------|                   |
|----------------------------------------------------------|
Notes:
      This PCB _almost_ looks like a bootleg. There are no Namco markings, no Namco part numbers, and all ROMs are unmarked
      except for the original manufacturer part number. The PCB does not have zero Ohm resistor jumpers, but instead, blobs
      of solder to jumper pads which are not labelled anywhere on the PCB.
      There is a location for a KEYCUS chip, but it's not populated. There is a PAL but it's not marked except for the
      original manufacturer part number. The main PCB used by this game was a Tekken 3 which has been converted. The game
      it was converted to (Soul Calibur) was more popular but has an unusual ROM board that might not have been available
      and this PCB might have been produced to replace it, perhaps at a lower cost as well?
      The actual dump of the ROMs on this PCB match the original Soul Calibur dump exactly (a type 2 ROM PCB), even though
      the ROM sizes and types are different.

      WAVEx : MR27C3252CZ 32M SOP44 MASKROM
      ROMx  : MR27C3252CZ 32M SOP44 MASKROM
      FLx   : MR27C3252CZ 32M SOP44 MASKROM
      KEYCUS: Not populated
      IC4   : GAL16V8B (PLCC20, no other markings)

This PCB is used on:

               Software
Game           Revision      PCB                                            KEYCUS   ROMs Populated
-----------------------------------------------------------------------------------------------------------
Soul Calibur   SOC14/VER.C   JO 11-04-98                                    -----    WAVE0, WAVE1
                                                                                     ROM0, ROM0-1, ROM1
                                                                                     ROM1-1, ROM2, ROM2-1
                                                                                     FL3, FL4



CPU PCB
-------
COH-700 GP-15
|-------------------------------------------|
|    74F74   74ALS08                        |
|                                           |
| |------|                                  |
| |KM4132|   |-----------|  |-----------|   |
| |G271Q |   | SONY      |  | SONY      |   |
| |------|   | CXD8654Q  |  | CXD8661R  |   |
|            |           |  |           |   |
| |------|   |           |  |           |   |
| |KM4132|   |           |  |           |   |
| |G271Q |   |-----------|  |-----------|   |
| |------|     53.693MHz       100MHz       |
|                                           |
|                                           |
|                                           |
|                                           |
|                 KM416V1204   KM416V1204   |
|  MC44200FT                                |
|                     *            *        |
|                                           |
|-------------------------------------------|
Notes:
      KM4132G271Q: Samsung Electronics 128K x 32Bit x 2 Banks SGRAM (x2, QFP100)
      KM416V1204 : Samsung Electronics 1M x16 EDO DRAM (x2, TSOP44(40))
      *          : Unpopulated positions for KM416V1204 EDO DRAM
      CXD8654Q   : Sony CXD8654Q GPU (QFP208)
      CXD8661R   : Sony CXD8661R R3000A-based CPU (QFP208)
      MC44200FT  : Motorola MC44200FT Triple 8-bit Video DAC


NETWORK PCB
-----------
SYSTEM12 HSC PCB 8661960601 (8661970601)
|---------------------------------------------------|
|           USB-A   USB-B                           |
|                                   DS8921          |
|                                                   |
|    J2 74LVT245                                    |
|    |-|            N341256         N341256         |
|    | |74LVT245    N341256         N341256         |
|    | |                                            |
|    | |74LVT244                       |------|     |
|    | |        |--------|             | C422 |     |
|    | |74F244  |        |             |      |     |
|    | |        |  C446  |             |------|     |
|    | |74F244  |        |                          |
|    | |        |        |                          |
|    | |74F244  |        |                          |
|    | |        |--------|          74F74  |-----|  |
|    | |74F244               2061ASC-1     |H8/  |  |
|    |-|                                   |3002 |  |
|         S12HSC3A  S12HSC1A  14.7456MHz   |-----|  |
|                                                   |
|                             S12HSC2A    74HC32    |
|                                                   |
|                                                   |
|---------------------------------------------------|
Notes:
      DS8921   : National DS8921ATM Differential Line Driver and Receiver Pair (SOIC8)
      USB-A    : Standard USB type A connector, PCB labelled '(TX)'
      USB-B    : Standard USB type B connector, PCB labelled '(RX)'
      C446     : Namco Custom C446 (QFP160)
      C422     : Namco Custom C422 (QFP64)
      2061ASC-1: IC Designs 2061ASC-1 programmable clock generator (SOIC16)
      N341256  : NKK N341256 32K x8 SRAM (x4, SOJ28)
      H8/3002  : Hitachi H8/3002 HD6413002F17 Microcontroller (QFP100), clocked at 14.7456MHz.
      S12HSC1A : PALCE 16V8H  (PLCC20, labelled 'S12HSC1A')
      S12HSC2A : PALCE 22V10H (PLCC28, labelled 'S12HSC2A')
      S12HSC3A : PALCE 22V10H (PLCC28, labelled 'S12HSC3A')
      J2       : Custom Namco connector for plug-in CPU PCB

      This PCB was found on the following games (so far)....

      Libero Grande (LG2/VER.A)
      Tekken 3      (TET2/VER.A)
      Ehrgeiz       (EG2/VER.A)

      Note that the games will also work without this PCB.
 */

#include "driver.h"
#include "state.h"
#include "cpu/mips/psx.h"
#include "cpu/h83002/h83002.h"
#include "includes/psx.h"
#include "machine/at28c16.h"
#include "sound/c352.h"
#include <time.h>

#define GUNX( a ) ( ( readinputport( a ) * 511 ) / 0x1ff )	// display and game want 512
#define GUNY( a ) ( ( readinputport( a ) * 479 ) / 0x1ff )	// display needs 480
#define GUNY2( a ) ( ( readinputport( a ) * 447 ) / 0x1ff )	// game wants 448

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

static data32_t *namcos12_sharedram;

static WRITE32_HANDLER( sharedram_w )
{
	verboselog( 1, "sharedram_w( %08x, %08x, %08x )\n", ( offset * 4 ), data, mem_mask );
	COMBINE_DATA( &namcos12_sharedram[ offset ] );
}

static READ32_HANDLER( sharedram_r )
{
	verboselog( 1, "sharedram_r( %08x, %08x ) %08x\n", ( offset * 4 ), mem_mask, namcos12_sharedram[ offset ] );
	return namcos12_sharedram[ offset ];
}

static WRITE16_HANDLER( sharedram_sub_w )
{
	data16_t *shared16 = (data16_t *)namcos12_sharedram;

	COMBINE_DATA(&shared16[BYTE_XOR_LE(offset)]);
}

static READ16_HANDLER( sharedram_sub_r )
{
	data16_t *shared16 = (data16_t *)namcos12_sharedram;

	return shared16[BYTE_XOR_LE(offset)];
}

static INTERRUPT_GEN( namcos12_vblank )
{
	psx_vblank();

	/* kludge: protection hacks */
	if( strcmp( Machine->gamedrv->name, "fgtlayer" ) == 0 )
	{
		if( g_p_n_psxram[ 0x2ac494 / 4 ] == 0x080ab125 )
		{
			g_p_n_psxram[ 0x2ac494 / 4 ] = 0;
		}
	}
	else if( strcmp( Machine->gamedrv->name, "pacapp" ) == 0 )
	{
		if( g_p_n_psxram[ 0x16d50 / 4 ] == 0x08005b54 )
		{
			g_p_n_psxram[ 0x16d50 / 4 ] = 0;
		}
	}
}

static data32_t m_n_bankoffset, m_n_bankoffseth;

// called after a state load to properly set things up again
static void s12_resetbank(void)
{
	if (!strcmp( Machine->gamedrv->name, "golgo13" ))
	{
		cpu_setbank( 1, memory_region( REGION_USER2 ) + ((m_n_bankoffseth << 23) | (m_n_bankoffset << 21)) );
	}
	else
	{
		cpu_setbank( 1, memory_region( REGION_USER2 ) + ( m_n_bankoffset * 0x200000 ) );
	}
}

static WRITE32_HANDLER( bankoffset_w )
{
	// Golgo 13 has different banking (maybe the keycus controls it?)
	if (!strcmp( Machine->gamedrv->name, "golgo13" ))
	{
		if (data & 8)
		{
			m_n_bankoffseth = data & 0x6;
			m_n_bankoffset = 0;
		}
		else
		{
			m_n_bankoffset = data & 0x7;
		}
	}
	else
	{
		m_n_bankoffset = data;
	}

	s12_resetbank();

	verboselog( 1, "bankoffset_w( %08x, %08x, %08x ) %08x\n", offset, data, mem_mask, m_n_bankoffset );
}

static data32_t m_n_dmaoffset, m_n_dmabias;

static WRITE32_HANDLER( dmaoffset_w )
{
	if( ACCESSING_LSW32 )
	{
		m_n_dmaoffset = ( offset * 4 ) + ( data << 16 );
	}
	verboselog( 1, "dmaoffset_w( %08x, %08x, %08x ) %08x\n", offset, data, mem_mask, m_n_dmaoffset );
}

static void namcos12_rom_read( UINT32 n_address, INT32 n_size )
{
	INT32 n_left;
	UINT32 *p_n_src;
	UINT32 *p_n_dst;

	verboselog( 1, "namcos12_rom_read( %08x, %08x )\n", n_address, n_size );

	if (( m_n_dmaoffset >= 0x80000000 ) || ( m_n_dmabias == 0x1f300000 ))
	{
		p_n_src = (data32_t *)( memory_region( REGION_USER1 ) + ( m_n_dmaoffset & 0x003fffff ) );
	}
	else
	{
		p_n_src = (data32_t *)( memory_region( REGION_USER2 ) + ( m_n_dmaoffset & 0x7fffffff ) );
	}

	p_n_dst = &g_p_n_psxram[ n_address / 4 ];

	n_left = ( g_n_psxramsize - n_address ) / 4;
	if( n_size > n_left )
	{
		n_size = n_left;
	}
	while( n_size > 0 )
	{
		*( p_n_dst++ ) = *( p_n_src++ );
		n_size--;
	}
}

static WRITE32_HANDLER( s12_dma_bias_w )
{
	m_n_dmabias = data;
}

static ADDRESS_MAP_START( namcos12_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f000000, 0x1f000003) AM_READWRITE(MRA32_NOP, bankoffset_w)			/* banking */
	AM_RANGE(0x1f008000, 0x1f008003) AM_WRITENOP    /* ?? */
	AM_RANGE(0x1f010000, 0x1f010003) AM_WRITENOP    /* ?? */
	AM_RANGE(0x1f018000, 0x1f018003) AM_WRITENOP    /* ?? */
	AM_RANGE(0x1f080000, 0x1f083fff) AM_READWRITE(sharedram_r, sharedram_w) AM_BASE(&namcos12_sharedram) /* shared ram?? */
	AM_RANGE(0x1f140000, 0x1f140fff) AM_READWRITE( at28c16_32le_16lsb_0_r, at28c16_32le_16lsb_0_w ) /* eeprom */
	AM_RANGE(0x1f1bff08, 0x1f1bff0f) AM_WRITENOP    /* ?? */
	AM_RANGE(0x1f700000, 0x1f70ffff) AM_WRITE(dmaoffset_w)  /* dma */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801003) AM_WRITE(s12_dma_bias_w)
	AM_RANGE(0x1f801004, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READNOP
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_NOP
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fa00000, 0x1fbfffff) AM_ROMBANK(1) /* banked roms */
	AM_RANGE(0x1fc00000, 0x1fffffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fffffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfffffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END


static DRIVER_INIT( namcos12 )
{
	psx_driver_init();

	psx_dma_install_read_handler( 5, namcos12_rom_read );

	state_save_register_UINT32( "namcos12", 0, "m_n_dmaoffset", &m_n_dmaoffset, 1 );
	state_save_register_UINT32( "namcos12", 0, "m_n_dmabias", &m_n_dmabias, 1 );
	state_save_register_UINT32( "namcos12", 0, "m_n_bankoffset", &m_n_bankoffset, 1 );
	state_save_register_UINT32( "namcos12", 0, "m_n_bankoffseth", &m_n_bankoffseth, 1 );
	state_save_register_func_postload( s12_resetbank );

	/* kludge: some kind of protection? */
	if( strcmp( Machine->gamedrv->name, "tekkentt" ) == 0 ||
		strcmp( Machine->gamedrv->name, "fgtlayer" ) == 0 ||
		strcmp( Machine->gamedrv->name, "golgo13" ) == 0 ||
		strcmp( Machine->gamedrv->name, "mrdrillr" ) == 0 ||
		strcmp( Machine->gamedrv->name, "pacapp" ) == 0 )
	{
		data8_t *RAM = memory_region( REGION_USER1 );

		*( (data32_t *)&RAM[ 0x20280 ] ) = 0;
		*( (data32_t *)&RAM[ 0x20284 ] ) = 0;
		*( (data32_t *)&RAM[ 0x20288 ] ) = 0;
	}
}

MACHINE_INIT( namcos12 )
{
	psx_machine_init();
	bankoffset_w(0,0,0);
}


/* H8/3002 MCU stuff */
static ADDRESS_MAP_START( s12h8rwmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x08ffff) AM_READWRITE( sharedram_sub_r, sharedram_sub_w )
	AM_RANGE(0x280000, 0x287fff) AM_READWRITE( c352_0_r, c352_0_w )
	AM_RANGE(0x300000, 0x300001) AM_READ( input_port_1_word_r )
	AM_RANGE(0x300002, 0x300003) AM_READ( input_port_2_word_r )
	AM_RANGE(0x300010, 0x300011) AM_NOP	// golgo13 writes here a lot, possibly also a wait state generator?
	AM_RANGE(0x300030, 0x300031) AM_NOP	// most S12 bioses write here simply to generate a wait state.  there is no deeper meaning.
ADDRESS_MAP_END

static READ8_HANDLER( s12_mcu_p8_r )
{
	return 0x02;
}

// emulation of the Epson R4543 real time clock
// in System 12, bit 0 of H8/3002 port A is connected to it's chip enable
// the actual I/O takes place through the H8/3002's serial port B.

static int s12_porta = 0, s12_rtcstate = 0;

static READ8_HANDLER( s12_mcu_pa_r )
{
	return s12_porta;
}

static WRITE8_HANDLER( s12_mcu_pa_w )
{
	// bit 0 = chip enable for the RTC
	// reset the state on the rising edge of the bit
	if ((!(s12_porta & 1)) && (data & 1))
	{
		s12_rtcstate = 0;
	}

	s12_porta = data;
}

INLINE UINT8 make_bcd(UINT8 data)
{
	return ((data / 10) << 4) | (data % 10);
}

static READ8_HANDLER( s12_mcu_rtc_r )
{
	data8_t ret = 0;
	time_t curtime;
	struct tm *exptime;
	static int weekday[7] = { 7, 1, 2, 3, 4, 5, 6 };

	time(&curtime);
	exptime = localtime(&curtime);

	switch (s12_rtcstate)
	{
		case 0:
			ret = make_bcd(exptime->tm_sec);	// seconds (BCD, 0-59) in bits 0-6, bit 7 = battery low
			break;
		case 1:
			ret = make_bcd(exptime->tm_min);	// minutes (BCD, 0-59)
			break;
		case 2:
			ret = make_bcd(exptime->tm_hour);	// hour (BCD, 0-23)
			break;
		case 3:
			ret = make_bcd(weekday[exptime->tm_wday]); // day of the week (1 = Monday, 7 = Sunday)
			break;
		case 4:
			ret = make_bcd(exptime->tm_mday);	// day (BCD, 1-31)
			break;
		case 5:
			ret = make_bcd(exptime->tm_mon + 1);	// month (BCD, 1-12)
			break;
		case 6:
			ret = make_bcd(exptime->tm_year % 100);	// year (BCD, 0-99)
			break;
	}

	s12_rtcstate++;

	return ret;
}

static int s12_lastpB = 0x50, s12_setstate = 0, s12_setnum, s12_settings[8];

static READ8_HANDLER( s12_mcu_portB_r )
{
	// golgo13 won't boot if this doesn't toggle every read
	s12_lastpB ^= 0x80;
	return s12_lastpB;
}

static WRITE8_HANDLER( s12_mcu_portB_w )
{
	// bit 7 = chip enable for the video settings controller
	if (data & 0x80)
	{
		s12_setstate = 0;
	}

	s12_lastpB = data;
}

static WRITE8_HANDLER( s12_mcu_settings_w )
{
	if (s12_setstate)
	{
		// data
		s12_settings[s12_setnum] = data;

		if (s12_setnum == 7)
		{
			logerror("S12 video settings: Contrast: %02x  R: %02x  G: %02x  B: %02x\n",
				BITSWAP8(s12_settings[0], 0, 1, 2, 3, 4, 5, 6, 7),
				BITSWAP8(s12_settings[1], 0, 1, 2, 3, 4, 5, 6, 7),
				BITSWAP8(s12_settings[2], 0, 1, 2, 3, 4, 5, 6, 7),
				BITSWAP8(s12_settings[3], 0, 1, 2, 3, 4, 5, 6, 7));
		}
	}
	else
	{	// setting number
		s12_setnum = (data >> 4)-1;
	}

	s12_setstate ^= 1;
}

/* Golgo 13 lightgun inputs
 *
 * Note: The H8/3002's ADC is 10 bits wide, but
 * it expects the 10-bit value to be left-justified
 * within the 16-bit word.  Hence the odd shifting
 * here.
 */

static READ8_HANDLER( s12_mcu_gun_h_r )
{
	int rv;

	// if game has no lightgun ports, return 0
	if (port_tag_to_index("IN3") == -1)
	{
		return 0;
	}

	rv = GUNX(3) + 156;

	if (offset & 1)
	{
		return (rv&0x3)<<6;
	}

	return (rv&0x3fc)>>2;
}

static READ8_HANDLER( s12_mcu_gun_v_r )
{
	int rv;

	// if game has no lightgun ports, return 0
	if (port_tag_to_index("IN4") == -1)
	{
		return 0;
	}

	rv = 0x1de - (GUNY2(4));	// + 0x1e = way on top

	if (offset & 1)
	{
		return (rv&0x3)<<6;
	}

	return (rv&0x3fc)>>2;
}

static ADDRESS_MAP_START( s12h8iomap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(H8_PORT7, H8_PORT7) AM_READ( input_port_0_r )
	AM_RANGE(H8_PORT8, H8_PORT8) AM_READ( s12_mcu_p8_r ) AM_WRITENOP
	AM_RANGE(H8_PORTA, H8_PORTA) AM_READWRITE( s12_mcu_pa_r, s12_mcu_pa_w )
	AM_RANGE(H8_PORTB, H8_PORTB) AM_READWRITE( s12_mcu_portB_r, s12_mcu_portB_w )
	AM_RANGE(H8_SERIAL_B, H8_SERIAL_B) AM_READ( s12_mcu_rtc_r ) AM_WRITE( s12_mcu_settings_w )
	AM_RANGE(H8_ADC_0_H, H8_ADC_0_L) AM_NOP
	AM_RANGE(H8_ADC_1_H, H8_ADC_1_L) AM_READ( s12_mcu_gun_h_r )	// golgo 13 gun X-axis
	AM_RANGE(H8_ADC_2_H, H8_ADC_2_L) AM_READ( s12_mcu_gun_v_r )	// golgo 13 gun Y-axis
	AM_RANGE(H8_ADC_3_H, H8_ADC_3_L) AM_NOP
ADDRESS_MAP_END

static struct C352interface c352_interface =
{
	REGION_SOUND1
};

static VIDEO_UPDATE( golgo13 )
{
	video_update_psx( bitmap, cliprect );

	draw_crosshair(bitmap, GUNX(3), GUNY(4), cliprect, 0);
}

static MACHINE_DRIVER_START( coh700 )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( namcos12_map, 0 )
	MDRV_CPU_VBLANK_INT( namcos12_vblank, 1 )

	MDRV_CPU_ADD(H83002, 14745600 )	/* verified 14.7456 MHz */
	MDRV_CPU_PROGRAM_MAP( s12h8rwmap, 0 )
	MDRV_CPU_IO_MAP( s12h8iomap, 0 )
	MDRV_CPU_VBLANK_INT( irq1_line_pulse, 1 );

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( namcos12 )
	MDRV_NVRAM_HANDLER( at28c16_0 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(C352, 0)
	MDRV_SOUND_CONFIG(c352_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
	MDRV_SOUND_ROUTE(2, "left", 0.50)
	MDRV_SOUND_ROUTE(3, "right", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( golgo13 )
	MDRV_IMPORT_FROM( coh700 )

	MDRV_VIDEO_UPDATE( golgo13 )
MACHINE_DRIVER_END

INPUT_PORTS_START( namcos12 )
	/* IN 0 */
	PORT_START_TAG("IN0")
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR(Service_Mode) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Freeze" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0xff3f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 1 */
	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	/* IN 2 */
	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON6 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END

INPUT_PORTS_START( golgo13 )
	/* IN 0 */
	PORT_START_TAG("IN0")
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR(Service_Mode) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Freeze" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0xff3f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 1 */
	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	/* IN 2 */
	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Test Switch") PORT_CODE(KEYCODE_F2) PORT_TOGGLE //That should help with test mode
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SERVICE1 )

	/* IN 3 */
	PORT_START_TAG("IN3")
	PORT_BIT( 0x1ff, 0x00, IPT_LIGHTGUN_X ) PORT_MINMAX(0,0x1ff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)

	/* IN 4 */
	PORT_START_TAG("IN4")
	PORT_BIT( 0x1ff, 0x00, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,0x1ff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15) PORT_PLAYER(1)
INPUT_PORTS_END

ROM_START( aquarush )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "aq1vera.2l",   0x0000000, 0x200000, CRC(91eb9258) SHA1(30e225eb551bfe1bed6b342dd6d597345d64b677) )
	ROM_LOAD16_BYTE( "aq1vera.2p",   0x0000001, 0x200000, CRC(a92f21aa) SHA1(bde33f1f66aaa55031c6b2972b042eef87047cce) )

	ROM_REGION32_LE( 0x1000000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD(        "aq1rom0.7",    0x000000, 0x800000, CRC(3e64dd33) SHA1(dce3bb84c3069bc202d04f24d7702158dd3194d4) )
	ROM_LOAD(        "aq1rom1.8",    0x800000, 0x800000, CRC(e4d415cf) SHA1(bbd244adaf704d7daf7341ff3b0a92162927a59b) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "aq1vera.11s", 0x000000, 0x080000, CRC(78277e02) SHA1(577ebb6d7ab5e304fb1dc1e7fd5649762e0d1786) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "aq1wave0.2",          0x000000, 0x800000, CRC(0cf7278d) SHA1(aee31e4d9b3522f42325071768803c542aa6de09) )
ROM_END

ROM_START( ehrgeiz )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "eg3vera.2l",   0x0000000, 0x200000, CRC(64c00ff0) SHA1(fc7980bc8d98c810aed2eb6b3265d150784dfc15) )
	ROM_LOAD16_BYTE( "eg3vera.2p",   0x0000001, 0x200000, CRC(e722c030) SHA1(4669a7861c14d97048728989708a0fa3733f83a8) )

	ROM_REGION32_LE( 0x1c00000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "eg1rom0l.12",  0x0000000, 0x800000, CRC(fe11a48e) SHA1(bdb2af5949bb4a324dab01489e9488aa5b9bd129) )
	ROM_LOAD16_BYTE( "eg1rom0u.11",  0x0000001, 0x800000, CRC(7cb90c25) SHA1(0ff6f7bf8b7eb2e0706bd235fcb9a95ea639fae6) )
	ROM_LOAD16_BYTE( "eg1fl1l.9",    0x1000000, 0x200000, CRC(dd4cac2b) SHA1(f49d0055303b3d8639e94f93bce6bf160cc99913) )
	ROM_LOAD16_BYTE( "eg1fl1u.10",   0x1000001, 0x200000, CRC(e3348407) SHA1(829bf1be0f74fd385e325d774f1449e28aba1e4d) )
	ROM_LOAD16_BYTE( "eg1fl2l.7",    0x1400000, 0x200000, CRC(34493262) SHA1(9dd5b1b25e7232460bf5ee5b339d9f536ec26979) )
	ROM_LOAD16_BYTE( "eg1fl2u.8",    0x1400001, 0x200000, CRC(4fb84f1d) SHA1(714e0ef56871d7b4568bc6dda08bbb1c01dbba37) )
	ROM_LOAD16_BYTE( "eg1fl3l.5",    0x1800000, 0x200000, CRC(f9441f87) SHA1(70944160620ba2fc80b4fc3a7b61c33622298a8b) )
	ROM_LOAD16_BYTE( "eg1fl3u.6",    0x1800001, 0x200000, CRC(cea397de) SHA1(3791dbadb5699c805c27930e59c61af4d62f77f7) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "eg3vera.11s", 0x0000000, 0x080000, CRC(9e44645e) SHA1(374eb4a4c09d6b5b7af5ff0efec16b4d2aacbe2b) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "eg1wave0.2",          0x0000000, 0x800000, CRC(961fe69f) SHA1(0189a061959a8d94b9d2db627911264faf9f28fd) )
ROM_END

ROM_START( ehrgeiza )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "eg2vera.2e",   0x0000000, 0x200000, CRC(9174ec90) SHA1(273bb9c9f0a7eb48470601a0eadf450908ac4d92) )
	ROM_LOAD16_BYTE( "eg2vera.2j",   0x0000001, 0x200000, CRC(a8388248) SHA1(89eeb6095cc8c7ad6cdc8480cff6f688f07f64d7) )

	ROM_REGION32_LE( 0x1c00000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "eg1rom0l.12",  0x0000000, 0x800000, CRC(fe11a48e) SHA1(bdb2af5949bb4a324dab01489e9488aa5b9bd129) )
	ROM_LOAD16_BYTE( "eg1rom0u.11",  0x0000001, 0x800000, CRC(7cb90c25) SHA1(0ff6f7bf8b7eb2e0706bd235fcb9a95ea639fae6) )
	ROM_LOAD16_BYTE( "eg1fl1l.9",    0x1000000, 0x200000, CRC(dd4cac2b) SHA1(f49d0055303b3d8639e94f93bce6bf160cc99913) )
	ROM_LOAD16_BYTE( "eg1fl1u.10",   0x1000001, 0x200000, CRC(e3348407) SHA1(829bf1be0f74fd385e325d774f1449e28aba1e4d) )
	ROM_LOAD16_BYTE( "eg1fl2l.7",    0x1400000, 0x200000, CRC(34493262) SHA1(9dd5b1b25e7232460bf5ee5b339d9f536ec26979) )
	ROM_LOAD16_BYTE( "eg1fl2u.8",    0x1400001, 0x200000, CRC(4fb84f1d) SHA1(714e0ef56871d7b4568bc6dda08bbb1c01dbba37) )
	ROM_LOAD16_BYTE( "eg1fl3l.5",    0x1800000, 0x200000, CRC(f9441f87) SHA1(70944160620ba2fc80b4fc3a7b61c33622298a8b) )
	ROM_LOAD16_BYTE( "eg1fl3u.6",    0x1800001, 0x200000, CRC(cea397de) SHA1(3791dbadb5699c805c27930e59c61af4d62f77f7) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "eg3vera.11s", 0x0000000, 0x080000, CRC(9e44645e) SHA1(374eb4a4c09d6b5b7af5ff0efec16b4d2aacbe2b) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "eg1wave0.2",          0x0000000, 0x800000, CRC(961fe69f) SHA1(0189a061959a8d94b9d2db627911264faf9f28fd) )
ROM_END

ROM_START( fgtlayer )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "ftl1vera.2e",  0x0000000, 0x200000, CRC(f4156e79) SHA1(cedb917940be8c74fa4ddb48213ce6917444e306) )
	ROM_LOAD16_BYTE( "ftl1vera.2j",  0x0000001, 0x200000, CRC(c65b57c0) SHA1(0051aa46d09fbe9d896ae5f534e21955373f1d46) )

	ROM_REGION32_LE( 0x2000000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD(        "ftl1rom0.9",   0x0000000, 0x800000, CRC(e33ce365) SHA1(f7e5b98d2e8e5f233265909477c84331f016ebfb) )
	ROM_LOAD(        "ftl1rom1.10",  0x0800000, 0x800000, CRC(a1ec7d08) SHA1(e693a0b16235c44d4165b2f53dc25d5288297c27) )
	ROM_LOAD(        "ftl1rom2.11",  0x1000000, 0x800000, CRC(204be858) SHA1(95b22b678b7d11b0ec907698c18cebd84437c656) )
	ROM_LOAD16_BYTE( "ftl1fl3l.7",   0x1800000, 0x200000, CRC(e4fb01e1) SHA1(c7883e86afb58412281a317bfdf62a21488421be) )
	ROM_LOAD16_BYTE( "ftl1fl3h.8",   0x1800001, 0x200000, CRC(67a5c56f) SHA1(6f0a4f93b4975b24efb26c3dd47b791c92c55fbf) )
	ROM_LOAD16_BYTE( "ftl1fl4l.5",   0x1c00000, 0x200000, CRC(8039242c) SHA1(9c3f9637d7cd0c004c0c39aee815eed228ebad20) )
	ROM_LOAD16_BYTE( "ftl1fl4h.6",   0x1c00001, 0x200000, CRC(5ad59726) SHA1(b3a68f7ba2052b99407a5423223202001f2a4f67) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "ftl1vera.11s", 0x0000000, 0x080000, CRC(e3f957cd) SHA1(1c7f2033025fb9c40654cff26d78697baf697c59) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "ftl1wave0.2",         0x0000000, 0x800000, CRC(ee009a2b) SHA1(c332bf59917b2673d7acb864bf92d25d74a350b6) )
	ROM_LOAD( "ftl1wave1.1",         0x0800000, 0x800000, CRC(a54a89cd) SHA1(543b47c6442f7a78e26992b041235a91d719bb89) )
ROM_END

ROM_START( golgo13 )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "glg1vera.2l",  0x0000000, 0x200000, CRC(aa15abfe) SHA1(e82b408746e01c50c5cb0dcef804974d1e97078a) )
	ROM_LOAD16_BYTE( "glg1vera.2p",  0x0000001, 0x200000, CRC(37a4cf90) SHA1(b5470d44036e9de8220b669f71b50bcec42d9a18) )

	ROM_REGION32_LE( 0x3c00000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "glg1rom0l.10", 0x0000000, 0x800000, CRC(2837524e) SHA1(81d811c2cf8121854ed6046dc421cda7fcd44014) )
	ROM_LOAD16_BYTE( "glg1rom0u.14", 0x0000001, 0x800000, CRC(4482deff) SHA1(03dcd12633b167d9af664ae826fc2c2ff096c0a6) )
	ROM_LOAD16_BYTE( "glg1rom1l.11", 0x1000000, 0x800000, CRC(080f3550) SHA1(40a5257baeb366e32ff51f571efb12a556f93430) )
	ROM_LOAD16_BYTE( "glg1rom1u.15", 0x1000001, 0x800000, CRC(b49e8613) SHA1(d25ede8046ce309d5a5515a2e44b6773fdd56333) )
	ROM_LOAD16_BYTE( "glg1rom2l.12", 0x2000000, 0x800000, CRC(e1a79a87) SHA1(c3f31b1bc6a3b51361df0d89d3ff0717875fd800) )
	ROM_LOAD16_BYTE( "glg1rom2u.16", 0x2000001, 0x800000, CRC(cda22852) SHA1(e609a55e0ec91b532469c3607dcc96456eec6a07) )
	ROM_LOAD16_BYTE( "glg1fl3l.4",   0x3000000, 0x200000, CRC(16ddc2cb) SHA1(71460bd4ad35b3488dc1326fe2f600df40988e06) )
	ROM_LOAD16_BYTE( "glg1fl3u.5",   0x3000001, 0x200000, CRC(d90d72d1) SHA1(0027389344862486c53cd0e1554b36eed65aa5b0) )
	ROM_LOAD16_BYTE( "glg1fl4l.6",   0x3400000, 0x200000, CRC(820d8133) SHA1(2d780612055a0fa5de756cfa46ce7f134139e550) )
	ROM_LOAD16_BYTE( "glg1fl4u.7",   0x3400001, 0x200000, CRC(02d78160) SHA1(2eafcbd3ba550acf7816074e6c7eee65f7d64859) )
	ROM_LOAD16_BYTE( "glg1fl5l.8",   0x3800000, 0x200000, CRC(090b2508) SHA1(c3587aeab71f6a9dcc90bf2af496303e9d20eac8) )
	ROM_LOAD16_BYTE( "glg1fl5u.9",   0x3800001, 0x200000, CRC(2231a07c) SHA1(480e17219101a0dae5cd64507e31cd7e711c95fa) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "glg1vera.11s", 0x0000000, 0x080000, CRC(5c33f240) SHA1(ec8fc8d83466b28dfa35b93e16d8164883513b19) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "glg1wave0.1",         0x0000000, 0x800000, CRC(672d4f7c) SHA1(16a7564a2c68840a438a33ac2381df4e70e1bb45) )
	ROM_LOAD( "glg1wave1.13",        0x0800000, 0x800000, CRC(480b0a1a) SHA1(341d5ec8ad0f3c0a121eeeec9466aaeec2bd1c74) )
ROM_END

ROM_START( lbgrande )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "lg2vera.2l",   0x0000000, 0x200000, CRC(5ed6b152) SHA1(fdab457862bd6e0a3178c9329bd0978b6aa3ae2f) )
	ROM_LOAD16_BYTE( "lg2vera.2p",   0x0000001, 0x200000, CRC(97c57149) SHA1(bb9bc1ba3ea826eb1c987b11218c0afa0fc54bdc) )

	ROM_REGION32_LE( 0x1c00000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "lg1rom0l.6",   0x0000000, 0x400000, CRC(c5df7f27) SHA1(07c596efb2533b9adc579874b7e8ef7fcc7f73c3) )
	ROM_LOAD16_BYTE( "lg1rom0u.9",   0x0000001, 0x400000, CRC(74607817) SHA1(448a9213fa566fdbab5d789df064da7dc946ba2c) )
	ROM_LOAD16_BYTE( "lg1fl3l.12",   0x1800000, 0x200000, CRC(c9947d3e) SHA1(239b1f81ffac6a54b438082664124b6cf51a9b1c) )
	ROM_LOAD16_BYTE( "lg1fl3u.13",   0x1800001, 0x200000, CRC(f3d69f45) SHA1(546f588f144e1a75eee4a6d0a6cef8a3f79f0238) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "lg1vera.11s", 0x0000000, 0x080000, CRC(de717a09) SHA1(78f26ff630c50632916fa17fa870dcde7f13781d) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "lg1wave0.5",          0x0000000, 0x400000, CRC(4647fada) SHA1(99f5e9ded0c83f1a0d3670f6380bc15c1380671e) )
ROM_END

ROM_START( mdhorse )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "mdh1vera.2l",  0x0000000, 0x200000, CRC(fbb567b2) SHA1(899dccdfbc8dcbcdaf9b5df93e249a36f8cbf999) )
	ROM_LOAD16_BYTE( "mdh1vera.2p",  0x0000001, 0x200000, CRC(a0f182ab) SHA1(70c789ea88248c1f810f9fdb3feaf808acbaa8cd) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "mdh1rom0l",    0x0000000, 0x800000, CRC(ca5bf806) SHA1(8c88b1f29c96a0696ac9428c411cde10faacce35) )
	ROM_LOAD16_BYTE( "mdh1rom0u",    0x0000001, 0x800000, CRC(315e9539) SHA1(340fcd196f53f64b3f56ef73101eddc9e142d907) )
	ROM_LOAD16_BYTE( "mdh1rom1l",    0x1000000, 0x800000, CRC(9f610211) SHA1(8459733c52d1c62033a4aeb9985b4a6e863a62d0) )
	ROM_LOAD16_BYTE( "mdh1rom1u",    0x1000001, 0x800000, CRC(a2e43560) SHA1(b0657a22701c6f2098f210d59e4c9bc88593a750) )
	ROM_LOAD16_BYTE( "mdh1rom2l",    0x2000000, 0x800000, CRC(84840fa9) SHA1(0f051db55b30f7dd473f4df9bb5e7d38c39dc785) )
	ROM_LOAD16_BYTE( "mdh1rom2u",    0x2000001, 0x800000, CRC(9490dafe) SHA1(c1bd9343535876eac5369a86105013b4a7d731b3) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "mdh1vera.11s", 0x0000000, 0x080000, CRC(20d7ba29) SHA1(95a056d1f1ac70dda8ced832b506076485348a33) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "mdh1wave0",           0x0000000, 0x800000, CRC(7b031123) SHA1(7cbc1f71d259405f9f1ef26026d51abcb255b057) )
ROM_END

ROM_START( mrdrillr )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "dri1vera.2l",  0x0000000, 0x200000, CRC(751ca21d) SHA1(1c271bba83d387c797ce8daa43885bcb6e1a51a6) )
	ROM_LOAD16_BYTE( "dri1vera.2p",  0x0000001, 0x200000, CRC(2a2b0704) SHA1(5a8b40c6cf0adc43ca2ee0c576ec82f314aacd2c) )

	ROM_REGION32_LE( 0x0800000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "dri1rom0l.6",  0x0000000, 0x400000, CRC(021bb2fa) SHA1(bfe3e46e9728d5b5a692f432515267ff8b8255e7) )
	ROM_LOAD16_BYTE( "dri1rom0u.9",  0x0000001, 0x400000, CRC(5aae85ea) SHA1(a54dcc050c12ed3d77efc328e366e99c392eb139) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "dri1vera.11s", 0x0000000, 0x080000, CRC(33ea9c0e) SHA1(5018d7a1a45ec3133cd928435db8804f66321924) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "dri1wave0.5",         0x0000000, 0x800000, CRC(32928df1) SHA1(79af92a2d24a0e3d5bfe1785776b0f86a93882ce) )
ROM_END

ROM_START( pacapp )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "ppp1vera.2l",  0x0000000, 0x200000, CRC(6e74bd05) SHA1(41a2e06538cea3bced2992f5858a3f0cd1c0b4aa) )
	ROM_LOAD16_BYTE( "ppp1vera.2p",  0x0000001, 0x200000, CRC(b7a2f724) SHA1(820ae04ec416b8394a1d919279748bde3460cb96) )

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "ppp1rom0l.6",  0x0000000, 0x400000, CRC(b152fdd8) SHA1(23c5c07680a62e941a7b1a28897f986dd9399801) )
	ROM_LOAD16_BYTE( "ppp1rom0u.9",  0x0000001, 0x400000, CRC(c615c26e) SHA1(db1aae37ebee2a74636415e4b1b0b17790a6a67e) )
	ROM_LOAD16_BYTE( "ppp1rom1l.7",  0x0800000, 0x400000, CRC(46eaedbd) SHA1(afe3c966fcf083d89b45d44d871bed1b8caa3014) )
	ROM_LOAD16_BYTE( "ppp1rom1u.10", 0x0800001, 0x400000, CRC(32f27dce) SHA1(06de870b83c972403d96b8b9a8ee5a192a99451d) )
	ROM_LOAD16_BYTE( "ppp1rom2l.8",  0x1000000, 0x400000, CRC(dca7e5ed) SHA1(b43b6e086912009295ceae11d2d540733353d7b6) )
	ROM_LOAD16_BYTE( "ppp1rom2u.11", 0x1000001, 0x400000, CRC(cada7a0d) SHA1(e8d927a4680911b77de1d906b5e0140697e9c67b) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "ppp1vera.11s", 0x0000000, 0x080000, CRC(22242317) SHA1(e46bf3c594136168faedebbd59f53ff9a6ecf3c1) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "ppp1wave0.5",         0x0000000, 0x800000, CRC(184ccc7d) SHA1(b74638cebef209638c625aac7e8c5b924f56a8bb) )
	ROM_LOAD( "ppp1wave1.4",         0x0800000, 0x800000, CRC(cbcf74c5) SHA1(a089277c9befc87b5bbe0d4e5b8187a4ad5ef143) )
ROM_END

ROM_START( soulclbr )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "soc14verc.2e", 0x0000000, 0x200000, CRC(c40e9614) SHA1(dc20469f0d657423e472fdf5897852ab9fb8bb73) )
	ROM_LOAD16_BYTE( "soc14verc.2j", 0x0000001, 0x200000, CRC(80c41446) SHA1(e5620a4f0ffba913169a779df73384b7ca8780b9) )

	ROM_REGION32_LE( 0x2000000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD(        "soc1rom0.7",   0x0000000, 0x800000, CRC(cdc47b55) SHA1(315ea6b819de5c4883aa400f1b9f4172637757bf) )
	ROM_LOAD(        "soc1rom1.8",   0x0800000, 0x800000, CRC(30d2dd5a) SHA1(1c0c467ba339e0241efb8d5c3b025a046b2ca676) )
	ROM_LOAD(        "soc1rom2.9",   0x1000000, 0x800000, CRC(dbb93955) SHA1(624cd8ad94e8ae53206f798bff81784afe95e5f1) )
	ROM_LOAD(        "soc1fl3.6",    0x1800000, 0x400000, CRC(24d94c38) SHA1(0f9b9ab11dd4e02086d7b9104ce2f5d4e93cd696) )
	ROM_LOAD(        "soc1fl4.5",    0x1c00000, 0x400000, CRC(6212090e) SHA1(ed5e50771180935a0c2e760e7369673098722201) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "soc1vera.11s", 0x0000000, 0x080000, CRC(52aa206a) SHA1(5abe9d6f800fa1b9623aa08b16e9b959b840e50b) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "soc1wave0.2",         0x0000000, 0x800000, CRC(c100618d) SHA1(b87f88ee42ad9c5affa674e5f816d902143fed99) )
ROM_END

ROM_START( soulclba )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "soc1vera.2l",  0x0000000, 0x200000, CRC(37e0a203) SHA1(3915b5e530c8e70a07aa8ccedeb66633ae5f670e) )
	ROM_LOAD16_BYTE( "soc1vera.2p",  0x0000001, 0x200000, CRC(7cd87a35) SHA1(5a4837b6f6a49c88126a0ddbb8059a4da77127bc) )

	ROM_REGION32_LE( 0x2000000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD(        "soc1rom0.7",   0x0000000, 0x800000, CRC(cdc47b55) SHA1(315ea6b819de5c4883aa400f1b9f4172637757bf) )
	ROM_LOAD(        "soc1rom1.8",   0x0800000, 0x800000, CRC(30d2dd5a) SHA1(1c0c467ba339e0241efb8d5c3b025a046b2ca676) )
	ROM_LOAD(        "soc1rom2.9",   0x1000000, 0x800000, CRC(dbb93955) SHA1(624cd8ad94e8ae53206f798bff81784afe95e5f1) )
	ROM_LOAD(        "soc1fl3.6",    0x1800000, 0x400000, CRC(24d94c38) SHA1(0f9b9ab11dd4e02086d7b9104ce2f5d4e93cd696) )
	ROM_LOAD(        "soc1fl4.5",    0x1c00000, 0x400000, CRC(6212090e) SHA1(ed5e50771180935a0c2e760e7369673098722201) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "soc1vera.11s", 0x0000000, 0x080000, CRC(52aa206a) SHA1(5abe9d6f800fa1b9623aa08b16e9b959b840e50b) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "soc1wave0.2",         0x0000000, 0x800000, CRC(c100618d) SHA1(b87f88ee42ad9c5affa674e5f816d902143fed99) )
ROM_END

ROM_START( sws99 )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "ss91vera.2e",  0x0000000, 0x200000, CRC(4dd928d7) SHA1(d76c0f52d1a2cd101a6879e6ff57ed1c52b5e228) )
	ROM_LOAD16_BYTE( "ss91vera.2j",  0x0000001, 0x200000, CRC(40777a48) SHA1(6e3052ddbe3943eb2418cd50102cead88b850240) )

	ROM_REGION32_LE( 0x2000000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD(        "ss91rom0.9",   0x0000000, 0x800000, CRC(db5bc50d) SHA1(b8b59f6db3a374277871c39b3657cb193525e558) )
	ROM_LOAD(        "ss91rom1.10",  0x0800000, 0x800000, CRC(4d71f29f) SHA1(3ccc8410d383bfd9fde44574ebb9c24a235cc734) )
	ROM_FILL(                        0x1000000, 0x800000, 0 )
	ROM_LOAD16_BYTE( "ss91fl3l.7",   0x1800000, 0x200000, CRC(61efd65b) SHA1(1e97d5cd51bf778995c3e568ac7e1d3514264d48) )
	ROM_LOAD16_BYTE( "ss91fl3h.8",   0x1800001, 0x200000, CRC(7f3c8c54) SHA1(b473a503cc5d532922780139ab3cab974d3df65b) )
	ROM_LOAD16_BYTE( "ss91fl4l.5",   0x1c00000, 0x200000, CRC(a6af9511) SHA1(63f14f4fc2b210348e41cdfa552a7a7c86fb1b99) )
	ROM_LOAD16_BYTE( "ss91fl4h.6",   0x1c00001, 0x200000, CRC(be3730a4) SHA1(48802229d31c1eef7f4173eb05060a328b702336) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "ss91vera.11s", 0x0000000, 0x080000, CRC(c6bc5c31) SHA1(c6ef46c3fa8a7749618126d360e614ea6c8d9c54) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "ss91wave0.2",         0x0000000, 0x800000, CRC(1c5e2ff1) SHA1(4f29dfd49f6b5c3ca3b758823d368051354bd673) )
	ROM_LOAD( "ss91wave1.1",         0x0800000, 0x800000, CRC(5f4c8861) SHA1(baee7182c32bc064d805de5a16948faf78941ac4) )
ROM_END

ROM_START( tekken3 )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "tet1vere.2e",  0x0000000, 0x200000, CRC(8b01113b) SHA1(45fdfd58293641ed16bc59c633a85a9cf64ccbaf) )
	ROM_LOAD16_BYTE( "tet1vere.2j",  0x0000001, 0x200000, CRC(df4c96fb) SHA1(2e223045bf5b80ccf615106e869760c5b7aa8d44) )

	ROM_REGION32_LE( 0x1c00000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "tet1rom0l.6",  0x0000000, 0x400000, CRC(2886bb32) SHA1(08ad9da2df25ad8c933a812ac238c81135072929) )
	ROM_LOAD16_BYTE( "tet1rom0u.9",  0x0000001, 0x400000, CRC(c5705b92) SHA1(20df20c8d18eb4712d565a3df9a8d9270dee6aaa) )
	ROM_LOAD16_BYTE( "tet1rom1l.7",  0x0800000, 0x400000, CRC(0397d283) SHA1(ebafcd14cdb2601214129a84fc6830846f5cd274) )
	ROM_LOAD16_BYTE( "tet1rom1u.10", 0x0800001, 0x400000, CRC(502ba5cd) SHA1(19c1282245c6dbfc945c0bd0f3918968c3e5c3ed) )
	ROM_LOAD16_BYTE( "tet1rom2l.8",  0x1000000, 0x400000, CRC(e03b1c24) SHA1(8579b95a8fd06b7d2893ff88b228fd794162dff1) )
	ROM_LOAD16_BYTE( "tet1rom2u.11", 0x1000001, 0x400000, CRC(75eb2ab3) SHA1(dee43884e542391903f6aaae2c166e7921a86fb4) )
	ROM_LOAD16_BYTE( "tet1fl3l.12",  0x1800000, 0x200000, CRC(45513073) SHA1(8a36f58ee2d292b50e00c6bf275f9def358032d8) )
	ROM_LOAD16_BYTE( "tet1fl3u.13",  0x1800001, 0x200000, CRC(1917d993) SHA1(cabc44514a3e62a18a7f8f883603241447d6948b) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "tet1vere.11s", 0x0000000, 0x080000, CRC(c92b98d1) SHA1(8ae6fba8c5b6b9a2ab9541eac8553b282f35750d) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "tet1wave0.5",         0x0000000, 0x400000, CRC(77ba7975) SHA1(fe9434dcf0fb232c85efaaae1b4b13d36099620a) )
	ROM_LOAD( "tet1wave1.4",         0x0400000, 0x400000, CRC(ffeba79f) SHA1(941412bbe9d0305d9a23c224c1bb774c4321f6df) )
ROM_END

ROM_START( tekken3a )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "tet2vera.2e",  0x0000000, 0x200000, CRC(7270f157) SHA1(e73c5970e58f9e8c5696f4e3b15908fbec6c21ce) )
	ROM_LOAD16_BYTE( "tet2vera.2j",  0x0000001, 0x200000, CRC(94ceb446) SHA1(c730eb5c770991ae3ae0b9ba63681ce037e46746) )

	ROM_REGION32_LE( 0x1c00000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD16_BYTE( "tet1rom0l.6",  0x0000000, 0x400000, CRC(2886bb32) SHA1(08ad9da2df25ad8c933a812ac238c81135072929) )
	ROM_LOAD16_BYTE( "tet1rom0u.9",  0x0000001, 0x400000, CRC(c5705b92) SHA1(20df20c8d18eb4712d565a3df9a8d9270dee6aaa) )
	ROM_LOAD16_BYTE( "tet1rom1l.7",  0x0800000, 0x400000, CRC(0397d283) SHA1(ebafcd14cdb2601214129a84fc6830846f5cd274) )
	ROM_LOAD16_BYTE( "tet1rom1u.10", 0x0800001, 0x400000, CRC(502ba5cd) SHA1(19c1282245c6dbfc945c0bd0f3918968c3e5c3ed) )
	ROM_LOAD16_BYTE( "tet1rom2l.8",  0x1000000, 0x400000, CRC(e03b1c24) SHA1(8579b95a8fd06b7d2893ff88b228fd794162dff1) )
	ROM_LOAD16_BYTE( "tet1rom2u.11", 0x1000001, 0x400000, CRC(75eb2ab3) SHA1(dee43884e542391903f6aaae2c166e7921a86fb4) )
	ROM_LOAD16_BYTE( "tet1fl3l.12",  0x1800000, 0x200000, CRC(45513073) SHA1(8a36f58ee2d292b50e00c6bf275f9def358032d8) )
	ROM_LOAD16_BYTE( "tet1fl3u.13",  0x1800001, 0x200000, CRC(1917d993) SHA1(cabc44514a3e62a18a7f8f883603241447d6948b) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "tet1vere.11s", 0x0000000, 0x080000, CRC(c92b98d1) SHA1(8ae6fba8c5b6b9a2ab9541eac8553b282f35750d) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "tet1wave0.5",         0x0000000, 0x400000, CRC(77ba7975) SHA1(fe9434dcf0fb232c85efaaae1b4b13d36099620a) )
	ROM_LOAD( "tet1wave1.4",         0x0400000, 0x400000, CRC(ffeba79f) SHA1(941412bbe9d0305d9a23c224c1bb774c4321f6df) )
ROM_END

ROM_START( tekkentt )
	ROM_REGION32_LE( 0x00400000, REGION_USER1, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "teg3verb.2l",  0x0000000, 0x200000, CRC(1efb7b85) SHA1(0623bb6571caf046ff7b4f83f11ee84a92c4b462) )
	ROM_LOAD16_BYTE( "teg3verb.2p",  0x0000001, 0x200000, CRC(7caef9b2) SHA1(5c56d69ba2f723d0a4fbe4902196efc6ba9d5094) )

	ROM_REGION32_LE( 0x3800000, REGION_USER2, 0 ) /* main data */
	ROM_LOAD32_WORD( "teg3rom0e.9",  0x0000000, 0x800000, CRC(c962a373) SHA1(d662dbd89ef62c5ac3150a018fc2d35ef2ee94ac) )
	ROM_LOAD32_WORD( "teg3rom0o.13", 0x0000002, 0x800000, CRC(badb7dcf) SHA1(8c0bf7f6351c5a2a0996df371a901cf90c68cd8c) )
	ROM_LOAD32_WORD( "teg3rom1e.10", 0x1000000, 0x800000, CRC(b3d56124) SHA1(4df20c74ba63f7362caf15e9b8949fab655704fb) )
	ROM_LOAD32_WORD( "teg3rom1o.14", 0x1000002, 0x800000, CRC(2434ceb6) SHA1(f19f1599acbd6fd48793a2ee5a500ca817d9df56) )
	ROM_LOAD32_WORD( "teg3rom2e.11", 0x2000000, 0x800000, CRC(6e5c3428) SHA1(e3cdb60a4445406877b2e273385f34bfb0974220) )
	ROM_LOAD32_WORD( "teg3rom2o.15", 0x2000002, 0x800000, CRC(21ce9dfa) SHA1(f27e8210ee236c327aa3e1ce4dd408abc6580a1b) )
	ROM_LOAD16_BYTE( "teg3flel.4",   0x3000000, 0x200000, CRC(88b3823c) SHA1(6f31acb642c57daccbfdb87b790037e261c8c73c) )
	ROM_LOAD16_BYTE( "teg3fleu.5",   0x3000001, 0x200000, CRC(36df0867) SHA1(6bec8560ad4c122dc909daa83aa9089ba5b281f7) )
	ROM_LOAD16_BYTE( "teg3flol.6",   0x3400000, 0x200000, CRC(03a76765) SHA1(ae35ae28375f2a3e52d72b77ec09750c326cc269) )
	ROM_LOAD16_BYTE( "teg3flou.7",   0x3400001, 0x200000, CRC(6d6947d1) SHA1(2f307bc4070fadb510c0473bc91d917b2d845ca5) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD16_WORD_SWAP( "teg3verb.11s", 0x0000000, 0x080000, CRC(67d0c469) SHA1(da164702fc21b9f46a9e32c89e7b1d36070ddf79) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "teg3wave0.1",         0x0000000, 0x800000, CRC(4bd99104) SHA1(f76b0576cc28fe49d3c1c402988b933933e52e15) )
	ROM_LOAD( "teg3wave1.12",        0x0800000, 0x800000, CRC(dbc74fff) SHA1(601b7e7361ea744b34e3fa1fc39d88641de7f4c6) )
ROM_END

GAMEX( 1996, tekken3,   0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Tekken 3 (TET1/VER.E1)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1996, tekken3a,  tekken3,  coh700, namcos12, namcos12, ROT0, "Namco",         "Tekken 3 (TET2/VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1997, lbgrande,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Libero Grande (LG2/VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1998, soulclbr,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Soul Calibur (SOC14/VER.C)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND )
GAMEX( 1998, soulclba,  soulclbr, coh700, namcos12, namcos12, ROT0, "Namco",         "Soul Calibur (SOC11/VER.A2)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND )
GAMEX( 1998, ehrgeiz,   0,        coh700, namcos12, namcos12, ROT0, "Square/Namco",  "Ehrgeiz (EG3/VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1998, ehrgeiza,  ehrgeiz,  coh700, namcos12, namcos12, ROT0, "Square/Namco",  "Ehrgeiz (EG2/VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1998, mdhorse,   0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Derby Quiz My Dream Horse (MDH1/VER.A2)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1998, fgtlayer,  0,        coh700, namcos12, namcos12, ROT0, "Arika/Namco",   "Fighting Layer (FTL0/VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1999, pacapp,    0,        coh700, namcos12, namcos12, ROT0, "Produce/Namco", "Paca Paca Passion (PPP1/VER.A2)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) /* id */
GAMEX( 1999, sws99,     0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Super World Stadium '99 (SS91/VER.A3)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) /* id */
GAMEX( 1999, tekkentt,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Tekken Tag Tournament (TEG3/VER.B)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND )
GAMEX( 1999, mrdrillr,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Mr Driller (DRI1/VER.A2)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1999, aquarush,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Aqua Rush (AQ1/VER.A1)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1999, golgo13,   0,        golgo13,golgo13,  namcos12, ROT0, "Raizing/Namco", "Golgo 13 (GLG1/VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND ) /* id */
