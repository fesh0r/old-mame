/***************************************************************************

    softlist.c

    Software list construction helpers.


***************************************************************************/

#include "driver.h"
#include "softlist.h"


/*-------------------------------------------------
    software_list_get_by_name - return a pointer to
    a software list given its name.
-------------------------------------------------*/
const software_list* software_list_get_by_name(const char *name)
{
	int listnum;

	/* scan for a match in the list of software lists */
	for ( listnum = 0; software_lists[listnum] != NULL; listnum++ )
	{
		if ( mame_stricmp( software_lists[listnum]->name, name ) == 0 )
		{
			return software_lists[listnum];
		}
	}

	return NULL;
}


/*-------------------------------------------------
    software_get_by_name - return a pointer to 
    a software entry list given its name and a
    software list.
-------------------------------------------------*/

const software_entry* software_get_by_name(const software_list* list, const char *name)
{
	const char *softname;
	int entrynum;

	assert( name != NULL );

	softname = strrchr( name, ':' );
	if ( softname )
		softname++;	/* Skip ':' */
	else
		softname = name;

	/* scan for a match in the software list */
	for ( entrynum = 0; list->entries[entrynum].name != NULL; entrynum++ )
	{
		if ( mame_stricmp( list->entries[entrynum].name, softname ) == 0 )
		{
			return &list->entries[entrynum];
		}
	}
	return NULL;
}


/*-------------------------------------------------
    software_lists_get_count - returns the amount of
    drivers
-------------------------------------------------*/

int software_lists_get_count(void)
{
    int count;

    for (count = 0; software_lists[count] != NULL; count++) ;
    return count;
}
