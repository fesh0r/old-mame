//============================================================
//
//  datamap.c - Win32 dialog and options bridge code
//
//============================================================

#ifndef _DATAMAP_H_
#define _DATAMAP_H_

#include "options.h"


//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef enum _datamap_entry_type datamap_entry_type;
enum _datamap_entry_type
{
	DM_NONE = 0,
	DM_BOOL,
	DM_INT,
	DM_FLOAT,
	DM_STRING
};


typedef enum _datamap_callback_type datamap_callback_type;
enum _datamap_callback_type
{
	DCT_READ_CONTROL,
	DCT_POPULATE_CONTROL,
	DCT_UPDATE_STATUS,

	DCT_COUNT
};


typedef struct _datamap datamap;
// MSH - Callback can now return TRUE, signifying that changes have been made, but should NOT be broadcast.
typedef BOOL (*datamap_callback)(datamap *map, HWND dialog, HWND control, core_options *opts, const char *option_name);
typedef void (*get_option_name_callback)(datamap *map, HWND dialog, HWND control, char *buffer, size_t buffer_size);


//============================================================
//  PROTOTYPES
//============================================================

// datamap creation and disposal
datamap *datamap_create(void);
void datamap_free(datamap *map);

// datamap setup
void datamap_add(datamap *map, int dlgitem, datamap_entry_type type, const char *option_name);
void datamap_set_callback(datamap *map, int dlgitem, datamap_callback_type callback_type, datamap_callback callback);
void datamap_set_option_name_callback(datamap *map, int dlgitem, get_option_name_callback get_option_name);
void datamap_set_trackbar_range(datamap *map, int dlgitem, float min, float max, float increments);
void datamap_set_int_format(datamap *map, int dlgitem, const char *format);
void datamap_set_float_format(datamap *map, int dlgitem, const char *format);

// datamap operations
BOOL datamap_read_control(datamap *map, HWND dialog, core_options *opts, int dlgitem);
void datamap_read_all_controls(datamap *map, HWND dialog, core_options *opts);
void datamap_populate_control(datamap *map, HWND dialog, core_options *opts, int dlgitem);
void datamap_populate_all_controls(datamap *map, HWND dialog, core_options *opts);
void datamap_update_control(datamap *map, HWND dialog, core_options *opts, int dlgitem);
void datamap_update_all_controls(datamap *map, HWND dialog, core_options *opts);

#endif // _DATAMAP_H_
