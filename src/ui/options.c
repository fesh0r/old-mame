/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  options.c

  Stores global options and per-game options;

***************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <winreg.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <malloc.h>
#include <math.h>
#include <driver.h>
#include "mame32.h"
#include "m32util.h"
#include "resource.h"
#include "treeview.h"
#include "file.h"
#include "splitters.h"

// #define UPGRADE
/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

static void LoadFolderFilter(int folder_index,int filters);

static REG_OPTION * GetOption(REG_OPTION *option_array,int num_options,const char *key);
static void LoadOption(REG_OPTION *option,const char *value_str);
static BOOL LoadGameVariableOrFolderFilter(char *key,const char *value);
static void ParseKeyValueStrings(char *buffer,char **key,char **value);
static void LoadOptionsAndSettings(void);
static BOOL LoadOptions(const char *filename,options_type *o,BOOL load_global_game_options);
static void SaveSettings(void);

static void  LoadRegGameOptions(HKEY hKey, options_type *o, int driver_index);
static DWORD GetRegOption(HKEY hKey, const char *name);
static void  GetRegBoolOption(HKEY hKey, const char *name, BOOL *value);
static char  *GetRegStringOption(HKEY hKey, const char *name);

static void WriteStringOptionToFile(FILE *fptr,const char *key,const char *value);
static void WriteIntOptionToFile(FILE *fptr,const char *key,int value);
static void WriteBoolOptionToFile(FILE *fptr,const char *key,BOOL value);
static void WriteColorOptionToFile(FILE *fptr,const char *key,COLORREF value);

static BOOL IsOptionEqual(int option_index,options_type *o1,options_type *o2);
static void WriteOptionToFile(FILE *fptr,REG_OPTION *regOpt);

static void  ColumnEncodeString(void* data, char* str);
static void  ColumnDecodeString(const char* str, void* data);

static void  ColumnDecodeWidths(const char *ptr, void* data);

static void  SplitterEncodeString(void* data, char* str);
static void  SplitterDecodeString(const char* str, void* data);

static void  ListEncodeString(void* data, char* str);
static void  ListDecodeString(const char* str, void* data);

static void  FontEncodeString(void* data, char* str);
static void  FontDecodeString(const char* str, void* data);

static void  GetRegObj(HKEY hKey, REG_OPTION *regOpt);

/***************************************************************************
    Internal defines
 ***************************************************************************/

/* Used to create/retrieve Registry database */
/* #define KEY_BASE "Software\\Freeware\\TestMame32" */

#define DOTBACKUP       ".Backup"
#define DOTDEFAULTS     ".Defaults"
#define DOTFOLDERS      ".Folders"

#define KEY_BASE        "Software\\Freeware\\" MAME32NAME

#define KEY_BASE_FMT         KEY_BASE "\\"
#define KEY_BASE_FMT_CCH     sizeof(KEY_BASE_FMT)-1

#define KEY_BACKUP      KEY_BASE "\\" DOTBACKUP "\\"
#define KEY_BACKUP_CCH  sizeof(KEY_BACKUP)-1

#define KEY_BASE_DOTBACKUP      KEY_BASE_FMT DOTBACKUP
#define KEY_BASE_DOTDEFAULTS    KEY_BASE_FMT DOTDEFAULTS
#define KEY_BASE_DOTFOLDERS     KEY_BASE_FMT DOTFOLDERS
#define KEY_BACKUP_DOTDEFAULTS      KEY_BACKUP DOTDEFAULTS

#define UI_INI_FILENAME MAME32NAME "ui.ini"
#define DEFAULT_OPTIONS_INI_FILENAME MAME32NAME ".ini"

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	int folder_index;
	int filters;
} folder_filter_type;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static settings_type settings;

static options_type gOpts;  // Used in conjunction with regGameOpts

static options_type global; // Global 'default' options
static options_type *game_options;  // Array of Game specific options
static game_variables_type *game_variables;  // Array of game specific extra data

// UI options in mame32ui.ini
static REG_OPTION regSettings[] =
{
	{"DefaultGame",        "default_game",       RO_STRING,  &settings.default_game,      0, 0},
	{"FolderID",           "default_folder_id",  RO_INT,     &settings.folder_id,        0, 0},
	{"ShowScreenShot",     "show_image_section", RO_BOOL,    &settings.show_screenshot,  0, 0},
	// this one should be encoded
	{"ShowFlyer",          "show_image_type",    RO_INT,     &settings.show_pict_type,   0, 0},
	{"ShowToolBar",        "show_tool_bar",      RO_BOOL,    &settings.show_toolbar,     0, 0},
	{"ShowStatusBar",      "show_status_bar",    RO_BOOL,    &settings.show_statusbar,   0, 0},
	{"ShowFolderList",     "show_folder_section",RO_BOOL,    &settings.show_folderlist,  0, 0},
	{"ShowTabCtrl",        "show_tabs",          RO_BOOL,    &settings.show_tabctrl,     0, 0},
	{"GameCheck",          "check_game",         RO_BOOL,    &settings.game_check,       0, 0},
	{"VersionCheck",       "check_version",      RO_BOOL,    &settings.version_check,    0, 0},
	{"JoyGUI",             "joystick_in_interface",RO_BOOL,&settings.use_joygui,     0, 0},
	{"Broadcast",          "broadcast_game_name",RO_BOOL,    &settings.broadcast,        0, 0},
	{"Random_Bg",          "random_background",  RO_BOOL,    &settings.random_bg,        0, 0},

	{"SortColumn",         "sort_column",        RO_INT,     &settings.sort_column,      0, 0},
	{"SortReverse",        "sort_reversed",      RO_BOOL,    &settings.sort_reverse,     0, 0},
	{"X",                  "window_x",           RO_INT,     &settings.area.x,           0, 0},
	{"Y",                  "window_y",           RO_INT,     &settings.area.y,           0, 0},
	{"Width",              "window_width",       RO_INT,     &settings.area.width,       0, 0},
	{"Height",             "window_height",      RO_INT,     &settings.area.height,      0, 0},
	{"State",              "window_state",       RO_INT,     &settings.windowstate,      0, 0},

	{"text_color",         "text_color",         RO_COLOR,   &settings.list_font_color,  0, 0},
	{"clone_color",        "clone_color",        RO_COLOR,   &settings.list_clone_color,  0, 0},
	/* ListMode needs to be before ColumnWidths settings */
	{"ListMode",           "list_mode",          RO_ENCODE,  &settings.view,             ListEncodeString,     ListDecodeString},
	{"Splitters",          "splitters",          RO_ENCODE,  settings.splitter,          SplitterEncodeString, SplitterDecodeString},
	{"ListFont",           "list_font",          RO_ENCODE,  &settings.list_font,        FontEncodeString,     FontDecodeString},
	{"ColumnWidths",       "column_widths",      RO_ENCODE,  &settings.column_width,     ColumnEncodeString,   ColumnDecodeWidths},
	{"ColumnOrder",        "column_order",       RO_ENCODE,  &settings.column_order,     ColumnEncodeString,   ColumnDecodeString},
	{"ColumnShown",        "column_shown",       RO_ENCODE,  &settings.column_shown,     ColumnEncodeString,   ColumnDecodeString},

	// only here for upgrading from the registry
	{"ShowDisclaimer",     "",                   RO_BOOL,    &settings.show_disclaimer,  0, 0},
	{"ShowGameInfo",       "",                   RO_BOOL,    &settings.show_gameinfo,    0, 0},

	{"Language",           "",                   RO_STRING,  &settings.language,         0, 0},
	{"FlyerDir",           "",                   RO_STRING,  &settings.flyerdir,         0, 0},
	{"CabinetDir",         "",                   RO_STRING,  &settings.cabinetdir,       0, 0},
	{"MarqueeDir",         "",                   RO_STRING,  &settings.marqueedir,       0, 0},
	{"TitlesDir",          "",                   RO_STRING,  &settings.titlesdir,        0, 0},
	{"BkgroundDir",        "",                   RO_STRING,  &settings.bgdir,            0, 0},

#ifdef MESS
	{"biospath",           "",                   RO_STRING,  &settings.romdirs,	         0, 0},
	{"softwarepath",       "",                   RO_STRING,  &settings.softwaredirs,     0, 0},
	{"CRC_directory",      "",                   RO_STRING,  &settings.crcdir,           0, 0},
#else
	{"rompath",            "",                   RO_STRING,  &settings.romdirs,          0, 0},
#endif
	{"samplepath",         "",                   RO_STRING,  &settings.sampledirs,       0, 0},
	{"inipath",			   "",                   RO_STRING,  &settings.inidir,           0, 0},
	{"cfg_directory",      "",                   RO_STRING,  &settings.cfgdir,           0, 0},
	{"nvram_directory",    "",                   RO_STRING,  &settings.nvramdir,         0, 0},
	{"memcard_directory",  "",                   RO_STRING,  &settings.memcarddir,       0, 0},
	{"input_directory",    "",                   RO_STRING,  &settings.inpdir,           0, 0},
	{"hiscore_directory",  "",                   RO_STRING,  &settings.hidir,            0, 0},
	{"state_directory",    "",                   RO_STRING,  &settings.statedir,         0, 0},
	{"artwork_directory",  "",                   RO_STRING,  &settings.artdir,           0, 0},
	{"snapshot_directory", "",                   RO_STRING,  &settings.imgdir,           0, 0},
	{"diff_directory",     "",                   RO_STRING,  &settings.diffdir,          0, 0},
	{"icons_directory",    "",                   RO_STRING,  &settings.iconsdir,         0, 0},
	{"cheat_file",         "",                   RO_STRING,  &settings.cheat_filename,   0, 0},
	{"history_file",       "",                   RO_STRING,  &settings.history_filename, 0, 0},
	{"mameinfo_file",      "",                   RO_STRING,  &settings.mameinfo_filename,0, 0},
	{"ctrlr_directory",    "",                   RO_STRING,  &settings.ctrlrdir,         0, 0},
	{"folder_directory",   "",                   RO_STRING,  &settings.folderdir,        0, 0},

#ifdef MESS
	{"softwarepath",       "",                   RO_STRING, &settings.softwaredirs,     0, 0},
	{"crc_directory",      "",                   RO_STRING, &settings.crcdir,           0, 0},
	{"MessColumnWidths",   "mess_column_widths", RO_ENCODE,  &settings.mess_column_width,MessColumnEncodeString, MessColumnDecodeWidths},
	{"MessColumnOrder",    "mess_column_order",  RO_ENCODE,  &settings.mess_column_order,MessColumnEncodeString, MessColumnDecodeString},
	{"MessColumnShown",    "mess_column_shown",  RO_ENCODE,  &settings.mess_column_shown,MessColumnEncodeString, MessColumnDecodeString}
#endif
};
#define NUM_SETTINGS (sizeof(regSettings) / sizeof(regSettings[0]))

// options in mame32.ini or (gamename).ini
static REG_OPTION regGameOpts[] =
{
	// video
	{ "autoframeskip",      "autoframeskip",          RO_BOOL,    &gOpts.autoframeskip,     0, 0},
	{ "frameskip",          "frameskip",              RO_INT,     &gOpts.frameskip,         0, 0},
	{ "waitvsync",          "waitvsync",              RO_BOOL,    &gOpts.wait_vsync,        0, 0},
	{ "triplebuffer",       "triplebuffer",           RO_BOOL,    &gOpts.use_triplebuf,     0, 0},
	{ "window",             "window",                 RO_BOOL,    &gOpts.window_mode,       0, 0},
	{ "ddraw",              "ddraw",                  RO_BOOL,    &gOpts.use_ddraw,         0, 0},
	{ "hwstretch",          "hwstretch",              RO_BOOL,    &gOpts.ddraw_stretch,     0, 0},
	{ "resolution",         "resolution",             RO_STRING,  &gOpts.resolution,        0, 0},
	{ "refresh",            "refresh",                RO_INT,     &gOpts.gfx_refresh,       0, 0},
	{ "scanlines",          "scanlines",              RO_BOOL,    &gOpts.scanlines,         0, 0},
	{ "switchres",          "switchres",              RO_BOOL,    &gOpts.switchres,         0, 0},
	{ "switchbpp",          "switchbpp",              RO_BOOL,    &gOpts.switchbpp,         0, 0},
	{ "maximize",           "maximize",               RO_BOOL,    &gOpts.maximize,          0, 0},
	{ "keepaspect",         "keepaspect",             RO_BOOL,    &gOpts.keepaspect,        0, 0},
	{ "matchrefresh",       "matchrefresh",           RO_BOOL,    &gOpts.matchrefresh,      0, 0},
	{ "syncrefresh",        "syncrefresh",            RO_BOOL,    &gOpts.syncrefresh,       0, 0},
	{ "throttle",           "throttle",               RO_BOOL,    &gOpts.throttle,          0, 0},
	{ "full_screen_brightness", "full_screen_brightness", RO_DOUBLE,  &gOpts.gfx_brightness,    0, 0},
	{ "frames_to_run",      "frames_to_run",          RO_INT,     &gOpts.frames_to_display, 0, 0},
	{ "effect",             "effect",                 RO_STRING,  &gOpts.effect,            0, 0},
	{ "screen_aspect",      "screen_aspect",          RO_STRING,  &gOpts.aspect,            0, 0},

	// input
	{ "mouse",              "mouse",                  RO_BOOL,    &gOpts.use_mouse,         0, 0},
	{ "joystick",           "joystick",               RO_BOOL,    &gOpts.use_joystick,      0, 0},
	{ "a2d",                "a2d",                    RO_DOUBLE,  &gOpts.f_a2d,             0, 0},
	{ "steadykey",          "steadykey",              RO_BOOL,    &gOpts.steadykey,         0, 0},
	{ "lightgun",           "lightgun",               RO_BOOL,    &gOpts.lightgun,          0, 0},
	{ "ctrlr",              "ctrlr",                  RO_STRING,  &gOpts.ctrlr,             0, 0},

	// core video
	{ "brightness",         "brightness",             RO_DOUBLE,  &gOpts.f_bright_correct,  0, 0}, 
	{ "pause_brightness",   "pause_brightness",       RO_DOUBLE,  &gOpts.f_pause_bright    ,0, 0}, 
	{ "norotate",           "norotate",               RO_BOOL,    &gOpts.norotate,          0, 0},
	{ "ror",                "ror",                    RO_BOOL,    &gOpts.ror,               0, 0},
	{ "rol",                "rol",                    RO_BOOL,    &gOpts.rol,               0, 0},
	{ "auto_ror",           "auto_ror",               RO_BOOL,    &gOpts.auto_ror,          0, 0},
	{ "auto_rol",           "auto_rol",               RO_BOOL,    &gOpts.auto_rol,          0, 0},
	{ "flipx",              "flipx",                  RO_BOOL,    &gOpts.flipx,             0, 0},
	{ "flipy",              "flipy",                  RO_BOOL,    &gOpts.flipy,             0, 0},
	{ "debug_resolution",   "debug_resolution",       RO_STRING,  &gOpts.debugres,          0, 0}, 
	{ "gamma",              "gamma",                  RO_DOUBLE,  &gOpts.f_gamma_correct,   0, 0},

	// vector
	{ "antialias",          "antialias",              RO_BOOL,    &gOpts.antialias,         0, 0},
	{ "translucency",       "translucency",           RO_BOOL,    &gOpts.translucency,      0, 0},
	{ "beam",               "beam",                   RO_DOUBLE,  &gOpts.f_beam,            0, 0},
	{ "flicker",            "flicker",                RO_DOUBLE,  &gOpts.f_flicker,         0, 0},
	{ "intensity",          "intensity",              RO_DOUBLE,  &gOpts.f_intensity,       0, 0},

	// sound
	{ "samplerate",         "samplerate",             RO_INT,     &gOpts.samplerate,        0, 0},
	{ "use_samples",        "samples",                RO_BOOL,    &gOpts.use_samples,       0, 0},
	{ "resamplefilter",     "resamplefilter",         RO_BOOL,    &gOpts.use_filter,        0, 0},
	{ "sound",              "sound",                  RO_BOOL,    &gOpts.enable_sound,      0, 0},
	{ "volume",             "volume",                 RO_INT,     &gOpts.attenuation,       0, 0},

	// misc artwork options
	{ "artwork",            "artwork",                RO_BOOL,    &gOpts.use_artwork,       0, 0},
	{ "backdrops",          "backdrop",               RO_BOOL,    &gOpts.backdrops,         0, 0},
	{ "overlays",           "overlay",                RO_BOOL,    &gOpts.overlays,          0, 0},
	{ "bezels",             "bezel",                  RO_BOOL,    &gOpts.bezels,            0, 0},
	{ "artwork_crop",       "artwork_crop",           RO_BOOL,    &gOpts.artwork_crop,      0, 0},
	{ "artres",             "artres",                 RO_INT,     &gOpts.artres,            0, 0},

	// misc
	{ "cheat",              "cheat",                  RO_BOOL,    &gOpts.cheat,             0, 0},
	{ "debug",              "debug",                  RO_BOOL,    &gOpts.mame_debug,        0, 0},
	{ "log",                "log",                    RO_BOOL,    &gOpts.errorlog,          0, 0},
	{ "sleep",              "sleep",                  RO_BOOL,    &gOpts.sleep,             0, 0},
	{ "old_timing",         "rdtsc",                  RO_BOOL,    &gOpts.old_timing,        0, 0},
	{ "leds",               "leds",                   RO_BOOL,    &gOpts.leds,              0, 0}

#ifdef MESS
	/* mess options */
	,
	{ "use_new_ui",		"newui",                      RO_BOOL,    &gOpts.use_new_ui,		0, 0, FALSE},
	{ "ram_size",		"ramsize",                    RO_INT,     &gOpts.ram_size,			0, 0, TRUE},
	{ "cartridge",		"cartridge",                  RO_STRING,  &gOpts.software[IO_CARTSLOT],	0, 0, TRUE},
	{ "floppydisk",		"floppydisk",                 RO_STRING,  &gOpts.software[IO_FLOPPY],	0, 0, TRUE},
	{ "harddisk",		"harddisk",                   RO_STRING,  &gOpts.software[IO_HARDDISK],	0, 0, TRUE},
	{ "cylinder",		"cylinder",                   RO_STRING,  &gOpts.software[IO_CYLINDER],	0, 0, TRUE},
	{ "cassette",		"cassette",                   RO_STRING,  &gOpts.software[IO_CASSETTE],	0, 0, TRUE},
	{ "punchcard",		"punchcard",                  RO_STRING,  &gOpts.software[IO_PUNCHCARD],	0, 0, TRUE},
	{ "punchtape",		"punchtape",                  RO_STRING,  &gOpts.software[IO_PUNCHTAPE],	0, 0, TRUE},
	{ "printer",		"printer",                    RO_STRING,  &gOpts.software[IO_PRINTER],	0, 0, TRUE},
	{ "serial",			"serial",                     RO_STRING,  &gOpts.software[IO_SERIAL],	0, 0, TRUE},
	{ "parallel",		"parallel",                   RO_STRING,  &gOpts.software[IO_PARALLEL],	0, 0, TRUE},
	{ "snapshot",		"snapshot",                   RO_STRING,  &gOpts.software[IO_SNAPSHOT],	0, 0, TRUE},
	{ "quickload",		"quickload",                  RO_STRING,  &gOpts.software[IO_QUICKLOAD],0, 0, TRUE}
#endif /* MESS */
};
#define NUM_GAME_OPTIONS (sizeof(regGameOpts) / sizeof(regGameOpts[0]))

// options in mame32.ini that we'll never override with with game-specific options
static REG_OPTION global_game_options[] =
{
	{"ShowDisclaimer",     "show_disclaimer",    RO_BOOL,    &settings.show_disclaimer,  0, 0},
	{"ShowGameInfo",       "show_game_info",     RO_BOOL,    &settings.show_gameinfo,    0, 0},
	{"high_priority",      "high_priority",      RO_BOOL,    &settings.high_priority,    0, 0},

	{"Language",           "language",           RO_STRING,  &settings.language,         0, 0},
	{"FlyerDir",           "flyer_directory",    RO_STRING,  &settings.flyerdir,         0, 0},
	{"CabinetDir",         "cabinet_directory",  RO_STRING,  &settings.cabinetdir,       0, 0},
	{"MarqueeDir",         "marquee_directory",  RO_STRING,  &settings.marqueedir,       0, 0},
	{"TitlesDir",          "title_directory",    RO_STRING,  &settings.titlesdir,        0, 0},
	{"BkgroundDir",        "background_directory",RO_STRING, &settings.bgdir,            0, 0},

#ifdef MESS
	{"biospath",           "biospath",           RO_STRING,  &settings.romdirs,          0, 0},
	{"softwarepath",       "softwarepath",       RO_STRING,  &settings.softwaredirs,     0, 0},
	{"CRC_directory",      "CRC_directory",      RO_STRING,  &settings.crcdir,           0, 0},
#else
	{"rompath",            "rompath",            RO_STRING,  &settings.romdirs,          0, 0},
#endif
	{"samplepath",         "samplepath",         RO_STRING,  &settings.sampledirs,       0, 0},
	{"inipath",			   "ini_directory",		 RO_STRING,  &settings.inidir,           0, 0},
	{"cfg_directory",      "cfg_directory",      RO_STRING,  &settings.cfgdir,           0, 0},
	{"nvram_directory",    "nvram_directory",    RO_STRING,  &settings.nvramdir,         0, 0},
	{"memcard_directory",  "memcard_directory",  RO_STRING,  &settings.memcarddir,       0, 0},
	{"input_directory",    "input_directory",    RO_STRING,  &settings.inpdir,           0, 0},
	{"hiscore_directory",  "hiscore_directory",  RO_STRING,  &settings.hidir,            0, 0},
	{"state_directory",    "state_directory",    RO_STRING,  &settings.statedir,         0, 0},
	{"artwork_directory",  "artwork_directory",  RO_STRING,  &settings.artdir,           0, 0},
	{"snapshot_directory", "snapshot_directory", RO_STRING,  &settings.imgdir,           0, 0},
	{"diff_directory",     "diff_directory",     RO_STRING,  &settings.diffdir,          0, 0},
	{"icons_directory",    "icons_directory",    RO_STRING,  &settings.iconsdir,         0, 0},
	{"cheat_file",         "cheat_file",         RO_STRING,  &settings.cheat_filename,   0, 0},
	{"history_file",       "history_file",       RO_STRING,  &settings.history_filename, 0, 0},
	{"mameinfo_file",      "mameinfo_file",      RO_STRING,  &settings.mameinfo_filename,0, 0},
	{"ctrlr_directory",    "ctrlr_directory",    RO_STRING,  &settings.ctrlrdir,         0, 0},
	{"folder_directory",   "folder_directory",   RO_STRING,  &settings.folderdir,        0, 0},

};
#define NUM_GLOBAL_GAME_OPTIONS (sizeof(global_game_options) / sizeof(global_game_options[0]))


static int  num_games = 0;
static BOOL save_gui_settings = TRUE;
static BOOL save_default_options = TRUE;

/* Default sizes based on 8pt font w/sort arrow in that column */
static int default_column_width[] = { 187, 68, 84, 84, 64, 88, 74,108, 60,144, 84 };
static int default_column_shown[] = {   1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0 };
/* Hidden columns need to go at the end of the order array */
static int default_column_order[] = {   0,  2,  3,  4,  5,  6,  7,  8,  9,  1, 10 };

static const char *view_modes[VIEW_MAX] = { "Large Icons", "Small Icons", "List", "Details", "Grouped" };

folder_filter_type *folder_filters;
int size_folder_filters;
int num_folder_filters;

extern const char g_szDefaultGame[];

/***************************************************************************
    External functions  
 ***************************************************************************/

BOOL OptionsInit()
{
	int i;

	num_games = GetNumGames();

	settings.default_game    = strdup(g_szDefaultGame);
	settings.folder_id       = 0;
	settings.view            = VIEW_GROUPED;
	settings.show_folderlist = TRUE;
	settings.show_toolbar    = TRUE;
	settings.show_statusbar  = TRUE;
	settings.show_screenshot = TRUE;
	settings.show_tabctrl    = TRUE;
	settings.game_check      = TRUE;
	settings.version_check   = TRUE;
	settings.use_joygui      = FALSE;
	settings.broadcast       = FALSE;
	settings.random_bg       = FALSE;

	for (i = 0; i < COLUMN_MAX; i++)
	{
		settings.column_width[i] = default_column_width[i];
		settings.column_order[i] = default_column_order[i];
		settings.column_shown[i] = default_column_shown[i];
	}

#ifdef MESS
	for (i = 0; i < MESS_COLUMN_MAX; i++)
	{
		settings.mess_column_width[i] = default_mess_column_width[i];
		settings.mess_column_order[i] = default_mess_column_order[i];
		settings.mess_column_shown[i] = default_mess_column_shown[i];
	}
#endif
	settings.sort_column = 0;
	settings.sort_reverse= FALSE;
	settings.area.x      = 0;
	settings.area.y      = 0;
	settings.area.width  = 640;
	settings.area.height = 400;
	settings.windowstate = 1;
	settings.splitter[0] = 152;
	settings.splitter[1] = 362;
#ifdef MESS
	/* an algorithm to adjust for the fact that we need a larger window for the
	 * software picker
	 */
	settings.splitter[1] -= (settings.splitter[1] - settings.splitter[0]) / 4;
	settings.area.width += settings.splitter[1] - settings.splitter[0];
	settings.splitter[2] = settings.splitter[1] + (settings.splitter[1] - settings.splitter[0]);
#endif

	settings.language          = strdup("english");
	settings.flyerdir          = strdup("flyers");
	settings.cabinetdir        = strdup("cabinets");
	settings.marqueedir        = strdup("marquees");
	settings.titlesdir         = strdup("titles");

#ifdef MESS
	settings.romdirs           = strdup("bios");
#else
	settings.romdirs           = strdup("roms");
#endif
	settings.sampledirs        = strdup("samples");
#ifdef MESS
	settings.softwaredirs      = strdup("software");
	settings.crcdir            = strdup("crc");
#endif
	settings.inidir 		   = strdup("ini");
	settings.cfgdir            = strdup("cfg");
	settings.nvramdir          = strdup("nvram");
	settings.memcarddir        = strdup("memcard");
	settings.inpdir            = strdup("inp");
	settings.hidir             = strdup("hi");
	settings.statedir          = strdup("sta");
	settings.artdir            = strdup("artwork");
	settings.imgdir            = strdup("snap");
	settings.diffdir           = strdup("diff");
	settings.iconsdir          = strdup("icons");
	settings.bgdir             = strdup("bkground");
	settings.cheat_filename    = strdup("cheat.dat");
#ifdef MESS
	settings.history_filename  = strdup("sysinfo.dat");
	settings.mameinfo_filename = strdup("messinfo.dat");
#else
	settings.history_filename  = strdup("history.dat");
	settings.mameinfo_filename = strdup("mameinfo.dat");
#endif
	settings.ctrlrdir          = strdup("ctrlr");
	settings.folderdir         = strdup("folders");

	settings.list_font.lfHeight         = -8;
	settings.list_font.lfWidth          = 0;
	settings.list_font.lfEscapement     = 0;
	settings.list_font.lfOrientation    = 0;
	settings.list_font.lfWeight         = FW_NORMAL;
	settings.list_font.lfItalic         = FALSE;
	settings.list_font.lfUnderline      = FALSE;
	settings.list_font.lfStrikeOut      = FALSE;
	settings.list_font.lfCharSet        = ANSI_CHARSET;
	settings.list_font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	settings.list_font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	settings.list_font.lfQuality        = DEFAULT_QUALITY;
	settings.list_font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	strcpy(settings.list_font.lfFaceName, "MS Sans Serif");

	settings.list_font_color = (COLORREF)-1;
	settings.list_clone_color = (COLORREF)-1;

	settings.show_disclaimer = TRUE;
	settings.show_gameinfo = TRUE;
	settings.high_priority = FALSE;

	/* video */
	global.autoframeskip     = TRUE;
	global.frameskip         = 0;
	global.wait_vsync        = FALSE;
	global.use_triplebuf     = FALSE;
	global.window_mode       = FALSE;
	global.use_ddraw         = TRUE;
	global.ddraw_stretch     = TRUE;
	global.resolution        = strdup("auto");
	global.gfx_refresh       = 0;
	global.scanlines         = FALSE;
	global.switchres         = TRUE;
	global.switchbpp         = TRUE;
	global.maximize          = TRUE;
	global.keepaspect        = TRUE;
	global.matchrefresh      = FALSE;
	global.syncrefresh       = FALSE;
	global.throttle          = TRUE;
	global.gfx_brightness    = 1.0;
	global.frames_to_display = 0;
	global.effect            = strdup("none");
	global.aspect            = strdup("4:3");

	/* input */
	global.use_mouse         = FALSE;
	global.use_joystick      = FALSE;
	global.f_a2d             = 0.3;
	global.steadykey         = FALSE;
	global.lightgun          = FALSE;
	global.ctrlr             = strdup("Standard");

	/* Core video */
	global.f_bright_correct  = 1.0;
	global.f_pause_bright    = 0.65;
	global.norotate          = FALSE;
	global.ror               = FALSE;
	global.rol               = FALSE;
	global.auto_ror          = FALSE;
	global.auto_rol          = FALSE;
	global.flipx             = FALSE;
	global.flipy             = FALSE;
	global.debugres          = strdup("auto");
	global.f_gamma_correct   = 1.0;

	/* Core vector */
	global.antialias         = TRUE;
	global.translucency      = TRUE;
	global.f_beam            = 1.0;
	global.f_flicker         = 0.0;
	global.f_intensity		 = 1.5;

	/* Sound */
	global.samplerate        = 44100;
	global.use_samples       = TRUE;
	global.use_filter        = TRUE;
	global.enable_sound      = TRUE;
	global.attenuation       = 0;

	/* misc artwork options */
	global.use_artwork       = TRUE;
	global.backdrops         = TRUE;
	global.overlays          = TRUE;
	global.bezels            = TRUE;
	global.artwork_crop      = FALSE;
	global.artres            = 0; /* auto */

	/* misc */
	global.cheat             = FALSE;
	global.mame_debug        = FALSE;
	global.errorlog          = FALSE;
	global.sleep             = FALSE;
	global.old_timing        = TRUE;
	global.leds				 = FALSE;
#ifdef MESS
	global.use_new_ui = TRUE;
	for (i = 0; i < IO_COUNT; i++)
		global.software[i] = strdup("");
#endif

	// game_options[x] is valid iff game_variables[i].options_loaded == true
	game_options = (options_type *)malloc(num_games * sizeof(options_type));
	game_variables = (game_variables_type *)malloc(num_games * sizeof(game_variables_type));

	if (!game_options || !game_variables)
		return FALSE;

	memset(game_options, 0, num_games * sizeof(options_type));
	memset(game_variables, 0, num_games * sizeof(game_variables_type));

	for (i = 0; i < num_games; i++)
	{
		game_variables[i].play_count = 0;
		game_variables[i].has_roms = UNKNOWN;
		game_variables[i].has_samples = UNKNOWN;
		
		game_variables[i].options_loaded = FALSE;
		game_variables[i].use_default = TRUE;
	}

	size_folder_filters = 1;
	num_folder_filters = 0;
	folder_filters = (folder_filter_type *) malloc(size_folder_filters * sizeof(folder_filter_type));
	if (!folder_filters)
		return FALSE;

	LoadOptionsAndSettings();

	// have our mame core (file code) know about our rom path
	// this leaks a little, but the win32 file core writes to this string
	set_pathlist(FILETYPE_ROM,strdup(settings.romdirs));
	set_pathlist(FILETYPE_SAMPLE,strdup(settings.sampledirs));
#ifdef MESS
	set_pathlist(FILETYPE_CRC,strdup(settings.crcdir));
#endif
	return TRUE;

}

void OptionsExit(void)
{
	int i;

	for (i=0;i<num_games;i++)
	{
		FreeGameOptions(&game_options[i]);
	}

    free(game_options);

	FreeGameOptions(&global);

    FreeIfAllocated(&settings.default_game);
    FreeIfAllocated(&settings.language);
    FreeIfAllocated(&settings.romdirs);
    FreeIfAllocated(&settings.sampledirs);
    FreeIfAllocated(&settings.inidir);
    FreeIfAllocated(&settings.cfgdir);
    FreeIfAllocated(&settings.hidir);
    FreeIfAllocated(&settings.inpdir);
    FreeIfAllocated(&settings.imgdir);
    FreeIfAllocated(&settings.statedir);
    FreeIfAllocated(&settings.artdir);
    FreeIfAllocated(&settings.memcarddir);
    FreeIfAllocated(&settings.flyerdir);
    FreeIfAllocated(&settings.cabinetdir);
    FreeIfAllocated(&settings.marqueedir);
    FreeIfAllocated(&settings.titlesdir);
    FreeIfAllocated(&settings.nvramdir);
    FreeIfAllocated(&settings.diffdir);
    FreeIfAllocated(&settings.iconsdir);
    FreeIfAllocated(&settings.bgdir);
	FreeIfAllocated(&settings.cheat_filename);
	FreeIfAllocated(&settings.history_filename);
	FreeIfAllocated(&settings.mameinfo_filename);
    FreeIfAllocated(&settings.ctrlrdir);
	FreeIfAllocated(&settings.folderdir);
}

// frees the sub-data (strings)
void FreeGameOptions(options_type *o)
{
	int i;

	for (i=0;i<NUM_GAME_OPTIONS;i++)
	{
		if (regGameOpts[i].m_iType == RO_STRING)
		{
			char **string_to_free = 
				(char **)((char *)o + ((char *)regGameOpts[i].m_vpData - (char *)&gOpts));
			if (*string_to_free  != NULL)
			{
				free(*string_to_free);
				*string_to_free = NULL;
			}
		}
	}
}

// performs a "deep" copy--strings in source are allocated and copied in dest
void CopyGameOptions(options_type *source,options_type *dest)
{
	int i;

	*dest = *source;

	// now there's a bunch of strings in dest that need to be reallocated
	// to be a separate copy
	for (i=0;i<NUM_GAME_OPTIONS;i++)
	{
		if (regGameOpts[i].m_iType == RO_STRING)
		{
			char **string_to_copy = 
				(char **)((char *)dest + ((char *)regGameOpts[i].m_vpData - (char *)&gOpts));
			if (*string_to_copy != NULL)
			{
				*string_to_copy = strdup(*string_to_copy);
			}
		}
	}
}

options_type * GetDefaultOptions(void)
{
	return &global;
}

options_type * GetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	if (game_variables[driver_index].use_default)
	{
		CopyGameOptions(&global,&game_options[driver_index]);
	}

	if (game_variables[driver_index].options_loaded == FALSE)
	{
		LoadGameOptions(driver_index);
		game_variables[driver_index].options_loaded = TRUE;
	}

	return &game_options[driver_index];
}

BOOL GetGameUsesDefaults(int driver_index)
{
	if (driver_index < 0)
	{
		dprintf("got getgameusesdefaults with driver index %i",driver_index);
		return TRUE;
	}
	return game_variables[driver_index].use_default;
}

void SetGameUsesDefaults(int driver_index,BOOL use_defaults)
{
	if (driver_index < 0)
	{
		dprintf("got setgameusesdefaults with driver index %i",driver_index);
		return;
	}
	game_variables[driver_index].use_default = use_defaults;
}

void LoadFolderFilter(int folder_index,int filters)
{
	//dprintf("loaded folder filter %i %i",folder_index,filters);

	if (num_folder_filters == size_folder_filters)
	{
		size_folder_filters *= 2;
		folder_filters = (folder_filter_type *)realloc(
			folder_filters,size_folder_filters * sizeof(folder_filter_type));
	}
	folder_filters[num_folder_filters].folder_index = folder_index;
	folder_filters[num_folder_filters].filters = filters;

	num_folder_filters++;
}

void ResetGUI(void)
{
	save_gui_settings = FALSE;
}

void SetViewMode(int val)
{
	settings.view = val;
}

int GetViewMode(void)
{
	return settings.view;
}

void SetGameCheck(BOOL game_check)
{
	settings.game_check = game_check;
}

BOOL GetGameCheck(void)
{
	return settings.game_check;
}

void SetVersionCheck(BOOL version_check)
{
	settings.version_check = version_check;
}

BOOL GetVersionCheck(void)
{
	return settings.version_check;
}

void SetJoyGUI(BOOL use_joygui)
{
	settings.use_joygui = use_joygui;
}

BOOL GetJoyGUI(void)
{
	return settings.use_joygui;
}

void SetBroadcast(BOOL broadcast)
{
	settings.broadcast = broadcast;
}

BOOL GetBroadcast(void)
{
	return settings.broadcast;
}

void SetShowDisclaimer(BOOL show_disclaimer)
{
	settings.show_disclaimer = show_disclaimer;
}

BOOL GetShowDisclaimer(void)
{
	return settings.show_disclaimer;
}

void SetShowGameInfo(BOOL show_gameinfo)
{
	settings.show_gameinfo = show_gameinfo;
}

BOOL GetShowGameInfo(void)
{
	return settings.show_gameinfo;
}

void SetHighPriority(BOOL high_priority)
{
	settings.high_priority = high_priority;
}

BOOL GetHighPriority(void)
{
	return settings.high_priority;
}

void SetRandomBackground(BOOL random_bg)
{
	settings.random_bg = random_bg;
}

BOOL GetRandomBackground(void)
{
	return settings.random_bg;
}

void SetSavedFolderID(UINT val)
{
	settings.folder_id = val;
}

UINT GetSavedFolderID(void)
{
	return settings.folder_id;
}

void SetShowScreenShot(BOOL val)
{
	settings.show_screenshot = val;
}

BOOL GetShowScreenShot(void)
{
	return settings.show_screenshot;
}

void SetShowFolderList(BOOL val)
{
	settings.show_folderlist = val;
}

BOOL GetShowFolderList(void)
{
	return settings.show_folderlist;
}

void SetShowStatusBar(BOOL val)
{
	settings.show_statusbar = val;
}

BOOL GetShowStatusBar(void)
{
	return settings.show_statusbar;
}

void SetShowTabCtrl (BOOL val)
{
	settings.show_tabctrl = val;
}

BOOL GetShowTabCtrl (void)
{
	return settings.show_tabctrl;
}

void SetShowToolBar(BOOL val)
{
	settings.show_toolbar = val;
}

BOOL GetShowToolBar(void)
{
	return settings.show_toolbar;
}

void SetShowPictType(int val)
{
	settings.show_pict_type = val;
}

int GetShowPictType(void)
{
	return settings.show_pict_type;
}

void SetDefaultGame(const char *name)
{
	FreeIfAllocated(&settings.default_game);

	if (name != NULL)
		settings.default_game = strdup(name);
}

const char *GetDefaultGame(void)
{
	return settings.default_game;
}

void SetWindowArea(AREA *area)
{
	memcpy(&settings.area, area, sizeof(AREA));
}

void GetWindowArea(AREA *area)
{
	memcpy(area, &settings.area, sizeof(AREA));
}

void SetWindowState(UINT state)
{
	settings.windowstate = state;
}

UINT GetWindowState(void)
{
	return settings.windowstate;
}

void SetListFont(LOGFONT *font)
{
	memcpy(&settings.list_font, font, sizeof(LOGFONT));
}

void GetListFont(LOGFONT *font)
{
	memcpy(font, &settings.list_font, sizeof(LOGFONT));
}

void SetListFontColor(COLORREF uColor)
{
	if (settings.list_font_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_font_color = (COLORREF)-1;
	else
		settings.list_font_color = uColor;
}

COLORREF GetListFontColor(void)
{
	if (settings.list_font_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_font_color;
}

void SetListCloneColor(COLORREF uColor)
{
	if (settings.list_clone_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_clone_color = (COLORREF)-1;
	else
		settings.list_clone_color = uColor;
}

COLORREF GetListCloneColor(void)
{
	if (settings.list_clone_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_clone_color;

}

void SetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_width[i] = width[i];
}

void GetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		width[i] = settings.column_width[i];
}

void SetSplitterPos(int splitterId, int pos)
{
	if (splitterId < GetSplitterCount())
		settings.splitter[splitterId] = pos;
}

int  GetSplitterPos(int splitterId)
{
	if (splitterId < GetSplitterCount())
		return settings.splitter[splitterId];

	return -1; /* Error */
}

void SetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_order[i] = order[i];
}

void GetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		order[i] = settings.column_order[i];
}

void SetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_shown[i] = shown[i];
}

void GetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		shown[i] = settings.column_shown[i];
}

void SetSortColumn(int column)
{
	settings.sort_column = column;
}

int GetSortColumn(void)
{
	return settings.sort_column;
}

void SetSortReverse(BOOL reverse)
{
	settings.sort_reverse = reverse;
}

BOOL GetSortReverse(void)
{
	return settings.sort_reverse;
}

const char* GetLanguage(void)
{
	return settings.language;
}

void SetLanguage(const char* lang)
{
	FreeIfAllocated(&settings.language);

	if (lang != NULL)
		settings.language = strdup(lang);
}

const char* GetRomDirs(void)
{
	return settings.romdirs;
}

void SetRomDirs(const char* paths)
{
	FreeIfAllocated(&settings.romdirs);

	if (paths != NULL)
	{
		settings.romdirs = strdup(paths);

		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		set_pathlist(FILETYPE_ROM,strdup(settings.romdirs));
	}
}

const char* GetSampleDirs(void)
{
	return settings.sampledirs;
}

void SetSampleDirs(const char* paths)
{
	FreeIfAllocated(&settings.sampledirs);

	if (paths != NULL)
	{
		settings.sampledirs = strdup(paths);
		
		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		set_pathlist(FILETYPE_SAMPLE,strdup(settings.sampledirs));
	}

}

const char * GetIniDir(void)
{
	return settings.inidir;
}

void SetIniDir(const char *path)
{
	FreeIfAllocated(&settings.inidir);

	if (path != NULL)
		settings.inidir = strdup(path);
}

const char* GetCtrlrDir(void)
{
	return settings.ctrlrdir;
}

void SetCtrlrDir(const char* path)
{
	FreeIfAllocated(&settings.ctrlrdir);

	if (path != NULL)
		settings.ctrlrdir = strdup(path);
}

const char* GetCfgDir(void)
{
	return settings.cfgdir;
}

void SetCfgDir(const char* path)
{
	FreeIfAllocated(&settings.cfgdir);

	if (path != NULL)
		settings.cfgdir = strdup(path);
}

const char* GetHiDir(void)
{
	return settings.hidir;
}

void SetHiDir(const char* path)
{
	FreeIfAllocated(&settings.hidir);

	if (path != NULL)
		settings.hidir = strdup(path);
}

const char* GetNvramDir(void)
{
	return settings.nvramdir;
}

void SetNvramDir(const char* path)
{
	FreeIfAllocated(&settings.nvramdir);

	if (path != NULL)
		settings.nvramdir = strdup(path);
}

const char* GetInpDir(void)
{
	return settings.inpdir;
}

void SetInpDir(const char* path)
{
	FreeIfAllocated(&settings.inpdir);

	if (path != NULL)
		settings.inpdir = strdup(path);
}

const char* GetImgDir(void)
{
	return settings.imgdir;
}

void SetImgDir(const char* path)
{
	FreeIfAllocated(&settings.imgdir);

	if (path != NULL)
		settings.imgdir = strdup(path);
}

const char* GetStateDir(void)
{
	return settings.statedir;
}

void SetStateDir(const char* path)
{
	FreeIfAllocated(&settings.statedir);

	if (path != NULL)
		settings.statedir = strdup(path);
}

const char* GetArtDir(void)
{
	return settings.artdir;
}

void SetArtDir(const char* path)
{
	FreeIfAllocated(&settings.artdir);

	if (path != NULL)
		settings.artdir = strdup(path);
}

const char* GetMemcardDir(void)
{
	return settings.memcarddir;
}

void SetMemcardDir(const char* path)
{
	FreeIfAllocated(&settings.memcarddir);

	if (path != NULL)
		settings.memcarddir = strdup(path);
}

const char* GetFlyerDir(void)
{
	return settings.flyerdir;
}

void SetFlyerDir(const char* path)
{
	FreeIfAllocated(&settings.flyerdir);

	if (path != NULL)
		settings.flyerdir = strdup(path);
}

const char* GetCabinetDir(void)
{
	return settings.cabinetdir;
}

void SetCabinetDir(const char* path)
{
	FreeIfAllocated(&settings.cabinetdir);

	if (path != NULL)
		settings.cabinetdir = strdup(path);
}

const char* GetMarqueeDir(void)
{
	return settings.marqueedir;
}

void SetMarqueeDir(const char* path)
{
	FreeIfAllocated(&settings.marqueedir);

	if (path != NULL)
		settings.marqueedir = strdup(path);
}

const char* GetTitlesDir(void)
{
	return settings.titlesdir;
}

void SetTitlesDir(const char* path)
{
	FreeIfAllocated(&settings.titlesdir);

	if (path != NULL)
		settings.titlesdir = strdup(path);
}

const char* GetDiffDir(void)
{
	return settings.diffdir;
}

void SetDiffDir(const char* path)
{
	FreeIfAllocated(&settings.diffdir);

	if (path != NULL)
		settings.diffdir = strdup(path);
}

const char* GetIconsDir(void)
{
	return settings.iconsdir;
}

void SetIconsDir(const char* path)
{
	FreeIfAllocated(&settings.iconsdir);

	if (path != NULL)
		settings.iconsdir = strdup(path);
}

const char* GetBgDir (void)
{
	return settings.bgdir;
}

void SetBgDir (const char* path)
{
	FreeIfAllocated(&settings.bgdir);

	if (path != NULL)
		settings.bgdir = strdup (path);
}

const char* GetFolderDir(void)
{
	return settings.folderdir;
}

void SetFolderDir(const char* path)
{
	FreeIfAllocated(&settings.folderdir);

	if (path != NULL)
		settings.folderdir = strdup(path);
}

const char* GetCheatFileName(void)
{
	return settings.cheat_filename;
}

void SetCheatFileName(const char* path)
{
	FreeIfAllocated(&settings.cheat_filename);

	if (path != NULL)
		settings.cheat_filename = strdup(path);
}

const char* GetHistoryFileName(void)
{
	return settings.history_filename;
}

void SetHistoryFileName(const char* path)
{
	FreeIfAllocated(&settings.history_filename);

	if (path != NULL)
		settings.history_filename = strdup(path);
}


const char* GetMAMEInfoFileName(void)
{
	return settings.mameinfo_filename;
}

void SetMAMEInfoFileName(const char* path)
{
	FreeIfAllocated(&settings.mameinfo_filename);

	if (path != NULL)
		settings.mameinfo_filename = strdup(path);
}

void ResetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	// make sure it's all loaded up.
	GetGameOptions(driver_index);

	if (game_variables[driver_index].use_default == FALSE)
	{
		FreeGameOptions(&game_options[driver_index]);
		game_variables[driver_index].use_default = TRUE;
		
		// this will delete the custom file
		SaveGameOptions(driver_index);
	}
}

void ResetGameDefaults(void)
{
	save_default_options = FALSE;
}

void ResetAllGameOptions(void)
{
	int i;

	for (i = 0; i < num_games; i++)
	{
		ResetGameOptions(i);
	}
}

int GetHasRoms(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].has_roms;
}

void SetHasRoms(int driver_index, int has_roms)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].has_roms = has_roms;
}

int  GetHasSamples(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].has_samples;
}

void SetHasSamples(int driver_index, int has_samples)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].has_samples = has_samples;
}

void IncrementPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].play_count++;

	// maybe should do this
	//SavePlayCount(driver_index);
}

int GetPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].play_count;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void ColumnEncodeStringWithCount(void* data, char *str, int count)
{
	int* value = (int*)data;
	int  i;
	char buffer[100];

	snprintf(buffer,sizeof(buffer),"%d",value[0]);
	
	strcpy(str,buffer);

    for (i = 1; i < count; i++)
	{
		snprintf(buffer,sizeof(buffer),",%d",value[i]);
		strcat(str,buffer);
	}
}

static void ColumnDecodeStringWithCount(const char* str, void* data, int count)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
    for (i = 0; p && i < count; i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
    }
	}

static void ColumnEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, COLUMN_MAX);
}

static void ColumnDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, COLUMN_MAX);
}

static void ColumnDecodeWidths(const char* str, void* data)
{
	if (settings.view == VIEW_REPORT || settings.view == VIEW_GROUPED)
		ColumnDecodeString(str, data);
}

static void SplitterEncodeString(void* data, char* str)
{
	int* value = (int*)data;
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < GetSplitterCount(); i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void SplitterDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
	for (i = 0; p && i < GetSplitterCount(); i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}

static void ListDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int i;

	*value = VIEW_GROUPED;

	for (i = VIEW_LARGE_ICONS; i < VIEW_MAX; i++)
	{
		if (strcmp(str, view_modes[i]) == 0)
		{
			*value = i;
			return;
		}
	}
}

static void ListEncodeString(void* data, char *str)
{
	int* value = (int*)data;

	strcpy(str, view_modes[*value]);
}

/* Parse the given comma-delimited string into a LOGFONT structure */
static void FontDecodeString(const char* str, void* data)
{
	LOGFONT* f = (LOGFONT*)data;
	char*	 ptr;
	
	sscanf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i",
		   &f->lfHeight,
		   &f->lfWidth,
		   &f->lfEscapement,
		   &f->lfOrientation,
		   &f->lfWeight,
		   (int*)&f->lfItalic,
		   (int*)&f->lfUnderline,
		   (int*)&f->lfStrikeOut,
		   (int*)&f->lfCharSet,
		   (int*)&f->lfOutPrecision,
		   (int*)&f->lfClipPrecision,
		   (int*)&f->lfQuality,
		   (int*)&f->lfPitchAndFamily);
	ptr = strrchr(str, ',');
	if (ptr != NULL)
		strcpy(f->lfFaceName, ptr + 1);
}

/* Encode the given LOGFONT structure into a comma-delimited string */
static void FontEncodeString(void* data, char *str)
{
	LOGFONT* f = (LOGFONT*)data;

	sprintf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i,%s",
			f->lfHeight,
			f->lfWidth,
			f->lfEscapement,
			f->lfOrientation,
			f->lfWeight,
			f->lfItalic,
			f->lfUnderline,
			f->lfStrikeOut,
			f->lfCharSet,
			f->lfOutPrecision,
			f->lfClipPrecision,
			f->lfQuality,
			f->lfPitchAndFamily,
			f->lfFaceName);
}

static REG_OPTION * GetOption(REG_OPTION *option_array,int num_options,const char *key)
{
	int i;

	for (i=0;i<num_options;i++)
	{
		if (option_array[i].ini_name[0] != '\0')
		{
			if (strcmp(option_array[i].ini_name,key) == 0)
				return &option_array[i];
		}
	}
	return NULL;
}

static void LoadOption(REG_OPTION *option,const char *value_str)
{
	//dprintf("trying to load %s type %i [%s]",option->ini_name,option->m_iType,value_str);

	switch (option->m_iType)
	{
	case RO_DOUBLE :
		*((double *)option->m_vpData) = atof(value_str);
		break;

	case RO_COLOR :
	{
		unsigned int r,g,b;
		if (sscanf(value_str,"%ui,%ui,%ui",&r,&g,&b) == 3)
			*((COLORREF *)option->m_vpData) = RGB(r,g,b);

		break;
	}

	case RO_STRING:
		if (*(char**)option->m_vpData != NULL)
			free(*(char**)option->m_vpData);
		*(char **)option->m_vpData = strdup(value_str);
		break;

	case RO_BOOL:
	{
		int value_int;
		if (sscanf(value_str,"%i",&value_int) == 1)
			*((int *)option->m_vpData) = (value_int != 0);
		break;
	}

	case RO_INT:
	{
		int value_int;
		if (sscanf(value_str,"%i",&value_int) == 1)
			*((int *)option->m_vpData) = value_int;
		break;
	}

	case RO_ENCODE:
		option->decode(value_str,option->m_vpData);
		break;

	default:
		break;
	}
	
}

static BOOL LoadGameVariableOrFolderFilter(char *key,const char *value)
{
	REG_OPTION fake_option;

	const char *suffix;

	suffix = "_play_count";
	if (StringIsSuffixedBy(key, suffix))
	{
		int driver_index;

		key[strlen(key) - strlen(suffix)] = '\0';
		driver_index = GetGameNameIndex(key);
		if (driver_index < 0)
		{
			dprintf("error loading game variable for invalid game %s",key);
			return TRUE;
		}

		strcpy(fake_option.ini_name,"drivername_play_count");
		fake_option.m_iType = RO_INT;
		fake_option.m_vpData = &game_variables[driver_index].play_count;
		LoadOption(&fake_option,value);
		return TRUE;
	}

	suffix = "_have_roms";
	if (StringIsSuffixedBy(key, suffix))
	{
		int driver_index;

		key[strlen(key) - strlen(suffix)] = '\0';
		driver_index = GetGameNameIndex(key);
		if (driver_index < 0)
		{
			dprintf("error loading game variable for invalid game %s",key);
			return TRUE;
		}

		strcpy(fake_option.ini_name,"drivername_have_roms");
		fake_option.m_iType = RO_BOOL;
		fake_option.m_vpData = &game_variables[driver_index].has_roms;
		LoadOption(&fake_option,value);
		return TRUE;
	}

	suffix = "_have_samples";
	if (StringIsSuffixedBy(key, suffix))
	{
		int driver_index;

		key[strlen(key) - strlen(suffix)] = '\0';
		driver_index = GetGameNameIndex(key);
		if (driver_index < 0)
		{
			dprintf("error loading game variable for invalid game %s",key);
			return TRUE;
		}

		strcpy(fake_option.ini_name,"drivername_have_samples");
		fake_option.m_iType = RO_BOOL;
		fake_option.m_vpData = &game_variables[driver_index].has_samples;
		LoadOption(&fake_option,value);
		return TRUE;
	}

	suffix = "_filters";
	if (StringIsSuffixedBy(key, suffix))
	{
		int folder_index;
		int filters;

		key[strlen(key) - strlen(suffix)] = '\0';
		if (sscanf(key,"%i",&folder_index) != 1)
		{
			dprintf("error loading game variable for invalid game %s",key);
			return TRUE;
		}
		if (folder_index < 0)
			return TRUE;

		if (sscanf(value,"%i",&filters) != 1)
			return TRUE;

		LoadFolderFilter(folder_index,filters);
		return TRUE;
	}

#ifdef MESS
	suffix = "_extra_software";
	if (StringIsSuffixedBy(key, suffix))
	{
		int driver_index;

		key[strlen(key) - strlen(suffix)] = '\0';
		driver_index = GetGameNameIndex(key);
		if (driver_index < 0)
		{
			dprintf("error loading game variable for invalid game %s",key);
			return TRUE;
		}

		strcpy(fake_option.ini_name,"drivername_extra_software");
		fake_option.m_iType = RO_STRING;
		fake_option.m_vpData = &game_variables[driver_index].extra_software_paths;
		LoadOption(&fake_option, value);
		return TRUE;
	}
#endif

	return FALSE;
}

// out of a string, parse two substrings (non-blanks, or quoted).
// we modify buffer, and return key and value to point to the substrings, or NULL
static void ParseKeyValueStrings(char *buffer,char **key,char **value)
{
	char *ptr;
	BOOL quoted;

	*key = NULL;
	*value = NULL;

	ptr = buffer;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	*key = ptr;
	quoted = FALSE;
	while (1)
	{
		if (*ptr == 0 || (!quoted && (*ptr == ' ' || *ptr == '\t')) || (quoted && *ptr == '\"'))
		{
			// found end of key
			if (*ptr == 0)
				return;

			*ptr = '\0';
			ptr++;
			break;
		}

		if (*ptr == '\"')
			quoted = TRUE;
		ptr++;
	}

	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	*value = ptr;
	quoted = FALSE;
	while (1)
	{
		if (*ptr == 0 || (!quoted && (*ptr == ' ' || *ptr == '\t')) || (quoted && *ptr == '\"'))
		{
			// found end of value;
			*ptr = '\0';
			break;
		}

		if (*ptr == '\"')
			quoted = TRUE;
		ptr++;
	}

	if (**key == '\"')
		(*key)++;

	if (**value == '\"')
		(*value)++;

	if (**key == '\0' || **value == '\0')
	{
		*key = NULL;
		*value = NULL;
	}
}

/* Register access functions below */
static void LoadOptionsAndSettings(void)
{
	HKEY    hKey;
	LONG    result;
	int     i;
	BOOL    bResetDefs = FALSE;
	BOOL    bVersionCheck = TRUE;

	char buffer[512];
	FILE *fptr;

	fptr = fopen(UI_INI_FILENAME,"rt");
	if (fptr != NULL)
	{
		while (fgets(buffer,sizeof(buffer),fptr) != NULL)
		{
			char *key,*value_str;
			REG_OPTION *option;

			if (buffer[0] == '\0' || buffer[0] == '#')
				continue;
			// we're guaranteed that strlen(buffer) >= 1 now
			buffer[strlen(buffer)-1] = '\0';

			ParseKeyValueStrings(buffer,&key,&value_str);
			if (key == NULL || value_str == NULL)
			{
				//dprintf("invalid line [%s]",buffer);
				continue;
			}
			option = GetOption(regSettings,NUM_SETTINGS,key);
			if (option == NULL)
			{
				// search for game_have_rom/have_sample/play_count thing
				if (LoadGameVariableOrFolderFilter(key,value_str) == FALSE)
				{
					dprintf("found unknown option %s",key);
				}
			}
			else
			{
				LoadOption(option,value_str);
			}

		}

		fclose(fptr);
	}
	else
	{	
	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */

	result = RegOpenKeyEx( HKEY_CURRENT_USER,
                           KEY_BASE,
                           0, 
                           KEY_QUERY_VALUE, 
                           &hKey );
	if (result == ERROR_SUCCESS)
	{
		BOOL bReset = FALSE;

        /* force reset of configuration? */
		GetRegBoolOption(hKey, "ResetGUI",			&bReset);

        /* reset all games to default? */
		GetRegBoolOption(hKey, "ResetGameDefaults", &bResetDefs);

        /* perform version check? */
		GetRegBoolOption(hKey, "VersionCheck",		&bVersionCheck);

		if (bReset)
		{
            /* Reset configuration (read from .Backup if available) */

			RegCloseKey(hKey);

			/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Backup */

			if ( RegOpenKeyEx( HKEY_CURRENT_USER, 
                               KEY_BASE_DOTBACKUP,
							   0, 
                               KEY_QUERY_VALUE, 
                               &hKey ) != ERROR_SUCCESS )
            {
				return;
            }
		}

		RegDeleteValue(hKey,"ListDetails");

        /* read settings */

		for (i = 0; i < NUM_SETTINGS; i++)
        {
			GetRegObj(hKey, &regSettings[i]);
        }

		RegCloseKey(hKey);
	}

	}

	snprintf(buffer,sizeof(buffer),"%s\\%s",GetIniDir(),DEFAULT_OPTIONS_INI_FILENAME);
	gOpts = global;
	if (LoadOptions(buffer,&global,TRUE))
	{
		global = gOpts;
		ZeroMemory(&gOpts,sizeof(gOpts));
	}
	else
	{
		/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Defaults */

		result = RegOpenKeyEx( HKEY_CURRENT_USER, 
							   bResetDefs ? KEY_BACKUP_DOTDEFAULTS : KEY_BASE_DOTDEFAULTS, 
							   0, 
							   KEY_QUERY_VALUE, 
							   &hKey );
		if (result == ERROR_SUCCESS)
		{
			/* read game default options */
			
			LoadRegGameOptions(hKey, &global, -1);
			RegCloseKey(hKey);
		}
	}

#ifdef UPGRADE
	for (i = 0 ; i < num_games; i++)
	{
		LoadGameOptions(i);
	}
#endif

}

void LoadGameOptions(int driver_index)
{
    char    keyString[80];
	HKEY    hKey;
	LONG    result;

	char buffer[512];

    strcpy( keyString, KEY_BASE_FMT );

	snprintf(buffer,sizeof(buffer),"%s\\%s.ini",GetIniDir(),drivers[driver_index]->name);
	
	CopyGameOptions(&global,&gOpts);
	if (LoadOptions(buffer,&game_options[driver_index],FALSE))
	{
		// successfully loaded
		game_options[driver_index] = gOpts;
		game_variables[driver_index].use_default = FALSE;
	}
	else
	{
		strcpy( keyString + KEY_BASE_FMT_CCH, drivers[driver_index]->name );
		
		result = RegOpenKeyEx(HKEY_CURRENT_USER, keyString, 0, KEY_QUERY_VALUE, &hKey);
		if (result == ERROR_SUCCESS)
		{
			LoadRegGameOptions(hKey, &game_options[driver_index], driver_index);
			RegCloseKey(hKey);
		}
	}
}

static BOOL LoadOptions(const char *filename,options_type *o,BOOL load_global_game_options)
{
	FILE *fptr;
	char buffer[512];

	fptr = fopen(filename,"rt");
	if (fptr == NULL)
		return FALSE;

	while (fgets(buffer,sizeof(buffer),fptr) != NULL)
	{
		char *key,*value_str;
		REG_OPTION *option;

		if (buffer[0] == '\0' || buffer[0] == '#')
			continue;
		// we're guaranteed that strlen(buffer) >= 1 now
		buffer[strlen(buffer)-1] = '\0';
		
		ParseKeyValueStrings(buffer,&key,&value_str);
		if (key == NULL || value_str == NULL)
		{
			dprintf("invalid line [%s]",buffer);
			continue;
		}
		option = GetOption(regGameOpts,NUM_GAME_OPTIONS,key);
		if (option == NULL)
		{
			if (load_global_game_options)
				option = GetOption(global_game_options,NUM_GLOBAL_GAME_OPTIONS,key);
			
			if (option == NULL)
			{
				dprintf("load game options found unknown option %s",key);
				continue;
			}
		}

		//dprintf("loading option <%s> <%s>",option,value_str);
		LoadOption(option,value_str);
	}

	fclose(fptr);
	return TRUE;
}

static DWORD GetRegOption(HKEY hKey, const char *name)
{
	DWORD dwType;
	DWORD dwSize;
	DWORD value = -1;

	if (RegQueryValueEx(hKey, name, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
	{
		if (dwType == REG_DWORD)
		{
			dwSize = 4;
			RegQueryValueEx(hKey, name, 0, &dwType, (LPBYTE)&value, &dwSize);
		}
	}
	return value;
}

static void GetRegBoolOption(HKEY hKey, const char *name, BOOL *value)
{
	char *ptr;

	if ((ptr = GetRegStringOption(hKey, name)) != NULL)
	{
		*value = (*ptr == '0') ? FALSE : TRUE;
	}
}

static char *GetRegStringOption(HKEY hKey, const char *name)
{
	DWORD dwType;
	DWORD dwSize;
	static char str[300];

	memset(str, '\0', 300);

	if (RegQueryValueEx(hKey, name, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
	{
		if (dwType == REG_SZ)
		{
			dwSize = 299;
			RegQueryValueEx(hKey, name, 0, &dwType, (unsigned char *)str, &dwSize);
		}
	}
	else
	{
		return NULL;
	}

	return str;
}

DWORD GetFolderFlags(int folder_index)
{
	int i;

	for (i=0;i<num_folder_filters;i++)
		if (folder_filters[i].folder_index == folder_index)
		{
			//dprintf("found folder filter %i %i",folder_index,folder_filters[i].filters);
			return folder_filters[i].filters;
		}
	return 0;
}

void SaveOptions(void)
{
	SaveSettings();

#ifdef UPGRADE
{
	int i;

	SaveDefaultOptions();

	for (i=0;i<num_games;i++)
		SaveGameOptions(i);
}
#endif

}


void SaveGameOptions(int driver_index)
{
	int i;
	FILE *fptr;
	char buffer[512];
	BOOL options_different = FALSE;

	if (game_variables[driver_index].use_default == FALSE)
	{
		for (i=0;i<NUM_GAME_OPTIONS;i++)
		{
			if (IsOptionEqual(i,&game_options[driver_index],&global) == FALSE)
			{
				options_different = TRUE;
			}

		}
	}

	snprintf(buffer,sizeof(buffer),"%s\\%s.ini",GetIniDir(),drivers[driver_index]->name);
	if (options_different)
	{
		fptr = fopen(buffer,"wt");
		if (fptr != NULL)
		{
			fprintf(fptr,"### ");
			fprintf(fptr,"%s",drivers[driver_index]->name);
			fprintf(fptr," ###\n\n");

			for (i=0;i<NUM_GAME_OPTIONS;i++)
			{
				if (IsOptionEqual(i,&game_options[driver_index],&global) == FALSE)
				{
					gOpts = game_options[driver_index];
					WriteOptionToFile(fptr,&regGameOpts[i]);
				}
			}

			fclose(fptr);
		}
	}
	else
	{
		if (DeleteFile(buffer) == 0)
		{
			dprintf("error deleting %s; error %d\n",buffer, GetLastError());
		}
	}
}

void SaveDefaultOptions(void)
{
	int i;
	FILE *fptr;
	char buffer[512];

	snprintf(buffer,sizeof(buffer),"%s\\%s",GetIniDir(),DEFAULT_OPTIONS_INI_FILENAME);

	fptr = fopen(buffer,"wt");
	if (fptr != NULL)
	{
		fprintf(fptr,"### " DEFAULT_OPTIONS_INI_FILENAME " ###\n\n");
		
		if (save_gui_settings)
		{
			fprintf(fptr,"### global-only options ###\n\n");
		
			for (i=0;i<NUM_GLOBAL_GAME_OPTIONS;i++)
			{
				if (!global_game_options[i].m_bOnlyOnGame)
					WriteOptionToFile(fptr,&global_game_options[i]);
			}
		}

		if (save_default_options)
		{
			fprintf(fptr,"\n### default game options ###\n\n");
			gOpts = global;
			for (i = 0; i < NUM_GAME_OPTIONS; i++)
			{
				if (!regGameOpts[i].m_bOnlyOnGame)
					WriteOptionToFile(fptr, &regGameOpts[i]);
			}
		}

		fclose(fptr);
	}
}

static void WriteStringOptionToFile(FILE *fptr,const char *key,const char *value)
{
	if (value[0] && !strchr(value,' '))
		fprintf(fptr,"%s %s\n",key,value);
	else
		fprintf(fptr,"%s \"%s\"\n",key,value);
}

static void WriteIntOptionToFile(FILE *fptr,const char *key,int value)
{
	fprintf(fptr,"%s %i\n",key,value);
}

static void WriteBoolOptionToFile(FILE *fptr,const char *key,BOOL value)
{
	fprintf(fptr,"%s %i\n",key,value ? 1 : 0);
}

static void WriteColorOptionToFile(FILE *fptr,const char *key,COLORREF value)
{
	fprintf(fptr,"%s %i,%i,%i\n",key,(int)(value & 0xff),(int)((value >> 8) & 0xff),
			(int)((value >> 16) & 0xff));
}

static BOOL IsOptionEqual(int option_index,options_type *o1,options_type *o2)
{
	switch (regGameOpts[option_index].m_iType)
	{
	case RO_DOUBLE:
	{
		double a,b;
		gOpts = *o1;
		a = *(double *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(double *)regGameOpts[option_index].m_vpData;
		return fabs(a-b) < 0.000001;
	}
	case RO_COLOR :
	{
		COLORREF a,b;
		gOpts = *o1;
		a = *(COLORREF *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(COLORREF *)regGameOpts[option_index].m_vpData;
		return a == b;
	}
	case RO_STRING:
	{
		char *a,*b;
		gOpts = *o1;
		a = *(char **)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(char **)regGameOpts[option_index].m_vpData;
		return strcmp(a,b) == 0;
	}
	case RO_BOOL:
	{
		BOOL a,b;
		gOpts = *o1;
		a = *(BOOL *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(BOOL *)regGameOpts[option_index].m_vpData;
		return a == b;
	}
	case RO_INT:
	{
		int a,b;
		gOpts = *o1;
		a = *(int *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(int *)regGameOpts[option_index].m_vpData;
		return a == b;
	}
	case RO_ENCODE:
	{
		char a[500],b[500];
		gOpts = *o1;
		regGameOpts[option_index].encode(regGameOpts[option_index].m_vpData,a);
		gOpts = *o2;
		regGameOpts[option_index].encode(regGameOpts[option_index].m_vpData,b);
		return strcmp(a,b);
	}

	default:
		break;
	}

	return TRUE;
}


static void WriteOptionToFile(FILE *fptr,REG_OPTION *regOpt)
{
	BOOL*	pBool;
	int*	pInt;
	char*	pString;
	double* pDouble;
	const char *key = regOpt->ini_name;
	char	cTemp[80];
	
	switch (regOpt->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpt->m_vpData;
        _gcvt( *pDouble, 10, cTemp );
		WriteStringOptionToFile(fptr, key, cTemp);
		break;

	case RO_COLOR :
	{
		COLORREF color = *(COLORREF *)regOpt->m_vpData;
		if (color != (COLORREF)-1)
			WriteColorOptionToFile(fptr,key,color);
		break;
	}

	case RO_STRING:
		pString = *((char **)regOpt->m_vpData);
		if (pString)
		    WriteStringOptionToFile(fptr, key, pString);
		break;

	case RO_BOOL:
		pBool = (BOOL*)regOpt->m_vpData;
		WriteBoolOptionToFile(fptr, key, *pBool);
		break;

	case RO_INT:
		pInt = (int*)regOpt->m_vpData;
		WriteIntOptionToFile(fptr, key, *pInt);
		break;

	case RO_ENCODE:
		regOpt->encode(regOpt->m_vpData, cTemp);
		WriteStringOptionToFile(fptr, key, cTemp);
		break;

	default:
		break;
	}

}

static void SaveSettings(void)
{
	int i;
	FILE *fptr;

	fptr = fopen(UI_INI_FILENAME,"wt");
	if (fptr != NULL)
	{
		fprintf(fptr,"### " UI_INI_FILENAME " ###\n\n");
		fprintf(fptr,"### interface ###\n\n");
		
		if (save_gui_settings)
		{
			for (i=0;i<NUM_SETTINGS;i++)
			{
				if ((regSettings[i].ini_name[0] != '\0') && !regSettings[i].m_bOnlyOnGame)
					WriteOptionToFile(fptr,&regSettings[i]);
			}
		}
		
		fprintf(fptr,"\n");
		fprintf(fptr,"### folder filters ###\n\n");
		
		for (i=0;i<GetNumFolders();i++)
		{
			LPTREEFOLDER lpFolder = GetFolder(i);

			if ((lpFolder->m_dwFlags & F_MASK) != 0)
			{
				fprintf(fptr,"# %s\n",lpFolder->m_lpTitle);
				fprintf(fptr,"%i_filters %i\n",i,(int)(lpFolder->m_dwFlags & F_MASK));
			}
		}

		fprintf(fptr,"\n");
		fprintf(fptr,"### game variables ###\n\n");
		for (i=0;i<num_games;i++)
		{
			int driver_index = GetIndexFromSortedIndex(i); 
			// need to improve this to not save too many
			if (game_variables[driver_index].play_count != 0)
			{
				fprintf(fptr,"%s_play_count %i\n",
						drivers[driver_index]->name,game_variables[driver_index].play_count);
			}
			
			if (game_variables[driver_index].has_roms != UNKNOWN)
			{
				fprintf(fptr,"%s_have_roms %i\n",
						drivers[driver_index]->name,game_variables[driver_index].has_roms);
			}
			if (DriverUsesSamples(driver_index) && 
				game_variables[driver_index].has_samples != UNKNOWN)
			{
				fprintf(fptr,"%s_have_samples %i\n",
						drivers[driver_index]->name,game_variables[driver_index].has_samples);
			}
#ifdef MESS
			if (game_variables[driver_index].extra_software_paths && game_variables[driver_index].extra_software_paths[0])
			{
				fprintf(fptr,"%s_extra_software %s\n",
						drivers[driver_index]->name,game_variables[driver_index].extra_software_paths);
			}
#endif
		}
		fclose(fptr);
	}
}

static void LoadRegGameOptions(HKEY hKey, options_type *o, int driver_index)
{
	int 	i;
	DWORD	value;
	DWORD	size;

	assert(driver_index < num_games);

	/* look for window.  If it's not there, then use default options for this game */
	if (RegQueryValueEx(hKey, "window", 0, &value, NULL, &size) != ERROR_SUCCESS)
	   return;

	if (driver_index >= 0)
		game_variables[driver_index].use_default = FALSE;

	/* copy passed in options to our struct */
	gOpts = *o;
	
	for (i = 0; i < NUM_GAME_OPTIONS; i++)
		GetRegObj(hKey, &regGameOpts[i]);

	/* copy options back out */
	*o = gOpts;
}

static void GetRegObj(HKEY hKey, REG_OPTION * regOpts)
{
	char*	cName = regOpts->m_cName;
	char*	pString;
	int*	pInt;
	double* pDouble;
	int 	value;
	
	switch (regOpts->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpts->m_vpData;
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
        {
            *pDouble = atof( pString );
        }
		break;

	case RO_COLOR :
	{
		unsigned int r,g,b;
		char *value_str = GetRegStringOption(hKey,cName);
		if (value_str != NULL && sscanf(value_str,"%ui,%ui,%ui",&r,&g,&b) == 3)
			*((COLORREF *)regOpts->m_vpData) = RGB(r,g,b);

		break;
	}

	case RO_STRING:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
		{
			if (*(char**)regOpts->m_vpData != NULL)
				free(*(char**)regOpts->m_vpData);
			*(char**)regOpts->m_vpData = strdup(pString);
		}
		break;

	case RO_BOOL:
		GetRegBoolOption(hKey, cName, (BOOL*)regOpts->m_vpData);
		break;

	case RO_INT:
		pInt = (BOOL*)regOpts->m_vpData;
		value = GetRegOption(hKey, cName);
		if (value != -1)
			*pInt = value;
		break;

	case RO_ENCODE:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
			regOpts->decode(pString, regOpts->m_vpData);
		break;

	default:
		break;
	}
	
}

char* GetVersionString(void)
{
	return build_version;
}

/* End of options.c */
