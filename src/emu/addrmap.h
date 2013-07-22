/***************************************************************************

    addrmap.h

    Macros and helper functions for handling address map definitions.

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

#ifndef __EMU_H__
#error Dont include this file directly; include emu.h instead.
#endif

#ifndef __ADDRMAP_H__
#define __ADDRMAP_H__



//**************************************************************************
//  CONSTANTS
//**************************************************************************

// address map handler types
enum map_handler_type
{
	AMH_NONE = 0,
	AMH_RAM,
	AMH_ROM,
	AMH_NOP,
	AMH_UNMAP,
	AMH_DEVICE_DELEGATE,
	AMH_LEGACY_SPACE_HANDLER,
	AMH_PORT,
	AMH_BANK,
	AMH_DEVICE_SUBMAP
};



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// address map handler data
class map_handler_data
{
public:
	map_handler_data()
		: m_type(AMH_NONE),
			m_bits(0),
			m_mask(0),
			m_name(NULL),
			m_devbase(NULL),
			m_tag(NULL) { }

	map_handler_type        m_type;             // type of the handler
	UINT8                   m_bits;             // width of the handler in bits, or 0 for default
	UINT64                  m_mask;             // mask for which lanes apply
	const char *            m_name;             // name of the handler
	device_t *              m_devbase;          // pointer to "base" device
	const char *            m_tag;              // tag for I/O ports and banks
};



// ======================> address_map_entry

// address_map_entry is a linked list element describing one address range in a map
class address_map_entry
{
public:
	// construction/destruction
	address_map_entry(address_map &map, offs_t start, offs_t end);

	// getters
	address_map_entry *next() const { return m_next; }

	// simple inline setters
	void set_mirror(offs_t _mirror) { m_addrmirror = _mirror; }
	void set_read_type(map_handler_type _type) { m_read.m_type = _type; }
	void set_write_type(map_handler_type _type) { m_write.m_type = _type; }
	void set_region(const char *tag, offs_t offset) { m_region = tag; m_rgnoffs = offset; }
	void set_share(const char *tag) { assert(m_share == NULL); m_share = tag; }

	// mask setting
	void set_mask(offs_t _mask);

	// I/O port configuration
	void set_read_port(device_t &device, const char *tag);
	void set_write_port(device_t &device, const char *tag);
	void set_readwrite_port(device_t &device, const char *tag);

	// memory bank configuration
	void set_read_bank(device_t &device, const char *tag);
	void set_write_bank(device_t &device, const char *tag);
	void set_readwrite_bank(device_t &device, const char *tag);

	// submap referencing
	void set_submap(device_t &device, const char *tag, address_map_delegate func, int bits, UINT64 mask);

	// public state
	address_map_entry *     m_next;                 // pointer to the next entry
	address_map &           m_map;                  // reference to our owning map
	astring                 m_region_string;        // string used to hold derived names

	// basic information
	offs_t                  m_addrstart;            // start address
	offs_t                  m_addrend;              // end address
	offs_t                  m_addrmirror;           // mirror bits
	offs_t                  m_addrmask;             // mask bits
	map_handler_data        m_read;                 // data for read handler
	map_handler_data        m_write;                // data for write handler
	const char *            m_share;                // tag of a shared memory block
	const char *            m_region;               // tag of region containing the memory backing this entry
	offs_t                  m_rgnoffs;              // offset within the region

	// handlers
	read8_delegate          m_rproto8;              // 8-bit read proto-delegate
	read16_delegate         m_rproto16;             // 16-bit read proto-delegate
	read32_delegate         m_rproto32;             // 32-bit read proto-delegate
	read64_delegate         m_rproto64;             // 64-bit read proto-delegate
	read8_space_func        m_rspace8;              // 8-bit legacy address space handler
	read16_space_func       m_rspace16;             // 16-bit legacy address space handler
	read32_space_func       m_rspace32;             // 32-bit legacy address space handler
	read64_space_func       m_rspace64;             // 64-bit legacy address space handler
	write8_delegate         m_wproto8;              // 8-bit write proto-delegate
	write16_delegate        m_wproto16;             // 16-bit write proto-delegate
	write32_delegate        m_wproto32;             // 32-bit write proto-delegate
	write64_delegate        m_wproto64;             // 64-bit write proto-delegate
	write8_space_func       m_wspace8;              // 8-bit legacy address space handler
	write16_space_func      m_wspace16;             // 16-bit legacy address space handler
	write32_space_func      m_wspace32;             // 32-bit legacy address space handler
	write64_space_func      m_wspace64;             // 64-bit legacy address space handler

	address_map_delegate    m_submap_delegate;
	int                     m_submap_bits;

	// information used during processing
	void *                  m_memory;               // pointer to memory backing this entry
	offs_t                  m_bytestart;            // byte-adjusted start address
	offs_t                  m_byteend;              // byte-adjusted end address
	offs_t                  m_bytemirror;           // byte-adjusted mirror bits
	offs_t                  m_bytemask;             // byte-adjusted mask bits

protected:
	// internal handler setters for 8-bit functions
	void internal_set_handler(read8_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(write8_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(read8_space_func rfunc, const char *rstring, write8_space_func wfunc,  const char *wstring, UINT64 mask);
	void internal_set_handler(device_t &device, read8_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, write8_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, read8_delegate rfunc, write8_delegate wfunc, UINT64 mask);

	// internal handler setters for 16-bit functions
	void internal_set_handler(read16_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(write16_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(read16_space_func rfunc, const char *rstring, write16_space_func wfunc, const char *wstring, UINT64 mask);
	void internal_set_handler(device_t &device, read16_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, write16_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, read16_delegate rfunc, write16_delegate wfunc, UINT64 mask);

	// internal handler setters for 32-bit functions
	void internal_set_handler(read32_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(write32_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(read32_space_func rfunc, const char *rstring, write32_space_func wfunc, const char *wstring, UINT64 mask);
	void internal_set_handler(device_t &device, read32_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, write32_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, read32_delegate rfunc, write32_delegate wfunc, UINT64 mask);

	// internal handler setters for 64-bit functions
	void internal_set_handler(read64_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(write64_space_func func, const char *string, UINT64 mask);
	void internal_set_handler(read64_space_func rfunc, const char *rstring, write64_space_func wfunc, const char *wstring, UINT64 mask);
	void internal_set_handler(device_t &device, read64_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, write64_delegate func, UINT64 mask);
	void internal_set_handler(device_t &device, read64_delegate rfunc, write64_delegate wfunc, UINT64 mask);

private:
	// helper functions
	bool unitmask_is_appropriate(UINT8 width, UINT64 unitmask, const char *string);
};


// ======================> address_map_entry8

// 8-bit address map version of address_map_entry which only permits valid 8-bit handlers
class address_map_entry8 : public address_map_entry
{
public:
	address_map_entry8(address_map &map, offs_t start, offs_t end);

	// native-size handlers
	void set_handler(read8_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(write8_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(read8_space_func rfunc, const char *rstring, write8_space_func wfunc, const char *wstring) { internal_set_handler(rfunc, rstring, wfunc, wstring, 0); }
	void set_handler(device_t &device, read8_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, write8_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, read8_delegate rfunc, write8_delegate wfunc) { internal_set_handler(device, rfunc, wfunc, 0); }
};


// ======================> address_map_entry16

// 16-bit address map version of address_map_entry which only permits valid 16-bit handlers
class address_map_entry16 : public address_map_entry
{
public:
	address_map_entry16(address_map &map, offs_t start, offs_t end);

	// native-size handlers
	void set_handler(read16_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(write16_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(read16_space_func rfunc, const char *rstring, write16_space_func wfunc, const char *wstring) { internal_set_handler(rfunc, rstring, wfunc, wstring, 0); }
	void set_handler(device_t &device, read16_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, write16_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, read16_delegate rfunc, write16_delegate wfunc) { internal_set_handler(device, rfunc, wfunc, 0); }

	// 8-bit handlers
	void set_handler(read8_space_func func, const char *string, UINT16 mask) { internal_set_handler(func, string, mask); }
	void set_handler(write8_space_func func, const char *string, UINT16 mask) { internal_set_handler(func, string, mask); }
	void set_handler(read8_space_func rfunc, const char *rstring, write8_space_func wfunc, const char *wstring, UINT16 mask) { internal_set_handler(rfunc, rstring, wfunc, wstring, mask); }
	void set_handler(device_t &device, read8_delegate func, UINT16 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, write8_delegate func, UINT16 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, read8_delegate rfunc, write8_delegate wfunc, UINT16 mask) { internal_set_handler(device, rfunc, wfunc, mask); }
};


// ======================> address_map_entry32

// 32-bit address map version of address_map_entry which only permits valid 32-bit handlers
class address_map_entry32 : public address_map_entry
{
public:
	address_map_entry32(address_map &map, offs_t start, offs_t end);

	// native-size handlers
	void set_handler(read32_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(write32_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(read32_space_func rfunc, const char *rstring, write32_space_func wfunc, const char *wstring) { internal_set_handler(rfunc, rstring, wfunc, wstring, 0); }
	void set_handler(device_t &device, read32_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, write32_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, read32_delegate rfunc, write32_delegate wfunc) { internal_set_handler(device, rfunc, wfunc, 0); }

	// 16-bit handlers
	void set_handler(read16_space_func func, const char *string, UINT32 mask) { internal_set_handler(func, string, mask); }
	void set_handler(write16_space_func func, const char *string, UINT32 mask) { internal_set_handler(func, string, mask); }
	void set_handler(read16_space_func rfunc, const char *rstring, write16_space_func wfunc, const char *wstring, UINT32 mask) { internal_set_handler(rfunc, rstring, wfunc, wstring, mask); }
	void set_handler(device_t &device, read16_delegate func, UINT32 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, write16_delegate func, UINT32 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, read16_delegate rfunc, write16_delegate wfunc, UINT32 mask) { internal_set_handler(device, rfunc, wfunc, mask); }

	// 8-bit handlers
	void set_handler(read8_space_func func, const char *string, UINT32 mask) { internal_set_handler(func, string, mask); }
	void set_handler(write8_space_func func, const char *string, UINT32 mask) { internal_set_handler(func, string, mask); }
	void set_handler(read8_space_func rfunc, const char *rstring, write8_space_func wfunc, const char *wstring, UINT32 mask) { internal_set_handler(rfunc, rstring, wfunc, wstring, mask); }
	void set_handler(device_t &device, read8_delegate func, UINT32 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, write8_delegate func, UINT32 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, read8_delegate rfunc, write8_delegate wfunc, UINT32 mask) { internal_set_handler(device, rfunc, wfunc, mask); }
};


// ======================> address_map_entry64

// 64-bit address map version of address_map_entry which only permits valid 64-bit handlers
class address_map_entry64 : public address_map_entry
{
public:
	address_map_entry64(address_map &map, offs_t start, offs_t end);

	// native-size handlers
	void set_handler(read64_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(write64_space_func func, const char *string) { internal_set_handler(func, string, 0); }
	void set_handler(read64_space_func rfunc, const char *rstring, write64_space_func wfunc, const char *wstring) { internal_set_handler(rfunc, rstring, wfunc, wstring, 0); }
	void set_handler(device_t &device, read64_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, write64_delegate func) { internal_set_handler(device, func, 0); }
	void set_handler(device_t &device, read64_delegate rfunc, write64_delegate wfunc) { internal_set_handler(device, rfunc, wfunc, 0); }

	// 32-bit handlers
	void set_handler(read32_space_func func, const char *string, UINT64 mask) { internal_set_handler(func, string, mask); }
	void set_handler(write32_space_func func, const char *string, UINT64 mask) { internal_set_handler(func, string, mask); }
	void set_handler(read32_space_func rfunc, const char *rstring, write32_space_func wfunc, const char *wstring, UINT64 mask) { internal_set_handler(rfunc, rstring, wfunc, wstring, mask); }
	void set_handler(device_t &device, read32_delegate func, UINT64 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, write32_delegate func, UINT64 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, read32_delegate rfunc, write32_delegate wfunc, UINT64 mask) { internal_set_handler(device, rfunc, wfunc, mask); }

	// 16-bit handlers
	void set_handler(read16_space_func func, const char *string, UINT64 mask) { internal_set_handler(func, string, mask); }
	void set_handler(write16_space_func func, const char *string, UINT64 mask) { internal_set_handler(func, string, mask); }
	void set_handler(read16_space_func rfunc, const char *rstring, write16_space_func wfunc, const char *wstring, UINT64 mask) { internal_set_handler(rfunc, rstring, wfunc, wstring, mask); }
	void set_handler(device_t &device, read16_delegate func, UINT64 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, write16_delegate func, UINT64 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, read16_delegate rfunc, write16_delegate wfunc, UINT64 mask) { internal_set_handler(device, rfunc, wfunc, mask); }

	// 8-bit handlers
	void set_handler(read8_space_func func, const char *string, UINT64 mask) { internal_set_handler(func, string, mask); }
	void set_handler(write8_space_func func, const char *string, UINT64 mask) { internal_set_handler(func, string, mask); }
	void set_handler(read8_space_func rfunc, const char *rstring, write8_space_func wfunc, const char *wstring, UINT64 mask) { internal_set_handler(rfunc, rstring, wfunc, wstring, mask); }
	void set_handler(device_t &device, read8_delegate func, UINT64 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, write8_delegate func, UINT64 mask) { internal_set_handler(device, func, mask); }
	void set_handler(device_t &device, read8_delegate rfunc, write8_delegate wfunc, UINT64 mask) { internal_set_handler(device, rfunc, wfunc, mask); }
};


// ======================> address_map

// address_map holds global map parameters plus the head of the list of entries
class address_map
{
public:
	// construction/destruction
	address_map(device_t &device, address_spacenum spacenum);
	address_map(device_t &device, address_map_entry *entry);
	address_map(const address_space &space, offs_t start, offs_t end, int bits, UINT64 unitmask, device_t &device, address_map_delegate submap_delegate);
	~address_map();

	// configuration
	void configure(address_spacenum _spacenum, UINT8 _databits);

	// setters
	void set_global_mask(offs_t mask);
	void set_unmap_value(UINT8 value) { m_unmapval = value; }

	// add a new entry of the given type
	address_map_entry8 *add(offs_t start, offs_t end, address_map_entry8 *ptr);
	address_map_entry16 *add(offs_t start, offs_t end, address_map_entry16 *ptr);
	address_map_entry32 *add(offs_t start, offs_t end, address_map_entry32 *ptr);
	address_map_entry64 *add(offs_t start, offs_t end, address_map_entry64 *ptr);

	// public data
	address_spacenum        m_spacenum;         // space number of the map
	UINT8                   m_databits;         // data bits represented by the map
	UINT8                   m_unmapval;         // unmapped memory value
	offs_t                  m_globalmask;       // global mask
	simple_list<address_map_entry> m_entrylist; // list of entries

	void uplift_submaps(running_machine &machine, device_t &device, device_t &owner, endianness_t endian);
};


//**************************************************************************
//  ADDRESS MAP MACROS
//**************************************************************************

// so that "0" can be used for unneeded address maps
#define construct_address_map_0 NULL

// start/end tags for the address map
#define ADDRESS_MAP_NAME(_name) construct_address_map_##_name

#define ADDRESS_MAP_START(_name, _space, _bits, _class) \
void ADDRESS_MAP_NAME(_name)(address_map &map, device_t &device) \
{ \
	typedef read##_bits##_delegate read_delegate; \
	typedef write##_bits##_delegate write_delegate; \
	address_map_entry##_bits *curentry = NULL; \
	(void)curentry; \
	map.configure(_space, _bits); \
	typedef _class drivdata_class;
#define DEVICE_ADDRESS_MAP_START(_name, _bits, _class) \
void _class :: _name(::address_map &map, device_t &device) \
{ \
	typedef read##_bits##_delegate read_delegate; \
	typedef write##_bits##_delegate write_delegate; \
	address_map_entry##_bits *curentry = NULL; \
	(void)curentry; \
	map.configure(AS_PROGRAM, _bits);  \
	typedef _class drivdata_class;
#define ADDRESS_MAP_END \
}

// use this to declare external references to an address map
#define ADDRESS_MAP_EXTERN(_name, _bits) \
	extern void ADDRESS_MAP_NAME(_name)(address_map &map, device_t &device)

// use this to declare an address map as a member of a modern device class
// need to qualify with :: to avoid a collision with descendants of device_memory_interface
#define DECLARE_ADDRESS_MAP(_name, _bits) \
	void _name(::address_map &map, device_t &device)


// global controls
#define ADDRESS_MAP_GLOBAL_MASK(_mask) \
	map.set_global_mask(_mask);
#define ADDRESS_MAP_UNMAP_LOW \
	map.set_unmap_value(0);
#define ADDRESS_MAP_UNMAP_HIGH \
	map.set_unmap_value(~0);

// importing data from other address maps
#define AM_IMPORT_FROM(_name) \
	ADDRESS_MAP_NAME(_name)(map, device);
// importing data from inherited address maps
#define AM_INHERIT_FROM(_name) \
	_name(map, device);

// address ranges
#define AM_RANGE(_start, _end) \
	curentry = map.add(_start, _end, curentry);
#define AM_MASK(_mask) \
	curentry->set_mask(_mask);
#define AM_MIRROR(_mirror) \
	curentry->set_mirror(_mirror);

// legacy space reads
#define AM_READ_LEGACY(_handler) \
	curentry->set_handler(_handler, #_handler);

// legacy space writes
#define AM_WRITE_LEGACY(_handler) \
	curentry->set_handler(_handler, #_handler);


// legacy space reads/writes
#define AM_READWRITE_LEGACY(_rhandler, _whandler) \
	curentry->set_handler(_rhandler, #_rhandler, _whandler, #_whandler);
#define AM_READWRITE8_LEGACY(_rhandler, _whandler, _unitmask) \
	curentry->set_handler(_rhandler, #_rhandler, _whandler, #_whandler, _unitmask);
#define AM_READWRITE32_LEGACY(_rhandler, _whandler, _unitmask) \
	curentry->set_handler(_rhandler, #_rhandler, _whandler, #_whandler, _unitmask);

// legacy device reads
#define AM_DEVREAD_LEGACY(_tag, _handler) \
	curentry->set_handler(device, read_delegate(&_handler, #_handler, _tag, (device_t *)0));
#define AM_DEVREAD8_LEGACY(_tag, _handler, _unitmask) \
	curentry->set_handler(device, read8_delegate(&_handler, #_handler, _tag, (device_t *)0), _unitmask);



// legacy device writes
#define AM_DEVWRITE_LEGACY(_tag, _handler) \
	curentry->set_handler(device, write_delegate(&_handler, #_handler, _tag, (device_t *)0));
#define AM_DEVWRITE8_LEGACY(_tag, _handler, _unitmask) \
	curentry->set_handler(device, write8_delegate(&_handler, #_handler, _tag, (device_t *)0), _unitmask);


// legacy device reads/writes
#define AM_DEVREADWRITE_LEGACY(_tag, _rhandler, _whandler) \
	curentry->set_handler(device, read_delegate(&_rhandler, #_rhandler, _tag, (device_t *)0), write_delegate(&_whandler, #_whandler, _tag, (device_t *)0));
#define AM_DEVREADWRITE8_LEGACY(_tag, _rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read8_delegate(&_rhandler, #_rhandler, _tag, (device_t *)0), write8_delegate(&_whandler, #_whandler, _tag, (device_t *)0), _unitmask);
#define AM_DEVREADWRITE16_LEGACY(_tag, _rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read16_delegate(&_rhandler, #_rhandler, _tag, (device_t *)0), write16_delegate(&_whandler, #_whandler, _tag, (device_t *)0), _unitmask);

// driver data reads
#define AM_READ(_handler) \
	curentry->set_handler(device, read_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0));
#define AM_READ8(_handler, _unitmask) \
	curentry->set_handler(device, read8_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0), _unitmask);
#define AM_READ16(_handler, _unitmask) \
	curentry->set_handler(device, read16_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0), _unitmask);
#define AM_READ32(_handler, _unitmask) \
	curentry->set_handler(device, read32_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0), _unitmask);

// driver data writes
#define AM_WRITE(_handler) \
	curentry->set_handler(device, write_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0));
#define AM_WRITE8(_handler, _unitmask) \
	curentry->set_handler(device, write8_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0), _unitmask);
#define AM_WRITE16(_handler, _unitmask) \
	curentry->set_handler(device, write16_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0), _unitmask);
#define AM_WRITE32(_handler, _unitmask) \
	curentry->set_handler(device, write32_delegate(&drivdata_class::_handler, "driver_data::" #_handler, DEVICE_SELF, (drivdata_class *)0), _unitmask);

// driver data reads/writes
#define AM_READWRITE(_rhandler, _whandler) \
	curentry->set_handler(device, read_delegate(&drivdata_class::_rhandler, "driver_data::" #_rhandler, DEVICE_SELF, (drivdata_class *)0), write_delegate(&drivdata_class::_whandler, "driver_data::" #_whandler, DEVICE_SELF, (drivdata_class *)0));
#define AM_READWRITE8(_rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read8_delegate(&drivdata_class::_rhandler, "driver_data::" #_rhandler, DEVICE_SELF, (drivdata_class *)0), write8_delegate(&drivdata_class::_whandler, "driver_data::" #_whandler, DEVICE_SELF, (drivdata_class *)0), _unitmask);
#define AM_READWRITE16(_rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read16_delegate(&drivdata_class::_rhandler, "driver_data::" #_rhandler, DEVICE_SELF, (drivdata_class *)0), write16_delegate(&drivdata_class::_whandler, "driver_data::" #_whandler, DEVICE_SELF, (drivdata_class *)0), _unitmask);
#define AM_READWRITE32(_rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read32_delegate(&drivdata_class::_rhandler, "driver_data::" #_rhandler, DEVICE_SELF, (drivdata_class *)0), write32_delegate(&drivdata_class::_whandler, "driver_data::" #_whandler, DEVICE_SELF, (drivdata_class *)0), _unitmask);

// device reads
#define AM_DEVREAD(_tag, _class, _handler) \
	curentry->set_handler(device, read_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0));
#define AM_DEVREAD8(_tag, _class, _handler, _unitmask) \
	curentry->set_handler(device, read8_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0), _unitmask);
#define AM_DEVREAD16(_tag, _class, _handler, _unitmask) \
	curentry->set_handler(device, read16_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0), _unitmask);
#define AM_DEVREAD32(_tag, _class, _handler, _unitmask) \
	curentry->set_handler(device, read32_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0), _unitmask);

// device writes
#define AM_DEVWRITE(_tag, _class, _handler) \
	curentry->set_handler(device, write_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0));
#define AM_DEVWRITE8(_tag, _class, _handler, _unitmask) \
	curentry->set_handler(device, write8_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0), _unitmask);
#define AM_DEVWRITE16(_tag, _class, _handler, _unitmask) \
	curentry->set_handler(device, write16_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0), _unitmask);
#define AM_DEVWRITE32(_tag, _class, _handler, _unitmask) \
	curentry->set_handler(device, write32_delegate(&_class::_handler, #_class "::" #_handler, _tag, (_class *)0), _unitmask);

// device reads/writes
#define AM_DEVREADWRITE(_tag, _class, _rhandler, _whandler) \
	curentry->set_handler(device, read_delegate(&_class::_rhandler, #_class "::" #_rhandler, _tag, (_class *)0), write_delegate(&_class::_whandler, #_class "::" #_whandler, _tag, (_class *)0));
#define AM_DEVREADWRITE8(_tag, _class, _rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read8_delegate(&_class::_rhandler, #_class "::" #_rhandler, _tag, (_class *)0), write8_delegate(&_class::_whandler, #_class "::" #_whandler, _tag, (_class *)0), _unitmask);
#define AM_DEVREADWRITE16(_tag, _class, _rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read16_delegate(&_class::_rhandler, #_class "::" #_rhandler, _tag, (_class *)0), write16_delegate(&_class::_whandler, #_class "::" #_whandler, _tag, (_class *)0), _unitmask);
#define AM_DEVREADWRITE32(_tag, _class, _rhandler, _whandler, _unitmask) \
	curentry->set_handler(device, read32_delegate(&_class::_rhandler, #_class "::" #_rhandler, _tag, (_class *)0), write32_delegate(&_class::_whandler, #_class "::" #_whandler, _tag, (_class *)0), _unitmask);

// device mapping
#define AM_DEVICE(_tag, _class, _handler) \
	curentry->set_submap(device, _tag, address_map_delegate(&_class::_handler, #_class "::" #_handler, (_class *)0), 0, 0);
#define AM_DEVICE8(_tag, _class, _handler, _unitmask) \
	curentry->set_submap(device, _tag, address_map_delegate(&_class::_handler, #_class "::" #_handler, (_class *)0), 8, _unitmask);
#define AM_DEVICE16(_tag, _class, _handler, _unitmask) \
	curentry->set_submap(device, _tag, address_map_delegate(&_class::_handler, #_class "::" #_handler, (_class *)0), 16, _unitmask);
#define AM_DEVICE32(_tag, _class, _handler, _unitmask) \
	curentry->set_submap(device, _tag, address_map_delegate(&_class::_handler, #_class "::" #_handler, (_class *)0), 32, _unitmask);

// special-case accesses
#define AM_ROM \
	curentry->set_read_type(AMH_ROM);
#define AM_RAM \
	curentry->set_read_type(AMH_RAM); \
	curentry->set_write_type(AMH_RAM);
#define AM_READONLY \
	curentry->set_read_type(AMH_RAM);
#define AM_WRITEONLY \
	curentry->set_write_type(AMH_RAM);
#define AM_UNMAP \
	curentry->set_read_type(AMH_UNMAP); \
	curentry->set_write_type(AMH_UNMAP);
#define AM_READUNMAP \
	curentry->set_read_type(AMH_UNMAP);
#define AM_WRITEUNMAP \
	curentry->set_write_type(AMH_UNMAP);
#define AM_NOP \
	curentry->set_read_type(AMH_NOP); \
	curentry->set_write_type(AMH_NOP);
#define AM_READNOP \
	curentry->set_read_type(AMH_NOP);
#define AM_WRITENOP \
	curentry->set_write_type(AMH_NOP);

// port accesses
#define AM_READ_PORT(_tag) \
	curentry->set_read_port(device, _tag);
#define AM_WRITE_PORT(_tag) \
	curentry->set_write_port(device, _tag);
#define AM_READWRITE_PORT(_tag) \
	curentry->set_readwrite_port(device, _tag);

// bank accesses
#define AM_READ_BANK(_tag) \
	curentry->set_read_bank(device, _tag);
#define AM_WRITE_BANK(_tag) \
	curentry->set_write_bank(device, _tag);
#define AM_READWRITE_BANK(_tag) \
	curentry->set_readwrite_bank(device, _tag);

// attributes for accesses
#define AM_REGION(_tag, _offs) \
	curentry->set_region(_tag, _offs);
#define AM_SHARE(_tag) \
	curentry->set_share(_tag);

// common shortcuts
#define AM_ROMBANK(_bank)                   AM_READ_BANK(_bank)
#define AM_RAMBANK(_bank)                   AM_READWRITE_BANK(_bank)
#define AM_RAM_READ(_read)                  AM_READ(_read) AM_WRITEONLY
#define AM_RAM_WRITE(_write)                AM_READONLY AM_WRITE(_write)
#define AM_RAM_DEVREAD(_tag, _class, _read) AM_DEVREAD(_tag, _class, _read) AM_WRITEONLY
#define AM_RAM_DEVWRITE(_tag, _class, _write) AM_READONLY AM_DEVWRITE(_tag, _class, _write)


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// use this to refer to the owning device when providing a device tag
static const char DEVICE_SELF[] = "";

// use this to refer to the owning device's owner when providing a device tag
static const char DEVICE_SELF_OWNER[] = "^";


#endif  /* __ADDRMAP_H__ */
