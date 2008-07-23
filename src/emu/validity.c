/***************************************************************************

    validity.c

    Validity checks on internal data structures.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "eminline.h"
#include "driver.h"
#include "hash.h"
#include <ctype.h>
#include <stdarg.h>
#include "unicode.h"
#include <zlib.h>


/***************************************************************************
    DEBUGGING
***************************************************************************/

#define REPORT_TIMES		(0)



/***************************************************************************
    COMPILE-TIME VALIDATION
***************************************************************************/

/* if the following lines error during compile, your PTR64 switch is set incorrectly in the makefile */
#ifdef PTR64
UINT8 your_ptr64_flag_is_wrong[(int)(sizeof(void *) - 7)];
#else
UINT8 your_ptr64_flag_is_wrong[(int)(5 - sizeof(void *))];
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define QUARK_HASH_SIZE		389



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _quark_entry quark_entry;
struct _quark_entry
{
	UINT32 crc;
	struct _quark_entry *next;
};


typedef struct _quark_table quark_table;
struct _quark_table
{
	UINT32 entries;
	UINT32 hashsize;
	quark_entry *entry;
	quark_entry **hash;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static quark_table *source_table;
static quark_table *name_table;
static quark_table *description_table;
static quark_table *roms_table;
static quark_table *inputs_table;
static quark_table *defstr_table;



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    input_port_string_from_index - return an
    indexed string from the input port system
-------------------------------------------------*/

INLINE const char *input_port_string_from_index(UINT32 index)
{
	input_port_token token;
	token.i = index;
	return input_port_string_from_token(token);
}


/*-------------------------------------------------
    quark_string_crc - compute the CRC of a string
-------------------------------------------------*/

INLINE UINT32 quark_string_crc(const char *string)
{
	return crc32(0, (UINT8 *)string, (UINT32)strlen(string));
}


/*-------------------------------------------------
    quark_add - add a quark to the table and
    connect it to the hash tables
-------------------------------------------------*/

INLINE void quark_add(quark_table *table, int index, UINT32 crc)
{
	quark_entry *entry = &table->entry[index];
	int hash = crc % table->hashsize;
	entry->crc = crc;
	entry->next = table->hash[hash];
	table->hash[hash] = entry;
}


/*-------------------------------------------------
    quark_table_get_first - return a pointer to the
    first hash entry connected to a CRC
-------------------------------------------------*/

INLINE quark_entry *quark_table_get_first(quark_table *table, UINT32 crc)
{
	return table->hash[crc % table->hashsize];
}



/***************************************************************************
    QUARK TABLES
***************************************************************************/

/*-------------------------------------------------
    quark_table_alloc - allocate an array of
    quark entries and a hash table
-------------------------------------------------*/

static quark_table *quark_table_alloc(UINT32 entries, UINT32 hashsize)
{
	quark_table *table;
	UINT32 total_bytes;

	/* determine how many total bytes we need */
	total_bytes = sizeof(*table) + entries * sizeof(table->entry[0]) + hashsize * sizeof(table->hash[0]);
	table = auto_malloc(total_bytes);

	/* fill in the details */
	table->entries = entries;
	table->hashsize = hashsize;

	/* compute the pointers */
	table->entry = (quark_entry *)((UINT8 *)table + sizeof(*table));
	table->hash = (quark_entry **)((UINT8 *)table->entry + entries * sizeof(table->entry[0]));

	/* reset the hash table */
	memset(table->hash, 0, hashsize * sizeof(table->hash[0]));
	return table;
}


/*-------------------------------------------------
    quark_tables_create - build "quarks" for fast
    string operations
-------------------------------------------------*/

static void quark_tables_create(void)
{
	int drivnum = driver_list_get_count(drivers), strnum;

	/* allocate memory for the quark tables */
	source_table = quark_table_alloc(drivnum, QUARK_HASH_SIZE);
	name_table = quark_table_alloc(drivnum, QUARK_HASH_SIZE);
	description_table = quark_table_alloc(drivnum, QUARK_HASH_SIZE);
	roms_table = quark_table_alloc(drivnum, QUARK_HASH_SIZE);
	inputs_table = quark_table_alloc(drivnum, QUARK_HASH_SIZE);

	/* build the quarks and the hash tables */
	for (drivnum = 0; drivers[drivnum]; drivnum++)
	{
		const game_driver *driver = drivers[drivnum];

		/* fill in the quark with hashes of the strings */
		quark_add(source_table,      drivnum, quark_string_crc(driver->source_file));
		quark_add(name_table,        drivnum, quark_string_crc(driver->name));
		quark_add(description_table, drivnum, quark_string_crc(driver->description));

		/* only track actual driver ROM entries */
		if (driver->rom && (driver->flags & GAME_NO_STANDALONE) == 0)
			quark_add(roms_table,    drivnum, (FPTR)driver->rom);
	}

	/* allocate memory for a quark table of strings */
	defstr_table = quark_table_alloc(INPUT_STRING_COUNT, 97);

	/* add all the default strings */
	for (strnum = 1; strnum < INPUT_STRING_COUNT; strnum++)
	{
		const char *string = input_port_string_from_index(strnum);
		if (string != NULL)
			quark_add(defstr_table, strnum, quark_string_crc(string));
	}
}



/***************************************************************************
    VALIDATION FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    validate_inlines - validate inline function
    behaviors
-------------------------------------------------*/

static int validate_inlines(void)
{
#undef rand
	volatile UINT64 testu64a = rand() ^ (rand() << 15) ^ ((UINT64)rand() << 30) ^ ((UINT64)rand() << 45);
	volatile INT64 testi64a = rand() ^ (rand() << 15) ^ ((INT64)rand() << 30) ^ ((INT64)rand() << 45);
#ifdef PTR64
	volatile INT64 testi64b = rand() ^ (rand() << 15) ^ ((INT64)rand() << 30) ^ ((INT64)rand() << 45);
#endif
	volatile UINT32 testu32a = rand() ^ (rand() << 15);
	volatile UINT32 testu32b = rand() ^ (rand() << 15);
	volatile INT32 testi32a = rand() ^ (rand() << 15);
	volatile INT32 testi32b = rand() ^ (rand() << 15);
	INT32 resulti32, expectedi32;
	UINT32 resultu32, expectedu32;
	INT64 resulti64, expectedi64;
	UINT64 resultu64, expectedu64;
	INT32 remainder, expremainder;
	UINT32 uremainder, expuremainder, bigu32 = 0xffffffff;
	int error = FALSE;

	/* use only non-zero, positive numbers */
	if (testu64a == 0) testu64a++;
	if (testi64a == 0) testi64a++;
	else if (testi64a < 0) testi64a = -testi64a;
#ifdef PTR64
	if (testi64b == 0) testi64b++;
	else if (testi64b < 0) testi64b = -testi64b;
#endif
	if (testu32a == 0) testu32a++;
	if (testu32b == 0) testu32b++;
	if (testi32a == 0) testi32a++;
	else if (testi32a < 0) testi32a = -testi32a;
	if (testi32b == 0) testi32b++;
	else if (testi32b < 0) testi32b = -testi32b;

	resulti64 = mul_32x32(testi32a, testi32b);
	expectedi64 = (INT64)testi32a * (INT64)testi32b;
	if (resulti64 != expectedi64)
		{ mame_printf_error("Error testing mul_32x32 (%08X x %08X) = %08X%08X (expected %08X%08X)\n", testi32a, testi32b, (UINT32)(resulti64 >> 32), (UINT32)resulti64, (UINT32)(expectedi64 >> 32), (UINT32)expectedi64); error = TRUE; }

	resultu64 = mulu_32x32(testu32a, testu32b);
	expectedu64 = (UINT64)testu32a * (UINT64)testu32b;
	if (resultu64 != expectedu64)
		{ mame_printf_error("Error testing mulu_32x32 (%08X x %08X) = %08X%08X (expected %08X%08X)\n", testu32a, testu32b, (UINT32)(resultu64 >> 32), (UINT32)resultu64, (UINT32)(expectedu64 >> 32), (UINT32)expectedu64); error = TRUE; }

	resulti32 = mul_32x32_hi(testi32a, testi32b);
	expectedi32 = ((INT64)testi32a * (INT64)testi32b) >> 32;
	if (resulti32 != expectedi32)
		{ mame_printf_error("Error testing mul_32x32_hi (%08X x %08X) = %08X (expected %08X)\n", testi32a, testi32b, resulti32, expectedi32); error = TRUE; }

	resultu32 = mulu_32x32_hi(testu32a, testu32b);
	expectedu32 = ((INT64)testu32a * (INT64)testu32b) >> 32;
	if (resultu32 != expectedu32)
		{ mame_printf_error("Error testing mulu_32x32_hi (%08X x %08X) = %08X (expected %08X)\n", testu32a, testu32b, resultu32, expectedu32); error = TRUE; }

	resulti32 = mul_32x32_shift(testi32a, testi32b, 7);
	expectedi32 = ((INT64)testi32a * (INT64)testi32b) >> 7;
	if (resulti32 != expectedi32)
		{ mame_printf_error("Error testing mul_32x32_shift (%08X x %08X) >> 7 = %08X (expected %08X)\n", testi32a, testi32b, resulti32, expectedi32); error = TRUE; }

	resultu32 = mulu_32x32_shift(testu32a, testu32b, 7);
	expectedu32 = ((INT64)testu32a * (INT64)testu32b) >> 7;
	if (resultu32 != expectedu32)
		{ mame_printf_error("Error testing mulu_32x32_shift (%08X x %08X) >> 7 = %08X (expected %08X)\n", testu32a, testu32b, resultu32, expectedu32); error = TRUE; }

	while ((INT64)testi32a * (INT64)0x7fffffff < testi64a)
		testi64a /= 2;
	while ((UINT64)testu32a * (UINT64)bigu32 < testu64a)
		testu64a /= 2;

	resulti32 = div_64x32(testi64a, testi32a);
	expectedi32 = testi64a / (INT64)testi32a;
	if (resulti32 != expectedi32)
		{ mame_printf_error("Error testing div_64x32 (%08X%08X / %08X) = %08X (expected %08X)\n", (UINT32)(testi64a >> 32), (UINT32)testi64a, testi32a, resulti32, expectedi32); error = TRUE; }

	resultu32 = divu_64x32(testu64a, testu32a);
	expectedu32 = testu64a / (UINT64)testu32a;
	if (resultu32 != expectedu32)
		{ mame_printf_error("Error testing divu_64x32 (%08X%08X / %08X) = %08X (expected %08X)\n", (UINT32)(testu64a >> 32), (UINT32)testu64a, testu32a, resultu32, expectedu32); error = TRUE; }

	resulti32 = div_64x32_rem(testi64a, testi32a, &remainder);
	expectedi32 = testi64a / (INT64)testi32a;
	expremainder = testi64a % (INT64)testi32a;
	if (resulti32 != expectedi32 || remainder != expremainder)
		{ mame_printf_error("Error testing div_64x32_rem (%08X%08X / %08X) = %08X,%08X (expected %08X,%08X)\n", (UINT32)(testi64a >> 32), (UINT32)testi64a, testi32a, resulti32, remainder, expectedi32, expremainder); error = TRUE; }

	resultu32 = divu_64x32_rem(testu64a, testu32a, &uremainder);
	expectedu32 = testu64a / (UINT64)testu32a;
	expuremainder = testu64a % (UINT64)testu32a;
	if (resultu32 != expectedu32 || uremainder != expuremainder)
		{ mame_printf_error("Error testing divu_64x32_rem (%08X%08X / %08X) = %08X,%08X (expected %08X,%08X)\n", (UINT32)(testu64a >> 32), (UINT32)testu64a, testu32a, resultu32, uremainder, expectedu32, expuremainder); error = TRUE; }

	resulti32 = mod_64x32(testi64a, testi32a);
	expectedi32 = testi64a % (INT64)testi32a;
	if (resulti32 != expectedi32)
		{ mame_printf_error("Error testing mod_64x32 (%08X%08X / %08X) = %08X (expected %08X)\n", (UINT32)(testi64a >> 32), (UINT32)testi64a, testi32a, resulti32, expectedi32); error = TRUE; }

	resultu32 = modu_64x32(testu64a, testu32a);
	expectedu32 = testu64a % (UINT64)testu32a;
	if (resultu32 != expectedu32)
		{ mame_printf_error("Error testing modu_64x32 (%08X%08X / %08X) = %08X (expected %08X)\n", (UINT32)(testu64a >> 32), (UINT32)testu64a, testu32a, resultu32, expectedu32); error = TRUE; }

	while ((INT64)testi32a * (INT64)0x7fffffff < ((INT32)testi64a << 3))
		testi64a /= 2;
	while ((UINT64)testu32a * (UINT64)0xffffffff < ((UINT32)testu64a << 3))
		testu64a /= 2;

	resulti32 = div_32x32_shift((INT32)testi64a, testi32a, 3);
	expectedi32 = ((INT64)(INT32)testi64a << 3) / (INT64)testi32a;
	if (resulti32 != expectedi32)
		{ mame_printf_error("Error testing div_32x32_shift (%08X << 3) / %08X = %08X (expected %08X)\n", (INT32)testi64a, testi32a, resulti32, expectedi32); error = TRUE; }

	resultu32 = divu_32x32_shift((UINT32)testu64a, testu32a, 3);
	expectedu32 = ((UINT64)(UINT32)testu64a << 3) / (UINT64)testu32a;
	if (resultu32 != expectedu32)
		{ mame_printf_error("Error testing divu_32x32_shift (%08X << 3) / %08X = %08X (expected %08X)\n", (UINT32)testu64a, testu32a, resultu32, expectedu32); error = TRUE; }

	if (fabs(recip_approx(100.0) - 0.01) > 0.0001)
		{ mame_printf_error("Error testing recip_approx\n"); error = TRUE; }

	testi32a = (testi32a & 0x0000ffff) | 0x400000;
	if (count_leading_zeros(testi32a) != 9)
		{ mame_printf_error("Error testing count_leading_zeros\n"); error = TRUE; }
	testi32a = (testi32a | 0xffff0000) & ~0x400000;
	if (count_leading_ones(testi32a) != 9)
		{ mame_printf_error("Error testing count_leading_ones\n"); error = TRUE; }

	testi32b = testi32a;
	if (compare_exchange32(&testi32a, testi32b, 1000) != testi32b || testi32a != 1000)
		{ mame_printf_error("Error testing compare_exchange32\n"); error = TRUE; }
#ifdef PTR64
	testi64b = testi64a;
	if (compare_exchange64(&testi64a, testi64b, 1000) != testi64b || testi64a != 1000)
		{ mame_printf_error("Error testing compare_exchange64\n"); error = TRUE; }
#endif
	if (atomic_exchange32(&testi32a, testi32b) != 1000)
		{ mame_printf_error("Error testing atomic_exchange32\n"); error = TRUE; }
	if (atomic_add32(&testi32a, 45) != testi32b + 45)
		{ mame_printf_error("Error testing atomic_add32\n"); error = TRUE; }
	if (atomic_increment32(&testi32a) != testi32b + 46)
		{ mame_printf_error("Error testing atomic_increment32\n"); error = TRUE; }
	if (atomic_decrement32(&testi32a) != testi32b + 45)
		{ mame_printf_error("Error testing atomic_decrement32\n"); error = TRUE; }

	return error;
}


/*-------------------------------------------------
    validate_driver - validate basic driver
    information
-------------------------------------------------*/

static int validate_driver(int drivnum, const machine_config *config)
{
	const game_driver *driver = drivers[drivnum];
	const game_driver *clone_of;
	quark_entry *entry;
	int error = FALSE;
	const char *s;
	UINT32 crc;

	/* determine the clone */
	clone_of = driver_get_clone(driver);

	/* if we have at least 100 drivers, validate the clone */
	/* (100 is arbitrary, but tries to avoid tiny.mak dependencies) */
	if (driver_list_get_count(drivers) > 100 && !clone_of && strcmp(driver->parent, "0"))
	{
		mame_printf_error("%s: %s is a non-existant clone\n", driver->source_file, driver->parent);
		error = TRUE;
	}

	/* look for recursive cloning */
	if (clone_of == driver)
	{
		mame_printf_error("%s: %s is set as a clone of itself\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* look for clones that are too deep */
	if (clone_of != NULL && (clone_of = driver_get_clone(clone_of)) != NULL && (clone_of->flags & GAME_IS_BIOS_ROOT) == 0)
	{
		mame_printf_error("%s: %s is a clone of a clone\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* make sure the driver name is 8 chars or less */
	if (strlen(driver->name) > 8)
	{
		mame_printf_error("%s: %s driver name must be 8 characters or less\n", driver->source_file, driver->name);
		error = TRUE;
	}

	/* make sure the year is only digits, '?' or '+' */
	for (s = driver->year; *s; s++)
		if (!isdigit(*s) && *s != '?' && *s != '+')
		{
			mame_printf_error("%s: %s has an invalid year '%s'\n", driver->source_file, driver->name, driver->year);
			error = TRUE;
			break;
		}

#ifndef MESS
	/* make sure sound-less drivers are flagged */
	if ((driver->flags & GAME_IS_BIOS_ROOT) == 0 && config->sound[0].type == SOUND_DUMMY && (driver->flags & GAME_NO_SOUND) == 0 && strcmp(driver->name, "minivadr"))
	{
		mame_printf_error("%s: %s missing GAME_NO_SOUND flag\n", driver->source_file, driver->name);
		error = TRUE;
	}
#endif

	/* find duplicate driver names */
	crc = quark_string_crc(driver->name);
	for (entry = quark_table_get_first(name_table, crc); entry; entry = entry->next)
		if (entry->crc == crc && entry != &name_table->entry[drivnum])
		{
			const game_driver *match = drivers[entry - name_table->entry];
			if (!strcmp(match->name, driver->name))
			{
				mame_printf_error("%s: %s is a duplicate name (%s, %s)\n", driver->source_file, driver->name, match->source_file, match->name);
				error = TRUE;
			}
		}

	/* find duplicate descriptions */
	crc = quark_string_crc(driver->description);
	for (entry = quark_table_get_first(description_table, crc); entry; entry = entry->next)
		if (entry->crc == crc && entry != &description_table->entry[drivnum])
		{
			const game_driver *match = drivers[entry - description_table->entry];
			if (!strcmp(match->description, driver->description))
			{
				mame_printf_error("%s: %s is a duplicate description (%s, %s)\n", driver->source_file, driver->description, match->source_file, match->name);
				error = TRUE;
			}
		}

	/* find shared ROM entries */
#ifndef MESS
	if (driver->rom && (driver->flags & GAME_IS_BIOS_ROOT) == 0)
	{
		crc = (FPTR)driver->rom;
		for (entry = quark_table_get_first(roms_table, crc); entry; entry = entry->next)
			if (entry->crc == crc && entry != &roms_table->entry[drivnum])
			{
				const game_driver *match = drivers[entry - roms_table->entry];
				if (match->rom == driver->rom)
				{
					mame_printf_error("%s: %s uses the same ROM set as (%s, %s)\n", driver->source_file, driver->description, match->source_file, match->name);
					error = TRUE;
				}
			}
	}
#endif /* MESS */

	return error;
}


/*-------------------------------------------------
    validate_roms - validate ROM definitions
-------------------------------------------------*/

static int validate_roms(int drivnum, const machine_config *config, UINT32 *region_length)
{
	const game_driver *driver = drivers[drivnum];
	const rom_entry *romp;
	const char *last_name = "???";
	int cur_region = -1;
	int error = FALSE;
	int items_since_region = 1;
	int bios_flags = 0, last_bios = 0;

	/* reset region info */
	memset(region_length, 0, REGION_MAX * sizeof(*region_length));

	/* scan the ROM entries */
	for (romp = driver->rom; romp && !ROMENTRY_ISEND(romp); romp++)
	{
		/* if this is a region, make sure it's valid, and record the length */
		if (ROMENTRY_ISREGION(romp))
		{
			int type = ROMREGION_GETTYPE(romp);

			/* if we haven't seen any items since the last region, print a warning */
			if (items_since_region == 0)
				mame_printf_warning("%s: %s has empty ROM region (warning)\n", driver->source_file, driver->name);
			items_since_region = (ROMREGION_ISERASE(romp) || ROMREGION_ISDISPOSE(romp)) ? 1 : 0;

			/* check for an invalid region */
			if (type >= REGION_MAX || type <= REGION_INVALID)
			{
				mame_printf_error("%s: %s has invalid ROM_REGION type %x\n", driver->source_file, driver->name, type);
				error = TRUE;
				cur_region = -1;
			}

			/* check for a duplicate */
			else if (region_length[type] != 0)
			{
				mame_printf_error("%s: %s has duplicate ROM_REGION type %x\n", driver->source_file, driver->name, type);
				error = TRUE;
				cur_region = -1;
			}

			/* if all looks good, remember the length and note the region */
			else
			{
				cur_region = type;
				region_length[type] = ROMREGION_GETLENGTH(romp);
			}
		}

		/* If this is a system bios, make sure it is using the next available bios number */
		else if (ROMENTRY_ISSYSTEM_BIOS(romp))
		{
			bios_flags = ROM_GETBIOSFLAGS(romp);
			if (last_bios+1 != bios_flags)
			{
				const char *name = ROM_GETHASHDATA(romp);
				mame_printf_error("%s: %s has non-sequential bios %s\n", driver->source_file, driver->name, name);
				error = TRUE;
			}
			last_bios = bios_flags;
		}

		/* if this is a file, make sure it is properly formatted */
		else if (ROMENTRY_ISFILE(romp))
		{
			const char *hash;
			const char *s;

			items_since_region++;

			/* track the last filename we found */
			last_name = ROM_GETNAME(romp);

			/* make sure it's all lowercase */
			for (s = last_name; *s; s++)
				if (tolower(*s) != *s)
				{
					mame_printf_error("%s: %s has upper case ROM name %s\n", driver->source_file, driver->name, last_name);
					error = TRUE;
					break;
				}

			/* if this is a bios rom, make sure it has the same flags as the last system bios entry */
			bios_flags = ROM_GETBIOSFLAGS(romp);
			if (bios_flags != 0)
			{
				if (bios_flags != last_bios)
				{
					mame_printf_error("%s: %s has bios rom name %s without preceding matching system bios definition\n", driver->source_file, driver->name, last_name);
					error = TRUE;
				}
			}

			/* make sure the has is valid */
			hash = ROM_GETHASHDATA(romp);
			if (!hash_verify_string(hash))
			{
				mame_printf_error("%s: rom '%s' has an invalid hash string '%s'\n", driver->name, last_name, hash);
				error = TRUE;
			}
		}

		/* for any non-region ending entries, make sure they don't extend past the end */
		if (!ROMENTRY_ISREGIONEND(romp) && cur_region != -1)
		{
			items_since_region++;

			if (ROM_GETOFFSET(romp) + ROM_GETLENGTH(romp) > region_length[cur_region])
			{
				mame_printf_error("%s: %s has ROM %s extending past the defined memory region\n", driver->source_file, driver->name, last_name);
				error = TRUE;
			}
		}
	}

	/* final check for empty regions */
	if (items_since_region == 0)
		mame_printf_warning("%s: %s has empty ROM region (warning)\n", driver->source_file, driver->name);

	return error;
}


/*-------------------------------------------------
    validate_cpu - validate CPUs and memory maps
-------------------------------------------------*/

static int validate_cpu(int drivnum, const machine_config *config, const UINT32 *region_length)
{
	const game_driver *driver = drivers[drivnum];
	int error = FALSE;
	int cpunum;
	cpu_validity_check_func cpu_validity_check;

	/* loop over all the CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		extern void dummy_get_info(UINT32 state, cpuinfo *info);
		const cpu_config *cpu = &config->cpu[cpunum];
		int spacenum, checknum;

		/* skip empty entries */
		if (cpu->type == CPU_DUMMY)
			continue;

		/* check for duplicate tags */
		if (cpu->tag != NULL)
			for (checknum = 0; checknum < cpunum; checknum++)
				if (config->cpu[checknum].tag != NULL && strcmp(cpu->tag, config->cpu[checknum].tag) == 0)
				{
					mame_printf_error("%s: %s has multiple CPUs tagged as '%s'\n", driver->source_file, driver->name, cpu->tag);
					error = TRUE;
				}

		/* checks to see if this driver is using a dummy CPU */
		if (cputype_get_interface(cpu->type)->get_info == dummy_get_info)
		{
			mame_printf_error("%s: %s uses non-present CPU\n", driver->source_file, driver->name);
			error = TRUE;
			continue;
		}

		/* check the CPU for incompleteness */
		if (!cputype_get_info_fct(cpu->type, CPUINFO_PTR_GET_CONTEXT)
			|| !cputype_get_info_fct(cpu->type, CPUINFO_PTR_SET_CONTEXT)
			|| !cputype_get_info_fct(cpu->type, CPUINFO_PTR_RESET)
			|| !cputype_get_info_fct(cpu->type, CPUINFO_PTR_EXECUTE))
		{
			mame_printf_error("%s: %s uses an incomplete CPU\n", driver->source_file, driver->name);
			error = TRUE;
			continue;
		}

		/* check for CPU-specific validity check */
		cpu_validity_check = (cpu_validity_check_func) cputype_get_info_fct(cpu->type, CPUINFO_PTR_VALIDITY_CHECK);
		if (cpu_validity_check != NULL)
		{
			if ((*cpu_validity_check)(driver, config->cpu[cpunum].reset_param))
				error = TRUE;
		}

		/* loop over all address spaces */
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		{
#define SPACE_SHIFT(a)		((addr_shift < 0) ? ((a) << -addr_shift) : ((a) >> addr_shift))
#define SPACE_SHIFT_END(a)	((addr_shift < 0) ? (((a) << -addr_shift) | ((1 << -addr_shift) - 1)) : ((a) >> addr_shift))
			int databus_width = cputype_databus_width(cpu->type, spacenum);
			int addr_shift = cputype_addrbus_shift(cpu->type, spacenum);
			int alignunit = databus_width/8;
			address_map_entry *entry;
			address_map *map;

			/* check to see that the same map is not used twice */
			if (cpu->address_map[spacenum][0] != NULL && cpu->address_map[spacenum][0] == cpu->address_map[spacenum][1])
			{
				mame_printf_error("%s: %s uses identical memory maps for CPU #%d spacenum %d\n", driver->source_file, driver->name, cpunum, spacenum);
				error = TRUE;
			}

			/* construct the maps */
			map = address_map_alloc(config, cpunum, spacenum);

			/* if this is an empty map, just skip it */
			if (map->entrylist == NULL)
			{
				address_map_free(map);
				continue;
			}

			/* validate the global map parameters */
			if (map->spacenum != spacenum)
			{
				mame_printf_error("%s: %s CPU #%d space %d has address space %d handlers!", driver->source_file, driver->name, cpunum, spacenum, map->spacenum);
				error = TRUE;
			}
			if (map->databits != databus_width)
			{
				mame_printf_error("%s: %s cpu #%d uses wrong memory handlers for %s space! (width = %d, memory = %08x)\n", driver->source_file, driver->name, cpunum, address_space_names[spacenum], databus_width, map->databits);
				error = TRUE;
			}

			/* loop over entries and look for errors */
			for (entry = map->entrylist; entry != NULL; entry = entry->next)
			{
				UINT32 bytestart = SPACE_SHIFT(entry->addrstart);
				UINT32 byteend = SPACE_SHIFT_END(entry->addrend);

				/* look for inverted start/end pairs */
				if (byteend < bytestart)
				{
					mame_printf_error("%s: %s wrong %s memory read handler start = %08x > end = %08x\n", driver->source_file, driver->name, address_space_names[spacenum], entry->addrstart, entry->addrend);
					error = TRUE;
				}

				/* look for misaligned entries */
				if ((bytestart & (alignunit - 1)) != 0 || (byteend & (alignunit - 1)) != (alignunit - 1))
				{
					mame_printf_error("%s: %s wrong %s memory read handler start = %08x, end = %08x ALIGN = %d\n", driver->source_file, driver->name, address_space_names[spacenum], entry->addrstart, entry->addrend, alignunit);
					error = TRUE;
				}

				/* if this is a program space, auto-assign implicit ROM entries */
				if ((FPTR)entry->read.generic == STATIC_ROM && !entry->region)
				{
					entry->region = REGION_CPU1 + cpunum;
					entry->region_offs = entry->addrstart;
				}

				/* if this entry references a memory region, validate it */
				if (entry->region && entry->share == 0)
				{
					offs_t length = region_length[entry->region];

					if (length == 0)
					{
						mame_printf_error("%s: %s CPU %d space %d memory map entry %X-%X references non-existant region %d\n", driver->source_file, driver->name, cpunum, spacenum, entry->addrstart, entry->addrend, entry->region);
						error = TRUE;
					}
					else if (entry->region_offs + (byteend - bytestart + 1) > length)
					{
						mame_printf_error("%s: %s CPU %d space %d memory map entry %X-%X extends beyond region %d size (%X)\n", driver->source_file, driver->name, cpunum, spacenum, entry->addrstart, entry->addrend, entry->region, length);
						error = TRUE;
					}
				}

				/* make sure all devices exist */
				if (entry->read_devtype != NULL && device_list_find_by_tag(config->devicelist, entry->read_devtype, entry->read_devtag) == NULL)
				{
					mame_printf_error("%s: %s CPU %d space %d memory map entry references nonexistant device type %s, tag %s\n", driver->source_file, driver->name, cpunum, spacenum, devtype_name(entry->read_devtype), entry->read_devtag);
					error = TRUE;
				}
				if (entry->write_devtype != NULL && device_list_find_by_tag(config->devicelist, entry->write_devtype, entry->write_devtag) == NULL)
				{
					mame_printf_error("%s: %s CPU %d space %d memory map entry references nonexistant device type %s, tag %s\n", driver->source_file, driver->name, cpunum, spacenum, devtype_name(entry->write_devtype), entry->write_devtag);
					error = TRUE;
				}
			}

			/* release the address map */
			address_map_free(map);

			/* validate the interrupts */
			if (cpu->vblank_interrupt != NULL)
			{
				if (video_screen_count(config) == 0)
				{
					mame_printf_error("%s: %s cpu #%d has a VBLANK interrupt, but the driver is screenless !\n", driver->source_file, driver->name, cpunum);
					error = TRUE;
				}
				else if (cpu->vblank_interrupts_per_frame == 0)
				{
					mame_printf_error("%s: %s cpu #%d has a VBLANK interrupt handler with 0 interrupts!\n", driver->source_file, driver->name, cpunum);
					error = TRUE;
				}
				else if (cpu->vblank_interrupts_per_frame == 1)
				{
					if (cpu->vblank_interrupt_screen == NULL)
					{
						mame_printf_error("%s: %s cpu #%d has a valid VBLANK interrupt handler with no screen tag supplied!\n", driver->source_file, driver->name, cpunum);
						error = TRUE;
					}
					else if (device_list_index(config->devicelist, VIDEO_SCREEN, cpu->vblank_interrupt_screen) == -1)
					{
						mame_printf_error("%s: %s cpu #%d VBLANK interrupt with a non-existant screen tag (%s)!\n", driver->source_file, driver->name, cpunum, cpu->vblank_interrupt_screen);
						error = TRUE;
					}
				}
			}
			else if (cpu->vblank_interrupts_per_frame != 0)
			{
				mame_printf_error("%s: %s cpu #%d has no VBLANK interrupt handler but a non-0 interrupt count is given!\n", driver->source_file, driver->name, cpunum);
				error = TRUE;
			}

			if ((cpu->timed_interrupt != NULL) && (cpu->timed_interrupt_period == 0))
			{
				mame_printf_error("%s: %s cpu #%d has a timer interrupt handler with 0 period!\n", driver->source_file, driver->name, cpunum);
				error = TRUE;
			}
			else if ((cpu->timed_interrupt == NULL) && (cpu->timed_interrupt_period != 0))
			{
				mame_printf_error("%s: %s cpu #%d has a no timer interrupt handler but has a non-0 period given!\n", driver->source_file, driver->name, cpunum);
				error = TRUE;
			}
		}
	}

	return error;
}


/*-------------------------------------------------
    validate_display - validate display
    configurations
-------------------------------------------------*/

static int validate_display(int drivnum, const machine_config *config)
{
	const game_driver *driver = drivers[drivnum];
	const device_config *device;
	int palette_modes = FALSE;
	int error = FALSE;

	/* loop over screens */
	for (device = video_screen_first(config); device != NULL; device = video_screen_next(device))
	{
		const screen_config *scrconfig = device->inline_config;

		/* sanity check dimensions */
		if ((scrconfig->width <= 0) || (scrconfig->height <= 0))
		{
			mame_printf_error("%s: %s screen \"%s\" has invalid display dimensions\n", driver->source_file, driver->name, device->tag);
			error = TRUE;
		}

		/* sanity check display area */
		if (scrconfig->type != SCREEN_TYPE_VECTOR)
		{
			if ((scrconfig->visarea.max_x < scrconfig->visarea.min_x) ||
				(scrconfig->visarea.max_y < scrconfig->visarea.min_y) ||
				(scrconfig->visarea.max_x >= scrconfig->width) ||
				(scrconfig->visarea.max_y >= scrconfig->height))
			{
				mame_printf_error("%s: %s screen \"%s\" has an invalid display area\n", driver->source_file, driver->name, device->tag);
				error = TRUE;
			}

			/* sanity check screen formats */
			if (scrconfig->format != BITMAP_FORMAT_INDEXED16 &&
				scrconfig->format != BITMAP_FORMAT_RGB15 &&
				scrconfig->format != BITMAP_FORMAT_RGB32)
			{
				mame_printf_error("%s: %s screen \"%s\" has unsupported format\n", driver->source_file, driver->name, device->tag);
				error = TRUE;
			}
			if (scrconfig->format == BITMAP_FORMAT_INDEXED16)
				palette_modes = TRUE;
		}

		/* check for zero frame rate */
		if (scrconfig->refresh == 0)
		{
			mame_printf_error("%s: %s screen \"%s\" has a zero refresh rate\n", driver->source_file, driver->name, device->tag);
			error = TRUE;
		}
	}

	/* check for empty palette */
	if (palette_modes && config->total_colors == 0)
	{
		mame_printf_error("%s: %s has zero palette entries\n", driver->source_file, driver->name);
		error = TRUE;
	}

	return error;
}


/*-------------------------------------------------
    validate_gfx - validate graphics decoding
    configuration
-------------------------------------------------*/

static int validate_gfx(int drivnum, const machine_config *config, const UINT32 *region_length)
{
	const game_driver *driver = drivers[drivnum];
	int error = FALSE;
	int gfxnum;

	/* bail if no gfx */
	if (!config->gfxdecodeinfo)
		return FALSE;

	/* iterate over graphics decoding entries */
	for (gfxnum = 0; gfxnum < MAX_GFX_ELEMENTS && config->gfxdecodeinfo[gfxnum].memory_region != -1; gfxnum++)
	{
		const gfx_decode_entry *gfx = &config->gfxdecodeinfo[gfxnum];
		int region = gfx->memory_region;
		int xscale = (config->gfxdecodeinfo[gfxnum].xscale == 0) ? 1 : config->gfxdecodeinfo[gfxnum].xscale;
		int yscale = (config->gfxdecodeinfo[gfxnum].yscale == 0) ? 1 : config->gfxdecodeinfo[gfxnum].yscale;
		const gfx_layout *gl = gfx->gfxlayout;
		int israw = (gl->planeoffset[0] == GFX_RAW);
		int planes = gl->planes;
		UINT16 width = gl->width;
		UINT16 height = gl->height;
		UINT32 total = gl->total;

		/* if we have a valid region, and we're not using auto-sizing, check the decode against the region length */
		if (region && !IS_FRAC(total))
		{
			int len, avail, plane, start;
			UINT32 charincrement = gl->charincrement;
			const UINT32 *poffset = gl->planeoffset;

			/* determine which plane is the largest */
			start = 0;
			for (plane = 0; plane < planes; plane++)
				if (poffset[plane] > start)
					start = poffset[plane];
			start &= ~(charincrement - 1);

			/* determine the total length based on this info */
			len = total * charincrement;

			/* do we have enough space in the region to cover the whole decode? */
			avail = region_length[region] - (gfx->start & ~(charincrement/8-1));

			/* if not, this is an error */
			if ((start + len) / 8 > avail)
			{
				mame_printf_error("%s: %s has gfx[%d] extending past allocated memory\n", driver->source_file, driver->name, gfxnum);
				error = TRUE;
			}
		}
		if (israw)
		{
			if (total != RGN_FRAC(1,1))
			{
				mame_printf_error("%s: %s has gfx[%d] with unsupported layout total\n", driver->source_file, driver->name, gfxnum);
				error = TRUE;
			}

			if (xscale != 1 || yscale != 1)
			{
				mame_printf_error("%s: %s has gfx[%d] with unsupported xscale/yscale\n", driver->source_file, driver->name, gfxnum);
				error = TRUE;
			}
		}
		else
		{
			if (planes > MAX_GFX_PLANES)
			{
				mame_printf_error("%s: %s has gfx[%d] with invalid planes\n", driver->source_file, driver->name, gfxnum);
				error = TRUE;
			}

			if (xscale * width > MAX_ABS_GFX_SIZE || yscale * height > MAX_ABS_GFX_SIZE)
			{
				mame_printf_error("%s: %s has gfx[%d] with invalid xscale/yscale\n", driver->source_file, driver->name, gfxnum);
				error = TRUE;
			}
		}
	}

	return error;
}


/*-------------------------------------------------
    get_defstr_index - return the index of the
    string assuming it is one of the default
    strings
-------------------------------------------------*/

static int get_defstr_index(const char *name, const game_driver *driver, int *error)
{
	UINT32 crc = quark_string_crc(name);
	quark_entry *entry;
	int strindex = 0;

	/* scan the quark table of input port strings */
	for (entry = quark_table_get_first(defstr_table, crc); entry != NULL; entry = entry->next)
		if (entry->crc == crc && strcmp(name, input_port_string_from_index(entry - defstr_table->entry)) == 0)
		{
			strindex = entry - defstr_table->entry;
			break;
		}

	/* check for strings that should be DEF_STR */
	if (strindex != 0 && name != input_port_string_from_index(strindex) && error != NULL)
	{
		mame_printf_error("%s: %s must use DEF_STR( %s )\n", driver->source_file, driver->name, name);
		*error = TRUE;
	}

	return strindex;
}


/*-------------------------------------------------
    validate_analog_input_field - validate an
    analog input field
-------------------------------------------------*/

static void validate_analog_input_field(const input_field_config *field, const game_driver *driver, int *error)
{
	INT32 analog_max = field->max;
	INT32 analog_min = field->min;
	int shift;

	if (field->type == IPT_POSITIONAL || field->type == IPT_POSITIONAL_V)
	{
		for (shift = 0; (shift <= 31) && (~field->mask & (1 << shift)); shift++) ;
		/* convert the positional max value to be in the bitmask for testing */
		analog_max = (analog_max - 1) << shift;

		/* positional port size must fit in bits used */
		if (((field->mask >> shift) + 1) < field->max)
		{
			mame_printf_error("%s: %s has an analog port with a positional port size bigger then the mask size\n", driver->source_file, driver->name);
			*error = TRUE;
		}
	}
	else
	{
		/* only positional controls use PORT_WRAPS */
		if (field->flags & ANALOG_FLAG_WRAPS)
		{
			mame_printf_error("%s: %s only positional analog ports use PORT_WRAPS\n", driver->source_file, driver->name);
			*error = TRUE;
		}
	}

	/* analog ports must have a valid sensitivity */
	if (field->sensitivity == 0)
	{
		mame_printf_error("%s: %s has an analog port with zero sensitivity\n", driver->source_file, driver->name);
		*error = TRUE;
	}

	/* check that the default falls in the bitmask range */
	if (field->defvalue & ~field->mask)
	{
		mame_printf_error("%s: %s has an analog port with a default value out of the bitmask range\n", driver->source_file, driver->name);
		*error = TRUE;
	}

	/* tests for absolute devices */
	if (field->type >= __ipt_analog_absolute_start && field->type <= __ipt_analog_absolute_end)
	{
		INT32 default_value = field->defvalue;

		/* adjust for signed values */
		if (analog_min > analog_max)
		{
			analog_min = -analog_min;
			if (default_value > analog_max)
				default_value = -default_value;
		}

		/* check that the default falls in the MINMAX range */
		if (default_value < analog_min || default_value > analog_max)
		{
			mame_printf_error("%s: %s has an analog port with a default value out PORT_MINMAX range\n", driver->source_file, driver->name);
			*error = TRUE;
		}

		/* check that the MINMAX falls in the bitmask range */
		/* we use the unadjusted min for testing */
		if (field->min & ~field->mask || analog_max & ~field->mask)
		{
			mame_printf_error("%s: %s has an analog port with a PORT_MINMAX value out of the bitmask range\n", driver->source_file, driver->name);
			*error = TRUE;
		}

		/* absolute analog ports do not use PORT_RESET */
		if (field->flags & ANALOG_FLAG_RESET)
		{
			mame_printf_error("%s: %s - absolute analog ports do not use PORT_RESET\n", driver->source_file, driver->name);
			*error = TRUE;
		}
	}

	/* tests for relative devices */
	else
	{
		/* tests for non IPT_POSITIONAL relative devices */
		if (field->type != IPT_POSITIONAL && field->type != IPT_POSITIONAL_V)
		{
			/* relative devices do not use PORT_MINMAX */
			if (field->min != 0 || field->max != field->mask)
			{
				mame_printf_error("%s: %s - relative ports do not use PORT_MINMAX\n", driver->source_file, driver->name);
				*error = TRUE;
			}

			/* relative devices do not use a default value */
			/* the counter is at 0 on power up */
			if (field->defvalue != 0)
			{
				mame_printf_error("%s: %s - relative ports do not use a default value other then 0\n", driver->source_file, driver->name);
				*error = TRUE;
			}
		}
	}
}


/*-------------------------------------------------
    validate_dip_settings - validate a DIP switch
    setting
-------------------------------------------------*/

static void validate_dip_settings(const input_field_config *field, const game_driver *driver, int *error)
{
	const char *demo_sounds = input_port_string_from_index(INPUT_STRING_Demo_Sounds);
	const char *flipscreen = input_port_string_from_index(INPUT_STRING_Flip_Screen);
	UINT8 coin_list[INPUT_STRING_1C_9C + 1 - INPUT_STRING_9C_1C] = { 0 };
	const input_setting_config *setting;
	int coin_error = FALSE;

	/* iterate through the settings */
	for (setting = field->settinglist; setting != NULL; setting = setting->next)
	{
		int strindex = get_defstr_index(setting->name, driver, error);

		/* note any coinage strings */
		if (strindex >= INPUT_STRING_9C_1C && strindex <= INPUT_STRING_1C_9C)
			coin_list[strindex - INPUT_STRING_9C_1C] = 1;

		/* make sure demo sounds default to on */
		if (field->name == demo_sounds && strindex == INPUT_STRING_On && field->defvalue != setting->value)
		{
			mame_printf_error("%s: %s Demo Sounds must default to On\n", driver->source_file, driver->name);
			*error = TRUE;
		}

		/* check for bad demo sounds options */
		if (field->name == demo_sounds && (strindex == INPUT_STRING_Yes || strindex == INPUT_STRING_No))
		{
			mame_printf_error("%s: %s has wrong Demo Sounds option %s (must be Off/On)\n", driver->source_file, driver->name, setting->name);
			*error = TRUE;
		}

		/* check for bad flip screen options */
		if (field->name == flipscreen && (strindex == INPUT_STRING_Yes || strindex == INPUT_STRING_No))
		{
			mame_printf_error("%s: %s has wrong Flip Screen option %s (must be Off/On)\n", driver->source_file, driver->name, setting->name);
			*error = TRUE;
		}

		/* if we have a neighbor, compare ourselves to him */
		if (setting->next != NULL)
		{
			int next_strindex = get_defstr_index(setting->next->name, driver, error);

			/* check for inverted off/on dispswitch order */
			if (strindex == INPUT_STRING_On && next_strindex == INPUT_STRING_Off)
			{
				mame_printf_error("%s: %s has inverted Off/On dipswitch order\n", driver->source_file, driver->name);
				*error = TRUE;
			}

			/* check for inverted yes/no dispswitch order */
			else if (strindex == INPUT_STRING_Yes && next_strindex == INPUT_STRING_No)
			{
				mame_printf_error("%s: %s has inverted No/Yes dipswitch order\n", driver->source_file, driver->name);
				*error = TRUE;
			}

			/* check for inverted upright/cocktail dispswitch order */
			else if (strindex == INPUT_STRING_Cocktail && next_strindex == INPUT_STRING_Upright)
			{
				mame_printf_error("%s: %s has inverted Upright/Cocktail dipswitch order\n", driver->source_file, driver->name);
				*error = TRUE;
			}

			/* check for proper coin ordering */
			else if (strindex >= INPUT_STRING_9C_1C && strindex <= INPUT_STRING_1C_9C && next_strindex >= INPUT_STRING_9C_1C && next_strindex <= INPUT_STRING_1C_9C &&
					 strindex >= next_strindex && memcmp(&setting->condition, &setting->next->condition, sizeof(setting->condition)) == 0)
			{
				mame_printf_error("%s: %s has unsorted coinage %s > %s\n", driver->source_file, driver->name, setting->name, setting->next->name);
				coin_error = *error = TRUE;
			}
		}
	}

	/* if we have a coin error, demonstrate the correct way */
	if (coin_error)
	{
		int entry;

		mame_printf_error("%s: %s proper coin sort order should be:\n", driver->source_file, driver->name);
		for (entry = 0; entry < ARRAY_LENGTH(coin_list); entry++)
			if (coin_list[entry])
				mame_printf_error("%s\n", input_port_string_from_index(INPUT_STRING_9C_1C + entry));
	}
}


/*-------------------------------------------------
    validate_inputs - validate input configuration
-------------------------------------------------*/

static int validate_inputs(int drivnum, const machine_config *config)
{
	const input_port_config *portlist;
	const input_port_config *scanport;
	const input_port_config *port;
	const input_field_config *field;
	const game_driver *driver = drivers[drivnum];
	int empty_string_found = FALSE;
	char errorbuf[1024];
	quark_entry *entry;
	int error = FALSE;
	UINT32 crc;

	/* skip if no ports */
	if (driver->ipt == NULL)
		return FALSE;

	/* skip if we already validated these ports */
	crc = (FPTR)driver->ipt;
	for (entry = quark_table_get_first(inputs_table, crc); entry != NULL; entry = entry->next)
		if (entry->crc == crc && driver->ipt == drivers[entry - inputs_table->entry]->ipt)
			return FALSE;

	/* otherwise, add ourself to the list */
	quark_add(inputs_table, drivnum, crc);

	/* allocate the input ports */
	portlist = input_port_config_alloc(driver->ipt, errorbuf, sizeof(errorbuf));
	if (errorbuf[0] != 0)
	{
		mame_printf_error("%s: %s has input port errors:\n%s\n", driver->source_file, driver->name, errorbuf);
		error = TRUE;
	}

	/* check for duplicate tags */
	for (port = portlist; port != NULL; port = port->next)
		if (port->tag != NULL)
			for (scanport = port->next; scanport != NULL; scanport = scanport->next)
				if (scanport->tag != NULL && strcmp(port->tag, scanport->tag) == 0)
				{
					mame_printf_error("%s: %s has a duplicate input port tag \"%s\"\n", driver->source_file, driver->name, port->tag);
					error = TRUE;
				}

	/* iterate over the results */
	for (port = portlist; port != NULL; port = port->next)
		for (field = port->fieldlist; field != NULL; field = field->next)
		{
			int strindex = 0;

			/* verify analog inputs */
			if (input_type_is_analog(field->type))
				validate_analog_input_field(field, driver, &error);

			/* verify dip switches */
			if (field->type == IPT_DIPSWITCH)
			{
				/* dip switch fields must have a name */
				if (field->name == NULL)
				{
					mame_printf_error("%s: %s has a DIP switch name or setting with no name\n", driver->source_file, driver->name);
					error = TRUE;
				}

				/* verify the settings list */
				validate_dip_settings(field, driver, &error);
			}

			/* look for invalid (0) types which should be mapped to IPT_OTHER */
			if (field->type == IPT_INVALID)
			{
				mame_printf_error("%s: %s has an input port with an invalid type (0); use IPT_OTHER instead\n", driver->source_file, driver->name);
				error = TRUE;
			}

			/* verify names */
			if (field->name != NULL)
			{
				/* check for empty string */
				if (field->name[0] == 0 && !empty_string_found)
				{
					mame_printf_error("%s: %s has an input with an empty string\n", driver->source_file, driver->name);
					empty_string_found = error = TRUE;
				}

				/* check for trailing spaces */
				if (field->name[0] != 0 && field->name[strlen(field->name) - 1] == ' ')
				{
					mame_printf_error("%s: %s input '%s' has trailing spaces\n", driver->source_file, driver->name, field->name);
					error = TRUE;
				}

				/* check for invalid UTF-8 */
				if (!utf8_is_valid_string(field->name))
				{
					mame_printf_error("%s: %s input '%s' has invalid characters\n", driver->source_file, driver->name, field->name);
					error = TRUE;
				}

				/* look up the string and print an error if default strings are not used */
				strindex = get_defstr_index(field->name, driver, &error);
			}
		}

#ifdef MESS
	if (mess_validate_input_ports(drivnum, config, portlist))
		error = TRUE;
#endif /* MESS */

	/* free the config */
	input_port_config_free(portlist);
	return error;
}


/*-------------------------------------------------
    validate_sound - validate sound and
    speaker configuration
-------------------------------------------------*/

static int validate_sound(int drivnum, const machine_config *config)
{
	const game_driver *driver = drivers[drivnum];
	const device_config *curspeak, *checkspeak;
	int error = FALSE;
	int sndnum;

	/* make sure the speaker layout makes sense */
	for (curspeak = speaker_output_first(config); curspeak != NULL; curspeak = speaker_output_next(curspeak))
	{
		int check;

		/* check for duplicate tags */
		for (checkspeak = speaker_output_first(config); checkspeak != NULL; checkspeak = speaker_output_next(checkspeak))
			if (checkspeak != curspeak && strcmp(checkspeak->tag, curspeak->tag) == 0)
			{
				mame_printf_error("%s: %s has multiple speakers tagged as '%s'\n", driver->source_file, driver->name, checkspeak->tag);
				error = TRUE;
			}

		/* make sure there are no sound chips with the same tag */
		for (check = 0; check < MAX_SOUND && config->sound[check].type != SOUND_DUMMY; check++)
			if (config->sound[check].tag && strcmp(curspeak->tag, config->sound[check].tag) == 0)
			{
				mame_printf_error("%s: %s has both a speaker and a sound chip tagged as '%s'\n", driver->source_file, driver->name, curspeak->tag);
				error = TRUE;
			}
	}

	/* make sure the sounds are wired to the speakers correctly */
	for (sndnum = 0; sndnum < MAX_SOUND && config->sound[sndnum].type != SOUND_DUMMY; sndnum++)
	{
		int routenum, checknum;

		/* check for duplicate tags */
		if (config->sound[sndnum].tag != NULL)
			for (checknum = 0; checknum < sndnum; checknum++)
				if (config->sound[checknum].tag != NULL && strcmp(config->sound[sndnum].tag, config->sound[checknum].tag) == 0)
				{
					mame_printf_error("%s: %s has multiple sound chips tagged as '%s'\n", driver->source_file, driver->name, config->sound[sndnum].tag);
					error = TRUE;
				}

		/* loop over all the routes */
		for (routenum = 0; routenum < config->sound[sndnum].routes; routenum++)
		{
			/* find a speaker with the requested tag */
			for (checkspeak = speaker_output_first(config); checkspeak != NULL; checkspeak = speaker_output_next(checkspeak))
				if (strcmp(config->sound[sndnum].route[routenum].target, checkspeak->tag) == 0)
					break;

			/* if we didn't find one, look for another sound chip with the tag */
			if (checkspeak == NULL)
			{
				int check;

				for (check = 0; check < MAX_SOUND && config->sound[check].type != SOUND_DUMMY; check++)
					if (check != sndnum && config->sound[check].tag && strcmp(config->sound[check].tag, config->sound[sndnum].route[routenum].target) == 0)
						break;

				/* if we didn't find one, it's an error */
				if (check >= MAX_SOUND || config->sound[check].type == SOUND_DUMMY)
				{
					mame_printf_error("%s: %s attempting to route sound to non-existant speaker '%s'\n", driver->source_file, driver->name, config->sound[sndnum].route[routenum].target);
					error = TRUE;
				}
			}
		}
	}

	return error;
}


/*-------------------------------------------------
    validate_devices - run per-device validity
    checks
-------------------------------------------------*/

static int validate_devices(int drivnum, const machine_config *config)
{
	int error = FALSE;
	const game_driver *driver = drivers[drivnum];
	const device_config *device;

	for (device = device_list_first(config->devicelist, DEVICE_TYPE_WILDCARD); device != NULL; device = device_list_next(device, DEVICE_TYPE_WILDCARD))
	{
		device_validity_check_func validity_check = (device_validity_check_func) device_get_info_fct(device, DEVINFO_FCT_VALIDITY_CHECK);
		if (validity_check != NULL)
		{
			if ((*validity_check)(driver, device))
				error = TRUE;
		}
	}
	return error;
}


/*-------------------------------------------------
    mame_validitychecks - master validity checker
-------------------------------------------------*/

int mame_validitychecks(const game_driver *curdriver)
{
	osd_ticks_t prep = 0;
	osd_ticks_t expansion = 0;
	osd_ticks_t driver_checks = 0;
	osd_ticks_t rom_checks = 0;
	osd_ticks_t cpu_checks = 0;
	osd_ticks_t gfx_checks = 0;
	osd_ticks_t display_checks = 0;
	osd_ticks_t input_checks = 0;
	osd_ticks_t sound_checks = 0;
	osd_ticks_t device_checks = 0;
#ifdef MESS
	osd_ticks_t mess_checks = 0;
#endif

	int drivnum;
	int error = FALSE;
	UINT16 lsbtest;
	UINT8 a, b;

	/* basic system checks */
	a = 0xff;
	b = a + 1;
	if (b > a)	{ mame_printf_error("UINT8 must be 8 bits\n"); error = TRUE; }

	if (sizeof(INT8)   != 1)	{ mame_printf_error("INT8 must be 8 bits\n"); error = TRUE; }
	if (sizeof(UINT8)  != 1)	{ mame_printf_error("UINT8 must be 8 bits\n"); error = TRUE; }
	if (sizeof(INT16)  != 2)	{ mame_printf_error("INT16 must be 16 bits\n"); error = TRUE; }
	if (sizeof(UINT16) != 2)	{ mame_printf_error("UINT16 must be 16 bits\n"); error = TRUE; }
	if (sizeof(INT32)  != 4)	{ mame_printf_error("INT32 must be 32 bits\n"); error = TRUE; }
	if (sizeof(UINT32) != 4)	{ mame_printf_error("UINT32 must be 32 bits\n"); error = TRUE; }
	if (sizeof(INT64)  != 8)	{ mame_printf_error("INT64 must be 64 bits\n"); error = TRUE; }
	if (sizeof(UINT64) != 8)	{ mame_printf_error("UINT64 must be 64 bits\n"); error = TRUE; }
#ifdef PTR64
	if (sizeof(void *) != 8)	{ mame_printf_error("PTR64 flag enabled, but was compiled for 32-bit target\n"); error = TRUE; }
#else
	if (sizeof(void *) != 4)	{ mame_printf_error("PTR64 flag not enabled, but was compiled for 64-bit target\n"); error = TRUE; }
#endif
	lsbtest = 0;
	*(UINT8 *)&lsbtest = 0xff;
#ifdef LSB_FIRST
	if (lsbtest == 0xff00)		{ mame_printf_error("LSB_FIRST specified, but running on a big-endian machine\n"); error = TRUE; }
#else
	if (lsbtest == 0x00ff)		{ mame_printf_error("LSB_FIRST not specified, but running on a little-endian machine\n"); error = TRUE; }
#endif

	/* validate inline function behavior */
	error = validate_inlines() || error;

	/* make sure the CPU and sound interfaces are up and running */
	cpuintrf_init(NULL);
	sndintrf_init(NULL);

	init_resource_tracking();
	begin_resource_tracking();
	osd_profiling_ticks();

	/* prepare by pre-scanning all the drivers and adding their info to hash tables */
	prep -= osd_profiling_ticks();
	quark_tables_create();
	prep += osd_profiling_ticks();

	/* iterate over all drivers */
	for (drivnum = 0; drivers[drivnum]; drivnum++)
	{
		const game_driver *driver = drivers[drivnum];
		UINT32 region_length[REGION_MAX];
		machine_config *config;

/* ASG -- trying this for a while to see if submission failures increase */
#if 1
		/* non-debug builds only care about games in the same driver */
		if (curdriver != NULL && strcmp(curdriver->source_file, driver->source_file) != 0)
			continue;
#endif

		/* expand the machine driver */
		expansion -= osd_profiling_ticks();
		config = machine_config_alloc(driver->machine_config);
		expansion += osd_profiling_ticks();

		/* validate the driver entry */
		driver_checks -= osd_profiling_ticks();
		error = validate_driver(drivnum, config) || error;
		driver_checks += osd_profiling_ticks();

		/* validate the ROM information */
		rom_checks -= osd_profiling_ticks();
		error = validate_roms(drivnum, config, region_length) || error;
		rom_checks += osd_profiling_ticks();

		/* validate the CPU information */
		cpu_checks -= osd_profiling_ticks();
		error = validate_cpu(drivnum, config, region_length) || error;
		cpu_checks += osd_profiling_ticks();

		/* validate the display */
		display_checks -= osd_profiling_ticks();
		error = validate_display(drivnum, config) || error;
		display_checks += osd_profiling_ticks();

		/* validate the graphics decoding */
		gfx_checks -= osd_profiling_ticks();
		error = validate_gfx(drivnum, config, region_length) || error;
		gfx_checks += osd_profiling_ticks();

		/* validate input ports */
		input_checks -= osd_profiling_ticks();
		error = validate_inputs(drivnum, config) || error;
		input_checks += osd_profiling_ticks();

		/* validate sounds and speakers */
		sound_checks -= osd_profiling_ticks();
		error = validate_sound(drivnum, config) || error;
		sound_checks += osd_profiling_ticks();

		/* validate devices */
		device_checks -= osd_profiling_ticks();
		error = validate_devices(drivnum, config) || error;
		device_checks += osd_profiling_ticks();

		machine_config_free(config);
	}

#ifdef MESS
	mess_checks -= osd_profiling_ticks();
	if (mess_validitychecks())
		error = TRUE;
	mess_checks += osd_profiling_ticks();
#endif /* MESS */

#if (REPORT_TIMES)
	mame_printf_info("Prep:      %8dm\n", (int)(prep / 1000000));
	mame_printf_info("Expansion: %8dm\n", (int)(expansion / 1000000));
	mame_printf_info("Driver:    %8dm\n", (int)(driver_checks / 1000000));
	mame_printf_info("ROM:       %8dm\n", (int)(rom_checks / 1000000));
	mame_printf_info("CPU:       %8dm\n", (int)(cpu_checks / 1000000));
	mame_printf_info("Display:   %8dm\n", (int)(display_checks / 1000000));
	mame_printf_info("Graphics:  %8dm\n", (int)(gfx_checks / 1000000));
	mame_printf_info("Input:     %8dm\n", (int)(input_checks / 1000000));
	mame_printf_info("Sound:     %8dm\n", (int)(sound_checks / 1000000));
#ifdef MESS
	mame_printf_info("MESS:      %8dm\n", (int)(mess_checks / 1000000));
#endif
#endif

	end_resource_tracking();
	exit_resource_tracking();

	return error;
}
