/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  Properties.c

    Properties Popup and Misc UI support routines.

    Created 8/29/98 by Mike Haaland (mhaaland@hypertech.com)

***************************************************************************/
#define WIN32_LEAN_AND_MEAN
#define NONAMELESSUNION 1
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#undef NONAMELESSUNION
#undef bool //hack for the mingw compiler and mess code
#include <ddraw.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <driver.h>
#include <info.h>
#include "audit32.h"
#include "options.h"
#include "file.h"
#include "resource.h"
#include "DIJoystick.h"     /* For DIJoystick avalibility. */
#include "m32util.h"
#include "directdraw.h"
#include "properties.h"

#include "Mame32.h"
#include "DataMap.h"
#include "help.h"
#include "resource.hm"

#ifdef _MSC_VER
#if _MSC_VER > 1200
#define HAS_DUMMYUNIONNAME
#endif
#endif

/***************************************************************
 * Imported function prototypes
 ***************************************************************/

extern BOOL GameUsesTrackball(int game);
extern int load_driver_history(const struct GameDriver *drv, char *buffer, int bufsize);

/**************************************************************
 * Local function prototypes
 **************************************************************/

static INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK GameDisplayOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

static void SetStereoEnabled(HWND hWnd, int the_index);
static void SetYM3812Enabled(HWND hWnd, int the_index);
static void SetSamplesEnabled(HWND hWnd, int the_index, BOOL bSoundEnabled);
static void InitializeOptions(HWND hDlg);
static void InitializeMisc(HWND hDlg);
static void OptOnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);
static void BeamSelectionChange(HWND hwnd);
static void FlickerSelectionChange(HWND hwnd);
static void GammaSelectionChange(HWND hwnd);
static void BrightnessSelectionChange(HWND hwnd);
static void ResDepthSelectionChange(HWND hWnd, HWND hWndCtrl);
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl);
static void VolumeSelectionChange(HWND hwnd);
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwDepth, DWORD dwRefresh);
static void InitializeDisplayModeUI(HWND hwnd);
static void InitializeSoundUI(HWND hwnd);
static void InitializeSkippingUI(HWND hwnd);
static void InitializeRotateUI(HWND hwnd);
static void InitializeDepthUI(HWND hwnd);
static void InitializeResDepthUI(HWND hwnd);
static void InitializeRefreshUI(HWND hwnd);
static void InitializeDefaultInputUI(HWND hWnd);
static void InitializeEffectUI(HWND hWnd);
static void PropToOptions(HWND hWnd, options_type *o);
static void OptionsToProp(HWND hWnd, options_type *o);
static void SetPropEnabledControls(HWND hWnd);

static void BuildDataMap(void);
static void ResetDataMap(void);

static void HistoryFixBuf(char *buf);

/**************************************************************
 * Local private variables
 **************************************************************/

static options_type  origGameOpts;
static options_type* pGameOpts = NULL;

static int  g_nGame            = 0;
static BOOL g_bInternalSet     = FALSE;
static BOOL g_bUseDefaults     = FALSE;
static BOOL g_bReset           = FALSE;
static int  g_nSampleRateIndex = 0;
static int  g_nVolumeIndex     = 0;
static int  g_nDepthIndex      = 0;
static int  g_nGammaIndex      = 0;
static int  g_nBeamIndex       = 0;
static int  g_nFlickerIndex    = 0;
static int  g_nRotateIndex     = 0;
static int  g_nInputIndex      = 0;
static int  g_nBrightnessIndex = 0;
static int  g_nEffectIndex     = 0;

/* Game history variables */
#define MAX_HISTORY_LEN     (8 * 1024)

static char   historyBuf[MAX_HISTORY_LEN];
static char   tempHistoryBuf[MAX_HISTORY_LEN];

/* Property sheets */
static DWORD dwDlgId[] =
{
	IDD_PROP_GAME,
	IDD_PROP_AUDIT,
	IDD_PROP_DISPLAY,
	IDD_PROP_ADVANCED,
	IDD_PROP_SOUND,
	IDD_PROP_INPUT,
	IDD_PROP_MISC,
#ifdef MESS
	IDD_PROP_SOFTWARE,
#endif
	IDD_PROP_VECTOR
};

#define NUM_PROPSHEETS (sizeof(dwDlgId) / sizeof(dwDlgId[0]))

#if HAS_HELP
/* Help IDs */
static DWORD dwHelpIDs[] =
{
	IDC_ANTIALIAS,          HIDC_ANTIALIAS,
	IDC_ARTWORK,            HIDC_ARTWORK,
	IDC_ASPECTRATIOD,       HIDC_ASPECTRATIOD,
	IDC_ASPECTRATION,       HIDC_ASPECTRATION,
	IDC_AUTOFRAMESKIP,      HIDC_AUTOFRAMESKIP,
	IDC_BEAM,               HIDC_BEAM,
	IDC_BRIGHTNESS,         HIDC_BRIGHTNESS,
	IDC_CHEAT,              HIDC_CHEAT,
	IDC_DDRAW,              HIDC_DDRAW,
	IDC_DEFAULT_INPUT,      HIDC_DEFAULT_INPUT,
	IDC_DEPTH,              HIDC_DEPTH,
	IDC_DIRTY,              HIDC_DIRTY,
	IDC_EFFECT,             HIDC_EFFECT,
	IDC_FILTER_CLONES,      HIDC_FILTER_CLONES,
	IDC_FILTER_EDIT,        HIDC_FILTER_EDIT,
#ifndef NEOFREE
	IDC_FILTER_NEOGEO,      HIDC_FILTER_NEOGEO,
#endif
	IDC_FILTER_NONWORKING,  HIDC_FILTER_NONWORKING,
	IDC_FILTER_ORIGINALS,   HIDC_FILTER_ORIGINALS,
	IDC_FILTER_RASTER,      HIDC_FILTER_RASTER,
	IDC_FILTER_UNAVAILABLE, HIDC_FILTER_UNAVAILABLE,
	IDC_FILTER_VECTOR,      HIDC_FILTER_VECTOR,
	IDC_FILTER_WORKING,     HIDC_FILTER_WORKING,
	IDC_FLICKER,            HIDC_FLICKER,
	IDC_FLIPX,              HIDC_FLIPX,
	IDC_FLIPY,              HIDC_FLIPY,
	IDC_FRAMESKIP,          HIDC_FRAMESKIP,
	IDC_GAMMA,              HIDC_GAMMA,
	IDC_HISTORY,            HIDC_HISTORY,
	IDC_HWSTRETCH,          HIDC_HWSTRETCH,
	IDC_JOYSTICK,           HIDC_JOYSTICK,
	IDC_KEEPASPECT,         HIDC_KEEPASPECT,
	IDC_LANGUAGECHECK,      HIDC_LANGUAGECHECK,
	IDC_LANGUAGEEDIT,       HIDC_LANGUAGEEDIT,
	IDC_LOG,                HIDC_LOG,
	IDC_MATCHREFRESH,       HIDC_MATCHREFRESH,
	IDC_MAXIMIZE,           HIDC_MAXIMIZE,
	IDC_NOROTATE,           HIDC_NOROTATE,
	IDC_PROP_RESET,         HIDC_PROP_RESET,
	IDC_REFRESH,            HIDC_REFRESH,
	IDC_RESDEPTH,           HIDC_RESDEPTH,
	IDC_RESET_DEFAULT,      HIDC_RESET_DEFAULT,
	IDC_RESET_FILTERS,      HIDC_RESET_FILTERS,
	IDC_RESET_GAMES,        HIDC_RESET_GAMES,
	IDC_RESET_UI,           HIDC_RESET_UI,
	IDC_ROTATE,             HIDC_ROTATE,
	IDC_SAMPLERATE,         HIDC_SAMPLERATE,
	IDC_SAMPLES,            HIDC_SAMPLES,
	IDC_SCANLINES,          HIDC_SCANLINES,
	IDC_SIZES,              HIDC_SIZES,
	IDC_START_GAME_CHECK,   HIDC_START_GAME_CHECK,
	IDC_START_VERSION_WARN, HIDC_START_VERSION_WARN,
	IDC_SWITCHBPP,          HIDC_SWITCHBPP,
	IDC_SWITCHRES,          HIDC_SWITCHRES,
	IDC_SYNCREFRESH,        HIDC_SYNCREFRESH,
	IDC_THROTTLE,           HIDC_THROTTLE,
	IDC_TRANSLUCENCY,       HIDC_TRANSLUCENCY,
	IDC_TRIPLE_BUFFER,      HIDC_TRIPLE_BUFFER,
	IDC_USE_DEFAULT,        HIDC_USE_DEFAULT,
	IDC_USE_FILTER,         HIDC_USE_FILTER,
	IDC_USE_MOUSE,          HIDC_USE_MOUSE,
	IDC_USE_SOUND,          HIDC_USE_SOUND,
	IDC_VOLUME,             HIDC_VOLUME,
	IDC_WAITVSYNC,          HIDC_WAITVSYNC,
	IDC_WINDOWED,           HIDC_WINDOWED,
	0,                      0
};
#endif /* HAS_HELP */

static struct ComboBoxEffect
{
	const char*	m_pText;
	const char* m_pData;
} g_ComboBoxEffect[] =
{
	{ "None",                           "none"    },
	{ "25% scanlines",                  "scan25"  },
	{ "50% scanlines",                  "scan50"  },
	{ "75% scanlines",                  "scan75"  },
	{ "75% vertical scanlines",         "scan75v" },
	{ "RGB triad of 16 pixels",         "rgb16"   },
	{ "RGB triad of 6 pixels",          "rgb6"    },
	{ "RGB triad of 4 pixels",          "rgb4"    },
	{ "RGB triad of 4 vertical pixels", "rgb4v"   },
	{ "RGB triad of 3 pixels",          "rgb3"    },
	{ "RGB tiny",                       "rgbtiny" }
};

#define NUMEFFECTS (sizeof(g_ComboBoxEffect) / sizeof(g_ComboBoxEffect[0]))

/***************************************************************
 * Public functions
 ***************************************************************/

#if HAS_HELP
DWORD GetHelpIDs(void)
{
	return (DWORD) (LPSTR) dwHelpIDs;
}
#endif /* HAS_HELP */

/* Checks of all ROMs are available for 'game' and returns result
 * Returns TRUE if all ROMs found, 0 if any ROMs are missing.
 */
BOOL FindRomSet(int game)
{
	const struct RomModule	*region, *rom;
	const struct GameDriver *gamedrv;
	const char				*name;
	int 					err;
	unsigned int			length, icrc;

	gamedrv = drivers[game];

	if (!osd_faccess(gamedrv->name, OSD_FILETYPE_ROM))
	{
		/* if the game is a clone, try loading the ROM from the main version */
		if (gamedrv->clone_of == 0
		||	(gamedrv->clone_of->flags & NOT_A_DRIVER)
		||	!osd_faccess(gamedrv->clone_of->name, OSD_FILETYPE_ROM))
			return FALSE;
	}

	/* loop over regions, then over files */
	for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			extern struct GameDriver driver_0;
			const struct GameDriver *drv;

			name = ROM_GETNAME(rom);
			icrc = ROM_GETCRC(rom);
			length = 0;

			/* obtain CRC-32 and length of ROM file */
			drv = gamedrv;
			do
			{
				err = osd_fchecksum(drv->name, name, &length, &icrc);
				drv = drv->clone_of;
			}
			while (err && drv && drv != &driver_0);

			if (err)
				return FALSE;
		}
	}

	return TRUE;
}

/* Checks if the game uses external samples at all
 * Returns TRUE if this driver expects samples
 */
BOOL GameUsesSamples(int game)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)

	int i;
	static const struct GameDriver *gamedrv;

	gamedrv = drivers[game];

	for (i = 0; gamedrv->drv->sound[i].sound_type && i < MAX_SOUND; i++)
	{
		const char **samplenames = NULL;

#if (HAS_SAMPLES == 1)
		if (gamedrv->drv->sound[i].sound_type == SOUND_SAMPLES)
			samplenames = ((struct Samplesinterface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

#if (HAS_VLM5030 == 1)
		if (gamedrv->drv->sound[i].sound_type == SOUND_VLM5030)
			samplenames = ((struct VLM5030interface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

		if (samplenames != 0 && samplenames[0] != 0)
			return TRUE;
	}

#endif

	return FALSE;
}

/* Checks for all samples in a sample set.
 * Returns TRUE if all samples are found, FALSE if any are missing.
 */
BOOL FindSampleSet(int game)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)

	static const struct GameDriver *gamedrv;
	const char* sharedname;
	BOOL bStatus;
	int  skipfirst;
	int  j, i;

	if (GameUsesSamples(game) == FALSE)
		return TRUE;

	gamedrv = drivers[game];

	for (i = 0; gamedrv->drv->sound[i].sound_type && i < MAX_SOUND; i++)
	{
		const char **samplenames = NULL;

#if (HAS_SAMPLES == 1)
		if (gamedrv->drv->sound[i].sound_type == SOUND_SAMPLES)
			samplenames = ((struct Samplesinterface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

#if (HAS_VLM5030 == 1)
		if (gamedrv->drv->sound[i].sound_type == SOUND_VLM5030)
			samplenames = ((struct VLM5030interface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

		if (samplenames != 0 && samplenames[0] != 0)
		{
			BOOL have_samples = FALSE;
			BOOL have_shared  = FALSE;

			if (samplenames[0][0]=='*')
			{
				sharedname = samplenames[0]+1;
				skipfirst = 1;
			}
			else
			{
				sharedname = NULL;
				skipfirst = 0;
			}

			/* do we have samples for this game? */
			have_samples = osd_faccess(gamedrv->name, OSD_FILETYPE_SAMPLE);

			/* try shared samples */
			if (skipfirst)
				have_shared = osd_faccess(sharedname, OSD_FILETYPE_SAMPLE);

			/* if still not found, we're done */
			if (!have_samples && !have_shared)
				return FALSE;

			for (j = skipfirst; samplenames[j] != 0; j++)
			{
				bStatus = FALSE;

				/* skip empty definitions */
				if (strlen(samplenames[j]) == 0)
					continue;

				if (have_samples)
					bStatus = File_Status(gamedrv->name, samplenames[j], OSD_FILETYPE_SAMPLE);

				if (!bStatus && have_shared)
				{
					bStatus = File_Status(sharedname, samplenames[j], OSD_FILETYPE_SAMPLE);
					if (!bStatus)
					{
						return FALSE;
					}
				}
			}
		}
	}

#endif

	return TRUE;
}

void InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd)
{
	PROPSHEETHEADER pshead;
	PROPSHEETPAGE   pspage[NUM_PROPSHEETS];
	int             i;
	int             maxPropSheets;

	g_nGame = -1;
	maxPropSheets = NUM_PROPSHEETS - 2;

	/* Get default options to populate property sheets */
	pGameOpts = GetDefaultOptions();
	g_bUseDefaults = FALSE;
	/* Stash the result for comparing later */
	memcpy(&origGameOpts, pGameOpts, sizeof(options_type));
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(PROPSHEETHEADER));
	ZeroMemory(pspage, sizeof(PROPSHEETPAGE) * maxPropSheets);

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	pshead.pszCaption                 = "Default Game";
	pshead.nPages                     = maxPropSheets;
	pshead.DUMMYUNIONNAME2.nStartPage = 0;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Fill out the property page templates */
	for (i = 0; i < maxPropSheets; i++)
	{
		pspage[i].dwSize                     = sizeof(PROPSHEETPAGE);
		pspage[i].dwFlags                    = 0;
		pspage[i].hInstance                  = hInst;
		pspage[i].DUMMYUNIONNAME.pszTemplate = MAKEINTRESOURCE(dwDlgId[i + 2]);
		pspage[i].pfnCallback                = NULL;
		pspage[i].lParam                     = 0;
		pspage[i].pfnDlgProc                 = GameOptionsProc;
	}
	pspage[2 - 2].pfnDlgProc = GameDisplayOptionsProc;

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		MessageBox(0, temp, "Error", IDOK);
	}
}

void InitPropertyPage(HINSTANCE hInst, HWND hWnd, int game_num)
{
	InitPropertyPageToPage(hInst, hWnd, game_num, PROPERTIES_PAGE);
}

void InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, int game_num, int start_page)
{
	PROPSHEETHEADER pshead;
	PROPSHEETPAGE   pspage[NUM_PROPSHEETS];
	int             i;
	int             maxPropSheets;

	InitGameAudit(game_num);
	g_nGame = game_num;
	maxPropSheets = (drivers[g_nGame]->drv->video_attributes & VIDEO_TYPE_VECTOR) ? NUM_PROPSHEETS : NUM_PROPSHEETS - 1;

	/* Get Game options to populate property sheets */
	pGameOpts = GetGameOptions(game_num);
	g_bUseDefaults = pGameOpts->use_default;
	/* Stash the result for comparing later */
	memcpy(&origGameOpts, pGameOpts, sizeof(options_type));
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(PROPSHEETHEADER));
	ZeroMemory(pspage, sizeof(PROPSHEETPAGE) * maxPropSheets);

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	pshead.pszCaption                 = ModifyThe(drivers[g_nGame]->description);
	pshead.nPages                     = maxPropSheets;
	pshead.DUMMYUNIONNAME2.nStartPage = start_page;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Fill out the property page templates */
	for (i = 0; i < maxPropSheets; i++)
	{
		pspage[i].dwSize                     = sizeof(PROPSHEETPAGE);
		pspage[i].dwFlags                    = 0;
		pspage[i].hInstance                  = hInst;
		pspage[i].DUMMYUNIONNAME.pszTemplate = MAKEINTRESOURCE(dwDlgId[i]);
		pspage[i].pfnCallback                = NULL;
		pspage[i].lParam                     = 0;
	}

	pspage[0].pfnDlgProc = GamePropertiesDialogProc;
	pspage[1].pfnDlgProc = GameAuditDialogProc;

	pspage[2].pfnDlgProc = GameDisplayOptionsProc;
	pspage[3].pfnDlgProc = GameOptionsProc;
	pspage[4].pfnDlgProc = GameOptionsProc;
	pspage[5].pfnDlgProc = GameOptionsProc;
	pspage[6].pfnDlgProc = GameOptionsProc;

#ifdef MESS
    pspage[7].pfnDlgProc = GameOptionsProc;
#endif

	/* If this is a vector game, add the vector prop sheet */
	if (maxPropSheets == NUM_PROPSHEETS)
	{
		pspage[NUM_PROPSHEETS - 1].pfnDlgProc = GameOptionsProc;
	}

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		MessageBox(0, temp, "Error", IDOK);
	}
}

/*********************************************************************
 * Local Functions
 *********************************************************************/

/* Build CPU info string */
static char *GameInfoCPU(UINT nIndex)
{
	int i;
	static char buf[1024] = "";

	ZeroMemory(buf, sizeof(buf));
	i = 0;
	while (i < MAX_CPU && drivers[nIndex]->drv->cpu[i].cpu_type)
	{
		if (drivers[nIndex]->drv->cpu[i].cpu_clock >= 1000000)
			sprintf(&buf[strlen(buf)], "%s %d.%06d MHz",
					cputype_name(drivers[nIndex]->drv->cpu[i].cpu_type),
					drivers[nIndex]->drv->cpu[i].cpu_clock / 1000000,
					drivers[nIndex]->drv->cpu[i].cpu_clock % 1000000);
		else
			sprintf(&buf[strlen(buf)], "%s %d.%03d kHz",
					cputype_name(drivers[nIndex]->drv->cpu[i].cpu_type),
					drivers[nIndex]->drv->cpu[i].cpu_clock / 1000,
					drivers[nIndex]->drv->cpu[i].cpu_clock % 1000);

		if (drivers[nIndex]->drv->cpu[i].cpu_type & CPU_AUDIO_CPU)
			strcat(buf, " (sound)");

		strcat(buf, "\n");

		i++;
    }

	return buf;
}

/* Build Sound system info string */
static char *GameInfoSound(UINT nIndex)
{
	int i;
	static char buf[1024] = "";

	ZeroMemory(buf, sizeof(buf));
	i = 0;
	while (i < MAX_SOUND && drivers[nIndex]->drv->sound[i].sound_type)
	{
		if (1 < sound_num(&drivers[nIndex]->drv->sound[i]))
			sprintf(&buf[strlen(buf)], "%d x ", sound_num(&drivers[nIndex]->drv->sound[i]));

		sprintf(&buf[strlen(buf)], "%s", sound_name(&drivers[nIndex]->drv->sound[i]));

		if (sound_clock(&drivers[nIndex]->drv->sound[i]))
		{
			if (sound_clock(&drivers[nIndex]->drv->sound[i]) >= 1000000)
				sprintf(&buf[strlen(buf)], " %d.%06d MHz",
						sound_clock(&drivers[nIndex]->drv->sound[i]) / 1000000,
						sound_clock(&drivers[nIndex]->drv->sound[i]) % 1000000);
			else
				sprintf(&buf[strlen(buf)], " %d.%03d kHz",
						sound_clock(&drivers[nIndex]->drv->sound[i]) / 1000,
						sound_clock(&drivers[nIndex]->drv->sound[i]) % 1000);
		}

		strcat(buf,"\n");

		i++;
	}
	return buf;
}

/* Build Display info string */
static char *GameInfoScreen(UINT nIndex)
{
	static char buf[1024];

	if (drivers[nIndex]->drv->video_attributes & VIDEO_TYPE_VECTOR)
		strcpy(buf, "Vector Game");
	else
	{
		if (drivers[nIndex]->flags & ORIENTATION_SWAP_XY)
			sprintf(buf,"%d x %d (vert) %5.2f Hz",
					drivers[nIndex]->drv->default_visible_area.max_y - drivers[nIndex]->drv->default_visible_area.min_y + 1,
					drivers[nIndex]->drv->default_visible_area.max_x - drivers[nIndex]->drv->default_visible_area.min_x + 1,
					drivers[nIndex]->drv->frames_per_second);
		else
			sprintf(buf,"%d x %d (horz) %5.2f Hz",
					drivers[nIndex]->drv->default_visible_area.max_x - drivers[nIndex]->drv->default_visible_area.min_x + 1,
					drivers[nIndex]->drv->default_visible_area.max_y - drivers[nIndex]->drv->default_visible_area.min_y + 1,
					drivers[nIndex]->drv->frames_per_second);
	}
	return buf;
}

/* Build color information string */
static char *GameInfoColors(UINT nIndex)
{
	static char buf[1024];

	ZeroMemory(buf, sizeof(buf));
	if (drivers[nIndex]->drv->video_attributes & VIDEO_TYPE_VECTOR)
		strcpy(buf, "Vector Game");
	else
	{
		sprintf(buf, "%d colors ", drivers[nIndex]->drv->total_colors);
	}

	return buf;
}

/* Build game status string */
char *GameInfoStatus(UINT nIndex)
{
	switch (GetHasRoms(nIndex))
	{
	case 0:
		return "ROMs missing";

	case 1:
		if (drivers[nIndex]->flags & GAME_BROKEN)
			return "Not working";
		if (drivers[nIndex]->flags & GAME_WRONG_COLORS)
			return "Colors are totally wrong";
		if (drivers[nIndex]->flags & GAME_IMPERFECT_COLORS)
			return "Imperfect Colors";
		else
			return "Working";

	default:
	case 2:
		return "Unknown";
	}
}

/* Build game manufacturer string */
static char *GameInfoManufactured(UINT nIndex)
{
	static char buf[1024];

	sprintf(buf, "%s %s", drivers[nIndex]->year, drivers[nIndex]->manufacturer);
	return buf;
}

/* Build Game title string */
char *GameInfoTitle(UINT nIndex)
{
	static char buf[1024];

	if (nIndex == -1)
		strcpy(buf, "Global game options\nDefault options used by all games");
	else
		sprintf(buf, "%s\n\"%s\"", ModifyThe(drivers[nIndex]->description), drivers[nIndex]->name);
	return buf;
}

/* Build game clone infromation string */
static char *GameInfoCloneOf(UINT nIndex)
{
	static char buf[1024];

	buf[0] = '\0';

	if (drivers[nIndex]->clone_of != 0
	&&  !(drivers[nIndex]->clone_of->flags & NOT_A_DRIVER))
	{
		sprintf(buf, "%s - \"%s\"",
				ModifyThe(drivers[nIndex]->clone_of->description),
				drivers[nIndex]->clone_of->name);
	}

	return buf;
}

/* Handle the information property page */
static INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
#if defined(USE_SINGLELINE_TABCONTROL)
		{
			HWND hWnd = PropSheet_GetTabControl(GetParent(hDlg));
			DWORD tabStyle = (GetWindowLong(hWnd,GWL_STYLE) & ~TCS_MULTILINE);
			SetWindowLong(hWnd,GWL_STYLE,tabStyle | TCS_SINGLELINE);
		}
#endif

		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE),         GameInfoTitle(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_MANUFACTURED),  GameInfoManufactured(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_STATUS),        GameInfoStatus(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_CPU),           GameInfoCPU(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SOUND),         GameInfoSound(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SCREEN),        GameInfoScreen(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_COLORS),        GameInfoColors(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_CLONEOF),       GameInfoCloneOf(g_nGame));
		if (drivers[g_nGame]->clone_of != 0
		&& !(drivers[g_nGame]->clone_of->flags & NOT_A_DRIVER))
		{
			ShowWindow(GetDlgItem(hDlg, IDC_PROP_CLONEOF_TEXT), SW_SHOW);
		}
		else
		{
			ShowWindow(GetDlgItem(hDlg, IDC_PROP_CLONEOF_TEXT), SW_HIDE);
		}

		ShowWindow(hDlg, SW_SHOW);
		return 1;
	}
    return 0;
}

static BOOL ReadSkipCtrl(HWND hWnd, UINT nCtrlID, int *value)
{
	HWND hCtrl;
	char buf[100];
	int  nValue = *value;

	hCtrl = GetDlgItem(hWnd, nCtrlID);
	if (hCtrl)
	{
		/* Skip lines control */
		Edit_GetText(hCtrl, buf, 100);
		if (sscanf(buf, "%d", value) != 1)
			*value = 0;
	}

	return (nValue == *value) ? FALSE : TRUE;
}

/* Handle all options property pages */
static INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		/* Fill in the Game info at the top of the sheet */
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(g_nGame));
		InitializeOptions(hDlg);
		InitializeMisc(hDlg);

		PopulateControls(hDlg);
		OptionsToProp(hDlg, pGameOpts);
		SetPropEnabledControls(hDlg);
		if (g_nGame == -1)
			ShowWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), SW_HIDE);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);

		EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), g_bReset);
		ShowWindow(hDlg, SW_SHOW);

		return 1;

	case WM_HSCROLL:
		/* slider changed */
		HANDLE_WM_HSCROLL(hDlg, wParam, lParam, OptOnHScroll);
		g_bUseDefaults = FALSE;
		g_bReset = TRUE;
		EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), TRUE);
		PropSheet_Changed(GetParent(hDlg), hDlg);
		break;

	case WM_COMMAND:
		{
			/* Below, 'changed' is used to signify the 'Apply'
			 * button should be enabled.
			 */
			WORD wID         = GET_WM_COMMAND_ID(wParam, lParam);
			HWND hWndCtrl    = GET_WM_COMMAND_HWND(wParam, lParam);
			WORD wNotifyCode = GET_WM_COMMAND_CMD(wParam, lParam);
			BOOL changed     = FALSE;

			switch (wID)
			{
			case IDC_DEPTH:
			case IDC_SIZES:
			case IDC_FRAMESKIP:
			case IDC_EFFECT:
			case IDC_DEFAULT_INPUT:
			case IDC_ROTATE:
			case IDC_SAMPLERATE:
				if (wNotifyCode == CBN_SELCHANGE)
					changed = TRUE;
				break;

			case IDC_WINDOWED:
				changed = ReadControl(hDlg, wID);
				break;

			case IDC_RESDEPTH:
				if (wNotifyCode == LBN_SELCHANGE)
				{
					ResDepthSelectionChange(hDlg, hWndCtrl);
					changed = TRUE;
				}
				break;

			case IDC_REFRESH:
				if (wNotifyCode == LBN_SELCHANGE)
				{
					RefreshSelectionChange(hDlg, hWndCtrl);
					changed = TRUE;
				}
				break;

			case IDC_ASPECTRATION:
			case IDC_ASPECTRATIOD:
				if (wNotifyCode == EN_CHANGE)
				{
					if (g_bInternalSet == FALSE)
						changed = TRUE;
				}
				break;

			case IDC_TRIPLE_BUFFER:
				changed = ReadControl(hDlg, wID);
				break;

			case IDC_PROP_RESET:
				if (wNotifyCode != BN_CLICKED)
					break;

				memcpy(pGameOpts, &origGameOpts, sizeof(options_type));
				BuildDataMap();
				PopulateControls(hDlg);
				OptionsToProp(hDlg, pGameOpts);
				SetPropEnabledControls(hDlg);
				g_bReset = FALSE;
				PropSheet_UnChanged(GetParent(hDlg), hDlg);
				g_bUseDefaults = pGameOpts->use_default;
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				break;

			case IDC_USE_DEFAULT:
				if (g_nGame != -1)
				{
					pGameOpts->use_default = TRUE;
					pGameOpts = GetGameOptions(g_nGame);
					g_bUseDefaults = pGameOpts->use_default;
					BuildDataMap();
					PopulateControls(hDlg);
					OptionsToProp(hDlg, pGameOpts);
					SetPropEnabledControls(hDlg);
					if (origGameOpts.use_default != g_bUseDefaults)
					{
						PropSheet_Changed(GetParent(hDlg), hDlg);
						g_bReset = TRUE;
					}
					else
					{
						PropSheet_UnChanged(GetParent(hDlg), hDlg);
						g_bReset = FALSE;
					}
					EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				}

				break;
#ifdef MESS
			case IDC_DIR_BROWSE:
				if (wNotifyCode == BN_CLICKED)
					changed = SoftwareDirectories_OnInsertBrowse(hDlg, TRUE, NULL);
				break;

			case IDC_DIR_INSERT:
				if (wNotifyCode == BN_CLICKED)
					changed = SoftwareDirectories_OnInsertBrowse(hDlg, FALSE, NULL);
				break;

			case IDC_DIR_DELETE:
				if (wNotifyCode == BN_CLICKED)
					changed = SoftwareDirectories_OnDelete(hDlg);
				break;
#endif

			default:
				if (wNotifyCode == BN_CLICKED)
				{
					switch (wID)
					{
					case IDC_SCANLINES:
						if (Button_GetCheck(GetDlgItem(hDlg, IDC_SCANLINES)))
						{
							Button_SetCheck(GetDlgItem(hDlg, IDC_VSCANLINES), FALSE);
						}
						break;

					case IDC_VSCANLINES:
						if (Button_GetCheck(GetDlgItem(hDlg, IDC_VSCANLINES)))
						{
							Button_SetCheck(GetDlgItem(hDlg, IDC_SCANLINES), FALSE);
						}
						break;
					}
					changed = TRUE;
				}
			}

			/* Enable the apply button */
			if (changed == TRUE)
			{
				pGameOpts->use_default = g_bUseDefaults = FALSE;
				PropSheet_Changed(GetParent(hDlg), hDlg);
				g_bReset = TRUE;
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
			}
			SetPropEnabledControls(hDlg);
		}
		break;

	case WM_NOTIFY:
		switch (((NMHDR *) lParam)->code)
		{
		case PSN_SETACTIVE:
			/* Initialize the controls. */
			PopulateControls(hDlg);
			OptionsToProp(hDlg, pGameOpts);
			SetPropEnabledControls(hDlg);
			EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
			break;

		case PSN_APPLY:
			/* Save and apply the options here */
			PropToOptions(hDlg, pGameOpts);
			ReadControls(hDlg);
			pGameOpts->use_default = g_bUseDefaults;
			if (g_nGame == -1)
				pGameOpts = GetDefaultOptions();
			else
				pGameOpts = GetGameOptions(g_nGame);

			memcpy(&origGameOpts, pGameOpts, sizeof(options_type));
			BuildDataMap();
			PopulateControls(hDlg);
			OptionsToProp(hDlg, pGameOpts);
			SetPropEnabledControls(hDlg);
			EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
			g_bReset = FALSE;
			PropSheet_UnChanged(GetParent(hDlg), hDlg);
			SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
			break;

		case PSN_KILLACTIVE:
			/* Save Changes to the options here. */
			ReadControls(hDlg);
			ResetDataMap();
			pGameOpts->use_default = g_bUseDefaults;
			PropToOptions(hDlg, pGameOpts);
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			return 1;

		case PSN_RESET:
			/* Reset to the original values. Disregard changes */
			memcpy(pGameOpts, &origGameOpts, sizeof(options_type));
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			break;

		case PSN_HELP:
			/* User wants help for this property page */
			break;

#ifdef MESS
        case LVN_ENDLABELEDIT:
            return SoftwareDirectories_OnEndLabelEdit(hDlg, (NMHDR *) lParam);

        case LVN_BEGINLABELEDIT:
			return SoftwareDirectories_OnBeginLabelEdit(hDlg, (NMHDR *) lParam);
#endif
		}
		break;

#if HAS_HELP
	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		Help_HtmlHelp(((LPHELPINFO)lParam)->hItemHandle, MAME32HELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		Help_HtmlHelp((HWND)wParam, MAME32HELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;
#endif /* HAS_HELP */
	}
	EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), g_bReset);

	return 0;
}

static INT_PTR CALLBACK GameDisplayOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		{
		}
	}

	return GameOptionsProc(hDlg, Msg, wParam, lParam);
}

/* Read controls that are not handled in the DataMap */
static void PropToOptions(HWND hWnd, options_type *o)
{
	char buf[100];
	HWND hCtrl;
	HWND hCtrl2;
	int  nIndex;

	o->use_default = g_bUseDefaults;

	/* resolution size */
	hCtrl = GetDlgItem(hWnd, IDC_SIZES);
	if (hCtrl)
	{
		/* Screen size control */
		nIndex = ComboBox_GetCurSel(hCtrl);

		if (nIndex == 0)
			strcpy(o->resolution, "0x0"); /* auto */
		else
		{
			int w, h;

			ComboBox_GetText(hCtrl, buf, 100);
			if (sscanf(buf, "%d x %d", &w, &h) == 2)
			{
				sprintf(o->resolution, "%dx%d", w, h);
			}
			else
			{
				strcpy(o->resolution, "0x0"); /* auto */
			}
		}

		/* resolution depth */
		hCtrl = GetDlgItem(hWnd, IDC_RESDEPTH);
		if (hCtrl)
		{
			int nResDepth = 0;

			nIndex = ComboBox_GetCurSel(hCtrl);
			if (nIndex != CB_ERR)
				nResDepth = ComboBox_GetItemData(hCtrl, nIndex);

			switch (nResDepth)
			{
			default:
			case 0:  strcat(o->resolution, "x0"); break;
			case 16: strcat(o->resolution, "x16"); break;
			case 24: strcat(o->resolution, "x24"); break;
			case 32: strcat(o->resolution, "x32"); break;
			}
		}
	}

	/* refresh */
	hCtrl = GetDlgItem(hWnd, IDC_REFRESH);
	if (hCtrl)
	{
		nIndex = ComboBox_GetCurSel(hCtrl);

		pGameOpts->gfx_refresh = ComboBox_GetItemData(hCtrl, nIndex);
	}

	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	if (hCtrl && hCtrl2)
	{
		int n = 0;
		int d = 0;

		Edit_GetText(hCtrl, buf, sizeof(buf));
		sscanf(buf, "%d", &n);

		Edit_GetText(hCtrl2, buf, sizeof(buf));
		sscanf(buf, "%d", &d);

		if (n == 0 || d == 0)
		{
			n = 4;
			d = 3;
		}

		sprintf(o->aspect, "%d:%d", n, d);
	}
#ifdef MESS
	SoftwareDirectories_GetList(hWnd, pGameOpts->extra_software_paths, sizeof(pGameOpts->extra_software_paths) / sizeof(pGameOpts->extra_software_paths[0]));
#endif
}

/* Populate controls that are not handled in the DataMap */
static void OptionsToProp(HWND hWnd, options_type* o)
{
	HWND hCtrl;
	HWND hCtrl2;
	char buf[100];
	int  h = 0;
	int  w = 0;
	int  n = 0;
	int  d = 0;

	g_bInternalSet = TRUE;

	/* video */

	/* get desired resolution */
	if (!stricmp(o->resolution, "auto"))
	{
		w = h = 0;
	}
	else
	if (sscanf(o->resolution, "%dx%dx%d", &w, &h, &d) < 2)
	{
		w = h = d = 0;
	}

	/* Setup sizes list based on depth. */
	UpdateDisplayModeUI(hWnd, d, o->gfx_refresh);

	/* Screen size drop down list */
	hCtrl = GetDlgItem(hWnd, IDC_SIZES);
	if (hCtrl)
	{
		if (w == 0 && h == 0)
		{
			/* default to auto */
			ComboBox_SetCurSel(hCtrl, 0);
		}
		else
		{
			/* Select the mode in the list. */
			int nSelection = 0;
			int nCount = 0;

			/* Get the number of items in the control */
			nCount = ComboBox_GetCount(hCtrl);

			while (0 < nCount--)
			{
				int nWidth, nHeight;

				/* Get the screen size */
				ComboBox_GetLBText(hCtrl, nCount, buf);

				if (sscanf(buf, "%d x %d", &nWidth, &nHeight) == 2)
				{
					/* If we match, set nSelection to the right value */
					if (w == nWidth
					&&  h == nHeight)
					{
						nSelection = nCount;
						break;
					}
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
	}

	/* Screen depth drop down list */
	hCtrl = GetDlgItem(hWnd, IDC_RESDEPTH);
	if (hCtrl)
	{
		if (d == 0)
		{
			/* default to auto */
			ComboBox_SetCurSel(hCtrl, 0);
		}
		else
		{
			/* Select the mode in the list. */
			int nSelection = 0;
			int nCount = 0;

			/* Get the number of items in the control */
			nCount = ComboBox_GetCount(hCtrl);

			while (0 < nCount--)
			{
				int nDepth;

				/* Get the screen depth */
				nDepth = ComboBox_GetItemData(hCtrl, nCount);

				/* If we match, set nSelection to the right value */
				if (d == nDepth)
				{
					nSelection = nCount;
					break;
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
	}

	/* Screen refresh list */
	hCtrl = GetDlgItem(hWnd, IDC_REFRESH);
	if (hCtrl)
	{
		if (o->gfx_refresh == 0)
		{
			/* default to auto */
			ComboBox_SetCurSel(hCtrl, 0);
		}
		else
		{
			/* Select the mode in the list. */
			int nSelection = 0;
			int nCount = 0;

			/* Get the number of items in the control */
			nCount = ComboBox_GetCount(hCtrl);

			while (0 < nCount--)
			{
				int nRefresh;

				/* Get the screen depth */
				nRefresh = ComboBox_GetItemData(hCtrl, nCount);

				/* If we match, set nSelection to the right value */
				if (o->gfx_refresh == nRefresh)
				{
					nSelection = nCount;
					break;
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
	}

	hCtrl = GetDlgItem(hWnd, IDC_BRIGHTNESSDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.02f", o->gfx_brightness);
		Static_SetText(hCtrl, buf);
	}

	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	if (hCtrl && hCtrl2)
	{
		n = 0;
		d = 0;

		if (sscanf(o->aspect, "%d:%d", &n, &d) == 2 && n != 0 && d != 0)
		{
			sprintf(buf, "%d", n);
			Edit_SetText(hCtrl, buf);
			sprintf(buf, "%d", d);
			Edit_SetText(hCtrl2, buf);
		}
		else
		{
			Edit_SetText(hCtrl,  "4");
			Edit_SetText(hCtrl2, "3");
		}
	}

	/* core video */
	hCtrl = GetDlgItem(hWnd, IDC_GAMMADISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.02f", o->gamma_correct);
		Static_SetText(hCtrl, buf);
	}

	/* vector */
	hCtrl = GetDlgItem(hWnd, IDC_BEAMDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.02f", o->f_beam);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_FLICKERDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.02f", o->f_flicker);
		Static_SetText(hCtrl, buf);
	}

	/* sound */
	hCtrl = GetDlgItem(hWnd, IDC_VOLUMEDISP);
	if (hCtrl)
	{
		sprintf(buf, "%ddB", o->attenuation);
		Static_SetText(hCtrl, buf);
	}

#ifdef MESS
	SoftwareDirectories_InitList(hWnd, pGameOpts->extra_software_paths);
#endif

	g_bInternalSet = FALSE;
}

/* Adjust controls - tune them to the currently selected game */
static void SetPropEnabledControls(HWND hWnd)
{
	HWND hCtrl;
	int  nIndex;
	int  sound;
	int  ddraw = 0;

	nIndex = g_nGame;

	hCtrl = GetDlgItem(hWnd, IDC_DDRAW);
	ddraw = Button_GetCheck(hCtrl);

	EnableWindow(GetDlgItem(hWnd, IDC_WAITVSYNC),       ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_TRIPLE_BUFFER),   ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_HWSTRETCH),       ddraw && DirectDraw_HasHWStretch());
	EnableWindow(GetDlgItem(hWnd, IDC_SWITCHRES),       ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_SWITCHBPP),       ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_MATCHREFRESH),    ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_SYNCREFRESH),     ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_REFRESH),         ddraw && DirectDraw_HasRefresh());
	EnableWindow(GetDlgItem(hWnd, IDC_REFRESHTEXT),     ddraw && DirectDraw_HasRefresh());
	EnableWindow(GetDlgItem(hWnd, IDC_BRIGHTNESS),      ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_BRIGHTNESSTEXT),  ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_BRIGHTNESSDISP),  ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOTEXT), ddraw && DirectDraw_HasHWStretch() && Button_GetCheck(GetDlgItem(hWnd, IDC_HWSTRETCH)));
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATION),    ddraw && DirectDraw_HasHWStretch() && Button_GetCheck(GetDlgItem(hWnd, IDC_HWSTRETCH)));
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOD),    ddraw && DirectDraw_HasHWStretch() && Button_GetCheck(GetDlgItem(hWnd, IDC_HWSTRETCH)));

	hCtrl = GetDlgItem(hWnd, IDC_JOYSTICK);
	if (hCtrl)
		Button_Enable(hCtrl, DIJoystick.Available());

	/* Trackball / Mouse options */
	if (nIndex == -1 || GameUsesTrackball(nIndex))
	{
		Button_Enable(GetDlgItem(hWnd, IDC_USE_MOUSE), TRUE);
	}
	else
	{
		Button_Enable(GetDlgItem(hWnd, IDC_USE_MOUSE), FALSE);
	}

	/* Sound options */
	hCtrl = GetDlgItem(hWnd, IDC_USE_SOUND);
	if (hCtrl)
	{
		sound = Button_GetCheck(hCtrl);
		ComboBox_Enable(GetDlgItem(hWnd, IDC_SAMPLERATE), (sound != 0));

		EnableWindow(GetDlgItem(hWnd, IDC_VOLUME),     (sound != 0));
		EnableWindow(GetDlgItem(hWnd, IDC_RATETEXT),   (sound != 0));
		EnableWindow(GetDlgItem(hWnd, IDC_USE_FILTER), (sound != 0));
		EnableWindow(GetDlgItem(hWnd, IDC_VOLUMEDISP), (sound != 0));
		EnableWindow(GetDlgItem(hWnd, IDC_VOLUMETEXT), (sound != 0));
		SetSamplesEnabled(hWnd, nIndex, sound);
		SetStereoEnabled(hWnd, nIndex);
		SetYM3812Enabled(hWnd, nIndex);
	}

	/* Dirty rectangles */
	if (nIndex == -1
	||  (drivers[nIndex]->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
	||  (drivers[nIndex]->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		Button_Enable(GetDlgItem(hWnd, IDC_DIRTY), TRUE);
	}
	else
	{
		Button_Enable(  GetDlgItem(hWnd, IDC_DIRTY), FALSE);
		Button_SetCheck(GetDlgItem(hWnd, IDC_DIRTY), FALSE);
	}

	if (Button_GetCheck(GetDlgItem(hWnd, IDC_AUTOFRAMESKIP)))
		EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), FALSE);
	else
		EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), TRUE);
}

/**************************************************************
 * Control Helper functions for data exchange
 **************************************************************/

static void AssignSampleRate(HWND hWnd)
{
	switch (g_nSampleRateIndex)
	{
		case 0:  pGameOpts->samplerate = 11025; break;
		case 1:  pGameOpts->samplerate = 22050; break;
		case 2:  pGameOpts->samplerate = 44100; break;
		default: pGameOpts->samplerate = 44100; break;
	}
}

static void AssignVolume(HWND hWnd)
{
	pGameOpts->attenuation = g_nVolumeIndex - 32;
}

static void AssignDepth(HWND hWnd)
{
	switch (g_nDepthIndex)
	{
		default:
		case 0:  pGameOpts->color_depth =  0; break;
		case 1:  pGameOpts->color_depth = 15; break;
		case 2:  pGameOpts->color_depth = 16; break;
		case 3:  pGameOpts->color_depth = 32; break;
	}
}

static void AssignGamma(HWND hWnd)
{
	pGameOpts->gamma_correct = g_nGammaIndex / 20.0 + 0.5;
}

static void AssignBrightness(HWND hWnd)
{
	pGameOpts->gfx_brightness = g_nBrightnessIndex / 20.0 + 0.1;
}

static void AssignBeam(HWND hWnd)
{
	pGameOpts->f_beam = g_nBeamIndex / 20.0 + 1.0;
}

static void AssignFlicker(HWND hWnd)
{
	pGameOpts->f_flicker = g_nFlickerIndex;
}

static void AssignRotate(HWND hWnd)
{
	pGameOpts->ror = 0;
	pGameOpts->rol = 0;

	switch (g_nRotateIndex)
	{
		case 1:  pGameOpts->ror = 1; break;
		case 2:  pGameOpts->rol = 1; break;
		default: break;
	}
}

static void AssignInput(HWND hWnd)
{
	pGameOpts->hotrod   = 0;
	pGameOpts->hotrodse = 0;

	switch (g_nInputIndex)
	{
		case 1:  pGameOpts->hotrod   = 1; break;
		case 2:  pGameOpts->hotrodse = 1; break;
		default: break;
	}
}

static void AssignEffect(HWND hWnd)
{
	const char* pData = (const char*)ComboBox_GetItemData(hWnd, g_nEffectIndex);
	if (pData != NULL)
		strcpy(pGameOpts->effect, pData);
}

/************************************************************
 * DataMap initializers
 ************************************************************/

/* Initialize local helper variables */
static void ResetDataMap(void)
{
	int i;
	g_nGammaIndex      = (int)((pGameOpts->gamma_correct  - 0.5) * 20.0);
	g_nBrightnessIndex = (int)((pGameOpts->gfx_brightness - 0.1) * 20.0);
	g_nBeamIndex       = (int)((pGameOpts->f_beam         - 1.0) * 20.0);
	g_nFlickerIndex    = (int)(pGameOpts->f_flicker);

	if (pGameOpts->hotrod == 0 && pGameOpts->hotrodse == 0)
		g_nInputIndex = 0;
	else
	if (pGameOpts->hotrod == 1 && pGameOpts->hotrodse == 0)
		g_nInputIndex = 1;
	else
	if (pGameOpts->hotrod == 0 && pGameOpts->hotrodse == 1)
		g_nInputIndex = 2;
	else
		g_nInputIndex = 0;

	if (pGameOpts->ror == 0 && pGameOpts->rol == 0)
		g_nRotateIndex = 0;
	else
	if (pGameOpts->ror == 1 && pGameOpts->rol == 0)
		g_nRotateIndex = 1;
	else
	if (pGameOpts->ror == 0 && pGameOpts->rol == 1)
		g_nRotateIndex = 2;
	else
		g_nRotateIndex = 0;

	g_nVolumeIndex = pGameOpts->attenuation + 32;
	switch (pGameOpts->samplerate)
	{
		case 11025:  g_nSampleRateIndex = 0; break;
		case 22050:  g_nSampleRateIndex = 1; break;
		default:
		case 44100:  g_nSampleRateIndex = 2; break;
	}

	switch (pGameOpts->color_depth)
	{
		default:
		case 0:  g_nDepthIndex = 0; break;
		case 15: g_nDepthIndex = 1; break;
		case 16: g_nDepthIndex = 2; break;
		case 32: g_nDepthIndex = 3; break;
	}

	g_nEffectIndex = 0;
	for (i = 0; i < NUMEFFECTS; i++)
	{
		if (!stricmp(pGameOpts->effect, g_ComboBoxEffect[i].m_pData))
			g_nEffectIndex = i;
	}
}

/* Build the control mapping by adding all needed information to the DataMap */
static void BuildDataMap(void)
{
	InitDataMap();

	ResetDataMap();

	/* video */
	DataMapAdd(IDC_AUTOFRAMESKIP, DM_BOOL, CT_BUTTON,   &pGameOpts->autoframeskip, 0, 0, 0);
	DataMapAdd(IDC_FRAMESKIP,     DM_INT,  CT_COMBOBOX, &pGameOpts->frameskip,     0, 0, 0);
	DataMapAdd(IDC_WAITVSYNC,     DM_BOOL, CT_BUTTON,   &pGameOpts->wait_vsync,    0, 0, 0);
	DataMapAdd(IDC_TRIPLE_BUFFER, DM_BOOL, CT_BUTTON,   &pGameOpts->use_triplebuf, 0, 0, 0);
	DataMapAdd(IDC_WINDOWED,      DM_BOOL, CT_BUTTON,   &pGameOpts->window_mode,   0, 0, 0);
	DataMapAdd(IDC_DDRAW,         DM_BOOL, CT_BUTTON,   &pGameOpts->use_ddraw,     0, 0, 0);
	DataMapAdd(IDC_HWSTRETCH,     DM_BOOL, CT_BUTTON,   &pGameOpts->ddraw_stretch, 0, 0, 0);
	/* pGameOpts->resolution */
	/* pGameOpts->gfx_refresh */
	DataMapAdd(IDC_SCANLINES,     DM_BOOL, CT_BUTTON,   &pGameOpts->scanlines,     0, 0, 0);
	DataMapAdd(IDC_SWITCHRES,     DM_BOOL, CT_BUTTON,   &pGameOpts->switchres,     0, 0, 0);
	DataMapAdd(IDC_SWITCHBPP,     DM_BOOL, CT_BUTTON,   &pGameOpts->switchbpp,     0, 0, 0);
	DataMapAdd(IDC_MAXIMIZE,      DM_BOOL, CT_BUTTON,   &pGameOpts->maximize,      0, 0, 0);
	DataMapAdd(IDC_KEEPASPECT,    DM_BOOL, CT_BUTTON,   &pGameOpts->keepaspect,    0, 0, 0);
	DataMapAdd(IDC_MATCHREFRESH,  DM_BOOL, CT_BUTTON,   &pGameOpts->matchrefresh,  0, 0, 0);
	DataMapAdd(IDC_SYNCREFRESH,   DM_BOOL, CT_BUTTON,   &pGameOpts->syncrefresh,   0, 0, 0);
	DataMapAdd(IDC_DIRTY,         DM_BOOL, CT_BUTTON,   &pGameOpts->use_dirty,     0, 0, 0);
	DataMapAdd(IDC_THROTTLE,      DM_BOOL, CT_BUTTON,   &pGameOpts->throttle,      0, 0, 0);
	DataMapAdd(IDC_BRIGHTNESS,    DM_INT,  CT_SLIDER,   &g_nBrightnessIndex, 0, 0, AssignBrightness);
	/* pGameOpts->frames_to_display */
	DataMapAdd(IDC_EFFECT,        DM_INT,  CT_COMBOBOX, &g_nEffectIndex, 0, 0, AssignEffect);
	/* pGameOpts->aspect */

	/* input */
	DataMapAdd(IDC_DEFAULT_INPUT, DM_INT,  CT_COMBOBOX, &g_nInputIndex, 0, 0, AssignInput);
	DataMapAdd(IDC_USE_MOUSE,     DM_BOOL, CT_BUTTON,   &pGameOpts->use_mouse,     0, 0, 0);
	DataMapAdd(IDC_JOYSTICK,      DM_BOOL, CT_BUTTON,   &pGameOpts->use_joystick,  0, 0, 0);
	DataMapAdd(IDC_STEADYKEY,     DM_BOOL, CT_BUTTON,   &pGameOpts->steadykey,     0, 0, 0);

	/* core video */
	DataMapAdd(IDC_DEPTH,         DM_INT,  CT_COMBOBOX, &g_nDepthIndex, 0, 0, AssignDepth);
	DataMapAdd(IDC_NOROTATE,      DM_BOOL, CT_BUTTON,   &pGameOpts->norotate,      0, 0, 0);
	DataMapAdd(IDC_ROTATE,        DM_INT,  CT_COMBOBOX, &g_nRotateIndex, 0, 0, AssignRotate);
	DataMapAdd(IDC_FLIPX,         DM_BOOL, CT_BUTTON,   &pGameOpts->flipx,         0, 0, 0);
	DataMapAdd(IDC_FLIPY,         DM_BOOL, CT_BUTTON,   &pGameOpts->flipy,         0, 0, 0);
	/* debugres */
	DataMapAdd(IDC_GAMMA,         DM_INT,  CT_SLIDER,   &g_nGammaIndex, 0, 0, AssignGamma);

	/* vector */
	DataMapAdd(IDC_ANTIALIAS,     DM_BOOL, CT_BUTTON,   &pGameOpts->antialias,     0, 0, 0);
	DataMapAdd(IDC_TRANSLUCENCY,  DM_BOOL, CT_BUTTON,   &pGameOpts->translucency,  0, 0, 0);
	DataMapAdd(IDC_BEAM,          DM_INT,  CT_SLIDER,   &g_nBeamIndex, 0, 0, AssignBeam);
	DataMapAdd(IDC_FLICKER,       DM_INT,  CT_SLIDER,   &g_nFlickerIndex, 0, 0, AssignFlicker);

	/* sound */
	DataMapAdd(IDC_SAMPLERATE,    DM_INT,  CT_COMBOBOX, &g_nSampleRateIndex, 0, 0, AssignSampleRate);
	DataMapAdd(IDC_SAMPLES,       DM_BOOL, CT_BUTTON,   &pGameOpts->use_samples,   0, 0, 0);
	DataMapAdd(IDC_USE_FILTER,    DM_BOOL, CT_BUTTON,   &pGameOpts->use_filter,    0, 0, 0);
	DataMapAdd(IDC_USE_SOUND,     DM_BOOL, CT_BUTTON,   &pGameOpts->enable_sound,  0, 0, 0);
	DataMapAdd(IDC_VOLUME,        DM_INT,  CT_SLIDER,   &g_nVolumeIndex, 0, 0, AssignVolume);

	/* misc */
	DataMapAdd(IDC_ARTWORK,       DM_BOOL, CT_BUTTON,   &pGameOpts->use_artwork,   0, 0, 0);
	DataMapAdd(IDC_CHEAT,         DM_BOOL, CT_BUTTON,   &pGameOpts->cheat,         0, 0, 0);
/*	DataMapAdd(IDC_DEBUG,         DM_BOOL, CT_BUTTON,   &pGameOpts->mame_debug,    0, 0, 0);*/
	DataMapAdd(IDC_LOG,           DM_BOOL, CT_BUTTON,   &pGameOpts->errorlog,      0, 0, 0);
#ifdef MESS
	DataMapAdd(IDC_NEW_FILEMGR,   DM_BOOL, CT_BUTTON,   &pGameOpts->use_new_filemgr, 0, 0, 0);
#endif
}

static void SetStereoEnabled(HWND hWnd, int the_index)
{
	BOOL enabled = FALSE;
	HWND hCtrl;

	hCtrl = GetDlgItem(hWnd, IDC_STEREO);
	if (hCtrl)
	{
		if (the_index == -1 || drivers[the_index]->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
			enabled = TRUE;

		EnableWindow(hCtrl, enabled);
	}
}

static void SetYM3812Enabled(HWND hWnd, int the_index)
{
	int i;
	BOOL enabled;
	HWND hCtrl;

	hCtrl = GetDlgItem(hWnd, IDC_USE_FM_YM3812);
	if (hCtrl)
	{
		enabled = FALSE;
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (the_index == -1
#if HAS_YM3812
			||  drivers[the_index]->drv->sound[i].sound_type == SOUND_YM3812
#endif
#if HAS_YM3526
			||  drivers[the_index]->drv->sound[i].sound_type == SOUND_YM3526
#endif
#if HAS_YM2413
			||  drivers[the_index]->drv->sound[i].sound_type == SOUND_YM2413
#endif
			)
				enabled = TRUE;
		}

		EnableWindow(hCtrl, enabled);
	}
}

static void SetSamplesEnabled(HWND hWnd, int the_index, BOOL bSoundEnabled)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)
	int i;
	BOOL enabled = FALSE;
	HWND hCtrl;

	hCtrl = GetDlgItem(hWnd, IDC_SAMPLES);
	if (hCtrl)
	{
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (the_index == -1
			||  drivers[the_index]->drv->sound[i].sound_type == SOUND_SAMPLES
#if HAS_VLM5030
			||  drivers[the_index]->drv->sound[i].sound_type == SOUND_VLM5030
#endif
			)
				enabled = TRUE;
		}

		enabled = enabled && bSoundEnabled;
		EnableWindow(hCtrl, enabled);
	}
#endif
}

/* Moved here cause it's called in a few places */
static void InitializeOptions(HWND hDlg)
{
	InitializeDepthUI(hDlg);
	InitializeResDepthUI(hDlg);
	InitializeRefreshUI(hDlg);
	InitializeDisplayModeUI(hDlg);
	InitializeSoundUI(hDlg);
	InitializeSkippingUI(hDlg);
	InitializeRotateUI(hDlg);
	InitializeDefaultInputUI(hDlg);
	InitializeEffectUI(hDlg);
}

/* Moved here because it is called in several places */
static void InitializeMisc(HWND hDlg)
{
	Button_Enable(GetDlgItem(hDlg, IDC_JOYSTICK), DIJoystick.Available());

	SendMessage(GetDlgItem(hDlg, IDC_GAMMA), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 30)); /* [0.50, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_BRIGHTNESS), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 38)); /* [0.10, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_FLICKER), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); /* [0.0, 100.0] in 1.0 increments */

	SendMessage(GetDlgItem(hDlg, IDC_BEAM), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 300)); /* [1.00, 16.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 32)); /* [-32, 0] */
}

static void OptOnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	if (hwndCtl == GetDlgItem(hwnd, IDC_FLICKER))
	{
		FlickerSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_GAMMA))
	{
		GammaSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_BRIGHTNESS))
	{
		BrightnessSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_BEAM))
	{
		BeamSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_FLICKER))
	{
		FlickerSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_VOLUME))
	{
		VolumeSelectionChange(hwnd);
	}
}

/* Handle changes to the Beam slider */
static void BeamSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dBeam;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_BEAM), TBM_GETPOS, 0, 0);

	dBeam = nValue / 20.0 + 1.0;

	/* Set the static display to the new value */
	sprintf(buf, "%03.02f", dBeam);
	Static_SetText(GetDlgItem(hwnd, IDC_BEAMDISP), buf);
}

/* Handle changes to the Flicker slider */
static void FlickerSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dFlicker;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_FLICKER), TBM_GETPOS, 0, 0);

	dFlicker = nValue;

	/* Set the static display to the new value */
	sprintf(buf, "%03.02f", dFlicker);
	Static_SetText(GetDlgItem(hwnd, IDC_FLICKERDISP), buf);
}

/* Handle changes to the Gamma slider */
static void GammaSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dGamma;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_GAMMA), TBM_GETPOS, 0, 0);

	dGamma = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	sprintf(buf, "%03.02f", dGamma);
	Static_SetText(GetDlgItem(hwnd, IDC_GAMMADISP), buf);
}

/* Handle changes to the Brightness slider */
static void BrightnessSelectionChange(HWND hwnd)
{
	char   buf[100];
	int    nValue;
	double dBrightness;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_BRIGHTNESS), TBM_GETPOS, 0, 0);

	dBrightness = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	sprintf(buf, "%03.02f", dBrightness);
	Static_SetText(GetDlgItem(hwnd, IDC_BRIGHTNESSDISP), buf);
}

/* Handle changes to the Color Depth drop down */
static void ResDepthSelectionChange(HWND hWnd, HWND hWndCtrl)
{
	int nCurSelection;

	nCurSelection = ComboBox_GetCurSel(hWndCtrl);
	if (nCurSelection != CB_ERR)
	{
		HWND hRefreshCtrl;
		int nResDepth = 0;
		int nRefresh  = 0;

		nResDepth = ComboBox_GetItemData(hWndCtrl, nCurSelection);

		hRefreshCtrl = GetDlgItem(hWnd, IDC_REFRESH);
		if (hRefreshCtrl)
		{
			nCurSelection = ComboBox_GetCurSel(hRefreshCtrl);
			if (nCurSelection != CB_ERR)
				nRefresh = ComboBox_GetItemData(hRefreshCtrl, nCurSelection);
		}

		UpdateDisplayModeUI(hWnd, nResDepth, nRefresh);
	}
}

/* Handle changes to the Refresh drop down */
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl)
{
	int nCurSelection;

	nCurSelection = ComboBox_GetCurSel(hWndCtrl);
	if (nCurSelection != CB_ERR)
	{
		HWND hResDepthCtrl;
		int nResDepth = 0;
		int nRefresh  = 0;

		nRefresh = ComboBox_GetItemData(hWndCtrl, nCurSelection);

		hResDepthCtrl = GetDlgItem(hWnd, IDC_RESDEPTH);
		if (hResDepthCtrl)
		{
			nCurSelection = ComboBox_GetCurSel(hResDepthCtrl);
			if (nCurSelection != CB_ERR)
				nResDepth = ComboBox_GetItemData(hResDepthCtrl, nCurSelection);
		}

		UpdateDisplayModeUI(hWnd, nResDepth, nRefresh);
	}
}

/* Handle changes to the Volume slider */
static void VolumeSelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%ddB", nValue - 32);
	Static_SetText(GetDlgItem(hwnd, IDC_VOLUMEDISP), buf);
}

/* Adjust possible choices in the Screen Size drop down */
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwDepth, DWORD dwRefresh)
{
	int                   i;
	char                  buf[100];
	struct tDisplayModes* pDisplayModes;
	int                   nPick;
	int                   nCount = 0;
	int                   nSelection = 0;
	DWORD                 w = 0, h = 0;
	HWND                  hCtrl = GetDlgItem(hwnd, IDC_SIZES);

	if (!hCtrl)
		return;

	/* Find out what is currently selected if anything. */
	nPick = ComboBox_GetCurSel(hCtrl);
	if (nPick != 0 && nPick != CB_ERR)
	{
		ComboBox_GetText(GetDlgItem(hwnd, IDC_SIZES), buf, 100);
		if (sscanf(buf, "%ld x %ld", &w, &h) != 2)
		{
			w = 0;
			h = 0;
		}
	}

	/* Remove all items in the list. */
	ComboBox_ResetContent(hCtrl);

	ComboBox_AddString(hCtrl, "Auto");

	pDisplayModes = DirectDraw_GetDisplayModes();

	for (i = 0; i < pDisplayModes->m_nNumModes; i++)
	{
		if ((pDisplayModes->m_Modes[i].m_dwBPP     == dwDepth   || dwDepth   == 0)
		&&  (pDisplayModes->m_Modes[i].m_dwRefresh == dwRefresh || dwRefresh == 0))
		{
			sprintf(buf, "%li x %li", pDisplayModes->m_Modes[i].m_dwWidth,
									  pDisplayModes->m_Modes[i].m_dwHeight);

			if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
			{
				ComboBox_AddString(hCtrl, buf);
				nCount++;

				if (w == pDisplayModes->m_Modes[i].m_dwWidth
				&&  h == pDisplayModes->m_Modes[i].m_dwHeight)
					nSelection = nCount;
			}
		}
	}

	ComboBox_SetCurSel(hCtrl, nSelection);
}

/* Initialize the Display options to auto mode */
static void InitializeDisplayModeUI(HWND hwnd)
{
	UpdateDisplayModeUI(hwnd, 0, 0);
}

/* Initialize the sound options */
static void InitializeSoundUI(HWND hwnd)
{
	HWND    hCtrl;

	hCtrl = GetDlgItem(hwnd, IDC_SAMPLERATE);
	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "11025");
		ComboBox_AddString(hCtrl, "22050");
		ComboBox_AddString(hCtrl, "44100");
		ComboBox_SetCurSel(hCtrl, 1);
	}
}

/* Populate the Frame Skipping drop down */
static void InitializeSkippingUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_FRAMESKIP);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Draw every frame");
		ComboBox_AddString(hCtrl, "Skip 1 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 2 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 3 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 4 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 5 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 6 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 7 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 8 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 9 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 10 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 11 of 12 frames");
	}
}

/* Populate the Rotate drop down */
static void InitializeRotateUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_ROTATE);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "None");           /* 0 */
		ComboBox_AddString(hCtrl, "Clockwise");      /* 1 */
		ComboBox_AddString(hCtrl, "Anti-clockwise"); /* 2 */
	}
}

/* Populate the Color depth drop down */
static void InitializeDepthUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_DEPTH);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Auto");
		ComboBox_AddString(hCtrl, "15 bit");
		ComboBox_AddString(hCtrl, "16 bit");
		ComboBox_AddString(hCtrl, "32 bit");

		ComboBox_SetItemData(hCtrl, 0,  0);
		ComboBox_SetItemData(hCtrl, 1, 15);
		ComboBox_SetItemData(hCtrl, 2, 16);
		ComboBox_SetItemData(hCtrl, 3, 32);
	}
}

/* Populate the resolution depth drop down */
static void InitializeResDepthUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_RESDEPTH);

	if (hCtrl)
	{
		struct tDisplayModes* pDisplayModes;
		int nCount = 0;
		int i;

		/* Remove all items in the list. */
		ComboBox_ResetContent(hCtrl);

		ComboBox_AddString(hCtrl, "Auto");
		ComboBox_SetItemData(hCtrl, nCount++, 0);

		pDisplayModes = DirectDraw_GetDisplayModes();

		for (i = 0; i < pDisplayModes->m_nNumModes; i++)
		{
			if (pDisplayModes->m_Modes[i].m_dwBPP == 16
			||  pDisplayModes->m_Modes[i].m_dwBPP == 24
			||  pDisplayModes->m_Modes[i].m_dwBPP == 32)
			{
				char buf[16];

				sprintf(buf, "%li bit", pDisplayModes->m_Modes[i].m_dwBPP);

				if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
				{
					ComboBox_InsertString(hCtrl, nCount, buf);
					ComboBox_SetItemData(hCtrl, nCount++, pDisplayModes->m_Modes[i].m_dwBPP);
				}
			}
		}
	}
}

/* Populate the refresh drop down */
static void InitializeRefreshUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_REFRESH);

	if (hCtrl)
	{
		struct tDisplayModes* pDisplayModes;
		int nCount = 0;
		int i;

		/* Remove all items in the list. */
		ComboBox_ResetContent(hCtrl);

		ComboBox_AddString(hCtrl, "Auto");
		ComboBox_SetItemData(hCtrl, nCount++, 0);

		pDisplayModes = DirectDraw_GetDisplayModes();

		for (i = 0; i < pDisplayModes->m_nNumModes; i++)
		{
			if (pDisplayModes->m_Modes[i].m_dwRefresh != 0)
			{
				char buf[16];

				sprintf(buf, "%li Hz", pDisplayModes->m_Modes[i].m_dwRefresh);

				if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
				{
					ComboBox_InsertString(hCtrl, nCount, buf);
					ComboBox_SetItemData(hCtrl, nCount++, pDisplayModes->m_Modes[i].m_dwRefresh);
				}
			}
		}
	}
}

/* Populate the Default Input drop down */
static void InitializeDefaultInputUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_DEFAULT_INPUT);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Standard");
		ComboBox_AddString(hCtrl, "HotRod");
		ComboBox_AddString(hCtrl, "HotRod SE");
	}
}

static void InitializeEffectUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_EFFECT);

	if (hCtrl)
	{
		int i;
		for (i = 0; i < NUMEFFECTS; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxEffect[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxEffect[i].m_pData);
		}
	}
}

/**************************************************************************
    Game History functions
 **************************************************************************/

/* Load indexes from history.dat if found */
char * GameHistory(int game_index)
{
	historyBuf[0] = '\0';

	if (load_driver_history(drivers[game_index], historyBuf, sizeof(historyBuf)) == 0)
		HistoryFixBuf(historyBuf);

	return historyBuf;
}

static void HistoryFixBuf(char *buf)
{
	char *s  = tempHistoryBuf;
	char *p  = buf;
	int  len = 0;

	if (strlen(buf) < 3)
	{
		*buf = '\0';
		return;
	}

	while (*p && len < MAX_HISTORY_LEN - 1)
	{
		if (*p == '\n')
		{
			*s++ = '\r';
			len++;
		}

		*s++ = *p++;
		len++;
	}

	*s++ = '\0';
	strcpy(buf, tempHistoryBuf);
}

/* End of source file */
