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

#ifdef __GNUC__
#undef bool
#endif

#define WIN32_LEAN_AND_MEAN
#define NONAMELESSUNION 1
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#undef NONAMELESSUNION
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

static void SetStereoEnabled(HWND hWnd, int nIndex);
static void SetYM3812Enabled(HWND hWnd, int nIndex);
static void SetSamplesEnabled(HWND hWnd, int nIndex, BOOL bSoundEnabled);
static void InitializeOptions(HWND hDlg);
static void InitializeMisc(HWND hDlg);
static void OptOnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);
static void BeamSelectionChange(HWND hwnd);
static void FlickerSelectionChange(HWND hwnd);
static void GammaSelectionChange(HWND hwnd);
static void BrightCorrectSelectionChange(HWND hwnd);
static void BrightnessSelectionChange(HWND hwnd);
static void IntensitySelectionChange(HWND hwnd);
static void A2DSelectionChange(HWND hwnd);
static void ResDepthSelectionChange(HWND hWnd, HWND hWndCtrl);
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl);
static void VolumeSelectionChange(HWND hwnd);
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwDepth, DWORD dwRefresh);
static void InitializeDisplayModeUI(HWND hwnd);
static void InitializeSoundUI(HWND hwnd);
static void InitializeSkippingUI(HWND hwnd);
static void InitializeRotateUI(HWND hwnd);
static void InitializeResDepthUI(HWND hwnd);
static void InitializeRefreshUI(HWND hwnd);
static void InitializeDefaultInputUI(HWND hWnd);
static void InitializeEffectUI(HWND hWnd);
static void InitializeArtresUI(HWND hWnd);
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
static int  g_nGammaIndex      = 0;
static int  g_nBrightCorrectIndex = 0;
static int  g_nBeamIndex       = 0;
static int  g_nFlickerIndex    = 0;
static int  g_nIntensityIndex  = 0;
static int  g_nRotateIndex     = 0;
static int  g_nInputIndex      = 0;
static int  g_nBrightnessIndex = 0;
static int  g_nEffectIndex     = 0;
static int  g_nA2DIndex		   = 0;

static HICON g_hIcon = NULL;
/* Game history variables */
#define MAX_HISTORY_LEN     (8 * 1024)

static char   historyBuf[MAX_HISTORY_LEN];
static char   tempHistoryBuf[MAX_HISTORY_LEN];

/* Property sheets */
struct PropertySheetInfo
{
	BOOL bOnDefaultPage;
	BOOL (*pfnFilterProc)(const struct InternalMachineDriver *drv, const struct GameDriver *gamedrv);
	DWORD dwDlgID;
	DLGPROC pfnDlgProc;
};

static BOOL PropSheetFilter_Vector(const struct InternalMachineDriver *drv, const struct GameDriver *gamedrv)
{
	return (drv->video_attributes & VIDEO_TYPE_VECTOR) != 0;
}

static struct PropertySheetInfo s_propSheets[] =
{
	{ FALSE,	NULL,						IDD_PROP_GAME,			GamePropertiesDialogProc },
	{ FALSE,	NULL,						IDD_PROP_AUDIT,			GameAuditDialogProc },
	{ TRUE,		NULL,						IDD_PROP_DISPLAY,		GameDisplayOptionsProc },
	{ TRUE,		NULL,						IDD_PROP_ADVANCED,		GameOptionsProc },
	{ TRUE,		NULL,						IDD_PROP_SOUND,			GameOptionsProc },
	{ TRUE,		NULL,						IDD_PROP_INPUT,			GameOptionsProc },
	{ TRUE,		NULL,						IDD_PROP_MISC,			GameOptionsProc },
#ifdef MESS
	{ TRUE,		NULL,						IDD_PROP_SOFTWARE,		GameOptionsProc },
	{ FALSE,	PropSheetFilter_Config,	IDD_PROP_CONFIGURATION,	GameOptionsProc },
#endif
	{ TRUE,		PropSheetFilter_Vector,	IDD_PROP_VECTOR,		GameOptionsProc }
};

#define NUM_PROPSHEETS (sizeof(s_propSheets) / sizeof(s_propSheets[0]))

/* Help IDs */
static DWORD dwHelpIDs[] =
{
	
	IDC_A2D,				HIDC_A2D,
	IDC_ANTIALIAS,          HIDC_ANTIALIAS,
	IDC_ARTRES,				HIDC_ARTRES,
	IDC_ARTWORK,            HIDC_ARTWORK,
	IDC_ARTWORK_CROP,		HIDC_ARTWORK_CROP,
	IDC_ASPECTRATIOD,       HIDC_ASPECTRATIOD,
	IDC_ASPECTRATION,       HIDC_ASPECTRATION,
	IDC_AUTOFRAMESKIP,      HIDC_AUTOFRAMESKIP,
	IDC_BACKDROPS,			HIDC_BACKDROPS,
	IDC_BEAM,               HIDC_BEAM,
	IDC_BEZELS,				HIDC_BEZELS,
	IDC_BRIGHTNESS,         HIDC_BRIGHTNESS,
	IDC_BRIGHTCORRECT,      HIDC_BRIGHTCORRECT,
	IDC_BROADCAST,			HIDC_BROADCAST,
	IDC_RANDOM_BG,          HIDC_RANDOM_BG,
	IDC_CHEAT,              HIDC_CHEAT,
	IDC_DDRAW,              HIDC_DDRAW,
	IDC_DEFAULT_INPUT,      HIDC_DEFAULT_INPUT,
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
	IDC_INTENSITY,          HIDC_INTENSITY,
	IDC_JOYSTICK,           HIDC_JOYSTICK,
	IDC_KEEPASPECT,         HIDC_KEEPASPECT,
	IDC_LANGUAGECHECK,      HIDC_LANGUAGECHECK,
	IDC_LANGUAGEEDIT,       HIDC_LANGUAGEEDIT,
	IDC_LEDS,				HIDC_LEDS,
	IDC_LOG,                HIDC_LOG,
	IDC_SLEEP,				HIDC_SLEEP,
	IDC_MATCHREFRESH,       HIDC_MATCHREFRESH,
	IDC_MAXIMIZE,           HIDC_MAXIMIZE,
	IDC_NOROTATE,           HIDC_NOROTATE,
	IDC_OVERLAYS,			HIDC_OVERLAYS,
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
	{ "RGB tiny",                       "rgbtiny" },
	{ "RGB sharp",                      "sharp"   }
};

#define NUMEFFECTS (sizeof(g_ComboBoxEffect) / sizeof(g_ComboBoxEffect[0]))

/***************************************************************
 * Public functions
 ***************************************************************/

DWORD GetHelpIDs(void)
{
	return (DWORD) (LPSTR) dwHelpIDs;
}

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
    struct InternalMachineDriver drv;

	expand_machine_driver(drivers[game]->drv,&drv);

	for (i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++)
	{
		const char **samplenames = NULL;

#if (HAS_SAMPLES == 1)
		if (drv.sound[i].sound_type == SOUND_SAMPLES)
			samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
#endif

        /*
#if (HAS_VLM5030 == 1)
		if (drv.sound[i].sound_type == SOUND_VLM5030)
			samplenames = ((struct VLM5030interface *)drv.sound[i].sound_interface)->samplenames;
#endif
        */
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
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[game]->drv,&drv);
	gamedrv = drivers[game];

	if (GameUsesSamples(game) == FALSE)
		return TRUE;

	for (i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++)
	{
		const char **samplenames = NULL;

#if (HAS_SAMPLES == 1)
		if (drv.sound[i].sound_type == SOUND_SAMPLES)
			samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
#endif
        /*
#if (HAS_VLM5030 == 1)
		if (drv.sound[i].sound_type == SOUND_VLM5030)
			samplenames = ((struct VLM5030interface *)drv.sound[i].sound_interface)->samplenames;
#endif
        */
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

	/* Get default options to populate property sheets */
	pGameOpts = GetDefaultOptions();
	g_bUseDefaults = FALSE;
	/* Stash the result for comparing later */
	memcpy(&origGameOpts, pGameOpts, sizeof(options_type));
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(pshead));
	ZeroMemory(pspage, sizeof(pspage));

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	pshead.pszCaption                 = "Default Game";
	pshead.DUMMYUNIONNAME2.nStartPage = 0;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Fill out the property page templates */
	maxPropSheets = 0;
	for (i = 0; i < NUM_PROPSHEETS; i++)
	{
		if (s_propSheets[i].bOnDefaultPage)
		{
			pspage[maxPropSheets].dwSize                     = sizeof(PROPSHEETPAGE);
			pspage[maxPropSheets].dwFlags                    = 0;
			pspage[maxPropSheets].hInstance                  = hInst;
			pspage[maxPropSheets].DUMMYUNIONNAME.pszTemplate = MAKEINTRESOURCE(s_propSheets[i].dwDlgID);
			pspage[maxPropSheets].pfnCallback                = NULL;
			pspage[maxPropSheets].lParam                     = 0;
			pspage[maxPropSheets].pfnDlgProc                 = s_propSheets[i].pfnDlgProc;
			maxPropSheets++;
		}
	}
	pshead.nPages = maxPropSheets;

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		MessageBox(0, temp, "Error", IDOK);
	}
}

void InitPropertyPage(HINSTANCE hInst, HWND hWnd, int game_num, HICON hIcon)
{
	InitPropertyPageToPage(hInst, hWnd, game_num, hIcon, PROPERTIES_PAGE);
}

void InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, int game_num, HICON hIcon, int start_page)
{
	PROPSHEETHEADER pshead;
	PROPSHEETPAGE   pspage[NUM_PROPSHEETS];
	int             i;
	int             maxPropSheets;
	struct InternalMachineDriver drv;
	expand_machine_driver(drivers[game_num]->drv,&drv);

	g_hIcon = CopyIcon(hIcon);
	InitGameAudit(game_num);
	g_nGame = game_num;

	/* Get Game options to populate property sheets */
	pGameOpts = GetGameOptions(game_num);
	g_bUseDefaults = pGameOpts->use_default;
	/* Stash the result for comparing later */
	memcpy(&origGameOpts, pGameOpts, sizeof(options_type));
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(PROPSHEETHEADER));
	maxPropSheets = 0;

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	pshead.pszCaption                 = ModifyThe(drivers[g_nGame]->description);
	pshead.DUMMYUNIONNAME2.nStartPage = start_page;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Fill out the property page templates */
	for (i = 0; i < NUM_PROPSHEETS; i++)
	{
		if (!s_propSheets[i].pfnFilterProc || s_propSheets[i].pfnFilterProc(&drv, drivers[game_num]))
		{
			memset(&pspage[maxPropSheets], 0, sizeof(pspage[i]));
			pspage[maxPropSheets].dwSize                     = sizeof(PROPSHEETPAGE);
			pspage[maxPropSheets].dwFlags                    = 0;
			pspage[maxPropSheets].hInstance                  = hInst;
			pspage[maxPropSheets].DUMMYUNIONNAME.pszTemplate = MAKEINTRESOURCE(s_propSheets[i].dwDlgID);
			pspage[maxPropSheets].pfnCallback                = NULL;
			pspage[maxPropSheets].lParam                     = 0;
			pspage[maxPropSheets].pfnDlgProc				 = s_propSheets[i].pfnDlgProc;
			maxPropSheets++;
		}
	}
	pshead.nPages = maxPropSheets;

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
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));
	i = 0;
	while (i < MAX_CPU && drv.cpu[i].cpu_type)
	{
		if (drv.cpu[i].cpu_clock >= 1000000)
			sprintf(&buf[strlen(buf)], "%s %d.%06d MHz",
					cputype_name(drv.cpu[i].cpu_type),
					drv.cpu[i].cpu_clock / 1000000,
					drv.cpu[i].cpu_clock % 1000000);
		else
			sprintf(&buf[strlen(buf)], "%s %d.%03d kHz",
					cputype_name(drv.cpu[i].cpu_type),
					drv.cpu[i].cpu_clock / 1000,
					drv.cpu[i].cpu_clock % 1000);

		if (drv.cpu[i].cpu_type & CPU_AUDIO_CPU)
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
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));
	i = 0;
	while (i < MAX_SOUND && drv.sound[i].sound_type)
	{
		if (1 < sound_num(&drv.sound[i]))
			sprintf(&buf[strlen(buf)], "%d x ", sound_num(&drv.sound[i]));

		sprintf(&buf[strlen(buf)], "%s", sound_name(&drv.sound[i]));

		if (sound_clock(&drv.sound[i]))
		{
			if (sound_clock(&drv.sound[i]) >= 1000000)
				sprintf(&buf[strlen(buf)], " %d.%06d MHz",
						sound_clock(&drv.sound[i]) / 1000000,
						sound_clock(&drv.sound[i]) % 1000000);
			else
				sprintf(&buf[strlen(buf)], " %d.%03d kHz",
						sound_clock(&drv.sound[i]) / 1000,
						sound_clock(&drv.sound[i]) % 1000);
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
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	if (drv.video_attributes & VIDEO_TYPE_VECTOR)
		strcpy(buf, "Vector Game");
	else
	{
		if (drivers[nIndex]->flags & ORIENTATION_SWAP_XY)
			sprintf(buf,"%d x %d (vert) %5.2f Hz",
					drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1,
					drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1,
					drv.frames_per_second);
		else
			sprintf(buf,"%d x %d (horz) %5.2f Hz",
					drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1,
					drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1,
					drv.frames_per_second);
	}
	return buf;
}

/* Build color information string */
static char *GameInfoColors(UINT nIndex)
{
	static char buf[1024];
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));
	if (drv.video_attributes & VIDEO_TYPE_VECTOR)
		strcpy(buf, "Vector Game");
	else
	{
		sprintf(buf, "%d colors ", drv.total_colors);
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
				ConvertAmpersandString(ModifyThe(drivers[nIndex]->clone_of->description)),
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
		if (g_hIcon)
			SendMessage(GetDlgItem(hDlg, IDC_GAME_ICON), STM_SETICON, (WPARAM) g_hIcon, 0);
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
			case IDC_SIZES:
			case IDC_FRAMESKIP:
			case IDC_EFFECT:
			case IDC_DEFAULT_INPUT:
			case IDC_ROTATE:
			case IDC_SAMPLERATE:
			case IDC_ARTRES:
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

			default:
#ifdef MESS
				if (MessPropertiesCommand(hDlg, wNotifyCode, wID, &changed))
					break;
#endif

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

	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		Help_HtmlHelp(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		Help_HtmlHelp((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;

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
	MessPropToOptions(hWnd, o);
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
		sprintf(buf, "%03.02f", o->f_gamma_correct);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_BRIGHTCORRECTDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.02f", o->f_bright_correct);
		Static_SetText(hCtrl, buf);
	}

	/* Input */
	hCtrl = GetDlgItem(hWnd, IDC_A2DDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.02f", o->f_a2d);
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

	hCtrl = GetDlgItem(hWnd, IDC_INTENSITYDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.02f", o->f_intensity);
		Static_SetText(hCtrl, buf);
	}

	/* sound */
	hCtrl = GetDlgItem(hWnd, IDC_VOLUMEDISP);
	if (hCtrl)
	{
		sprintf(buf, "%ddB", o->attenuation);
		Static_SetText(hCtrl, buf);
	}

	g_bInternalSet = FALSE;

	g_nInputIndex = 0;
	hCtrl = GetDlgItem(hWnd, IDC_DEFAULT_INPUT);	
	if (hCtrl)
	{
		int nCount;

		/* Get the number of items in the control */
		nCount = ComboBox_GetCount(hCtrl);
        
		while (0 < nCount--)
		{
			ComboBox_GetLBText(hCtrl, nCount, buf);

			if (stricmp (buf,o->ctrlr) == 0)
				g_nInputIndex = nCount;
		}

		ComboBox_SetCurSel(hCtrl, g_nInputIndex);
	}

#ifdef MESS
	MessOptionsToProp(hWnd, o);
#endif
}

/* Adjust controls - tune them to the currently selected game */
static void SetPropEnabledControls(HWND hWnd)
{
	HWND hCtrl;
	int  nIndex;
	int  sound;
	int  ddraw = 0;
	int  useart = 0;
	int joystick_attached = 9;

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

	/* Artwork options */
	hCtrl = GetDlgItem(hWnd, IDC_ARTWORK);
	useart = Button_GetCheck(hCtrl);

	EnableWindow(GetDlgItem(hWnd, IDC_ARTWORK_CROP),	useart);
	EnableWindow(GetDlgItem(hWnd, IDC_BACKDROPS),		useart);
	EnableWindow(GetDlgItem(hWnd, IDC_BEZELS),			useart);
	EnableWindow(GetDlgItem(hWnd, IDC_OVERLAYS),		useart);
	EnableWindow(GetDlgItem(hWnd, IDC_ARTRES),			useart);
	EnableWindow(GetDlgItem(hWnd, IDC_ARTRESTEXT),		useart);

	/* Joystick options */
	joystick_attached = DIJoystick.Available();

	Button_Enable(GetDlgItem(hWnd,IDC_JOYSTICK),		joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_A2DTEXT),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_A2DDISP),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_A2D),				joystick_attached);

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

static void AssignBrightCorrect(HWND hWnd)
{
	/* "1.0", 0.5, 2.0 */
	pGameOpts->f_bright_correct = g_nBrightCorrectIndex / 20.0 + 0.5;
	
}

static void AssignGamma(HWND hWnd)
{
	pGameOpts->f_gamma_correct = g_nGammaIndex / 20.0 + 0.5;
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

static void AssignIntensity(HWND hWnd)
{
	pGameOpts->f_intensity = g_nIntensityIndex / 20.0 + 0.5;
}

static void AssignA2D(HWND hWnd)
{
	pGameOpts->f_a2d = g_nA2DIndex / 20.0;
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
	ComboBox_GetLBText (hWnd, g_nInputIndex, pGameOpts->ctrlr);
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
	g_nGammaIndex			= (int)((pGameOpts->f_gamma_correct  - 0.5) * 20.0);
	g_nBrightnessIndex = (int)((pGameOpts->gfx_brightness - 0.1) * 20.0);
	g_nBrightCorrectIndex	= (int)((pGameOpts->f_bright_correct - 0.5) * 20.0);
	g_nBeamIndex       = (int)((pGameOpts->f_beam         - 1.0) * 20.0);
	g_nFlickerIndex    = (int)(pGameOpts->f_flicker);
	g_nIntensityIndex		= (int)((pGameOpts->f_intensity      - 0.5) * 20.0);
	g_nA2DIndex				= (int)(pGameOpts->f_a2d                    * 20.0);

	// if no controller type was specified or it was standard
	if ((pGameOpts->ctrlr == NULL) || *pGameOpts->ctrlr == 0 || (stricmp(pGameOpts->ctrlr,"Standard") == 0))
	{
		// automatically set to hotrod or hotrodse if selected
		if (pGameOpts->hotrod == 1 && pGameOpts->hotrodse == 0)
   			strcpy (pGameOpts->ctrlr, "HotRod");
		else
		if (pGameOpts->hotrod == 0 && pGameOpts->hotrodse == 1)
   			strcpy (pGameOpts->ctrlr, "HotRodSE");
		else
   			strcpy (pGameOpts->ctrlr, "Standard");
	}
	// turn off hotrod/hotrodse selection
	pGameOpts->hotrod = 0;
	pGameOpts->hotrodse = 0;

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
	DataMapAdd(IDC_THROTTLE,      DM_BOOL, CT_BUTTON,   &pGameOpts->throttle,      0, 0, 0);
	DataMapAdd(IDC_BRIGHTNESS,    DM_INT,  CT_SLIDER,   &g_nBrightnessIndex, 0, 0, AssignBrightness);
	/* pGameOpts->frames_to_display */
	DataMapAdd(IDC_EFFECT,        DM_INT,  CT_COMBOBOX, &g_nEffectIndex, 0, 0, AssignEffect);
	/* pGameOpts->aspect */

	/* input */
	DataMapAdd(IDC_DEFAULT_INPUT, DM_INT,  CT_COMBOBOX, &g_nInputIndex, 0, 0, AssignInput);
	DataMapAdd(IDC_USE_MOUSE,     DM_BOOL, CT_BUTTON,   &pGameOpts->use_mouse,     0, 0, 0);
	DataMapAdd(IDC_JOYSTICK,      DM_BOOL, CT_BUTTON,   &pGameOpts->use_joystick,  0, 0, 0);
	DataMapAdd(IDC_A2D,           DM_INT,  CT_SLIDER,   &g_nA2DIndex,              0, 0, AssignA2D);
	DataMapAdd(IDC_STEADYKEY,     DM_BOOL, CT_BUTTON,   &pGameOpts->steadykey,     0, 0, 0);
	DataMapAdd(IDC_LIGHTGUN,      DM_BOOL, CT_BUTTON,   &pGameOpts->lightgun,      0, 0, 0);   

	/* core video */
	DataMapAdd(IDC_BRIGHTCORRECT, DM_INT,  CT_SLIDER,   &g_nBrightCorrectIndex,    0, 0, AssignBrightCorrect);
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
	DataMapAdd(IDC_INTENSITY,     DM_INT,  CT_SLIDER,   &g_nIntensityIndex,        0, 0, AssignIntensity);	

	/* sound */
	DataMapAdd(IDC_SAMPLERATE,    DM_INT,  CT_COMBOBOX, &g_nSampleRateIndex, 0, 0, AssignSampleRate);
	DataMapAdd(IDC_SAMPLES,       DM_BOOL, CT_BUTTON,   &pGameOpts->use_samples,   0, 0, 0);
	DataMapAdd(IDC_USE_FILTER,    DM_BOOL, CT_BUTTON,   &pGameOpts->use_filter,    0, 0, 0);
	DataMapAdd(IDC_USE_SOUND,     DM_BOOL, CT_BUTTON,   &pGameOpts->enable_sound,  0, 0, 0);
	DataMapAdd(IDC_VOLUME,        DM_INT,  CT_SLIDER,   &g_nVolumeIndex, 0, 0, AssignVolume);

	/* misc artwork options */
	DataMapAdd(IDC_ARTWORK,       DM_BOOL, CT_BUTTON,   &pGameOpts->use_artwork,   0, 0, 0);
	DataMapAdd(IDC_BACKDROPS,     DM_BOOL, CT_BUTTON,   &pGameOpts->backdrops,     0, 0, 0);
	DataMapAdd(IDC_OVERLAYS,      DM_BOOL, CT_BUTTON,   &pGameOpts->overlays,      0, 0, 0);
	DataMapAdd(IDC_BEZELS,        DM_BOOL, CT_BUTTON,   &pGameOpts->bezels,        0, 0, 0);
	DataMapAdd(IDC_ARTRES,        DM_INT,  CT_COMBOBOX, &pGameOpts->artres,        0, 0, 0);
	DataMapAdd(IDC_ARTWORK_CROP,  DM_BOOL, CT_BUTTON,   &pGameOpts->artwork_crop,  0, 0, 0);

	/* misc */
	DataMapAdd(IDC_CHEAT,         DM_BOOL, CT_BUTTON,   &pGameOpts->cheat,         0, 0, 0);
/*	DataMapAdd(IDC_DEBUG,         DM_BOOL, CT_BUTTON,   &pGameOpts->mame_debug,    0, 0, 0);*/
	DataMapAdd(IDC_LOG,           DM_BOOL, CT_BUTTON,   &pGameOpts->errorlog,      0, 0, 0);
	DataMapAdd(IDC_SLEEP,         DM_BOOL, CT_BUTTON,   &pGameOpts->sleep,         0, 0, 0);
	DataMapAdd(IDC_LEDS,          DM_BOOL, CT_BUTTON,   &pGameOpts->leds,          0, 0, 0);
#ifdef MESS
	DataMapAdd(IDC_USE_NEW_UI,    DM_BOOL, CT_BUTTON,   &pGameOpts->use_new_ui, 0, 0, 0);
#endif
}

static void SetStereoEnabled(HWND hWnd, int nIndex)
{
	BOOL enabled = FALSE;
	HWND hCtrl;
    struct InternalMachineDriver drv;

	if (-1 != nIndex)
		expand_machine_driver(drivers[nIndex]->drv,&drv);

	hCtrl = GetDlgItem(hWnd, IDC_STEREO);
	if (hCtrl)
	{
		if (nIndex == -1 || drv.sound_attributes & SOUND_SUPPORTS_STEREO)
			enabled = TRUE;

		EnableWindow(hCtrl, enabled);
	}
}

static void SetYM3812Enabled(HWND hWnd, int nIndex)
{
	int i;
	BOOL enabled;
	HWND hCtrl;
    struct InternalMachineDriver drv;

	if (-1 != nIndex)
		expand_machine_driver(drivers[nIndex]->drv,&drv);

	hCtrl = GetDlgItem(hWnd, IDC_USE_FM_YM3812);
	if (hCtrl)
	{
		enabled = FALSE;
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (nIndex == -1
#if HAS_YM3812
			||  drv.sound[i].sound_type == SOUND_YM3812
#endif
#if HAS_YM3526
			||  drv.sound[i].sound_type == SOUND_YM3526
#endif
#if HAS_YM2413
			||  drv.sound[i].sound_type == SOUND_YM2413
#endif
			)
				enabled = TRUE;
		}

		EnableWindow(hCtrl, enabled);
	}
}

static void SetSamplesEnabled(HWND hWnd, int nIndex, BOOL bSoundEnabled)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)
	int i;
	BOOL enabled = FALSE;
	HWND hCtrl;
    struct InternalMachineDriver drv;

	hCtrl = GetDlgItem(hWnd, IDC_SAMPLES);

	if (-1 != nIndex)
		expand_machine_driver(drivers[nIndex]->drv,&drv);
	
	if (hCtrl)
	{
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (nIndex == -1
			||  drv.sound[i].sound_type == SOUND_SAMPLES
#if HAS_VLM5030
			||  drv.sound[i].sound_type == SOUND_VLM5030
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
	InitializeResDepthUI(hDlg);
	InitializeRefreshUI(hDlg);
	InitializeDisplayModeUI(hDlg);
	InitializeSoundUI(hDlg);
	InitializeSkippingUI(hDlg);
	InitializeRotateUI(hDlg);
	InitializeDefaultInputUI(hDlg);
	InitializeEffectUI(hDlg);
	InitializeArtresUI(hDlg);
}

/* Moved here because it is called in several places */
static void InitializeMisc(HWND hDlg)
{
	Button_Enable(GetDlgItem(hDlg, IDC_JOYSTICK), DIJoystick.Available());

	SendMessage(GetDlgItem(hDlg, IDC_GAMMA), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 30)); /* [0.50, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_BRIGHTCORRECT), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 30)); /* [0.50, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_BRIGHTNESS), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 38)); /* [0.10, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_INTENSITY), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 50)); /* [0.50, 3.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_A2D), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 20)); /* [0.00, 1.00] in .05 increments */

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
	if (hwndCtl == GetDlgItem(hwnd, IDC_BRIGHTCORRECT))
	{
		BrightCorrectSelectionChange(hwnd);
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
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_INTENSITY))
	{
		IntensitySelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_A2D))
	{
		A2DSelectionChange(hwnd);
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

/* Handle changes to the Brightness Correction slider */
static void BrightCorrectSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_BRIGHTCORRECT), TBM_GETPOS, 0, 0);

	dValue = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	sprintf(buf, "%03.02f", dValue);
	Static_SetText(GetDlgItem(hwnd, IDC_BRIGHTCORRECTDISP), buf);
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

/* Handle changes to the Intensity slider */
static void IntensitySelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dIntensity;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_INTENSITY), TBM_GETPOS, 0, 0);

	dIntensity = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	sprintf(buf, "%03.02f", dIntensity);
	Static_SetText(GetDlgItem(hwnd, IDC_INTENSITYDISP), buf);
}

/* Handle changes to the A2D slider */
static void A2DSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dA2D;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_A2D), TBM_GETPOS, 0, 0);

	dA2D = nValue / 20.0;

	/* Set the static display to the new value */
	sprintf(buf, "%03.02f", dA2D);
	Static_SetText(GetDlgItem(hwnd, IDC_A2DDISP), buf);
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

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char *ext;
	int isZipFile;
	char root[256];
	char path[256];

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Standard");

		sprintf (path, "%s\\*.*", GetCtrlrDir());

		hFind = FindFirstFile(path, &FindFileData);

	    if (hFind != INVALID_HANDLE_VALUE)
		{
			do 
			{
				if (strcmp (FindFileData.cFileName,".") != 0 && 
					strcmp (FindFileData.cFileName,"..") != 0)
				{
					// copy the filename
					strcpy (root,FindFileData.cFileName);

					// assume it's not a zip file
					isZipFile = 0;

					// find the extension
					ext = strrchr (root,'.');
					if (ext)
					{
						// check if it's a zip file
						if (strcmp (ext, ".zip") == 0)
						{
							isZipFile = 1;
						}

						// and strip off the extension
						*ext = 0;
					}

					if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || isZipFile)
					{
						ComboBox_AddString(hCtrl, root);
					}
				}
			}
			while (FindNextFile (hFind, &FindFileData) != 0);
			
			FindClose (hFind);
		}
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

/* Populate the Art Resolution drop down */
static void InitializeArtresUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_ARTRES);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Auto");		/* 0 */
		ComboBox_AddString(hCtrl, "Standard");  /* 1 */
		ComboBox_AddString(hCtrl, "High");		/* 2 */
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
