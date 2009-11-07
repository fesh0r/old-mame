/****************************************************************************

    drivers/palm.c
    Palm (MC68328) emulation

    Driver by MooglyGuy

    Additional bug fixing by R. Belmont

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "includes/mc68328.h"
#include "sound/dac.h"
#include "debugger.h"
#include "devices/messram.h"

static CPU_DISASSEMBLE(palm_dasm_override);

static UINT8 port_f_latch;
static UINT16 spim_data;

/***************************************************************************
    MACHINE HARDWARE
***************************************************************************/

static INPUT_CHANGED( pen_check )
{
    UINT8 button = input_port_read(field->port->machine, "PENB");
    const device_config *mc68328_device = devtag_get_device(field->port->machine, "dragonball");
    if(button)
    {
        mc68328_set_penirq_line(mc68328_device, 1);
    }
    else
    {
        mc68328_set_penirq_line(mc68328_device, 0);
    }
}

static INPUT_CHANGED( button_check )
{
    UINT8 button_state = input_port_read(field->port->machine, "PORTD");
    const device_config *mc68328_device = devtag_get_device(field->port->machine, "dragonball");

    mc68328_set_port_d_lines(mc68328_device, button_state, (int)(FPTR)param);
}

static WRITE8_DEVICE_HANDLER( palm_port_f_out )
{
    port_f_latch = data;
}

static READ8_DEVICE_HANDLER( palm_port_c_in )
{
    return 0x10;
}

static READ8_DEVICE_HANDLER( palm_port_f_in )
{
    return port_f_latch;
}

static WRITE16_DEVICE_HANDLER( palm_spim_out )
{
    spim_data = data;
}

static READ16_DEVICE_HANDLER( palm_spim_in )
{
    return spim_data;
}

static void palm_spim_exchange( const device_config *device )
{
    UINT8 x = input_port_read(device->machine, "PENX");
    UINT8 y = input_port_read(device->machine, "PENY");

    switch( port_f_latch & 0x0f )
    {
        case 0x06:
            spim_data = (0xff - x) * 2;
            break;

        case 0x09:
            spim_data = (0xff - y) * 2;
            break;
    }
}

static MACHINE_START( palm )
{
    const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
    memory_install_read16_handler (space, 0x000000, messram_get_size(devtag_get_device(machine, "messram")) - 1, messram_get_size(devtag_get_device(machine, "messram")) - 1, 0, (read16_space_func)1);
    memory_install_write16_handler(space, 0x000000, messram_get_size(devtag_get_device(machine, "messram")) - 1, messram_get_size(devtag_get_device(machine, "messram")) - 1, 0, (write16_space_func)1);
    memory_set_bankptr(machine, 1, messram_get_ptr(devtag_get_device(machine, "messram")));

    state_save_register_global(machine, port_f_latch);
    state_save_register_global(machine, spim_data);
}

static MACHINE_RESET( palm )
{
    // Copy boot ROM
    UINT8* bios = memory_region(machine, "bios");
    memset(messram_get_ptr(devtag_get_device(machine, "messram")), 0, messram_get_size(devtag_get_device(machine, "messram")));
    memcpy(messram_get_ptr(devtag_get_device(machine, "messram")), bios, 0x20000);

    device_reset(cputag_get_cpu(machine, "maincpu"));
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START(palm_map, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE(0xc00000, 0xe07fff) AM_ROM AM_REGION("bios", 0)
    AM_RANGE(0xfff000, 0xffffff) AM_DEVREADWRITE(MC68328_TAG, mc68328_r, mc68328_w)
ADDRESS_MAP_END


/***************************************************************************
    AUDIO HARDWARE
***************************************************************************/

static WRITE8_DEVICE_HANDLER( palm_dac_transition )
{
    dac_data_w( devtag_get_device(device->machine, "dac"), 0x7f * data );
}


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static DRIVER_INIT( palm )
{
    debug_cpu_set_dasm_override(cputag_get_cpu(machine, "maincpu"), CPU_DISASSEMBLE_NAME(palm_dasm_override));
}

static const mc68328_interface palm_dragonball_iface =
{
	"maincpu",

    NULL,                   // Port A Output
    NULL,                   // Port B Output
    NULL,                   // Port C Output
    NULL,                   // Port D Output
    NULL,                   // Port E Output
    palm_port_f_out,        // Port F Output
    NULL,                   // Port G Output
    NULL,                   // Port J Output
    NULL,                   // Port K Output
    NULL,                   // Port M Output

    NULL,                   // Port A Input
    NULL,                   // Port B Input
    palm_port_c_in,         // Port C Input
    NULL,                   // Port D Input
    NULL,                   // Port E Input
    palm_port_f_in,         // Port F Input
    NULL,                   // Port G Input
    NULL,                   // Port J Input
    NULL,                   // Port K Input
    NULL,                   // Port M Input

    palm_dac_transition,

    palm_spim_out,
    palm_spim_in,
    palm_spim_exchange
};


static MACHINE_DRIVER_START( palm )

    /* basic machine hardware */
    MDRV_CPU_ADD( "maincpu", M68000, 32768*506 )        /* 16.580608 MHz */
    MDRV_CPU_PROGRAM_MAP( palm_map)
    MDRV_SCREEN_ADD( "screen", RASTER )
    MDRV_SCREEN_REFRESH_RATE( 60 )
    MDRV_SCREEN_VBLANK_TIME( ATTOSECONDS_IN_USEC(1260) )
    MDRV_QUANTUM_TIME( HZ(60) )

    MDRV_MACHINE_START( palm )
    MDRV_MACHINE_RESET( palm )

    /* video hardware */
    MDRV_VIDEO_ATTRIBUTES( VIDEO_UPDATE_BEFORE_VBLANK )
    MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
    MDRV_SCREEN_SIZE( 160, 220 )
    MDRV_SCREEN_VISIBLE_AREA( 0, 159, 0, 219 )
    MDRV_PALETTE_LENGTH( 2 )
    MDRV_PALETTE_INIT( mc68328 )
    MDRV_DEFAULT_LAYOUT(layout_lcd)

    MDRV_VIDEO_START( mc68328 )
    MDRV_VIDEO_UPDATE( mc68328 )

    /* audio hardware */
    MDRV_SPEAKER_STANDARD_MONO("mono")
    MDRV_SOUND_ADD("dac", DAC, 0)
    MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

    MDRV_MC68328_ADD( palm_dragonball_iface )

MACHINE_DRIVER_END

static INPUT_PORTS_START( palm )
    PORT_START( "PENX" )
    PORT_BIT( 0xff, 0x50, IPT_LIGHTGUN_X ) PORT_NAME("Pen X") PORT_MINMAX(0, 0xa0) PORT_SENSITIVITY(50) PORT_CROSSHAIR(X, 1.0, 0.0, 0)

    PORT_START( "PENY" )
    PORT_BIT( 0xff, 0x50, IPT_LIGHTGUN_Y ) PORT_NAME("Pen Y") PORT_MINMAX(0, 0xa0) PORT_SENSITIVITY(50) PORT_CROSSHAIR(Y, 1.0, 0.0, 0)

    PORT_START( "PENB" )
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Pen Button") PORT_CODE(MOUSECODE_BUTTON1) PORT_CHANGED(pen_check, NULL)

    PORT_START( "PORTD" )
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Power") PORT_CODE(KEYCODE_D)   PORT_CHANGED(button_check, (void*)0)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Up") PORT_CODE(KEYCODE_Y)      PORT_CHANGED(button_check, (void*)1)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Down") PORT_CODE(KEYCODE_H)    PORT_CHANGED(button_check, (void*)2)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Button 1") PORT_CODE(KEYCODE_F)   PORT_CHANGED(button_check, (void*)3)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Button 2") PORT_CODE(KEYCODE_G)   PORT_CHANGED(button_check, (void*)4)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Button 3") PORT_CODE(KEYCODE_J)   PORT_CHANGED(button_check, (void*)5)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_NAME("Button 4") PORT_CODE(KEYCODE_K)   PORT_CHANGED(button_check, (void*)6)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

#define PALM_68328_BIOS \
    ROM_REGION16_BE( 0x208000, "bios", 0 )  \
    ROM_SYSTEM_BIOS( 0, "1.0e", "Palm OS 1.0 (English)" )   \
    ROMX_LOAD( "palmos10-en.rom", 0x000000, 0x080000, CRC(82030062) SHA1(00d85c6a0588133cc4651555e9605a61fc1901fc), ROM_GROUPWORD | ROM_BIOS(1) ) \
    ROM_SYSTEM_BIOS( 1, "2.0eper", "Palm OS 2.0 Personal (English)" ) \
    ROMX_LOAD( "palmos20-en-pers.rom", 0x000000, 0x100000, CRC(40ea8baa) SHA1(8e26e213de42da1317c375fb1f394bb945b9d178), ROM_GROUPWORD | ROM_BIOS(2) ) \
    ROM_SYSTEM_BIOS( 2, "2.0epro", "Palm OS 2.0 Professional (English)" ) \
    ROMX_LOAD( "palmos20-en-pro.rom", 0x000000, 0x100000, CRC(baa5b36a) SHA1(535bd9548365d300f85f514f318460443a021476), ROM_GROUPWORD | ROM_BIOS(3) ) \
    ROM_SYSTEM_BIOS( 3, "2.0eprod", "Palm OS 2.0 Professional (English, Debug)" ) \
    ROMX_LOAD( "palmis20-en-pro-dbg.rom", 0x000000, 0x100000, CRC(0d1d3a3b) SHA1(f18a80baa306d4d46b490589ee9a2a5091f6081c), ROM_GROUPWORD | ROM_BIOS(4) ) \
    ROM_SYSTEM_BIOS( 4, "3.0e", "Palm OS 3.0 (English)" ) \
    ROMX_LOAD( "palmos30-en.rom", 0x008000, 0x200000, CRC(6f461f3d) SHA1(7fbf592b4dc8c222be510f6cfda21d48ebe22413), ROM_GROUPWORD | ROM_BIOS(5) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 5, "3.0ed", "Palm OS 3.0 (English, Debug)" ) \
    ROMX_LOAD( "palmos30-en-dbg.rom", 0x008000, 0x200000, CRC(4deda226) SHA1(1c67d6fee2b6a4acd51cda6ef3490305730357ad), ROM_GROUPWORD | ROM_BIOS(6) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 6, "3.0g", "Palm OS 3.0 (German)" ) \
    ROMX_LOAD( "palmos30-de.rom", 0x008000, 0x200000, CRC(b991d6c3) SHA1(73e7539517b0d931e9fa99d6f6914ad46fb857b4), ROM_GROUPWORD | ROM_BIOS(7) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 7, "3.0f", "Palm OS 3.0 (French)" ) \
    ROMX_LOAD( "palmos30-fr.rom", 0x008000, 0x200000, CRC(a2a9ff6c) SHA1(7cb119f896017e76e4680510bee96207d9d28e44), ROM_GROUPWORD | ROM_BIOS(8) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 8, "3.0s", "Palm OS 3.0 (Spanish)" ) \
    ROMX_LOAD( "palmos30-sp.rom", 0x008000, 0x200000, CRC(63a595be) SHA1(f6e03a2fedf0cbe6228613f50f8e8717e797877d), ROM_GROUPWORD | ROM_BIOS(9) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 9, "3.3e", "Palm OS 3.3 (English)" ) \
    ROMX_LOAD( "palmos33-en-iii.rom", 0x008000, 0x200000, CRC(1eae0253) SHA1(e4626f1d33eca8368284d906b2152dcd28b71bbd), ROM_GROUPWORD | ROM_BIOS(10) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 10, "3.3f", "Palm OS 3.3 (French)" ) \
    ROMX_LOAD( "palmos33-fr-iii.rom", 0x008000, 0x200000, CRC(d7894f5f) SHA1(c7c90df814d4f97958194e0bc28c595e967a4529), ROM_GROUPWORD | ROM_BIOS(11) ) \
    ROM_RELOAD(0x000000, 0x004000)	\
    ROM_SYSTEM_BIOS( 11, "3.3g", "Palm OS 3.3 (German)" ) \
    ROMX_LOAD( "palmos33-de-iii.rom", 0x008000, 0x200000, CRC(a5a99c45) SHA1(209b0154942dab80b56d5e6e68fa20b9eb75f5fe), ROM_GROUPWORD | ROM_BIOS(12) ) \
    ROM_RELOAD(0x000000, 0x004000)

ROM_START( pilot1k )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "1.0e" )
ROM_END

ROM_START( pilot5k )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "1.0e" )
ROM_END

ROM_START( palmpers )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "2.0eper" )
ROM_END

ROM_START( palmpro )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "2.0epro" )
ROM_END

ROM_START( palmiii )
    PALM_68328_BIOS
    ROM_DEFAULT_BIOS( "3.0e" )
ROM_END

ROM_START( palmv )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.1e", "Palm OS 3.1 (English)" )
    ROMX_LOAD( "palmv31-en.rom", 0x008000, 0x200000, CRC(4656b2ae) SHA1(ec66a93441fbccfd8e0c946baa5d79c478c83e85), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 1, "3.1g", "Palm OS 3.1 (German)" )
    ROMX_LOAD( "palmv31-de.rom", 0x008000, 0x200000, CRC(a9631dcf) SHA1(63b44d4d3fc2f2196c96d3b9b95da526df0fac77), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 2, "3.1f", "Palm OS 3.1 (French)" )
    ROMX_LOAD( "palmv31-fr.rom", 0x008000, 0x200000, CRC(0d933a1c) SHA1(d0454f1159705d0886f8a68e1b8a5e96d2ca48f6), ROM_GROUPWORD | ROM_BIOS(3) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 3, "3.1s", "Palm OS 3.1 (Spanish)" )
    ROMX_LOAD( "palmv31-sp.rom", 0x008000, 0x200000, CRC(cc46ca1f) SHA1(93bc78ca84d34916d7e122b745adec1068230fcd), ROM_GROUPWORD | ROM_BIOS(4) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 4, "3.1j", "Palm OS 3.1 (Japanese)" )
	ROMX_LOAD( "palmv31-jp.rom", 0x008000, 0x200000, CRC(c786db12) SHA1(4975ff2af76892370c5d4d7d6fa87a84480e79d6), ROM_GROUPWORD | ROM_BIOS(5) )
    ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 5, "3.1e2", "Palm OS 3.1 (English) v2" )
    ROMX_LOAD( "palmv31-en-2.rom", 0x008000, 0x200000, CRC(caced2bd) SHA1(95970080601f72a77a4c338203ed8809fab17abf), ROM_GROUPWORD | ROM_BIOS(6) )
	ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "3.1e2" )
ROM_END

ROM_START( palmvx )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.3e", "Palm OS 3.3 (English)" )
    ROMX_LOAD( "palmvx33-en.rom", 0x000000, 0x200000, CRC(3fc0cc6d) SHA1(6873d5fa99ac372f9587c769940c9b3ac1745a0a), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_SYSTEM_BIOS( 1, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmvx40-en.rom", 0x000000, 0x200000, CRC(488e4638) SHA1(10a10fc8617743ebd5df19c1e99ca040ac1da4f5), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_SYSTEM_BIOS( 2, "4.1e", "Palm OS 4.1 (English)" )
    ROMX_LOAD( "palmvx41-en.rom", 0x000000, 0x200000, CRC(e59f4dff) SHA1(5e3000db318eeb8cd1f4d9729d0c9ebca560fa4a), ROM_GROUPWORD | ROM_BIOS(3) )
    ROM_DEFAULT_BIOS( "4.1e" )
ROM_END

ROM_START( palmiiic )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
	ROM_SYSTEM_BIOS( 0, "3.5eb", "Palm OS 3.5 (English) beta" )
	ROMX_LOAD( "palmiiic350-en-beta.rom", 0x008000, 0x200000, CRC(d58521a4) SHA1(508742ea1e078737666abd4283cf5e6985401c9e), ROM_GROUPWORD | ROM_BIOS(1) )
	ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 1, "3.5eb", "Palm OS 3.5 (Chinese)" )
    ROMX_LOAD( "palmiiic350-ch.rom", 0x008000, 0x200000, CRC(a9779f3a) SHA1(1541102cd5234665233072afe8f0e052134a5334), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 2, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmiiic40-en.rom", 0x008000, 0x200000, CRC(6b2a5ad2) SHA1(54321dcaedcc80de57a819cfd599d8d1b2e26eeb), ROM_GROUPWORD | ROM_BIOS(3) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.0e" )
ROM_END

ROM_START( palmm100 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.51e", "Palm OS 3.5.1 (English)" )
    ROMX_LOAD( "palmm100-351-en.rom", 0x008000, 0x200000, CRC(ae8dda60) SHA1(c46248d6f05cb2f4337985610cedfbdc12ac47cf), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "3.51e" )
ROM_END

ROM_START( palmm130 )
	ROM_REGION16_BE( 0x408000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmm130-40-en.rom", 0x008000, 0x400000, CRC(58046b7e) SHA1(986057010d62d5881fba4dede2aba0d4d5008b16), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.0e" )
ROM_END

ROM_START( palmm505 )
	ROM_REGION16_BE( 0x408000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.0e", "Palm OS 4.0 (English)" )
    ROMX_LOAD( "palmos40-en-m505.rom", 0x008000, 0x400000, CRC(822a4679) SHA1(a4f5e9f7edb1926647ea07969200c5c5e1521bdf), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_SYSTEM_BIOS( 1, "4.1e", "Palm OS 4.1 (English)" )
    ROMX_LOAD( "palmos41-en-m505.rom", 0x008000, 0x400000, CRC(d248202a) SHA1(65e1bd08b244c589b4cd10fe573e0376aba90e5f), ROM_GROUPWORD | ROM_BIOS(2) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.1e" )
ROM_END

ROM_START( palmm515 )
	ROM_REGION16_BE( 0x408000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.1e", "Palm OS 4.1 (English)" )
    ROMX_LOAD( "palmos41-en-m515.rom", 0x008000, 0x400000, CRC(6e143436) SHA1(a0767ea26cc493a3f687525d173903fef89f1acb), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "4.0e" )
ROM_END

ROM_START( visor )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "3.52e", "Palm OS 3.5.2 (English)" )
    ROMX_LOAD( "visor-352-en.rom", 0x008000, 0x200000, CRC(c9e55271) SHA1(749e9142f4480114c5e0d7f21ea354df7273ac5b), ROM_GROUPWORD | ROM_BIOS(1) )
    ROM_RELOAD(0x000000, 0x004000)
    ROM_DEFAULT_BIOS( "3.52e" )
ROM_END

ROM_START( spt1500 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
    ROM_SYSTEM_BIOS( 0, "4.1pim", "Version 4.1 (pim)" )
  	ROMX_LOAD( "spt1500v41-pim.rom",      0x008000, 0x200000, CRC(29e50eaf) SHA1(3e920887bdf74f8f83935977b02f22d5217723eb), ROM_GROUPWORD | ROM_BIOS(1) )
  	ROM_RELOAD(0x000000, 0x004000)
  	ROM_SYSTEM_BIOS( 1, "4.1pim", "Version 4.1 (pimnoft)" )
  	ROMX_LOAD( "spt1500v41-pimnoft.rom",  0x008000, 0x200000, CRC(4b44f284) SHA1(4412e946444706628b94d2303b02580817e1d370), ROM_GROUPWORD | ROM_BIOS(2) )
  	ROM_RELOAD(0x000000, 0x004000)
  	ROM_SYSTEM_BIOS( 2, "4.1pim", "Version 4.1 (nopimnoft)" )
  	ROMX_LOAD( "spt1500v41-nopimnoft.rom",0x008000, 0x200000, CRC(4ba19190) SHA1(d713c1390b82eb4e5fbb39aa10433757c5c49e02), ROM_GROUPWORD | ROM_BIOS(3) )
  	ROM_RELOAD(0x000000, 0x004000)
ROM_END

ROM_START( spt1700 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
	ROM_SYSTEM_BIOS( 0, "1.03pim", "Version 1.03 (pim)" )
	ROMX_LOAD( "spt1700v103-pim.rom",     0x008000, 0x200000, CRC(9df4ee50) SHA1(243a19796f15219cbd73e116f7dfb236b3d238cd), ROM_GROUPWORD | ROM_BIOS(1) )
	ROM_RELOAD(0x000000, 0x004000)
ROM_END

ROM_START( spt1740 )
	ROM_REGION16_BE( 0x208000, "bios", 0 )
	ROM_SYSTEM_BIOS( 0, "1.03pim", "Version 1.03 (pim)" )
	ROMX_LOAD( "spt1740v103-pim.rom",     0x008000, 0x200000, CRC(c29f341c) SHA1(b56d7f8a0c15b1105972e24ed52c846b5e27b195), ROM_GROUPWORD | ROM_BIOS(1) )
	ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 1, "1.03pim", "Version 1.03 (pimnoft)" )
	ROMX_LOAD( "spt1740v103-pimnoft.rom", 0x008000, 0x200000, CRC(b2d49d5c) SHA1(c133dc021b6797cdb93b666c5b315b00b5bb0917), ROM_GROUPWORD | ROM_BIOS(2) )
	ROM_RELOAD(0x000000, 0x004000)
	ROM_SYSTEM_BIOS( 2, "1.03pim", "Version 1.03 (nopim)" )
	ROMX_LOAD( "spt1740v103-nopim.rom",   0x008000, 0x200000, CRC(8ea7e652) SHA1(2a4b5d6a426e627b3cb82c47109cfe2497eba29a), ROM_GROUPWORD | ROM_BIOS(3) )
	ROM_RELOAD(0x000000, 0x004000)
ROM_END

static MACHINE_DRIVER_START( pilot1k )
	MDRV_IMPORT_FROM(palm)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("512K,1M,2M,4M,8M")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pilot5k )
	MDRV_IMPORT_FROM(palm)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("512K")
	MDRV_RAM_EXTRA_OPTIONS("1M,2M,4M,8M")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( palmpro )
	MDRV_IMPORT_FROM(palm)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1M")
	MDRV_RAM_EXTRA_OPTIONS("2M,4M,8M")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( palmiii )
	MDRV_IMPORT_FROM(palm)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("2M")
	MDRV_RAM_EXTRA_OPTIONS("4M,8M")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( palmv )
	MDRV_IMPORT_FROM(palm)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("2M")
	MDRV_RAM_EXTRA_OPTIONS("4M,8M")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( palmvx )
	MDRV_IMPORT_FROM(palm)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("8M")
MACHINE_DRIVER_END

/*    YEAR  NAME      PARENT    COMPAT   MACHINE   INPUT     INIT      CONFIG    COMPANY FULLNAME */
COMP( 1996, pilot1k,  0,        0,       pilot1k,     palm,     palm,     0,  "U.S. Robotics", "Pilot 1000", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1996, pilot5k,  pilot1k,  0,       pilot5k,     palm,     palm,     0,  "U.S. Robotics", "Pilot 5000", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1997, palmpers, pilot1k,  0,       pilot5k,     palm,     palm,     0,  "U.S. Robotics", "Palm Pilot Personal", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1997, palmpro,  pilot1k,  0,       palmpro,     palm,     palm,     0,  "U.S. Robotics", "Palm Pilot Pro", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1998, palmiii,  pilot1k,  0,       palmiii,     palm,     palm,     0,  "3Com", "Palm III", GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
COMP( 1998, palmiiic, pilot1k,  0,       palmiii,     palm,     palm,     0,  "Palm Inc.", "Palm IIIc", GAME_NOT_WORKING )
COMP( 2000, palmm100, pilot1k,  0,       palmiii,     palm,     palm,     0,  "Palm Inc.", "Palm m100", GAME_NOT_WORKING )
COMP( 2000, palmm130, pilot1k,  0,       palmiii,     palm,     palm,     0,  "Palm Inc.", "Palm m130", GAME_NOT_WORKING )
COMP( 2001, palmm505, pilot1k,  0,       palmiii,     palm,     palm,     0,  "Palm Inc.", "Palm m505", GAME_NOT_WORKING )
COMP( 2001, palmm515, pilot1k,  0,       palmiii,     palm,     palm,     0,  "Palm Inc.", "Palm m515", GAME_NOT_WORKING )
COMP( 1999, palmv,    pilot1k,  0,       palmv,       palm,     palm,     0,  	 "3Com", "Palm V", GAME_NOT_WORKING )
COMP( 1999, palmvx,   pilot1k,  0,       palmvx,      palm,     palm,     0,   "Palm Inc.", "Palm Vx", GAME_NOT_WORKING )
COMP( 2001, visor,    pilot1k,  0,       palmvx,      palm,     palm,     0,   "Handspring", "Visor Edge", GAME_NOT_WORKING )
COMP( 19??, spt1500,  pilot1k,  0,       palmvx,      palm,     palm,     0,   "Symbol", "SPT 1500", GAME_NOT_WORKING )
COMP( 19??, spt1700,  pilot1k,  0,       palmvx,      palm,     palm,     0,   "Symbol", "SPT 1700", GAME_NOT_WORKING )
COMP( 19??, spt1740,  pilot1k,  0,       palmvx,      palm,     palm,     0,   "Symbol", "SPT 1740", GAME_NOT_WORKING )

#include "palm_dbg.c"
