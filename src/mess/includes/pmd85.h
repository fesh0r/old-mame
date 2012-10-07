/*****************************************************************************
 *
 * includes/pmd85.h
 *
 ****************************************************************************/

#ifndef PMD85_H_
#define PMD85_H_

#include "machine/i8255.h"

class pmd85_state : public driver_device
{
public:
	pmd85_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 m_rom_module_present;
	UINT8 m_ppi_port_outputs[4][3];
	UINT8 m_startup_mem_map;
	UINT8 m_pmd853_memory_mapping;
	int m_previous_level;
	int m_clk_level;
	int m_clk_level_tape;
	UINT8 m_model;
	emu_timer * m_cassette_timer;
	void (*update_memory)(running_machine &);
	DECLARE_READ8_MEMBER(pmd85_io_r);
	DECLARE_WRITE8_MEMBER(pmd85_io_w);
	DECLARE_READ8_MEMBER(mato_io_r);
	DECLARE_WRITE8_MEMBER(mato_io_w);
	DECLARE_DIRECT_UPDATE_MEMBER(pmd85_opbaseoverride);
	DECLARE_DRIVER_INIT(mato);
	DECLARE_DRIVER_INIT(pmd852a);
	DECLARE_DRIVER_INIT(pmd851);
	DECLARE_DRIVER_INIT(pmd853);
	DECLARE_DRIVER_INIT(alfa);
	DECLARE_DRIVER_INIT(c2717);
	virtual void machine_reset();
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_pmd85(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_CALLBACK_MEMBER(pmd85_cassette_timer_callback);
	TIMER_CALLBACK_MEMBER(pmd_reset);
	TIMER_CALLBACK_MEMBER(setup_machine_state);
	DECLARE_READ8_MEMBER(pmd85_ppi_0_porta_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_0_portb_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_0_portc_r);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_0_porta_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_0_portb_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_0_portc_w);
	DECLARE_READ8_MEMBER(mato_ppi_0_portb_r);
	DECLARE_READ8_MEMBER(mato_ppi_0_portc_r);
	DECLARE_WRITE8_MEMBER(mato_ppi_0_portc_w);
	DECLARE_READ8_MEMBER(pmd85_ppi_1_porta_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_1_portb_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_1_portc_r);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_1_porta_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_1_portb_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_1_portc_w);
	DECLARE_READ8_MEMBER(pmd85_ppi_2_porta_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_2_portb_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_2_portc_r);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_2_porta_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_2_portb_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_2_portc_w);
	DECLARE_READ8_MEMBER(pmd85_ppi_3_porta_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_3_portb_r);
	DECLARE_READ8_MEMBER(pmd85_ppi_3_portc_r);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_3_porta_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_3_portb_w);
	DECLARE_WRITE8_MEMBER(pmd85_ppi_3_portc_w);
};


/*----------- defined in machine/pmd85.c -----------*/

extern const struct pit8253_config pmd85_pit8253_interface;
extern const i8255_interface pmd85_ppi8255_interface[4];
extern const i8255_interface alfa_ppi8255_interface[3];
extern const i8255_interface mato_ppi8255_interface;

/*----------- defined in video/pmd85.c -----------*/

extern const unsigned char pmd85_palette[3*3];


#endif /* PMD85_H_ */
