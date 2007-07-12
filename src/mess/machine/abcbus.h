/*

		              a     b
		-12 V	<--   *  1  *   --> -12V
		0 V		---   *  2  *   --- 0 V
		RESIN_	-->   *  3  *   --> XMEMWR_
		0 V		---	  *  4  *   --> XMEMFL_
		INT_    -->   *  5  *   --> phi
		D7      <->   *  6  *   --- 0 V
		D6      <->   *  7  *   --- 0 V
		D5      <->   *  8  *   --- 0 V
		D4      <->   *  9  *   --- 0 V
		D3      <->   * 10  *   --- 0 V
		D2      <->   * 11  *   --- 0 V
		D1      <->   * 12  *   --- 0 V
		D0      <->   * 13  *   --- 0 V
		              * 14  *   --> A15
		RST_    <--   * 15  *   --> A14
		IN1		<--   * 16  *   --> A13
		IN0     <--   * 17  *   --> A12
		OUT5    <--   * 18  *   --> A11
		OUT4    <--   * 19  *   --> A10
		OUT3    <--   * 20  *   --> A9
		OUT2    <--   * 21  *   --> A8
		OUT0    <--   * 22  *   --> A7
		OUT1    <--   * 23  *   --> A6
		NMI_    -->   * 24  *   --> A5
		INP2_   <--   * 25  *   --> A4
	   XINPSTB_ <--   * 26  *   --> A3
	  XOUTPSTB_ <--   * 27  *   --> A2
		XM_     -->   * 28  *   --> A1
		RFSH_   <--   * 29  *   --> A0
		RDY     -->   * 30  *   --> MEMRQ_
		+5 V    <--   * 31  *   --> +5 V
		+12 V   <--   * 32  *   --> +12 V

*/

/*

	Type					Size		Tracks	Sides	Sectors/track	Sectors		Speed			Drives

	ABC-830		Floppy		160 KB		40		1		16				640			250 Kbps		MPI 51, BASF 6106
				Floppy		80 KB		40		1		8				320			250 Kbps		Scandia Metric FD2		
	ABC-832		Floppy		640 KB		80		2		16				2560		250 Kbps		Micropolis 1015, Micropolis 1115, BASF 6118
	ABC-834		Floppy		640 KB		80		2		16				2560		250 Kbps		Teac FD 55 F
	ABC-838		Floppy		1 MB		77		2		25				3978 		500 Kbps		BASF 6104, BASF 6115
	ABC-850		Floppy		640 KB		80		2		16				2560		250 Kbps		TEAC FD 55 F, BASF 6136
				HDD			10 MB		320		4		32				40960 		5 Mbps			Rodime 202, BASF 6186
	ABC-852		Floppy		640 KB		80		2		16				2560		250 Kbps		TEAC FD 55 F, BASF 6136
				HDD			20 MB		615		4		32				78720		5 Mbps			NEC 5126
				Streamer	45 MB		9											90 Kbps			Archive 5945-C (tape: DC300XL 450ft)
	ABC-856		Floppy		640 KB		80		2		16				2560		250 Kbps		TEAC FD 55 F
				HDD			64 MB		1024	8		32				262144		5 Mbps			Micropolis 1325
				Streamer	45 MB		9											90 Kbps			Archive 5945-C (tape: DC300XL 450ft)

*/

enum
{
	ABCBUS_CHANNEL_ABC850_852_856 = 36,
	ABCBUS_CHANNEL_ABC832_834_850 = 44,
	ABCBUS_CHANNEL_ABC830 = 45,
	ABCBUS_CHANNEL_ABC838 = 46
};

READ8_HANDLER( abcbus_data_r );
WRITE8_HANDLER( abcbus_data_w );
READ8_HANDLER( abcbus_status_r );
WRITE8_HANDLER( abcbus_channel_w );
WRITE8_HANDLER( abcbus_command_w );
READ8_HANDLER( abcbus_reset_r );
READ8_HANDLER( abcbus_strobe_r );
WRITE8_HANDLER( abcbus_strobe_w );
DEVICE_LOAD( abc_floppy );
