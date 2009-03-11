###########################################################################
#
#   mess.mak
#
#   MESS target makefile
#
###########################################################################



###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################

# uncomment next line to build imgtool
BUILD_IMGTOOL = 1

# uncomment next line to build wimgtool
BUILD_WIMGTOOL = 1

# uncomment next line to build messtest
BUILD_MESSTEST = 1

# uncomment next line to build dat2html
BUILD_DAT2HTML = 1


###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################



# include MESS core defines
include $(SRC)/mess/messcore.mak


#-------------------------------------------------
# specify available CPU cores; some of these are
# only for MAME and so aren't included
#-------------------------------------------------

CPUS += Z80
#CPUS += Z180
CPUS += I8085
CPUS += M6502
CPUS += M65C02
CPUS += M65SC02
#CPUS += M65CE02
CPUS += M6509
CPUS += M6510
CPUS += M6510T
CPUS += M7501
CPUS += M8502
CPUS += N2A03
#CPUS += DECO16
CPUS += M4510
CPUS += H6280
CPUS += I8086
CPUS += I8088
CPUS += I80186
CPUS += I80188
CPUS += I80286
CPUS += I386
CPUS += I486
CPUS += PENTIUM
#CPUS += MEDIAGX
CPUS += V20
#CPUS += V25
#CPUS += V30
CPUS += V30MZ
#CPUS += V33
#CPUS += V35
#CPUS += V60
#CPUS += V70
CPUS += MCS48
#CPUS += I8031
#CPUS += I8032
CPUS += I8051
#CPUS += I8052
#CPUS += I8751
#CPUS += I8752
#CPUS += I80C31
#CPUS += I80C32
#CPUS += I80C51
#CPUS += I80C52
#CPUS += I87C51
#CPUS += I87C52
#CPUS += AT89C4051
#CPUS += DS5002FP
CPUS += M6800
#CPUS += M6801
#CPUS += M6802
CPUS += M6803
#CPUS += M6808
CPUS += HD63701
#CPUS += NSC8105
CPUS += M6805
#CPUS += M68705
#CPUS += HD63705
CPUS += HD6309
CPUS += M6809
CPUS += M6809E
#CPUS += KONAMI
CPUS += M680X0
CPUS += T11
CPUS += S2650
#CPUS += TMS340X0
CPUS += TMS9900
#CPUS += TMS9940
CPUS += TMS9980
#CPUS += TMS9985
#CPUS += TMS9989
CPUS += TMS9995
#CPUS += TMS99105A
#CPUS += TMS99110A
#CPUS += TMS99000
CPUS += TMS99010
#CPUS += Z8000
#CPUS += TMS32010
#CPUS += TMS32025
#CPUS += TMS32026
#CPUS += TMS32031
#CPUS += TMS32032
#CPUS += TMS32051
#CPUS += TMS57002
#CPUS += CCPU
#CPUS += ADSP21XX
CPUS += PSXCPU
#CPUS += CXD8661R
#CPUS += ASAP
CPUS += UPD7810
#CPUS += UPD7807
CPUS += UPD7801
CPUS += ARM
CPUS += ARM7
CPUS += JAGUAR
#CPUS += CUBEQCPU
#CPUS += ESRIP
CPUS += R3000
#CPUS += R3041
CPUS += R4600
#CPUS += R4650
#CPUS += R4700
CPUS += R5000
#CPUS += QED5271
#CPUS += RM7000
#CPUS += SH1
CPUS += SH2
CPUS += SH4
#CPUS += DSP32C
#CPUS += PIC16C54
#CPUS += PIC16C55
#CPUS += PIC16C56
#CPUS += PIC16C57
#CPUS += PIC16C58
CPUS += G65816
CPUS += SPC700
#CPUS += E116T
#CPUS += E116XT
#CPUS += E116XS
#CPUS += E116XSR
#CPUS += E132N
#CPUS += E132T
#CPUS += E132XN
#CPUS += E132XT
#CPUS += E132XS
#CPUS += E132XSR
#CPUS += GMS30C2116
#CPUS += GMS30C2132
#CPUS += GMS30C2216
#CPUS += GMS30C2232
#CPUS += I860
#CPUS += I960
#CPUS += H83002
#CPUS += H83334
CPUS += V810
#CPUS += M37702
#CPUS += M37710
CPUS += POWERPC
#CPUS += SE3208
#CPUS += MC68HC11
#CPUS += ADSP21062
#CPUS += DSP56156
CPUS += RSP
#CPUS += ALPHA8201
#CPUS += ALPHA8301
CPUS += CDP1802
CPUS += COP400
#CPUS += TLCS90
#CPUS += MB8841
#CPUS += MB8842
#CPUS += MB8843
#CPUS += MB8844
#CPUS += MB86233
CPUS += SSP1601
CPUS += APEXC
CPUS += CP1610
CPUS += F8
#CPUS += LH5801
CPUS += PDP1
CPUS += SATURN
CPUS += SC61860
CPUS += TX0
CPUS += LR35902
CPUS += TMS7000
CPUS += TMS7000_EXL
CPUS += SM8500
CPUS += MINX
CPUS += I8035
#CPUS += I8041
CPUS += I8048
#CPUS += I8648
CPUS += I8748
#CPUS += MB8884
CPUS += I8039
CPUS += I8049
CPUS += I8749
#CPUS += N7751
#CPUS += M58715
#CPUS += I8741
CPUS += I8042
#CPUS += I8242
#CPUS += I8742
#CPUS += PPC403GA
#CPUS += PPC403GCX
#CPUS += PPC601
CPUS += PPC602
CPUS += PPC603
CPUS += PPC603E
#CPUS += PPC603R
#CPUS += PPC604
#CPUS += MPC8240
CPUS += 8080
CPUS += 8085A
#CPUS += ADSP2100
#CPUS += ADSP2101
#CPUS += ADSP2104
#CPUS += ADSP2105
#CPUS += ADSP2115
#CPUS += ADSP2181
CPUS += I8X41
#CPUS += M68000
#CPUS += M68008
#CPUS += M68010
#CPUS += M68EC020
#CPUS += M68020
#CPUS += M68040
#CPUS += TMS34010
#CPUS += TMS34020



#-------------------------------------------------
# specify available sound cores; some of these are
# only for MAME and so aren't included
#-------------------------------------------------

#SOUNDS += CUSTOM
#SOUNDS += SAMPLES
SOUNDS += DAC
SOUNDS += DMADAC
SOUNDS += SPEAKER
SOUNDS += BEEP
SOUNDS += DISCRETE
SOUNDS += AY8910
SOUNDS += YM2151
SOUNDS += YM2203
SOUNDS += YM2413
#SOUNDS += YM2608
#SOUNDS += YM2610
#SOUNDS += YM2610B
SOUNDS += YM2612
#SOUNDS += YM3438
SOUNDS += YM3812
#SOUNDS += YM3526
#SOUNDS += Y8950
#SOUNDS += YMF262
#SOUNDS += YMF271
#SOUNDS += YMF278B
#SOUNDS += YMZ280B
SOUNDS += SN76477
SOUNDS += SN76496
SOUNDS += POKEY
SOUNDS += TIA
SOUNDS += NES
SOUNDS += ASTROCADE
#SOUNDS += NAMCO
#SOUNDS += NAMCO_15XX
#SOUNDS += NAMCO_CUS30
#SOUNDS += NAMCO_52XX
#SOUNDS += NAMCO_63701X
#SOUNDS += SNKWAVE
#SOUNDS += C140
#SOUNDS += C352
#SOUNDS += TMS36XX
#SOUNDS += TMS3615
#SOUNDS += TMS5100
#SOUNDS += TMS5110
#SOUNDS += TMS5110A
#SOUNDS += CD2801
#SOUNDS += TMC0281
#SOUNDS += CD2802
#SOUNDS += M58817
SOUNDS += TMC0285
SOUNDS += TMS5200
SOUNDS += TMS5220
#SOUNDS += VLM5030
#SOUNDS += ADPCM
SOUNDS += MSM5205
#SOUNDS += MSM5232
SOUNDS += OKIM6258
SOUNDS += OKIM6295
#SOUNDS += OKIM6376
#SOUNDS += UPD7759
#SOUNDS += HC55516
#SOUNDS += K005289
#SOUNDS += K007232
SOUNDS += K051649
#SOUNDS += K053260
#SOUNDS += K054539
#SOUNDS += SEGAPCM
#SOUNDS += MULTIPCM
SOUNDS += SCSP
SOUNDS += AICA
#SOUNDS += RF5C68
#SOUNDS += RF5C400
#SOUNDS += CEM3394
SOUNDS += QSOUND
SOUNDS += SAA1099
#SOUNDS += IREMGA20
SOUNDS += ES5503
#SOUNDS += ES5505
#SOUNDS += ES5506
#SOUNDS += BSMT2000
#SOUNDS += GAELCO_CG1V
#SOUNDS += GAELCO_GAE1
SOUNDS += C6280
#SOUNDS += SP0250
SOUNDS += PSXSPU
SOUNDS += CDDA
#SOUNDS += ICS2115
#SOUNDS += ST0016
#SOUNDS += NILE
#SOUNDS += X1_010
#SOUNDS += VRENDER0
#SOUNDS += VOTRAX
#SOUNDS += ES8712
SOUNDS += CDP1869
#SOUNDS += S14001A
SOUNDS += WAVE
SOUNDS += SID6581
SOUNDS += SID8580
SOUNDS += SP0256
#SOUNDS += DIGITALKER


#-------------------------------------------------
# this is the list of driver libraries that
# comprise MESS plus messdriv.o which contains
# the list of drivers
#-------------------------------------------------

DRVLIBS = \
	$(MESSOBJ)/messdriv.o \
	$(MESSOBJ)/3do.a \
	$(MESSOBJ)/ac1.a \
	$(MESSOBJ)/acorn.a \
	$(MESSOBJ)/advision.a \
	$(MESSOBJ)/amiga.a \
	$(MESSOBJ)/amstrad.a \
	$(MESSOBJ)/apexc.a \
	$(MESSOBJ)/apf.a \
	$(MESSOBJ)/apple.a \
	$(MESSOBJ)/aquarius.a \
	$(MESSOBJ)/arcadia.a \
	$(MESSOBJ)/ascii.a \
	$(MESSOBJ)/at.a \
	$(MESSOBJ)/atari.a \
	$(MESSOBJ)/avigo.a \
	$(MESSOBJ)/b2m.a \
	$(MESSOBJ)/bally.a \
	$(MESSOBJ)/bandai.a \
	$(MESSOBJ)/be.a \
	$(MESSOBJ)/bk.a \
	$(MESSOBJ)/bondwell.a \
	$(MESSOBJ)/cbm.a \
	$(MESSOBJ)/cbmshare.a \
	$(MESSOBJ)/cgenie.a \
	$(MESSOBJ)/coco.a \
	$(MESSOBJ)/coleco.a \
	$(MESSOBJ)/compis.a \
	$(MESSOBJ)/comx.a \
	$(MESSOBJ)/concept.a \
	$(MESSOBJ)/cpschngr.a \
	$(MESSOBJ)/cybiko.a \
	$(MESSOBJ)/dai.a \
	$(MESSOBJ)/dgn_beta.a \
	$(MESSOBJ)/einis.a \
	$(MESSOBJ)/enterp.a \
	$(MESSOBJ)/epoch.a \
	$(MESSOBJ)/epson.a \
	$(MESSOBJ)/exeltel.a \
	$(MESSOBJ)/exidy.a \
	$(MESSOBJ)/fairch.a \
	$(MESSOBJ)/galaxy.a \
	$(MESSOBJ)/gce.a \
	$(MESSOBJ)/glasgow.a \
	$(MESSOBJ)/gmaster.a \
	$(MESSOBJ)/grundy.a \
	$(MESSOBJ)/homelab.a \
	$(MESSOBJ)/hp48.a \
	$(MESSOBJ)/intv.a \
	$(MESSOBJ)/irisha.a \
	$(MESSOBJ)/jupiter.a \
	$(MESSOBJ)/kaypro.a \
	$(MESSOBJ)/kim1.a \
	$(MESSOBJ)/kramermc.a \
	$(MESSOBJ)/luxor.a \
	$(MESSOBJ)/lviv.a \
	$(MESSOBJ)/lynx.a \
	$(MESSOBJ)/magnavox.a \
	$(MESSOBJ)/mbee.a \
	$(MESSOBJ)/mc10.a \
	$(MESSOBJ)/memotech.a \
	$(MESSOBJ)/mephisto.a \
	$(MESSOBJ)/mikro80.a \
	$(MESSOBJ)/mk1.a \
	$(MESSOBJ)/mk2.a \
	$(MESSOBJ)/motorola.a \
	$(MESSOBJ)/multitch.a \
	$(MESSOBJ)/nascom1.a \
	$(MESSOBJ)/nec.a \
	$(MESSOBJ)/necpc.a \
	$(MESSOBJ)/nintendo.a \
	$(MESSOBJ)/ondra.a \
	$(MESSOBJ)/orion.a \
	$(MESSOBJ)/osborne.a \
	$(MESSOBJ)/osi.a \
	$(MESSOBJ)/p2000.a \
	$(MESSOBJ)/palm.a \
	$(MESSOBJ)/pasogo.a \
	$(MESSOBJ)/pc.a \
	$(MESSOBJ)/pcshare.a \
	$(MESSOBJ)/pdp1.a \
	$(MESSOBJ)/pel.a \
	$(MESSOBJ)/pk8020.a \
	$(MESSOBJ)/pmd85.a \
	$(MESSOBJ)/pp01.a \
	$(MESSOBJ)/primo.a \
	$(MESSOBJ)/radio.a \
	$(MESSOBJ)/rca.a \
	$(MESSOBJ)/rockwell.a \
	$(MESSOBJ)/rt1715.a \
	$(MESSOBJ)/samcoupe.a \
	$(MESSOBJ)/sapi1.a \
	$(MESSOBJ)/sega.a \
	$(MESSOBJ)/sgi.a \
	$(MESSOBJ)/sharp.a \
	$(MESSOBJ)/sinclair.a \
	$(MESSOBJ)/sony.a \
	$(MESSOBJ)/sord.a \
	$(MESSOBJ)/special.a \
	$(MESSOBJ)/ssystem3.a \
	$(MESSOBJ)/super80.a \
	$(MESSOBJ)/svi.a \
	$(MESSOBJ)/svision.a \
	$(MESSOBJ)/synertec.a \
	$(MESSOBJ)/tangerin.a \
	$(MESSOBJ)/tatung.a \
	$(MESSOBJ)/teamconc.a \
	$(MESSOBJ)/telmac.a \
	$(MESSOBJ)/thomson.a \
	$(MESSOBJ)/ti85.a \
	$(MESSOBJ)/ti99.a \
	$(MESSOBJ)/tiger.a \
	$(MESSOBJ)/trs80.a \
	$(MESSOBJ)/tutor.a \
	$(MESSOBJ)/tx0.a \
	$(MESSOBJ)/ut88.a \
	$(MESSOBJ)/vc4000.a \
	$(MESSOBJ)/veb.a \
	$(MESSOBJ)/vector06.a \
	$(MESSOBJ)/votrax.a \
	$(MESSOBJ)/vtech.a \
	$(MESSOBJ)/shared.a \



#-------------------------------------------------
# the following files are general components and
# shared across a number of drivers
#-------------------------------------------------

$(MESSOBJ)/shared.a: \
	$(MESS_MACHINE)/6530miot.o	\
	$(MESS_DEVICES)/cartslot.o	\
	$(MESS_DEVICES)/multcart.o	\
	$(MESS_DEVICES)/flopdrv.o	\
	$(MESS_DEVICES)/harddriv.o	\
	$(MESS_DEVICES)/chd_cd.o	\
	$(MESS_FORMATS)/ioprocs.o	\
	$(MESS_FORMATS)/flopimg.o	\
	$(MESS_FORMATS)/cassimg.o	\
	$(MESS_FORMATS)/basicdsk.o	\
	$(MESS_FORMATS)/pc_dsk.o	\
	$(MESS_DEVICES)/mflopimg.o	\
	$(MESS_DEVICES)/cassette.o	\
	$(MESS_DEVICES)/printer.o	\
	$(MESS_DEVICES)/bitbngr.o	\
	$(MESS_DEVICES)/snapquik.o	\
	$(MESS_DEVICES)/basicdsk.o	\
	$(MESS_DEVICES)/dsk.o		\
	$(MESS_DEVICES)/microdrv.o	\
	$(MESS_MACHINE)/6551.o		\
	$(MESS_MACHINE)/smartmed.o	\
	$(MESS_VIDEO)/m6847.o		\
	$(MESS_VIDEO)/m6845.o		\
	$(MESS_MACHINE)/msm8251.o  \
	$(MESS_MACHINE)/tc8521.o   \
	$(MESS_MACHINE)/74145.o    \
	$(MESS_MACHINE)/ins8250.o \
	$(MESS_MACHINE)/pc_mouse.o \
	$(MESS_MACHINE)/pc_lpt.o    \
	$(MESS_MACHINE)/ctronics.o \
	$(MAME_MACHINE)/pckeybrd.o \
	$(MESS_MACHINE)/d88.o      \
	$(MESS_MACHINE)/serial.o   \
	$(MESS_FORMATS)/wavfile.o	\
	$(MESS_FORMATS)/coco_cas.o	\
	$(MESS_FORMATS)/coco_dsk.o	\
	$(MESS_MACHINE)/mm58274c.o \
	$(MESS_FORMATS)/rk_cas.o	\
	$(MESS_MACHINE)/nec765.o   \
	$(MESS_MACHINE)/wd17xx.o   \
	$(MESS_MACHINE)/68901mfp.o \
	$(MESS_DEVICES)/z80bin.o	\
	$(MESS_VIDEO)/cdp1864.o		\
	$(MESS_VIDEO)/crtc6845.o	\
	$(MESS_MACHINE)/z80dart.o	\
	$(MESS_VIDEO)/msm6255.o	\
	$(MESS_MACHINE)/8530scc.o		\



#-------------------------------------------------
# manufacturer-specific groupings for drivers
#-------------------------------------------------

$(MESSOBJ)/neocd.a:						\
	$(MESS_DRIVERS)/neocd.o		\
	$(EMU_MACHINE)/neogeo.o			\
	$(EMU_VIDEO)/neogeo.o			\
	$(EMU_MACHINE)/pd4990a.o		\

$(MESSOBJ)/coleco.a:   \
	$(MESS_DRIVERS)/coleco.o	\
	$(MESS_MACHINE)/adam.o		\
	$(MESS_DRIVERS)/adam.o		\
	$(MESS_FORMATS)/adam_dsk.o	\

$(MESSOBJ)/arcadia.a:  \
	$(MESS_DRIVERS)/arcadia.o	\
	$(MESS_AUDIO)/arcadia.o	\
	$(MESS_VIDEO)/arcadia.o	\

$(MESSOBJ)/sega.a:			\
	$(MESS_DRIVERS)/genesis.o	\
	$(MESS_MACHINE)/genesis.o	\
	$(MESS_DRIVERS)/saturn.o	\
	$(MAME_MACHINE)/stvcd.o		\
	$(MAME_MACHINE)/scudsp.o	\
	$(MAME_VIDEO)/stvvdp1.o		\
	$(MAME_VIDEO)/stvvdp2.o		\
	$(MESS_VIDEO)/smsvdp.o		\
	$(MESS_MACHINE)/sms.o		\
	$(MESS_DRIVERS)/sms.o		\
	$(MAME_DRIVERS)/megadriv.o  	\
	$(MESS_DRIVERS)/sg1000.o	\
	$(MESS_DRIVERS)/dc.o		\
	$(MAME_MACHINE)/dc.o 		\
	$(MAME_MACHINE)/naomibd.o	\
	$(MAME_MACHINE)/gdcrypt.o	\
	$(MAME_VIDEO)/dc.o		\

$(MESSOBJ)/atari.a:			\
	$(MAME_VIDEO)/tia.o		\
	$(MAME_MACHINE)/atari.o		\
	$(MAME_VIDEO)/atari.o		\
	$(MAME_VIDEO)/antic.o		\
	$(MAME_VIDEO)/gtia.o		\
	$(MESS_MACHINE)/atarifdc.o	\
	$(MESS_DRIVERS)/atari.o		\
	$(MESS_MACHINE)/a7800.o		\
	$(MESS_DRIVERS)/a7800.o		\
	$(MESS_VIDEO)/a7800.o		\
	$(MESS_DRIVERS)/a2600.o		\
	$(MESS_DRIVERS)/jaguar.o	\
	$(MAME_AUDIO)/jaguar.o		\
	$(MAME_VIDEO)/jaguar.o		\
	$(MESS_FORMATS)/a26_cas.o	\
	$(MESS_DRIVERS)/atarist.o 	\
	$(MESS_VIDEO)/atarist.o 	\
	$(MESS_AUDIO)/lmc1992.o		\

$(MESSOBJ)/gce.a:	           	\
	$(MESS_DRIVERS)/vectrex.o	\
	$(MESS_VIDEO)/vectrex.o		\
	$(MESS_MACHINE)/vectrex.o	\

$(MESSOBJ)/nintendo.a:			\
	$(MESS_AUDIO)/gb.o		\
	$(MESS_VIDEO)/gb.o		\
	$(MESS_MACHINE)/gb.o		\
	$(MESS_DRIVERS)/gb.o		\
	$(MESS_MACHINE)/nes_mmc.o	\
	$(MAME_VIDEO)/ppu2c0x.o		\
	$(MESS_VIDEO)/nes.o		\
	$(MESS_MACHINE)/nes.o		\
	$(MESS_DRIVERS)/nes.o		\
	$(MAME_AUDIO)/snes.o		\
	$(MAME_MACHINE)/snes.o		\
	$(MAME_VIDEO)/snes.o		\
	$(MESS_MACHINE)/snescart.o	\
	$(MESS_DRIVERS)/snes.o	 	\
	$(MESS_DRIVERS)/n64.o		\
	$(MAME_MACHINE)/n64.o		\
	$(MAME_VIDEO)/n64.o		\
	$(MESS_MACHINE)/pokemini.o	\
	$(MESS_DRIVERS)/pokemini.o	\

$(MESSOBJ)/amiga.a: \
	$(MAME_VIDEO)/amiga.o		\
	$(MAME_MACHINE)/amiga.o		\
	$(MAME_AUDIO)/amiga.o		\
	$(MESS_MACHINE)/amigafdc.o	\
	$(MESS_MACHINE)/amigacrt.o	\
	$(MESS_MACHINE)/amigacd.o	\
	$(MESS_MACHINE)/matsucd.o	\
	$(MESS_MACHINE)/amigakbd.o	\
	$(MESS_DRIVERS)/amiga.o		\

$(MESSOBJ)/cbmshare.a: \
	$(MESS_FORMATS)/cbm_tap.o 	\
	$(MESS_MACHINE)/6525tpi.o	\
	$(MESS_MACHINE)/cbm.o		\
	$(MESS_MACHINE)/cbmdrive.o	\
	$(MESS_MACHINE)/vc1541.o	\
	$(MESS_MACHINE)/cbmieeeb.o 	\
	$(MESS_MACHINE)/cbmserb.o 	\
	$(MESS_MACHINE)/cbmipt.o   	\
	$(MESS_MACHINE)/c64.o    	\
	$(MESS_MACHINE)/c65.o		\
	$(MESS_MACHINE)/c128.o		\
	$(MESS_VIDEO)/vdc8563.o		\
	$(MESS_VIDEO)/vic6567.o

$(MESSOBJ)/cbm.a: \
	$(MESS_VIDEO)/pet.o		\
	$(MESS_DRIVERS)/pet.o		\
	$(MESS_MACHINE)/pet.o		\
	$(MESS_DRIVERS)/c64.o		\
	$(MESS_MACHINE)/vc20.o		\
	$(MESS_DRIVERS)/vc20.o		\
	$(MESS_AUDIO)/ted7360.o		\
	$(MESS_AUDIO)/t6721.o		\
	$(MESS_MACHINE)/c16.o		\
	$(MESS_DRIVERS)/c16.o		\
	$(MESS_DRIVERS)/cbmb.o		\
	$(MESS_MACHINE)/cbmb.o		\
	$(MESS_VIDEO)/cbmb.o		\
	$(MESS_DRIVERS)/c65.o		\
	$(MESS_DRIVERS)/c128.o		\
	$(MESS_AUDIO)/vic6560.o		\
	$(MESS_VIDEO)/ted7360.o		\
	$(MESS_VIDEO)/vic6560.o  

$(MESSOBJ)/coco.a:   \
	$(MESS_MACHINE)/6883sam.o	\
	$(MESS_MACHINE)/ds1315.o	\
	$(MESS_MACHINE)/coco.o		\
	$(MESS_VIDEO)/coco.o		\
	$(MESS_DRIVERS)/coco.o		\
	$(MESS_VIDEO)/coco3.o		\
	$(MESS_FORMATS)/cocopak.o	\
	$(MESS_DEVICES)/coco_vhd.o	\
	$(MESS_DEVICES)/cococart.o	\
	$(MESS_DEVICES)/coco_fdc.o	\
	$(MESS_DEVICES)/coco_pak.o	\
	$(MESS_DEVICES)/coco_232.o	\
	$(MESS_DEVICES)/orch90.o	\

$(MESSOBJ)/mc10.a:	\
	$(MESS_MACHINE)/mc10.o		\
	$(MESS_DRIVERS)/mc10.o		\

$(MESSOBJ)/dgn_beta.a:	\
	$(MESS_MACHINE)/dgn_beta.o	\
	$(MESS_VIDEO)/dgn_beta.o	\
	$(MESS_DRIVERS)/dgn_beta.o	\

$(MESSOBJ)/trs80.a:    \
	$(MESS_MACHINE)/trs80.o	 \
	$(MESS_VIDEO)/trs80.o	 \
	$(MESS_FORMATS)/trs_dsk.o	\
	$(MESS_FORMATS)/trs_cas.o	\
	$(MESS_DRIVERS)/trs80.o

$(MESSOBJ)/cgenie.a:   \
	$(MESS_DRIVERS)/cgenie.o	\
	$(MESS_VIDEO)/cgenie.o	 \
	$(MESS_AUDIO)/cgenie.o	 \
	$(MESS_MACHINE)/cgenie.o	 \
	$(MESS_FORMATS)/cgen_cas.o

$(MESSOBJ)/pdp1.a:	   \
	$(MESS_VIDEO)/pdp1.o	\
	$(MESS_MACHINE)/pdp1.o	\
	$(MESS_DRIVERS)/pdp1.o	\

$(MESSOBJ)/apexc.a:     \
	$(MESS_DRIVERS)/apexc.o

$(MESSOBJ)/kaypro.a:   \
	$(MESS_DRIVERS)/kaypro.o	\
	$(MESS_MACHINE)/cpm_bios.o	\
	$(MESS_VIDEO)/kaypro.o	 \
	$(MESS_AUDIO)/kaypro.o	 \
	$(MESS_MACHINE)/kaypro.o	 \

$(MESSOBJ)/sinclair.a: \
	$(MESS_VIDEO)/border.o		\
	$(MESS_VIDEO)/spectrum.o		\
	$(MESS_VIDEO)/timex.o		\
	$(MESS_VIDEO)/zx.o		\
	$(MESS_DRIVERS)/zx.o		\
	$(MESS_MACHINE)/zx.o		\
	$(MESS_DRIVERS)/spectrum.o		\
	$(MESS_DRIVERS)/spec128.o		\
	$(MESS_DRIVERS)/timex.o		\
	$(MESS_DRIVERS)/specpls3.o		\
	$(MESS_DRIVERS)/scorpion.o		\
	$(MESS_DRIVERS)/pentagon.o		\
	$(MESS_MACHINE)/spectrum.o		\
	$(MESS_MACHINE)/beta.o		\
	$(MESS_FORMATS)/zx81_p.o		\
	$(MESS_FORMATS)/tzx_cas.o		\
	$(MESS_DRIVERS)/ql.o		\
	$(MESS_VIDEO)/zx8301.o		\
	$(MESS_MACHINE)/zx8302.o		\

$(MESSOBJ)/apple.a:   \
	$(MESS_VIDEO)/apple2.o		\
	$(MESS_MACHINE)/apple2.o		\
	$(MESS_DRIVERS)/apple2.o		\
	$(MESS_VIDEO)/apple2gs.o		\
	$(MESS_MACHINE)/apple2gs.o		\
	$(MESS_DRIVERS)/apple2gs.o		\
	$(MESS_FORMATS)/ap2_dsk.o		\
	$(MESS_FORMATS)/ap_dsk35.o		\
	$(MESS_MACHINE)/ay3600.o		\
	$(MESS_MACHINE)/ap2_slot.o		\
	$(MESS_MACHINE)/ap2_lang.o		\
	$(MESS_MACHINE)/mockngbd.o		\
	$(MESS_MACHINE)/lisa.o			\
	$(MESS_DRIVERS)/lisa.o			\
	$(MESS_MACHINE)/applefdc.o		\
	$(MESS_DEVICES)/sonydriv.o		\
	$(MESS_DEVICES)/appldriv.o		\
	$(MESS_AUDIO)/mac.o			\
	$(MESS_VIDEO)/mac.o			\
	$(MESS_MACHINE)/mac.o			\
	$(MESS_DRIVERS)/mac.o			\
	$(MESS_VIDEO)/apple1.o		\
	$(MESS_MACHINE)/apple1.o		\
	$(MESS_DRIVERS)/apple1.o		\
	$(MESS_VIDEO)/apple3.o		\
	$(MESS_MACHINE)/apple3.o		\
	$(MESS_DRIVERS)/apple3.o		\
	$(MESS_MACHINE)/ncr5380.o

$(MESSOBJ)/avigo.a: \
	$(MESS_VIDEO)/avigo.o		\
	$(MESS_DRIVERS)/avigo.o		\

$(MESSOBJ)/ti85.a: \
	$(MESS_DRIVERS)/ti85.o		\
	$(MESS_FORMATS)/ti85_ser.o	\
	$(MESS_VIDEO)/ti85.o		\
	$(MESS_MACHINE)/ti85.o		\

$(MESSOBJ)/rca.a: \
	$(MESS_DRIVERS)/vip.o  \
	$(MESS_DRIVERS)/studio2.o  \
	$(MESS_VIDEO)/cdp1861.o \
	$(MESS_VIDEO)/cdp1862.o \
	$(MESS_AUDIO)/cdp1863.o

$(MESSOBJ)/fairch.a: \
	$(MESS_VIDEO)/channelf.o \
	$(MESS_AUDIO)/channelf.o \
	$(MESS_DRIVERS)/channelf.o 

$(MESSOBJ)/ti99.a:	   \
	$(MESS_MACHINE)/tms9901.o	\
	$(MESS_MACHINE)/tms9902.o	\
	$(MESS_MACHINE)/ti99_4x.o	\
	$(MESS_MACHINE)/990_hd.o	\
	$(MESS_MACHINE)/990_tap.o	\
	$(MESS_MACHINE)/ti990.o		\
	$(MESS_MACHINE)/994x_ser.o	\
	$(MESS_MACHINE)/at29040.o	\
	$(MESS_MACHINE)/99_dsk.o	\
	$(MESS_MACHINE)/99_ide.o	\
	$(MESS_MACHINE)/99_peb.o	\
	$(MESS_MACHINE)/99_hsgpl.o	\
	$(MESS_MACHINE)/99_usbsm.o	\
	$(MESS_MACHINE)/smc92x4.o	\
	$(MESS_MACHINE)/strata.o	\
	$(MESS_MACHINE)/geneve.o	\
	$(MESS_MACHINE)/990_dk.o	\
	$(MESS_AUDIO)/spchroms.o	\
	$(MESS_DRIVERS)/ti990_4.o	\
	$(MESS_DRIVERS)/ti99_4x.o	\
	$(MESS_DRIVERS)/ti99_4p.o	\
	$(MESS_DRIVERS)/geneve.o	\
	$(MESS_DRIVERS)/tm990189.o	\
	$(MESS_DRIVERS)/ti99_8.o	\
	$(MESS_VIDEO)/911_vdt.o	\
	$(MESS_VIDEO)/733_asr.o	\
	$(MESS_DRIVERS)/ti990_10.o	\
	$(MESS_DRIVERS)/ti99_2.o	\

$(MESSOBJ)/tutor.a:	\
	$(MESS_DRIVERS)/tutor.o

$(MESSOBJ)/bally.a:    \
	$(MAME_VIDEO)/astrocde.o \
	$(MESS_DRIVERS)/astrocde.o

$(MESSOBJ)/pcshare.a:					\
	$(MAME_MACHINE)/pcshare.o	\
	$(MESS_MACHINE)/pc_turbo.o	\
	$(MESS_AUDIO)/sblaster.o	\
	$(MESS_MACHINE)/pc_fdc.o	\
	$(MESS_MACHINE)/pc_hdc.o	\
	$(MESS_MACHINE)/pc_joy.o	\
	$(MESS_MACHINE)/kb_keytro.o	\
	$(MESS_VIDEO)/pc_video.o	\
	$(MESS_VIDEO)/pc_mda.o	\
	$(MESS_VIDEO)/pc_cga.o	\
	$(MESS_VIDEO)/cgapal.o	\
	$(MESS_VIDEO)/pc_vga.o	\
	$(MESS_VIDEO)/crtc_ega.o	\
	$(MESS_VIDEO)/pc_ega.o

$(MESSOBJ)/pc.a:	   \
	$(MESS_VIDEO)/pc_aga.o	 \
	$(MESS_MACHINE)/ibmpc.o	 \
	$(MESS_MACHINE)/tandy1t.o  \
	$(MESS_MACHINE)/amstr_pc.o \
	$(MESS_MACHINE)/europc.o	 \
	$(MESS_MACHINE)/pc.o       \
	$(MESS_DRIVERS)/pc.o		\
	$(MESS_VIDEO)/pc_t1t.o	 

$(MESSOBJ)/at.a:	   \
	$(MESS_MACHINE)/pc_ide.o   \
	$(MESS_MACHINE)/ps2.o	 \
	$(MESS_MACHINE)/at.o       \
	$(MESS_DRIVERS)/at.o	\
	$(MESS_MACHINE)/i82439tx.o

$(MESSOBJ)/p2000.a:    \
	$(MESS_VIDEO)/saa5050.o  \
	$(MESS_VIDEO)/p2000m.o	 \
	$(MESS_DRIVERS)/p2000t.o	 \
	$(MESS_MACHINE)/p2000t.o	 \
	
$(MESSOBJ)/osi.a:    \
	$(MESS_DRIVERS)/osi.o	\
	$(MESS_VIDEO)/osi.o	\

$(MESSOBJ)/amstrad.a:  \
	$(MESS_DRIVERS)/amstrad.o  \
	$(MESS_MACHINE)/amstrad.o  \
	$(MESS_VIDEO)/pcw.o	 \
	$(MESS_DRIVERS)/pcw.o	 \
	$(MESS_DRIVERS)/pcw16.o	 \
	$(MESS_VIDEO)/pcw16.o	 \
	$(MESS_VIDEO)/nc.o	 \
	$(MESS_DRIVERS)/nc.o	 \
	$(MESS_MACHINE)/nc.o	 \

$(MESSOBJ)/veb.a:      \
	$(MESS_VIDEO)/kc.o	\
	$(MESS_DRIVERS)/kc.o	\
	$(MESS_MACHINE)/kc.o	\

$(MESSOBJ)/nec.a:	   \
	$(MAME_VIDEO)/vdc.o	 \
	$(MESS_MACHINE)/pce.o	 \
	$(MESS_DRIVERS)/pce.o	\
	$(MESS_DRIVERS)/pcfx.o

$(MESSOBJ)/necpc.a:	   \
	$(MESS_DRIVERS)/pc8801.o	 \
	$(MESS_MACHINE)/pc8801.o	 \
	$(MESS_VIDEO)/pc8801.o	\

$(MESSOBJ)/enterp.a :   \
	$(MESS_AUDIO)/dave.o	 \
	$(MESS_VIDEO)/epnick.o	 \
	$(MESS_DRIVERS)/enterp.o

$(MESSOBJ)/ascii.a :   \
	$(MESS_FORMATS)/fmsx_cas.o \
	$(MESS_DRIVERS)/msx.o	\
	$(MESS_MACHINE)/msx_slot.o	 \
	$(MESS_MACHINE)/msx.o	 \

$(MESSOBJ)/kim1.a :    \
	$(MESS_DRIVERS)/kim1.o	\
	$(MESS_FORMATS)/kim1_cas.o

$(MESSOBJ)/synertec.a :    \
	$(MESS_MACHINE)/sym1.o	 \
	$(MESS_DRIVERS)/sym1.o

$(MESSOBJ)/rockwell.a :    \
	$(MESS_VIDEO)/dl1416.o \
	$(MESS_VIDEO)/aim65.o \
	$(MESS_MACHINE)/aim65.o	 \
	$(MESS_DRIVERS)/aim65.o

$(MESSOBJ)/vc4000.a :   \
	$(MESS_AUDIO)/vc4000.o	\
	$(MESS_DRIVERS)/vc4000.o	\
	$(MESS_VIDEO)/vc4000.o	\

$(MESSOBJ)/tangerin.a :\
	$(MESS_DEVICES)/mfmdisk.o	\
	$(MESS_VIDEO)/microtan.o	\
	$(MESS_MACHINE)/microtan.o	\
	$(MESS_DRIVERS)/microtan.o	\
	$(MESS_FORMATS)/oric_tap.o	\
	$(MESS_DRIVERS)/oric.o		\
	$(MESS_VIDEO)/oric.o		\
	$(MESS_MACHINE)/oric.o		\

$(MESSOBJ)/vtech.a :   \
	$(MESS_VIDEO)/vtech1.o	\
	$(MESS_MACHINE)/vtech1.o	\
	$(MESS_DRIVERS)/vtech1.o	\
	$(MESS_VIDEO)/vtech2.o	\
	$(MESS_MACHINE)/vtech2.o	\
	$(MESS_DRIVERS)/vtech2.o	\
	$(MESS_FORMATS)/vt_cas.o	\
	$(MESS_FORMATS)/vt_dsk.o	\
	$(MESS_DRIVERS)/crvision.o	\

$(MESSOBJ)/super80.a :   \
	$(MESS_DRIVERS)/super80.o	\
	$(MESS_VIDEO)/super80.o

$(MESSOBJ)/jupiter.a : \
	$(MESS_DRIVERS)/jupiter.o	\
	$(MESS_MACHINE)/jupiter.o	\
	$(MESS_FORMATS)/jupi_tap.o

$(MESSOBJ)/mbee.a :    \
	$(MESS_VIDEO)/mbee.o	 \
	$(MESS_MACHINE)/mbee.o	 \
	$(MESS_DRIVERS)/mbee.o

$(MESSOBJ)/advision.a: \
	$(MESS_VIDEO)/advision.o \
	$(MESS_MACHINE)/advision.o \
	$(MESS_DRIVERS)/advision.o

$(MESSOBJ)/nascom1.a:  \
	$(MESS_VIDEO)/nascom1.o  \
	$(MESS_MACHINE)/nascom1.o  \
	$(MESS_DRIVERS)/nascom1.o

$(MESSOBJ)/cpschngr.a: \
	$(MESS_DRIVERS)/cpschngr.o \
	$(MESS_VIDEO)/cpschngr.o \
	$(MAME_MACHINE)/kabuki.o \

$(MESSOBJ)/memotech.a:	   \
	$(MESS_DRIVERS)/mtx.o \
	$(MESS_MACHINE)/mtx.o

$(MESSOBJ)/acorn.a:    \
	$(MESS_DRIVERS)/acrnsys1.o \
	$(MESS_MACHINE)/ins8154.o \
	$(MESS_MACHINE)/i8271.o	 \
	$(MESS_MACHINE)/upd7002.o  \
	$(MESS_VIDEO)/saa505x.o	     \
	$(MESS_VIDEO)/bbc.o	     \
	$(MESS_MACHINE)/bbc.o	     \
	$(MESS_DRIVERS)/bbc.o	     \
	$(MESS_DRIVERS)/bbcbc.o	     \
	$(MESS_DRIVERS)/a310.o	 \
	$(MAME_MACHINE)/archimds.o \
	$(MESS_DRIVERS)/z88.o	     \
	$(MESS_VIDEO)/z88.o      \
	$(MESS_VIDEO)/atom.o	 \
	$(MESS_DRIVERS)/atom.o	 \
	$(MESS_MACHINE)/atom.o	 \
	$(MESS_FORMATS)/uef_cas.o	\
	$(MESS_FORMATS)/csw_cas.o	\
	$(MESS_VIDEO)/electron.o	\
	$(MESS_MACHINE)/electron.o	\
	$(MESS_DRIVERS)/electron.o

$(MESSOBJ)/samcoupe.a: \
	$(MESS_FORMATS)/coupedsk.o \
	$(MESS_VIDEO)/samcoupe.o	 \
	$(MESS_DRIVERS)/samcoupe.o	\
	$(MESS_MACHINE)/samcoupe.o	 \

$(MESSOBJ)/sharp.a:    \
	$(MESS_VIDEO)/mz700.o		\
	$(MESS_DRIVERS)/mz700.o		\
	$(MESS_FORMATS)/mz_cas.o	\
	$(MESS_DRIVERS)/pocketc.o	\
	$(MESS_VIDEO)/pc1401.o	\
	$(MESS_MACHINE)/pc1401.o	\
	$(MESS_VIDEO)/pc1403.o	\
	$(MESS_MACHINE)/pc1403.o	\
	$(MESS_VIDEO)/pc1350.o	\
	$(MESS_MACHINE)/pc1350.o	\
	$(MESS_VIDEO)/pc1251.o	\
	$(MESS_MACHINE)/pc1251.o	\
	$(MESS_VIDEO)/pocketc.o	\
	$(MESS_MACHINE)/mz700.o		\
	$(MESS_DRIVERS)/x68k.o	\
	$(MESS_VIDEO)/x68k.o	\
	$(MESS_MACHINE)/hd63450.o   \
	$(MESS_MACHINE)/rp5c15.o	\
	$(MESS_MACHINE)/x68k_hdc.o	\
	$(MESS_DRIVERS)/mz80.o		\
	$(MESS_VIDEO)/mz80.o		\
	$(MESS_MACHINE)/mz80.o	\

$(MESSOBJ)/hp48.a:     \
	$(MESS_MACHINE)/hp48.o     \
	$(MESS_VIDEO)/hp48.o     \
	$(MESS_DRIVERS)/hp48.o \
	$(MESS_DEVICES)/xmodem.o \
	$(MESS_DEVICES)/kermit.o \

$(MESSOBJ)/aquarius.a: \
	$(MESS_DRIVERS)/aquarius.o	\
	$(MESS_VIDEO)/aquarius.o \

$(MESSOBJ)/exidy.a:    \
	$(MESS_MACHINE)/ay31015.o   \
	$(MESS_DRIVERS)/exidy.o		\
	$(MESS_VIDEO)/exidy.o      \

$(MESSOBJ)/galaxy.a:   \
	$(MESS_VIDEO)/galaxy.o   \
	$(MESS_DRIVERS)/galaxy.o	\
	$(MESS_MACHINE)/galaxy.o   \
	$(MESS_FORMATS)/gtp_cas.o	\

$(MESSOBJ)/lviv.a:   \
	$(MESS_VIDEO)/lviv.o   \
	$(MESS_DRIVERS)/lviv.o   \
	$(MESS_MACHINE)/lviv.o   \
	$(MESS_FORMATS)/lviv_lvt.o

$(MESSOBJ)/pmd85.a:   \
	$(MESS_VIDEO)/pmd85.o   \
	$(MESS_DRIVERS)/pmd85.o   \
	$(MESS_MACHINE)/pmd85.o   \
	$(MESS_FORMATS)/pmd_pmd.o

$(MESSOBJ)/magnavox.a: \
	$(MESS_MACHINE)/odyssey2.o \
	$(MESS_VIDEO)/odyssey2.o \
	$(MESS_DRIVERS)/odyssey2.o

$(MESSOBJ)/teamconc.a: \
	$(MESS_VIDEO)/comquest.o \
	$(MESS_DRIVERS)/comquest.o

$(MESSOBJ)/svision.a:  \
	$(MESS_DRIVERS)/svision.o \
	$(MESS_AUDIO)/svision.o

$(MESSOBJ)/lynx.a:     \
	$(MESS_DRIVERS)/lynx.o     \
	$(MESS_AUDIO)/lynx.o     \
	$(MESS_MACHINE)/lynx.o

$(MESSOBJ)/mk1.a:      \
	$(MESS_DRIVERS)/mk1.o

$(MESSOBJ)/mk2.a:      \
	$(MESS_DRIVERS)/mk2.o

$(MESSOBJ)/mephisto.a:      \
	$(MESS_DRIVERS)/mephisto.o

$(MESSOBJ)/glasgow.a:      \
	$(MESS_DRIVERS)/glasgow.o

$(MESSOBJ)/ssystem3.a: \
	$(MESS_VIDEO)/ssystem3.o \
	$(MESS_DRIVERS)/ssystem3.o

$(MESSOBJ)/motorola.a: \
	$(MESS_VIDEO)/mekd2.o    \
	$(MESS_MACHINE)/mekd2.o    \
	$(MESS_DRIVERS)/mekd2.o

$(MESSOBJ)/svi.a:      \
	$(MESS_MACHINE)/svi318.o   \
	$(MESS_DRIVERS)/svi318.o   \
	$(MESS_FORMATS)/svi_cas.o

$(MESSOBJ)/intv.a:     \
	$(MESS_VIDEO)/intv.o	\
	$(MESS_VIDEO)/stic.o	\
	$(MESS_MACHINE)/intv.o	\
	$(MESS_AUDIO)/intv.o	\
	$(MESS_DRIVERS)/intv.o

$(MESSOBJ)/apf.a:      \
	$(MESS_DRIVERS)/apf.o	\
	$(MESS_MACHINE)/apf.o	\
	$(MESS_VIDEO)/apf.o   \
	$(MESS_FORMATS)/apf_apt.o

$(MESSOBJ)/sord.a:     \
	$(MESS_DRIVERS)/sord.o	\
	$(MESS_FORMATS)/sord_cas.o

$(MESSOBJ)/tatung.a:     \
	$(MESS_DRIVERS)/einstein.o

$(MESSOBJ)/sony.a:     \
	$(MESS_DRIVERS)/psx.o	\
	$(MAME_MACHINE)/psx.o	\
	$(MAME_VIDEO)/psx.o

$(MESSOBJ)/dai.a:     \
	$(MESS_DRIVERS)/dai.o     \
	$(MESS_VIDEO)/dai.o     \
	$(MESS_AUDIO)/dai.o     \
	$(MESS_MACHINE)/tms5501.o \
	$(MESS_MACHINE)/dai.o     \

$(MESSOBJ)/concept.a:  \
	$(MESS_DRIVERS)/concept.o   \
	$(MESS_MACHINE)/concept.o	\
	$(MESS_MACHINE)/corvushd.o

$(MESSOBJ)/bandai.a:     \
	$(MESS_DRIVERS)/wswan.o   \
	$(MESS_MACHINE)/wswan.o   \
	$(MESS_VIDEO)/wswan.o   \
	$(MESS_AUDIO)/wswan.o

$(MESSOBJ)/compis.a:					\
	$(MESS_DRIVERS)/compis.o	\
	$(MESS_MACHINE)/compis.o	\
	$(MESS_FORMATS)/cpis_dsk.o	\
	$(MESS_VIDEO)/i82720.o 

$(MESSOBJ)/multitch.a:					\
	$(MESS_DRIVERS)/mpf1.o		\

$(MESSOBJ)/telmac.a:					\
	$(MESS_DRIVERS)/tmc1800.o	\
	$(MESS_VIDEO)/tmc1800.o	\
	$(MESS_DRIVERS)/tmc600.o	\
	$(MESS_VIDEO)/tmc600.o	\
	$(MESS_DRIVERS)/tmc2000e.o	\

$(MESSOBJ)/exeltel.a:					\
	$(MESS_DRIVERS)/exelv.o		\
	$(MESS_VIDEO)/tms3556.o		\

$(MESSOBJ)/tx0.a:				\
	$(MESS_VIDEO)/crt.o	\
	$(MESS_DRIVERS)/tx0.o	\
	$(MESS_MACHINE)/tx0.o	\
	$(MESS_VIDEO)/tx0.o	\

$(MESSOBJ)/luxor.a:					\
	$(MESS_DRIVERS)/abc80.o	\
	$(MESS_VIDEO)/abc80.o	\
	$(MESS_DRIVERS)/abc80x.o	\
	$(MESS_VIDEO)/abc800.o	\
	$(MESS_VIDEO)/abc802.o	\
	$(MESS_VIDEO)/abc806.o	\
	$(MESS_MACHINE)/abcbus.o	\
	$(MESS_MACHINE)/abc77.o	\
	$(MESS_MACHINE)/abc99.o	\
	$(MESS_MACHINE)/e0516.o	\
	$(MESS_MACHINE)/conkort.o \

$(MESSOBJ)/palm.a:	\
	$(MESS_DRIVERS)/palm.o		\
	$(MESS_MACHINE)/mc68328.o	\
	$(MESS_VIDEO)/mc68328.o		\

$(MESSOBJ)/sgi.a:						\
	$(MESS_MACHINE)/sgi.o		\
	$(MESS_DRIVERS)/ip20.o		\
	$(MESS_DRIVERS)/ip22.o	\
	$(MESS_VIDEO)/newport.o

$(MESSOBJ)/primo.a:				\
	$(MESS_DRIVERS)/primo.o	\
	$(MESS_MACHINE)/primo.o	\
	$(MESS_VIDEO)/primo.o	\
	$(MESS_FORMATS)/primoptp.o

$(MESSOBJ)/be.a:						\
	$(MESS_DRIVERS)/bebox.o		\
	$(MESS_MACHINE)/bebox.o		\
	$(MESS_MACHINE)/mpc105.o	\
	$(MESS_VIDEO)/cirrus.o

$(MESSOBJ)/thomson.a:			\
	$(MESS_MACHINE)/mc6843.o    \
	$(MESS_MACHINE)/mc6846.o	\
	$(MESS_MACHINE)/mc6854.o    \
	$(MESS_DRIVERS)/thomson.o   \
	$(MESS_MACHINE)/thomson.o   \
	$(MESS_VIDEO)/thomson.o   \
	$(MESS_DEVICES)/thomflop.o \
	$(MESS_FORMATS)/thom_dsk.o \
	$(MESS_FORMATS)/thom_cas.o \
	$(MESS_AUDIO)/mea8000.o

$(MESSOBJ)/tiger.a:				\
	$(MESS_DRIVERS)/gamecom.o	\
	$(MESS_MACHINE)/gamecom.o	\
	$(MESS_VIDEO)/gamecom.o

$(MESSOBJ)/3do.a:			\
	$(MESS_DRIVERS)/3do.o	\
	$(MESS_MACHINE)/3do.o

$(MESSOBJ)/cybiko.a:			\
	$(MESS_DRIVERS)/cybiko.o	\
	$(MESS_MACHINE)/cybiko.o	\
	$(MESS_MACHINE)/pcf8593.o	\
	$(MESS_VIDEO)/hd66421.o		\
	$(MESS_MACHINE)/at45dbxx.o	\
	$(MESS_MACHINE)/sst39vfx.o

$(MESSOBJ)/osborne.a:			\
	$(MESS_DRIVERS)/osborne1.o	\
	$(MESS_MACHINE)/osborne1.o	\

$(MESSOBJ)/epson.a:			\
	$(MESS_DRIVERS)/ex800.o	\
	$(MESS_DRIVERS)/px4.o

$(MESSOBJ)/epoch.a:				\
	$(MESS_DRIVERS)/gamepock.o	\
	$(MESS_MACHINE)/gamepock.o	\

$(MESSOBJ)/pel.a:      \
	$(MESS_DRIVERS)/galeb.o \
	$(MESS_MACHINE)/galeb.o \
	$(MESS_VIDEO)/galeb.o \
	$(MESS_FORMATS)/orao_cas.o		\
	$(MESS_DRIVERS)/orao.o \
	$(MESS_MACHINE)/orao.o \
	$(MESS_VIDEO)/orao.o \

$(MESSOBJ)/gmaster.a:			\
	$(MESS_DRIVERS)/gmaster.o	\
	$(MESS_AUDIO)/gmaster.o

$(MESSOBJ)/pasogo.a:			\
	$(MESS_DRIVERS)/pasogo.o	\

$(MESSOBJ)/ut88.a:      \
	$(MESS_DRIVERS)/ut88.o \
	$(MESS_MACHINE)/ut88.o \
	$(MESS_VIDEO)/ut88.o \

$(MESSOBJ)/mikro80.a:      \
	$(MESS_DRIVERS)/mikro80.o \
	$(MESS_MACHINE)/mikro80.o \
	$(MESS_VIDEO)/mikro80.o \

$(MESSOBJ)/special.a:      \
	$(MESS_AUDIO)/special.o \
	$(MESS_DRIVERS)/special.o \
	$(MESS_MACHINE)/special.o \
	$(MESS_VIDEO)/special.o \

$(MESSOBJ)/orion.a:      \
	$(MESS_DRIVERS)/orion.o \
	$(MESS_MACHINE)/orion.o \
	$(MESS_VIDEO)/orion.o \

$(MESSOBJ)/bk.a:      \
	$(MESS_DRIVERS)/bk.o \
	$(MESS_MACHINE)/bk.o \
	$(MESS_VIDEO)/bk.o \

$(MESSOBJ)/b2m.a:      \
	$(MESS_DRIVERS)/b2m.o \
	$(MESS_MACHINE)/b2m.o \
	$(MESS_VIDEO)/b2m.o \

$(MESSOBJ)/radio.a:      \
	$(MESS_DRIVERS)/radio86.o \
	$(MESS_MACHINE)/radio86.o \
	$(MESS_VIDEO)/radio86.o \
	$(MESS_DRIVERS)/apogee.o \
	$(MESS_DRIVERS)/partner.o \
	$(MESS_MACHINE)/partner.o \
	$(MESS_DRIVERS)/mikrosha.o \
	$(MESS_VIDEO)/i8275.o \

$(MESSOBJ)/comx.a:				\
	$(MESS_DRIVERS)/comx35.o	\
	$(MESS_DRIVERS)/comxpl80.o	\
	$(MESS_VIDEO)/comx35.o		\
	$(MESS_MACHINE)/comx35.o	\
	$(MESS_MACHINE)/cdp1871.o	\
	
$(MESSOBJ)/bondwell.a: \
	$(MESS_DRIVERS)/bw2.o \

$(MESSOBJ)/irisha.a:      \
	$(MESS_DRIVERS)/irisha.o \
	$(MESS_MACHINE)/irisha.o \
	$(MESS_VIDEO)/irisha.o \

$(MESSOBJ)/pk8020.a:      \
	$(MESS_DRIVERS)/pk8020.o \
	$(MESS_MACHINE)/pk8020.o \
	$(MESS_VIDEO)/pk8020.o \

$(MESSOBJ)/vector06.a:      \
	$(MESS_DRIVERS)/vector06.o \
	$(MESS_MACHINE)/vector06.o \
	$(MESS_VIDEO)/vector06.o \

$(MESSOBJ)/rt1715.a:      \
	$(MESS_DRIVERS)/rt1715.o \
	$(MESS_MACHINE)/rt1715.o \
	$(MESS_VIDEO)/rt1715.o \

$(MESSOBJ)/homelab.a: \
	$(MESS_DRIVERS)/homelab.o \
	$(MESS_VIDEO)/homelab.o		\
	$(MESS_MACHINE)/homelab.o	\

$(MESSOBJ)/pp01.a:      \
	$(MESS_DRIVERS)/pp01.o \
	$(MESS_MACHINE)/pp01.o \
	$(MESS_VIDEO)/pp01.o \

$(MESSOBJ)/ondra.a:      \
	$(MESS_DRIVERS)/ondra.o \
	$(MESS_MACHINE)/ondra.o \
	$(MESS_VIDEO)/ondra.o \

$(MESSOBJ)/sapi1.a:      \
	$(MESS_DRIVERS)/sapi1.o \
	$(MESS_MACHINE)/sapi1.o \
	$(MESS_VIDEO)/sapi1.o \

$(MESSOBJ)/kramermc.a:      \
	$(MESS_DRIVERS)/kramermc.o \
	$(MESS_MACHINE)/kramermc.o \
	$(MESS_VIDEO)/kramermc.o \

$(MESSOBJ)/einis.a:      \
	$(MESS_DRIVERS)/pecom.o \
	$(MESS_MACHINE)/pecom.o \
	$(MESS_VIDEO)/pecom.o \
	
$(MESSOBJ)/ac1.a:      \
	$(MESS_DRIVERS)/ac1.o \
	$(MESS_MACHINE)/ac1.o \
	$(MESS_VIDEO)/ac1.o \

$(MESSOBJ)/grundy.a: \
	$(MESS_DRIVERS)/newbrain.o \
	$(MESS_VIDEO)/newbrain.o \
	$(MESS_MACHINE)/adc080x.o

$(MESSOBJ)/votrax.a: \
	$(MESS_DRIVERS)/votrpss.o \

#-------------------------------------------------
# layout dependencies
#-------------------------------------------------

$(OBJ)/render.o:	$(MESS_LAYOUT)/horizont.lh \
					$(MESS_LAYOUT)/vertical.lh \

$(MESS_DRIVERS)/acrnsys1.o:	$(MESS_LAYOUT)/acrnsys1.lh
$(MESS_DRIVERS)/aim65.o:	$(MESS_LAYOUT)/aim65.lh
$(MESS_DRIVERS)/bw2.o:		$(MESS_LAYOUT)/bw2.lh
$(MESS_DRIVERS)/coco.o:		$(MESS_LAYOUT)/coco3.lh
$(MESS_DRIVERS)/cybiko.o:	$(MESS_LAYOUT)/cybiko.lh
$(MESS_DRIVERS)/ex800.o:	$(MESS_LAYOUT)/ex800.lh
$(MESS_DRIVERS)/gamecom.o:	$(MESS_LAYOUT)/gamecom.lh
$(MESS_DRIVERS)/gamepock.o:	$(MESS_LAYOUT)/gamepock.lh
$(MESS_DRIVERS)/gb.o:		$(MESS_LAYOUT)/gb.lh
$(MESS_DRIVERS)/glasgow.o:	$(MESS_LAYOUT)/glasgow.lh
$(MESS_DRIVERS)/gmaster.o:	$(MESS_LAYOUT)/gmaster.lh
$(MESS_DRIVERS)/kim1.o:		$(MESS_LAYOUT)/kim1.lh
$(MESS_DRIVERS)/mephisto.o:	$(MESS_LAYOUT)/mephisto.lh
$(MESS_DRIVERS)/mk1.o:		$(MESS_LAYOUT)/mk1.lh
$(MESS_DRIVERS)/mk2.o:		$(MESS_LAYOUT)/mk2.lh
$(MESS_DRIVERS)/mpf1.o:		$(MESS_LAYOUT)/mpf1.lh
$(MESS_DRIVERS)/nc.o:		$(MESS_LAYOUT)/nc200.lh
$(MESS_DRIVERS)/palm.o:		$(MESS_LAYOUT)/palm.lh
$(MESS_DRIVERS)/pokemini.o:	$(MESS_LAYOUT)/pokemini.lh
$(MESS_DRIVERS)/px4.o:		$(MESS_LAYOUT)/px4.lh
$(MESS_DRIVERS)/svi318.o:	$(MESS_LAYOUT)/sv328806.lh
$(MESS_DRIVERS)/svision.o:	$(MESS_LAYOUT)/svision.lh
$(MESS_DRIVERS)/sym1.o:		$(MESS_LAYOUT)/sym1.lh
$(MESS_DRIVERS)/ut88.o:		$(MESS_LAYOUT)/ut88mini.lh
$(MESS_DRIVERS)/votrpss.o:	$(MESS_LAYOUT)/votrpss.lh
$(MESS_DRIVERS)/x68k.o:		$(MESS_LAYOUT)/x68000.lh


#-------------------------------------------------
# MESS-specific tools
#-------------------------------------------------

ifdef BUILD_IMGTOOL
include $(MESSSRC)/tools/imgtool/imgtool.mak
TOOLS += $(IMGTOOL)
endif

ifdef BUILD_MESSTEST
include $(MESSSRC)/tools/messtest/messtest.mak
TOOLS += $(MESSTEST)
endif

ifdef BUILD_DAT2HTML
include $(MESSSRC)/tools/dat2html/dat2html.mak
TOOLS += $(DAT2HTML)
endif

# include OS-specific MESS stuff
ifeq ($(OSD),windows)
include $(MESSSRC)/tools/messdocs/messdocs.mak

ifdef BUILD_WIMGTOOL
include $(MESSSRC)/tools/imgtool/windows/wimgtool.mak
TOOLS += $(WIMGTOOL)
endif
endif



#-------------------------------------------------
# MESS special OSD rules
#-------------------------------------------------

include $(SRC)/mess/osd/$(OSD)/$(OSD).mak