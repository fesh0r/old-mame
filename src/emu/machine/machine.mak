###########################################################################
#
#   machine.mak
#
#   Rules for building machine cores
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


MACHINESRC = $(EMUSRC)/machine
MACHINEOBJ = $(EMUOBJ)/machine


#-------------------------------------------------
#
#@src/emu/machine/40105.h,MACHINES += CMOS40105
#-------------------------------------------------

ifneq ($(filter CMOS40105,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/40105.o
endif


#-------------------------------------------------
#
#@src/emu/machine/53c7xx.h,MACHINES += NCR53C7XX
#-------------------------------------------------

ifneq ($(filter NCR53C7XX,$(MACHINES)),)
MACHINES += NSCSI
MACHINEOBJS += $(MACHINEOBJ)/53c7xx.o
endif

#-------------------------------------------------
#
#@src/emu/machine/53c810.h,MACHINES += LSI53C810
#-------------------------------------------------

ifneq ($(filter LSI53C810,$(MACHINES)),)
MACHINES += SCSI
MACHINEOBJS += $(MACHINEOBJ)/53c810.o
endif

#-------------------------------------------------
#
#@src/emu/machine/6522via.h,MACHINES += 6522VIA
#-------------------------------------------------

ifneq ($(filter 6522VIA,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/6522via.o
endif

#-------------------------------------------------
#
#@src/emu/machine/6525tpi.h,MACHINES += TPI6525
#-------------------------------------------------

ifneq ($(filter TPI6525,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/6525tpi.o
endif

#-------------------------------------------------
#
#@src/emu/machine/6526cia.h,MACHINES += 6526CIA
#-------------------------------------------------

ifneq ($(filter 6526CIA,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/6526cia.o
endif

#-------------------------------------------------
#
#@src/emu/machine/6532riot.h,MACHINES += RIOT6532
#-------------------------------------------------

ifneq ($(filter RIOT6532,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/6532riot.o
endif

#-------------------------------------------------
#
#@src/emu/machine/6821pia.h,MACHINES += 6821PIA
#-------------------------------------------------

ifneq ($(filter 6821PIA,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/6821pia.o
endif

#-------------------------------------------------
#
#@src/emu/machine/6840ptm.h,MACHINES += 6840PTM
#-------------------------------------------------

ifneq ($(filter 6840PTM,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/6840ptm.o
endif

#-------------------------------------------------
#
#@src/emu/machine/6850acia.h,MACHINES += ACIA6850
#-------------------------------------------------

ifneq ($(filter ACIA6850,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/6850acia.o
endif

#-------------------------------------------------
#
#@src/emu/machine/68681.h,MACHINES += 68681
#@src/emu/machine/n68681.h,MACHINES += 68681
#-------------------------------------------------

ifneq ($(filter 68681,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/68681.o
MACHINEOBJS += $(MACHINEOBJ)/n68681.o
endif

#-------------------------------------------------
#
#@src/emu/machine/7200fifo.h,MACHINES += 7200FIFO
#-------------------------------------------------

ifneq ($(filter 7200FIFO,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/7200fifo.o
endif

#-------------------------------------------------
#
#@src/emu/machine/74123.h,MACHINES += TTL74123
#-------------------------------------------------

ifneq ($(filter TTL74123,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/74123.o
endif

#-------------------------------------------------
#
#@src/emu/machine/74145.h,MACHINES += TTL74145
#-------------------------------------------------

ifneq ($(filter TTL74145,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/74145.o
endif

#-------------------------------------------------
#
#@src/emu/machine/74148.h,MACHINES += TTL74148
#-------------------------------------------------

ifneq ($(filter TTL74148,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/74148.o
endif

#-------------------------------------------------
#
#@src/emu/machine/74153.h,MACHINES += TTL74153
#-------------------------------------------------

ifneq ($(filter TTL74153,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/74153.o
endif

#-------------------------------------------------
#
#@src/emu/machine/74181.h,MACHINES += TTL74181
#-------------------------------------------------

ifneq ($(filter TTL74181,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/74181.o
endif

#-------------------------------------------------
#
#@src/emu/machine/7474.h,MACHINES += TTL7474
#-------------------------------------------------

ifneq ($(filter TTL7474,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/7474.o
endif

#-------------------------------------------------
#
#@src/emu/machine/8042kbdc.h,MACHINES += KBDC8042
#-------------------------------------------------

ifneq ($(filter KBDC8042,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/8042kbdc.o
endif

#-------------------------------------------------
#
#@src/emu/machine/8257dma.h,MACHINES += I8257
#-------------------------------------------------

ifneq ($(filter I8257,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/8257dma.o
endif

#-------------------------------------------------
#
#@src/emu/machine/aakart.h,MACHINES += AAKARTDEV
#-------------------------------------------------

ifneq ($(filter AAKARTDEV,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/aakart.o
endif

#-------------------------------------------------
#
#@src/emu/machine/adc0808.h,MACHINES += ADC0808
#-------------------------------------------------

ifneq ($(filter ADC0808,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/adc0808.o
endif

#-------------------------------------------------
#
#@src/emu/machine/adc083x.h,MACHINES += ADC083X
#-------------------------------------------------

ifneq ($(filter ADC083X,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/adc083x.o
endif

#-------------------------------------------------
#
#@src/emu/machine/adc1038.h,MACHINES += ADC1038
#-------------------------------------------------

ifneq ($(filter ADC1038,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/adc1038.o
endif

#-------------------------------------------------
#
#@src/emu/machine/adc1213x.h,MACHINES += ADC1213X
#-------------------------------------------------

ifneq ($(filter ADC1213X,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/adc1213x.o
endif

#-------------------------------------------------
#
#@src/emu/machine/aicartc.h,MACHINES += AICARTC
#-------------------------------------------------

ifneq ($(filter AICARTC,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/aicartc.o
endif

#-------------------------------------------------
#
#@src/emu/machine/am53cf96.h,MACHINES += AM53CF96
#-------------------------------------------------

ifneq ($(filter AM53CF96,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/am53cf96.o
endif

#-------------------------------------------------
#
#@src/emu/machine/am9517a.h,MACHINES += AM9517A
#-------------------------------------------------

ifneq ($(filter AM9517A,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/am9517a.o
endif

#-------------------------------------------------
#
#@src/emu/machine/amigafdc.h,MACHINES += AMIGAFDC
#-------------------------------------------------

ifneq ($(filter AMIGAFDC,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/amigafdc.o
endif

#-------------------------------------------------
#
#@src/emu/machine/at28c16.h,MACHINES += AT28C16
#-------------------------------------------------

ifneq ($(filter AT28C16,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/at28c16.o
endif

#-------------------------------------------------
#
#@src/emu/machine/at29040a.h,MACHINES += AT29040
#-------------------------------------------------

ifneq ($(filter AT29040,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/at29040a.o
endif

#-------------------------------------------------
#
#@src/emu/machine/at45dbxx.h,MACHINES += AT45DBXX
#-------------------------------------------------

ifneq ($(filter AT45DBXX,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/at45dbxx.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ataflash.h,MACHINES += ATAFLASH
#-------------------------------------------------

ifneq ($(filter ATAFLASH,$(MACHINES)),)
MACHINES += IDE
MACHINES += PCCARD
MACHINEOBJS += $(MACHINEOBJ)/ataflash.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ay31015.h,MACHINES += AY31015
#-------------------------------------------------

ifneq ($(filter AY31015,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ay31015.o
endif

#-------------------------------------------------
#
#@src/emu/machine/bankdev.h,MACHINES += BANKDEV
#-------------------------------------------------

ifneq ($(filter BANKDEV,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/bankdev.o
endif

#-------------------------------------------------
#
#@src/emu/machine/cdp1852.h,MACHINES += CDP1852
#-------------------------------------------------

ifneq ($(filter CDP1852,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/cdp1852.o
endif

#-------------------------------------------------
#
#@src/emu/machine/cdp1871.h,MACHINES += CDP1871
#-------------------------------------------------

ifneq ($(filter CDP1871,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/cdp1871.o
endif

#-------------------------------------------------
#
#@src/emu/machine/cdu76s.h,MACHINES += CDU76S
#-------------------------------------------------

ifneq ($(filter CDU76S,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/cdu76s.o
endif

#-------------------------------------------------
#
#@src/emu/machine/com8116.h,MACHINES += COM8116
#-------------------------------------------------

ifneq ($(filter COM8116,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/com8116.o
endif

#-------------------------------------------------
#
#@src/emu/machine/cr589.h,MACHINES += CR589
#-------------------------------------------------

ifneq ($(filter CR589,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/cr589.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ds1302.h,MACHINES += DS1302
#-------------------------------------------------

ifneq ($(filter DS1302,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ds1302.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ds2401.h,MACHINES += DS2401
#-------------------------------------------------

ifneq ($(filter DS2401,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ds2401.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ds2404.h,MACHINES += DS2404
#-------------------------------------------------

ifneq ($(filter DS2404,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ds2404.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ds75160a.h,MACHINES += DS75160A
#-------------------------------------------------

ifneq ($(filter DS75160A,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ds75160a.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ds75161a.h,MACHINES += DS75161A
#-------------------------------------------------

ifneq ($(filter DS75161A,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ds75161a.o
endif

#-------------------------------------------------
#
#@src/emu/machine/e0516.h,MACHINES += E0516
#-------------------------------------------------

ifneq ($(filter E0516,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/e0516.o
endif

#-------------------------------------------------
#
#@src/emu/machine/eeprom.h,MACHINES += EEPROMDEV
#@src/emu/machine/eepromser.h,MACHINES += EEPROMDEV
#@src/emu/machine/eeprompar.h,MACHINES += EEPROMDEV
#-------------------------------------------------

ifneq ($(filter EEPROMDEV,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/eeprom.o
MACHINEOBJS += $(MACHINEOBJ)/eepromser.o
MACHINEOBJS += $(MACHINEOBJ)/eeprompar.o
endif

#-------------------------------------------------
#
#@src/emu/machine/er2055.h,MACHINES += ER2055
#-------------------------------------------------

ifneq ($(filter ER2055,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/er2055.o
endif

#-------------------------------------------------
#
#@src/emu/machine/er59256.h,MACHINES += ER59256
#-------------------------------------------------

ifneq ($(filter ER59256,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/er59256.o
endif

#-------------------------------------------------
#
#@src/emu/machine/f3853.h,MACHINES += F3853
#-------------------------------------------------

ifneq ($(filter F3853,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/f3853.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i2cmem.h,MACHINES += I2CMEM
#-------------------------------------------------

ifneq ($(filter I2CMEM,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i2cmem.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8155.h,MACHINES += I8155
#-------------------------------------------------

ifneq ($(filter I8155,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8155.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8212.h,MACHINES += I8212
#-------------------------------------------------

ifneq ($(filter I8212,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8212.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8214.h,MACHINES += I8214
#-------------------------------------------------

ifneq ($(filter I8214,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8214.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8243.h,MACHINES += I8243
#-------------------------------------------------

ifneq ($(filter I8243,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8243.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8251.h,MACHINES += I8251
#-------------------------------------------------

ifneq ($(filter I8251,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8251.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8279.h,MACHINES += I8279
#-------------------------------------------------

ifneq ($(filter I8279,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8279.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8355.h,MACHINES += I8355
#-------------------------------------------------

ifneq ($(filter I8355,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8355.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i80130.h,MACHINES += I80130
#-------------------------------------------------

ifneq ($(filter I80130,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i80130.o
endif

#-------------------------------------------------
#
#@src/emu/machine/atadev.h,MACHINES += IDE
#@src/emu/machine/ataintf.h,MACHINES += IDE
#-------------------------------------------------

ifneq ($(filter IDE,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/atadev.o
MACHINEOBJS += $(MACHINEOBJ)/atahle.o
MACHINEOBJS += $(MACHINEOBJ)/ataintf.o
MACHINEOBJS += $(MACHINEOBJ)/atapicdr.o
MACHINEOBJS += $(MACHINEOBJ)/atapihle.o
MACHINEOBJS += $(MACHINEOBJ)/idectrl.o
MACHINEOBJS += $(MACHINEOBJ)/idehd.o
MACHINEOBJS += $(MACHINEOBJ)/vt83c461.o
MACHINES += T10
endif

#-------------------------------------------------
#
#@src/emu/machine/im6402.h,MACHINES += IM6402
#-------------------------------------------------

ifneq ($(filter IM6402,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/im6402.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ins8154.h,MACHINES += INS8154
#-------------------------------------------------

ifneq ($(filter INS8154,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ins8154.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ins8250.h,MACHINES += INS8250
#-------------------------------------------------

ifneq ($(filter INS8250,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ins8250.o
endif

#-------------------------------------------------
#
#@src/emu/machine/intelfsh.h,MACHINES += INTELFLASH
#-------------------------------------------------

ifneq ($(filter INTELFLASH,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/intelfsh.o
endif

#-------------------------------------------------
#
#@src/emu/machine/jvsdev.h,MACHINES += JVS
#@src/emu/machine/jvshost.h,MACHINES += JVS
#-------------------------------------------------

ifneq ($(filter JVS,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/jvsdev.o
MACHINEOBJS += $(MACHINEOBJ)/jvshost.o
endif

#-------------------------------------------------
#
#@src/emu/machine/k033906.h,MACHINES += K033906
#-------------------------------------------------

ifneq ($(filter K033906,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/k033906.o
endif

#-------------------------------------------------
#
#@src/emu/machine/k053252.h,MACHINES += K053252
#-------------------------------------------------

ifneq ($(filter K053252,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/k053252.o
endif

#-------------------------------------------------
#
#@src/emu/machine/k056230.h,MACHINES += K056230
#-------------------------------------------------

ifneq ($(filter K056230,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/k056230.o
endif

#-------------------------------------------------
#
#@src/emu/machine/latch8.h,MACHINES += LATCH8
#-------------------------------------------------

ifneq ($(filter LATCH8,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/latch8.o
endif

#-------------------------------------------------
#
#@src/emu/machine/lc89510.h,MACHINES += LC89510
#-------------------------------------------------

ifneq ($(filter LC89510,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/lc89510.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ldpr8210.h,MACHINES += LDPR8210
#-------------------------------------------------

ifneq ($(filter LDPR8210,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ldpr8210.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ldstub.h,MACHINES += LDSTUB
#-------------------------------------------------

ifneq ($(filter LDSTUB,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ldstub.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ldv1000.h,MACHINES += LDV1000
#-------------------------------------------------

ifneq ($(filter LDV1000,$(MACHINES)),)
MACHINES += Z80CTC
MACHINES += I8255
MACHINEOBJS += $(MACHINEOBJ)/ldv1000.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ldvp931.h,MACHINES += LDVP931
#-------------------------------------------------

ifneq ($(filter LDVP931,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ldvp931.o
endif

#-------------------------------------------------
#
#@src/emu/machine/linflash.h,MACHINES += LINFLASH
#-------------------------------------------------

ifneq ($(filter LINFLASH,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/linflash.o
endif

#-------------------------------------------------
#
#@src/emu/machine/m6m80011ap.h,MACHINES += M6M80011AP
#-------------------------------------------------

ifneq ($(filter M6M80011AP,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/m6m80011ap.o
endif

#-------------------------------------------------
#
#@src/emu/machine/matsucd.h,MACHINES += MATSUCD
#-------------------------------------------------

ifneq ($(filter MATSUCD,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/matsucd.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mb14241.h,MACHINES += MB14241
#-------------------------------------------------

ifneq ($(filter MB14241,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mb14241.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mb3773.h,MACHINES += MB3773
#-------------------------------------------------

ifneq ($(filter MB3773,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mb3773.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mb87078.h,MACHINES += MB87078
#-------------------------------------------------

ifneq ($(filter MB87078,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mb87078.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mb89371.h,MACHINES += MB89371
#-------------------------------------------------

ifneq ($(filter MB89371,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mb89371.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mc146818.h,MACHINES += MC146818
#-------------------------------------------------

ifneq ($(filter MC146818,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mc146818.o $(MACHINEOBJ)/ds128x.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mc2661.h,MACHINES += MC2661
#-------------------------------------------------

ifneq ($(filter MC2661,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mc2661.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mc6843.h,MACHINES += MC6843
#-------------------------------------------------

ifneq ($(filter MC6843,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mc6843.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mc6846.h,MACHINES += MC6846
#-------------------------------------------------

ifneq ($(filter MC6846,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mc6846.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mc6852.h,MACHINES += MC6852
#-------------------------------------------------

ifneq ($(filter MC6852,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mc6852.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mc6854.h,MACHINES += MC6854
#-------------------------------------------------

ifneq ($(filter MC6854,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mc6854.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mc68901.h,MACHINES += MC68901
#-------------------------------------------------

ifneq ($(filter MC68901,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mc68901.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mccs1850.h,MACHINES += MCCS1850
#-------------------------------------------------

ifneq ($(filter MCCS1850,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mccs1850.o
endif

#-------------------------------------------------
#
#@src/emu/machine/68307.h,MACHINES += M68307
#-------------------------------------------------

ifneq ($(filter M68307,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/68307.o
MACHINEOBJS += $(MACHINEOBJ)/68307sim.o
MACHINEOBJS += $(MACHINEOBJ)/68307bus.o
MACHINEOBJS += $(MACHINEOBJ)/68307ser.o
MACHINEOBJS += $(MACHINEOBJ)/68307tmu.o
endif

#-------------------------------------------------
#
#@src/emu/machine/68340.h,MACHINES += M68340
#-------------------------------------------------

ifneq ($(filter M68340,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/68340.o
MACHINEOBJS += $(MACHINEOBJ)/68340sim.o
MACHINEOBJS += $(MACHINEOBJ)/68340dma.o
MACHINEOBJS += $(MACHINEOBJ)/68340ser.o
MACHINEOBJS += $(MACHINEOBJ)/68340tmu.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mcf5206e.h,MACHINES += MCF5206E
#-------------------------------------------------

ifneq ($(filter MCF5206E,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mcf5206e.o
endif

#-------------------------------------------------
#
#@src/emu/machine/microtch.h,MACHINES += MICROTOUCH
#-------------------------------------------------

ifneq ($(filter MICROTOUCH,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/microtch.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mm58274c.h,MACHINES += MM58274C
#-------------------------------------------------

ifneq ($(filter MM58274C,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mm58274c.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mm74c922.h,MACHINES += MM74C922
#-------------------------------------------------

ifneq ($(filter MM74C922,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mm74c922.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos6526.h,MACHINES += MOS6526
#-------------------------------------------------

ifneq ($(filter MOS6526,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos6526.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos6529.h,MACHINES += MOS6529
#-------------------------------------------------

ifneq ($(filter MOS6529,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos6529.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos6702.h,MACHINES += MOS6702
#-------------------------------------------------

ifneq ($(filter MOS6702,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos6702.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos8706.h,MACHINES += MOS8706
#-------------------------------------------------

ifneq ($(filter MOS8706,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos8706.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos8722.h,MACHINES += MOS8722
#-------------------------------------------------

ifneq ($(filter MOS8722,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos8722.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos8726.h,MACHINES += MOS8726
#-------------------------------------------------

ifneq ($(filter MOS8726,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos8726.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos6530.h,MACHINES += MIOT6530
#-------------------------------------------------

ifneq ($(filter MIOT6530,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos6530.o
endif

#-------------------------------------------------
#
#@src/emu/machine/mos6551.h,MACHINES += MOS6551
#-------------------------------------------------

ifneq ($(filter MOS6551,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/mos6551.o
endif

#-------------------------------------------------
#
#@src/emu/machine/msm5832.h,MACHINES += MSM5832
#-------------------------------------------------

ifneq ($(filter MSM5832,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/msm5832.o
endif

#-------------------------------------------------
#
#@src/emu/machine/msm58321.h,MACHINES += MSM58321
#-------------------------------------------------

ifneq ($(filter MSM58321,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/msm58321.o
endif

#-------------------------------------------------
#
#@src/emu/machine/msm6242.h,MACHINES += MSM6242
#-------------------------------------------------

ifneq ($(filter MSM6242,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/msm6242.o
endif

#-------------------------------------------------
#
#@src/emu/machine/ncr539x.h,MACHINES += NCR539x
#-------------------------------------------------

ifneq ($(filter NCR539x,$(MACHINES)),)
MACHINES += SCSI
MACHINEOBJS += $(MACHINEOBJ)/ncr539x.o
endif

#-------------------------------------------------
#
#@src/emu/machine/nmc9306.h,MACHINES += NMC9306
#-------------------------------------------------

ifneq ($(filter NMC9306,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/nmc9306.o
endif

#-------------------------------------------------
#
#@src/emu/machine/nscsi_bus.h,MACHINES += NSCSI
#@src/emu/machine/nscsi_cd.h,MACHINES += NSCSI
#@src/emu/machine/nscsi_hd.h,MACHINES += NSCSI
#-------------------------------------------------

ifneq ($(filter NSCSI,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/nscsi_bus.o
MACHINEOBJS += $(MACHINEOBJ)/nscsi_cd.o
MACHINEOBJS += $(MACHINEOBJ)/nscsi_hd.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pcf8593.h,MACHINES += PCF8593
#-------------------------------------------------

ifneq ($(filter PCF8593,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pcf8593.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pci.h,MACHINES += PCI
#-------------------------------------------------

ifneq ($(filter PCI,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pci.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pckeybrd.h,MACHINES += PCKEYBRD
#-------------------------------------------------

ifneq ($(filter PCKEYBRD,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pckeybrd.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pd4990a.h,MACHINES += PD4990A_OLD
#-------------------------------------------------

ifneq ($(filter PD4990A_OLD,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pd4990a.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pic8259.h,MACHINES += PIC8259
#-------------------------------------------------

ifneq ($(filter PIC8259,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pic8259.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pit8253.h,MACHINES += PIT8253
#-------------------------------------------------

ifneq ($(filter PIT8253,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pit8253.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pla.h,MACHINES += PLA
#-------------------------------------------------

ifneq ($(filter PLA,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pla.o
endif

#-------------------------------------------------
#
#@src/emu/machine/rf5c296.h,MACHINES += RF5C296
#-------------------------------------------------

ifneq ($(filter RF5C296,$(MACHINES)),)
MACHINES += PCCARD
MACHINEOBJS += $(MACHINEOBJ)/rf5c296.o
endif

#-------------------------------------------------
#
#@src/emu/machine/roc10937.h,MACHINES += ROC10937
#-------------------------------------------------

ifneq ($(filter ROC10937,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/roc10937.o
endif

#-------------------------------------------------
#
#@src/emu/machine/rp5c01.h,MACHINES += RP5C01
#-------------------------------------------------

ifneq ($(filter RP5C01,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/rp5c01.o
endif

#-------------------------------------------------
#
#@src/emu/machine/rp5c15.h,MACHINES += RP5C15
#-------------------------------------------------

ifneq ($(filter RP5C15,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/rp5c15.o
endif

#-------------------------------------------------
#
#@src/emu/machine/rp5h01.h,MACHINES += RP5H01
#-------------------------------------------------

ifneq ($(filter RP5H01,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/rp5h01.o
endif

#-------------------------------------------------
#
#@src/emu/machine/64h156.h,MACHINES += RP5C15
#-------------------------------------------------

ifneq ($(filter R64H156,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/64h156.o
endif

#-------------------------------------------------
#
#@src/emu/machine/rtc4543.h,MACHINES += RTC4543
#-------------------------------------------------

ifneq ($(filter RTC4543,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/rtc4543.o
endif

#-------------------------------------------------
#
#@src/emu/machine/rtc65271.h,MACHINES += RTC65271
#-------------------------------------------------

ifneq ($(filter RTC65271,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/rtc65271.o
endif

#-------------------------------------------------
#
#@src/emu/machine/rtc9701.h,MACHINES += RTC9701
#-------------------------------------------------

ifneq ($(filter RTC9701,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/rtc9701.o
endif

#-------------------------------------------------
#
#@src/emu/machine/s2636.h,MACHINES += S2636
#-------------------------------------------------

ifneq ($(filter S2636,$(MACHINES)),)
MACHINEOBJS+= $(MACHINEOBJ)/s2636.o
endif

#-------------------------------------------------
#
#@src/emu/machine/s3520cf.h,MACHINES += S3520CF
#-------------------------------------------------

ifneq ($(filter S3520CF,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/s3520cf.o
endif

#-------------------------------------------------
#
#@src/emu/machine/s3c2400.h,MACHINES += S3C2400
#-------------------------------------------------

ifneq ($(filter S3C2400,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/s3c2400.o
endif

#-------------------------------------------------
#
#@src/emu/machine/s3c2410.h,MACHINES += S3C2410
#-------------------------------------------------

ifneq ($(filter S3C2410,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/s3c2410.o
endif

#-------------------------------------------------
#
#@src/emu/machine/s3c2440.h,MACHINES += S3C2440
#-------------------------------------------------

ifneq ($(filter S3C2440,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/s3c2440.o
endif

#-------------------------------------------------
#
#@src/emu/machine/saturn.h,MACHINES += SATURN
#-------------------------------------------------

ifneq ($(filter SATURN,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/saturn.o
endif

#-------------------------------------------------
#
#@src/emu/machine/scsibus.h,MACHINES += SCSI
#@src/emu/machine/scsicb.h,MACHINES += SCSI
#@src/emu/machine/scsicd.h,MACHINES += SCSI
#@src/emu/machine/scsidev.h,MACHINES += SCSI
#@src/emu/machine/scsihd.h,MACHINES += SCSI
#@src/emu/machine/scsihle.h,MACHINES += SCSI
#-------------------------------------------------

ifneq ($(filter SCSI,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/scsibus.o
MACHINEOBJS += $(MACHINEOBJ)/scsicb.o
MACHINEOBJS += $(MACHINEOBJ)/scsicd.o
MACHINEOBJS += $(MACHINEOBJ)/scsidev.o
MACHINEOBJS += $(MACHINEOBJ)/scsihd.o
MACHINEOBJS += $(MACHINEOBJ)/scsihle.o
MACHINES += T10
endif

#-------------------------------------------------
#
#@src/emu/machine/seibu_cop.h,MACHINES += SEIBU_COP
#-------------------------------------------------

ifneq ($(filter SEIBU_COP,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/seibu_cop.o
endif

#-------------------------------------------------
#
#@src/emu/machine/serflash.h,MACHINES += SERFLASH
#-------------------------------------------------

ifneq ($(filter SERFLASH,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/serflash.o
endif

#-------------------------------------------------
#
#@src/emu/machine/smc91c9x.h,MACHINES += SMC91C9X
#-------------------------------------------------

ifneq ($(filter SMC91C9X,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/smc91c9x.o
endif

#-------------------------------------------------
#
#@src/emu/machine/smpc.h,MACHINES += SMPC
#-------------------------------------------------

ifneq ($(filter SMPC,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/smpc.o
endif

#-------------------------------------------------
#
#@src/emu/machine/stvcd.h,MACHINES += STVCD
#-------------------------------------------------

ifneq ($(filter STVCD,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/stvcd.o
endif

#-------------------------------------------------
#
#-------------------------------------------------

ifneq ($(filter T10,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/t10mmc.o
MACHINEOBJS += $(MACHINEOBJ)/t10sbc.o
MACHINEOBJS += $(MACHINEOBJ)/t10spc.o
MACHINES += T10
endif

#-------------------------------------------------
#
#@src/emu/machine/tc009xlvc.h,MACHINES += TC0091LVC
#-------------------------------------------------

ifneq ($(filter TC0091LVC,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/tc009xlvc.o
endif

#-------------------------------------------------
#
#@src/emu/machine/timekpr.h,MACHINES += TIMEKPR
#-------------------------------------------------

ifneq ($(filter TIMEKPR,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/timekpr.o
endif

#-------------------------------------------------
#
#@src/emu/machine/tmp68301.h,MACHINES += TMP68301
#-------------------------------------------------

ifneq ($(filter TMP68301,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/tmp68301.o
endif

#-------------------------------------------------
#
#@src/emu/machine/tms6100.h,MACHINES += TMS6100
#-------------------------------------------------

ifneq ($(filter TMS6100,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/tms6100.o
endif

#-------------------------------------------------
#
#@src/emu/machine/tms9901.h,MACHINES += TMS9901
#-------------------------------------------------

ifneq ($(filter TMS9901,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/tms9901.o
endif

#-------------------------------------------------
#
#@src/emu/machine/tms9902.h,MACHINES += TMS9902
#-------------------------------------------------

ifneq ($(filter TMS9902,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/tms9902.o
endif

#-------------------------------------------------
#
#@src/emu/machine/upd1990a.h,MACHINES += UPD1990A
#-------------------------------------------------

ifneq ($(filter UPD1990A,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/upd1990a.o
endif

#-------------------------------------------------
#
#@src/emu/machine/upd4701.h,MACHINES += UPD4701
#-------------------------------------------------

ifneq ($(filter UPD4701,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/upd4701.o
endif

#-------------------------------------------------
#
#@src/emu/machine/upd7002.h,MACHINES += UPD7002
#-------------------------------------------------

ifneq ($(filter UPD7002,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/upd7002.o
endif

#-------------------------------------------------
#
#@src/emu/machine/upd765.h,MACHINES += UPD765
#-------------------------------------------------

ifneq ($(filter UPD765,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/upd765.o
endif

#-------------------------------------------------
#
#@src/emu/machine/v3021.h,MACHINES += V3021
#-------------------------------------------------

ifneq ($(filter V3021,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/v3021.o
endif

#-------------------------------------------------
#
#@src/emu/machine/wd_fdc.h,MACHINES += WD_FDC
#-------------------------------------------------

ifneq ($(filter WD_FDC,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/wd_fdc.o
MACHINEOBJS += $(MACHINEOBJ)/fdc_pll.o
endif

#-------------------------------------------------
#
#@src/emu/machine/wd11c00_17.h,MACHINES += WD11C00_17
#-------------------------------------------------

ifneq ($(filter WD11C00_17,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/wd11c00_17.o
endif

#-------------------------------------------------
#
#@src/emu/machine/wd17xx.h,MACHINES += WD17XX
#-------------------------------------------------

ifneq ($(filter WD17XX,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/wd17xx.o
endif

#-------------------------------------------------
#
#@src/emu/machine/wd2010.h,MACHINES += WD2010
#-------------------------------------------------

ifneq ($(filter WD2010,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/wd2010.o
endif

#-------------------------------------------------
#
#@src/emu/machine/wd33c93.h,MACHINES += WD33C93
#-------------------------------------------------

ifneq ($(filter WD33C93,$(MACHINES)),)
MACHINES += SCSI
MACHINEOBJS += $(MACHINEOBJ)/wd33c93.o
endif

#-------------------------------------------------
#
#@src/emu/machine/x2212.h,MACHINES += X2212
#-------------------------------------------------

ifneq ($(filter X2212,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/x2212.o
endif

#-------------------------------------------------
#
#@src/emu/machine/x76f041.h,MACHINES += X76F041
#-------------------------------------------------

ifneq ($(filter X76F041,$(MACHINES)),)
MACHINES += SECFLASH
MACHINEOBJS += $(MACHINEOBJ)/x76f041.o
endif

#-------------------------------------------------
#
#@src/emu/machine/x76f100.h,MACHINES += X76F100
#-------------------------------------------------

ifneq ($(filter X76F100,$(MACHINES)),)
MACHINES += SECFLASH
MACHINEOBJS += $(MACHINEOBJ)/x76f100.o
endif

#-------------------------------------------------
#
#@src/emu/machine/z80ctc.h,MACHINES += Z80CTC
#-------------------------------------------------

ifneq ($(filter Z80CTC,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/z80ctc.o
endif

#-------------------------------------------------
#
#@src/emu/machine/z80dart.h,MACHINES += Z80DART
#-------------------------------------------------

ifneq ($(filter Z80DART,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/z80dart.o
endif

#-------------------------------------------------
#
#@src/emu/machine/z80dma.h,MACHINES += Z80DMA
#-------------------------------------------------

ifneq ($(filter Z80DMA,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/z80dma.o
endif

#-------------------------------------------------
#
#@src/emu/machine/z80pio.h,MACHINES += Z80PIO
#-------------------------------------------------

ifneq ($(filter Z80PIO,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/z80pio.o
endif

#-------------------------------------------------
#
#@src/emu/machine/z80sio.h,MACHINES += Z80SIO
#-------------------------------------------------

ifneq ($(filter Z80SIO,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/z80sio.o
endif

#-------------------------------------------------
#
#@src/emu/machine/z80sti.h,MACHINES += Z80STI
#-------------------------------------------------

ifneq ($(filter Z80STI,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/z80sti.o
endif

#-------------------------------------------------
#
#@src/emu/machine/z8536.h,MACHINES += Z8536
#-------------------------------------------------

ifneq ($(filter Z8536,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/z8536.o
endif

#-------------------------------------------------
#
#@src/emu/machine/secflash.h,MACHINES += SECFLASH
#-------------------------------------------------

ifneq ($(filter SECFLASH,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/secflash.o
endif

#-------------------------------------------------
#
#@src/emu/machine/pccard.h,MACHINES += PCCARD
#-------------------------------------------------

ifneq ($(filter PCCARD,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/pccard.o
endif

#-------------------------------------------------
#
#@src/emu/machine/i8255.h,MACHINES += I8255
#-------------------------------------------------

ifneq ($(filter I8255,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/i8255.o
endif

$(MACHINEOBJ)/s3c2400.o:    $(MACHINESRC)/s3c24xx.c
$(MACHINEOBJ)/s3c2410.o:    $(MACHINESRC)/s3c24xx.c
$(MACHINEOBJ)/s3c2440.o:    $(MACHINESRC)/s3c24xx.c

#-------------------------------------------------
#
#@src/emu/machine/ncr5380n.h,MACHINES += NCR5380N
#-------------------------------------------------

ifneq ($(filter NCR5380N,$(MACHINES)),)
MACHINEOBJS += $(MACHINEOBJ)/ncr5380n.o
endif

