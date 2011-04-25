/**********************************************************************

    Commodore 2040/3040/4040/8050/8250/SFD-1001 Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C2040__
#define __C2040__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/mos6530.h"
#include "machine/ieee488.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C2040_TAG			"c2040"
#define C3040_TAG			"c3040"
#define C4040_TAG			"c4040"
#define C8050_TAG			"c8050"
#define C8250_TAG			"c8250"
#define SFD1001_TAG			"sfd1001"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C2040_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C2040, 0) \
	c2040_device_config::static_set_config(device, _address, c2040_device_config::TYPE_2040);

#define MCFG_C3040_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C3040, 0) \
	c2040_device_config::static_set_config(device, _address, c2040_device_config::TYPE_3040);

#define MCFG_C4040_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C4040, 0) \
	c2040_device_config::static_set_config(device, _address, c2040_device_config::TYPE_4040);

#define MCFG_C8050_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C8050, 0) \
	c2040_device_config::static_set_config(device, _address, c2040_device_config::TYPE_8050);

#define MCFG_C8250_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C8250, 0) \
	c2040_device_config::static_set_config(device, _address, c2040_device_config::TYPE_8250);

#define MCFG_SFD1001_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, SFD1001, 0) \
	c2040_device_config::static_set_config(device, _address, c2040_device_config::TYPE_SFD1001);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c2040_device_config

class c2040_device_config :   public device_config,
							  public device_config_ieee488_interface
{
    friend class c2040_device;
    friend class c3040_device;
    friend class c4040_device;
    friend class c8050_device;
    friend class c8250_device;
    friend class sfd1001_device;

    // construction/destruction
    c2040_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	enum
	{
		TYPE_2040 = 0,
		TYPE_3040,
		TYPE_4040,
		TYPE_8050,
		TYPE_8250,
		TYPE_SFD1001,
	};

	// allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// inline configuration helpers
	static void static_set_config(device_config *device, int address, int variant);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	// device_config overrides
    virtual void device_config_complete();

	int m_address;
	int m_variant;
};


// ======================> c2040_device

class c2040_device :  public device_t,
					  public device_ieee488_interface
{
    friend class c2040_device_config;

    // construction/destruction
    c2040_device(running_machine &_machine, const c2040_device_config &_config);

public:
	// not really public
	static void on_disk0_change(device_image_interface &image);
	static void on_disk1_change(device_image_interface &image);

	DECLARE_READ8_MEMBER( dio_r );
	DECLARE_WRITE8_MEMBER( dio_w );
	DECLARE_READ8_MEMBER( riot1_pa_r );
	DECLARE_WRITE8_MEMBER( riot1_pa_w );
	DECLARE_READ8_MEMBER( riot1_pb_r );
	DECLARE_WRITE8_MEMBER( riot1_pb_w );
	DECLARE_READ8_MEMBER( via_pa_r );
	DECLARE_WRITE8_MEMBER( via_pb_w );
	DECLARE_READ_LINE_MEMBER( ready_r );
	DECLARE_READ_LINE_MEMBER( err_r );
	DECLARE_WRITE_LINE_MEMBER( mode_sel_w );
	DECLARE_WRITE_LINE_MEMBER( rw_sel_w );
	DECLARE_READ8_MEMBER( pi_r );
	DECLARE_WRITE8_MEMBER( pi_w );
	DECLARE_READ8_MEMBER( miot_pb_r );
	DECLARE_WRITE8_MEMBER( miot_pb_w );
	DECLARE_READ8_MEMBER( c8050_via_pb_r );
	DECLARE_WRITE8_MEMBER( c8050_via_pb_w );
	DECLARE_READ8_MEMBER( c8050_miot_pb_r );
	DECLARE_WRITE8_MEMBER( c8050_miot_pb_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	// device_ieee488_interface overrides
	void ieee488_atn(int state);
	void ieee488_ifc(int state);

	inline void update_ieee_signals();
	inline void update_gcr_data();
	inline void read_current_track(int unit);
	inline void spindle_motor(int unit, int mtr);
	inline void micropolis_step_motor(int unit, int stp);
	inline void mpi_step_motor(int unit, int stp);
	inline void initialize(int drives);

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_fdccpu;
	required_device<riot6532_device> m_riot0;
	required_device<riot6532_device> m_riot1;
	required_device<device_t> m_miot;
	required_device<via6522_device> m_via;
	required_device<device_t> m_image0;
	optional_device<device_t> m_image1;
	required_device<ieee488_device> m_bus;

	struct {
		// motors
		int m_stp;								// stepper motor phase
		int m_mtr;								// spindle motor on

		// track
		UINT8 m_track_buffer[G64_BUFFER_SIZE];	// track data buffer
		int m_track_len;						// track length
		int m_buffer_pos;						// byte position within track buffer
		int m_bit_pos;							// bit position within track buffer byte

		// devices
		device_t *m_image;
	} m_unit[2];

	int m_drive;						// selected drive
	int m_side;							// selected side

	// IEEE-488 bus
	int m_rfdo;							// not ready for data output
	int m_daco;							// not data accepted output
	int m_atna;							// attention acknowledge

	// track
	int m_ds;							// density select
	int m_bit_count;					// GCR bit counter
	UINT16 m_sr;						// GCR data shift register
	UINT8 m_pi;							// parallel data input
	UINT8* m_gcr;						// GCR encoder/decoder ROM
	UINT16 m_i;							// GCR encoder/decoded ROM address
	UINT8 m_e;							// GCR encoder/decoded ROM data

	// signals
	int m_ready;						// byte ready
	int m_mode;							// mode select
	int m_rw;							// read/write select
	int m_miot_irq;						// MIOT interrupt

	// timers
	emu_timer *m_bit_timer;

    const c2040_device_config &m_config;
};


// ======================> c3040_device

class c3040_device :  public c2040_device
{
    friend class c2040_device_config;

    // construction/destruction
    c3040_device(running_machine &_machine, const c2040_device_config &_config);
};


// ======================> c4040_device

class c4040_device :  public c2040_device
{
    friend class c2040_device_config;

    // construction/destruction
    c4040_device(running_machine &_machine, const c2040_device_config &_config);
};


// ======================> c8050_device

class c8050_device :  public c2040_device
{
    friend class c2040_device_config;

	// construction/destruction
    c8050_device(running_machine &_machine, const c2040_device_config &_config);
};


// ======================> c8250_device

class c8250_device :  public c8050_device
{
    friend class c2040_device_config;

    // construction/destruction
    c8250_device(running_machine &_machine, const c2040_device_config &_config);
};


// ======================> sfd1001_device

class sfd1001_device :  public c8050_device
{
    friend class c2040_device_config;

    // construction/destruction
    sfd1001_device(running_machine &_machine, const c2040_device_config &_config);
};


// device type definition
extern const device_type C2040;
extern const device_type C3040;
extern const device_type C4040;
extern const device_type C8050;
extern const device_type C8250;
extern const device_type SFD1001;



#endif
