/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef HELP_H
#define HELP_H

#ifdef _MSC_VER
#if _MSC_VER <= 1200
#define DWORD_PTR	DWORD
#endif
#endif

#ifdef MESS
/* MESS32 text files */
#define HELPTEXT_RELEASE    "docs\\Mess.txt"
#define HELPTEXT_WHATS_NEW  "Messnew.txt"
#else
/* MAME32 text files */
#define HELPTEXT_RELEASE    "windows.txt"
#define HELPTEXT_WHATS_NEW  "whatsnew.txt"
#endif

#ifndef MESS
#define HAS_HELP 1
#else
#define HAS_HELP 0
#endif

#if HAS_HELP

#include <htmlhelp.h>

#if !defined(MAME32HELP)
#define MAME32HELP "mame32.chm"
#endif

extern int  Help_Init(void);
extern void Help_Exit(void);
extern HWND Help_HtmlHelp(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

#endif /* HAS_HELP */

#endif /* HELP_H */
