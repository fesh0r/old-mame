//============================================================
//
//  sdldir.c - SDL core directory access functions
//
//  Copyright (c) 1996-2010, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#define _LARGEFILE64_SOURCE
#ifdef SDLMAME_LINUX
#define __USE_LARGEFILE64
#endif
#ifndef SDLMAME_BSD
#define _XOPEN_SOURCE 500
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __USE_BSD
#define __USE_BSD	// to get DT_xxx on Linux
#endif
#undef _POSIX_C_SOURCE  // to get DT_xxx on OS X
#include <dirent.h>

#include "osdcore.h"

#if defined(SDLMAME_WIN32) || defined(SDLMAME_OS2)
#define PATHSEPCH '\\'
#define INVPATHSEPCH '/'
#else
#define PATHSEPCH '/'
#define INVPATHSEPCH '\\'
#endif

struct _osd_directory
{
	osd_directory_entry ent;
#if defined(SDLMAME_DARWIN) || defined(SDLMAME_WIN32) || defined(SDLMAME_NO64BITIO) || defined(SDLMAME_BSD) || defined(SDLMAME_OS2)
	struct dirent *data;
#else
	struct dirent64 *data;
#endif
	DIR *fd;
};


#if defined (SDLMAME_LINUX) || defined (SDLMAME_BSD) || defined(SDLMAME_DARWIN)
static osd_dir_entry_type get_attributes_enttype(int attributes)
{
	if (attributes == DT_DIR)
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}
#else
static osd_dir_entry_type get_attributes_stat(const char *file)
{
#if defined(SDLMAME_WIN32) || defined(SDLMAME_NO64BITIO) || defined(SDLMAME_OS2)
	struct stat st;
	if(stat(file, &st))
		return (osd_dir_entry_type) 0;
#else
	struct stat64 st;
	if(stat64(file, &st))
		return 0;
#endif

#ifdef SDLMAME_WIN32
	if (S_ISBLK(st.st_mode)) return ENTTYPE_DIR;
#else
	if (S_ISDIR(st.st_mode)) return ENTTYPE_DIR;
#endif

	return ENTTYPE_FILE;
}
#endif

static UINT64 osd_get_file_size(const char *file)
{
#if defined(SDLMAME_DARWIN) || defined(SDLMAME_WIN32) || defined(SDLMAME_NO64BITIO) || defined(SDLMAME_BSD) || defined(SDLMAME_OS2)
	struct stat st;
	if(stat(file, &st))
		return 0;
#else
	struct stat64 st;
	if(stat64(file, &st))
		return 0;
#endif
	return st.st_size;
}

//============================================================
//  osd_opendir
//============================================================

osd_directory *osd_opendir(const char *dirname)
{
	osd_directory *dir = NULL;
	char *tmpstr, *envstr;
	int i, j;

	dir = (osd_directory *) malloc(sizeof(osd_directory));
	if (dir)
	{
		memset(dir, 0, sizeof(osd_directory));
		dir->fd = NULL;
	}

	tmpstr = (char *) malloc(strlen(dirname)+1);
	strcpy(tmpstr, dirname);

	if (tmpstr[0] == '$')
	{
		char *envval;
		envstr = (char *) malloc(strlen(tmpstr)+1);

		strcpy(envstr, tmpstr);

		i = 0;
		while (envstr[i] != PATHSEPCH && envstr[i] != 0 && envstr[i] != '.')
		{
			i++;
		}

		envstr[i] = '\0';

		envval = getenv(&envstr[1]);
		if (envval != NULL)
		{
			j = strlen(envval) + strlen(tmpstr) + 1;
			free(tmpstr);
			tmpstr = (char *) malloc(j);

			// start with the value of $HOME
			strcpy(tmpstr, envval);
			// replace the null with a path separator again
			envstr[i] = PATHSEPCH;
			// append it
			strcat(tmpstr, &envstr[i]);
		}
		else
			fprintf(stderr, "Warning: osd_opendir environment variable %s not found.\n", envstr);
		free(envstr);
	}

	dir->fd = opendir(tmpstr);

	if (dir && (dir->fd == NULL))
	{
		free(dir);
		dir = NULL;
	}

	if (tmpstr)
	  free(tmpstr);
	return dir;
}


//============================================================
//  osd_readdir
//============================================================

const osd_directory_entry *osd_readdir(osd_directory *dir)
{
	#if defined(SDLMAME_DARWIN) || defined(SDLMAME_WIN32) || defined(SDLMAME_NO64BITIO) || defined(SDLMAME_BSD) || defined(SDLMAME_OS2)
	dir->data = readdir(dir->fd);
	#else
	dir->data = readdir64(dir->fd);
	#endif

	if (dir->data == NULL)
		return NULL;

	dir->ent.name = dir->data->d_name;
	#if defined (SDLMAME_LINUX) || defined (SDLMAME_BSD) || defined(SDLMAME_DARWIN)
	dir->ent.type = get_attributes_enttype(dir->data->d_type);
	#else
	dir->ent.type = get_attributes_stat(dir->data->d_name);
	#endif
	dir->ent.size = osd_get_file_size(dir->data->d_name);
	return &dir->ent;
}


//============================================================
//  osd_closedir
//============================================================

void osd_closedir(osd_directory *dir)
{
	if (dir->fd != NULL)
		closedir(dir->fd);
	free(dir);
}

