//============================================================
//
//	tapedlg.c - Win32 MESS tape control
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

// MAME/MESS headers
#include "tapedlg.h"
#include "mess.h"
#include "messres.h"
#include "windows/window.h"

//============================================================
//	PARAMETERS
//============================================================

#define WNDLONG_DIALOG			GWLP_USERDATA

//============================================================
//	STRUCTURES
//============================================================

struct tape_dialog
{
	HWND window;
	HWND slider;
	HWND caption;
	HWND status;
	UINT_PTR timer;
};

#if HAS_WAVE
//============================================================
//	LOCAL VARIABLES
//============================================================

static struct tape_dialog tape_dialogs[MAX_DEV_INSTANCES];

//============================================================
//	get_tapedialog
//============================================================

static struct tape_dialog *get_tapedialog(HWND dialog)
{
	LONG_PTR lp = GetWindowLongPtr(dialog, WNDLONG_DIALOG);
	return (struct tape_dialog *) lp;
}

//============================================================
//	tapedialog_timerproc
//============================================================

static void CALLBACK tapedialog_timerproc(HWND dialog, UINT msg, UINT_PTR idevent, DWORD time)
{
	struct tape_dialog *dlg;
	int id;
	int curpos, endpos;
	char tapestatus[32];
	mess_image *img;

	dlg = get_tapedialog(dialog);
	id = dlg - tape_dialogs;

	img = image_from_devtype_and_index(IO_CASSETTE, id);

	tapecontrol_gettime(tapestatus, sizeof(tapestatus) / sizeof(tapestatus[0]),
		img, &curpos, &endpos);

	SendMessage(dlg->slider, TBM_SETRANGEMIN, FALSE, 0);
	SendMessage(dlg->slider, TBM_SETRANGEMAX, FALSE, endpos);
	SendMessage(dlg->slider, TBM_SETPOS, TRUE, curpos);

	SetWindowText(dlg->status, tapestatus);
}

//============================================================
//	tapedialog_dlgproc
//============================================================

static INT_PTR CALLBACK tapedialog_dlgproc(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam)
{
	INT_PTR result = 0;
	struct tape_dialog *dlg;

	switch(msg) {
	case WM_INITDIALOG:
		dlg = &tape_dialogs[lparam];
		SetWindowLongPtr(dialog, WNDLONG_DIALOG, (LONG_PTR) dlg);
		dlg->window = dialog;
		dlg->timer = SetTimer(dialog, (UINT_PTR) 0, 100, tapedialog_timerproc);
		dlg->slider = GetDlgItem(dialog, IDC_SLIDER);
		dlg->caption = GetDlgItem(dialog, IDC_CAPTION);
		dlg->status = GetDlgItem(dialog, IDC_STATUS);
		ShowWindow(dialog, SW_SHOW);
		result = 1;
		break;

	case WM_CLOSE:
		KillTimer(dialog, (UINT_PTR) 0);
		dlg = get_tapedialog(dialog);
		memset(dlg, 0, sizeof(*dlg));
		break;
	}
	return result;
}

//============================================================
//	tapedialog_init
//============================================================

void tapedialog_init(void)
{
	memset(tape_dialogs, 0, sizeof(tape_dialogs));
}

//============================================================
//	tapedialog_show
//============================================================

void tapedialog_show(int id)
{
	HMODULE module;

	if (!win_window_mode)
		win_toggle_full_screen();

	if (tape_dialogs[id].window)
	{
		SetWindowPos(tape_dialogs[id].window, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
	else
	{
		module = GetModuleHandle(EMULATORDLL);
		CreateDialogParam(module, MAKEINTRESOURCE(IDD_TAPEDIALOG),
			win_video_window, tapedialog_dlgproc, id);
	}
}
#endif /* HAS_WAVE */

