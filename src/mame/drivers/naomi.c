/*

  Sega Naomi
 + Related Systems (possibly to be split at a later date)

  Driver by Samuele Zannoli, R. Belmont, and ElSemi,
            David Haywood & Angelo Salese

Sega Naomi is Dreamcast based Arcade hardware.

Current Compatibility notes (GD-rom games only)
|----------|-------|-----|------|------|------|--------------------------------|-----------------------------------------------------|
|romname   | Pl(s) | Rot | Test | Boot | Game | Control Type                   | Notes                                               |
|----------|-------|-----|------|------|------|--------------------------------|-----------------------------------------------------|
|gundmgd   |   1   |  H  | Yes  | Yes  | Yes  | joystick with 4 buttons        |                                                     |
|sfz3ugd   |   2   |  H  | Yes  | Yes  | No   | joystick with 6 buttons        | Hangs after pressing start                          |
|cvsgd     |   2   |  H  | Yes  | Yes  | Yes  | joystick with 4 buttons        | 2 + 2 extra buttons                                 |
|gundmxgd  |   1   |  H  | Yes  | Yes  | Yes  | joystick with 4+6 buttons      |                                                     |
|cvs2gd    |   2   |  H  | Yes  | Yes  | Yes  | joystick with 6 buttons        |                                                     |
|ikaruga   |   2   |  V  | Yes  | Yes  | Yes  | joystick with 3 buttons?       |                                                     |
|ggxx      |   2   |  H  | Yes  | Yes  | Yes  | joystick with 5 buttons        |                                                     |
|moeru     |   2   |  H  | No   | No   | No   | joystick with 3 buttons?       | Broken i/o, stuck at the "now loading" msg          |
|chocomk   |   2   |  H  | Yes  | Yes  | Yes  | joystick with 3 buttons?       |                                                     |
|quizqgd   |   2   |  V  | Yes  | Yes  | Yes  | joystick(?) with 4 buttons     |                                                     |
|ggxxrl    |   2   |  H  | Yes  | Yes  | Yes  | joystick with 5 buttons        |                                                     |
|tetkiwam  |   2   |  H  | No   | No   | No   | joystick with 3 buttons?       | Black screen, tests the SCIF (UART) regs?           |
|shikgam2  |   2   |  V  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o, crashes on the proper gameplay          |
|usagui    |   1   |  H  | Yes  | Yes  | No   | mahjong panel                  |                                                     |
|bdrdown   |   2   |  H  | Yes  | Yes  | Yes  | joystick with 3 buttons?       |                                                     |
|psyvar2   |   1   |  V  | No   | No   | No   | joystick with 3 buttons?       |                                                     |
|cfield    |   1?  |  H  | No?  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|trizeal   |   2   |  V  | No   | No   | No   | joystick with 3 buttons?       |                                                     |
|meltybld  |   2   |  H  | No?  | Yes  | No?  | joystick with 4 buttons?       | Crashes in attract/gameplay with wrong mask         |
|senko     |   2   |  H  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|senkoo    |   2   |  H  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|ss2005    |   1   |  H  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|radirgy   |   1   |  V  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|ggxxsla   |   2   |  H  | Yes  | Yes  | Yes  | joystick with 5 buttons        |                                                     |
|kurucham  |   2   |  H  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o, crashes in attract with wrong mask      |
|undefeat  |   2   |  V  | No   | No   | No   | joystick with 3 buttons?       | Stuck at the "now loading" msg                      |
|trgheart  |   1   |  V  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|jingystm  |   2   |  H  | Yes  | Yes  | No   | joystick with 4 buttons?       | Broken i/o                                          |
|meltyb    |   2   |  H  | No?  | Yes  | No?  | joystick with 4 buttons?       | Crashes in attract/gameplay with wrong mask         |
|meltyba   |   2   |  H  | No?  | Yes  | No?  | joystick with 4 buttons?       | Crashes in attract/gameplay with wrong mask         |
|karous    |   1   |  V  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|takoron   |   2   |  H  | Yes  | Yes  | Yes  | joystick with 3 buttons        |                                                     |
--------------------------------------------------------------------------------------------------------------------------------------
|confmiss  |   2   |  H  | Yes  | Yes  | No   | lightgun                       |                                                     |
|sprtjam   |   2   |  H  | Yes  | Yes  | Yes  | joystick with 2 buttons        |                                                     |
|slashout  |   1   |  H  | Yes  | Yes  | Yes  | joystick with 4 buttons        |                                                     |
|spkrbtl   |   2   |  H  | Yes  | Yes  | No   | joystick with 4 buttons        | Crashes when loading the gameplay                   |
|monkeyba  |   1   |  H  | Yes  | Yes  | No   | ad stick                       | Crashes when loading the gameplay                   |
|dygolf    |   2   |  H  | No   | No   | No   | trackball + 4 buttons          | Moans about the jvs settings                        |
|wsbbgd    |   2   |  H  | No   | No   | No   | ad stick + 2 buttons           | Moans about the jvs settings                        |
|vtennisg  |   2   |  H  | Yes  | Yes  | No   | joystick with 2 buttons        | Broken gfxs, crashes after a while                  |
|vathlete  |   2   |  H  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
|vtennis2  |   2   |  H  | No   | No   | No   | joystick with 3 buttons        |                                                     |
|keyboard  |   2   |  H  | Yes  | Yes  | No   | keyboard                       | Broken i/o, crashes after a while                   |
|lupinsho  |   2   |  H  | Yes  | Yes  | No   | lightgun                       | Broken i/o, crashes after a while                   |
|luptype   |   2   |  H  | Yes  | Yes  | No   | keyboard                       | Broken i/o, crashes after a while                   |
|mok       |   2   |  H  | Yes  | Yes  | No   | lightgun                       | Broken i/o                                          |
|ngdup23a  |   2   |  H  | Yes  | Yes  | Yes  | n/a                            | Missing DIMM emulation                              |
|ngdup23c  |   2   |  H  | Yes  | Yes  | Yes  | n/a                            | Missing DIMM emulation                              |
|puyofev   |   2   |  H  | Yes  | Yes  | No   | joystick with 3 buttons?       | Broken i/o                                          |
--------------------------------------------------------------------------------------------------------------------------------------

Test: If it enters into test mode (not actually working test mode, i.e. if inputs doesn't work)
Boot: If game boots and draws some gfxs
Game: If game can be coined up and can actually be played without problems.
If the three aforementioned are all yes and there aren't any notes, game is fully working.


---------------------------------------------------------------------------------------------------------------------------------------------------

Guru's Readme
-------------

Sega NAOMI Mainboard
Sega, 1998-2005

PCB Layout
----------
837-13544-01
171-7772F
837-13707 (sticker)
(C) SEGA 1999
|---------------------------------------------------|
|    CN1                           CN3              |
|PC910  62256   EPF8452AQC160-3                     |
|    BATTERY EPC1064   JP4  3771  93C46             |
|A179B    315-6188.IC31     3771                3773|
|ADM485  BIOS.IC27   5264165            5264165     |
|                    5264165  |-----|   5264165     |
|    CN3                      |POWER|               |
|                             |VR2  | 33.3333MHZ    |
|CN26                         |-----|          27MHZ|
|                                         CY2308SC-3|
|        KM416S4030      |------|     HY57V161610   |
|                        | SH4  |     HY57V161610   |
| C844G        315-6232  |      |             32MHZ |
|            33.8688MHZ  |------|     HY57V161610   |
|                         xMHz        HY57V161610   |
|      PCM1725    JP1                    62256      |
|                     HY57V161610                   |
|                          HY57V161610              |
|              315-6145                             |
|CN25                CY2308SC-1          315-6146   |
|          LED1                              93C46  |
|          LED2                     14.7456MHZ      |
|---------------------------------------------------|
Notes:
      CN1/2/3         - Connectors for ROM cart
      CN25/26         - Connectors for filter board
      EPF8452AQC160-3 - Altera FLEX EPF8452AQC160-3 FPGA (QFP160)
      315-6188.IC31   - Altera EPC1064 (DIP8)
                        According to the datasheet, it's an FPGA Configuration
                        Device which loads the Altera Flex EPF8452 with some info
                        on power-up.
      JP1             - set to 2-3. Alt setting is 1-2
      JP4             - set to 2-3. Alt setting is 1-2
      93C46           - 128 bytes serial EEPROM
      A179B 96K       - TI SN75179B Differential driver and receiver pair (like RS485)
      ADM485          - Analog Devices ADM485
      BIOS.IC27       - 27C160 EPROM
      5264165         - Hitachi 5264165FTTA60 (video RAM)
      HY57V161610     - Hyundai 57V161610DTC-8 (main program RAM)
      CY2308SC-3      - Clock generator IC
      KM416S4030      - Samsung KM416S4030 16MBit SDRAM (sound related RAM?)
      315-6232        - Sega Custom IC (QFP100)
      315-6145        - Sega Custom IC (QFP56)
      315-6146        - Sega Custom IC (QFP176)
      C844G           - ? (SOIC14)
      62256           - 32kx8 SRAM
      PCM1725         - Burr-Brown PCM1725
      xMHz            - Small round XTAL (possibly 32.768kHz for a clock?)
      SH4             - Hitachi SH4 CPU (BGAxxx, with heatsink and fan)
      POWERVR2        - POWERVR2 video generator (BGAxxx, with heatsink)


Filter Board
------------
839-1069
|----------------------------------------------------|
|SW2 SW1   DIPSW   CN5                      CN12 CN10|
|                                                    |
|                                                    |
|           DIN1                     DIN2            |
|                                                    |
|               CNTX  CNRX                           |
| CN9    CN8                                         |
|    CN7     CN6           CN4       CN3    CN2   CN1|
|----------------------------------------------------|
Notes:
      CN1/CN2   - Power input
      CN3       - HD15 (i.e. VGA connector) RGB Video Output @ 15kHz or 31.5kHz
      CN4       - RCA Audio Output connectors
      CN5       - USB connector (connection to I/O board)
      CN6       - 10 pin connector labelled 'MAPLE 0-1'
      CN7       - 11 pin connector labelled 'MAPLE 2-3'
      CN8       - RS422 connector
      CN9       - Midi connector
      CNTX/CNRX - Network connectors
      DIN1/DIN2 - Connectors joining to mainboard CN25/26
      SW1       - Test Switch
      SW2       - Service Switch
      DIPSW     - 4-position DIP switch block
      CN10      - 12 volt output for internal case exhaust fan
      CN11      - RGB connector (not populated)
      CN12      - 5 volt output connector

---------------------------------------------------------------------------------------------------------------------------------------------------

Guru's Readme
-------------

Atomis Wave (system overview)
Sammy/SEGA, 2002

This is one of the latest arcade systems so far, basically just a Dreamcast using ROM carts.

PCB Layout
----------

Sammy AM3AGA-04 Main PCB 2002 (top side)
1111-00006701 (bottom side)
   |--------------------------------------------|
  |-      TA8210AH               D4721   3V_BATT|
  |VGA                              BS62LV1023TC|
  |VOL                                          |
  |SER      |-----|               PCM1725U      |
  |-        |ROMEO|       D4516161              |
   |CN3     |     |                    BIOS.IC23|
|--|SW1     |-----|                             |
|                     |-----|           |-----| |
|           33.8688MHz|315- |           |ROMEO| |
|           |-----|   |6232 |           |     | |
|           |315- |   |-----|           |-----| |
|           |6258 |  32.768kHz                  |
|J          |-----|                             |
|A                 |-------------|    D4564323  |
|M                 |             |              |
|M                 |             |  |--------|  |
|A        D4516161 |  315-6267   |  |        |  |
|                  |             |  |  SH4   |  |
|                  |             |  |        |  |
|  TD62064         |             |  |        |  |
|         D4516161 |             |  |--------|  |
|                  |-------------|              |
|                                     D4564323  |
|--|             D4516161  D4516161             |
   |                             W129AG  13.5MHz|
   |--------------------------------------------|
Notes:
------
BS62LV1023TC-70 - Brilliance Semiconductor Low Power 128k x8 CMOS SRAM (TSOP32)
TA8210AH        - Toshiba 19W x 2 Channel Power Amplifier
D4516161AG5     - NEC uPD4516161AG5-A80 1M x16 (16MBit) SDRAM (SSOP50)
D4564323        - NEC uPD4564323G5-A10 512K x 32 x 4 (64MBit) SDRAM (SSOP86)
D4721           - NEC uPD4721 RS232 Line Driver Receiver IC (SSOP20)
PCM1725U        - Burr-Brown PCM1725U 16Bit Digital to Analog Converter (SOIC14)
2100            - New Japan Radio Co. Ltd. NJM2100 Dual Op Amp (SOIC8)
ROMEO           - Sammy AX0201A01 'ROMEO' 4111-00000501 0250 K13 custom ASIC (TQFP100)
315-6232        - SEGA 315-6232 custom ASIC (QFP100)
315-6258        - SEGA 315-6258 custom ASIC (QFP56)
315-6267        - SEGA 315-6267 custom ASIC (BGAxxx)
TD62064         - Toshiba TD62064 NPN 50V 1.5A Quad Darlington Driver (SOIC16)
SH4             - Hitachi SH-4 HD6417091RA CPU (BGA256)
BIOS.IC23       - Macronix 29L001MC-10 3.3volt FlashROM (SOP44)
                  This is a little strange, the ROM appears to be a standard SOP44 with reverse pinout but the
                  address lines are shifted one pin out of position compared to industry-standard pinouts.
                  The actual part number doesn't exist on the Macronix web site, even though they have datasheets for
                  everything else! So it's probably a custom design for Sammy and top-secret!
                  The size is assumed to be 1MBit and is 8-bit (D0-D7 only). The pinout appears to be this.....

                               +--\/--+
tied to WE of BS62LV1023   VCC | 1  44| VCC - tied to a transistor, probably RESET
                           NC  | 2  43| NC
                           A9  | 3  42| NC
                           A10 | 4  41| A8
                           A11 | 5  40| A7
                           A12 | 6  39| A6
                           A13 | 7  38| A5
                           A14 | 8  37| A4
                           A15 | 9  36| A3
                           A16 |10  35| A2
                           NC  |11  34| A1
                           NC  |12  33| CE - tied to 315-6267
                           GND |13  32| GND
                           A0  |14  31| OE
                           D7  |15  30| D0
                           NC  |16  29| NC
                           D6  |17  28| D1
                           NC  |18  27| NC
                           D5  |19  26| D2
                           NC  |20  25| NC
                           D4  |21  24| D3
                           VCC |22  23| NC
                               +------+

W129AG          - IC Works W129AG Programmable Clock Frequency Generator, clock input of 13.5MHz (SOIC16)
SW1             - 2-position Dip Switch
VGA             - 15 pin VGA out connector @ 31.5kHz
SER             - 9 pin Serial connector  \
VOL             - Volume pot              / These are on a small daughterboard that plugs into the main PCB via a multi-wire cable.
CN3             - Unknown 10 pin connector labelled 'CN3'
3V_BATT         - Panasonic ML2020 3 Volt Coin Battery

The bottom of the PCB contains nothing significant except some connectors. One for the game cart, one for special controls
or I/O, one for a communication module, one for a cooling fan and one for the serial connection daughterboard.

---------------------------------------------------------------------------------------------------------------------------------------------------



Most of the information below is taken from other sites and has no yet been verified to be correct


Known Games on this Hardware

System
    Title                                                    Notes
---------------------------------------------------------------------------------------------------------------------------------------------------
Naomi                                                      |
                                                           |
    18 Wheeler                                             |
    Airline Pilot                                          |
    Cannon Spike                                           |
    Capcom vs SNK : Millenium Fight 2000                   |
    Charge N' Blast                                        |
    Cosmic Smash                                           |
    Crackin' DJ                                            |
    Crackin' DJ Part 2                                     |
    Crazy Taxi                                             |
    Dead or Alive 2                                        |
    Dead or Alive 2 Version 2000                           |
    Death Crimson OX                                       |
    Derby Owners Club                                      |
    Dynamite Baseball                                      |
    Dynamite Baseball '99                                  |
    F1 World Grand Prix                                    |
    F335 Challenge                                         |
    F355 Challenge Twin                                    |
    F355 2 - International Course Edition                  |
    F355 2 - International Course Edition Twin             |
    Fish Live                                              |
    Formation Battle In May                                |
    Giant Gram 2                                           |
    Giant Gram 2000                                        |
    Gigawing 2                                             |
    Guilty Gear X                                          |
    Gun Beat                                               |
    Gun Spike                                              |
    Gun Survivor 2 Biohazard - Code Veronica               |
    Heavy Metal : Geomatrix                                |
    Idol Janshi Suchie-Pai 3                               |
    Jambo! Safari                                          |
    Marvel vs Capcom 2                                     |
    Mazan : Flash Of The Blade                             |
    Mobile Suit Gundam : Federation VS Zeon                |  GDL-0001 253-5509-5069
    Ninja Assault                                          |
    OutTrigger                                             |
    Power Stone                                            |
    Power Stone 2                                          |
    Project Justice: Rival Schools 2/Moero! Justice Gakuen |
    Puyo Puyo Da!                                          |
    Puyo Puyo Fever                                        |
    Quiz Ah My Goddess                                     |
    Ring Out 4x4                                           |
    Samba de Amigo                                         |
    Sambo de Amigo ver.2000                                |
    Sega Marine Fishing                                    |
    Sega Srike Fighter                                     |
    Sega Tetris                                            |
    Shangri-La                                             |
    Shootout Pool                                          |
    Slashout                                               |
    Spawn                                                  |
    The House of the Dead II                               |
    The Typing of the Dead                                 |
    Tokyo Bus Tour                                         |
    Touch de Uno!                                          |
    Touch de Uno! 2                                        |
    Toukon Retsuden 4/New Japan Pro Wrestling              |
    Toy Fighters/Waffupu                                   |
    Virtua NBA                                             |
    Virtua Striker 2 ver.2000                              |
    Virtua Tennis / Power Smash                            |
    Virtua Tennis 2 / Power Smash 2                        |
    Virtual-On Oratorio Tangram MSBS Version 5.66          |
    Wave Runner GP                                         |
    World Kicks                                            |
    World Series '99                                       |
    WWF Royal Rumble                                       |
    Zero Gunner 2                                          |
    Zombie Revenge                                         |
                                                           |
---------------------------------------------------------------------------------------------------------------------------------------------------
Naomi GD-Rom                                               |
                                                           |
    Alienfront Online                                      |
    Azumanga Daioh Puzzle Bobble                           |
    Border Down                                            |
    Capcom vs SNK : Millenium Fight 2000 Pro               |
    Capcom vs SNK 2 : Millionare Fighting 2001             |
    Chaos Breaker                                          |
    Chaos Field                                            |
    Cleopatra Fortune Plus                                 |
    Confidential Mission                                   |
    Dimm Firm Update                                       |
    Dog Walking / InuNoOsanpo                              |
    Dragon Treasure                                        |
    Dragon Treasure II                                     |
    Dragon Treasure III                                    |
    Get Bass 2                                             |
    Guilty Gear XX : The Midnight Carnival                 |
    Guilty Gear XX #Reload                                 |
    Guilty Gear XX Slash                                   |
    Guilty Gear XX Accent Core                             |
    Ikaruga                                                |
    Jingi Storm                                            |
    Karous                                                 |
    Kuru Kuru Chameleon                                    |
    La keyboard                                            |
    Lupin 3 : The Shooting                                 |
    Lupin : The Typing                                     |
    Melty Blood - Act Cadenza                              |
    Mobile Suit Gundam: Federation VS Zeon                 |
    Mobile Suit Gundam: Federation VS Zeon DX              |
    Moeru Kajinyo                                          |
    Monkey Ball                                            |
    Musapey's Choco Marker                                 |
    Pochinya                                               |
    Psyvariar 2 - The Will To Fabricate                    |
    Quiz k tie Q mode                                      |
    Radilgy                                                |
    Senko No Ronde                                         |
    Shikigami No Shiro II / The Castle of Shikigami II     |
    Shakka to Tambourine                                   |
    Shakka to Tambourine 2001                              |
    Shakka to Tambourine 2001 Power Up!                    |
    Slashout                                               |
    Spikers Battle                                         |
    Sports Jam                                             |
    Street Fighter Zero 3 Upper                            |  GDL-0002 253-5509-5072
    Super Major League / World Series Baseball             |
    Super Shanghai 2005                                    |
    Tetris Kiwamemichi                                     |
    The Maze Of Kings                                      |
    Trigger Heart Exelica                                  |
    TriZeal                                                |
    Under Defeat                                           |
    Usagi Yasei no Topai - Yamashiro Mahjongg Compilation  |
    Virtua Athletics                                       |
    Virtua Golf / Dynamic Golf                             |
    Virtua Tennis / Power Smash                            |
    Virtua Tennis 2 / Power Smash 2                        |
    Wild Riders                                            |
    Zooo                                                   |
                                                           |
---------------------------------------------------------------------------------------------------------------------------------------------------
Naomi 2                                                    |
                                                           |
    Club Kart                                              |
    Club Kart : European Session                           |
    Club Kart Prize                                        |
    King Of Route 66                                       |
    Quest Of D                                             |
    Sega Driving Simulator                                 |
    Soul Surfer                                            |
    Virtua Fighter 4                                       |
    Virtua Fighter 4 Evolution                             |
    Virtua Striker III                                     |
    Wild Riders                                            |
                                                           |
---------------------------------------------------------------------------------------------------------------------------------------------------
Naomi 2 GD-ROM                                             |
                                                           |
    Beach Spikers                                          |
    Club Kart Cycraft edition                              |
    Initial D : Arcade Stage                               |
    Initial D : Arcade Stage 2                             | GDS-0026 253-5508-0345J (Japan),  GDS-0027 253-5508-0357 (Export)
    Initial D : Arcade Stage 3                             |
    Initial D3 Cycraft Edition                             |
    Virtua Fighter 4                                       |
    Virtua Fighter 4 Evolution                             |
    Virtua Fighter 4 Final Tuned                           |
    Virtua Striker III                                     |
    Wild Riders                                            |
                                                           |
---------------------------------------------------------------------------------------------------------------------------------------------------
Sammy Atomiswave                                           |
                                                           |
    Chase 1929                                             | prototype
    Demolish Fist                                          |
    Dirty Pigskin Football                                 |
    Dolphin Blue                                           |
    Extreme Hunting                                        |
    Faster Than Speed                                      |
    Force Five                                             |
    Guilty Gear Isuka                                      |
    Guilty Gear X Ver. 1.5                                 |
    Hokuto No Ken / Fist Of The Northstar                  |
    Horse Racing                                           |
    Kenju                                                  |
    Knights Of Valour : The Seven Spirits                  |
    Maximum Speed                                          |
    Metal Slug 6                                           |
    Neogeo Battle Coliseum                                 |
    Premier Eleven                                         |
    Ranger Mission                                         |
    Rumble Fish                                            |
    Rumble Fish 2                                          |
    Salaried Worker Golden Taro                            |
    Sammy Vs. Capcom                                       | development cancelled pre-prototype
    Samurai Spirits Tenkaichi Kenkakuden                   |
    Sports Shooting USA                                    |
    Sushi Bar                                              |
    The King Of Fighters Neowave                           |
    The King Of Fighters XI                                |
                                                           |
---------------------------------------------------------------------------------------------------------------------------------------------------



---------------------------------------------------------
Bios Version Information                                |
---------------------------------------------------------
    Bios                     |   Support | Support      |
    Label                    |   GD-ROM  | Cabinet Link |
---------------------------------------------------------
Naomi / GD-ROM               |           |              |
    EPR-21576D (and earlier) |   No      |    No        |
    EPR-21576E               |   Yes     |    No        |
    EPR-21576F               |   Yes     |    Yes       |
    EPR-21576G (and newer)   |   Yes     |    Yes       |
---------------------------------------------------------
Naomi 2 / GD-ROM             |           |              |
    EPR-23605                |   Yes     |    No        |
    EPR-23605A               |   Yes     |    Yes       |
    EPR-23605B (and newer)   |   Yes     |    Yes       |
---------------------------------------------------------



Sega NAOMI ROM cart usage
-------------------------

837-13668  171-7919A (C) Sega 1998
|----------------------------------------------------------|
|IC1   IC2   IC3   IC4   IC5   IC6         CN2             |
|ROM1  ROM2  ROM3  ROM4  ROM5  ROM6                     JP1|
|                                              IC42        |
|                                       71256              |
|                                       71256         IC22 |
|            IC7  IC8  IC9  IC10  IC11                     |
|            ROM7 ROM8 ROM9 ROM10 ROM11        IC41        |
|                                                          |
|                                              28MHz       |
|                                                          |
|    CN3              X76F100               CN1            |
|----------------------------------------------------------|
Notes:
      Not all MASKROM positions are populated.
      All MASKROMs (IC1 to IC21) are SOP44, either 32M or 64M
      The other side of the cart PCB just has more locations for
      SOP44 MASKROMs... IC12 to IC21 (ROM12 to ROM21)

      IC22    - DIP42 EPROM, either 27C160 or 27C322
      JP1     - Sets the size of the EPROM. 1-2 = 32M, 2-3 = 16M
      IC41    - Xilinx XC9536 (PLCC44)
      IC42    - SEGA 315-5881 (QFP100). Probably some kind of FPGA or CPLD. Usually different per game.
                On the end of the number, -JPN means it requires Japanese BIOS, -COM will run with any BIOS
      X76F100 - Xicor X76F100 secured EEPROM (SOIC8)
      71256   - IDT 71256 32kx8 SRAM (SOJ28)
      CN1/2/3 - connectors joining to main board

      Note! Generally, games that require a special I/O board or controller will not boot at all with a
            standard NAOMI I/O board. Usually they display a message saying the I/O board is not acceptable
            or not connected properly.


Games known to use this PCB include....

                           Sticker    EPROM        # of SOP44
Game                       on cart    IC22#        MASKROMs   IC41#      IC42#          Notes
--------------------------------------------------------------------------------------------------------------
Cosmic Smash               840-0044C  23428A       8          315-6213   317-0289-COM
*Dead Or Alive 2           841-0003C  22121        21         315-6213   317-5048-COM
*Dead Or Alive 2 Millenium 841-0003C  DOA2 Ver.M   21         315-6213   317-5048-COM
*Derby Owners Club         840-0016C  22099B       14         315-6213   317-0262-JPN
*Dynamite Baseball '99     840-0019C  22141B       19         315-6213   317-0269-JPN
*Dynamite Baseball Naomi   840-0001C  21575        21         315-6213   317-0246-JPN
*Giant Gram Pro Wrestle 2  840-0007C  21820        9          315-6213   317-0253-JPN
*Heavy Metal Geo Matrix    HMG016007  23716A       11         315-6213   317-5071-COM
Idol Janshi Suchie-Pai 3   841-0002C  21979        14         315-6213   317-5047-JPN   requires special I/O board and mahjong panel
*Out Trigger               840-0017C  22163        19         315-6213   317-0266-COM   requires analog controllers/special panel
*Power Stone               841-0001C  21597        8          315-6213   317-5046-COM
*Power Stone 2             841-0008C  23127        9          315-6213   317-5054-COM
*Samba de Amigo            840-0020C  22966B       16         315-6213   317-0270-COM   will boot but requires special controller to play it
Sega Marine Fishing        840-0027C  22221        10         315-6213   not populated  ROM 3&4 not populated. Requires special I/O board and fishing controller
*Slash Out                 840-0041C  23341        17         315-6213   317-0286-COM
*Spawn                     841-0005C  22977B       10         315-6213   317-5051-COM
Toy Fighter                840-0011C  22035        10         315-6212   317-0257-COM
Virtua Striker 2 2000      840-0010C  21929C       15         315-6213   317-0258-COM
*Zombie Revenge            840-0003C  21707        19         315-6213   317-0249-COM

* denotes not dumped yet












Atomiswave cart PCB layout and game usage
-----------------------------------------

Type 1 ROM Board:


AM3AGB-04
MROM PCB
2002
|----------------------------|
| XC9536                     |
|         IC18 IC17*   IC10  |
|                            |
|                            |
|              IC16*   IC11  |
|                            |
|                            |
||-|           IC15*   IC12  |
|| |                         |
|| |                         |
|| |CN1        IC14*   IC13  |
|| |                         |
||-|                         |
|----------------------------|
Notes:
           * - Denotes those devices are on the other side of the PCB
      CN1    - This connector plugs into the main board.
      XC9536 - Xilinx XC9536 in-system programmable CPLD (PLCC44), stamped with a
               game code. This code is different for each different game.
               The last 3 digits seems to be for the usage.
               F01 = CPLD/protection device and M01 = MASKROM

               Game (sorted by code)            Code
               ------------------------------------------
               Dolphin Blue                     AX0401F01
               Demolish Fist                    AX0601F01
               Guilt Gear Isuka                 AX1201F01
               Knights Of Valour Seven Spirits  AX1301F01
               Salaryman Kintaro                AX1401F01
               Fist Of The North Star           AX1901F01
               King Of Fighters NEOWAVE         AX2201F01


        IC18 - Fujitsu 29DL640E 64M TSOP48 FlashROM. This ROM has no additional custom markings
               The name in the archive has been devised purely for convenience.
               This ROM holds the main program.

IC10 to IC17 - Custom-badged 128M TSOP48 maskROMs. I suspect they are Macronix
               ROMs because the ROM on the main board is also a custom Macronix
               ROM and they have a history of producing custom ROMs for other
               companies that hide their ROM types like Nintendo etc.
               The ROMs match a pinout that is identical to....
               Macronix MX26F128J3 (TSOP48)
               Oki MR27V12800 (TSOP48)
               More importantly the size is standard TSOP48 20mm long.
               They have been read as Oki MR27V12800
               The pinout also matches the same ROMs found on Namco Mr Driller 2
               and some Namco and Capcom NAOMI carts where these ROMs are used,
               although in all cases those ROMs are 18mm long, not 20mm.

               IC10 - Not Populated for 7 ROMs or less (ROM 01 if 8 ROMs are populated)
               IC11 - ROM 01 (or ROM 02 if 8 ROMs are populated)
               IC12 - ROM 02 (or ROM 03 if 8 ROMs are populated)
               IC13 - ROM 03 (or ROM 04 if 8 ROMs are populated)
               IC14 - ROM 04 (or ROM 05 if 8 ROMs are populated)
               IC15 - ROM 05 (or ROM 06 if 8 ROMs are populated)
               IC16 - ROM 06 (or ROM 07 if 8 ROMs are populated)
               IC17 - ROM 07 (or ROM 08 if 8 ROMs are populated)

               ROM Codes
               ---------
                                                                          Number
               Game (sorted by code)            Code                      of ROMs
               ------------------------------------------------------------------
               Dolphin Blue                     AX0401M01 to AX0405M01    5
               Demolish Fist                    AX0601M01 to AX0607M01    7
               Guilty Gear Isuka                AX1201M01 to AX1208M01    8
               Knights Of Valour Seven Spirits  AX1301M01 to AX1307M01    7
               Salaryman Kintaro                AX1401M01 to AX1407M01    7
               Fist Of The North Star           AX1901M01 to AX1907M01    7
               King Of Fighters NEOWAVE         AX2201M01 to AX2206M01    6



Type 2 ROM Board:


AM3ALW-02
MROM2 PCB
2005
|----------------------------|
|     FMEM1                  |
|     FMEM2*   MROM12        |
|              MROM11*       |
|                      MROM9 |
|              MROM10  MROM8*|
| XCR3128XL*   MROM7*        |
|                            |
||-|           MROM6         |
|| |           MROM3*  MROM4 |
|| |                   MROM5*|
|| |CN1        MROM2         |
|| |           MROM1*        |
||-|                         |
|----------------------------|
Notes:
           * - Denotes those devices are on the other side of the PCB
         CN1 - This connector plugs into the main board.
   XCR3128XL - Xilinx XCR3128XL in-system programmable 128 Macro-cell CPLD (TQFP100)
               stamped with a game code. This code is different for each different game.
               The last 3 digits seems to be for the usage.
               F01 = CPLD/protection device and M01 = MASKROM

               Game (sorted by code)            Code
               ------------------------------------------
               Neogeo Battle Coliseum           AX3301F01


 FMEM1/FMEM2 - Fujitsu 29DL640E 64M TSOP48 FlashROM. This ROM has no additional custom markings
               The name in the archive has been devised purely for convenience.
               This ROM holds the main program.
               This location is wired to accept TSOP56 ROMs, however the actual chip populated
               is a TSOP48, using the middle pins. The other 2 pins on each side of the ROM
               are not connected to anything.

       MROM* - Custom-badged SSOP70 maskROMs. These may be OKI MR26V25605 or MR26V25655 (256M)
               or possibly 26V51253 (512M) or something else similar.

               ROM Codes
               ---------
                                                                          Number
               Game (sorted by code)            Code                      of ROMs
               ------------------------------------------------------------------
               Neogeo Battle Coliseum           AX3301M01 to AX3307M01    7


*/

#include "driver.h"
#include "cpu/arm7/arm7.h"
#include "video/generic.h"
#include "machine/eeprom.h"
#include "naomibd.h"
#include "cpu/sh4/sh4.h"
#include "cpu/arm7/arm7core.h"
#include "sound/aica.h"
#include "dc.h"

#define CPU_CLOCK (200000000)
static UINT32 *dc_sound_ram;
static UINT64 *naomi_ram64;
                                             /* MD2 MD1 MD0 MD6 MD4 MD3 MD5 MD7 MD8 */
static const struct sh4_config sh4cpu_config = {  1,  0,  1,  0,  0,  0,  1,  1,  0, CPU_CLOCK };

static INTERRUPT_GEN( naomi_vblank )
{
	dc_vblank(device->machine);
}

static READ64_HANDLER( naomi_arm_r )
{
	return *((UINT64 *)dc_sound_ram+offset);
}

static WRITE64_HANDLER( naomi_arm_w )
{
	COMBINE_DATA((UINT64 *)dc_sound_ram + offset);
}

static READ64_HANDLER( naomi_unknown1_r )
{
	if ((offset * 8) == 0xc0) // trick so that it does not "wait for multiboard sync"
		return -1;
	return 0;
}

static WRITE64_HANDLER( naomi_unknown1_w )
{
}

/*
* Non-volatile memories
*/

extern UINT8 maple0x86data1[0x80];


// some default eeprom settings
UINT8 jvseeprom_default_karous[128] =   { 0x58, 0xE5, 0x11, 0x42, 0x4D, 0x57, 0x00, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x58, 0xE5, 0x11, 0x42, 0x4D, 0x57, 0x00, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_sfz3ugd[128] =  { 0xE3, 0xF2, 0x10, 0x42, 0x43, 0x59, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xE3, 0xF2, 0x10, 0x42, 0x43, 0x59, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xBE, 0x1F, 0x18, 0x18, 0xBE, 0x1F, 0x18, 0x18, 0x30, 0x59, 0x43, 0x42, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x30, 0x59, 0x43, 0x42, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_shikgam2[128] = { 0xA4, 0xF6, 0x11, 0x42, 0x47, 0x45, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xA4, 0xF6, 0x11, 0x42, 0x47, 0x45,	0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,0x43, 0x84, 0x20, 0x20, 0x43, 0x84, 0x20, 0x20, 0x42, 0x47, 0x45, 0x30,	0x53, 0x48, 0x49, 0x4B, 0x49, 0x32, 0x52, 0x4F, 0x4D, 0x20, 0x30, 0x32,	0x32, 0x38, 0x00, 0x00, 0x4B, 0x4B, 0x00, 0x02, 0x00, 0x02, 0x01, 0x00,	0x00, 0x00, 0x00, 0x00, 0x42, 0x47, 0x45, 0x30, 0x53, 0x48, 0x49, 0x4B,	0x49, 0x32, 0x52, 0x4F, 0x4D, 0x20, 0x30, 0x32, 0x32, 0x38, 0x00, 0x00,	0x4B, 0x4B, 0x00, 0x02, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_quizqgd[128] =  { 0xAD, 0x1E, 0x11, 0x42, 0x46, 0x50, 0x31, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xAD, 0x1E, 0x11, 0x42, 0x46, 0x50,	0x31, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0xC4, 0x18, 0x1D, 0x20, 0xC4, 0x18, 0x1D, 0x20, 0x42, 0x46, 0x50, 0x31,	0x51, 0x2D, 0x4D, 0x4F, 0x44, 0x45, 0x51, 0x2D, 0x4D, 0x4F, 0x44, 0x45,	0x01, 0x01, 0x05, 0x01, 0x02, 0x00, 0x01, 0x03, 0x01, 0x00, 0x4B, 0x4B,	0x00, 0x00, 0x00, 0x00, 0x42, 0x46, 0x50, 0x31, 0x51, 0x2D, 0x4D, 0x4F,	0x44, 0x45, 0x51, 0x2D, 0x4D, 0x4F, 0x44, 0x45, 0x01, 0x01, 0x05, 0x01,	0x02, 0x00, 0x01, 0x03, 0x01, 0x00, 0x4B, 0x4B, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_senko[128] =    { 0xCD, 0xEE, 0x10, 0x42, 0x4B, 0x47, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xCD, 0xEE, 0x10, 0x42, 0x4B, 0x47,	0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0xE5, 0x91, 0x10, 0x10, 0xE5, 0x91, 0x10, 0x10, 0x07, 0x51, 0x17, 0x03,	0x00, 0x01, 0x02, 0x02, 0x46, 0x00, 0x96, 0x00, 0x46, 0x00, 0x00, 0x00,	0x07, 0x51, 0x17, 0x03, 0x00, 0x01, 0x02, 0x02, 0x46, 0x00, 0x96, 0x00,	0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_senkoo[128] =   { 0xCD, 0xEE, 0x10, 0x42, 0x4B, 0x47, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xCD, 0xEE, 0x10, 0x42, 0x4B, 0x47,	0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x12, 0xF0, 0x0C, 0x0C, 0x12, 0xF0, 0x0C, 0x0C, 0x07, 0x51, 0x17, 0x03,	0x01, 0x02, 0x46, 0x00, 0x96, 0x00, 0x00, 0x00, 0x07, 0x51, 0x17, 0x03,	0x01, 0x02, 0x46, 0x00, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_radirgy[128] =  { 0x0C, 0x29, 0x11, 0x42, 0x4B, 0x57, 0x00, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x0C, 0x29, 0x11, 0x42, 0x4B, 0x57,	0x00, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_trgheart[128] = { 0xF0, 0xC2, 0x11, 0x42, 0x4D, 0x41, 0x31, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xF0, 0xC2, 0x11, 0x42, 0x4D, 0x41,	0x31, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x7E, 0xE1, 0x24, 0x24, 0x7E, 0xE1, 0x24, 0x24, 0x30, 0x41, 0x4D, 0x42,	0x54, 0x52, 0x49, 0x47, 0x47, 0x45, 0x52, 0x48, 0x45, 0x41, 0x52, 0x54,	0x20, 0x45, 0x58, 0x45, 0x4C, 0x49, 0x43, 0x41, 0x00, 0x00, 0x00, 0x00,	0x01, 0x03, 0x02, 0x00, 0x09, 0x09, 0x09, 0x00, 0x30, 0x41, 0x4D, 0x42,	0x54, 0x52, 0x49, 0x47, 0x47, 0x45, 0x52, 0x48, 0x45, 0x41, 0x52, 0x54,	0x20, 0x45, 0x58, 0x45, 0x4C, 0x49, 0x43, 0x41, 0x00, 0x00, 0x00, 0x00,	0x01, 0x03, 0x02, 0x00, 0x09, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_gundmgd[128] =  { 0xCD, 0x77, 0x10, 0x42, 0x43, 0x56, 0x30, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xCD, 0x77, 0x10, 0x42, 0x43, 0x56,	0x30, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_gundmxgd[128] = { 0xAA, 0xAD, 0x10, 0x42, 0x44, 0x55, 0x30, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xAA, 0xAD, 0x10, 0x42, 0x44, 0x55,	0x30, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_psyvar2[128] =  { 0xCB, 0x6E, 0x11, 0x42, 0x48, 0x42, 0x30, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xCB, 0x6E, 0x11, 0x42, 0x48, 0x42,	0x30, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_cfield[128] =   { 0xFB, 0xD4, 0x10, 0x42, 0x4A, 0x42, 0x00, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xFB, 0xD4, 0x10, 0x42, 0x4A, 0x42,	0x00, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_puyofev[128] =  { 0x65, 0x7E, 0x10, 0x42, 0x48, 0x44, 0x00, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x65, 0x7E, 0x10, 0x42, 0x48, 0x44,	0x00, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x0E, 0x0B, 0x20, 0x20, 0x0E, 0x0B, 0x20, 0x20, 0x42, 0x48, 0x44, 0x00,	0x50, 0x55, 0x59, 0x4F, 0x20, 0x50, 0x55, 0x59, 0x4F, 0x20, 0x46, 0x45,	0x56, 0x45, 0x52, 0x2E, 0x20, 0x10, 0x03, 0x20, 0x01, 0x00, 0x02, 0x03,	0x00, 0x80, 0x00, 0x00, 0x42, 0x48, 0x44, 0x00, 0x50, 0x55, 0x59, 0x4F,	0x20, 0x50, 0x55, 0x59, 0x4F, 0x20, 0x46, 0x45, 0x56, 0x45, 0x52, 0x2E,	0x20, 0x10, 0x03, 0x20, 0x01, 0x00, 0x02, 0x03, 0x00, 0x80, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_bdrdown[128] =  { 0xA0, 0x75, 0x10, 0x42, 0x47, 0x48, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xA0, 0x75, 0x10, 0x42, 0x47, 0x48,	0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0xAE, 0x7E, 0x08, 0x08, 0xAE, 0x7E, 0x08, 0x08, 0x01, 0x00, 0x00, 0x00,	0x33, 0xB8, 0x96, 0xE4, 0x01, 0x00, 0x00, 0x00, 0x33, 0xB8, 0x96, 0xE4,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_ss2005[128] =   { 0xAD, 0x0A, 0x10, 0x42, 0x4B, 0x4C, 0x30, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xAD, 0x0A, 0x10, 0x42, 0x4B, 0x4C,	0x30, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_usagui[128] =   { 0x02, 0x8C, 0x10, 0x42, 0x47, 0x46, 0x00, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x02, 0x8C, 0x10, 0x42, 0x47, 0x46,	0x00, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_undefeat[128] = { 0x06, 0x18, 0x11, 0x42, 0x4C, 0x48, 0x30, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x06, 0x18, 0x11, 0x42, 0x4C, 0x48,	0x30, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_trizeal[128] =  { 0x8F, 0x05, 0x11, 0x42, 0x4A, 0x46, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x8F, 0x05, 0x11, 0x42, 0x4A, 0x46,	0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_moeru[128] =    { 0xA1, 0x18, 0x10, 0x42, 0x45, 0x55, 0x20, 0x09, 0x00, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0xA1, 0x18, 0x10, 0x42, 0x45, 0x55,	0x20, 0x09, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_tetkiwam[128] = { 0x3C, 0x11, 0x10, 0x42, 0x47, 0x43, 0x31, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x3C, 0x11, 0x10, 0x42, 0x47, 0x43,	0x31, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UINT8 jvseeprom_default_keyboard[128] = { 0x32, 0x7E, 0x10, 0x42, 0x45, 0x42, 0x30, 0x09, 0x10, 0x00, 0x01, 0x01,	0x01, 0x00, 0x11, 0x11, 0x11, 0x11, 0x32, 0x7E, 0x10, 0x42, 0x45, 0x42,	0x30, 0x09, 0x10, 0x00, 0x01, 0x01, 0x01, 0x00, 0x11, 0x11, 0x11, 0x11,	0xF0, 0x4A, 0x0C, 0x0C, 0xF0, 0x4A, 0x0C, 0x0C, 0x18, 0x09, 0x01, 0x20,	0x02, 0x00, 0x3C, 0x00, 0x32, 0x00, 0x00, 0x00, 0x18, 0x09, 0x01, 0x20,	0x02, 0x00, 0x3C, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


static NVRAM_HANDLER( naomi_eeproms )
{
	if (read_or_write)
	{
		/* JVS 'eeprom' */
		mame_fwrite(file,maple0x86data1,0x80);


		// mainboard eeprom?
		eeprom_save(file);

	}
	else
	{
		eeprom_init(machine, &eeprom_interface_93C46);
		if (file)
		{
			UINT8 tmp[0x80];

			/* JVS 'eeprom' */
			mame_fread(file,maple0x86data1,0x80);

			mame_fread(file,&tmp,0x80);
			eeprom_set_data((UINT8 *)tmp, 0x80);

			// mainboard eeprom?
            eeprom_load(file);

		}
        else
		{
		//  int a;

			UINT32 length, size;
			UINT8 *dat;
			UINT8* jvseeprom_default = NULL;

			dat = eeprom_get_data_pointer(&length, &size);
			memset(dat, 0, length * size);

			// mainboard eeprom?
			eeprom_set_data((UINT8 *)"\011\241                              0000000000000000", 48);  // 2*checksum 30*unknown 16*serial

			// some games require defaults to boot (vertical, 1 player etc.)
			if (!strcmp(machine->gamedrv->name,"karous"))  jvseeprom_default = jvseeprom_default_karous;
			if (!strcmp(machine->gamedrv->name,"sfz3ugd")) jvseeprom_default = jvseeprom_default_sfz3ugd;
			if (!strcmp(machine->gamedrv->name,"shikgam2")) jvseeprom_default = jvseeprom_default_shikgam2;
			if (!strcmp(machine->gamedrv->name,"quizqgd")) jvseeprom_default = jvseeprom_default_quizqgd;
			if (!strcmp(machine->gamedrv->name,"senko")) jvseeprom_default = jvseeprom_default_senko;
			if (!strcmp(machine->gamedrv->name,"senkoo")) jvseeprom_default = jvseeprom_default_senkoo;
			if (!strcmp(machine->gamedrv->name,"radirgy")) jvseeprom_default = jvseeprom_default_radirgy;
			if (!strcmp(machine->gamedrv->name,"trgheart")) jvseeprom_default = jvseeprom_default_trgheart;
			if (!strcmp(machine->gamedrv->name,"gundmgd")) jvseeprom_default = jvseeprom_default_gundmgd;
			if (!strcmp(machine->gamedrv->name,"gundmxgd")) jvseeprom_default = jvseeprom_default_gundmxgd;
			if (!strcmp(machine->gamedrv->name,"psyvar2")) jvseeprom_default = jvseeprom_default_psyvar2;
			if (!strcmp(machine->gamedrv->name,"cfield")) jvseeprom_default = jvseeprom_default_cfield;
			if (!strcmp(machine->gamedrv->name,"puyofev")) jvseeprom_default = jvseeprom_default_puyofev;
			if (!strcmp(machine->gamedrv->name,"bdrdown")) jvseeprom_default = jvseeprom_default_bdrdown;
			if (!strcmp(machine->gamedrv->name,"ss2005")) jvseeprom_default = jvseeprom_default_ss2005;
			if (!strcmp(machine->gamedrv->name,"usagui")) jvseeprom_default = jvseeprom_default_usagui;
			if (!strcmp(machine->gamedrv->name,"undefeat")) jvseeprom_default = jvseeprom_default_undefeat;
			if (!strcmp(machine->gamedrv->name,"trizeal")) jvseeprom_default = jvseeprom_default_trizeal;
			if (!strcmp(machine->gamedrv->name,"moeru")) jvseeprom_default = jvseeprom_default_moeru;
			if (!strcmp(machine->gamedrv->name,"tetkiwam")) jvseeprom_default = jvseeprom_default_tetkiwam;
			if (!strcmp(machine->gamedrv->name,"keyboard")) jvseeprom_default = jvseeprom_default_keyboard;

			if (jvseeprom_default)
				memcpy(maple0x86data1, jvseeprom_default, 0x80);

		#if 0
			/* JVS 'eeprom' */
			for (a=0;a < 0x80;a++)
				maple0x86data1[a]=0x11+a;

			// checksums
			maple0x86data1[0]=0xb9;
			maple0x86data1[1]=0xb1;
			maple0x86data1[18]=0xb8;
			maple0x86data1[19]=0x8a;
		#endif
		}
	}
}

static READ64_HANDLER( eeprom_93c46a_r )
{
	int res;

	/* bit 3 is EEPROM data */
	res = eeprom_read_bit() << 4;
	return res;
}

static WRITE64_HANDLER( eeprom_93c46a_w )
{
	/* bit 4 is data */
	/* bit 2 is clock */
	/* bit 5 is cs */
	eeprom_write_bit(data & 0x8);
	eeprom_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
	eeprom_set_clock_line((data & 0x4) ? ASSERT_LINE : CLEAR_LINE);
}

/* Dreamcast MAP

0 0x00000000 - 0x001FFFFF MPX System/Boot ROM
0 0x00200000 - 0x0021FFFF Flash Memory
0 0x00400000 - 0x005F67FF Unassigned
0 0x005F6800 - 0x005F69FF System Control Reg.
0 0x005F6C00 - 0x005F6CFF Maple i/f Control Reg.
0 0x005F7000 - 0x005F70FF GD-ROM
0 0x005F7400 - 0x005F74FF G1 i/f Control Reg.
0 0x005F7800 - 0x005F78FF G2 i/f Control Reg.
0 0x005F7C00 - 0x005F7CFF PVR i/f Control Reg.
0 0x005F8000 - 0x005F9FFF TA / PVR Core Reg.
0 0x00600000 - 0x006007FF MODEM
0 0x00600800 - 0x006FFFFF G2 (Reserved)
0 0x00700000 - 0x00707FFF AICA- Sound Cntr. Reg.
0 0x00710000 - 0x0071000B AICA- RTC Cntr. Reg.
0 0x00800000 - 0x00FFFFFF AICA- Wave Memory
0 0x01000000 - 0x01FFFFFF Ext. Device
0 0x02000000 - 0x03FFFFFF Image Area (Mirror Area)

1 0x04000000 - 0x04FFFFFF MPX Tex.Mem. 64bit Acc.
1 0x05000000 - 0x05FFFFFF Tex.Mem. 32bit Acc.
1 0x06000000 - 0x07FFFFFF Image Area*

2 0x08000000 - 0x0BFFFFFF Unassigned

3 0x0C000000 - 0x0CFFFFFF System Memory
3 0x0D000000 - 0x0DFFFFFF (Mirror on DC, Extra RAM on Naomi)

3 0x0E000000 - 0x0FFFFFFF Image Area (Mirror Area)

4 0x10000000 - 0x107FFFFF MPX TA FIFO Polygon Cnv.
4 0x10800000 - 0x10FFFFFF TA FIFO YUV Conv.
4 0x11000000 - 0x11FFFFFF Tex.Mem. 32/64bit Acc.
4 0x12000000 - 0x13FFFFFF Image Area (Mirror Area)

5 0x14000000 - 0x17FFFFFF MPX Ext.

6 0x18000000 - 0x1BFFFFFF Unassigned

7 0x1C000000 - 0x1FFFFFFF(SH4 Internal area)

*/

/*
 * Common address map for Naomi 1, Naomi GD-Rom, Naomi 2, Atomiswave ...
 */

static ADDRESS_MAP_START( naomi_base_map, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM                                             // BIOS
	AM_RANGE(0x00200000, 0x00207fff) AM_RAM                                             // bios uses it (battery backed ram ?)
	AM_RANGE(0x005f6800, 0x005f69ff) AM_READWRITE( dc_sysctrl_r, dc_sysctrl_w )
	AM_RANGE(0x005f6c00, 0x005f6cff) AM_READWRITE( dc_maple_r, dc_maple_w )
	AM_RANGE(0x005f7400, 0x005f74ff) AM_READWRITE( dc_g1_ctrl_r, dc_g1_ctrl_w )
	AM_RANGE(0x005f7800, 0x005f78ff) AM_READWRITE( dc_g2_ctrl_r, dc_g2_ctrl_w )
	AM_RANGE(0x005f7c00, 0x005f7cff) AM_READWRITE( pvr_ctrl_r, pvr_ctrl_w )
	AM_RANGE(0x005f8000, 0x005f9fff) AM_READWRITE( pvr_ta_r, pvr_ta_w )
	AM_RANGE(0x00600000, 0x006007ff) AM_READWRITE( dc_modem_r, dc_modem_w )
	AM_RANGE(0x00700000, 0x00707fff) AM_DEVREADWRITE( "aica", dc_aica_reg_r, dc_aica_reg_w )
	AM_RANGE(0x00710000, 0x0071000f) AM_READWRITE( dc_rtc_r, dc_rtc_w )
	AM_RANGE(0x00800000, 0x00ffffff) AM_READWRITE( naomi_arm_r, naomi_arm_w )           // sound RAM (8 MB)
	AM_RANGE(0x0103ff00, 0x0103ffff) AM_READWRITE( naomi_unknown1_r, naomi_unknown1_w ) // bios uses it, actual start and end addresses not known
	AM_RANGE(0x04000000, 0x04ffffff) AM_RAM	AM_SHARE(2) AM_BASE( &dc_texture_ram )      // texture memory 64 bit access
	AM_RANGE(0x05000000, 0x05ffffff) AM_RAM AM_SHARE(2)                                 // mirror of texture RAM 32 bit access
	AM_RANGE(0x0c000000, 0x0dffffff) AM_RAM AM_BASE(&naomi_ram64)
	AM_RANGE(0x10000000, 0x107fffff) AM_WRITE( ta_fifo_poly_w )
	AM_RANGE(0x10800000, 0x10ffffff) AM_WRITE( ta_fifo_yuv_w )
	AM_RANGE(0x11000000, 0x11ffffff) AM_RAM AM_SHARE(2)                                 // another mirror of texture memory
	AM_RANGE(0x13000000, 0x13ffffff) AM_RAM AM_SHARE(2)                                 // another mirror of texture memory
	AM_RANGE(0xa0000000, 0xa01fffff) AM_ROM AM_REGION("maincpu", 0)
ADDRESS_MAP_END

/*
 * Naomi 1 address map
 */

static ADDRESS_MAP_START( naomi_map, ADDRESS_SPACE_PROGRAM, 64 )
	/* Area 0 */
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM AM_SHARE(3) AM_REGION("maincpu", 0) // BIOS
	AM_RANGE(0xa0000000, 0xa01fffff) AM_ROM AM_SHARE(3)  // non cachable access to  0x00000000 - 0x001fffff

	AM_RANGE(0x00200000, 0x00207fff) AM_RAM                                             // bios uses it (battery backed ram ?)
	AM_RANGE(0x005f6800, 0x005f69ff) AM_READWRITE( dc_sysctrl_r, dc_sysctrl_w )
	AM_RANGE(0x005f6c00, 0x005f6cff) AM_READWRITE( dc_maple_r, dc_maple_w )
	AM_RANGE(0x005f7000, 0x005f70ff) AM_DEVREADWRITE("rom_board", naomibd_r, naomibd_w)
	AM_RANGE(0x005f7400, 0x005f74ff) AM_READWRITE( dc_g1_ctrl_r, dc_g1_ctrl_w )
	AM_RANGE(0x005f7800, 0x005f78ff) AM_READWRITE( dc_g2_ctrl_r, dc_g2_ctrl_w )
	AM_RANGE(0x005f7c00, 0x005f7cff) AM_READWRITE( pvr_ctrl_r, pvr_ctrl_w )
	AM_RANGE(0x005f8000, 0x005f9fff) AM_READWRITE( pvr_ta_r, pvr_ta_w )
	AM_RANGE(0x00600000, 0x006007ff) AM_READWRITE( dc_modem_r, dc_modem_w )
	AM_RANGE(0x00700000, 0x00707fff) AM_DEVREADWRITE( "aica", dc_aica_reg_r, dc_aica_reg_w )
	AM_RANGE(0x00710000, 0x0071000f) AM_READWRITE( dc_rtc_r, dc_rtc_w )
	AM_RANGE(0x00800000, 0x00ffffff) AM_READWRITE( naomi_arm_r, naomi_arm_w )           // sound RAM (8 MB)


	AM_RANGE(0x0103ff00, 0x0103ffff) AM_READWRITE( naomi_unknown1_r, naomi_unknown1_w ) // bios uses it, actual start and end addresses not known

	/* Area 1 */
	AM_RANGE(0x04000000, 0x04ffffff) AM_RAM	AM_SHARE(2) AM_BASE( &dc_texture_ram )      // texture memory 64 bit access
	AM_RANGE(0x05000000, 0x05ffffff) AM_RAM AM_SHARE(2)                                 // mirror of texture RAM 32 bit access

	/* Area 2*/
	AM_RANGE(0x08000000, 0x0bffffff) AM_NOP // 'Unassigned'

	/* Area 3 */
	AM_RANGE(0x0c000000, 0x0cffffff) AM_RAM AM_BASE(&naomi_ram64) AM_SHARE(4)
	AM_RANGE(0x0d000000, 0x0dffffff) AM_RAM AM_SHARE(5)// extra ram on Naomi (mirror on DC)
	AM_RANGE(0x0e000000, 0x0effffff) AM_RAM AM_SHARE(4)// mirror
	AM_RANGE(0x0f000000, 0x0fffffff) AM_RAM AM_SHARE(5)// mirror

	AM_RANGE(0x8c000000, 0x8cffffff) AM_RAM AM_SHARE(4) // RAM access through cache


	/* Area 4 */
	AM_RANGE(0x10000000, 0x107fffff) AM_WRITE( ta_fifo_poly_w )
	AM_RANGE(0x10800000, 0x10ffffff) AM_WRITE( ta_fifo_yuv_w )
	AM_RANGE(0x11000000, 0x11ffffff) AM_RAM AM_SHARE(2)
	/*       0x12000000 -0x13ffffff Mirror area of  0x10000000 -0x11ffffff */
	AM_RANGE(0x12000000, 0x127fffff) AM_WRITE( ta_fifo_poly_w )
	AM_RANGE(0x12800000, 0x12ffffff) AM_WRITE( ta_fifo_yuv_w )
	AM_RANGE(0x13000000, 0x13ffffff) AM_RAM AM_SHARE(2)

	/* Area 5 */
	//AM_RANGE(0x14000000, 0x17ffffff) AM_NOP // MPX Ext.

	/* Area 6 */
	//AM_RANGE(0x18000000, 0x1bffffff) AM_NOP // Unassigned

	/* Area 7 */
	//AM_RANGE(0x1c000000, 0x1fffffff) AM_NOP // SH4 Internal
ADDRESS_MAP_END


static ADDRESS_MAP_START( naomi_port, ADDRESS_SPACE_IO, 64 )
	AM_RANGE(0x00, 0x0f) AM_READWRITE(eeprom_93c46a_r, eeprom_93c46a_w)
ADDRESS_MAP_END


/*
 * Aica
 */
static void aica_irq(const device_config *device, int irq)
{
	cpu_set_input_line(device->machine->cpu[1], ARM7_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}


static const aica_interface aica_config =
{
	TRUE,
	0,
	aica_irq
};



static ADDRESS_MAP_START( dc_audio_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_BASE( &dc_sound_ram )                /* shared with SH-4 */
	AM_RANGE(0x00800000, 0x00807fff) AM_DEVREADWRITE("aica", dc_arm_aica_r, dc_arm_aica_w)
ADDRESS_MAP_END

/*
* Input ports
*/

/* for now we hardwire a joystick + 6 buttons for every game.*/
static INPUT_PORTS_START( naomi )
	PORT_START("IN0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Service")
	PORT_SERVICE_NO_TOGGLE( 0x01, IP_ACTIVE_LOW )
	PORT_START("IN1")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_START("IN2")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_START("IN3")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_START("IN4")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START("COINS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_CHANGED(dc_coin_slots_callback, &dc_coin_counts[0])
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_CHANGED(dc_coin_slots_callback, &dc_coin_counts[1])
INPUT_PORTS_END

static MACHINE_RESET( naomi )
{
	MACHINE_RESET_CALL(dc);
	aica_set_ram_base(devtag_get_device(machine, "aica"), dc_sound_ram, 8*1024*1024);
}

/*
 * Common for Naomi 1, Naomi GD-Rom, Naomi 2, Atomiswave ...
 */

static MACHINE_DRIVER_START( naomi_base )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", SH4, CPU_CLOCK) // SH4!!!
	MDRV_CPU_CONFIG(sh4cpu_config)
	MDRV_CPU_PROGRAM_MAP(naomi_map,0)
	MDRV_CPU_IO_MAP(naomi_port,0)
	MDRV_CPU_VBLANK_INT("screen", naomi_vblank)

	MDRV_CPU_ADD("soundcpu", ARM7, ((XTAL_33_8688MHz*2)/3)/8)	// AICA bus clock is 2/3rds * 33.8688.  ARM7 gets 1 bus cycle out of each 8.
	MDRV_CPU_PROGRAM_MAP(dc_audio_map, 0)

	MDRV_MACHINE_START( dc )
	MDRV_MACHINE_RESET( naomi )

	MDRV_NVRAM_HANDLER(naomi_eeproms)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(dc)
	MDRV_VIDEO_UPDATE(dc)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
	MDRV_SOUND_ADD("aica", AICA, 0)
	MDRV_SOUND_CONFIG(aica_config)
	MDRV_SOUND_ROUTE(0, "lspeaker", 2.0)
	MDRV_SOUND_ROUTE(1, "rspeaker", 2.0)
MACHINE_DRIVER_END

/*
 * Naomi 1
 */

static MACHINE_DRIVER_START( naomi )
	MDRV_IMPORT_FROM(naomi_base)
	MDRV_NAOMI_ROM_BOARD_ADD("rom_board", "user1")
MACHINE_DRIVER_END

/*
 * Naomi 1 GD-Rom
 */

static MACHINE_DRIVER_START( naomigd )
	MDRV_IMPORT_FROM(naomi_base)
	MDRV_NAOMI_DIMM_BOARD_ADD("rom_board", "gdrom", "user1", "picreturn")
MACHINE_DRIVER_END

#define ROM_LOAD16_WORD_SWAP_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_BIOS(bios+1)) /* Note '+1' */

/* BIOS info:

Revisions A through C can handle game carts only
Revisions D and later can also handle GD-Rom board
Revisions F and later can also handle GD-Rom board and or the network GD-Rom board

F355 has it's own BIOS (3 screen version) and different JVS I/O Board

Info from roms starting at 0x1ffd60

EPR-21576h - NAOMI BOOT ROM 2002 07/08  1.8- (Japan)
EPR-21576g - NAOMI BOOT ROM 2001 09/10  1.70 (Japan)
EPR-21576e - NAOMI BOOT ROM 2000 08/25  1.50 (Japan)
EPR-21576d - NAOMI BOOT ROM 1999 06/04  1.40 (Japan)
EPR-21576b - NAOMI BOOT ROM 1999 02/15  1.20 (Japan)
EPR-21576  - NAOMI BOOT ROM 1998 12/18  1.00 (Japan)
EPR-21577e - NAOMI BOOT ROM 2000 08/25  1.50 (USA)
EPR-21577d - NAOMI BOOT ROM 1999 06/04  1.40 (USA)
EPR-21578e - NAOMI BOOT ROM 2000 08/25  1.50 (Export)
EPR-21578d - NAOMI BOOT ROM 1999 06/04  1.40 (Export)
EPR-21578b - NAOMI BOOT ROM 1999 02/15  1.20 (Export)
EPR-21579  - No known dump (Korea)
EPR-21580  - No known dump (Australia)
EPR-22851  - NAOMI BOOT ROM 1999 08/30  1.35 (Multisystem 3 screen Ferrari F355)

EPR-21577e & EPR-2178e differ by 7 bytes:

0x53e20 is the region byte (only one region byte)
0x1ffffa-0x1fffff is the BIOS checksum


House of the Dead 2 specific Naomi BIOS roms:

Info from roms starting at 0x1ff060

EPR-21330  - HOUSE OF THE DEAD 2 IPL ROM 1998 11/14 (USA)
EPR-21331  - HOUSE OF THE DEAD 2 IPL ROM 1998 11/14 (Export)

EPR-21330 & EPR-21331 differ by 7 bytes:

0x40000 is the region byte (only one region byte)
0x1ffffa-0x1fffff is the BIOS checksum


Ferrari F355 specific Naomi BIOS roms:

EPR-22850 - NAOMI BOOT ROM 1999 08/30  1.35 (USA)
EPR-22851 - NAOMI BOOT ROM 1999 08/30  1.35 (Export)

EPR-22850 & EPR-22851 differ by 7 bytes:

0x52F08 is the region byte (only one region byte)
0x1ffffa-0x1fffff is the BIOS checksum


Region byte encoding is as follows:

0x00 = Japan
0x01 = USA
0x02 = Export
0x?? = Korea
0x?? = Australia

Scan ROM for the text string "LOADING TEST MODE NOW" back up four (4) bytes for the region byte.
  NOTE: this doesn't work for the HOTD2 or multi screen boot roms

*/
// game specific bios roms quite clearly don't belong in here.
// Japan bios is default, because most games require it.
/* bios e works, the newer japan one gives 'board malfunction' with some games?' */
#define NAOMI_BIOS \
	ROM_SYSTEM_BIOS( 0, "bios0", "epr-21576e (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "epr-21576e.bin",  0x000000, 0x200000, CRC(08c0add7) SHA1(e7c1a7673cb2ccb21748ef44105e46d1bad7266d) ) \
	ROM_SYSTEM_BIOS( 1, "bios1", "epr-21576g (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 1, "epr-21576g.bin",  0x000000, 0x200000, CRC(d2a1c6bf) SHA1(6d27d71aec4dfba98f66316ae74a1426d567698a) ) \
	ROM_SYSTEM_BIOS( 2, "bios2", "epr-21576h (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 2, "epr-21576h.bin",  0x000000, 0x200000, CRC(d4895685) SHA1(91424d481ff99a8d3f4c45cea6d3f0eada049a6d) ) \
	ROM_SYSTEM_BIOS( 3, "bios3", "epr-21576d (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 3, "epr-21576d.bin",  0x000000, 0x200000, CRC(3b2afa7b) SHA1(d007e1d321c198a38c5baff86eb2ab84385d150a) ) \
	ROM_SYSTEM_BIOS( 4, "bios4", "epr-21576b (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 4, "epr-21576b.bin",  0x000000, 0x200000, CRC(755a6e07) SHA1(7e8b8ccfc063144d89668e7224dcd8a36c54f3b3) ) \
	ROM_SYSTEM_BIOS( 5, "bios5", "epr-21576 (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 5,  "epr-21576.bin", 0x000000, 0x200000, CRC(9dad3495) SHA1(5fb66f9a2b68d120f059c72758e65d34f461044a) ) \
	ROM_SYSTEM_BIOS( 6, "bios6", "epr-21578e (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 6, "epr-21578e.bin",  0x000000, 0x200000, CRC(087f09a3) SHA1(0418eb2cf9766f0b1b874a4e92528779e22c0a4a) ) \
	ROM_SYSTEM_BIOS( 7, "bios7", "epr-21578d (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 7, "epr-21578d.bin",  0x000000, 0x200000, CRC(dfd5f42a) SHA1(614a0db4743a5e5a206190d6786ade24325afbfd) ) \
	ROM_SYSTEM_BIOS( 8, "bios8", "epr-21578b (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 8, "epr-21578b.bin",  0x000000, 0x200000, CRC(6c9aad83) SHA1(555918de76d8dbee2a97d8a95297ef694b3e803f) ) \
	ROM_SYSTEM_BIOS( 9, "bios9", "epr-21577e (USA)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 9, "epr-21577e.bin",  0x000000, 0x200000, CRC(cf36e97b) SHA1(b085305982e7572e58b03a9d35f17ae319c3bbc6) ) \
	ROM_SYSTEM_BIOS( 10, "bios10", "epr-21577d (USA)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 10, "epr-21577d.bin",  0x000000, 0x200000, CRC(60ddcbbe) SHA1(58b15096d269d6df617ca1810b66b47deb184958) ) \
	ROM_SYSTEM_BIOS( 11, "bios11", "Ferrari F355 (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 11,  "epr-22851.bin", 0x000000, 0x200000, CRC(62483677) SHA1(3e3bcacf5f972c376b569f45307ee7fd0b5031b7) ) \
	ROM_SYSTEM_BIOS( 12, "bios12", "Ferrari F355 (USA)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 12,  "epr-22850.bin", 0x000000, 0x200000, CRC(28aa539d) SHA1(14485368656af80504b212da620179c49f84c1a2) ) \
	ROM_SYSTEM_BIOS( 13, "bios13", "Naomi Dev BIOS" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 13,  "dcnaodev.bios", 0x000000, 0x080000, CRC(7a50fab9) SHA1(ef79f448e0bf735d1264ad4f051d24178822110f) ) /* This one comes from a dev / beta board. The eprom was a 27C4096 */

// bios for House of the Dead 2
#define HOTD2_BIOS \
	ROM_SYSTEM_BIOS( 0, "bios0", "HOTD2 (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0,  "epr-21331.bin", 0x000000, 0x200000, CRC(065f8500) SHA1(49a3881e8d76f952ef5e887200d77b4a415d47fe) ) \
	ROM_SYSTEM_BIOS( 1, "bios1", "HOTD2 (US)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 1,  "epr-21330.bin", 0x000000, 0x200000, CRC(9e3bfa1b) SHA1(b539d38c767b0551b8e7956c1ff795de8bbe2fbc) ) \


/* only revisions d and higher support the GDROM, and there is an additional bios (and SH4!) on the DIMM board for the CD Controller */
/* bios e works, the newer japan one gives 'board malfunction' with some games?' */
#define NAOMIGD_BIOS \
	ROM_REGION( 0x200000, "maincpu", 0) \
	ROM_SYSTEM_BIOS( 0, "bios0", "epr-21576e (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "epr-21576e.bin",  0x000000, 0x200000, CRC(08c0add7) SHA1(e7c1a7673cb2ccb21748ef44105e46d1bad7266d) ) \
	ROM_SYSTEM_BIOS( 1, "bios1", "epr-21576g (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 1, "epr-21576g.bin",  0x000000, 0x200000, CRC(d2a1c6bf) SHA1(6d27d71aec4dfba98f66316ae74a1426d567698a) ) \
	ROM_SYSTEM_BIOS( 2, "bios2", "epr-21576h (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 2, "epr-21576h.bin",  0x000000, 0x200000, CRC(d4895685) SHA1(91424d481ff99a8d3f4c45cea6d3f0eada049a6d) ) \
	ROM_SYSTEM_BIOS( 3, "bios3", "epr-21576d (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 3, "epr-21576d.bin",  0x000000, 0x200000, CRC(3b2afa7b) SHA1(d007e1d321c198a38c5baff86eb2ab84385d150a) ) \
	ROM_SYSTEM_BIOS( 4, "bios4", "epr-21578e (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 4, "epr-21578e.bin",  0x000000, 0x200000, CRC(087f09a3) SHA1(0418eb2cf9766f0b1b874a4e92528779e22c0a4a) ) \
	ROM_SYSTEM_BIOS( 5, "bios5", "epr-21578d (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 5, "epr-21578d.bin",  0x000000, 0x200000, CRC(dfd5f42a) SHA1(614a0db4743a5e5a206190d6786ade24325afbfd) ) \
	ROM_SYSTEM_BIOS( 6, "bios6", "epr-21577e (USA)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 6, "epr-21577e.bin",  0x000000, 0x200000, CRC(cf36e97b) SHA1(b085305982e7572e58b03a9d35f17ae319c3bbc6) ) \
	ROM_SYSTEM_BIOS( 7, "bios7", "epr-21577d (USA)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 7, "epr-21577d.bin",  0x000000, 0x200000, CRC(60ddcbbe) SHA1(58b15096d269d6df617ca1810b66b47deb184958) ) \
	ROM_REGION( 0x200000, "user2", 0) \
	ROM_LOAD16_WORD_SWAP( "fpr-23489c.ic14", 0x000000, 0x200000, CRC(bc38bea1) SHA1(b36fcc6902f397d9749e9d02de1bbb7a5e29d468) ) \

/* NAOMI2 BIOS:

EPR-23605  - NAOMI BOOT ROM 2001 01/19  1.50 (Japan)
EPR-23605A - NAOMI BOOT ROM 2001 06/20  1.60 (Japan)
EPR-23605B - NAOMI BOOT ROM 2001 09/10  1.70 (Japan)
EPR-23605C - NAOMI BOOT ROM 2002 07/08  1.8- (Japan)
EPR-23607  - NAOMI BOOT ROM 2001 01/19  1.50 (USA)
EPR-23607B - NAOMI BOOT ROM 2001 09/10  1.70 (USA)
EPR-23608  - NAOMI BOOT ROM 2001 01/19  1.50 (Export)
EPR-23608B - NAOMI BOOT ROM 2001 09/10  1.70 (Export)

EPR-23605B, EPR-23607B & EPR-23608B all differ by 8 bytes:

0x0553a0 is the first region byte
0x1ecf40 is a second region byte (value is the same as the first region byte )
0x1fffa-1ffff is the BIOS rom checksum

Region byte encoding is as follows:

0x00 = Japan
0x01 = USA
0x02 = Export
0x?? = Korea
0x?? = Australia

*/

#define NAOMI2_BIOS \
	ROM_REGION( 0x200000, "maincpu", 0) \
	ROM_SYSTEM_BIOS( 0, "bios0", "epr-23605c (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "epr-23605c.bin",   0x000000, 0x200000, CRC(297ea6ed) SHA1(cfbfe57c80e6ee86a101fa83aec0a01e00c0f42a) ) \
	ROM_SYSTEM_BIOS( 1, "bios1", "epr-23605b (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 1, "epr-23605b.bin",   0x000000, 0x200000, CRC(3a3242d4) SHA1(aaca4df51ef91d926f8191d372f3dfe1d20d9484) ) \
	ROM_SYSTEM_BIOS( 2, "bios2", "epr-23605a (Japan)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 2, "epr-23605a.bin",   0x000000, 0x200000, CRC(7bc3fc2d) SHA1(a4a9531a7c66ff30046908cf71f6c7b6fb59c392) ) \
	ROM_SYSTEM_BIOS( 3, "bios3", "epr-23605 (Japan)"  ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 3, "epr-23605.bin",    0x000000, 0x200000, CRC(5731e446) SHA1(787b0844fc408cf124c12405c095c59948709ea6) ) \
	ROM_SYSTEM_BIOS( 4, "bios4", "epr-23608b (Export)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 4, "epr-23608b.bin",   0x000000, 0x200000, CRC(a554b1e3) SHA1(343b727a3619d1c75a9b6d4cc156a9050447f155) ) \
	ROM_SYSTEM_BIOS( 5, "bios5", "epr-23608 (Export)"  ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 5, "epr-23608.bin",    0x000000, 0x200000, CRC(929cc3a6) SHA1(47d00c818de23f733a4a33b1bbc72eb8aa729246) ) \
	ROM_SYSTEM_BIOS( 6, "bios6", "epr-23607b (USA)" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 6, "epr-23607b.bin",   0x000000, 0x200000, CRC(f308c5e9) SHA1(5470ab1cee6afecbd8ca8cf40f8fbe4ec2cb1471) ) \
	ROM_SYSTEM_BIOS( 7, "bios7", "epr-23607 (USA)"  ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 7, "epr-23607.bin",    0x000000, 0x200000, CRC(2b55add2) SHA1(547de5f97d3183c8cd069c4fa3c09f13d8b637d9) ) \

/* this is one flashrom, however the second half looks like it's used for game settings, may differ between dumps, and may not be needed / could be blanked */
#define AW_BIOS \
	ROM_SYSTEM_BIOS( 0, "bios0", "Atomiswave BIOS" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "bios.ic23_l",                         0x000000, 0x010000, BAD_DUMP CRC(e5693ce3) SHA1(1bde3ed87af64b0f675ebd47f12a53e1fc5709c1) ) /* Might be bad.. especially. bytes 0x0000, 0x6000, 0x8000 which gave different reads */ \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "bios.ic23_h-dolphin_blue_settings",   0x010000, 0x010000, BAD_DUMP CRC(5d5687c7) SHA1(2600ce09c44872d1793f6b55bf44342673da5ad1) ) /* it appears to flash settings game data here */ /* this is one flashrom, however the second half looks like it's used for game settings, may differ between dumps, and may not be needed / could be blanked */


ROM_START( naomi )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x8400000, "user1", ROMREGION_ERASE)
ROM_END

ROM_START( naomigd )
	NAOMIGD_BIOS

	ROM_REGION( 0x8400000, "user1", ROMREGION_ERASE)
ROM_END

ROM_START( hod2bios )
	ROM_REGION( 0x200000, "maincpu", 0)
	HOTD2_BIOS

	ROM_REGION( 0x8400000, "user1", ROMREGION_ERASE)
ROM_END

ROM_START( naomi2 )
	NAOMI2_BIOS

	ROM_REGION( 0x8400000, "user1", ROMREGION_ERASE)
ROM_END

ROM_START( awbios )
	ROM_REGION( 0x200000, "maincpu", 0)
	AW_BIOS

	ROM_REGION( 0x8400000, "user1", ROMREGION_ERASE)
ROM_END


ROM_START( fotns )
	ROM_REGION( 0x200000, "maincpu", 0)
	AW_BIOS

	ROM_REGION( 0x8000000, "user1", ROMREGION_ERASE)
	ROM_LOAD("ax1901p01.ic18", 0x0000000, 0x0800000,  CRC(a06998b0) SHA1(d617691db5170f6db176e40fc732966d523fd8cf) )
	ROM_LOAD("ax1901m01.ic11", 0x1000000, 0x1000000,  CRC(ff5a1642) SHA1(49cefcce173f9a811fe9c0c07bee53aeba2bc3a8) )
	ROM_LOAD("ax1902m01.ic12", 0x2000000, 0x1000000,  CRC(d9aae8a9) SHA1(bf87034088be0847b6e297b7665e0ea4d8cba631) )
	ROM_LOAD("ax1903m01.ic13", 0x3000000, 0x1000000,  CRC(1711b23d) SHA1(ab628b2ec678839c75245e245297818ef1592d3b) )
	ROM_LOAD("ax1904m01.ic14", 0x4000000, 0x1000000,  CRC(443bfb26) SHA1(6f7751afa0ca55dd0679758b27bed92b31c1b050) )
	ROM_LOAD("ax1905m01.ic15", 0x5000000, 0x1000000,  CRC(eb1cada0) SHA1(459d21d622c72606f1d3095e8a25b6c4adccf8ab) )
	ROM_LOAD("ax1906m01.ic16", 0x6000000, 0x1000000,  CRC(fe6da168) SHA1(d4ab6443383469bb5a4337005de917627a2e21cc) )
	ROM_LOAD("ax1907m01.ic17", 0x7000000, 0x1000000,  CRC(9d3a0520) SHA1(78583fd171b34439f77a04a97ebe3c9d1bab61cc) )
ROM_END

ROM_START( demofist )
	ROM_REGION( 0x200000, "maincpu", 0)
	AW_BIOS

	ROM_REGION( 0x8000000, "user1", ROMREGION_ERASE)
	ROM_LOAD("ax0601p01.ic18", 0x0000000, 0x0800000,  CRC(0efb38ad) SHA1(9400e37efe3e936474d74400ebdf28ad0869b67b) )
	/* incomplete, other rom names / sizes are.. ? */
ROM_END



struct AtomiswaveKey
{
    int P0[16];
    int P1[16];
    int S0[32];
    int S1[16];
    int S2[16];
    int S3[8];
};

static const struct AtomiswaveKey fotns_key = {
    {0,2,7,11,12,1,5,6,15,4,8,9,14,3,10,13},
    {12,8,3,7,0,15,1,11,6,10,4,14,9,5,13,2},
    {4,1,10,16,9,25,26,31,13,0,14,15,24,6,30,18,7,20,5,12,22,17,27,3,8,11,21,29,19,23,28,2},
    {3,2,11,14,10,13,12,0,7,6,8,15,5,1,4,9},
    {10,3,6,12,7,11,4,14,0,2,8,1,15,13,5,9},
    {7,1,6,5,4,2,0,3}
};

static const struct AtomiswaveKey df_key = {
    {1,4,5,6,9,7,10,11,13,0,8,12,14,2,3,15},
    {12,0,3,8,7,6,15,11,1,4,14,10,9,5,13,2},
    {9,27,15,6,28,30,7,12,21,0,1,25,22,3,16,29,13,4,24,20,2,5,23,19,18,10,8,14,17,11,31,26},
    {5,13,4,0,8,12,14,7,2,11,3,10,6,1,15,9},
    {11,6,2,9,12,1,7,4,10,0,13,3,8,14,15,5},
    {1,6,4,3,5,2,7,0}
};


static UINT16 atomiswave_decrypt(UINT16 cipherText, int address, const struct AtomiswaveKey* key)
{
    int b0,b1,b2,b3;
    int aux;

    aux = BITSWAP16(cipherText,
                    key->P0[15],key->P0[14],key->P0[13],key->P0[12],key->P0[11],key->P0[10],key->P0[9],key->P0[8],
                    key->P0[7],key->P0[6],key->P0[5],key->P0[4],key->P0[3],key->P0[2],key->P0[1],key->P0[0]);
    aux = aux ^ BITSWAP16(address/2,
                          key->P1[15],key->P1[14],key->P1[13],key->P1[12],key->P1[11],key->P1[10],key->P1[9],key->P1[8],
                          key->P1[7],key->P1[6],key->P1[5],key->P1[4],key->P1[3],key->P1[2],key->P1[1],key->P1[0]);

    b0 = aux&0x1f;
    b1 = (aux>>5)&0xf;
    b2 = (aux>>9)&0xf;
    b3 = aux>>13;

    b0 = key->S0[b0];
    b1 = key->S1[b1];
    b2 = key->S2[b2];
    b3 = key->S3[b3];

    return (b3<<13)|(b2<<9)|(b1<<5)|b0;
}


static DRIVER_INIT(fotns)
{
  	int i;
	UINT16 *src = (UINT16 *)(memory_region(machine, "user1"));

	long rom_size = memory_region_length(machine, "user1");

	for(i=0; i<rom_size/2; i++)
	{
		src[i] = atomiswave_decrypt(src[i], i*2, &fotns_key);
	}

#if 0
	{
		FILE *fp;
		const char *gamename = machine->gamedrv->name;
		char filename[256];
		sprintf(filename, "%s.dump", gamename);

		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(src, rom_size, 1, fp);
			fclose(fp);
		}
	}
#endif
}



static DRIVER_INIT(demofist)
{
  	int i;
	UINT16 *src = (UINT16 *)(memory_region(machine, "user1"));

	long rom_size = memory_region_length(machine, "user1");

	for(i=0; i<rom_size/2; i++)
	{
		src[i] = atomiswave_decrypt(src[i], i*2, &df_key);
	}

#if 0
	{
		FILE *fp;
		const char *gamename = machine->gamedrv->name;
		char filename[256];
		sprintf(filename, "%s.dump", gamename);

		fp=fopen(filename, "w+b");
		if (fp)
		{
			fwrite(src, rom_size, 1, fp);
			fclose(fp);
		}
	}
#endif
}



/* Info above each set is automatically generated from the IC22 rom and may not be accurate */

/*
SYSTEMID: NAOMI
JAP: GUN SPIKE
USA: CANNON SPIKE
EXP: CANNON SPIKE

NO.     Type    Byte    Word
IC22    32M     0000*   0000* invalid value
IC1     64M     7AC6    C534
IC2     64M     3959    6667
IC3     64M     F60D    69E5
IC4     64M     FBD4    AE40
IC5     64M     1717    F3EC
IC6     64M     A622    1D3D
IC7     64M     33A3    4480
IC8     64M     FC26    A49D
IC9     64M     528D    5206
IC10    64M     7C94    8779
IC11    64M     271E    BEF7
IC12    64M     BA24    102F
*/

ROM_START( cspike )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr23210.22", 0x00000, 0x400000,  CRC(a15c54b5) SHA1(5c7872244d3d648e4c04751f120d0e9d47239921) )

	ROM_REGION( 0x8000000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: CAPCOM VS SNK  JAPAN
USA: CAPCOM VS SNK  USA
EXP: CAPCOM VS SNK  EXPORT

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     B836    4AA4
IC2     64M     19C1    9965
IC3     64M     B98C    EFB2
IC4     64M     2458    31CD
IC5     64M     59D2    E957
IC6     64M     1004    7E0B
IC7     64M     C63F    B2A7
IC8     64M     9D78    342F
IC9     64M     681F    D97A
IC10    64M     7544    E4D3
IC11    64M     8351    8A4C
IC12    64M     B713    2408
IC13    64M     A12E    8DE4

*/

ROM_START( capsnk )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr23511a.ic22", 0x00000, 0x400000,  CRC(fe00650f) SHA1(ca8e9e9178ed2b6598bdea83be1bf0dd7aa509f9) )

	ROM_REGION( 0x8000000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: COSMIC SMASH IN JAPAN
USA: COSMIC SMASH IN USA
EXP: COSMIC SMASH IN EXPORT

NO.     Type    Byte    Word
IC22    32M     0000    0000     EPR23428A.22
IC1     64M     C82B    E769     MPR23420.1
IC2     64M     E0C3    43B6     MPR23421.2
IC3     64M     C896    F766     MPR23422.3
IC4     64M     2E60    4CBF     MPR23423.4
IC5     64M     BB81    7E26     MPR23424.5
IC6     64M     B3A8    F2EA     MPR23425.6
IC7     64M     05C5    A084     MPR23426.7
?IC8     64M     9E13    7535     MPR23427.8

Serial: BCHE-01A0803

*/

ROM_START( csmash )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x4800000, "user1", ROMREGION_ERASE00)
	ROM_LOAD("epr23428a.22", 0x0000000, 0x400000, CRC(d628dbce) SHA1(91ec1296ead572a64c37f8ac2c1a96742f19d50b) )
	ROM_RELOAD( 0x400000, 0x400000)
	ROM_LOAD("mpr23420.1", 0x0800000, 0x0800000, CRC(9d5991f2) SHA1(c75871db314b01935d1daaacf1a762e73e5fd411) )
	ROM_LOAD("mpr23421.2", 0x1000000, 0x0800000, CRC(6c351db3) SHA1(cdd601321a38fc34152517abdc473b73a4c6f630) )
	ROM_LOAD("mpr23422.3", 0x1800000, 0x0800000, CRC(a1d4bd29) SHA1(6c446fd1819f55412351f15cf57b769c0c56c1db) )
	ROM_LOAD("mpr23423.4", 0x2000000, 0x0800000, CRC(08cbf373) SHA1(0d9a593f5cc5d632d85d7253c135eef2e8e01598) )
	ROM_LOAD("mpr23424.5", 0x2800000, 0x0800000, CRC(f4404000) SHA1(e49d941e47e63bb7f3fddc3c3d2c1653611914ee) )
	ROM_LOAD("mpr23425.6", 0x3000000, 0x0800000, CRC(47f51da2) SHA1(af5ecd460114caed3a00157ffd3a2df0fbf348c0) )
	ROM_LOAD("mpr23426.7", 0x3800000, 0x0800000, CRC(7f91b13f) SHA1(2d534f77291ebfedc011bf0e803a1b9243fb477f) )
	ROM_LOAD("mpr23427.8", 0x4000000, 0x0800000, CRC(5851d525) SHA1(1cb1073542d75a3bcc0d363ed31d49bcaf1fd494) )
ROM_END

ROM_START( csmasho )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x4800000, "user1", ROMREGION_ERASE00)
	ROM_LOAD("epr23428.22", 0x0000000, 0x400000, CRC(f8597496) SHA1(2bb9f25b63b7410934ae4b1e052e1308a5c5a57f) )
	ROM_RELOAD( 0x400000, 0x400000)
	ROM_LOAD("mpr23420.1", 0x0800000, 0x0800000, CRC(9d5991f2) SHA1(c75871db314b01935d1daaacf1a762e73e5fd411) )
	ROM_LOAD("mpr23421.2", 0x1000000, 0x0800000, CRC(6c351db3) SHA1(cdd601321a38fc34152517abdc473b73a4c6f630) )
	ROM_LOAD("mpr23422.3", 0x1800000, 0x0800000, CRC(a1d4bd29) SHA1(6c446fd1819f55412351f15cf57b769c0c56c1db) )
	ROM_LOAD("mpr23423.4", 0x2000000, 0x0800000, CRC(08cbf373) SHA1(0d9a593f5cc5d632d85d7253c135eef2e8e01598) )
	ROM_LOAD("mpr23424.5", 0x2800000, 0x0800000, CRC(f4404000) SHA1(e49d941e47e63bb7f3fddc3c3d2c1653611914ee) )
	ROM_LOAD("mpr23425.6", 0x3000000, 0x0800000, CRC(47f51da2) SHA1(af5ecd460114caed3a00157ffd3a2df0fbf348c0) )
	ROM_LOAD("mpr23426.7", 0x3800000, 0x0800000, CRC(7f91b13f) SHA1(2d534f77291ebfedc011bf0e803a1b9243fb477f) )
	ROM_LOAD("mpr23427.8", 0x4000000, 0x0800000, CRC(5851d525) SHA1(1cb1073542d75a3bcc0d363ed31d49bcaf1fd494) )
ROM_END


/*

SYSTEMID: NAOMI
JAP:  POWER SMASH --------------
USA:  VIRTUA TENNIS IN USA -----
EXP:  VIRTUA TENNIS IN EXPORT --

NO. Type    Byte    Word
IC22    32M 0000    1111
IC1 64M 7422    83DD
IC2 64M 7F26    A93D
IC3 64M 8E02    D3FC
IC4 64M 2545    F734
IC5 64M E197    B75D
IC6 64M 9453    CF75
IC7 64M 29AC    2FEB
IC8 64M 0434    2E9E
IC9 64M C86E    79E6
IC10    64M C67A    BF14
IC11    64M F590    D280

*/

ROM_START( vtennis )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x6000000, "user1", 0)
	ROM_LOAD("epr22927.22", 0x0000000, 0x0400000,  CRC(89781723) SHA1(cf644aa66abcec6964d77485a0292f11ba80dd0d) )
	ROM_RELOAD( 0x400000, 0x400000)
	ROM_LOAD("ic1.bin", 0x0800000, 0x0800000, CRC(903873e5) SHA1(09af791bc02cca0e2dc72187679830ed9f4fc772) )
	ROM_LOAD("ic2.bin", 0x1000000, 0x0800000, CRC(5f020fa6) SHA1(bd2519be8c88ff34cf2fd2b17271d2b41b64ce9f) )
	ROM_LOAD("ic3.bin", 0x1800000, 0x0800000, CRC(3c3bf533) SHA1(db43ca9332e76b968b9b388b4824b768f82b9859) )
	ROM_LOAD("ic4.bin", 0x2000000, 0x0800000, CRC(3d8dd003) SHA1(91f494b06b9977215ab726a2499b5855d4d49e81) )
	ROM_LOAD("ic5.bin", 0x2800000, 0x0800000, CRC(efd781d4) SHA1(ced5a8dc8ff7677b3cac2a4fae04670c46cc96af) )
	ROM_LOAD("ic6.bin", 0x3000000, 0x0800000, CRC(79e75be1) SHA1(82318613c947907e01bbe50569b05ef24789d7c9) )
	ROM_LOAD("ic7.bin", 0x3800000, 0x0800000, CRC(44bd3883) SHA1(5c595d903d8865bf8bf3aafb1f527bff232718ed) )
	ROM_LOAD("ic8.bin", 0x4000000, 0x0800000, CRC(9ebdf0f8) SHA1(f1b688bda387fc00c70cb6a0c374c6c13926c138) )
	ROM_LOAD("ic9.bin", 0x4800000, 0x0800000, CRC(ecde9d57) SHA1(1fbe7fdf66a56f4f1765baf113dff95142bfd114) )
	ROM_LOAD("ic10.bin",0x5000000, 0x0800000, CRC(81057e42) SHA1(d41137ae28c64dbdb50150db8cf25851bc0709c4) )
	ROM_LOAD("ic11.bin",0x5800000, 0x0800000, CRC(57eec89d) SHA1(dd8f9a9155e51ee5260f559449fb0ea245077952) )
ROM_END


/*

SYSTEMID: NAOMI
JAP:  DERBY OWNERS CLUB ------------
USA:  DERBY OWNERS CLUB ------------
EXP:  DERBY OWNERS CLUB IN EXPORT --

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     8AF3    D0BC
IC2     64M     1E79    0410
IC3     64M     146D    C51E
IC4     64M     E9AD    86BE
IC5     64M     BBB2    8685
IC6     64M     A0E1    C2E0
IC7     64M     B8CF    67B5
IC8     64M     005E    C1D6
IC9     64M     1F53    9304
IC10    64M     FAC9    8AA4
IC11    64M     B6B1    5665
IC12    64M     21DB    74F5
IC13    64M     A991    A8AB
IC14    64M     05BD    428D

Serial: BAXE-02A1386

*/

ROM_START( derbyoc )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr22099b.22", 0x0000000, 0x0400000,  CRC(5e708879) SHA1(fada4f4bf29fc8f77f354167f8db4f904610fe1a) )

	ROM_REGION( 0x8000000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: DYNAMITE BASEBALL '99
USA: WORLD SERIES 99
EXP: WORLD SERIES 99

NO.     Type    Byte    Word
IC22    16M     0000    0000
IC1     64M     77B9    3C1B
IC2     64M     F7FB    025A
IC3     64M     B3D4    22C1
IC4     64M     060F    6279
IC5     64M     FE49    CAEB
IC6     64M     E34C    5FAD
IC7     64M     CC04    498C
IC8     64M     388C    DF17
IC9     64M     5B91    C458
IC10    64M     AF73    4A18
IC11    64M     2E5B    A198
IC12    64M     FFDB    41CA
IC13    64M     04E1    EA4C
IC14    64M     5B22    DA9A
IC15    64M     64E7    0873
IC16    64M     1EE7    BE11
IC17    64M     79C3    3608
IC18    64M     D4CE    5AEB
IC19    64M     E846    60B8

Serial: BBDE-01A0097

*/

ROM_START( dybb99 )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr22141b.22", 0x0000000, 0x0200000,  CRC(6d0e0785) SHA1(aa19e7bac4c266771d1e65cffa534a49d7566f51) )

	ROM_REGION( 0x9800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic17",0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic18",0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic19",0x9000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: GIANT GRAM 2000
USA: GIANT GRAM 2000
EXP: GIANT GRAM 2000

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     904A    81AE
IC2     64M     E9F7    B152
IC3     64M     A4D0    8FB7
IC4     64M     A869    64FB
IC5     64M     30CB    3483
IC6     64M     94DD    7F14
IC7     64M     BA8B    EA07
IC8     64M     6ADA    5CDA
IC9     64M     7CDA    86C1
IC10    64M     86F2    73A3
IC11    64M     44D8    1D11
IC12    64M     F25E    EDA8
IC13    64M     4804    6251
IC14    64M     E4FE    3808
IC15    64M     FD3D    D37A
IC16    64M     6D48    F5B3
IC17    64M     F0C6    CA29
IC18    64M     07C3    E4AE
IC19    64M     50F8    8500
IC20    64M     4EA2    D0CE
IC21    64M     090B    5667

Serial: BCCG-21A0451

*/

ROM_START( gram2000 )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr23377.22", 0x0000000, 0x0400000,  CRC(4ca3149c) SHA1(9d25fc659658b416202b033754669be2f3abcdbe) )

	ROM_REGION( 0xa800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic17",0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic18",0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic19",0x9000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic20",0x9800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic21",0xa000000, 0x0800000, NO_DUMP )
ROM_END

/*
SYSTEMID: NAOMI
JAP: GIANT GRAM
USA: GIANT GRAM
EXP: GIANT GRAM

NO.     Type    Byte    Word
IC22    16M     0000    1111
IC1     64M     E504    548E

Serial: BAJE-01A0021
*/

ROM_START( ggram2 )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr21820.22", 0x0000000, 0x0200000,  CRC(0a198278) SHA1(0df5fc8b56ddafc66d92cb3923b851a5717b551d) )

	ROM_REGION( 0x0800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: HEAVY METAL JAPAN
USA: HEAVY METAL USA
EXP: HEAVY METAL EURO

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     CBA3    16D2
IC2     64M     087A    079B
IC3     64M     CDB0    804C
IC4     64M     326A    E815
IC5     64M     C164    5DB4
IC6     64M     38A0    AAFC
IC7     64M     1134    DFCC
IC8     64M     6597    6975
IC9     64M     D6FB    8917
IC10    64M     6442    18AC
IC11    64M     4F77    EEFE

*/


ROM_START( hmgeo )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr23716a.22", 0x0000000, 0x0400000,  CRC(c5cb0d3b) SHA1(20de8f5ee183e996ccde77b10564a302939662db) )

	ROM_REGION( 0x5800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: GIGAWING2 JAPAN
USA: GIGAWING2 USA
EXP: GIGAWING2 EXPORT

NO.     Type    Byte    Word
IC22    16M     C1C3    618F
IC1     64M     8C09    3A15
IC2     64M     91DC    C17F
IC3     64M     25CB    2AA0
IC4     64M     EB35    C1FF
IC5     64M     8B25    914E
IC6     64M     72CB    68FA
IC7     64M     191E    2AF3
IC8     64M     EACA    12CD
IC9     64M     717F    40ED
IC10    64M     1E43    0F1A

*/

ROM_START( gwing2 )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr22270.22", 0x0000000, 0x0200000,  CRC(876b3c97) SHA1(eb171d4a0521c3bea42b4aae3607faec63e10581) )

	ROM_REGION( 0x5800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
ROM_END

/*
SYSTEMID: NAOMI
JAP: IDOL JANSHI SUCHIE-PAI 3
USA: DISABLE
EXP: DISABLE

NO.     Type    Byte    Word
IC22    16M     0000    0000
IC1     64M     E467    524B
IC2     64M     9D05    4992
IC3     64M     E3F7    6481
IC4     64M     6C22    25E3
IC5     64M     180F    E89F
IC6     64M     60C9    2B86
IC7     64M     4EDE    4539
IC8     64M     3AD3    0046
IC9     64M     8D37    BA16
IC10    64M     8AE3    4D71
IC11    64M     B519    1393
IC12    64M     4695    B159
IC13    64M     536F    D0C6
IC14    32M     81F9    DA1B
*/

ROM_START( suchie3 )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x8000000, "user1", 0)
	ROM_LOAD("epr21979.22",   0x0000000, 0x0200000, CRC(335c9e25) SHA1(476790fdd99a8c13336e795b4a39b071ed86a97c) )

	ROM_LOAD("mpr-21980.ic1", 0x0800000, 0x0800000, CRC(2b5f958a) SHA1(609585dda27c5e111378a92f04fa03ae11d42540) )
	ROM_LOAD("mpr-21981.ic2", 0x1000000, 0x0800000, CRC(b4fff4ee) SHA1(333fb5a662775662881154b654233f207782a8aa) )
	ROM_LOAD("mpr-21982.ic3", 0x1800000, 0x0800000, CRC(923ee0ff) SHA1(4f92cc1abfd948a1ed15fdca11251aba96bdc022) )
	ROM_LOAD("mpr-21983.ic4", 0x2000000, 0x0800000, CRC(dd659ab1) SHA1(96d9825fc5cf72a9ef83f10e480fd8925b1d6762) )
	ROM_LOAD("mpr-21984.ic5", 0x2800000, 0x0800000, CRC(b34de0c7) SHA1(dbb7a6a19af2571441b5ecbddddae6891809ffcf) )
	ROM_LOAD("mpr-21985.ic6", 0x3000000, 0x0800000, CRC(f1516e0a) SHA1(246d287df592cd69df689dc10e8647a9dbf804b7) )
	ROM_LOAD("mpr-21986.ic7", 0x3800000, 0x0800000, CRC(2779c418) SHA1(8d1a89ddf0c68f1eaf6eb0dafadf9b614492fff1) )
	ROM_LOAD("mpr-21987.ic8", 0x4000000, 0x0800000, CRC(6aaaacdd) SHA1(f5e67c88db8bce8f2f4cab73a5d0a24ba57c812b) )
	ROM_LOAD("mpr-21988.ic9", 0x4800000, 0x0800000, CRC(ed61b155) SHA1(679124f0f7c7bc4791025cff274d903cf5bcae70) )
	ROM_LOAD("mpr-21989.ic10",0x5000000, 0x0800000, CRC(ae8562cf) SHA1(e31986e53159729434a7952e8c4ed2adf8dd8e9d) )
	ROM_LOAD("mpr-21990.ic11",0x5800000, 0x0800000, CRC(57fd9fdd) SHA1(62b3bc4a2828751459557b63d900ca6d46792e24) )
	ROM_LOAD("mpr-21991.ic12",0x6000000, 0x0800000, CRC(d82f834a) SHA1(06902713bdf6f68182749916cacc9ae6528dc355) )
	ROM_LOAD("mpr-21992.ic13",0x6800000, 0x0800000, CRC(599a2fb8) SHA1(2a0007064ad2ee1e1a0fda1d5676df4ff19a9f2f) )
	ROM_LOAD("mpr-21993.ic14",0x7000000, 0x0400000, CRC(fb28cf0a) SHA1(d51b1d4514a93074d1f77bd1bc5995739604cf56) )
ROM_END


/* toy fighter - 1999 sega */

ROM_START( toyfight )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x8000000, "user1", 0)
	ROM_LOAD("epr-22035.ic22",   0x0000000, 0x0400000, CRC(dbc76493) SHA1(a9772bdb62610a39adf2b9f397781bcddda3e635) )

	ROM_LOAD("mpr-22025.ic1", 0x0800000, 0x0800000, CRC(30237202) SHA1(e229a7671b3a34b26a461716bd7b437da100e1c8) )
	ROM_LOAD("mpr-22026.ic2", 0x1000000, 0x0800000, CRC(f28e71ff) SHA1(019425fcf234beca2b586de5235cf9f171563533) )
	ROM_LOAD("mpr-22027.ic3", 0x1800000, 0x0800000, CRC(1a84632d) SHA1(f3880f21399c6713c48c710c06d0344a0a28f026) )
	ROM_LOAD("mpr-22028.ic4", 0x2000000, 0x0800000, CRC(2b34ccba) SHA1(76c39ea19c3be1d9a9ce9e67035be7543b71ff26) )
	ROM_LOAD("mpr-22029.ic5", 0x2800000, 0x0800000, CRC(8162953a) SHA1(15c9e10080a5f2e70c31b9b89a256050a1aed4e9) )
	ROM_LOAD("mpr-22030.ic6", 0x3000000, 0x0800000, CRC(5bf5fed6) SHA1(6c8eedb177aa49aee9a8b090f2e5f96644416c6c) )
	ROM_LOAD("mpr-22031.ic7", 0x3800000, 0x0800000, CRC(ee7c40cc) SHA1(b9d92ef5bae0e932ec8769a30ebd841a263d3e2a) )
	ROM_LOAD("mpr-22032.ic8", 0x4000000, 0x0800000, CRC(3c48c9ba) SHA1(00be199b23040f8e81db2ec489ba98cbf615652c) )
	ROM_LOAD("mpr-22033.ic9", 0x4800000, 0x0800000, CRC(5fe5586e) SHA1(3ff41ae1f81469597684faadd88e62b5e0634352) )
	ROM_LOAD("mpr-22034.ic10",0x5000000, 0x0800000, CRC(3aa5ce5e) SHA1(f00a906235e4522d6fc2ac771324114346875314) )
ROM_END


/*

SYSTEMID: NAOMI
JAP: SLASHOUT JAPAN VERSION
USA: SLASHOUT USA VERSION
EXP: SLASHOUT EXPORT VERSION

NO.  Type   Byte    Word
IC22 32M    0000    0000
IC1  64M    D1BF    FB18
IC2  64M    1F98    4295
IC3  64M    5F61    67E3
IC4  64M    C6A4    449B
IC5  64M    BB2A    58AB
IC6  64M    60B2    5262
IC7  64M    178B    3705
IC8  64M    E4B9    FF46
IC9  64M    D4FC    2273
IC10 64M    6BA5    8087
IC11 64M    7DBA    A143
IC12 64M    B708    0C61
IC13 64M    0C4A    8DF0
IC14 64M    B2FF    A057
IC15 64M    60DB    3D06
IC16 64M    B5EA    4965
IC17 64M    6586    1F3F

*/

ROM_START( slasho )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x9000000, "user1", 0)
	ROM_LOAD("epr23341.22", 0x0000000, 0x0400000,  CRC(477fa123) SHA1(d2474766dcd0b0e5fe317a858534829eb1c26789) )
	ROM_RELOAD( 0x400000, 0x400000)
	ROM_LOAD("ic1.bin", 0x0800000, 0x0800000, CRC(8624493a) SHA1(4fe940a889619f2a75c45e15efb2b8ed9020bc55) )
	ROM_LOAD("ic2.bin", 0x1000000, 0x0800000, CRC(f952d0d4) SHA1(4b5403b98bf977c1e3a045619e1eddb4e4ab69c7) )
	ROM_LOAD("ic3.bin", 0x1800000, 0x0800000, CRC(6c5ce16e) SHA1(110b5d536557ab6610a7c32db2e6e46901da9579) )
	ROM_LOAD("ic4.bin", 0x2000000, 0x0800000, CRC(1b3d02a0) SHA1(3e02a0cb3d945e5d6cea03236d6571d45a7afd51) )
	ROM_LOAD("ic5.bin", 0x2800000, 0x0800000, CRC(50053662) SHA1(3ced87ee533fd7a32d64c41f1fcbde9c648ab188) )
	ROM_LOAD("ic6.bin", 0x3000000, 0x0800000, CRC(96148e80) SHA1(c0f30556395edb9a7558006e89d6adc2f6bdc048) )
	ROM_LOAD("ic7.bin", 0x3800000, 0x0800000, CRC(15f2f9a1) SHA1(9cea71b6f6466ccd840218f5dcb09ea7525208d8) )
	ROM_LOAD("ic8.bin", 0x4000000, 0x0800000, CRC(a084ab51) SHA1(1f5c863012004bbeefc82b172a92011a175428a6) )
	ROM_LOAD("ic9.bin", 0x4800000, 0x0800000, CRC(50539e17) SHA1(38ec16a868c892e177fbb45be563e1b649956550) )
	ROM_LOAD("ic10.bin",0x5000000, 0x0800000, CRC(29891831) SHA1(f318bca11ac5eb24b32d5b910a596280221a44ab) )
	ROM_LOAD("ic11.bin",0x5800000, 0x0800000, CRC(c1ad0614) SHA1(e38ff316da889eb029d0a9348f6b2284f3a36f29) )
	ROM_LOAD("ic12.bin",0x6000000, 0x0800000, CRC(faeb25ed) SHA1(623f3f78c94ba44e77491c18a6521a19b1101a67) )
	ROM_LOAD("ic13.bin",0x6800000, 0x0800000, CRC(63589d0f) SHA1(53770cc1268892e8cdb76b6edf2fb39e8b605554) )
	ROM_LOAD("ic14.bin",0x7000000, 0x0800000, CRC(2bc46263) SHA1(38ec579768ac37ed3ad21911b1970241906af8ea) )
	ROM_LOAD("ic15.bin",0x7800000, 0x0800000, CRC(323e4db2) SHA1(c5484589c1613110faef6cf8b8f4def8867a8226) )
	ROM_LOAD("ic16.bin",0x8000000, 0x0800000, CRC(fd8c2736) SHA1(34ae1a4e35b4aac6666719fb4fc0959bd64ff3d6) )
	ROM_LOAD("ic17.bin",0x8800000, 0x0800000, CRC(001604f8) SHA1(615ec027d383d44d4aadb1175be6320e4139d7d1) )
ROM_END


/*

SYSTEMID: NAOMI
JAP: MOERO JUSTICE GAKUEN  JAPAN
USA: PROJECT JUSTICE  USA
EXP: PROJECT JUSTICE  EXPORT

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     3E87    5491
IC2     64M     2789    9802
IC3     64M     60E7    E775
IC4     64M     36F4    9353
IC5     64M     31B6    CEF6
IC6     64M     3F79    7B58
IC7     64M     620C    A31F
IC8     64M     A093    160C
IC9     64M     4DD9    4184
IC10    64M     AF3F    C64A
IC11    64M     0EE1    A0C2
IC12    64M     2EF9    E0A3
IC13    64M     72A5    3156
IC14    64M     D414    B896
IC15    64M     7BCE    3A7A
IC16    64M     E371    962D
IC17    64M     E813    E342
IC18    64M     D2B8    3989
IC19    64M     3A4B    4614
IC20    64M     11B0    9921
IC21    64M     698C    7A39

Serial: BCLE-01A2130

*/

ROM_START( pjustic )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr23548a.22", 0x0000000, 0x0400000,  CRC(f4ccf1ec) SHA1(97485b2a4b9452ffeea2501f42d20d718410e716) )

	ROM_REGION( 0xa800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic17",0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic18",0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic19",0x9000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic20",0x9800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic21",0xa000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: POWER STONE JAPAN
USA: POWER STONE USA
EXP: POWER STONE EURO

NO. Type    Byte    Word
IC22    16M 0000    0000
IC1 64M 0258    45D8
IC2 64M 0DF2    0810
IC3 64M 5F93    9FAF
IC4 64M 05E0    C80F
IC5 64M F023    3F68
IC6 64M 941E    F563
IC7 64M 374E    46F6
IC8 64M C529    0501

*/

ROM_START( pstone )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr21597.22", 0x0000000, 0x0200000,  CRC(62c7acc0) SHA1(bb61641a7f3650757132cde379447bdc9bd91c78) )

	ROM_REGION( 0x4000000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: POWER STONE 2 JAPAN
USA: POWER STONE 2 USA
EXP: POWER STONE 2 EURO

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     04FF    B3D4
IC2     64M     52D4    0BF0
IC3     64M     5273    0EB8
IC4     64M     B39A    21F5
IC5     64M     53CB    6540
IC6     64M     0AC8    74ED
IC7     64M     D05A    EB30
IC8     64M     8217    4E66
IC9     64M     193C    6851

Serial: BBJE-01A1613

*/

ROM_START( pstone2 )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr23127.22", 0x0000000, 0x0400000,  CRC(185761d6) SHA1(8c91b594dd59313d249c9da7b39dee21d3c9082e) )

	ROM_REGION( 0x4800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: OUTTRIGGER     JAPAN
USA: OUTTRIGGER     USA
EXP: OUTTRIGGER     EXPORT

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     362E    D34B
IC2     64M     4EF4    FF8D
IC3     64M     5E77    9052
IC4     64M     E123    41B3
IC5     64M     43A0    58D4
IC6     64M     C946    D3EE
IC7     64M     5313    3F17
IC8     64M     2591    FEB7
IC9     64M     CBA3    E150
IC10    64M     2639    D291
IC11    64M     3A96    86EA
IC12    64M     8586    3ED5
IC13    64M     9028    E59C
IC14    64M     8A42    26E2
IC15    64M     98C4    1618
IC16    64M     122B    8C85
IC17    64M     3D5E    F9B0
IC18    64M     1EFA    490E
IC19    64M     9F22    6F77

Serial (from 2 carts): BAZE-01A0288
                       BAZE-02A0217

*/

ROM_START( otrigger )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr22163.22", 0x0000000, 0x0400000, CRC(3bdafb6a) SHA1(c4c5a4ba94d85c4353df22d70bb08be67e9c22c3) )

	ROM_REGION( 0x9800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic17",0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic18",0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic19",0x9000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: SAMBA DE AMIGO
USA: SAMBADEAMIGO
EXP: SAMBADEAMIGO

NO.     Type    Byte    Word
IC22    32M     0000    0000
IC1     64M     B1FA    1BE9
IC2     64M     51FD    0C32
IC3     64M     8AA0    6E7A
IC4     64M     3B30    E31D
IC5     64M     D604    FBE3
IC6     64M     1D51    FF2D
IC7     64M     EE89    720D
IC8     64M     0551    7046
IC9     64M     6883    6427
IC10    64M     70E5    CEC3
IC11    64M     E70E    0C63
IC12    64M     0FD0    B1F8
IC13    64M     2D48    6B19
IC14    64M     CBFF    F163
IC15    64M     10D1    E09D
IC16    64M     A10B    DDB4

*/

ROM_START( samba )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr22966b.ic22", 0x0000000, 0x0400000,  CRC(893116b8) SHA1(35cb4f40690ff21af5ab7cc5adbc53228d6fb0b3) )

	ROM_REGION( 0x8000000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: SPAWN JAPAN
USA: SPAWN USA
EXP: SPAWN EURO

NO.     Type    Byte    Word
IC22    32M     FFFF    FFFF
IC1     64M     C56E    3D11
IC2     64M     A206    CC87
IC3     64M     FD3F    C5DF
IC4     64M     5833    09A4
IC5     64M     B42C    AA08
IC6     64M     C7A4    E2DE
IC7     64M     58CB    5DFD
IC8     64M     144B    783D
IC9     64M     A4A8    D0BE
IC10    64M     94A8    401F

Serial: BAVE-02A1305

*/

ROM_START( spawn )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr22977b.22", 0x0000000, 0x0400000,  CRC(814ff5d1) SHA1(5a0a9e55878927f98750000eb7d9391cbecfe21d) )

	ROM_REGION( 0x5000000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: VIRTUA NBA
USA: VIRTUA NBA
EXP: VIRTUA NBA

NO.     Type    Byte    Word
IC22    32M 0000    0000
IC1     64M 5C4A    BB88
IC2     64M 1799    B55E
IC3     64M FB19    6FE8
IC4     64M 6207    33FE
IC5     64M 38F0    F24C
IC6     64M A3B1    FF6F
IC7     64M 737F    B4DD
IC8     64M FD19    49CE
IC9     64M 424E    76D5
IC10    64M 84CC    B74C
IC11    64M 8FC6    D9C8
IC12    64M A838    143A
IC13    64M 88C3    456F
IC14    64M 1C72    971E
IC15    64M B950    F203
IC16    64M 39F6    54CE
IC17    64M 91C7    47B0
IC18    64M 5B94    7E77
IC19    64M DE42    F390
IC20    64M B876    73CE
IC21    64M AD60    2F74

*/

ROM_START( virnba )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr22949.22", 0x0000000, 0x0400000,  CRC(fd91447e) SHA1(0759d6517aeb684d0cb809c1ae1350615cc0aecc) )

	ROM_REGION( 0xa800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic17",0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic18",0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic19",0x9000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic20",0x9800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic21",0xa000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: VIRTUA STRIKER 2 VER.2000
USA: VIRTUA STRIKER 2 VER.2000
EXP: VIRTUA STRIKER 2 VER.2000

NO.     Type    Byte    Word
IC22    32M     2B49    A054    EPR21929C.22
IC1     64M     F5DD    E983    MPR21914
IC2     64M     4CB7    198B    MPR21915
IC3     64M     5661    47C0    MPR21916
IC4     64M     CD15    DC9A    MPR21917
IC5     64M     7855    BCC7    MPR21918
IC6     64M     59D2    CB75    MPR21919
IC7     64M     B795    BE9C    MPR21920
IC8     64M     D2DE    5AF2    MPR21921
IC9     64M     7AAD    0DD5    MPR21922
IC10    64M     B31B    2C4E    MPR21923
IC11    64M     5C32    D746    MPR21924
IC12    64M     1886    D5EA    MPR21925
IC13    64M     D7B3    24D7    MPR21926
IC14    64M     9EF2    E513    MPR21927
IC15    32M     0DF9    FC01    MPR21928

*/

ROM_START( vs2_2k )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x8000000, "user1", 0)
	ROM_LOAD("epr21929c.22", 0x0000000, 0x0400000, CRC(831af08a) SHA1(af4c74623be823fd061765cede354c6a9722fd10) )
	ROM_LOAD("mpr21914.u1",  0x0800000, 0x0800000, CRC(f91ef69b) SHA1(4ed23091efad7ddf1878a0bfcdcbba3cf151af84) )
	ROM_LOAD("mpr21915.u2",  0x1000000, 0x0800000, CRC(40128a67) SHA1(9d191c4ec33465f29bbc09491dde62f354a9ab15) )
	ROM_LOAD("mpr21916.u3",  0x1800000, 0x0800000, CRC(19708b3c) SHA1(7d1ef995ce870ffcb68f420a571efb084f5bfcf2) )
	ROM_LOAD("mpr21917.u4",  0x2000000, 0x0800000, CRC(b082379b) SHA1(42f585279da1de7e613e42b76e1b81986c48e6ea) )
	ROM_LOAD("mpr21918.u5",  0x2800000, 0x0800000, CRC(a3bc1a47) SHA1(0e5043ab6e118feb59f68c84c095cf5b1dba7d09) )
	ROM_LOAD("mpr21919.u6",  0x3000000, 0x0800000, CRC(b1dfada7) SHA1(b4c906bc96b615975f6319a1fdbd5b990e7e4124) )
	ROM_LOAD("mpr21920.u7",  0x3800000, 0x0800000, CRC(1c189e28) SHA1(93400de2cb803357fa17ae7e1a5297177f9bcfa1) )
	ROM_LOAD("mpr21921.u8",  0x4000000, 0x0800000, CRC(55bcb652) SHA1(4de2e7e584dd4999dc8e405837a18a904dfee0bf) )
	ROM_LOAD("mpr21922.u9",  0x4800000, 0x0800000, CRC(2ecda3ff) SHA1(54afc77a01470662d580f5676b4e8dc4d04f63f8) )
	ROM_LOAD("mpr21923.u10", 0x5000000, 0x0800000, CRC(a5cd42ad) SHA1(59f62e995d45311b1592434d1ffa42c261fa8ba1) )
	ROM_LOAD("mpr21924.u11", 0x5800000, 0x0800000, CRC(cc1a4ed9) SHA1(0e3aaeaa55f1d145fb4877b6d187a3ee78cf214e) )
	ROM_LOAD("mpr21925.u12", 0x6000000, 0x0800000, CRC(9452c5fb) SHA1(5a04f96d83cca6248f513de0c6240fc671bcadf9) )
	ROM_LOAD("mpr21926.u13", 0x6800000, 0x0800000, CRC(d6346491) SHA1(830971cbc14cab022a09ad4c6e11ee49c550e308) )
	ROM_LOAD("mpr21927.u14", 0x7000000, 0x0800000, CRC(a1901e1e) SHA1(2281f91ac696cc14886bcdf4b0685ce2f5bb8117) )
	ROM_LOAD("mpr21928.u15", 0x7800000, 0x0400000, CRC(d127d9a5) SHA1(78c95357344ea15469b84fa8b1332e76521892cd) )
ROM_END

/* Sega Marine Fishing */

ROM_START( smarinef )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x8000000, "user1", 0)
	ROM_LOAD("epr-22221.ic22", 0x0000000, 0x0400000, CRC(9d984375) SHA1(fe1185d70b4bc1529e3579fd6b2b678c7d548400) )
	ROM_LOAD("mpr-22208.ic1", 0x0800000, 0x0800000, CRC(6a1e418c) SHA1(7092c6a34ac0c2c6fb2b4b78415d08ef473785d9) )
	ROM_LOAD("mpr-22209.ic2", 0x1000000, 0x0800000, CRC(ecf5be54) SHA1(d7c264da4e232ce6f9b05c9920394f8027fa4a1d) )
	/* IC3 empty */
	/* IC4 empty */
	ROM_LOAD("mpr-22212.ic5", 0x2800000, 0x0800000, CRC(8305f462) SHA1(7993231fa71f509b3b7fec691b5a6139947a01e7) )
	ROM_LOAD("mpr-22213.ic6", 0x3000000, 0x0800000, CRC(0912eaea) SHA1(e4cb1262f3b53d3c619900767cfa192115a53d4b) )
	ROM_LOAD("mpr-22214.ic7", 0x3800000, 0x0800000, CRC(661526b6) SHA1(490321a893f706eaea49c6c35c01af6ae45adf01) )
	ROM_LOAD("mpr-22215.ic8", 0x4000000, 0x0800000, CRC(a80714fa) SHA1(b32dde5cc79a9ae9f7f34064c2382115e9303070) )
	ROM_LOAD("mpr-22216.ic9", 0x4800000, 0x0800000, CRC(cf3d1049) SHA1(a390304256dfac623b6fe1b205d918ce3eb67723) )
	ROM_LOAD("mpr-22217.ic10",0x5000000, 0x0800000, CRC(48c92fd6) SHA1(26b17a8d0130512807cf533a60c10c6d1e769de0) )
	ROM_LOAD("mpr-22218.ic11",0x5800000, 0x0800000, CRC(f9ca31b8) SHA1(ea3d0f38ca1a46c896c06f038a6362ad3c9f90b2) )
	ROM_LOAD("mpr-22219.ic12",0x6000000, 0x0800000, CRC(b3b45811) SHA1(045e7236b814f848d4c9767618ddcd4344d880ec) )
ROM_END


/*

SYSTEMID: NAOMI
JAP: ZOMBIE REVENGE IN JAPAN
USA: ZOMBIE REVENGE IN USA
EXP: ZOMBIE REVENGE IN EXPORT

NO. Type    Byte    Word
IC22    16M 0000    0000
IC1 64M 899B    97E1
IC2 64M 6F0B    2D2D
IC3 64M 4328    C898
IC4 64M 0205    57C5
IC5 64M 93A7    A717
IC6 64M 936B    A35B
IC7 64M 2F51    2BFC
IC8 64M D627    54C5
IC9 64M D2F9    B84C
IC10    64M 9B5A    B70B
IC11    64M 3F0F    9AEB
IC12    64M 5778    EBCA
IC13    64M 75DB    8563
IC14    64M 427A    577C
IC15    64M A7B7    D0D6
IC16    64M 9F01    FCFE
IC17    64M DFB4    58F7
IC18    64M C453    B313
IC19    64M 04B8    49FB

*/

ROM_START( zombrvn )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr21707.22", 0x0000000, 0x0200000,  CRC(4daa11e9) SHA1(2dc219a5e0d0b41cce6d07631baff0495c479e13) )

	ROM_REGION( 0x9800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic17",0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic18",0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic19",0x9000000, 0x0800000, NO_DUMP )
ROM_END

/*

SYSTEMID: NAOMI
JAP: DEAD OR ALIVE 2
USA: DEAD OR ALIVE 2 USA ------
EXP: DEAD OR ALIVE 2 EXPORT----

NO.     Type    Byte    Word
IC22    32M     2B49    A054
IC1     64M     B74A    1815
IC2     64M     6B34    AB5A
IC3     64M     7EEF    EA1F
IC4     64M     0700    8C2F
IC5     64M     E365    B9CC
IC6     64M     7FE0    DC66
IC7     64M     BF8D    439B
IC8     64M     84DC    2F86
IC9     64M     15CF    8961
IC10    64M     7776    B985
IC11    64M     BCE9    21E9
IC12    64M     87FA    E9C0
IC13    64M     B82E    47A7
IC14    64M     3821    846E
IC15    64M     B491    C66E
IC16    64M     5774    918D
IC17    64M     219B    A171
IC18    64M     4848    643A
IC19    64M     6E1F    2570
IC20    64M     0CED    F2A8
IC21    64M     002C    8ECA

*/

ROM_START( doa2 )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0xb000000, "user1", 0)
	ROM_LOAD("epr22121.22", 0x0000000, 0x0400000,  CRC(30f93b5e) SHA1(0e33383e7ab9a721dab4708b063598f2e9c9f2e7) ) // partially encrypted

	ROM_LOAD("mpr-22100.ic1", 0x0800000, 0x0800000, CRC(92a53e5e) SHA1(87fcdeee9c4e65a3eb6eb345eed85d4f2df26c3c) )
	ROM_LOAD("mpr-22101.ic2", 0x1000000, 0x0800000, CRC(14cd7dce) SHA1(5df14a5dad14bc922b4f88881dc2e9c8e74d6170) )
	ROM_LOAD("mpr-22102.ic3", 0x1800000, 0x0800000, CRC(34e778b1) SHA1(750ddf5cda9622dd2b0f7069d247ffd55911c38f) )
	ROM_LOAD("mpr-22103.ic4", 0x2000000, 0x0800000, CRC(6f3db8df) SHA1(e9bbcf7897594ae47a9e3c8641ccb2c09b0809fe) )
	ROM_LOAD("mpr-22104.ic5", 0x2800000, 0x0800000, CRC(fcc2787f) SHA1(c28eaf91fa64e49e2276702678a4f8f17e09c3b9) )
	ROM_LOAD("mpr-22105.ic6", 0x3000000, 0x0800000, CRC(3e2da942) SHA1(d8f28c40ab59fa96a1fb19ad3adbee687088a5ab) )
	ROM_LOAD("mpr-22106.ic7", 0x3800000, 0x0800000, CRC(03aceaaf) SHA1(977e5b660254e7c5fdbd9d52c1f00c8a174a5d7b) )
	ROM_LOAD("mpr-22107.ic8", 0x4000000, 0x0800000, CRC(6f1705e4) SHA1(b8215dd4ef7214e75c2ec79ad974a32422c17647) )
	ROM_LOAD("mpr-22108.ic9", 0x4800000, 0x0800000, CRC(d34d3d8a) SHA1(910f1e4d8a54a621d9212e1425152c3029c96234) )
	ROM_LOAD("mpr-22109.ic10",0x5000000, 0x0800000, CRC(00ef44dd) SHA1(3fd100007daf59693de2329df1b4981dcdf435cd) )
	ROM_LOAD("mpr-22110.ic11",0x5800000, 0x0800000, CRC(a193b577) SHA1(3513853f88c491905481dadc5ce00cc5819b2663) )
	ROM_LOAD("mpr-22111.ic12",0x6000000, 0x0800000, CRC(55dddebf) SHA1(a7b8702cf578f5be4dcf8e2eaf11bf8b71d1b4ad) )
	ROM_LOAD("mpr-22112.ic13",0x6800000, 0x0800000, CRC(c5ffe564) SHA1(efe4d0cb5a536b26489c6dd31b1e446a9be643c9) )
	ROM_LOAD("mpr-22113.ic14",0x7000000, 0x0800000, CRC(12e7adf0) SHA1(2755c3efc6ca6d5680ead1489f42798c0187c5a4) )
	ROM_LOAD("mpr-22114.ic15",0x7800000, 0x0800000, CRC(d181d0a0) SHA1(2a0e46dbb31f5c11b6ae2fc8c786192bf3701ec5) )
	ROM_LOAD("mpr-22115.ic16",0x8000000, 0x0800000, CRC(ee2c842d) SHA1(8e33f241300481bb8875bda37e3917be71ed2594) )
	ROM_LOAD("mpr-22116.ic17",0x8800000, 0x0800000, CRC(224ab770) SHA1(85d849ee077e36da1df759caa4a32525395f741c) )
	ROM_LOAD("mpr-22117.ic18",0x9000000, 0x0800000, CRC(884a45a9) SHA1(d947cb3a045c5463523355fa631d55148e12c31e) )
	ROM_LOAD("mpr-22118.ic19",0x9800000, 0x0800000, CRC(8d631cbf) SHA1(fe8a65d35b1cdaed650ddde931e59f0768ffff53) )
	ROM_LOAD("mpr-22119.ic20",0xa000000, 0x0800000, CRC(d608fa86) SHA1(54c8107cccec8cbb536f13cda5b220b7972190b7) )
	ROM_LOAD("mpr-22120.ic21",0xa800000, 0x0800000, CRC(a30facb4) SHA1(70415ca34095c795297486bce1f956f6a8d4817f) )
ROM_END

/*

SYSTEMID: NAOMI
JAP: DEAD OR ALIVE 2
USA: DEAD OR ALIVE 2 USA ------
EXP: DEAD OR ALIVE 2 EXPORT----

NO.     Type    Byte    Word
IC22    32M     2B49    A054
IC1     64M     B74A    1815
IC2     64M     6B34    AB5A
IC3     64M     7EEF    EA1F
IC4     64M     0700    8C2F
IC5     64M     E365    B9CC
IC6     64M     7FE0    DC66
IC7     64M     BF8D    439B
IC8     64M     84DC    2F86
IC9     64M     15CF    8961
IC10    64M     7776    B985
IC11    64M     BCE9    21E9
IC12    64M     87FA    E9C0
IC13    64M     B82E    47A7
IC14    64M     3821    846E
IC15    64M     B491    C66E
IC16    64M     5774    918D
IC17    64M     219B    A171
IC18    64M     4848    643A
IC19    64M     6E1F    2570
IC20    64M     0CED    F2A8
IC21    64M     002C    8ECA

Serial: BALH-13A0175

*/

ROM_START( doa2m )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0xb000000, "user1", 0)
	ROM_LOAD("doa2verm.22", 0x0000000, 0x0400000,  CRC(94b16f08) SHA1(225cd3e5dd5f21facf0a1d5e66fa17db8497573d) )

	ROM_LOAD("mpr-22100.ic1", 0x0800000, 0x0800000, CRC(92a53e5e) SHA1(87fcdeee9c4e65a3eb6eb345eed85d4f2df26c3c) )
	ROM_LOAD("mpr-22101.ic2", 0x1000000, 0x0800000, CRC(14cd7dce) SHA1(5df14a5dad14bc922b4f88881dc2e9c8e74d6170) )
	ROM_LOAD("mpr-22102.ic3", 0x1800000, 0x0800000, CRC(34e778b1) SHA1(750ddf5cda9622dd2b0f7069d247ffd55911c38f) )
	ROM_LOAD("mpr-22103.ic4", 0x2000000, 0x0800000, CRC(6f3db8df) SHA1(e9bbcf7897594ae47a9e3c8641ccb2c09b0809fe) )
	ROM_LOAD("mpr-22104.ic5", 0x2800000, 0x0800000, CRC(fcc2787f) SHA1(c28eaf91fa64e49e2276702678a4f8f17e09c3b9) )
	ROM_LOAD("mpr-22105.ic6", 0x3000000, 0x0800000, CRC(3e2da942) SHA1(d8f28c40ab59fa96a1fb19ad3adbee687088a5ab) )
	ROM_LOAD("mpr-22106.ic7", 0x3800000, 0x0800000, CRC(03aceaaf) SHA1(977e5b660254e7c5fdbd9d52c1f00c8a174a5d7b) )
	ROM_LOAD("mpr-22107.ic8", 0x4000000, 0x0800000, CRC(6f1705e4) SHA1(b8215dd4ef7214e75c2ec79ad974a32422c17647) )
	ROM_LOAD("mpr-22108.ic9", 0x4800000, 0x0800000, CRC(d34d3d8a) SHA1(910f1e4d8a54a621d9212e1425152c3029c96234) )
	ROM_LOAD("mpr-22109.ic10",0x5000000, 0x0800000, CRC(00ef44dd) SHA1(3fd100007daf59693de2329df1b4981dcdf435cd) )
	ROM_LOAD("mpr-22110.ic11",0x5800000, 0x0800000, CRC(a193b577) SHA1(3513853f88c491905481dadc5ce00cc5819b2663) )
	ROM_LOAD("mpr-22111.ic12",0x6000000, 0x0800000, CRC(55dddebf) SHA1(a7b8702cf578f5be4dcf8e2eaf11bf8b71d1b4ad) )
	ROM_LOAD("mpr-22112.ic13",0x6800000, 0x0800000, CRC(c5ffe564) SHA1(efe4d0cb5a536b26489c6dd31b1e446a9be643c9) )
	ROM_LOAD("mpr-22113.ic14",0x7000000, 0x0800000, CRC(12e7adf0) SHA1(2755c3efc6ca6d5680ead1489f42798c0187c5a4) )
	ROM_LOAD("mpr-22114.ic15",0x7800000, 0x0800000, CRC(d181d0a0) SHA1(2a0e46dbb31f5c11b6ae2fc8c786192bf3701ec5) )
	ROM_LOAD("mpr-22115.ic16",0x8000000, 0x0800000, CRC(ee2c842d) SHA1(8e33f241300481bb8875bda37e3917be71ed2594) )
	ROM_LOAD("mpr-22116.ic17",0x8800000, 0x0800000, CRC(224ab770) SHA1(85d849ee077e36da1df759caa4a32525395f741c) )
	ROM_LOAD("mpr-22117.ic18",0x9000000, 0x0800000, CRC(884a45a9) SHA1(d947cb3a045c5463523355fa631d55148e12c31e) )
	ROM_LOAD("mpr-22118.ic19",0x9800000, 0x0800000, CRC(8d631cbf) SHA1(fe8a65d35b1cdaed650ddde931e59f0768ffff53) )
	ROM_LOAD("mpr-22119.ic20",0xa000000, 0x0800000, CRC(d608fa86) SHA1(54c8107cccec8cbb536f13cda5b220b7972190b7) )
	ROM_LOAD("mpr-22120.ic21",0xa800000, 0x0800000, CRC(a30facb4) SHA1(70415ca34095c795297486bce1f956f6a8d4817f) )
ROM_END

/*

SYSTEMID: NAOMI
JAP: DYNAMITE BASEBALL NAOMI
USA: SAMPLE GAME IN USA--------
EXP: SAMPLE GAME

NO.     Type    Byte    Word
IC22    16M     EF41    1DBC
IC1     64M     2743    8DE9
IC2     64M     1D2B    B4D5
IC3     64M     9127    8536
IC4     64M     946A    851B
IC5     64M     BDF4    AF2C
IC6     64M     78A2    DADB
IC7     64M     9816    06D3
IC8     64M     F8D9    9C38
IC9     64M     3C7D    532A
IC10    64M     37A2    D3F1
IC11    64M     5BF2    05FC
IC12    64M     694F    A25A
IC13    64M     685C    CDA8
IC14    64M     3DFA    32A9
IC15    64M     071F    820F
IC16    64M     1E89    D6B5
IC17    64M     889C    504B
IC18    64M     8B78    1BB5
IC19    64M     9816    7EE9
IC20    64M     E5C2    CECB
IC21    64M     5C65    8F82

Serial: ??? (sticker removed)

*/

ROM_START( dybbnao )
	ROM_REGION( 0x200000, "maincpu", 0)
	NAOMI_BIOS

	ROM_REGION( 0x400000, "user1", 0)
	ROM_LOAD("epr21575.22", 0x0000000, 0x0200000, CRC(ba61e248) SHA1(3cce5d8b307038515d7da7ec567bfa2e3aafc274) )

	ROM_REGION( 0xa800000, "user2", 0)
	ROM_LOAD("ic1", 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic2", 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic3", 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic4", 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic5", 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic6", 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic7", 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic8", 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic9", 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic10",0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic11",0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic12",0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic13",0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic14",0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic15",0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic16",0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic17",0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic18",0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic19",0x9000000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic20",0x9800000, 0x0800000, NO_DUMP )
	ROM_LOAD("ic21",0xa000000, 0x0800000, NO_DUMP )
ROM_END

/* GD-ROM titles - a PIC supplies a decryption key

PIC stuff

command             response                   comment

kayjyo!?          ->:\x70\x1f\x71\x1f\0\0\0    (unlock gdrom)
C1strdf0          ->5BDA.BIN                   (lower part of boot filename string, BDA.BIN in this example)
D1strdf1          ->6\0\0\0\0\0\0\0            (upper part of filename string)
bsec_ver          ->8VER0001                   (always the same? )
atestpic          ->7TEST_OK                   (always the same? )
AKEYCODE          ->3.......                   (high 7 bytes of des key)
Bkeycode          ->4.\0\0\0\0\0\0             (low byte of des key, then \0 fill)
!.......          ->0DIMMID0                   (redefine upper 7 bytes of session key)
".......          ->1DIMMID1                   (redefine next 7 bytes)
#..               ->2DIMMID2                   (last 2 bytes)


default session key is
"NAOMIGDROMSYSTEM"

info from Elsemi:

it sends bsec_ver, and if it's ok, then the next commands are the session key changes
if you want to have the encryption described somewhere so it's not lost. it's simple:
unsigned char Enc(unsigned char val,unsigned char n)
{
    val^=Key[8+n];
    val+=Key[n];

    return val;
}

do for each value in the message to send
that will encrypt the char in the nth position in the packet to send
time to go to sleep


*/

#ifdef UNUSED_FUNCTION
// rather crude function to write out a key file
void naomi_write_keyfile(void)
{
	// default key structure
	UINT8 response[10][8] = {
	{ ':', 0x70, 0x1f, 0x71, 0x1f, 0x00, 0x00, 0x00 }, // response to kayjyo!?
	{ '8', 'V',  'E',  'R',  '0',  '0',  '0',  '1'  }, // response to bsec_ver
	{ '7', 'T',  'E',  'S',  'T',  '_',  'O',  'K'  }, // response to atestpic
	{ '6', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // response to D1strdf1 (upper part of filename)
	{ '5', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // response to C1strdf0 (lower part of filename)
	{ '4', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // response to Bkeycode (lower byte of DES key)
	{ '3', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // response to AKEYCODE (high 7 bytes of DES key)
	{ '2', 'D',  'I',  'M',  'M',  'I',  'D',  '2'  }, // response to #..      (rewrite low 2 bytes of session key)
	{ '1', 'D',  'I',  'M',  'M',  'I',  'D',  '1'  }, // response to "....... (rewrite middle 7 bytes of session key)
	{ '0', 'D',  'I',  'M',  'M',  'I',  'D',  '0'  }, // response to !....... (rewrite upper 7 bytes of session key)
	};

	int i;
	char bootname[256];
	char picname[256];

	// ######### edit this ###########
	UINT64 key = 0x4FF16D1A9E0BFBCDULL;

	memset(bootname,0x00,14);
	memset(picname,0x00,256);

	// ######### edit this ###########
	strcpy(picname,"317-5072-com.data");
	strcpy(bootname,"BCY.BIN");

	for (i=0;i<14;i++)
	{
		if (i<7)
		{
			response[4][i+1] = bootname[i];
		}
		else
		{
			response[3][i-6] = bootname[i];
		}
	}

	for (i=0;i<8;i++)
	{
		UINT8 keybyte = (key>>(7-i)*8)&0xff;

		if (i<7)
		{
			response[6][i+1] = keybyte;
		}
		else
		{
			response[5][1] = keybyte;
		}
	}


	{
		FILE *fp;
		fp=fopen(picname, "w+b");
		if (fp)
		{
			fwrite(response, 10*8, 1, fp);
			fclose(fp);
		}
	}


}
#endif

/* All games have the regional titles at the start of the IC22 rom in the following order

  JAPAN
  USA
  EXPORT (EURO in some titles)
  KOREA (ASIA in some titles)
  AUSTRALIA
  UNUSED
  UNUSED
  UNUSED

  with the lists below it has been assumed that if the title is listed for a region
  then it is available / works in that region, this has not been confirmed as correct.

*/

/* Naomi & Naomi GD-ROM */
GAME( 1998, naomi,    0,        naomi,    naomi,    0, ROT0, "Sega",            "Naomi Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )

/* Complete Dumps */
GAME( 2001, csmash,   naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Cosmic Smash (JPN, USA, EXP, KOR, AUS) (rev. A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, csmasho,  csmash,   naomi,    naomi,    0, ROT0, "Sega",            "Cosmic Smash (JPN, USA, EXP, KOR, AUS) (original)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, suchie3,  naomi,    naomi,    naomi,    0, ROT0, "Jaleco",          "Idol Janshi Suchie-Pai 3 (JPN)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, vs2_2k,   naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Virtua Striker 2 Ver. 2000 (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, smarinef, naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Sega Marine Fishing", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, toyfight, naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Toy Fighter", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, doa2,     naomi,    naomi,    naomi,    0, ROT0, "Tecmo",           "Dead or Alive 2 (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, doa2m,    doa2,     naomi,    naomi,    0, ROT0, "Tecmo",           "Dead or Alive 2 Millennium (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, vtennis,  naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Power Smash (JPN) / Virtua Tennis (USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, slasho,   naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Slashout (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )


/* Incomplete Dumps (just the program rom IC22) */
GAME( 2000, cspike,   naomi,    naomi,    naomi,    0, ROT0, "Psikyo / Capcom", "Gun Spike (JPN) / Cannon Spike (USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, capsnk,   naomi,    naomi,    naomi,    0, ROT0, "Capcom / SNK",    "Capcom Vs. SNK Millennium Fight 2000 (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, derbyoc,  naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Derby Owners Club (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1998, dybb99,   naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Dynamite Baseball '99 (JPN) / World Series '99 (USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, gram2000, naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Giant Gram 2000 (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, ggram2,   naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Giant Gram (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING ) // strings in rom don't contain '2'
GAME( 2001, hmgeo,    naomi,    naomi,    naomi,    0, ROT0, "Capcom",          "Heavy Metal Geomatrix (JPN, USA, EUR, ASI, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, gwing2,   naomi,    naomi,    naomi,    0, ROT0, "Takumi / Capcom", "Giga Wing 2 (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, pjustic,  naomi,    naomi,    naomi,    0, ROT0, "Capcom",          "Moero Justice Gakuen (JPN) / Project Justice (USA, EXP, KOR, AUS) ", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, pstone,   naomi,    naomi,    naomi,    0, ROT0, "Capcom",          "Power Stone (JPN, USA, EUR, ASI, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, pstone2,  naomi,    naomi,    naomi,    0, ROT0, "Capcom",          "Power Stone 2 (JPN, USA, EUR, ASI, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, otrigger, naomi,    naomi,    naomi,    0, ROT0, "Sega",            "OutTrigger (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, samba,    naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Samba De Amigo (JPN)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, spawn,    naomi,    naomi,    naomi,    0, ROT0, "Capcom",          "Spawn (JPN, USA, EUR, ASI, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, virnba,   naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Virtua NBA (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1999, zombrvn,  naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Zombie Revenge (JPN, USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1998, dybbnao,  naomi,    naomi,    naomi,    0, ROT0, "Sega",            "Dynamite Baseball NAOMI (JPN)", GAME_NO_SOUND|GAME_NOT_WORKING )

/* Games with game specific bios sets */
GAME( 1998, hod2bios, 0,        naomi,    naomi,    0, ROT0, "Sega",            "Naomi House of the Dead 2 Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )
/* HOTD2 isn't dumped */



ROM_START( gundmgd )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0001", 0, SHA1(615e19c22f32096f3aad557019a14313b60a4070) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5069-COM)
	//(sticker 253-5509-5069)
	ROM_LOAD("317-5069-com.data", 0x00, 0x50, CRC(8e2f0cbd) SHA1(a5f3a990a03bfa50a1a742593c5ec07645c8718d) )
ROM_END


ROM_START( sfz3ugd )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0002", 0, SHA1(af4669fdd7ce8e6ec4a170748d401e322a3d7ae8) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5072-COM)
	//(sticker 253-5509-5072)
	ROM_LOAD("317-5072-com.data", 0x00, 0x50, CRC(6d2992b9) SHA1(88e6dc6711f9f883362ba1217a3350d452a70896) )
ROM_END

ROM_START( cvsgd )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0004", 0,  SHA1(7a7fba0fbbc769c5120b08e6d692f1ac63a42225) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5076-JPN)
	//(sticker 253-5509-5076J)
	ROM_LOAD("317-5076-jpn.data", 0x00, 0x50, CRC(5004161b) SHA1(8b2cdfec12ffd9160bc74659e08d07cbc46a4011) )
ROM_END


ROM_START( gundmxgd )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0006", 0, SHA1(b28d6598711b5a9c744bbf07ad03fc60962d2e28) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5079-COM)
	//(sticker 253-5509-5079)
	ROM_LOAD("317-5079-com.data", 0x00, 0x50, CRC(e6abe978) SHA1(700e610d84e517793a22d6cabd1aef9c3b8bc092) )
ROM_END


ROM_START( cvs2gd )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0007a", 0, SHA1(56510390667b39b3915d8bc078660cbe093cf566) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5078-COM)
	//(sticker 253-5509-5078)
	ROM_LOAD("317-5078-com.data", 0x00, 0x50, CRC(1c8d94ee) SHA1(bec4a6901f62dc8f76f7b9d72284b3eaac340bf3) )
ROM_END

ROM_START( ikaruga )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0010", 0, SHA1(65dcc22dd9e9b70975096464ad8e31a4a73dc5fd) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5081-JPN)
	//(sticker 253-5509-5081J)
	ROM_LOAD("317-5081-jpn.data", 0x00, 0x50, CRC(d4cc5c8c) SHA1(44c0c5c2744fbd419b684cbc36f01973487bafbc) )
ROM_END


ROM_START( ggxx )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0011", 0, SHA1(b7328eb2c588d55284bdcea0fe89bb8e629a8669) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5082-COM)
	//(sticker 253-5509-5082)
	ROM_LOAD("317-5082-com.data", 0x00, 0x50, CRC(fa31209d) SHA1(bb18e6412a02510832f7200a06a3179ef1695ef2) )
ROM_END


ROM_START( moeru )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0013", 0, SHA1(c8869069c28bc8eec96d820886bc388d69d46143) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdl-0013.data", 0x00, 0x50, CRC(56de2066) SHA1(a16a6d9f7272d3f8d322c85222a0487a87811910) )
ROM_END

ROM_START( chocomk )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0014a", 0, SHA1(f88d8203c8692f51c9492d5549a3ad7d9583dc6f) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5085-JPN)
	//(sticker 253-5509-5085J)
	ROM_LOAD("317-5085-jpn.data", 0x00, 0x50, CRC(eecd8140) SHA1(471fb6b242eff646173265df891109e3e0a37a7d) )
ROM_END


ROM_START( quizqgd )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0017", 0, SHA1(94a9319633388968611892e36691b45c94b4f83f) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5090-JPN)
	//(sticker 253-5509-5090J)
	ROM_LOAD("317-5090-jpn.data", 0x00, 0x50, CRC(b4dd88f6) SHA1(c9aacd79c1088225fa5a69b7bd31a7c1286160e1) )
ROM_END



ROM_START( ggxxrl )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0019a", 0, SHA1(d44906505ff698eda6feee6c2b9402e19f64e5d3) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5092-JPN)
	//(sticker 253-5509-5092J)
	ROM_LOAD("317-5092-jpn.data", 0x00, 0x50, CRC(7c8cca4b) SHA1(92c5a0fd8916744eefc023e64daea69803573928) )
ROM_END

ROM_START( tetkiwam )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0020", 0, SHA1(7b2ef47ca2038d6a93615b760b03e8f7cb1b83c2) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5095-JPN)
	//(sticker 253-5509-5095J)
	ROM_LOAD("gdl-0020.data", 0x00, 0x50, CRC(06bc5013) SHA1(f7a46b7e34b20409ce2fdae80e5cdfff7adb9c64) )
ROM_END


ROM_START( shikgam2 )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0021", 0, SHA1(f5036711a28a211e8d71400a8322db3172c5733f) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5095-JPN)
	//(sticker 253-5509-5095J)
	ROM_LOAD("317-5095-jpn.data", 0x00, 0x50, CRC(6033ec89) SHA1(9e99a8ad43fa29296dbf2e13b3a3d4552130b4e8) )
ROM_END

ROM_START( usagui )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0022", 0, SHA1(45deba05a12abbf6390c0fc0e4cdeaedfa7d2ca5) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5095-JPN)
	//(sticker 253-5509-5095J)
	ROM_LOAD("gdl-0022.data", 0x00, 0x50, CRC(621e827a) SHA1(cdc7580f5d1dfe85d2806233f22bc4f13fd62946) )
ROM_END


ROM_START( bdrdown )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0023a", 0, SHA1(caac915104d61f2122f5afe27da1ef5fa9cf9f9a) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5097-JPN)
	//(sticker 253-5509-5097J)
	ROM_LOAD("317-5097-jpn.data", 0x00, 0x50, CRC(e689d047) SHA1(7e3e298d9a8076af0254faeb0eb89fbfce94718d) )
ROM_END

ROM_START( psyvar2 )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0024", 0,  SHA1(d346762036fb1c40a261a434b50e63459f306f14) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C621A (317-5100-JPN)
	//(sticker 253-5509-5100J)
	ROM_LOAD("317-5100-jpn.data", 0x00, 0x50, CRC(94316f0f) SHA1(e1ec2b4225105dbaa1e59e8a05027e73f7b725a9) )
ROM_END


ROM_START( cfield )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0025", 0, SHA1(be0d88eb4f48403a2ceaa7ef588ed60b96ba93bf) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdl-0025.data", 0x00, 0x50, CRC(32adf2eb) SHA1(d86752e6fe9ccac093c512828fca5b7ae62a3ff2) )
ROM_END


ROM_START( trizeal )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0026", 0, SHA1(e4c1e51292a7923b25bfc61d38fe386bf596002a) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C621A (317-5103-JPN)
	//(sticker 253-5509-5103J)
	ROM_LOAD("317-5103-jpn.data", 0x00, 0x50,  CRC(3affbf82) SHA1(268746e86e7546f4bab54bdd268f7b58f10c1aaf) )
ROM_END

ROM_START( meltybld )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0028c", 0, SHA1(66de09738551e351784cc9695a58b35fdf6b6c4b) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5104-JPN)
	//(sticker 253-5509-5104J)
	ROM_LOAD("317-5104-jpn.data", 0x00, 0x50, CRC(fedc8305) SHA1(c535545937213f726f25e6aa8eb3746a794e9100) )
ROM_END



ROM_START( senko )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0030a", 0,  SHA1(1f7ade47e37a0026451b5baf3ba746400de8d156) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5107-JPN)
	//(sticker 253-5509-5107J)
	ROM_LOAD("317-5107-jpn.data", 0x00, 0x50, CRC(7b607409) SHA1(a9946a0637453e4813bef18060d4420355cff800) )
ROM_END

ROM_START( senkoo )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0030", 0,  SHA1(c7f25c05f47a490c5da9369c588b6136e93c280e) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C621A (317-5107-JPN)
	//(sticker 253-5509-5107J)
	ROM_LOAD("317-5107-jpn.data", 0x00, 0x50, CRC(7b607409) SHA1(a9946a0637453e4813bef18060d4420355cff800) )
ROM_END


ROM_START( ss2005 )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0031a", 0, SHA1(6091525845fc2042ed43cae5a1b60c603e16cf97) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5108-JPN)
	//(sticker 253-5509-5108J)
	ROM_LOAD("317-5108-jpn.data", 0x00, 0x50, CRC(6a2eb334) SHA1(cab407d2e994f33aa921d50f399b17e6fbf98eb0) )
ROM_END

ROM_START( radirgy )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0032", 0, SHA1(ebd7a40e59082e660ebf9a2d4ae7cb64371dae8d) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdl-0032.data", 0x00, 0x50, CRC(04e4ac45) SHA1(4102a4d68f20a7e78f6c7e3494e7229018e30e39) )
ROM_END



ROM_START( ggxxsla )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0033a", 0, SHA1(29de69ae97a9099b1bbe936dfa965bb4a3195f68) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C621A (317-5111-JPN)
	//(sticker 253-5509-5111J)
	ROM_LOAD("317-5111-jpn.data", 0x00, 0x50, CRC(a517c70d) SHA1(5f9798941355fb9abce511508c860653d6369e72) )
ROM_END


ROM_START( kurucham )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0034", 0, SHA1(10fd7edb0b620133c003d686e5af2ed27004fa09) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdl-0034.data", 0x00, 0x50,  CRC(f40072a8) SHA1(366df2079a4d2ff7a93082c9bf849aad40ab079d) )
ROM_END

ROM_START( undefeat )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0035", 0, SHA1(91da482a6a082e48bee5b3bd20d9c92d23936965) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5117-JPN)
	//(sticker 253-5509-5117J)
	ROM_LOAD("317-5117-jpn.data", 0x00, 0x50,  CRC(f90f6d3b) SHA1(a18f803a8e951c375a3a55e4b0e74b698ae93f92) )
ROM_END


ROM_START( meltyb )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0039", 0, SHA1(ffc7f6e113ad69422a4f22f318bdf9b1dc5c25db) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5104-JPN)
	//(sticker 253-5509-5104J)
	ROM_LOAD("gdl-0039.data", 0x00, 0x50, CRC(4d6e2c77) SHA1(3bed734c291140d0a61afa40f221395369a251a9) )
ROM_END

ROM_START( meltyba )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0039a", 0, SHA1(e6aa3d65b43a20606e6754bcb8665438770a1f8c) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5104-JPN)
	//(sticker 253-5509-5104J)
	ROM_LOAD("gdl-0039.data", 0x00, 0x50, CRC(4d6e2c77) SHA1(3bed734c291140d0a61afa40f221395369a251a9) )
ROM_END


ROM_START( trgheart )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0036a", 0, SHA1(91f1e19136997cb1e2edfb1ad342b9427d1d3bfb) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5121-JPN)
	//(sticker 253-5509-5121J)
	ROM_LOAD("317-5121-jpn.data", 0x00, 0x50, CRC(a417b20f) SHA1(af6ed7ebf95948bff3e8df915b229189b8de1e46) )
ROM_END

ROM_START( jingystm )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0037", 0, SHA1(99ffe2987e3002b3871daf276d2be45f2e9c6e74) )


	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-5122-JPN)
	//(sticker 253-5509-5122J)
	ROM_LOAD("317-5122-jpn.data", 0x00, 0x50, CRC(0b85b7e4) SHA1(f4e419682ddc4b98a330e5ae543f9276c9bde030) )
ROM_END

ROM_START( karous )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0040", 0, SHA1(a62c8d4b6c5be44a4aeeea1a1a94f3d0fe542593) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdl-0040.data", 0x00, 0x50, CRC(9d37b5e3) SHA1(e1d3cdc2ed82c864c9ff54d9399a80b70ba150c5) )
ROM_END


ROM_START( takoron )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdl-0042", 0, SHA1(984a4fa012d83dd8c748304958c847c9867f4125) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdl-0042.data", 0x00, 0x50, CRC(e1a6dbe4) SHA1(61b458937acca55e4010f86b684aaa86b8c10eac) )
ROM_END

/* -------------------------------- 1st party -------------- */


/*
Title   CONFIDENTIAL MISSION
Media ID    FFCA
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDS-0001
Version V1.050
Release Date    20001011
Manufacturer ID SEGA ENTERPRISES
TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 3788    8558928
track02.raw 3939    6071    5016816
track03.bin 45150   549299  1185760800


PIC
317-0298-COM
*/

ROM_START( confmiss )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0001", 0, SHA1(bd05f197ba8643577883dd25d9d5a74c91b27ca9) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0298-com.data", 0x00, 0x50, CRC(c989b336) SHA1(40075500888626cc2261133eec496b3e753631e5) )
ROM_END


ROM_START( sprtjam )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0003", 0, SHA1(caaba214c1faca78b3370bcd4190eb2853d7f825) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0300-COM)
	//(sticker 253-5508-0300)
	ROM_LOAD("317-0300-com.data", 0x00, 0x50, CRC(9a08413f) SHA1(d57649dcc3af578d55a93dd7a3f41da62d580f54) )
ROM_END


ROM_START( slashout )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0004", 0, SHA1(3cce788393ed194ba9b603f9896ff893691d6b00) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0302-COM)
	//(sticker 253-5508-0302)
	ROM_LOAD("317-0302-com.data", 0x00, 0x50, CRC(4bf6cd62) SHA1(c1fdf12a4d80fa3008170c89d2dc583f19e0450b) )
ROM_END


ROM_START( spkrbtl )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0005", 0, SHA1(d1c3fb2350e4a89372373e7f629c42b741af29b3) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0303-COM)
	//(sticker 253-5508-0303)
	ROM_LOAD("317-0303-com.data", 0x00, 0x50, CRC(6e7888a3) SHA1(5ca78052bcfd9e9f81934cbddd9c173e88973e0e) )
ROM_END

/*
Title   MONKEY_BALL
Media ID    43EB
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDS-0008
Version V1.008
Release Date    20010425
Manufacturer ID
Track   Start Sector    End Sector  Track Size
track01.bin 150 449 705600
track02.raw 600 2732    5016816
track03.bin 45150   549299  1185760800


PIC

253-5508-0307
317-0307-COM
*/

ROM_START( monkeyba )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0008", 0, SHA1(2fadcd141bdbde77b2b335b270959a516af44d99) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0307-com.data", 0x00, 0x50, CRC(448bedc7) SHA1(092dbe5e28890d3ee40d62ca8cbf225c3ce90304) )
ROM_END


ROM_START( dygolf )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0009", 0, SHA1(d502155ddaf881c2c9505528004b9904aa32a59c) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0308-COM)
	//(sticker 253-5508-0308)
	ROM_LOAD("317-0308-com.data", 0x00, 0x50,  CRC(56f63af0) SHA1(3c453226fc53d2f700b3634db3ef8ce206d94392) )
ROM_END

ROM_START( wsbbgd )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0010", 0,  SHA1(c3135ede3a8bdadab91aed49abacbfbde8037069) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0309-COM)
	//(sticker 253-5508-0309)
	ROM_LOAD("317-0309-com.data", 0x00, 0x50, CRC(8792c550) SHA1(e8d6d91583d1673d8d3fa9ccb0ab1097c5c5ad08) )
ROM_END

ROM_START( vtennisg )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0011", 0,  SHA1(b778403d73c8cdd13383691c9be2094ddfc1ba84) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C621A (317-0312-COM)
	//(sticker 253-5508-0312)
	ROM_LOAD("317-0312-com.data", 0x00, 0x50, CRC(6b24f78f) SHA1(43f89815ec46cf014d941b4b9238da044b338b4c) )
ROM_END

ROM_START( keyboard )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0017", 0, SHA1(fb86eff3ef38de7fd78cfde897d5332d2092c172) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gds-0017.data", 0x00, 0x50, CRC(c1277eb3) SHA1(529ed5a133550e2854f8656cd377706060a7befa) )
ROM_END

ROM_START( vathlete )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0019", 0, SHA1(955d3c0cb991be3057138c562cff69c5ef887787) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0330-COM)
	//(sticker 253-5508-0330)
	ROM_LOAD("317-0330-com.data", 0x00, 0x50, CRC(f5e7f7d4) SHA1(3903337e82011d132993e4366475586866bd39b1) )
ROM_END

/*

Title   VIRTUA TENNIS 2 (POWER SMASH 2)
Media ID    D72C
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDS-0015A
Version V2.000
Release Date    20010827
Manufacturer ID
TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 449 705600
track02.raw 600 2732    5016816
track03.bin 45150   549299  1185760800


PIC

253-5508-0318
317-0318-EXP
*/

ROM_START( vtennis2 )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0015a", 0, SHA1(c6e9c9901bd4f075454b7f18baf08df81bc2f1ad) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0318-exp.data", 0x00, 0x50, CRC(7758ade6) SHA1(c62f35810bce466bfb0f55fd555066efd53e9bb6) )
ROM_END

ROM_START( lupinsho )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0018", 0, SHA1(0633a99a666f363ab30450a76b9753685d6b1f57) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0332-j.data", 0x00, 0x50, CRC(31f2b632) SHA1(bbf253bfe831308a7e7fde3a4a28e5bcd2fbb273) )
ROM_END

ROM_START( luptype )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0021a", 0,  SHA1(15c6f9434494a31693cbb8e33da36e0e8a8f7c62) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0332-JPN)
	//(sticker 253-5508-0332J)
	ROM_LOAD("317-0332-jpn.data", 0x00, 0x50, CRC(ab302661) SHA1(65164cf76d78b281772bfcbf5a733b0200e86e09) )
ROM_END

/*
Title   THE_MAZE_OF_THE_KINGS
Media ID    E3D0
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDS-0022
Version V1.001
Release Date    20020306
Manufacturer ID
TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 449 705600
track02.raw 600 2732    5016816
track03.bin 45150   549299  1185760800


PIC
317-0333-COM
253-5508-0333

*/
ROM_START( mok )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0022", 0, SHA1(70b41745225006e7876176cbd239edecd4c3f8b6) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0333-com.data", 0x00, 0x50, CRC(0c07970f) SHA1(8882dd2f8ed522790ea78eed80cfa9442f88f67b) )
ROM_END

ROM_START( ngdup23a )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0023a", 0, SHA1(cd9d808b59eb8f40673ec4353d476f2b9c7f783c) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE) // uses the vf4 pic
	//PIC16C622A (317-0314-COM)
	//(sticker 253-5508-0314)
	ROM_LOAD("317-0314-com.data", 0x00, 0x50, CRC(91a97eb4) SHA1(059342368bc5d25b494ed3c729870695f9584fc7) )
ROM_END

ROM_START( ngdup23c )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0023c", 0, SHA1(1fcb5530748886f4c4f45487d047859182ff7496))

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE) // uses the vf4 evolution pic
	//PIC16C622A (317-0338-JPN)
	//(sticker 253-5508-0338J)
	ROM_LOAD("317-0338-jpn.data", 0x00, 0x50, CRC(eeb2c9e9) SHA1(d30b5914c603219daea9923e1cf8da2be6096742) )
ROM_END

ROM_START( puyofev )
	NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0031", 0, SHA1(da2d421da9472b149619b6931bb2fe624be75fa2) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C621A (317-0375-COM)
	//(sticker 253-5508-0375)
	ROM_LOAD("317-0375-com.data", 0x00, 0x50, CRC(32bf1825) SHA1(42dfbc6777c154d8de6c6f7350da9ea737380220) )
ROM_END

static READ64_HANDLER( naomigd_bios_idle_skip_r )
{

	if (cpu_get_pc(space->cpu)==0xc04173c)
		cpu_spinuntil_time(space->cpu, ATTOTIME_IN_USEC(500));
		//cpu_spinuntil_int(space->cpu);
//  else
//      printf("%08x\n", cpu_get_pc(space->cpu));

	return naomi_ram64[0x2ad238/8];
}
static DRIVER_INIT(naomigd)
{
	memory_install_read64_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xc2ad238, 0xc2ad23f, 0, 0, naomigd_bios_idle_skip_r);
}

static READ64_HANDLER( naomigd_ggxxsla_idle_skip_r )
{
	if (cpu_get_pc(space->cpu)==0x0c0c9adc)
		cpu_spinuntil_time(space->cpu, ATTOTIME_IN_USEC(500));

	return naomi_ram64[0x1aae18/8];
}

static DRIVER_INIT( ggxxsla )
{
	memory_install_read64_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xc1aae18, 0xc1aae1f, 0, 0, naomigd_ggxxsla_idle_skip_r);
	DRIVER_INIT_CALL(naomigd);
}

static READ64_HANDLER( naomigd_ggxx_idle_skip_r )
{
	if (cpu_get_pc(space->cpu)==0xc0b5c3c) // or 0xc0bab0c
		cpu_spinuntil_time(space->cpu, ATTOTIME_IN_USEC(500));

	return naomi_ram64[0x1837b8/8];
}


static DRIVER_INIT( ggxx )
{
	memory_install_read64_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xc1837b8, 0xc1837bf, 0, 0, naomigd_ggxx_idle_skip_r);
	DRIVER_INIT_CALL(naomigd);
}

static READ64_HANDLER( naomigd_ggxxrl_idle_skip_r )
{
	if (cpu_get_pc(space->cpu)==0xc0b84bc) // or 0xc0bab0c
		cpu_spinuntil_time(space->cpu, ATTOTIME_IN_USEC(500));

	//printf("%08x\n", cpu_get_pc(space->cpu));

	return naomi_ram64[0x18d6c8/8];
}


static DRIVER_INIT( ggxxrl )
{
	memory_install_read64_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0xc18d6c8, 0xc18d6cf, 0, 0, naomigd_ggxxrl_idle_skip_r);
	DRIVER_INIT_CALL(naomigd);
}


/* Naomi GD-Rom Sets */
GAME( 2001, naomigd,   0,        naomi,    naomi, naomigd,       ROT0, "Sega",           "Naomi GD-ROM Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )

// GDL-xxxx (licensed games?)
GAME( 2001, gundmgd,   naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Capcom",           "Mobile Suit Gundam: Federation VS Zeon (GDL-0001)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, sfz3ugd,   naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Capcom",           "Street Fighter Zero 3 Upper (GDL-0002)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, cvsgd,     naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Capcom",           "Capcom vs SNK Millenium Fight 2000 Pro (GDL-0004)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, gundmxgd,  naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Capcom",           "Mobile Suit Gundam: Federation VS Zeon DX  (GDL-0006)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, cvs2gd,    naomigd,  naomigd,  naomi, naomigd,	 ROT0,   "Capcom",           "Capcom vs SNK 2 Millionaire Fighting 2001 (Rev A) (GDL-0007A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, ikaruga,   naomigd,  naomigd,  naomi, naomigd,   ROT270, "Treasure",         "Ikaruga (GDL-0010)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, ggxx,      naomigd,  naomigd,  naomi, ggxx,      ROT0,   "Arc System Works", "Guilty Gear XX (GDL-0011)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, moeru,     naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Altron",   		 "Moeru Casinyo (GDL-0013)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, chocomk,   naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Ecole Software",   "Musapey's Choco Marker (Rev A) (GDL-0014A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, quizqgd,   naomigd,  naomigd,  naomi, naomigd,   ROT270, "Amedio",           "Quiz Keitai Q mode (GDL-0017)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, ggxxrl,    naomigd,  naomigd,  naomi, ggxxrl,    ROT0,   "Arc System Works", "Guilty Gear XX #Reload (Rev A) (GDL-0019A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 200?, tetkiwam,  naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Success",          "Tetris Kiwamemichi (GDL-0020)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, shikgam2,  naomigd,  naomigd,  naomi, naomigd,   ROT270, "Alpha System",     "Shikigami No Shiro II / The Castle of Shikigami II (GDL-0021)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, usagui,    naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Warashi",          "Usagui - Yamashiro Mahjong Hen (GDL-0022)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, bdrdown,   naomigd,  naomigd,  naomi, naomigd,   ROT0,   "G-Rev",            "Border Down (Rev A) (GDL-0023A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, psyvar2,   naomigd,  naomigd,  naomi, naomigd,   ROT270, "G-Rev",            "Psyvariar 2 - The Will To Fabricate (GDL-0024)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, cfield,    naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Able",             "Chaos Field (GDL-0025)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, trizeal,   naomigd,  naomigd,  naomi, naomigd,   ROT270, "Taito",            "Trizeal (GDL-0026)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, meltybld,  naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Ecole Software",   "Melty Blood Act Cadenza (Rev C) (GDL-0028C)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, senko,     naomigd,  naomigd,  naomi, naomigd,   ROT0,   "G-Rev",            "Senko No Ronde NEW ver. (Rev A) (GDL-0030A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, senkoo,    senko,    naomigd,  naomi, naomigd,   ROT0,   "G-Rev",            "Senko No Ronde (GDL-0030)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, ss2005,    naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Starfish",         "Super Shanghai 2005 (Rev A) (GDL-0031A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, radirgy,   naomigd,  naomigd,  naomi, naomigd,   ROT270, "Milestone",        "Radirgy (GDL-0032)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, ggxxsla,   naomigd,  naomigd,  naomi, ggxxsla,   ROT0,   "Arc System Works", "Guilty Gear XX Slash (Rev A) (GDL-0033A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2006, kurucham,  naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Able",             "Kurukuru Chameleon (GDL-0034)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, undefeat,  naomigd,  naomigd,  naomi, naomigd,   ROT270, "G-Rev",            "Under Defeat (GDL-0035)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, trgheart,  naomigd,  naomigd,  naomi, naomigd,   ROT270, "Warashi",          "Trigger Heart Exelica (Rev A) (GDL-0036A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, jingystm,  naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Atrativa Japan",   "Jingi Storm - The Arcade (GDL-0037)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2006, meltyb,    naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Ecole Software",   "Melty Blood Act Cadenza Ver B (GDL-0039)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2006, meltyba,   meltyb,   naomigd,  naomi, naomigd,   ROT0,   "Ecole Software",   "Melty Blood Act Cadenza Ver B (Rev A) (GDL-0039A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2006, karous,    naomigd,  naomigd,  naomi, naomigd,   ROT270, "Milestone",        "Karous (GDL-0040)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2006, takoron,   naomigd,  naomigd,  naomi, naomigd,   ROT0,   "Compile",          "Noukone Puzzle Takoron (GDL-0042)", GAME_NO_SOUND|GAME_NOT_WORKING )





// GDS-xxxx (first party games?)
GAME( 2001, confmiss,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Confidential Mission (GDS-0001)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, sprtjam,   naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Sports Jam (GDS-0003)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, slashout,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Slashout (GDS-0004)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, spkrbtl,   naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Spikers Battle (GDS-0005)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, monkeyba,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Monkey Ball (GDS-0008)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, dygolf,    naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Virtua Golf / Dynamic Golf (GDS-0009)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, wsbbgd,    naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "World Series Baseball / Super Major League (GDS-0010)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, vtennisg,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Virtua Tennis (GDS-0011)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, vathlete,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Virtua Athletics / Virtua Athlete (GDS-0019)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, vtennis2,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Virtua Tennis 2 (Rev A) (GDS-0015A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, keyboard,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "La Keyboardxyu (GDS-0017)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, lupinsho,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Lupin The Third - The Shooting (GDS-0018)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, luptype,   naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Lupin The Third - The Typing (Rev A) (GDS-0021A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, mok,       naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "The Maze of the Kings (GDS-0022)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, ngdup23a,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Naomi DIMM Firmware Updater (GDL-0023A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, ngdup23c,  naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Naomi DIMM Firmware Updater (GDL-0023C)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, puyofev,   naomigd,  naomigd,  naomi, naomigd,  ROT0, "Sega",          "Puyo Puyo Fever (GDS-0031)", GAME_NO_SOUND|GAME_NOT_WORKING )

/* Naomi 2 & Naomi 2 GD-ROM */

ROM_START( vstrik3c )

	NAOMI2_BIOS

	ROM_REGION( 0xb000000, "user1", 0)
	ROM_LOAD("epr23663.22", 0x0000000, 0x0400000, CRC(7007fec7) SHA1(523168f0b218d0bd5c815d65bf0caba2c8468c9d) )
	ROM_LOAD("ic.1", 0x0800000, 0x0800000, CRC(db8bf632) SHA1(cd0c5c385dc33778eb6477f48a33d31f8461b810) )
	ROM_LOAD("ic.2", 0x1000000, 0x0800000, CRC(f5889f8b) SHA1(540aa6a9adf10d5426ac4962afdc300cc9c5c6ba) )
	ROM_LOAD("ic.3", 0x1800000, 0x0800000, CRC(74e7ef35) SHA1(6b63910fa05f98b31689882d3f7f32ea84fb11e5) )
	ROM_LOAD("ic.4", 0x2000000, 0x0800000, CRC(69e5eeb3) SHA1(1c615e6150e8c9f5211f7db55158c9b56b9b52fe) )
	ROM_LOAD("ic.5", 0x2800000, 0x0800000, CRC(7ea41bdb) SHA1(1a58c4731f9cc570f79e81a3b87b590d52cf3887) )
	ROM_LOAD("ic.6", 0x3000000, 0x0800000, CRC(cf730ca3) SHA1(f8e132463d574be906f573ca66fbc8d74959be08) )
	ROM_LOAD("ic.7", 0x3800000, 0x0800000, CRC(b8d838fe) SHA1(6722f54a170302978e06305773903a336c6fc9d8) )
	ROM_LOAD("ic.8", 0x4000000, 0x0800000, CRC(03aacb8b) SHA1(7ab12a3c79718fea8b7ae019acafb86bb7381c7e) )
	ROM_LOAD("ic.9", 0x4800000, 0x0800000, CRC(3252196e) SHA1(2e45de03931554e6500fbb83a0d2b1249f2d3015) )
	ROM_LOAD("ic.10",0x5000000, 0x0800000, CRC(b448948b) SHA1(1df5bbd14b24a952bb5289778ceea15ffe161491) )
	ROM_LOAD("ic.11",0x5800000, 0x0800000, CRC(937a3280) SHA1(46c6277402c029a55aa0e2417ea14634d17e2601) )
	ROM_LOAD("ic.12",0x6000000, 0x0800000, CRC(e3ca01df) SHA1(1c96e596d5ad6e08fcc7625788952d5ca5c80ce5) )
	ROM_LOAD("ic.13",0x6800000, 0x0800000, CRC(9bbbbbdc) SHA1(19d124743b686e4d5d38181cee547452402cdf61) )
	ROM_LOAD("ic.14",0x7000000, 0x0800000, CRC(eed0270a) SHA1(2b75a2d5bda05e68bb7d81570a7f4e180085fbd5) )
	ROM_LOAD("ic.15",0x7800000, 0x0800000, CRC(02bee590) SHA1(77ce37209f525be58d9373b5467b6be9f0e7fb4b) )
	ROM_LOAD("ic.16",0x8000000, 0x0800000, CRC(025f4f2f) SHA1(b104dc936036464dcf17524fd5e639c9852e1bd1) )
	ROM_LOAD("ic.17",0x8800000, 0x0800000, CRC(98fc3c54) SHA1(d2ba54b9bf15d2b3d623c5358bbf104b48d23223) )
	ROM_LOAD("ic.18",0x9000000, 0x0800000, CRC(e9fe50a4) SHA1(053f2a33ab7310c518a66b49d83fc817f2db3ef0) )
	ROM_LOAD("ic.19",0x9800000, 0x0800000, CRC(bbd0139c) SHA1(f87e729534d503f2bc37b7d595d25070ae33353c) )
	ROM_LOAD("ic.20",0xa000000, 0x0800000, CRC(5a69ab9e) SHA1(205a8facded5dbee8653807ef4d4b5e4210f56ef) )
	ROM_LOAD("ic.21",0xa800000, 0x0800000, CRC(d6ef7d68) SHA1(4ee396af6c5caf4c5af6e9ad0e03a7ac2c5039f4) )
ROM_END



ROM_START( vstrik3 )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0006", 0, SHA1(aca09a88506f5e462ad3fb33eac5478a2a010609) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0304-COM)
	//(sticker 253-5508-0304)
	ROM_LOAD("317-0304-com.data", 0x00, 0x50, CRC(a181c601) SHA1(6a489904941e638ac1069b66e76ee0bcec7d0bab) )
ROM_END

ROM_START( vf4 )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0012", 0, SHA1(c34588f59c6091cd1c3ef235171dad8d5247e707) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0314-COM)
	//(sticker 253-5508-0314)
	ROM_LOAD("317-0314-com.data", 0x00, 0x50, CRC(91a97eb4) SHA1(059342368bc5d25b494ed3c729870695f9584fc7) )
ROM_END

ROM_START( vf4b )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0012b", 0, SHA1(9b8e05c3d28a09323b13c198dfcc2b771bba67cd) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0314-COM)
	//(sticker 253-5508-0314)
	ROM_LOAD("317-0314-com.data", 0x00, 0x50, CRC(91a97eb4) SHA1(059342368bc5d25b494ed3c729870695f9584fc7) )
ROM_END

ROM_START( vf4c )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0012c", 0, SHA1(0ec149d7edfb326777cdc45a2ac8ad578a32aba1) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0314-COM)
	//(sticker 253-5508-0314)
	ROM_LOAD("317-0314-com.data", 0x00, 0x50, CRC(91a97eb4) SHA1(059342368bc5d25b494ed3c729870695f9584fc7) )
ROM_END


ROM_START( vf4evo )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0024b", 0, SHA1(a829169542f3bed76095ad6bfbbde7d494d04d72) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0338-JPN)
	//(sticker 253-5508-0338J)
	ROM_LOAD("317-0338-jpn.data", 0x00, 0x50, CRC(eeb2c9e9) SHA1(d30b5914c603219daea9923e1cf8da2be6096742) )
ROM_END

ROM_START( vf4evoa )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0024a", 0, SHA1(6225e778d73db18be26f882d4f9cd3b3a136d1c9) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0338-JPN)
	//(sticker 253-5508-0338J)
	ROM_LOAD("317-0338-jpn.data", 0x00, 0x50, CRC(eeb2c9e9) SHA1(d30b5914c603219daea9923e1cf8da2be6096742) )
ROM_END

ROM_START( initdv2j )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0026", 0, SHA1(253acede106b7fbf49e24458e7fda868720e9549) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gds-0026.rom", 0x00, 0x50, NO_DUMP) // file on GD-ROM is BFK.BIN , _NOT_ BEM.BIN which is for Initial D : Arcade Stage (Japan)
ROM_END

ROM_START( vf4tuned ) // are there multiple files on this GD-ROM? it only compresses to 500 meg when the rom file is closer to half tha
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0036f", 0, SHA1(ea35d6ecdf94e5c9a545952758da80f658755df0) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0387-COM)
	//(sticker 253-5508-0387)
	ROM_LOAD("317-0387-com.data", 0x00, 0x50, CRC(ab9f3851) SHA1(8b64dc6df176eb7adb48267709a27db221d5e3c3) )
ROM_END

ROM_START( vf4tunedd )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0036d", 0, SHA1(2f7654307a4c978c5af6c8238c44e70275dd34f9) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0387-COM)
	//(sticker 253-5508-0387)
	ROM_LOAD("317-0387-com.data", 0x00, 0x50, CRC(ab9f3851) SHA1(8b64dc6df176eb7adb48267709a27db221d5e3c3) )
ROM_END


ROM_START( vf4tuneda )
	NAOMI2_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0036a", 0, SHA1(cd630fc4e8f7ed5641b85c609584d7efe0eac137) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0387-COM)
	//(sticker 253-5508-0387)
	ROM_LOAD("317-0387-com.data", 0x00, 0x50, CRC(ab9f3851) SHA1(8b64dc6df176eb7adb48267709a27db221d5e3c3) )
ROM_END


/*

Title   BEACH SPIKERS
Media ID    0897
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDS-0014
Version V1.001
Release Date    20010613
Manufacturer ID
TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 449 705600
track02.raw 600 2746    5049744
track03.bin 45150   549299  1185760800

PIC

253-5508-0317
317-0317-COM

*/

ROM_START( beachspi )
	NAOMI2_BIOS
//  NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0014", 0, SHA1(1ebb3695196c11a86276e034df2e1c8d7fa6b96f) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0317-COM)
	//(sticker 253-5508-0317)
	ROM_LOAD("317-0317-com.data", 0x00, 0x50, CRC(66efe433) SHA1(7f7b52202ed9b1e20516aaa7553cc3cc677a70b5) )
ROM_END

ROM_START( initd )
	NAOMI2_BIOS
//  NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0020b", 0, SHA1(c0e901623ef4fcd97b7e4d29ae556e6f2e91b8ad) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	//PIC16C622A (317-0331-JPN)
	//(sticker 253-5508-0331J)
	ROM_LOAD("317-0331-jpn.data", 0x00, 0x50, CRC(bb39742e) SHA1(b3100b18aeb80ebfd5312ba5c320e7e647710b55) )
ROM_END

ROM_START( initdexp )
	NAOMI2_BIOS
//  NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0025", 0, SHA1(8ea92cf6b493f21b9453832edad7cbc5e5b350c1) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0343-com.data", 0x00, 0x50, CRC(e9d8fac0) SHA1(85f5bbffbd9d1f7162bae46ddd49e7870fe93662) )
ROM_END

ROM_START( initdv3j )
	NAOMI2_BIOS
//  NAOMIGD_BIOS

	ROM_REGION( 0x10000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gds-0032b", 0, SHA1(568411aa72ca308a03a6b5b61c79833464b88bc6) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gds-0032_pic", 0x00, 0x50, NO_DUMP ) // PIC was missing
ROM_END


GAME( 2001, naomi2,   0,        naomi,    naomi,    0, ROT0, "Sega",            "Naomi 2 Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )

//Naomi 2 Cart Games
GAME( 2001, vstrik3c, naomi2,  naomi,    naomi,    0,  ROT0, "Sega",          "Virtua Striker 3 (Cart) (USA, EXP, KOR, AUS)", GAME_NO_SOUND|GAME_NOT_WORKING )

// GDS-xxxx (first party games?)
GAME( 2001, vstrik3, naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Virtua Striker 3 (GDS-0006)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, vf4,     naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 (GDS-0012)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, vf4b,    vf4,     naomigd,    naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 (Rev B) (GDS-0012B)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, vf4c,    vf4,     naomigd,    naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 (Rev C) (GDS-0012C)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, initd,   naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Initial D Arcade Stage (Rev B) (Japan) (GDS-0020B)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, beachspi,naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Beach Spikers (GDS-0014)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, initdexp,naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Initial D Arcade Stage (Export) (GDS-0025)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, vf4evo,  naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 Evolution (Rev B) (GDS-0024B)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, vf4evoa, vf4evo,  naomigd,    naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 Evolution (Rev A) (GDS-0024A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2002, initdv2j,naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Initial D : Arcade Stage Ver. 2 (Japan) (GDS-0026)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, initdv3j,naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Initial D : Arcade Stage Ver. 3 (Japan) (Rev B) (GDS-0032B)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, vf4tuned,naomi2,  naomigd,    naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 Final Tuned (Rev F) (GDS-0036F)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, vf4tunedd,vf4tuned,naomigd,   naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 Final Tuned (Rev D) (GDS-0036D)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, vf4tuneda,vf4tuned,naomigd,   naomi,    0,  ROT0, "Sega",          "Virtua Fighter 4 Final Tuned (Rev A) (GDS-0036A)", GAME_NO_SOUND|GAME_NOT_WORKING )


/* Atomiswave */
GAME( 2001, awbios,   0,        naomi,    naomi,    0, ROT0, "Sammy",           "Atomiswave Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )
GAME( 2005, fotns,    awbios,   naomi,    naomi,    fotns, ROT0, "Arc System Works",           "Fist Of The North Star", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, demofist, awbios,   naomi,    naomi,    demofist, ROT0, "Polygon Magic / Dimps",           "Demolish Fist", GAME_NO_SOUND|GAME_NOT_WORKING )

/*********************************************************************************************************************
**********************************************************************************************************************
*********************************************************************************************************************/

// Triforce -- this is GameCube based, not Dreamcast/Naomi based, move out of here
// (currently here for testing the gd-rom stuff only)
#define TRIFORCE_BIOS \
	ROM_REGION( 0x200000, "maincpu", 0) \
	ROM_SYSTEM_BIOS( 0, "bios0", "Triforce Bios" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0,  "triforce_bootrom.bin", 0x000000, 0x200000, CRC(d1883221) SHA1(c3cb7227e4dbc2af861e76d00cb59726105a2e4c) ) \

ROM_START( triforce )
	TRIFORCE_BIOS

	ROM_REGION( 0x8400000, "user1", ROMREGION_ERASE)
ROM_END

/*
Title   VIRTUA_STRIKER_2002
Media ID    0DD8
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDT-0002
Version V1.005
Release Date    20020730


PIC

253-5508-337E
317-0337-EXP
*/

ROM_START( vs2002ex )
	TRIFORCE_BIOS
	//NAOMIGD_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdt-0002", 0, SHA1(471e896d43167c93cc229cfc94ff7ac6de7cf9a4) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0337-exp.data", 0x00, 0x50, CRC(aa6be604) SHA1(fabc43ecfb7ddf1d5a87f10884852027d6f4773b) )
ROM_END


ROM_START( gekpurya )
	TRIFORCE_BIOS
	//NAOMIGD_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdt-0008c", 0, SHA1(2c1bdb8324efc216edd771fe45c680ac726111a0) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdt-0008c.data", 0x00, 0x50, CRC(08434e5e) SHA1(2121999e851f6f62ab845e6de40849d850ac9d1c) )
ROM_END

ROM_START( tfupdate )
	TRIFORCE_BIOS
	//NAOMIGD_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdt-0011", 0, SHA1(71bfa8f53d211085c020d54f55eeeabf85212a0b) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdt-0011.data", 0x00, 0x50, CRC(08434e5e) SHA1(2121999e851f6f62ab845e6de40849d850ac9d1c) )
ROM_END


/*

Title   VIRTUA STRIKER 4
Media ID    93B2
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDT-0015
Version V1.001
Release Date    20041202
Manufacturer ID
TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 449 705600
track02.raw 600 1951    3179904
track03.bin 45150   549299  1185760800


PIC
255-5508-393E
317-0393-EXP

*/

ROM_START( vs4 )
	TRIFORCE_BIOS
	//NAOMIGD_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdt-0015", 0, SHA1(1f83712b2b170d6edf4a27c15b6f763cc3cc4b71) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0393-exp.data", 0x00, 0x50, CRC(2dcfecd7) SHA1(d805168e1564051ae5c47876ade2c9843253c6b4) )
ROM_END


/*

Title   TRF GDROM TBA EX SATL
Media ID    92C5
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDT-0010C
Version V4.000
Release Date    20040608
Manufacturer ID

TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 599 1058400
track02.raw 750 2101    3179904
track03.bin 45150   549299  1185760800
*/

ROM_START( avalon13 )
	TRIFORCE_BIOS
	//NAOMIGD_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdt-0010c", 0, SHA1(716c441d8dc9036a13c66ef0048cd6d32ac63c4e) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdt-0010c.data", 0x00, 0x50, CRC(6c51e5d6) SHA1(84afef983f1f855fe8722f55baa8ea5121da9369) )
ROM_END


/*

Title   TRF GDROM TBT SATL
Media ID    1348
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDT-0017B
Version V3.001
Release Date    20041102
Manufacturer ID

TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 599 1058400
track02.raw 750 2101    3179904
track03.bin 45150   549299  1185760800

*/

ROM_START( avalon20 )
	TRIFORCE_BIOS
	//NAOMIGD_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdt-0017b", 0, SHA1(e2dd32c322ffcaf38b82275d2721b71bb3dfc1f2) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdt-0017b.data", 0x00, 0x50, CRC(32cb46d4) SHA1(a58b9e03d57b317133d9b6c29e42852af8e77559) )
ROM_END



GAME( 2002, triforce, 0,        naomigd,    naomi,    0, ROT0, "Sega",           "Triforce Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )
GAME( 2002, vs2002ex, triforce, naomigd,    naomi,    0, ROT0, "Sega",           "Virtua Striker 2002 (GDT-0002)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, gekpurya, triforce, naomigd,    naomi,    0, ROT0, "Sega",           "Gekitou Pro Yakyuu Mizushima Shinji All Stars vs. Pro Yakyuu (Rev C) (GDT-0008C)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 200?, tfupdate, triforce, naomigd,    naomi,    0, ROT0, "Sega",           "Triforce DIMM Updater (GDT-0011)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, vs4,      triforce, naomigd,    naomi,    0, ROT0, "Sega",           "Virtua Striker 4 (GDT-0015)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, avalon13, triforce, naomigd,    naomi,    0, ROT0, "Sega",           "The Key Of Avalon 1.3 - Chaotic Sabbat - Client (GDT-0010C) (V4.000)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2004, avalon20, triforce, naomigd,    naomi,    0, ROT0, "Sega",           "The Key Of Avalon 2.0 - Eutaxy and Commandment - Client (GDT-0017B) (V3.001)", GAME_NO_SOUND|GAME_NOT_WORKING )


/*********************************************************************************************************************
**********************************************************************************************************************
*********************************************************************************************************************/

// Chihiro -- this is XBox based, not Dreamcast/Naomi based, move out of here
// (currently here for testing the gd-rom stuff only)
#define CHIHIRO_BIOS \
	ROM_REGION( 0x200000, "maincpu", 0) \
	ROM_SYSTEM_BIOS( 0, "bios0", "Chihiro Bios" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0,  "chihiro_bios.bin", 0x000000, 0x200000, NO_DUMP ) \

ROM_START( chihiro )
	CHIHIRO_BIOS

	ROM_REGION( 0x8400000, "user1", ROMREGION_ERASE)
ROM_END

/*

Title   GHOST SQUAD
Media ID    004F
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDX-0012A
Version V2.000
Release Date    20041209
Manufacturer ID

PIC
253-5508-0398
317-0398-COM

*/

ROM_START( ghostsqu )
	CHIHIRO_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdx-0012a", 0,  SHA1(d7d78ce4992cb16ee5b4ac6ca7a37c46b07e8c14) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0398-com.data", 0x00, 0x50, CRC(8c5391a2) SHA1(e64cadeb30c94c3cd4002630cd79cc76c7bde2ed) )
ROM_END

/*

Title   VIRTUA COP 3
Media ID    C4AD
Media Config    GD-ROM1/1
Regions J
Peripheral String   0000000
Product Number  GDX-0003A
Version V2.004
Release Date    20030226
Manufacturer ID
TOC DISC
Track   Start Sector    End Sector  Track Size
track01.bin 150 599 1058400
track02.raw 750 2101    3179904
track03.bin 45150   549299  1185760800


PIC
255-5508-354
317-054-COM

*/

ROM_START( vcop3 )
	CHIHIRO_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdx-0003a", 0,  SHA1(cdfec1d2ef02ae9e29cb1462f08904177bc4c9ea) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("317-0354-com.data", 0x00, 0x50,  CRC(df7e3217) SHA1(9f0f4bf6b15f3b6eeea81eaa27b3d25bd94110da) )
ROM_END

ROM_START( mj2 )
	CHIHIRO_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdx-0006c", 0, SHA1(505653117a73ed8b256ccf19450e7573a4dc57e9) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE) // key was missing
	ROM_LOAD("gdx-0006c.pic_data", 0x00, 0x50, NO_DUMP )
ROM_END


ROM_START( wangmid )
	CHIHIRO_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdx-0009b", 0, SHA1(e032b9fd8d5d09255592f02f7531a608e8179c9c) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdx-0009b.data", 0x00, 0x50, CRC(3af801f3) SHA1(e9a2558930f3f1f55d5b3c2cadad69329d931f26) )
ROM_END

ROM_START( wangmid2 )
	CHIHIRO_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdx-0015", 0, SHA1(259483fd211a70c23205ffd852316d616c5a2740) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE)
	ROM_LOAD("gdx-0015.data", 0x00, 0x50, CRC(75c716aa) SHA1(5c2bcf3d28a80b336c6882d5aeb010d04327f8c1) )
ROM_END

ROM_START( mj3 )
	CHIHIRO_BIOS

	ROM_REGION( 0x20000000, "user1", ROMREGION_ERASE) // allocate max size in init instead?

	DISK_REGION( "gdrom" )
	DISK_IMAGE_READONLY( "gdx-0017d", 0, SHA1(cfbbd452c8f4efe0e99f398f5521fc3574b913bb) )

	ROM_REGION( 0x50, "picreturn", ROMREGION_ERASE) // key was missing
	ROM_LOAD("gdx-0017d.pic_data", 0x00, 0x50, NO_DUMP )
ROM_END


GAME( 200?, chihiro,  0,       naomigd,    naomi,    0, ROT0, "Sega",           "Chihiro Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )
GAME( 2005, mj2,      chihiro, naomigd,    naomi,    0, ROT0, "Sega",           "Sega Network Taisen Mahjong MJ 2 (Rev C) [GDX-0006C]", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, wangmid,  chihiro, naomigd,    naomi,    0, ROT0, "Sega",           "Wangan Midnight Maximum Tune (Rev. B) (Export) (GDX-0009B)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, wangmid2, chihiro, naomigd,    naomi,    0, ROT0, "Sega",           "Wangan Midnight Maximum Tune 2 (Japan?) (GDX-0015)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, ghostsqu, chihiro, naomigd,    naomi,    0, ROT0, "Sega",           "Ghost Squad (Ver. A?) (GDX-0012A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, vcop3,    chihiro, naomigd,    naomi,    0, ROT0, "Sega",           "Virtua Cop 3 (GDX-0003A)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2005, mj3,      chihiro, naomigd,    naomi,    0, ROT0, "Sega",           "Sega Network Taisen Mahjong MJ 3 (Rev D) [GDX-0017D]", GAME_NO_SOUND|GAME_NOT_WORKING )


