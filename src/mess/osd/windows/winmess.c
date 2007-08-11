#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>
#include <tchar.h>

#include "osdmess.h"
#include "utils.h"
#include "winutil.h"
#include "strconv.h"
#include "winutf8.h"


//============================================================
//  win_error_to_mame_file_error
//============================================================

static file_error win_error_to_mame_file_error(DWORD error)
{
	file_error filerr;

	// convert a Windows error to a file_error
	switch (error)
	{
		case ERROR_SUCCESS:
			filerr = FILERR_NONE;
			break;

		case ERROR_OUTOFMEMORY:
			filerr = FILERR_OUT_OF_MEMORY;
			break;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			filerr = FILERR_NOT_FOUND;
			break;

		case ERROR_ACCESS_DENIED:
			filerr = FILERR_ACCESS_DENIED;
			break;

		case ERROR_SHARING_VIOLATION:
			filerr = FILERR_ALREADY_OPEN;
			break;

		default:
			filerr = FILERR_FAILURE;
			break;
	}
	return filerr;
}


//============================================================
//	osd_get_temp_filename
//============================================================

file_error osd_get_temp_filename(char *buffer, size_t buffer_len, const char *basename)
{
	TCHAR tempbuf[MAX_PATH];
	TCHAR *t_filename;
	char *u_tempbuf;

	if (!basename)
		basename = "tempfile.tmp";

	GetTempPath(sizeof(tempbuf) / sizeof(tempbuf[0]), tempbuf);
	t_filename = tstring_from_utf8(basename);
	_tcscat(tempbuf, t_filename);
	free(t_filename);
	DeleteFile(tempbuf);

	u_tempbuf = utf8_from_tstring(tempbuf);
	snprintf(buffer, buffer_len, "%s", u_tempbuf);
	free(u_tempbuf);

	return FILERR_NONE;
}


//============================================================
//	osd_copyfile
//============================================================

file_error osd_copyfile(const char *destfile, const char *srcfile)
{
	// wrapper for win_copy_file_utf8()
	if (!win_copy_file_utf8(srcfile, destfile, TRUE))
		return win_error_to_mame_file_error(GetLastError());

	return FILERR_NONE;
}


//============================================================
//  osd_stat
//============================================================

osd_directory_entry *osd_stat(const char *path)
{
	osd_directory_entry *result = NULL;
	TCHAR *t_path;
	HANDLE find = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA find_data;

	// convert the path to TCHARs
	t_path = tstring_from_utf8(path);
	if (t_path == NULL)
		goto done;

	// attempt to find the first file
	find = FindFirstFile(t_path, &find_data);
	if (find == INVALID_HANDLE_VALUE)
		goto done;

	// create an osd_directory_entry; be sure to make sure that the caller can
	// free all resources by just freeing the resulting osd_directory_entry
	result = (osd_directory_entry *) malloc(sizeof(*result) + strlen(path) + 1);
	if (!result)
		goto done;
	strcpy(((char *) result) + sizeof(*result), path);
	result->name = ((char *) result) + sizeof(*result);
	result->type = win_attributes_to_entry_type(find_data.dwFileAttributes);
	result->size = find_data.nFileSizeLow | ((UINT64) find_data.nFileSizeHigh << 32);

done:
	if (t_path)
		free(t_path);
	return result;
}



//============================================================
//  osd_is_path_separator
//============================================================

int osd_is_path_separator(char c)
{
	return (c == '/') || (c == '\\');
}



//============================================================
//	osd_dirname
//============================================================

char *osd_dirname(const char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
		return NULL;

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}


//============================================================
//	osd_basename
//============================================================

char *osd_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}


//============================================================
//	osd_mkdir
//============================================================

file_error osd_mkdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!CreateDirectory(tempstr, NULL))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_rmdir
//============================================================

file_error osd_rmdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!RemoveDirectory(tempstr))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_getcurdir
//============================================================

file_error osd_getcurdir(char *buffer, size_t buffer_len)
{
	file_error filerr = FILERR_NONE;
	char *tempstr = NULL;
	TCHAR path[_MAX_PATH];

	if (!GetCurrentDirectory(ARRAY_LENGTH(path), path))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

	tempstr = utf8_from_tstring(path);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}
	snprintf(buffer, buffer_len, "%s", tempstr);

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_setcurdir
//============================================================

file_error osd_setcurdir(const char *dir)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!SetCurrentDirectory(tempstr))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}



/* ************************************************************************ */
/* Directories                                                              */
/* ************************************************************************ */

#ifdef UNDER_CE
int osd_num_devices(void)
{
	return 1;
}

const char *osd_get_device_name(int idx)
{
	return "\\";
}
#else
int osd_num_devices(void)
{
	char szBuffer[128];
	char *p;
	int i;

	GetLogicalDriveStringsA(sizeof(szBuffer) / sizeof(szBuffer[0]), szBuffer);

	i = 0;
	for (p = szBuffer; *p; p += (strlen(p) + 1))
		i++;
	return i;
}

const char *osd_get_device_name(int idx)
{
	static char szBuffer[128];
	const char *p;

	GetLogicalDriveStringsA(sizeof(szBuffer) / sizeof(szBuffer[0]), szBuffer);

	p = szBuffer;
	while(idx--)
		p += strlen(p) + 1;

	return p;
}
#endif
