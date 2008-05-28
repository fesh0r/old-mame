/***************************************************************************

    memory.h

    Functions which handle the CPU memory accesses.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "mamecore.h"
#include "devintrf.h"
#include "tokenize.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* address spaces */
enum
{
	ADDRESS_SPACE_PROGRAM = 0,		/* program address space */
	ADDRESS_SPACE_DATA,				/* data address space */
	ADDRESS_SPACE_IO,				/* I/O address space */
	ADDRESS_SPACES					/* maximum number of address spaces */
};


/* static data access handler constants */
enum
{
	STATIC_INVALID = 0,									/* invalid - should never be used */
	STATIC_BANK1 = 1,									/* first memory bank */
	/* entries 1-32 are for fixed banks 1-32 specified by the driver */
	/* entries 33-67 are for dynamically allocated internal banks */
	STATIC_BANKMAX = 67,								/* last memory bank */
	STATIC_RAM,											/* RAM - reads/writes map to dynamic banks */
	STATIC_ROM,											/* ROM - reads = RAM; writes = UNMAP */
	STATIC_NOP,											/* NOP - reads = unmapped value; writes = no-op */
	STATIC_UNMAP,										/* unmapped - same as NOP except we log errors */
	STATIC_COUNT										/* total number of static handlers */
};


/* address map tokens */
enum
{
	ADDRMAP_TOKEN_INVALID,

	ADDRMAP_TOKEN_START,
	ADDRMAP_TOKEN_END,
	ADDRMAP_TOKEN_INCLUDE,

	ADDRMAP_TOKEN_GLOBAL_MASK,
	ADDRMAP_TOKEN_UNMAP_VALUE,

	ADDRMAP_TOKEN_RANGE,
	ADDRMAP_TOKEN_MASK,
	ADDRMAP_TOKEN_MIRROR,
	ADDRMAP_TOKEN_READ,
	ADDRMAP_TOKEN_WRITE,
	ADDRMAP_TOKEN_DEVICE_READ,
	ADDRMAP_TOKEN_DEVICE_WRITE,
	ADDRMAP_TOKEN_READ_PORT,
	ADDRMAP_TOKEN_REGION,
	ADDRMAP_TOKEN_SHARE,
	ADDRMAP_TOKEN_BASEPTR,
	ADDRMAP_TOKEN_BASE_MEMBER,
	ADDRMAP_TOKEN_SIZEPTR,
	ADDRMAP_TOKEN_SIZE_MEMBER
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* handler_data is an opaque type used to hold information about a particular handler */
typedef struct _handler_data handler_data;


/* offsets and addresses are 32-bit (for now...) */
typedef UINT32	offs_t;


/* opbase_data contains state data for opcode handling */
typedef struct _opbase_data opbase_data;
struct _opbase_data
{
	UINT8 *					rom;				/* opcode ROM base pointer */
	UINT8 *					ram;				/* opcode RAM base pointer */
	offs_t					mask;				/* opcode ROM address mask */
	offs_t					mem_min;			/* opcode ROM/RAM min */
	offs_t					mem_max;			/* opcode ROM/RAM max */
	UINT8		 			entry;				/* opcode readmem entry */
};


/* opcode base adjustment handler */
typedef offs_t	(*opbase_handler_func) (ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t address, ATTR_UNUSED opbase_data *opbase);


/* machine read/write handlers */
typedef UINT8	(*read8_machine_func)  (ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset);
typedef void	(*write8_machine_func) (ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);
typedef UINT16	(*read16_machine_func) (ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 mem_mask);
typedef void	(*write16_machine_func)(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 data, ATTR_UNUSED UINT16 mem_mask);
typedef UINT32	(*read32_machine_func) (ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 mem_mask);
typedef void	(*write32_machine_func)(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 data, ATTR_UNUSED UINT32 mem_mask);
typedef UINT64	(*read64_machine_func) (ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 mem_mask);
typedef void	(*write64_machine_func)(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 data, ATTR_UNUSED UINT64 mem_mask);


/* device read/write handlers */
typedef UINT8	(*read8_device_func)  (ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset);
typedef void	(*write8_device_func) (ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);
typedef UINT16	(*read16_device_func) (ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 mem_mask);
typedef void	(*write16_device_func)(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 data, ATTR_UNUSED UINT16 mem_mask);
typedef UINT32	(*read32_device_func) (ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 mem_mask);
typedef void	(*write32_device_func)(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 data, ATTR_UNUSED UINT32 mem_mask);
typedef UINT64	(*read64_device_func) (ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 mem_mask);
typedef void	(*write64_device_func)(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 data, ATTR_UNUSED UINT64 mem_mask);


/* data_accessors is a struct with accessors of all flavors */
typedef struct _data_accessors data_accessors;
struct _data_accessors
{
	void		(*change_pc)(offs_t byteaddress);

	UINT8		(*read_byte)(offs_t byteaddress);
	UINT16		(*read_word)(offs_t byteaddress);
	UINT16		(*read_word_masked)(offs_t byteaddress, UINT16 mask);
	UINT32		(*read_dword)(offs_t byteaddress);
	UINT32		(*read_dword_masked)(offs_t byteaddress, UINT32 mask);
	UINT64		(*read_qword)(offs_t byteaddress);
	UINT64		(*read_qword_masked)(offs_t byteaddress, UINT64 mask);

	void		(*write_byte)(offs_t byteaddress, UINT8 data);
	void		(*write_word)(offs_t byteaddress, UINT16 data);
	void		(*write_word_masked)(offs_t byteaddress, UINT16 data, UINT16 mask);
	void		(*write_dword)(offs_t byteaddress, UINT32 data);
	void		(*write_dword_masked)(offs_t byteaddress, UINT32 data, UINT32 mask);
	void		(*write_qword)(offs_t byteaddress, UINT64 data);
	void		(*write_qword_masked)(offs_t byteaddress, UINT64 data, UINT64 mask);
};


/* read_handler is a union of all the different read handler types */
typedef union _read_handler read_handler;
union _read_handler
{
	genf *					generic;			/* generic function pointer */
	read8_machine_func		mhandler8;			/* 8-bit machine read handler */
	read16_machine_func		mhandler16;			/* 16-bit machine read handler */
	read32_machine_func		mhandler32;			/* 32-bit machine read handler */
	read64_machine_func		mhandler64;			/* 64-bit machine read handler */
	read8_device_func		dhandler8;			/* 8-bit device read handler */
	read16_device_func		dhandler16;			/* 16-bit device read handler */
	read32_device_func		dhandler32;			/* 32-bit device read handler */
	read64_device_func		dhandler64;			/* 64-bit device read handler */
};


/* write_handler is a union of all the different write handler types */
typedef union _write_handler write_handler;
union _write_handler
{
	genf *					generic;			/* generic function pointer */
	write8_machine_func		mhandler8;			/* 8-bit machine write handler */
	write16_machine_func	mhandler16;			/* 16-bit machine write handler */
	write32_machine_func	mhandler32;			/* 32-bit machine write handler */
	write64_machine_func	mhandler64;			/* 64-bit machine write handler */
	write8_device_func		dhandler8;			/* 8-bit device write handler */
	write16_device_func		dhandler16;			/* 16-bit device write handler */
	write32_device_func		dhandler32;			/* 32-bit device write handler */
	write64_device_func		dhandler64;			/* 64-bit device write handler */
};


/* memory_handler is a union of all read and write handler types */
typedef union _memory_handler memory_handler;
union _memory_handler
{
	genf *					generic;			/* generic function pointer */
	read_handler			read;				/* read handler union */
	write_handler			write;				/* write handler union */
};


/* address_map_entry is a linked list element describing one address range in a map */
typedef struct _address_map_entry address_map_entry;
struct _address_map_entry
{
	address_map_entry *		next;				/* pointer to the next entry */

	offs_t					addrstart;			/* start address */
	offs_t					addrend;			/* end address */
	offs_t					addrmirror;			/* mirror bits */
	offs_t					addrmask;			/* mask bits */
	read_handler 			read;				/* read handler callback */
	UINT8					read_bits;			/* bits for the read handler callback (0=default, 1=8, 2=16, 3=32) */
	UINT8					read_mask;			/* mask bits indicating which subunits to process */
	const char *			read_name;			/* read handler callback name */
	device_type				read_devtype;		/* read device type for device references */
	const char *			read_devtag;		/* read tag for the relevant device */
	const char *			read_porttag;		/* tag for input port reading */
	write_handler 			write;				/* write handler callback */
	UINT8					write_bits;			/* bits for the write handler callback (0=default, 1=8, 2=16, 3=32) */
	UINT8					write_mask;			/* mask bits indicating which subunits to process */
	const char *			write_name;			/* write handler callback name */
	device_type				write_devtype;		/* read device type for device references */
	const char *			write_devtag;		/* read tag for the relevant device */
	UINT32					share;				/* index of a shared memory block */
	void **					baseptr;			/* receives pointer to memory (optional) */
	size_t *				sizeptr;			/* receives size of area in bytes (optional) */
	UINT32					baseptroffs_plus1;	/* offset of base pointer within driver_data, plus 1 */
	UINT32					sizeptroffs_plus1;	/* offset of size pointer within driver_data, plus 1 */
	UINT32					region;				/* region containing the memory backing this entry */
	offs_t					region_offs;		/* offset within the region */

	void *					memory;				/* pointer to memory backing this entry */
	offs_t					bytestart;			/* byte-adjusted start address */
	offs_t					byteend;			/* byte-adjusted end address */
	offs_t					bytemirror;			/* byte-adjusted mirror bits */
	offs_t					bytemask;			/* byte-adjusted mask bits */
};


/* address_map holds global map parameters plus the head of the list of entries */
typedef struct _address_map address_map;
struct _address_map
{
	UINT8					spacenum;			/* space number of the map */
	UINT8					databits;			/* data bits represented by the map */
	UINT8					unmapval;			/* unmapped memory value */
	offs_t					globalmask;			/* global mask */
	address_map_entry *		entrylist;			/* list of entries */
};


/* address_space holds live information about an address space */
typedef struct _address_space address_space;
struct _address_space
{
	offs_t					bytemask;			/* byte-adjusted address mask */
	UINT8 *					readlookup;			/* read table lookup */
	UINT8 *					writelookup;		/* write table lookup */
	handler_data *			readhandlers;		/* read handlers */
	handler_data *			writehandlers;		/* write handlers */
	const data_accessors *	accessors;			/* pointers to the data access handlers */
};


/* addrmap_token is a union of all types for a generic address map */
typedef union _addrmap_token addrmap_token;
union _addrmap_token
{
	TOKEN_COMMON_FIELDS
	const addrmap_token *	tokenptr;
	read_handler			read;				/* generic read handlers */
	write_handler			write;				/* generic write handlers */
	device_type				devtype;			/* device type */
	UINT8 **				memptr;				/* memory pointer */
	size_t *				sizeptr;			/* size pointer */
};


/* addrmap8_token is a union of all types for an 8-bit address map */
typedef union _addrmap8_token addrmap8_token;
union _addrmap8_token
{
	TOKEN_COMMON_FIELDS
	const addrmap_token *	tokenptr;
	read8_machine_func		mread;				/* pointer to native machine read handler */
	write8_machine_func		mwrite;				/* pointer to native machine write handler */
	read8_device_func		dread;				/* pointer to native device read handler */
	write8_device_func		dwrite;				/* pointer to native device write handler */
	read_handler			read;				/* generic read handlers */
	write_handler			write;				/* generic write handlers */
	device_type				devtype;			/* device type */
	UINT8 **				memptr;				/* memory pointer */
	size_t *				sizeptr;			/* size pointer */
};


/* addrmap16_token is a union of all types for a 16-bit address map */
typedef union _addrmap16_token addrmap16_token;
union _addrmap16_token
{
	TOKEN_COMMON_FIELDS
	const addrmap_token *	tokenptr;
	read16_machine_func		mread;				/* pointer to native read handler */
	write16_machine_func 	mwrite;				/* pointer to native write handler */
	read16_device_func		dread;				/* pointer to native device read handler */
	write16_device_func		dwrite;				/* pointer to native device write handler */
	read8_machine_func		mread8;				/* pointer to 8-bit machine read handler */
	write8_machine_func		mwrite8;			/* pointer to 8-bit machine write handler */
	read8_device_func		dread8;				/* pointer to 8-bit device read handler */
	write8_device_func		dwrite8;			/* pointer to 8-bit device write handler */
	read_handler			read;				/* generic read handlers */
	write_handler			write;				/* generic write handlers */
	device_type				devtype;			/* device type */
	UINT16 **				memptr;				/* memory pointer */
	size_t *				sizeptr;			/* size pointer */
};


/* addrmap32_token is a union of all types for a 32-bit address map */
typedef union _addrmap32_token addrmap32_token;
union _addrmap32_token
{
	TOKEN_COMMON_FIELDS
	const addrmap_token *	tokenptr;
	read32_machine_func		mread;				/* pointer to native read handler */
	write32_machine_func 	mwrite;				/* pointer to native write handler */
	read32_device_func		dread;				/* pointer to native device read handler */
	write32_device_func		dwrite;				/* pointer to native device write handler */
	read8_machine_func		mread8;				/* pointer to 8-bit machine read handler */
	write8_machine_func		mwrite8;			/* pointer to 8-bit machine write handler */
	read8_device_func		dread8;				/* pointer to 8-bit device read handler */
	write8_device_func		dwrite8;			/* pointer to 8-bit device write handler */
	read16_machine_func		mread16;			/* pointer to 16-bit machine read handler */
	write16_machine_func	mwrite16;			/* pointer to 16-bit machine write handler */
	read16_device_func		dread16;			/* pointer to 16-bit device read handler */
	write16_device_func		dwrite16;			/* pointer to 16-bit device write handler */
	read_handler			read;				/* generic read handlers */
	write_handler			write;				/* generic write handlers */
	device_type				devtype;			/* device type */
	UINT32 **				memptr;				/* memory pointer */
	size_t *				sizeptr;			/* size pointer */
};


/* addrmap64_token is a union of all types for a 64-bit address map */
typedef union _addrmap64_token addrmap64_token;
union _addrmap64_token
{
	TOKEN_COMMON_FIELDS
	const addrmap_token *	tokenptr;
	read64_machine_func		mread;				/* pointer to native read handler */
	write64_machine_func 	mwrite;				/* pointer to native write handler */
	read64_device_func		dread;				/* pointer to native device read handler */
	write64_device_func		dwrite;				/* pointer to native device write handler */
	read8_machine_func		mread8;				/* pointer to 8-bit machine read handler */
	write8_machine_func		mwrite8;			/* pointer to 8-bit machine write handler */
	read8_device_func		dread8;				/* pointer to 8-bit device read handler */
	write8_device_func		dwrite8;			/* pointer to 8-bit device write handler */
	read16_machine_func		mread16;			/* pointer to 16-bit machine read handler */
	write16_machine_func	mwrite16;			/* pointer to 16-bit machine write handler */
	read16_device_func		dread16;			/* pointer to 16-bit device read handler */
	write16_device_func		dwrite16;			/* pointer to 16-bit device write handler */
	read32_machine_func		mread32;			/* pointer to 32-bit machine read handler */
	write32_machine_func	mwrite32;			/* pointer to 32-bit machine write handler */
	read32_device_func		dread32;			/* pointer to 32-bit device read handler */
	write32_device_func		dwrite32;			/* pointer to 32-bit device write handler */
	read_handler			read;				/* generic read handlers */
	write_handler			write;				/* generic write handlers */
	device_type				devtype;			/* device type */
	UINT64 **				memptr;				/* memory pointer */
	size_t *				sizeptr;			/* size pointer */
};



/***************************************************************************
    MACROS
***************************************************************************/

/* opcode base adjustment handler function macro */
#define OPBASE_HANDLER(name)			offs_t name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t address, opbase_data *opbase)


/* machine read/write handler function macros */
#define READ8_HANDLER(name) 			UINT8  name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset)
#define WRITE8_HANDLER(name) 			void   name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data)
#define READ16_HANDLER(name)			UINT16 name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 mem_mask)
#define WRITE16_HANDLER(name)			void   name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 data, ATTR_UNUSED UINT16 mem_mask)
#define READ32_HANDLER(name)			UINT32 name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 mem_mask)
#define WRITE32_HANDLER(name)			void   name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 data, ATTR_UNUSED UINT32 mem_mask)
#define READ64_HANDLER(name)			UINT64 name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 mem_mask)
#define WRITE64_HANDLER(name)			void   name(ATTR_UNUSED running_machine *machine, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 data, ATTR_UNUSED UINT64 mem_mask)


/* device read/write handler function macros */
#define READ8_DEVICE_HANDLER(name) 		UINT8  name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset)
#define WRITE8_DEVICE_HANDLER(name) 	void   name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data)
#define READ16_DEVICE_HANDLER(name)		UINT16 name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 mem_mask)
#define WRITE16_DEVICE_HANDLER(name)	void   name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT16 data, ATTR_UNUSED UINT16 mem_mask)
#define READ32_DEVICE_HANDLER(name)		UINT32 name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 mem_mask)
#define WRITE32_DEVICE_HANDLER(name)	void   name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT32 data, ATTR_UNUSED UINT32 mem_mask)
#define READ64_DEVICE_HANDLER(name)		UINT64 name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 mem_mask)
#define WRITE64_DEVICE_HANDLER(name)	void   name(ATTR_UNUSED const device_config *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT64 data, ATTR_UNUSED UINT64 mem_mask)


/* static memory handler (SMH) macros that can be used in place of read/write handlers */
#define SMH_RAM							((void *)STATIC_RAM)
#define SMH_ROM							((void *)STATIC_ROM)
#define SMH_NOP							((void *)STATIC_NOP)
#define SMH_UNMAP						((void *)STATIC_UNMAP)
#define SMH_BANK(n)						((void *)(STATIC_BANK1 + (n) - 1))
#define SMH_BANK1						SMH_BANK(1)
#define SMH_BANK2						SMH_BANK(2)
#define SMH_BANK3						SMH_BANK(3)
#define SMH_BANK4						SMH_BANK(4)
#define SMH_BANK5						SMH_BANK(5)
#define SMH_BANK6						SMH_BANK(6)
#define SMH_BANK7						SMH_BANK(7)
#define SMH_BANK8						SMH_BANK(8)
#define SMH_BANK9						SMH_BANK(9)
#define SMH_BANK10						SMH_BANK(10)
#define SMH_BANK11						SMH_BANK(11)
#define SMH_BANK12						SMH_BANK(12)
#define SMH_BANK13						SMH_BANK(13)
#define SMH_BANK14						SMH_BANK(14)
#define SMH_BANK15						SMH_BANK(15)
#define SMH_BANK16						SMH_BANK(16)
#define SMH_BANK17						SMH_BANK(17)
#define SMH_BANK18						SMH_BANK(18)
#define SMH_BANK19						SMH_BANK(19)
#define SMH_BANK20						SMH_BANK(20)
#define SMH_BANK21						SMH_BANK(21)
#define SMH_BANK22						SMH_BANK(22)
#define SMH_BANK23						SMH_BANK(23)
#define SMH_BANK24						SMH_BANK(24)
#define SMH_BANK25						SMH_BANK(25)
#define SMH_BANK26						SMH_BANK(26)
#define SMH_BANK27						SMH_BANK(27)
#define SMH_BANK28						SMH_BANK(28)
#define SMH_BANK29						SMH_BANK(29)
#define SMH_BANK30						SMH_BANK(30)
#define SMH_BANK31						SMH_BANK(31)
#define SMH_BANK32						SMH_BANK(32)


/* helper macro for merging data with the memory mask */
#define COMBINE_DATA(varptr)			(*(varptr) = (*(varptr) & ~mem_mask) | (data & mem_mask))

#define ACCESSING_BITS_0_7				((mem_mask & 0x000000ff) != 0)
#define ACCESSING_BITS_8_15				((mem_mask & 0x0000ff00) != 0)
#define ACCESSING_BITS_16_23			((mem_mask & 0x00ff0000) != 0)
#define ACCESSING_BITS_24_31			((mem_mask & 0xff000000) != 0)
#define ACCESSING_BITS_32_39			((mem_mask & U64(0x000000ff00000000)) != 0)
#define ACCESSING_BITS_40_47			((mem_mask & U64(0x0000ff0000000000)) != 0)
#define ACCESSING_BITS_48_55			((mem_mask & U64(0x00ff000000000000)) != 0)
#define ACCESSING_BITS_56_63			((mem_mask & U64(0xff00000000000000)) != 0)

#define ACCESSING_BITS_0_15				((mem_mask & 0x0000ffff) != 0)
#define ACCESSING_BITS_16_31			((mem_mask & 0xffff0000) != 0)
#define ACCESSING_BITS_32_47			((mem_mask & U64(0x0000ffff00000000)) != 0)
#define ACCESSING_BITS_48_63			((mem_mask & U64(0xffff000000000000)) != 0)

#define ACCESSING_BITS_0_31				((mem_mask & 0xffffffff) != 0)
#define ACCESSING_BITS_32_63			((mem_mask & U64(0xffffffff00000000)) != 0)


/* bank switching for CPU cores */
#define change_pc(byteaddress)			memory_set_opbase(byteaddress)


/* opcode range safety check */
#define address_is_unsafe(A)			((UNEXPECTED((A) < opbase.mem_min) || UNEXPECTED((A) > opbase.mem_max)))


/* unsafe opcode and opcode argument reading */
#define cpu_opptr_unsafe(A)				((void *)&opbase.rom[(A) & opbase.mask])
#define cpu_readop_unsafe(A)			(opbase.rom[(A) & opbase.mask])
#define cpu_readop16_unsafe(A)			(*(UINT16 *)&opbase.rom[(A) & opbase.mask])
#define cpu_readop32_unsafe(A)			(*(UINT32 *)&opbase.rom[(A) & opbase.mask])
#define cpu_readop64_unsafe(A)			(*(UINT64 *)&opbase.rom[(A) & opbase.mask])
#define cpu_readop_arg_unsafe(A)		(opbase.ram[(A) & opbase.mask])
#define cpu_readop_arg16_unsafe(A)		(*(UINT16 *)&opbase.ram[(A) & opbase.mask])
#define cpu_readop_arg32_unsafe(A)		(*(UINT32 *)&opbase.ram[(A) & opbase.mask])
#define cpu_readop_arg64_unsafe(A)		(*(UINT64 *)&opbase.ram[(A) & opbase.mask])


/* wrappers for dynamic read handler installation */
#define memory_install_read_handler(machine, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_handler(machine, cpu, space, start, end, mask, mirror, rhandler, (FPTR)NULL, #rhandler, NULL)
#define memory_install_read8_handler(machine, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_handler8(machine, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)
#define memory_install_read16_handler(machine, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_handler16(machine, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)
#define memory_install_read32_handler(machine, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_handler32(machine, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)
#define memory_install_read64_handler(machine, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_handler64(machine, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)

#define memory_install_read_device_handler(device, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_device_handler(device, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)
#define memory_install_read8_device_handler(device, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_device_handler8(device, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)
#define memory_install_read16_device_handler(device, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_device_handler16(device, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)
#define memory_install_read32_device_handler(device, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_device_handler32(device, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)
#define memory_install_read64_device_handler(device, cpu, space, start, end, mask, mirror, rhandler) \
	_memory_install_device_handler64(device, cpu, space, start, end, mask, mirror, rhandler, NULL, #rhandler, NULL)


/* wrappers for dynamic write handler installation */
#define memory_install_write_handler(machine, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_handler(machine, cpu, space, start, end, mask, mirror, (FPTR)NULL, whandler, NULL, #whandler)
#define memory_install_write8_handler(machine, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_handler8(machine, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)
#define memory_install_write16_handler(machine, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_handler16(machine, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)
#define memory_install_write32_handler(machine, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_handler32(machine, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)
#define memory_install_write64_handler(machine, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_handler64(machine, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)

#define memory_install_write_device_handler(device, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_device_handler(device, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)
#define memory_install_write8_device_handler(device, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_device_handler8(device, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)
#define memory_install_write16_device_handler(device, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_device_handler16(device, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)
#define memory_install_write32_device_handler(device, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_device_handler32(device, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)
#define memory_install_write64_device_handler(device, cpu, space, start, end, mask, mirror, whandler) \
	_memory_install_device_handler64(device, cpu, space, start, end, mask, mirror, NULL, whandler, NULL, #whandler)


/* wrappers for dynamic read/write handler installation */
#define memory_install_readwrite_handler(machine, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_handler(machine, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite8_handler(machine, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_handler8(machine, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite16_handler(machine, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_handler16(machine, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite32_handler(machine, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_handler32(machine, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite64_handler(machine, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_handler64(machine, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)

#define memory_install_readwrite_device_handler(device, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_device_handler(device, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite8_device_handler(device, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_device_handler8(device, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite16_device_handler(device, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_device_handler16(device, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite32_device_handler(device, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_device_handler32(device, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)
#define memory_install_readwrite64_device_handler(device, cpu, space, start, end, mask, mirror, rhandler, whandler) \
	_memory_install_device_handler64(device, cpu, space, start, end, mask, mirror, rhandler, whandler, #rhandler, #whandler)


/* macros for accessing bytes and words within larger chunks */
#ifdef LSB_FIRST

#define BYTE_XOR_BE(a)  				((a) ^ 1)		/* read/write a byte to a 16-bit space */
#define BYTE_XOR_LE(a)  				(a)
#define BYTE4_XOR_BE(a) 				((a) ^ 3)		/* read/write a byte to a 32-bit space */
#define BYTE4_XOR_LE(a) 				(a)
#define WORD_XOR_BE(a)  				((a) ^ 2)		/* read/write a word to a 32-bit space */
#define WORD_XOR_LE(a)  				(a)
#define BYTE8_XOR_BE(a) 				((a) ^ 7)		/* read/write a byte to a 64-bit space */
#define BYTE8_XOR_LE(a) 				(a)
#define WORD2_XOR_BE(a)  				((a) ^ 6)		/* read/write a word to a 64-bit space */
#define WORD2_XOR_LE(a)  				(a)
#define DWORD_XOR_BE(a)  				((a) ^ 4)		/* read/write a dword to a 64-bit space */
#define DWORD_XOR_LE(a)  				(a)

#else

#define BYTE_XOR_BE(a)  				(a)
#define BYTE_XOR_LE(a)  				((a) ^ 1)		/* read/write a byte to a 16-bit space */
#define BYTE4_XOR_BE(a) 				(a)
#define BYTE4_XOR_LE(a) 				((a) ^ 3)		/* read/write a byte to a 32-bit space */
#define WORD_XOR_BE(a)  				(a)
#define WORD_XOR_LE(a)  				((a) ^ 2)		/* read/write a word to a 32-bit space */
#define BYTE8_XOR_BE(a) 				(a)
#define BYTE8_XOR_LE(a) 				((a) ^ 7)		/* read/write a byte to a 64-bit space */
#define WORD2_XOR_BE(a)  				(a)
#define WORD2_XOR_LE(a)  				((a) ^ 6)		/* read/write a word to a 64-bit space */
#define DWORD_XOR_BE(a)  				(a)
#define DWORD_XOR_LE(a)  				((a) ^ 4)		/* read/write a dword to a 64-bit space */

#endif



/***************************************************************************
    ADDRESS MAP MACROS
***************************************************************************/

/* so that "0" can be used for unneeded address maps */
#define address_map_0 NULL


/* maps a full 64-bit mask down to an 8-bit byte mask */
#define UNITMASK8(x) \
	((((UINT64)(x) >> (63-7)) & 0x80) | \
	 (((UINT64)(x) >> (55-6)) & 0x40) | \
	 (((UINT64)(x) >> (47-5)) & 0x20) | \
	 (((UINT64)(x) >> (39-4)) & 0x10) | \
	 (((UINT64)(x) >> (31-3)) & 0x08) | \
	 (((UINT64)(x) >> (23-2)) & 0x04) | \
	 (((UINT64)(x) >> (15-1)) & 0x02) | \
	 (((UINT64)(x) >> ( 7-0)) & 0x01))

/* maps a full 64-bit mask down to a 4-bit word mask */
#define UNITMASK16(x) \
	((((UINT64)(x) >> (63-3)) & 0x08) | \
	 (((UINT64)(x) >> (47-2)) & 0x04) | \
	 (((UINT64)(x) >> (31-1)) & 0x02) | \
	 (((UINT64)(x) >> (15-0)) & 0x01))

/* maps a full 64-bit mask down to a 2-bit dword mask */
#define UNITMASK32(x) \
	((((UINT64)(x) >> (63-1)) & 0x02) | \
	 (((UINT64)(x) >> (31-0)) & 0x01))



/* start/end tags for the address map */
#define ADDRESS_MAP_START(_name, _space, _bits) \
	const addrmap##_bits##_token address_map_##_name[] = { \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_START, 8, _space, 8, _bits, 8),

#define ADDRESS_MAP_END \
	TOKEN_UINT32_PACK1(ADDRMAP_TOKEN_END, 8) };

/* use this to declare external references to an address map */
#define ADDRESS_MAP_EXTERN(_name, _bits) \
	extern const addrmap##_bits##_token address_map_##_name[]


/* global controls */
#define ADDRESS_MAP_GLOBAL_MASK(_mask) \
	TOKEN_UINT64_PACK2(ADDRMAP_TOKEN_GLOBAL_MASK, 8, _mask, 32),

#define ADDRESS_MAP_UNMAP_LOW \
	TOKEN_UINT32_PACK2(ADDRMAP_TOKEN_UNMAP_VALUE, 8, 0, 1),

#define ADDRESS_MAP_UNMAP_HIGH \
	TOKEN_UINT32_PACK2(ADDRMAP_TOKEN_UNMAP_VALUE, 8, 1, 1),


/* importing data from other address maps */
#define AM_IMPORT_FROM(_name) \
	TOKEN_UINT32_PACK1(ADDRMAP_TOKEN_INCLUDE, 8), \
	TOKEN_PTR(tokenptr, address_map_##_name),


/* address ranges */
#define AM_RANGE(_start, _end) \
	TOKEN_UINT32_PACK1(ADDRMAP_TOKEN_RANGE, 8), \
	TOKEN_UINT64_PACK2(_start, 32, _end, 32),

#define AM_MASK(_mask) \
	TOKEN_UINT64_PACK2(ADDRMAP_TOKEN_MASK, 8, _mask, 32),

#define AM_MIRROR(_mirror) \
	TOKEN_UINT64_PACK2(ADDRMAP_TOKEN_MIRROR, 8, _mirror, 32),

#define AM_READ(_handler) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_READ, 8, 0, 8, 0, 8), \
	TOKEN_PTR(mread, _handler), \
	TOKEN_STRING(#_handler),

#define AM_READ8(_handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_READ, 8, 8, 8, UNITMASK8(_unitmask), 8), \
	TOKEN_PTR(mread8, _handler), \
	TOKEN_STRING(#_handler),

#define AM_READ16(_handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_READ, 8, 16, 8, UNITMASK16(_unitmask), 8), \
	TOKEN_PTR(mread16, _handler), \
	TOKEN_STRING(#_handler),

#define AM_READ32(_handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_READ, 8, 32, 8, UNITMASK32(_unitmask), 8), \
	TOKEN_PTR(mread32, _handler), \
	TOKEN_STRING(#_handler),

#define AM_WRITE(_handler) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_WRITE, 8, 0, 8, 0, 8), \
	TOKEN_PTR(mwrite, _handler), \
	TOKEN_STRING(#_handler),

#define AM_WRITE8(_handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_WRITE, 8, 8, 8, UNITMASK8(_unitmask), 8), \
	TOKEN_PTR(mwrite8, _handler), \
	TOKEN_STRING(#_handler),

#define AM_WRITE16(_handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_WRITE, 8, 16, 8, UNITMASK16(_unitmask), 8), \
	TOKEN_PTR(mwrite16, _handler), \
	TOKEN_STRING(#_handler),

#define AM_WRITE32(_handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_WRITE, 8, 32, 8, UNITMASK32(_unitmask), 8), \
	TOKEN_PTR(mwrite32, _handler), \
	TOKEN_STRING(#_handler),

#define AM_DEVREAD(_type, _tag, _handler) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_READ, 8, 0, 8, 0, 8), \
	TOKEN_PTR(dread, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_DEVREAD8(_type, _tag, _handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_READ, 8, 8, 8, UNITMASK8(_unitmask), 8), \
	TOKEN_PTR(dread8, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_DEVREAD16(_type, _tag, _handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_READ, 8, 16, 8, UNITMASK16(_unitmask), 8), \
	TOKEN_PTR(dread16, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_DEVREAD32(_type, _tag, _handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_READ, 8, 32, 8, UNITMASK32(_unitmask), 8), \
	TOKEN_PTR(dread32, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_DEVWRITE(_type, _tag, _handler) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_WRITE, 8, 0, 8, 0, 8), \
	TOKEN_PTR(dwrite, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_DEVWRITE8(_type, _tag, _handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_WRITE, 8, 8, 8, UNITMASK8(_unitmask), 8), \
	TOKEN_PTR(dwrite8, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_DEVWRITE16(_type, _tag, _handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_WRITE, 8, 16, 8, UNITMASK16(_unitmask), 8), \
	TOKEN_PTR(dwrite16, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_DEVWRITE32(_type, _tag, _handler, _unitmask) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_DEVICE_WRITE, 8, 32, 8, UNITMASK32(_unitmask), 8), \
	TOKEN_PTR(dwrite32, _handler), \
	TOKEN_STRING(#_handler), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define AM_READ_PORT(_tag) \
	TOKEN_UINT32_PACK3(ADDRMAP_TOKEN_READ_PORT, 8, 0, 8, 0, 8), \
	TOKEN_STRING(_tag),

#define AM_REGION(_region, _offs) \
	TOKEN_UINT64_PACK3(ADDRMAP_TOKEN_REGION, 8, _region, 24, _offs, 32),

#define AM_SHARE(_index) \
	TOKEN_UINT32_PACK2(ADDRMAP_TOKEN_SHARE, 8, _index, 24),

#define AM_BASE(_base) \
	TOKEN_UINT32_PACK1(ADDRMAP_TOKEN_BASEPTR, 8), \
	TOKEN_PTR(memptr, _base),

#define AM_BASE_MEMBER(_struct, _member) \
	TOKEN_UINT32_PACK2(ADDRMAP_TOKEN_BASE_MEMBER, 8, offsetof(_struct, _member), 24),

#define AM_SIZE(_size) \
	TOKEN_UINT32_PACK1(ADDRMAP_TOKEN_SIZEPTR, 8), \
	TOKEN_PTR(sizeptr, _size),

#define AM_SIZE_MEMBER(_struct, _member) \
	TOKEN_UINT32_PACK2(ADDRMAP_TOKEN_SIZE_MEMBER, 8, offsetof(_struct, _member), 24),


/* common shortcuts */
#define AM_READWRITE(_read,_write)			AM_READ(_read) AM_WRITE(_write)
#define AM_READWRITE8(_read,_write,_mask)	AM_READ8(_read,_mask) AM_WRITE8(_write,_mask)
#define AM_READWRITE16(_read,_write,_mask)	AM_READ16(_read,_mask) AM_WRITE16(_write,_mask)
#define AM_READWRITE32(_read,_write,_mask)	AM_READ32(_read,_mask) AM_WRITE32(_write,_mask)

#define AM_DEVREADWRITE(_type,_tag,_read,_write) AM_DEVREAD(_type,_tag,_read) AM_DEVWRITE(_type,_tag,_write)
#define AM_DEVREADWRITE8(_type,_tag,_read,_write,_mask) AM_DEVREAD8(_type,_tag,_read,_mask) AM_DEVWRITE8(_type,_tag,_write,_mask)
#define AM_DEVREADWRITE16(_type,_tag,_read,_write,_mask) AM_DEVREAD16(_type,_tag,_read,_mask) AM_DEVWRITE16(_type,_tag,_write,_mask)
#define AM_DEVREADWRITE32(_type,_tag,_read,_write,_mask) AM_DEVREAD32(_type,_tag,_read,_mask) AM_DEVWRITE32(_type,_tag,_write,_mask)

#define AM_ROM								AM_READ(SMH_ROM)
#define AM_ROMBANK(_bank)					AM_READ(SMH_BANK(_bank))

#define AM_RAM								AM_READWRITE(SMH_RAM, SMH_RAM)
#define AM_RAMBANK(_bank)					AM_READWRITE(SMH_BANK(_bank), SMH_BANK(_bank))
#define AM_RAM_WRITE(_write)				AM_READWRITE(SMH_RAM, _write)
#define AM_WRITEONLY						AM_WRITE(SMH_RAM)

#define AM_UNMAP							AM_READWRITE(SMH_UNMAP, SMH_UNMAP)
#define AM_NOP								AM_READWRITE(SMH_NOP, SMH_NOP)
#define AM_READNOP							AM_READ(SMH_NOP)
#define AM_WRITENOP							AM_WRITE(SMH_NOP)



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern address_space	active_address_space[];		/* address spaces */

extern const char *const address_space_names[ADDRESS_SPACES];



/***************************************************************************
    FUNCTION PROTOTYPES FOR CORE MEMORY FUNCTIONS
***************************************************************************/


/* ----- core system operations ----- */

/* initialize the memory system */
void memory_init(running_machine *machine);

/* set the current memory context */
void memory_set_context(int activecpu);

/* get a pointer to the set of memory accessor functions based on the address space,
   databus width, and endianness */
const data_accessors *memory_get_accessors(int spacenum, int databits, int endianness);



/* ----- address maps ----- */

/* build and allocate an address map for a CPU's address space */
address_map *address_map_alloc(const machine_config *drv, int cpunum, int spacenum);

/* release allocated memory for an address map */
void address_map_free(address_map *map);

/* return a pointer to the constructed address map for a CPU's address space */
const address_map *memory_get_address_map(int cpunum, int spacenum);



/* ----- opcode base control ----- */

/* registers an address range as having a decrypted data pointer */
void memory_set_decrypted_region(int cpunum, offs_t addrstart, offs_t addrend, void *base);

/* register a handler for opcode base changes on a given CPU */
opbase_handler_func memory_set_opbase_handler(int cpunum, opbase_handler_func function);

/* called by CPU cores to update the opcode base for the given address */
void memory_set_opbase(offs_t byteaddress);



/* return a base pointer to memory */
void *		memory_get_read_ptr(int cpunum, int spacenum, offs_t byteaddress);
void *		memory_get_write_ptr(int cpunum, int spacenum, offs_t byteaddress);
void *		memory_get_op_ptr(int cpunum, offs_t byteaddress, int arg);

/* memory banking */
void		memory_configure_bank(int banknum, int startentry, int numentries, void *base, offs_t stride);
void		memory_configure_bank_decrypted(int banknum, int startentry, int numentries, void *base, offs_t stride);
void		memory_set_bank(int banknum, int entrynum);
int 		memory_get_bank(int banknum);
void		memory_set_bankptr(int banknum, void *base);


/* debugging */
void		memory_set_debugger_access(int debugger);
void		memory_set_log_unmap(int spacenum, int log);
int			memory_get_log_unmap(int spacenum);

/* dynamic address space mapping */
void *		_memory_install_handler  (running_machine *machine, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, FPTR rhandler, FPTR whandler, const char *rhandler_name, const char *whandler_name);
UINT8 *		_memory_install_handler8 (running_machine *machine, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read8_machine_func rhandler, write8_machine_func whandler, const char *rhandler_name, const char *whandler_name);
UINT16 *	_memory_install_handler16(running_machine *machine, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read16_machine_func rhandler, write16_machine_func whandler, const char *rhandler_name, const char *whandler_name);
UINT32 *	_memory_install_handler32(running_machine *machine, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read32_machine_func rhandler, write32_machine_func whandler, const char *rhandler_name, const char *whandler_name);
UINT64 *	_memory_install_handler64(running_machine *machine, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read64_machine_func rhandler, write64_machine_func whandler, const char *rhandler_name, const char *whandler_name);

/* dynamic device address space mapping */
void *		_memory_install_device_handler  (const device_config *device, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, FPTR rhandler, FPTR whandler, const char *rhandler_name, const char *whandler_name);
UINT8 *		_memory_install_device_handler8 (const device_config *device, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read8_device_func rhandler, write8_device_func whandler, const char *rhandler_name, const char *whandler_name);
UINT16 *	_memory_install_device_handler16(const device_config *device, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read16_device_func rhandler, write16_device_func whandler, const char *rhandler_name, const char *whandler_name);
UINT32 *	_memory_install_device_handler32(const device_config *device, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read32_device_func rhandler, write32_device_func whandler, const char *rhandler_name, const char *whandler_name);
UINT64 *	_memory_install_device_handler64(const device_config *device, int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read64_device_func rhandler, write64_device_func whandler, const char *rhandler_name, const char *whandler_name);

/* memory debugging */
void 		memory_dump(FILE *file);
const char *memory_get_handler_string(int read0_or_write1, int cpunum, int spacenum, offs_t byteaddress);



/***************************************************************************
    HELPER MACROS AND INLINES
***************************************************************************/

/* generic memory access */
INLINE UINT8  program_read_byte (offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_byte)(byteaddress); }
INLINE UINT16 program_read_word (offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_word)(byteaddress); }
INLINE UINT32 program_read_dword(offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_dword)(byteaddress); }
INLINE UINT64 program_read_qword(offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->read_qword)(byteaddress); }

INLINE void	program_write_byte (offs_t byteaddress, UINT8  data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_byte)(byteaddress, data); }
INLINE void	program_write_word (offs_t byteaddress, UINT16 data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_word)(byteaddress, data); }
INLINE void	program_write_dword(offs_t byteaddress, UINT32 data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_dword)(byteaddress, data); }
INLINE void	program_write_qword(offs_t byteaddress, UINT64 data) { (*active_address_space[ADDRESS_SPACE_PROGRAM].accessors->write_qword)(byteaddress, data); }

INLINE UINT8  data_read_byte (offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_byte)(byteaddress); }
INLINE UINT16 data_read_word (offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_word)(byteaddress); }
INLINE UINT32 data_read_dword(offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_dword)(byteaddress); }
INLINE UINT64 data_read_qword(offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_DATA].accessors->read_qword)(byteaddress); }

INLINE void	data_write_byte (offs_t byteaddress, UINT8  data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_byte)(byteaddress, data); }
INLINE void	data_write_word (offs_t byteaddress, UINT16 data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_word)(byteaddress, data); }
INLINE void	data_write_dword(offs_t byteaddress, UINT32 data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_dword)(byteaddress, data); }
INLINE void	data_write_qword(offs_t byteaddress, UINT64 data) { (*active_address_space[ADDRESS_SPACE_DATA].accessors->write_qword)(byteaddress, data); }

INLINE UINT8  io_read_byte (offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_byte)(byteaddress); }
INLINE UINT16 io_read_word (offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_word)(byteaddress); }
INLINE UINT32 io_read_dword(offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_dword)(byteaddress); }
INLINE UINT64 io_read_qword(offs_t byteaddress) { return (*active_address_space[ADDRESS_SPACE_IO].accessors->read_qword)(byteaddress); }

INLINE void	io_write_byte (offs_t byteaddress, UINT8  data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_byte)(byteaddress, data); }
INLINE void	io_write_word (offs_t byteaddress, UINT16 data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_word)(byteaddress, data); }
INLINE void	io_write_dword(offs_t byteaddress, UINT32 data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_dword)(byteaddress, data); }
INLINE void	io_write_qword(offs_t byteaddress, UINT64 data) { (*active_address_space[ADDRESS_SPACE_IO].accessors->write_qword)(byteaddress, data); }

/* safe opcode and opcode argument reading */
UINT8	cpu_readop_safe(offs_t byteaddress);
UINT16	cpu_readop16_safe(offs_t byteaddress);
UINT32	cpu_readop32_safe(offs_t byteaddress);
UINT64	cpu_readop64_safe(offs_t byteaddress);
UINT8	cpu_readop_arg_safe(offs_t byteaddress);
UINT16	cpu_readop_arg16_safe(offs_t byteaddress);
UINT32	cpu_readop_arg32_safe(offs_t byteaddress);
UINT64	cpu_readop_arg64_safe(offs_t byteaddress);

/* opcode and opcode argument reading */
INLINE void * cpu_opptr(offs_t byteaddress)			{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_opptr_unsafe(byteaddress); }
INLINE UINT8  cpu_readop(offs_t byteaddress)		{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop_unsafe(byteaddress); }
INLINE UINT16 cpu_readop16(offs_t byteaddress)		{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop16_unsafe(byteaddress); }
INLINE UINT32 cpu_readop32(offs_t byteaddress)		{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop32_unsafe(byteaddress); }
INLINE UINT64 cpu_readop64(offs_t byteaddress)		{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop64_unsafe(byteaddress); }
INLINE UINT8  cpu_readop_arg(offs_t byteaddress)	{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop_arg_unsafe(byteaddress); }
INLINE UINT16 cpu_readop_arg16(offs_t byteaddress)	{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop_arg16_unsafe(byteaddress); }
INLINE UINT32 cpu_readop_arg32(offs_t byteaddress)	{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop_arg32_unsafe(byteaddress); }
INLINE UINT64 cpu_readop_arg64(offs_t byteaddress)	{ extern opbase_data opbase; if (address_is_unsafe(byteaddress)) { memory_set_opbase(byteaddress); } return cpu_readop_arg64_unsafe(byteaddress); }


/***************************************************************************
    FUNCTION PROTOTYPES FOR CORE READ/WRITE ROUTINES
***************************************************************************/

/* declare program address space handlers */
UINT8 program_read_byte_8le(offs_t address);
UINT16 program_read_word_8le(offs_t address);
UINT16 program_read_word_masked_8le(offs_t address, UINT16 mask);
UINT32 program_read_dword_8le(offs_t address);
UINT32 program_read_dword_masked_8le(offs_t address, UINT32 mask);
UINT64 program_read_qword_8le(offs_t address);
UINT64 program_read_qword_masked_8le(offs_t address, UINT64 mask);
void program_write_byte_8le(offs_t address, UINT8 data);
void program_write_word_8le(offs_t address, UINT16 data);
void program_write_word_masked_8le(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_8le(offs_t address, UINT32 data);
void program_write_dword_masked_8le(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_8le(offs_t address, UINT64 data);
void program_write_qword_masked_8le(offs_t address, UINT64 data, UINT64 mask);

UINT8 program_read_byte_8be(offs_t address);
UINT16 program_read_word_8be(offs_t address);
UINT16 program_read_word_masked_8be(offs_t address, UINT16 mask);
UINT32 program_read_dword_8be(offs_t address);
UINT32 program_read_dword_masked_8be(offs_t address, UINT32 mask);
UINT64 program_read_qword_8be(offs_t address);
UINT64 program_read_qword_masked_8be(offs_t address, UINT64 mask);
void program_write_byte_8be(offs_t address, UINT8 data);
void program_write_word_8be(offs_t address, UINT16 data);
void program_write_word_masked_8be(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_8be(offs_t address, UINT32 data);
void program_write_dword_masked_8be(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_8be(offs_t address, UINT64 data);
void program_write_qword_masked_8be(offs_t address, UINT64 data, UINT64 mask);

UINT8 program_read_byte_16le(offs_t address);
UINT16 program_read_word_16le(offs_t address);
UINT16 program_read_word_masked_16le(offs_t address, UINT16 mask);
UINT32 program_read_dword_16le(offs_t address);
UINT32 program_read_dword_masked_16le(offs_t address, UINT32 mask);
UINT64 program_read_qword_16le(offs_t address);
UINT64 program_read_qword_masked_16le(offs_t address, UINT64 mask);
void program_write_byte_16le(offs_t address, UINT8 data);
void program_write_word_16le(offs_t address, UINT16 data);
void program_write_word_masked_16le(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_16le(offs_t address, UINT32 data);
void program_write_dword_masked_16le(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_16le(offs_t address, UINT64 data);
void program_write_qword_masked_16le(offs_t address, UINT64 data, UINT64 mask);

UINT8 program_read_byte_16be(offs_t address);
UINT16 program_read_word_16be(offs_t address);
UINT16 program_read_word_masked_16be(offs_t address, UINT16 mask);
UINT32 program_read_dword_16be(offs_t address);
UINT32 program_read_dword_masked_16be(offs_t address, UINT32 mask);
UINT64 program_read_qword_16be(offs_t address);
UINT64 program_read_qword_masked_16be(offs_t address, UINT64 mask);
void program_write_byte_16be(offs_t address, UINT8 data);
void program_write_word_16be(offs_t address, UINT16 data);
void program_write_word_masked_16be(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_16be(offs_t address, UINT32 data);
void program_write_dword_masked_16be(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_16be(offs_t address, UINT64 data);
void program_write_qword_masked_16be(offs_t address, UINT64 data, UINT64 mask);

UINT8 program_read_byte_32le(offs_t address);
UINT16 program_read_word_32le(offs_t address);
UINT16 program_read_word_masked_32le(offs_t address, UINT16 mask);
UINT32 program_read_dword_32le(offs_t address);
UINT32 program_read_dword_masked_32le(offs_t address, UINT32 mask);
UINT64 program_read_qword_32le(offs_t address);
UINT64 program_read_qword_masked_32le(offs_t address, UINT64 mask);
void program_write_byte_32le(offs_t address, UINT8 data);
void program_write_word_32le(offs_t address, UINT16 data);
void program_write_word_masked_32le(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_32le(offs_t address, UINT32 data);
void program_write_dword_masked_32le(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_32le(offs_t address, UINT64 data);
void program_write_qword_masked_32le(offs_t address, UINT64 data, UINT64 mask);

UINT8 program_read_byte_32be(offs_t address);
UINT16 program_read_word_32be(offs_t address);
UINT16 program_read_word_masked_32be(offs_t address, UINT16 mask);
UINT32 program_read_dword_32be(offs_t address);
UINT32 program_read_dword_masked_32be(offs_t address, UINT32 mask);
UINT64 program_read_qword_32be(offs_t address);
UINT64 program_read_qword_masked_32be(offs_t address, UINT64 mask);
void program_write_byte_32be(offs_t address, UINT8 data);
void program_write_word_32be(offs_t address, UINT16 data);
void program_write_word_masked_32be(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_32be(offs_t address, UINT32 data);
void program_write_dword_masked_32be(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_32be(offs_t address, UINT64 data);
void program_write_qword_masked_32be(offs_t address, UINT64 data, UINT64 mask);

UINT8 program_read_byte_64le(offs_t address);
UINT16 program_read_word_64le(offs_t address);
UINT16 program_read_word_masked_64le(offs_t address, UINT16 mask);
UINT32 program_read_dword_64le(offs_t address);
UINT32 program_read_dword_masked_64le(offs_t address, UINT32 mask);
UINT64 program_read_qword_64le(offs_t address);
UINT64 program_read_qword_masked_64le(offs_t address, UINT64 mask);
void program_write_byte_64le(offs_t address, UINT8 data);
void program_write_word_64le(offs_t address, UINT16 data);
void program_write_word_masked_64le(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_64le(offs_t address, UINT32 data);
void program_write_dword_masked_64le(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_64le(offs_t address, UINT64 data);
void program_write_qword_masked_64le(offs_t address, UINT64 data, UINT64 mask);

UINT8 program_read_byte_64be(offs_t address);
UINT16 program_read_word_64be(offs_t address);
UINT16 program_read_word_masked_64be(offs_t address, UINT16 mask);
UINT32 program_read_dword_64be(offs_t address);
UINT32 program_read_dword_masked_64be(offs_t address, UINT32 mask);
UINT64 program_read_qword_64be(offs_t address);
UINT64 program_read_qword_masked_64be(offs_t address, UINT64 mask);
void program_write_byte_64be(offs_t address, UINT8 data);
void program_write_word_64be(offs_t address, UINT16 data);
void program_write_word_masked_64be(offs_t address, UINT16 data, UINT16 mask);
void program_write_dword_64be(offs_t address, UINT32 data);
void program_write_dword_masked_64be(offs_t address, UINT32 data, UINT32 mask);
void program_write_qword_64be(offs_t address, UINT64 data);
void program_write_qword_masked_64be(offs_t address, UINT64 data, UINT64 mask);


/* declare data address space handlers */
UINT8 data_read_byte_8le(offs_t address);
UINT16 data_read_word_8le(offs_t address);
UINT16 data_read_word_masked_8le(offs_t address, UINT16 mask);
UINT32 data_read_dword_8le(offs_t address);
UINT32 data_read_dword_masked_8le(offs_t address, UINT32 mask);
UINT64 data_read_qword_8le(offs_t address);
UINT64 data_read_qword_masked_8le(offs_t address, UINT64 mask);
void data_write_byte_8le(offs_t address, UINT8 data);
void data_write_word_8le(offs_t address, UINT16 data);
void data_write_word_masked_8le(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_8le(offs_t address, UINT32 data);
void data_write_dword_masked_8le(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_8le(offs_t address, UINT64 data);
void data_write_qword_masked_8le(offs_t address, UINT64 data, UINT64 mask);

UINT8 data_read_byte_8be(offs_t address);
UINT16 data_read_word_8be(offs_t address);
UINT16 data_read_word_masked_8be(offs_t address, UINT16 mask);
UINT32 data_read_dword_8be(offs_t address);
UINT32 data_read_dword_masked_8be(offs_t address, UINT32 mask);
UINT64 data_read_qword_8be(offs_t address);
UINT64 data_read_qword_masked_8be(offs_t address, UINT64 mask);
void data_write_byte_8be(offs_t address, UINT8 data);
void data_write_word_8be(offs_t address, UINT16 data);
void data_write_word_masked_8be(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_8be(offs_t address, UINT32 data);
void data_write_dword_masked_8be(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_8be(offs_t address, UINT64 data);
void data_write_qword_masked_8be(offs_t address, UINT64 data, UINT64 mask);

UINT8 data_read_byte_16le(offs_t address);
UINT16 data_read_word_16le(offs_t address);
UINT16 data_read_word_masked_16le(offs_t address, UINT16 mask);
UINT32 data_read_dword_16le(offs_t address);
UINT32 data_read_dword_masked_16le(offs_t address, UINT32 mask);
UINT64 data_read_qword_16le(offs_t address);
UINT64 data_read_qword_masked_16le(offs_t address, UINT64 mask);
void data_write_byte_16le(offs_t address, UINT8 data);
void data_write_word_16le(offs_t address, UINT16 data);
void data_write_word_masked_16le(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_16le(offs_t address, UINT32 data);
void data_write_dword_masked_16le(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_16le(offs_t address, UINT64 data);
void data_write_qword_masked_16le(offs_t address, UINT64 data, UINT64 mask);

UINT8 data_read_byte_16be(offs_t address);
UINT16 data_read_word_16be(offs_t address);
UINT16 data_read_word_masked_16be(offs_t address, UINT16 mask);
UINT32 data_read_dword_16be(offs_t address);
UINT32 data_read_dword_masked_16be(offs_t address, UINT32 mask);
UINT64 data_read_qword_16be(offs_t address);
UINT64 data_read_qword_masked_16be(offs_t address, UINT64 mask);
void data_write_byte_16be(offs_t address, UINT8 data);
void data_write_word_16be(offs_t address, UINT16 data);
void data_write_word_masked_16be(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_16be(offs_t address, UINT32 data);
void data_write_dword_masked_16be(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_16be(offs_t address, UINT64 data);
void data_write_qword_masked_16be(offs_t address, UINT64 data, UINT64 mask);

UINT8 data_read_byte_32le(offs_t address);
UINT16 data_read_word_32le(offs_t address);
UINT16 data_read_word_masked_32le(offs_t address, UINT16 mask);
UINT32 data_read_dword_32le(offs_t address);
UINT32 data_read_dword_masked_32le(offs_t address, UINT32 mask);
UINT64 data_read_qword_32le(offs_t address);
UINT64 data_read_qword_masked_32le(offs_t address, UINT64 mask);
void data_write_byte_32le(offs_t address, UINT8 data);
void data_write_word_32le(offs_t address, UINT16 data);
void data_write_word_masked_32le(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_32le(offs_t address, UINT32 data);
void data_write_dword_masked_32le(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_32le(offs_t address, UINT64 data);
void data_write_qword_masked_32le(offs_t address, UINT64 data, UINT64 mask);

UINT8 data_read_byte_32be(offs_t address);
UINT16 data_read_word_32be(offs_t address);
UINT16 data_read_word_masked_32be(offs_t address, UINT16 mask);
UINT32 data_read_dword_32be(offs_t address);
UINT32 data_read_dword_masked_32be(offs_t address, UINT32 mask);
UINT64 data_read_qword_32be(offs_t address);
UINT64 data_read_qword_masked_32be(offs_t address, UINT64 mask);
void data_write_byte_32be(offs_t address, UINT8 data);
void data_write_word_32be(offs_t address, UINT16 data);
void data_write_word_masked_32be(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_32be(offs_t address, UINT32 data);
void data_write_dword_masked_32be(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_32be(offs_t address, UINT64 data);
void data_write_qword_masked_32be(offs_t address, UINT64 data, UINT64 mask);

UINT8 data_read_byte_64le(offs_t address);
UINT16 data_read_word_64le(offs_t address);
UINT16 data_read_word_masked_64le(offs_t address, UINT16 mask);
UINT32 data_read_dword_64le(offs_t address);
UINT32 data_read_dword_masked_64le(offs_t address, UINT32 mask);
UINT64 data_read_qword_64le(offs_t address);
UINT64 data_read_qword_masked_64le(offs_t address, UINT64 mask);
void data_write_byte_64le(offs_t address, UINT8 data);
void data_write_word_64le(offs_t address, UINT16 data);
void data_write_word_masked_64le(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_64le(offs_t address, UINT32 data);
void data_write_dword_masked_64le(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_64le(offs_t address, UINT64 data);
void data_write_qword_masked_64le(offs_t address, UINT64 data, UINT64 mask);

UINT8 data_read_byte_64be(offs_t address);
UINT16 data_read_word_64be(offs_t address);
UINT16 data_read_word_masked_64be(offs_t address, UINT16 mask);
UINT32 data_read_dword_64be(offs_t address);
UINT32 data_read_dword_masked_64be(offs_t address, UINT32 mask);
UINT64 data_read_qword_64be(offs_t address);
UINT64 data_read_qword_masked_64be(offs_t address, UINT64 mask);
void data_write_byte_64be(offs_t address, UINT8 data);
void data_write_word_64be(offs_t address, UINT16 data);
void data_write_word_masked_64be(offs_t address, UINT16 data, UINT16 mask);
void data_write_dword_64be(offs_t address, UINT32 data);
void data_write_dword_masked_64be(offs_t address, UINT32 data, UINT32 mask);
void data_write_qword_64be(offs_t address, UINT64 data);
void data_write_qword_masked_64be(offs_t address, UINT64 data, UINT64 mask);


/* declare io address space handlers */
UINT8 io_read_byte_8le(offs_t address);
UINT16 io_read_word_8le(offs_t address);
UINT16 io_read_word_masked_8le(offs_t address, UINT16 mask);
UINT32 io_read_dword_8le(offs_t address);
UINT32 io_read_dword_masked_8le(offs_t address, UINT32 mask);
UINT64 io_read_qword_8le(offs_t address);
UINT64 io_read_qword_masked_8le(offs_t address, UINT64 mask);
void io_write_byte_8le(offs_t address, UINT8 data);
void io_write_word_8le(offs_t address, UINT16 data);
void io_write_word_masked_8le(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_8le(offs_t address, UINT32 data);
void io_write_dword_masked_8le(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_8le(offs_t address, UINT64 data);
void io_write_qword_masked_8le(offs_t address, UINT64 data, UINT64 mask);

UINT8 io_read_byte_8be(offs_t address);
UINT16 io_read_word_8be(offs_t address);
UINT16 io_read_word_masked_8be(offs_t address, UINT16 mask);
UINT32 io_read_dword_8be(offs_t address);
UINT32 io_read_dword_masked_8be(offs_t address, UINT32 mask);
UINT64 io_read_qword_8be(offs_t address);
UINT64 io_read_qword_masked_8be(offs_t address, UINT64 mask);
void io_write_byte_8be(offs_t address, UINT8 data);
void io_write_word_8be(offs_t address, UINT16 data);
void io_write_word_masked_8be(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_8be(offs_t address, UINT32 data);
void io_write_dword_masked_8be(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_8be(offs_t address, UINT64 data);
void io_write_qword_masked_8be(offs_t address, UINT64 data, UINT64 mask);

UINT8 io_read_byte_16le(offs_t address);
UINT16 io_read_word_16le(offs_t address);
UINT16 io_read_word_masked_16le(offs_t address, UINT16 mask);
UINT32 io_read_dword_16le(offs_t address);
UINT32 io_read_dword_masked_16le(offs_t address, UINT32 mask);
UINT64 io_read_qword_16le(offs_t address);
UINT64 io_read_qword_masked_16le(offs_t address, UINT64 mask);
void io_write_byte_16le(offs_t address, UINT8 data);
void io_write_word_16le(offs_t address, UINT16 data);
void io_write_word_masked_16le(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_16le(offs_t address, UINT32 data);
void io_write_dword_masked_16le(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_16le(offs_t address, UINT64 data);
void io_write_qword_masked_16le(offs_t address, UINT64 data, UINT64 mask);

UINT8 io_read_byte_16be(offs_t address);
UINT16 io_read_word_16be(offs_t address);
UINT16 io_read_word_masked_16be(offs_t address, UINT16 mask);
UINT32 io_read_dword_16be(offs_t address);
UINT32 io_read_dword_masked_16be(offs_t address, UINT32 mask);
UINT64 io_read_qword_16be(offs_t address);
UINT64 io_read_qword_masked_16be(offs_t address, UINT64 mask);
void io_write_byte_16be(offs_t address, UINT8 data);
void io_write_word_16be(offs_t address, UINT16 data);
void io_write_word_masked_16be(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_16be(offs_t address, UINT32 data);
void io_write_dword_masked_16be(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_16be(offs_t address, UINT64 data);
void io_write_qword_masked_16be(offs_t address, UINT64 data, UINT64 mask);

UINT8 io_read_byte_32le(offs_t address);
UINT16 io_read_word_32le(offs_t address);
UINT16 io_read_word_masked_32le(offs_t address, UINT16 mask);
UINT32 io_read_dword_32le(offs_t address);
UINT32 io_read_dword_masked_32le(offs_t address, UINT32 mask);
UINT64 io_read_qword_32le(offs_t address);
UINT64 io_read_qword_masked_32le(offs_t address, UINT64 mask);
void io_write_byte_32le(offs_t address, UINT8 data);
void io_write_word_32le(offs_t address, UINT16 data);
void io_write_word_masked_32le(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_32le(offs_t address, UINT32 data);
void io_write_dword_masked_32le(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_32le(offs_t address, UINT64 data);
void io_write_qword_masked_32le(offs_t address, UINT64 data, UINT64 mask);

UINT8 io_read_byte_32be(offs_t address);
UINT16 io_read_word_32be(offs_t address);
UINT16 io_read_word_masked_32be(offs_t address, UINT16 mask);
UINT32 io_read_dword_32be(offs_t address);
UINT32 io_read_dword_masked_32be(offs_t address, UINT32 mask);
UINT64 io_read_qword_32be(offs_t address);
UINT64 io_read_qword_masked_32be(offs_t address, UINT64 mask);
void io_write_byte_32be(offs_t address, UINT8 data);
void io_write_word_32be(offs_t address, UINT16 data);
void io_write_word_masked_32be(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_32be(offs_t address, UINT32 data);
void io_write_dword_masked_32be(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_32be(offs_t address, UINT64 data);
void io_write_qword_masked_32be(offs_t address, UINT64 data, UINT64 mask);

UINT8 io_read_byte_64le(offs_t address);
UINT16 io_read_word_64le(offs_t address);
UINT16 io_read_word_masked_64le(offs_t address, UINT16 mask);
UINT32 io_read_dword_64le(offs_t address);
UINT32 io_read_dword_masked_64le(offs_t address, UINT32 mask);
UINT64 io_read_qword_64le(offs_t address);
UINT64 io_read_qword_masked_64le(offs_t address, UINT64 mask);
void io_write_byte_64le(offs_t address, UINT8 data);
void io_write_word_64le(offs_t address, UINT16 data);
void io_write_word_masked_64le(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_64le(offs_t address, UINT32 data);
void io_write_dword_masked_64le(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_64le(offs_t address, UINT64 data);
void io_write_qword_masked_64le(offs_t address, UINT64 data, UINT64 mask);

UINT8 io_read_byte_64be(offs_t address);
UINT16 io_read_word_64be(offs_t address);
UINT16 io_read_word_masked_64be(offs_t address, UINT16 mask);
UINT32 io_read_dword_64be(offs_t address);
UINT32 io_read_dword_masked_64be(offs_t address, UINT32 mask);
UINT64 io_read_qword_64be(offs_t address);
UINT64 io_read_qword_masked_64be(offs_t address, UINT64 mask);
void io_write_byte_64be(offs_t address, UINT8 data);
void io_write_word_64be(offs_t address, UINT16 data);
void io_write_word_masked_64be(offs_t address, UINT16 data, UINT16 mask);
void io_write_dword_64be(offs_t address, UINT32 data);
void io_write_dword_masked_64be(offs_t address, UINT32 data, UINT32 mask);
void io_write_qword_64be(offs_t address, UINT64 data);
void io_write_qword_masked_64be(offs_t address, UINT64 data, UINT64 mask);


#endif	/* __MEMORY_H__ */
