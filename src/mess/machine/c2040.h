/**********************************************************************

    Commodore 2040/3040/4040/8050/8250/SFD-1001 Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C2040__
#define __C2040__


#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6502/m6504.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/cbmipt.h"
#include "machine/mos6530.h"
#include "machine/ieee488.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c2040_device

class c2040_device :  public device_t,
						public device_ieee488_interface
{
public:
	// construction/destruction
	c2040_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);
	c2040_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

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

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	// device_ieee488_interface overrides
	virtual void ieee488_atn(int state);
	virtual void ieee488_ifc(int state);

	virtual void byte_ready(int state) { }
	inline void update_ieee_signals();
	inline void update_gcr_data();
	inline void read_current_track(int unit);
	inline void spindle_motor(int unit, int mtr);
	inline void micropolis_step_motor(int unit, int stp);
	inline void mpi_step_motor(int unit, int stp);

	required_device<m6502_device> m_maincpu;
	required_device<m6504_device> m_fdccpu;
	required_device<riot6532_device> m_riot0;
	required_device<riot6532_device> m_riot1;
	required_device<mos6530_device> m_miot;
	required_device<via6522_device> m_via;
	required_device<legacy_floppy_image_device> m_image0;
	optional_device<legacy_floppy_image_device> m_image1;
	required_memory_region m_gcr;
	required_ioport m_address;

	struct {
		// motors
		int m_stp;                              // stepper motor phase
		int m_mtr;                              // spindle motor on

		// track
		UINT8 m_track_buffer[G64_BUFFER_SIZE];  // track data buffer
		int m_track_len;                        // track length
		int m_buffer_pos;                       // byte position within track buffer
		int m_bit_pos;                          // bit position within track buffer byte

		// devices
		device_t *m_image;
	} m_unit[2];

	int m_drive;                        // selected drive
	int m_side;                         // selected side

	// IEEE-488 bus
	int m_rfdo;                         // not ready for data output
	int m_daco;                         // not data accepted output
	int m_atna;                         // attention acknowledge
	int m_ifc;

	// track
	int m_ds;                           // density select
	int m_bit_count;                    // GCR bit counter
	UINT16 m_sr;                        // GCR data shift register
	UINT8 m_pi;                         // parallel data input
	UINT16 m_i;                         // GCR encoder/decoded ROM address
	UINT8 m_e;                          // GCR encoder/decoded ROM data

	// signals
	int m_ready;                        // byte ready
	int m_mode;                         // mode select
	int m_rw;                           // read/write select
	int m_miot_irq;                     // MIOT interrupt

	// timers
	emu_timer *m_bit_timer;
};


// ======================> c3040_device

class c3040_device :  public c2040_device
{
public:
	// construction/destruction
	c3040_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};


// ======================> c4040_device

class c4040_device :  public c2040_device
{
public:
	// construction/destruction
	c4040_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
};


// ======================> c8050_device

class c8050_device :  public c2040_device
{
public:
	// construction/destruction
	c8050_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	c8050_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, bool double_sided, const char *shortname, const char *source);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

	DECLARE_READ8_MEMBER( via_pb_r );
	DECLARE_WRITE8_MEMBER( via_pb_w );
	DECLARE_READ8_MEMBER( miot_pb_r );
	DECLARE_WRITE8_MEMBER( miot_pb_w );

protected:
	virtual void byte_ready(int state);

	bool m_double_sided;
};


// ======================> c8250_device

class c8250_device :  public c8050_device
{
public:
	// construction/destruction
	c8250_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
};


// ======================> c8250lp_device

class c8250lp_device :  public c8050_device
{
public:
	// construction/destruction
	c8250lp_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
};


// ======================> sfd1001_device

class sfd1001_device :  public c8050_device
{
public:
	// construction/destruction
	sfd1001_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
};


// device type definition
extern const device_type C2040;
extern const device_type C3040;
extern const device_type C4040;
extern const device_type C8050;
extern const device_type C8250;
extern const device_type C8250LP;
extern const device_type SFD1001;



#endif
