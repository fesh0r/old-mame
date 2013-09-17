/***************************************************************************

    eepromser.h

    Serial EEPROM devices.

****************************************************************************

    Copyright Aaron Giles
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#pragma once

#ifndef __EEPROMSER_H__
#define __EEPROMSER_H__

#include "eeprom.h"


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

// standard 93CX6 class of 16-bit EEPROMs
#define MCFG_EEPROM_SERIAL_93C06_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C06_16BIT, 0)
#define MCFG_EEPROM_SERIAL_93C46_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C46_16BIT, 0)
#define MCFG_EEPROM_SERIAL_93C56_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C56_16BIT, 0)
#define MCFG_EEPROM_SERIAL_93C57_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C57_16BIT, 0)
#define MCFG_EEPROM_SERIAL_93C66_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C66_16BIT, 0)
#define MCFG_EEPROM_SERIAL_93C76_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C76_16BIT, 0)
#define MCFG_EEPROM_SERIAL_93C86_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C86_16BIT, 0)

// some manufacturers use pin 6 as an "ORG" pin which, when pulled low, configures memory for 8-bit accesses
#define MCFG_EEPROM_SERIAL_93C46_8BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C46_8BIT, 0)
#define MCFG_EEPROM_SERIAL_93C56_8BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C56_8BIT, 0)
#define MCFG_EEPROM_SERIAL_93C57_8BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C57_8BIT, 0)
#define MCFG_EEPROM_SERIAL_93C66_8BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C66_8BIT, 0)
#define MCFG_EEPROM_SERIAL_93C76_8BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C76_8BIT, 0)
#define MCFG_EEPROM_SERIAL_93C86_8BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_93C86_8BIT, 0)

// ER5911 has a separate ready pin, a reduced command set, and supports 8/16 bit out of the box
#define MCFG_EEPROM_SERIAL_ER5911_8BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_ER5911_8BIT, 0)
#define MCFG_EEPROM_SERIAL_ER5911_16BIT_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, EEPROM_SERIAL_ER5911_16BIT, 0)

// optional enable for streaming reads
#define MCFG_EEPROM_SERIAL_ENABLE_STREAMING() \
	eeprom_serial_base_device::static_enable_streaming(*device);
// pass-throughs to the base class for setting default data
#define MCFG_EEPROM_SERIAL_DATA MCFG_EEPROM_DATA
#define MCFG_EEPROM_SERIAL_DEFAULT_VALUE MCFG_EEPROM_DEFAULT_VALUE



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************


// ======================> eeprom_serial_base_device

class eeprom_serial_base_device : public eeprom_base_device
{
protected:
	// construction/destruction
	eeprom_serial_base_device(const machine_config &mconfig, device_type devtype, const char *name, const char *tag, device_t *owner, const char *shortname, const char *file);

public:
	// inline configuration helpers
	static void static_set_address_bits(device_t &device, int addrbits);
	static void static_enable_streaming(device_t &device);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// read interfaces differ between implementations

	// commands
	enum eeprom_command
	{
		COMMAND_INVALID,
		COMMAND_READ,
		COMMAND_WRITE,
		COMMAND_ERASE,
		COMMAND_LOCK,
		COMMAND_UNLOCK,
		COMMAND_WRITEALL,
		COMMAND_ERASEALL
	};

	// states
	enum eeprom_state
	{
		STATE_IN_RESET,
		STATE_WAIT_FOR_START_BIT,
		STATE_WAIT_FOR_COMMAND,
		STATE_READING_DATA,
		STATE_WAIT_FOR_DATA,
		STATE_WAIT_FOR_COMPLETION
	};

	// events
	enum eeprom_event
	{
		EVENT_CS_RISING_EDGE = 1 << 0,
		EVENT_CS_FALLING_EDGE = 1 << 1,
		EVENT_CLK_RISING_EDGE = 1 << 2,
		EVENT_CLK_FALLING_EDGE = 1 << 3
	};

	// internal helpers
	void set_state(eeprom_state newstate);
	void handle_event(eeprom_event event);
	void execute_command();
	void execute_write_command();

	// subclass helpers
	void base_cs_write(int state);
	void base_clk_write(int state);
	void base_di_write(int state);
	int base_do_read();
	int base_ready_read();

	// subclass overrides
	virtual void parse_command_and_address() = 0;

	// configuration state
	UINT8           m_command_address_bits;     // number of address bits in a command
	bool            m_streaming_enabled;        // true if streaming is enabled

	// runtime state
	eeprom_state    m_state;                    // current internal state
	UINT8           m_cs_state;                 // state of the CS line
	attotime        m_last_cs_rising_edge_time; // time of the last CS rising edge
	UINT8           m_oe_state;                 // state of the OE line
	UINT8           m_clk_state;                // state of the CLK line
	UINT8           m_di_state;                 // state of the DI line
	bool            m_locked;                   // are we locked against writes?
	UINT32          m_bits_accum;               // number of bits accumulated
	UINT32          m_command_address_accum;    // accumulator of command+address bits
	eeprom_command  m_command;                  // current command
	UINT32          m_address;                  // current address extracted from command
	UINT32          m_shift_register;           // holds data coming in/going out
};



// ======================> eeprom_serial_93cxx_device

class eeprom_serial_93cxx_device : public eeprom_serial_base_device
{
protected:
	// construction/destruction
	eeprom_serial_93cxx_device(const machine_config &mconfig, device_type devtype, const char *name, const char *tag, device_t *owner, const char *shortname, const char *file);

public:
	// read handlers
	DECLARE_READ_LINE_MEMBER(do_read);  // combined DO+READY/BUSY

	// write handlers
	DECLARE_WRITE_LINE_MEMBER(cs_write);        // CS signal (active high)
	DECLARE_WRITE_LINE_MEMBER(clk_write);       // CLK signal (active high)
	DECLARE_WRITE_LINE_MEMBER(di_write);        // DI

protected:
	// subclass overrides
	virtual void parse_command_and_address();
};


// ======================> eeprom_serial_er5911_device

class eeprom_serial_er5911_device : public eeprom_serial_base_device
{
protected:
	// construction/destruction
	eeprom_serial_er5911_device(const machine_config &mconfig, device_type devtype, const char *name, const char *tag, device_t *owner, const char *shortname, const char *file);

public:
	// read handlers
	DECLARE_READ_LINE_MEMBER(do_read);          // DO
	DECLARE_READ_LINE_MEMBER(ready_read);       // READY/BUSY only

	// write handlers
	DECLARE_WRITE_LINE_MEMBER(cs_write);        // CS signal (active high)
	DECLARE_WRITE_LINE_MEMBER(clk_write);       // CLK signal (active high)
	DECLARE_WRITE_LINE_MEMBER(di_write);        // DI

protected:
	// subclass overrides
	virtual void parse_command_and_address();
};



//**************************************************************************
//  DERIVED TYPES
//**************************************************************************

// macro for declaring a new device class
#define DECLARE_SERIAL_EEPROM_DEVICE(_baseclass, _lowercase, _uppercase, _bits) \
class eeprom_serial_##_lowercase##_##_bits##bit_device : public eeprom_serial_##_baseclass##_device \
{ \
public: \
	eeprom_serial_##_lowercase##_##_bits##bit_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock); \
}; \
extern const device_type EEPROM_SERIAL_##_uppercase##_##_bits##BIT;
// standard 93CX6 class of 16-bit EEPROMs
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c06, 93C06, 16)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c46, 93C46, 16)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c56, 93C56, 16)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c57, 93C57, 16)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c66, 93C66, 16)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c76, 93C76, 16)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c86, 93C86, 16)

// some manufacturers use pin 6 as an "ORG" pin which, when pulled low, configures memory for 8-bit accesses
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c46, 93C46, 8)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c56, 93C56, 8)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c57, 93C57, 8)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c66, 93C66, 8)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c76, 93C76, 8)
DECLARE_SERIAL_EEPROM_DEVICE(93cxx, 93c86, 93C86, 8)

// ER5911 has a separate ready pin, a reduced command set, and supports 8/16 bit out of the box
DECLARE_SERIAL_EEPROM_DEVICE(er5911, er5911, ER5911, 8)
DECLARE_SERIAL_EEPROM_DEVICE(er5911, er5911, ER5911, 16)

#endif
