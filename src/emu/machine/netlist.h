/***************************************************************************

    netlist.h

    Discrete netlist implementation.

****************************************************************************

    Couriersud reserves the right to license the code under a less restrictive
    license going forward.

    Copyright Nicola Salmoria and the MAME team
    All rights reserved.

    Redistribution and use of this code or any derivative works are permitted
    provided that the following conditions are met:

    * Redistributions may not be sold, nor may they be used in a commercial
    product or activity.

    * Redistributions that are modified from the original source must include the
    complete source code, including the source code for all components used by a
    binary built from the modified sources. However, as a special exception, the
    source code distributed need not include anything that is normally distributed
    (in either source or binary form) with the major components (compiler, kernel,
    and so on) of the operating system on which the executable runs, unless that
    component itself accompanies the executable.

    * Redistributions must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.


****************************************************************************/

#ifndef NETLIST_H
#define NETLIST_H

#include "emu.h"
#include "tagmap.h"

#include "netlist/nl_base.h"
#include "netlist/nl_setup.h"

// MAME specific

#define MCFG_NETLIST_ADD(_tag, _setup )                                             \
	MCFG_DEVICE_ADD(_tag, NETLIST, NETLIST_CLOCK)                                   \
	MCFG_NETLIST_SETUP(_setup)
#define MCFG_NETLIST_REPLACE(_tag, _setup)                                          \
	MCFG_DEVICE_REPLACE(_tag, NETLIST, NETLIST_CLOCK)                               \
	MCFG_NETLIST_SETUP(_setup)
#define MCFG_NETLIST_SETUP(_setup)                                                  \
	netlist_mame_device::static_set_constructor(*device, NETLIST_NAME(_setup));





// ----------------------------------------------------------------------------------------
// MAME glue classes
// ----------------------------------------------------------------------------------------


class netlist_t : public netlist_base_t
{
public:

	netlist_t(device_t &parent)
	: netlist_base_t(),
		m_parent(parent)
	{}
	virtual ~netlist_t() { };

	inline running_machine &machine()   { return m_parent.machine(); }

	device_t &parent() { return m_parent; }

private:
	device_t &m_parent;
};

// ======================> netlist_mame_device

class netlist_mame_device : public device_t,
						public device_execute_interface
{
public:

	template<bool _Required, class _NETClass>
	class output_finder;
	class optional_output;
	template<class C>
	class required_output;
	class optional_param;
	class required_param;
	class on_device_start;

	// construction/destruction
	netlist_mame_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~netlist_mame_device() {}

	static void static_set_constructor(device_t &device, void (*setup_func)(netlist_setup_t &));

	netlist_setup_t &setup() { return *m_setup; }
	netlist_t &netlist() { return *m_netlist; }

	netlist_list_t<on_device_start *> m_device_start_list;

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_stop();
	virtual void device_reset();
	virtual void device_post_load();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	virtual UINT64 execute_clocks_to_cycles(UINT64 clocks) const;
	virtual UINT64 execute_cycles_to_clocks(UINT64 cycles) const;

	ATTR_HOT virtual void execute_run();

	netlist_t *m_netlist;

	netlist_setup_t *m_setup;

private:

	void save_state();

	void (*m_setup_func)(netlist_setup_t &);

	int m_icount;


};

// ======================> netlist_output_finder

class netlist_mame_device::on_device_start
{
public:
	virtual bool OnDeviceStart() = 0;
	virtual ~on_device_start() {}
};

// device finder template
template<bool _Required, class _NETClass>
class netlist_mame_device::output_finder : public object_finder_base<_NETClass>,
		netlist_mame_device::on_device_start
{
public:
	// construction/destruction
	output_finder(device_t &base, const char *tag, const char *output)
		: object_finder_base<_NETClass>(base, tag), m_netlist(NULL), m_output(output) { }

	// finder
	virtual bool findit(bool isvalidation = false)
	{
		if (isvalidation) return true;
		device_t *device = this->m_base.subdevice(this->m_tag);
		m_netlist = dynamic_cast<netlist_mame_device *>(device);
		if (device != NULL && m_netlist == NULL)
		{
			void mame_printf_warning(const char *format, ...) ATTR_PRINTF(1,2);
			mame_printf_warning("Device '%s' found but is not netlist\n", this->m_tag);
		}
		m_netlist->m_device_start_list.add(this);
		return this->report_missing(m_netlist != NULL, "device", _Required);
	}

protected:
	netlist_mame_device *m_netlist;
	const char *m_output;
};

// optional device finder
class netlist_mame_device::optional_output : public netlist_mame_device::output_finder<false, net_output_t>
{
public:
	optional_output(device_t &base, const char *tag, const char *output) : output_finder<false, net_output_t>(base, tag, output) { }

	virtual ~optional_output() {};

	virtual bool OnDeviceStart()
	{
		this->m_target = &m_netlist->setup().find_output(m_output);
		return this->report_missing(this->m_target != NULL, "output", false);
	}

};

// required devices are similar but throw an error if they are not found
template<class C>
class netlist_mame_device::required_output : public netlist_mame_device::output_finder<true, C>
{
public:
	required_output(device_t &base, const char *tag, const char *output) : output_finder<true, C>(base, tag, output) { }

	virtual ~required_output() {};

	virtual bool OnDeviceStart()
	{
		this->m_target = (C *) &(this->m_netlist->setup().find_output(this->m_output));
		return this->report_missing(this->m_target != NULL, "output", true);
	}

};

// optional device finder
class netlist_mame_device::optional_param : public netlist_mame_device::output_finder<false, net_param_t>
{
public:
	optional_param(device_t &base, const char *tag, const char *output) : output_finder<false, net_param_t>(base, tag, output) { }

	virtual bool OnDeviceStart()
	{
		this->m_target = &m_netlist->setup().find_param(m_output);
		return this->report_missing(this->m_target != NULL, "parameter", false);
	}

};

// required devices are similar but throw an error if they are not found
class netlist_mame_device::required_param : public netlist_mame_device::output_finder<true, net_param_t>
{
public:
	required_param(device_t &base, const char *tag, const char *output) : output_finder<true, net_param_t>(base, tag, output) { }

	virtual bool OnDeviceStart()
	{
		this->m_target = &m_netlist->setup().find_param(m_output);
		return this->report_missing(this->m_target != NULL, "output", true);
	}
};


// device type definition
extern const device_type NETLIST;

#endif
