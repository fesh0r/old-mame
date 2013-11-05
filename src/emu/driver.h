// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    driver.h

    Core driver device base class.

***************************************************************************/

#pragma once

#ifndef __EMU_H__
#error Dont include this file directly; include emu.h instead.
#endif

#ifndef __DRIVER_H__
#define __DRIVER_H__


//**************************************************************************
//  CONFIGURATION MACROS
//**************************************************************************

// core machine callbacks
#define MCFG_MACHINE_START_OVERRIDE(_class, _func) \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_MACHINE_START, driver_callback_delegate(&_class::MACHINE_START_NAME(_func), #_class "::machine_start_" #_func, downcast<_class *>(owner)));

#define MCFG_MACHINE_RESET_OVERRIDE(_class, _func) \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_MACHINE_RESET, driver_callback_delegate(&_class::MACHINE_RESET_NAME(_func), #_class "::machine_reset_" #_func, downcast<_class *>(owner)));

#define MCFG_MACHINE_RESET_REMOVE() \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_MACHINE_RESET, driver_callback_delegate());

// core sound callbacks
#define MCFG_SOUND_START_OVERRIDE(_class, _func) \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_SOUND_START, driver_callback_delegate(&_class::SOUND_START_NAME(_func), #_class "::sound_start_" #_func, downcast<_class *>(owner)));

#define MCFG_SOUND_RESET_OVERRIDE(_class, _func) \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_SOUND_RESET, driver_callback_delegate(&_class::SOUND_RESET_NAME(_func), #_class "::sound_reset_" #_func, downcast<_class *>(owner)));


// core video callbacks
#define MCFG_PALETTE_INIT_OVERRIDE(_class, _func) \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_PALETTE_INIT, driver_callback_delegate(&_class::PALETTE_INIT_NAME(_func), #_class "::palette_init_" #_func, downcast<_class *>(owner)));

#define MCFG_VIDEO_START_OVERRIDE(_class, _func) \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_VIDEO_START, driver_callback_delegate(&_class::VIDEO_START_NAME(_func), #_class "::video_start_" #_func, downcast<_class *>(owner)));

#define MCFG_VIDEO_RESET_OVERRIDE(_class, _func) \
	driver_device::static_set_callback(config.root_device(), driver_device::CB_VIDEO_RESET, driver_callback_delegate(&_class::VIDEO_RESET_NAME(_func), #_class "::video_reset_" #_func, downcast<_class *>(owner)));



//**************************************************************************
//  OTHER MACROS
//**************************************************************************

#define MACHINE_START_NAME(name)    machine_start_##name
#define MACHINE_START(name)         void MACHINE_START_NAME(name)(running_machine &machine) // legacy
#define MACHINE_START_CALL_MEMBER(name) MACHINE_START_NAME(name)()
#define MACHINE_START_CALL_LEGACY(name) MACHINE_START_NAME(name)(machine())
#define DECLARE_MACHINE_START(name) void MACHINE_START_NAME(name)()
#define MACHINE_START_MEMBER(cls,name) void cls::MACHINE_START_NAME(name)()

#define MACHINE_RESET_NAME(name)    machine_reset_##name
#define MACHINE_RESET(name)         void MACHINE_RESET_NAME(name)(running_machine &machine) // legacy
#define MACHINE_RESET_CALL_MEMBER(name) MACHINE_RESET_NAME(name)()
#define MACHINE_RESET_CALL_LEGACY(name) MACHINE_RESET_NAME(name)(machine())
#define DECLARE_MACHINE_RESET(name) void MACHINE_RESET_NAME(name)()
#define MACHINE_RESET_MEMBER(cls,name) void cls::MACHINE_RESET_NAME(name)()

#define SOUND_START_NAME(name)      sound_start_##name
#define DECLARE_SOUND_START(name)   void SOUND_START_NAME(name)()
#define SOUND_START_MEMBER(cls,name) void cls::SOUND_START_NAME(name)()

#define SOUND_RESET_NAME(name)      sound_reset_##name
#define SOUND_RESET_CALL_MEMBER(name) SOUND_RESET_NAME(name)()
#define DECLARE_SOUND_RESET(name)   void SOUND_RESET_NAME(name)()
#define SOUND_RESET_MEMBER(cls,name) void cls::SOUND_RESET_NAME(name)()

#define PALETTE_INIT_NAME(name)     palette_init_##name
#define PALETTE_INIT(name)          void PALETTE_INIT_NAME(name)(running_machine &machine) // legacy
#define PALETTE_INIT_CALL_MEMBER(name)      PALETTE_INIT_NAME(name)()
#define DECLARE_PALETTE_INIT(name)  void PALETTE_INIT_NAME(name)()
#define PALETTE_INIT_MEMBER(cls,name) void cls::PALETTE_INIT_NAME(name)()

#define VIDEO_START_NAME(name)      video_start_##name
#define VIDEO_START(name)           void VIDEO_START_NAME(name)(running_machine &machine) // legacy
#define VIDEO_START_CALL_MEMBER(name)       VIDEO_START_NAME(name)()
#define VIDEO_START_CALL_LEGACY(name)       VIDEO_START_NAME(name)(machine())
#define DECLARE_VIDEO_START(name)   void VIDEO_START_NAME(name)()
#define VIDEO_START_MEMBER(cls,name) void cls::VIDEO_START_NAME(name)()

#define VIDEO_RESET_NAME(name)      video_reset_##name
#define VIDEO_RESET_CALL_MEMBER(name)       VIDEO_RESET_NAME(name)()
#define DECLARE_VIDEO_RESET(name)   void VIDEO_RESET_NAME(name)()
#define VIDEO_RESET_MEMBER(cls,name) void cls::VIDEO_RESET_NAME(name)()



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

typedef delegate<void ()> driver_callback_delegate;

// legacy callback functions
/*ATTR_DEPRECATED*/ typedef void (*legacy_callback_func)(running_machine &machine);


// ======================> driver_device

// base class for machine driver-specific devices
class driver_device :   public device_t,
						public device_memory_interface
{
public:
	// construction/destruction
	driver_device(const machine_config &mconfig, device_type type, const char *tag);
	virtual ~driver_device();

	// getters
	const game_driver &system() const { assert(m_system != NULL); return *m_system; }

	// indexes into our generic callbacks
	enum callback_type
	{
		CB_MACHINE_START,
		CB_MACHINE_RESET,
		CB_SOUND_START,
		CB_SOUND_RESET,
		CB_PALETTE_INIT,
		CB_VIDEO_START,
		CB_VIDEO_RESET,
		CB_COUNT
	};

	// inline configuration helpers
	static void static_set_game(device_t &device, const game_driver &game);
	ATTR_DEPRECATED static void static_set_callback(device_t &device, callback_type type, legacy_callback_func callback);
	static void static_set_callback(device_t &device, callback_type type, driver_callback_delegate callback);

	// generic helpers
	template<class _DriverClass, void (_DriverClass::*_Function)()>
	static void driver_init_wrapper(running_machine &machine)
	{
		(machine.driver_data<_DriverClass>()->*_Function)();
	}

	// dummy driver_init callbacks
	void init_0() { }

	// memory helpers
	address_space &generic_space() const { return space(AS_PROGRAM); }

	// generic interrupt generators
	void generic_pulse_irq_line(device_execute_interface &exec, int irqline, int cycles);
	void generic_pulse_irq_line_and_vector(device_execute_interface &exec, int irqline, int vector, int cycles);

	INTERRUPT_GEN_MEMBER( nmi_line_pulse );
	INTERRUPT_GEN_MEMBER( nmi_line_assert );

	INTERRUPT_GEN_MEMBER( irq0_line_hold );
	INTERRUPT_GEN_MEMBER( irq0_line_pulse );
	INTERRUPT_GEN_MEMBER( irq0_line_assert );

	INTERRUPT_GEN_MEMBER( irq1_line_hold );
	INTERRUPT_GEN_MEMBER( irq1_line_pulse );
	INTERRUPT_GEN_MEMBER( irq1_line_assert );

	INTERRUPT_GEN_MEMBER( irq2_line_hold );
	INTERRUPT_GEN_MEMBER( irq2_line_pulse );
	INTERRUPT_GEN_MEMBER( irq2_line_assert );

	INTERRUPT_GEN_MEMBER( irq3_line_hold );
	INTERRUPT_GEN_MEMBER( irq3_line_pulse );
	INTERRUPT_GEN_MEMBER( irq3_line_assert );

	INTERRUPT_GEN_MEMBER( irq4_line_hold );
	INTERRUPT_GEN_MEMBER( irq4_line_pulse );
	INTERRUPT_GEN_MEMBER( irq4_line_assert );

	INTERRUPT_GEN_MEMBER( irq5_line_hold );
	INTERRUPT_GEN_MEMBER( irq5_line_pulse );
	INTERRUPT_GEN_MEMBER( irq5_line_assert );

	INTERRUPT_GEN_MEMBER( irq6_line_hold );
	INTERRUPT_GEN_MEMBER( irq6_line_pulse );
	INTERRUPT_GEN_MEMBER( irq6_line_assert );

	INTERRUPT_GEN_MEMBER( irq7_line_hold );
	INTERRUPT_GEN_MEMBER( irq7_line_pulse );
	INTERRUPT_GEN_MEMBER( irq7_line_assert );

	// watchdog read/write handlers
	DECLARE_WRITE8_MEMBER( watchdog_reset_w );
	DECLARE_READ8_MEMBER( watchdog_reset_r );
	DECLARE_WRITE16_MEMBER( watchdog_reset16_w );
	DECLARE_READ16_MEMBER( watchdog_reset16_r );
	DECLARE_WRITE32_MEMBER( watchdog_reset32_w );
	DECLARE_READ32_MEMBER( watchdog_reset32_r );

	// generic audio
	void soundlatch_setclearedvalue(UINT16 value) { m_latch_clear_value = value; }

	// sound latch readers
	UINT32 soundlatch_read(UINT8 index = 0);
	DECLARE_READ8_MEMBER( soundlatch_byte_r );
	DECLARE_READ8_MEMBER( soundlatch2_byte_r );
	DECLARE_READ8_MEMBER( soundlatch3_byte_r );
	DECLARE_READ8_MEMBER( soundlatch4_byte_r );
	DECLARE_READ16_MEMBER( soundlatch_word_r );
	DECLARE_READ16_MEMBER( soundlatch2_word_r );
	DECLARE_READ16_MEMBER( soundlatch3_word_r );
	DECLARE_READ16_MEMBER( soundlatch4_word_r );

	// sound latch writers
	void soundlatch_write(UINT8 index, UINT32 data);
	void soundlatch_write(UINT32 data) { soundlatch_write(0, data); }
	DECLARE_WRITE8_MEMBER( soundlatch_byte_w );
	DECLARE_WRITE8_MEMBER( soundlatch2_byte_w );
	DECLARE_WRITE8_MEMBER( soundlatch3_byte_w );
	DECLARE_WRITE8_MEMBER( soundlatch4_byte_w );
	DECLARE_WRITE16_MEMBER( soundlatch_word_w );
	DECLARE_WRITE16_MEMBER( soundlatch2_word_w );
	DECLARE_WRITE16_MEMBER( soundlatch3_word_w );
	DECLARE_WRITE16_MEMBER( soundlatch4_word_w );

	// sound latch clearers
	void soundlatch_clear(UINT8 index = 0);
	DECLARE_WRITE8_MEMBER( soundlatch_clear_byte_w );
	DECLARE_WRITE8_MEMBER( soundlatch2_clear_byte_w );
	DECLARE_WRITE8_MEMBER( soundlatch3_clear_byte_w );
	DECLARE_WRITE8_MEMBER( soundlatch4_clear_byte_w );

	// generic video
	void flip_screen_set(UINT32 on);
	void flip_screen_set_no_update(UINT32 on);
	void flip_screen_x_set(UINT32 on);
	void flip_screen_y_set(UINT32 on);
	UINT32 flip_screen() const { return m_flip_screen_x; }
	UINT32 flip_screen_x() const { return m_flip_screen_x; }
	UINT32 flip_screen_y() const { return m_flip_screen_y; }

	// templatized palette writers for 8-bit palette data
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE8_MEMBER( palette_8bit_byte_w );

	// templatized palette writers for 16-bit palette data
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE8_MEMBER( palette_16bit_byte_le_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE8_MEMBER( palette_16bit_byte_be_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE8_MEMBER( palette_16bit_byte_split_lo_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE8_MEMBER( palette_16bit_byte_split_hi_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE16_MEMBER( palette_16bit_word_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE32_MEMBER( palette_16bit_dword_le_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE32_MEMBER( palette_16bit_dword_be_w );

	// templatized palette writers for 32-bit palette data
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE16_MEMBER( palette_32bit_word_le_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE16_MEMBER( palette_32bit_word_be_w );
	template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift> DECLARE_WRITE32_MEMBER( palette_32bit_dword_w );

	// 3-3-2 RGB palette write handlers
	DECLARE_WRITE8_MEMBER( paletteram_BBGGGRRR_byte_w );
	DECLARE_WRITE8_MEMBER( paletteram_RRRGGGBB_byte_w );
	DECLARE_WRITE8_MEMBER( paletteram_BBGGRRII_byte_w );

	// 4-4-4 RGB palette write handlers
	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBGGGGRRRR_byte_le_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBGGGGRRRR_byte_be_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBGGGGRRRR_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBGGGGRRRR_byte_split_hi_w );
	DECLARE_WRITE16_MEMBER( paletteram_xxxxBBBBGGGGRRRR_word_w );

	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBRRRRGGGG_byte_le_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBRRRRGGGG_byte_be_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBRRRRGGGG_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxBBBBRRRRGGGG_byte_split_hi_w );
	DECLARE_WRITE16_MEMBER( paletteram_xxxxBBBBRRRRGGGG_word_w );

	DECLARE_WRITE8_MEMBER( paletteram_xxxxRRRRBBBBGGGG_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxRRRRBBBBGGGG_byte_split_hi_w );

	DECLARE_WRITE8_MEMBER( paletteram_xxxxRRRRGGGGBBBB_byte_le_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxRRRRGGGGBBBB_byte_be_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxRRRRGGGGBBBB_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_xxxxRRRRGGGGBBBB_byte_split_hi_w );
	DECLARE_WRITE16_MEMBER( paletteram_xxxxRRRRGGGGBBBB_word_w );

	DECLARE_WRITE8_MEMBER( paletteram_RRRRGGGGBBBBxxxx_byte_be_w );
	DECLARE_WRITE8_MEMBER( paletteram_RRRRGGGGBBBBxxxx_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_RRRRGGGGBBBBxxxx_byte_split_hi_w );
	DECLARE_WRITE16_MEMBER( paletteram_RRRRGGGGBBBBxxxx_word_w );

	// 4-4-4-4 IRGB palette write handlers
	DECLARE_WRITE16_MEMBER( paletteram_IIIIRRRRGGGGBBBB_word_w );
	DECLARE_WRITE16_MEMBER( paletteram_RRRRGGGGBBBBIIII_word_w );

	// 5-5-5 RGB palette write handlers
	DECLARE_WRITE8_MEMBER( paletteram_xBBBBBGGGGGRRRRR_byte_le_w );
	DECLARE_WRITE8_MEMBER( paletteram_xBBBBBGGGGGRRRRR_byte_be_w );
	DECLARE_WRITE8_MEMBER( paletteram_xBBBBBGGGGGRRRRR_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_xBBBBBGGGGGRRRRR_byte_split_hi_w );
	DECLARE_WRITE16_MEMBER( paletteram_xBBBBBGGGGGRRRRR_word_w );

	DECLARE_WRITE8_MEMBER( paletteram_xBBBBBRRRRRGGGGG_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_xBBBBBRRRRRGGGGG_byte_split_hi_w );

	DECLARE_WRITE8_MEMBER( paletteram_xRRRRRGGGGGBBBBB_byte_le_w );
	DECLARE_WRITE8_MEMBER( paletteram_xRRRRRGGGGGBBBBB_byte_be_w );
	DECLARE_WRITE8_MEMBER( paletteram_xRRRRRGGGGGBBBBB_byte_split_lo_w );
	DECLARE_WRITE8_MEMBER( paletteram_xRRRRRGGGGGBBBBB_byte_split_hi_w );
	DECLARE_WRITE16_MEMBER( paletteram_xRRRRRGGGGGBBBBB_word_w );
	DECLARE_WRITE32_MEMBER( paletteram_xRRRRRGGGGGBBBBB_dword_be_w );
	DECLARE_WRITE32_MEMBER( paletteram_xRRRRRGGGGGBBBBB_dword_le_w );

	DECLARE_WRITE8_MEMBER( paletteram_xGGGGGRRRRRBBBBB_byte_le_w );

	DECLARE_WRITE16_MEMBER( paletteram_xGGGGGRRRRRBBBBB_word_w );
	DECLARE_WRITE16_MEMBER( paletteram_xGGGGGBBBBBRRRRR_word_w );
	DECLARE_WRITE16_MEMBER( paletteram_RRRRRGGGGGBBBBBx_word_w );
	DECLARE_WRITE16_MEMBER( paletteram_GGGGGRRRRRBBBBBx_word_w );
	DECLARE_WRITE16_MEMBER( paletteram_RRRRGGGGBBBBRGBx_word_w );

	// 8-8-8 RGB palette write handlers
	DECLARE_WRITE16_MEMBER( paletteram_xrgb_word_be_w );
	DECLARE_WRITE16_MEMBER( paletteram_xbgr_word_be_w );

	// generic input port helpers
	DECLARE_CUSTOM_INPUT_MEMBER( custom_port_read );

	// general fatal error handlers
	DECLARE_READ8_MEMBER( fatal_generic_read );
	DECLARE_WRITE8_MEMBER( fatal_generic_write );

	// generic palette init routines
	DECLARE_PALETTE_INIT( all_black );
	DECLARE_PALETTE_INIT( black_and_white );
	DECLARE_PALETTE_INIT( monochrome_amber );
	DECLARE_PALETTE_INIT( monochrome_green );
	DECLARE_PALETTE_INIT( RRRR_GGGG_BBBB );
	DECLARE_PALETTE_INIT( RRRRR_GGGGG_BBBBB );
	DECLARE_PALETTE_INIT( BBBBB_GGGGG_RRRRR );
	DECLARE_PALETTE_INIT( RRRRR_GGGGGG_BBBBB );

protected:
	// helpers called at startup
	virtual void driver_start();
	virtual void machine_start();
	virtual void sound_start();
	virtual void palette_init();
	virtual void video_start();

	// helpers called at reset
	virtual void driver_reset();
	virtual void machine_reset();
	virtual void sound_reset();
	virtual void video_reset();

	// device-level overrides
	virtual const rom_entry *device_rom_region() const;
	virtual ioport_constructor device_input_ports() const;
	virtual void device_start();
	virtual void device_reset_after_children();

	// device_memory_interface overrides
	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const;

	// internal helpers
	inline UINT16 paletteram16_le(offs_t offset) const { return m_generic_paletteram_8[offset & ~1] | (m_generic_paletteram_8[offset |  1] << 8); }
	inline UINT16 paletteram16_be(offs_t offset) const { return m_generic_paletteram_8[offset |  1] | (m_generic_paletteram_8[offset & ~1] << 8); }
	inline UINT16 paletteram16_split(offs_t offset) const { return m_generic_paletteram_8[offset] | (m_generic_paletteram2_8[offset] << 8); }
	inline UINT32 paletteram32_be(offs_t offset) const { return m_generic_paletteram_16[offset | 1] | (m_generic_paletteram_16[offset & ~1] << 16); }

public:
	// generic devices
	optional_device<screen_device> m_screen;

	// generic pointers
	optional_shared_ptr<UINT8> m_generic_paletteram_8;
	optional_shared_ptr<UINT8> m_generic_paletteram2_8;
	optional_shared_ptr<UINT16> m_generic_paletteram_16;
	optional_shared_ptr<UINT16> m_generic_paletteram2_16;
	optional_shared_ptr<UINT32> m_generic_paletteram_32;
	optional_shared_ptr<UINT32> m_generic_paletteram2_32;

private:
	// helpers
	void irq_pulse_clear(void *ptr, INT32 param);
	void soundlatch_sync_callback(void *ptr, INT32 param);
	void updateflip();

	// configuration state
	const address_space_config  m_space_config;

	// internal state
	const game_driver *     m_system;                   // pointer to the game driver
	driver_callback_delegate m_callbacks[CB_COUNT];     // start/reset callbacks
	legacy_callback_func    m_legacy_callbacks[CB_COUNT]; // legacy start/reset callbacks

	// generic audio
	UINT16                  m_latch_clear_value;
	UINT16                  m_latched_value[4];
	UINT8                   m_latch_read[4];

	// generic video
	UINT32                  m_flip_screen_x;
	UINT32                  m_flip_screen_y;
};


// this template function creates a stub which constructs a device
template<class _DriverClass>
device_t *driver_device_creator(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
{
	assert(owner == NULL);
	assert(clock == 0);
	return global_alloc_clear(_DriverClass(mconfig, &driver_device_creator<_DriverClass>, tag));
}



//**************************************************************************
//  PALETTE WRITER TEMPLATES
//**************************************************************************

// write 8-bit palette data
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE8_MEMBER(driver_device::palette_8bit_byte_w)
{
	m_generic_paletteram_8[offset] = data;
	UINT8 paldata = m_generic_paletteram_8[offset];
	palette_set_color_rgb(machine(), offset, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 16-bit palette data to consecutive 8-bit addresses with LSB first
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE8_MEMBER(driver_device::palette_16bit_byte_le_w)
{
	m_generic_paletteram_8[offset] = data;
	UINT16 paldata = m_generic_paletteram_8[offset & ~1] | (m_generic_paletteram_8[offset | 1] << 8);
	palette_set_color_rgb(machine(), offset / 2, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 16-bit palette data to consecutive 8-bit addresses with MSB first
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE8_MEMBER(driver_device::palette_16bit_byte_be_w)
{
	m_generic_paletteram_8[offset] = data;
	UINT16 paldata = m_generic_paletteram_8[offset | 1] | (m_generic_paletteram_8[offset & ~1] << 8);
	palette_set_color_rgb(machine(), offset / 2, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 16-bit palette data to split 8-bit addresses (LSB)
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE8_MEMBER(driver_device::palette_16bit_byte_split_lo_w)
{
	m_generic_paletteram_8[offset] = data;
	UINT16 paldata = m_generic_paletteram_8[offset] | (m_generic_paletteram2_8[offset] << 8);
	palette_set_color_rgb(machine(), offset, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 16-bit palette data to split 8-bit addresses (MSB)
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE8_MEMBER(driver_device::palette_16bit_byte_split_hi_w)
{
	m_generic_paletteram2_8[offset] = data;
	UINT16 paldata = m_generic_paletteram_8[offset] | (m_generic_paletteram2_8[offset] << 8);
	palette_set_color_rgb(machine(), offset, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 16-bit palette data to 16-bit addresses
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE16_MEMBER(driver_device::palette_16bit_word_w)
{
	COMBINE_DATA(&m_generic_paletteram_16[offset]);
	UINT16 paldata = m_generic_paletteram_16[offset];
	palette_set_color_rgb(machine(), offset, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 16-bit palette data to packed 32-bit addresses (lower entry in lower 16 bits)
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE32_MEMBER(driver_device::palette_16bit_dword_le_w)
{
	COMBINE_DATA(&m_generic_paletteram_32[offset]);
	if (ACCESSING_BITS_0_15)
	{
		UINT16 paldata = m_generic_paletteram_32[offset];
		palette_set_color_rgb(machine(), offset * 2 + 0, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_GreenBits>(paldata >> _BlueShift));
	}
	if (ACCESSING_BITS_16_31)
	{
		UINT16 paldata = m_generic_paletteram_32[offset] >> 16;
		palette_set_color_rgb(machine(), offset * 2 + 1, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_GreenBits>(paldata >> _BlueShift));
	}
}

// write 16-bit palette data to packed 32-bit addresses (lower entry in upper 16 bits)
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE32_MEMBER(driver_device::palette_16bit_dword_be_w)
{
	COMBINE_DATA(&m_generic_paletteram_32[offset]);
	if (ACCESSING_BITS_16_31)
	{
		UINT16 paldata = m_generic_paletteram_32[offset] >> 16;
		palette_set_color_rgb(machine(), offset * 2 + 0, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
	}
	if (ACCESSING_BITS_0_15)
	{
		UINT16 paldata = m_generic_paletteram_32[offset];
		palette_set_color_rgb(machine(), offset * 2 + 1, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
	}
}

// write 32-bit palette data to consecutive 16-bit addresses with LSW first
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE16_MEMBER(driver_device::palette_32bit_word_le_w)
{
	COMBINE_DATA(&m_generic_paletteram_16[offset]);
	UINT32 paldata = m_generic_paletteram_16[offset & ~1] | (m_generic_paletteram_16[offset | 1] << 16);
	palette_set_color_rgb(machine(), offset / 2, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 32-bit palette data to consecutive 16-bit addresses with MSW first
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE16_MEMBER(driver_device::palette_32bit_word_be_w)
{
	COMBINE_DATA(&m_generic_paletteram_16[offset]);
	UINT32 paldata = m_generic_paletteram_16[offset | 1] | (m_generic_paletteram_16[offset & ~1] << 16);
	palette_set_color_rgb(machine(), offset / 2, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

// write 32-bit palette data to 32-bit addresses
template<int _RedBits, int _GreenBits, int _BlueBits, int _RedShift, int _GreenShift, int _BlueShift>
WRITE32_MEMBER(driver_device::palette_32bit_dword_w)
{
	COMBINE_DATA(&m_generic_paletteram_32[offset]);
	UINT32 paldata = m_generic_paletteram_32[offset];
	palette_set_color_rgb(machine(), offset, palexpand<_RedBits>(paldata >> _RedShift), palexpand<_GreenBits>(paldata >> _GreenShift), palexpand<_BlueBits>(paldata >> _BlueShift));
}

#endif  /* __DRIVER_H__ */
