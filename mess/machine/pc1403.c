#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"
#include "includes/pc1403.h"

/* C-CE while reset, program will not be destroyed! */

static UINT8 outa;
UINT8 pc1403_portc;

static int power=1; /* simulates pressed cce when mess is started */


/* 
   port 2:
     bits 0,1: external rom a14,a15 lines
   port 3: 
     bits 0..6 keyboard output select matrix line
*/
static UINT8 asic[4];

WRITE_HANDLER(pc1403_asic_write)
{
    asic[offset>>9]=data;
    switch( (offset>>9) ){
    case 0/*0x3800*/:
	// output
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 1/*0x3a00*/:
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 2/*0x3c00*/:
	cpu_setbank(1, memory_region(REGION_USER1)+((data&7)<<14));
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 3/*0x3e00*/: break;
    }
}

READ_HANDLER(pc1403_asic_read)
{
    UINT8 data=asic[offset>>9];
    switch( (offset>>9) ){
    case 0: case 1: case 2:
	logerror ("asic read %.4x %.2x\n",offset, data);
	break;
    }
    return data;
}

void pc1403_outa(int data)
{
    outa=data;
}

int pc1403_ina(void)
{
    UINT8 data=outa;

    if (asic[3]&1) {
	if (KEY_7) data|=1;
	if (KEY_8) data|=2;
	if (KEY_9) data|=4;
	if (KEY_DIV) data|=8;
	if (KEY_XM) data|=0x10;
	// 0x20
	// 0x40
	// 0x80
    }
    if (asic[3]&2) {
	if (KEY_4) data|=1;
	if (KEY_5) data|=2;
	if (KEY_6) data|=4;
	if (KEY_MUL) data|=8;
	if (KEY_RM) data|=0x10;
	if (KEY_SHIFT) data|=0x20;
	if (KEY_DEF) data|=0x40;
	if (KEY_SMALL) data|=0x80;
    }
    if (asic[3]&4) {
	if (KEY_1) data|=1;
	if (KEY_2) data|=2;
	if (KEY_3) data|=4;
	if (KEY_MINUS) data|=8;
	if (KEY_MPLUS) data|=0x10;
	if (KEY_Q) data|=0x20;
	if (KEY_A) data|=0x40;
	if (KEY_Z) data|=0x80;
    }
    if (asic[3]&8) {
	if (KEY_0) data|=1;
	if (KEY_SIGN) data|=2;
	if (KEY_POINT) data|=4;
	if (KEY_PLUS) data|=8;
	if (KEY_EQUALS) data|=0x10;
	if (KEY_W) data|=0x20;
	if (KEY_S) data|=0x40;
	if (KEY_X) data|=0x80;
    }
    if (asic[3]&0x10) {
	if (KEY_HYP) data|=1;
	if (KEY_SIN) data|=2;
	if (KEY_COS) data|=4;
	if (KEY_TAN) data|=8;
	//0x10 toggles indicator 3c bit 0 japan?
	if (KEY_E) data|=0x20;
	if (KEY_D) data|=0x40;
	if (KEY_C) data|=0x80;
    }
    if (asic[3]&0x20) {
	if (KEY_HEX) data|=1;
	if (KEY_DEG) data|=2;
	if (KEY_LN) data|=4;
	if (KEY_LOG) data|=8;
	//0x10 tilde
	if (KEY_R) data|=0x20;
	if (KEY_F) data|=0x40;
	if (KEY_V) data|=0x80;
    }
    if (asic[3]&0x40) {
	if (KEY_EXP) data|=1;
	if (KEY_POT) data|=2;
	if (KEY_ROOT) data|=4;
	if (KEY_SQUARE) data|=8;
	//0x10 - yen
	if (KEY_T) data|=0x20;
	if (KEY_G) data|=0x40;
	if (KEY_B) data|=0x80;
    }
    if (outa&1) {
	if (power||(KEY_CCE)) data|=2;
	if (KEY_STAT) data|=4;
	if (KEY_FE) data|=8;
	if (KEY_DOWN) data|=0x10;
	if (KEY_Y) data|=0x20;
	if (KEY_H) data|=0x40;
	if (KEY_N) data|=0x80;
    }
    if (outa&2) {
	if (KEY_BRACE_RIGHT) data|=4;
	if (KEY_1X) data|=8;
	if (KEY_UP) data|=0x10;
	if (KEY_U) data|=0x20;
	if (KEY_J) data|=0x40;
	if (KEY_M) data|=0x80;	
    }
    if (outa&4) {
	if (KEY_BRACE_LEFT) data|=8;
	if (KEY_LEFT) data|=0x10;
	if (KEY_I) data|=0x20;
	if (KEY_K) data|=0x40;
	if (KEY_SPC) data|=0x80;
    }
    if (outa&8) {
	if (KEY_RIGHT) data|=0x10;
	if (KEY_O) data|=0x20;
	if (KEY_L) data|=0x40;
	if (KEY_ENTER) data|=0x80;
    }
    if (outa&0x10) {
	if (KEY_P) data|=0x20;
	if (KEY_COMMA) data|=0x40;
	if (KEY_BASIC) data|=0x80;
    }
    if (outa&0x20) {
	//0x40 shift lock
	if (KEY_CAL) data|=0x80;
    }
    if (outa&0x40) {
	if (KEY_OFF) data|=0x80;
    }
    return data;
}

#if 0
int pc1403_inb(void)
{
	int data=outb;
	if (KEY_OFF) data|=1;
	return data;
}
#endif

void pc1403_outc(int data)
{
    pc1403_portc=data;
    logerror("%g pc %.4x outc %.2x\n",timer_get_time(), cpu_get_pc(), data);
}


bool pc1403_brk(void)
{
	return KEY_BRK;
}

bool pc1403_reset(void)
{
	return KEY_RESET;
}

/* currently enough to save the external ram */
static void pc1403_load(void)
{
	FILE *file;
	UINT8 *ram=memory_region(REGION_CPU1)+0x8000,
		*cpu=sc61860_internal_ram();

	if ( (file=(FILE*)osd_fopen(Machine->gamedrv->name,
								Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 0))==NULL) {
		power=0;
		return;
	}

	osd_fread(file, cpu, 96);
	osd_fread(file, ram, 0x8000);
	osd_fclose(file);
}

static void pc1403_save(void)
{
	FILE *file;
	UINT8 *ram=memory_region(REGION_CPU1)+0x8000,
		*cpu=sc61860_internal_ram();

	if ( (file=(FILE*)osd_fopen(Machine->gamedrv->name,
						 Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 1))==NULL)
		return;

	osd_fwrite(file, cpu, 96);
	osd_fwrite(file, ram, 0x8000);
	osd_fclose(file);
}

static void pc1403_power_up(int param)
{
	power=0;
}

void init_pc1403(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);

	for (i=0; i<128; i++) gfx[i]=i;

	pc1403_load();
	timer_pulse(1/500.0, 0,sc61860_2ms_tick);
	timer_set(1,0,pc1403_power_up);
}

void pc1403_machine_init(void)
{
	cpu_setbank(1, memory_region(REGION_USER1));
	if (RAM32K) {
		install_mem_read_handler (0, 0x8000, 0xdfff, MRA_RAM);
		install_mem_write_handler (0, 0x8000, 0xdfff, MWA_RAM);
	} else {
		install_mem_read_handler (0, 0x8000, 0xdfff, MRA_NOP);
		install_mem_write_handler (0, 0x8000, 0xdfff, MWA_NOP);
	}
}

void pc1403_machine_stop(void)
{
	pc1403_save();
}

