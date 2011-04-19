/**********************************************************************

    Motorola MC6852 Synchronous Serial Data Adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   Vss   1 |*    \_/     | 24  _CTS
               Rx DATA   2 |             | 23  _DCD
                Rx CLK   3 |             | 22  D0
                Tx CLK   4 |             | 21  D1
               SM/_DTR   5 |             | 20  D2
               Tx DATA   6 |   MC6852    | 19  D3
                  _IRQ   7 |             | 18  D4
                   TUF   8 |             | 17  D5
                _RESET   9 |             | 16  D6
                   _CS   9 |             | 15  D7
                    RS   9 |             | 14  E
                   Vcc  10 |_____________| 13  R/_W

**********************************************************************/

#pragma once

#ifndef __MC6852__
#define __MC6852__

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_MC6852_ADD(_tag, _clock, _config) \
	MCFG_DEVICE_ADD((_tag), MC6852, _clock)	\
	MCFG_DEVICE_CONFIG(_config)

#define MC6852_INTERFACE(name) \
	const mc6852_interface (name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> mc6852_interface

struct mc6852_interface
{
	UINT32					m_rx_clock;
	UINT32					m_tx_clock;

	devcb_read_line			m_in_rx_data_func;
	devcb_write_line		m_out_tx_data_func;

	devcb_write_line		m_out_irq_func;

	devcb_read_line			m_in_cts_func;
	devcb_read_line			m_in_dcd_func;
	devcb_write_line		m_out_sm_dtr_func;
	devcb_write_line		m_out_tuf_func;
};



// ======================> mc6852_device_config

class mc6852_device_config :   public device_config,
                               public mc6852_interface
{
    friend class mc6852_device;

    // construction/destruction
    mc6852_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

protected:
	// device_config overrides
	virtual void device_config_complete();
};



// ======================> mc6852_device

class mc6852_device :	public device_t
{
    friend class mc6852_device_config;

    // construction/destruction
    mc6852_device(running_machine &_machine, const mc6852_device_config &_config);

public:
    DECLARE_READ8_MEMBER( read );
    DECLARE_WRITE8_MEMBER( write );

	DECLARE_WRITE_LINE_MEMBER( rx_clk_w );
	DECLARE_WRITE_LINE_MEMBER( tx_clk_w );
	DECLARE_WRITE_LINE_MEMBER( cts_w );
	DECLARE_WRITE_LINE_MEMBER( dcd_w );

	DECLARE_READ_LINE_MEMBER( sm_dtr_r );
	DECLARE_READ_LINE_MEMBER( tuf_r );

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int m_param, void *ptr);

private:
	static const device_timer_id TIMER_RX = 0;
	static const device_timer_id TIMER_TX = 1;

	inline void receive();
	inline void transmit();

	devcb_resolved_read_line		m_in_rx_data_func;
	devcb_resolved_write_line		m_out_tx_data_func;
	devcb_resolved_write_line		m_out_irq_func;
	devcb_resolved_read_line		m_in_cts_func;
	devcb_resolved_read_line		m_in_dcd_func;
	devcb_resolved_write_line		m_out_sm_dtr_func;
	devcb_resolved_write_line		m_out_tuf_func;

	// registers
	UINT8 m_status;			// status register
	UINT8 m_cr[3];			// control registers
	UINT8 m_scr;			// sync code register
	UINT8 m_rx_fifo[3];		// receiver FIFO
	UINT8 m_tx_fifo[3];		// transmitter FIFO
	UINT8 m_tdr;			// transmit data register
	UINT8 m_tsr;			// transmit shift register
	UINT8 m_rdr;			// receive data register
	UINT8 m_rsr;			// receive shift register

	int m_cts;				// clear to send
	int m_dcd;				// data carrier detect
	int m_sm_dtr;			// sync match/data terminal ready
	int m_tuf;				// transmitter underflow

	// timers
	emu_timer *m_rx_timer;
	emu_timer *m_tx_timer;

	const mc6852_device_config &m_config;
};


// device type definition
extern const device_type MC6852;



#endif
