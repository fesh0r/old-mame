//============================================================
//
//	dialog.c - Win32 MESS dialogs handling
//
//============================================================

#include <windows.h>

#include "dialog.h"
#include "mame.h"
#include "../windows/window.h"
#include "ui_text.h"
#include "inputx.h"
#include "utils.h"
#include "strconv.h"
#include "mscommon.h"

#ifdef UNDER_CE
#include "invokegx.h"
#endif

//============================================================
//	These defines are necessary because the MinGW headers do
//	not have the latest definitions
//============================================================

#ifndef _WIN64
#ifndef GetWindowLongPtr
#define GetWindowLongPtr(hwnd, idx)			((LONG_PTR) GetWindowLong((hwnd), (idx)))
#endif

#ifndef SetWindowLongPtr
#define SetWindowLongPtr(hwnd, idx, val)	((LONG_PTR) SetWindowLong((hwnd), (idx), (val)))
#endif

#ifndef GWLP_USERDATA
#define GWLP_USERDATA						GWL_USERDATA
#endif

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC						GWL_WNDPROC
#endif
#endif /* _WIN64 */

//============================================================

enum
{
	TRIGGER_INITDIALOG	= 1,
	TRIGGER_APPLY		= 2
};

typedef LRESULT (*trigger_function)(HWND dlgwnd, UINT message, WPARAM wparam, LPARAM lparam);

struct dialog_info_trigger
{
	struct dialog_info_trigger *next;
	WORD dialog_item;
	WORD trigger_flags;
	UINT message;
	WPARAM wparam;
	LPARAM lparam;
	UINT16 *result;
	trigger_function trigger_proc;
};

struct dialog_info
{
	HGLOBAL handle;
	size_t handle_size;
	struct dialog_info_trigger *trigger_first;
	struct dialog_info_trigger *trigger_last;
	WORD item_count;
	WORD cx, cy;
	int combo_string_count;
	int combo_default_value;
	void *memory_pool;
};

//============================================================
//	IMPORTS
//============================================================

// from input.c
extern void win_poll_input(void);


//============================================================
//	PARAMETERS
//============================================================

#define DIM_VERTICAL_SPACING	2
#define DIM_HORIZONTAL_SPACING	2
#define DIM_NORMAL_ROW_HEIGHT	10
#define DIM_COMBO_ROW_HEIGHT	12
#define DIM_BUTTON_ROW_HEIGHT	12
#define DIM_LABEL_WIDTH			80
#define DIM_SEQ_WIDTH			120
#define DIM_COMBO_WIDTH			140
#define DIM_BUTTON_WIDTH		50

#define WNDLONG_DIALOG			GWLP_USERDATA

#define DLGITEM_BUTTON			0x0080
#define DLGITEM_EDIT			0x0081
#define DLGITEM_STATIC			0x0082
#define DLGITEM_LISTBOX			0x0083
#define DLGITEM_SCROLLBAR		0x0084
#define DLGITEM_COMBOBOX		0x0085

#define DLGTEXT_OK				ui_getstring(UI_OK)
#define DLGTEXT_APPLY			"Apply"
#define DLGTEXT_CANCEL			"Cancel"

#define FONT_SIZE				8
#define FONT_FACE				"Microsoft Sans Serif"

#define TIMER_ID				0xdeadbeef

//============================================================
//	dialog_trigger
//============================================================

static void dialog_trigger(HWND dlgwnd, WORD trigger_flags)
{
	LRESULT result;
	HWND dialog_item;
	struct dialog_info *di;
	struct dialog_info_trigger *trigger;
	LONG l;

	l = GetWindowLongPtr(dlgwnd, WNDLONG_DIALOG);
	di = (struct dialog_info *) l;
	assert(di);
	for (trigger = di->trigger_first; trigger; trigger = trigger->next)
	{
		if (trigger->trigger_flags & trigger_flags)
		{
			dialog_item = GetDlgItem(dlgwnd, trigger->dialog_item);
			assert(dialog_item);
			result = 0;

			if (trigger->message)
				result = SendMessage(dialog_item, trigger->message, trigger->wparam, trigger->lparam);
			if (trigger->trigger_proc)
				result = trigger->trigger_proc(dialog_item, trigger->message, trigger->wparam, trigger->lparam);

			if (trigger->result)
				*(trigger->result) = result;
		}
	}
}

//============================================================
//	dialog_proc
//============================================================

static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	INT_PTR handled = TRUE;
	TCHAR buf[32];
	const char *str;
	WORD command;

	switch(msg) {
	case WM_INITDIALOG:
		SetWindowLongPtr(dlgwnd, WNDLONG_DIALOG, (LONG_PTR) lparam);
		dialog_trigger(dlgwnd, TRIGGER_INITDIALOG);
		break;

	case WM_COMMAND:
		command = LOWORD(wparam);

		GetWindowText((HWND) lparam, buf, sizeof(buf) / sizeof(buf[0]));
		str = T2A(buf);
		if (!strcmp(str, DLGTEXT_OK))
			command = IDOK;
		else if (!strcmp(str, DLGTEXT_CANCEL))
			command = IDCANCEL;
		else
			command = 0;

		switch(command) {
		case IDOK:
			dialog_trigger(dlgwnd, TRIGGER_APPLY);
			/* fall through */

		case IDCANCEL:
			EndDialog(dlgwnd, 0);
			break;

		default:
			handled = FALSE;
			break;
		}
		break;

	default:
		handled = FALSE;
		break;
	}
	return handled;
}


//============================================================
//	dialog_write
//============================================================

static int dialog_write(struct dialog_info *di, const void *ptr, size_t sz, int align)
{
	HGLOBAL newhandle;
	size_t base;
	UINT8 *mem;
	UINT8 *mem2;

	if (!di->handle)
	{
		newhandle = GlobalAlloc(GMEM_ZEROINIT, sz);
		base = 0;
	}
	else
	{
		base = di->handle_size;
		base += align - 1;
		base -= base % align;
		newhandle = GlobalReAlloc(di->handle, base + sz, GMEM_ZEROINIT);
		if (!newhandle)
		{
			newhandle = GlobalAlloc(GMEM_ZEROINIT, base + sz);
			if (newhandle)
			{
				mem = GlobalLock(di->handle);
				mem2 = GlobalLock(newhandle);
				memcpy(mem2, mem, base);
				GlobalUnlock(di->handle);
				GlobalUnlock(newhandle);
				GlobalFree(di->handle);
			}
		}
	}
	if (!newhandle)
		return 1;

	mem = GlobalLock(newhandle);
	memcpy(mem + base, ptr, sz);
	GlobalUnlock(newhandle);
	di->handle = newhandle;
	di->handle_size = base + sz;
	return 0;
}


//============================================================
//	dialog_write_string
//============================================================

static int dialog_write_string(struct dialog_info *di, const char *str)
{
	const WCHAR *wstr;
	wstr = A2W(str);	
	return dialog_write(di, wstr, (wcslen(wstr) + 1) * sizeof(WCHAR), 2);
}

//============================================================
//	dialog_write_item
//============================================================

static int dialog_write_item(struct dialog_info *di, DWORD style, short x, short y,
	 short cx, short cy, const char *str, WORD class_atom)
{
	DLGITEMTEMPLATE item_template;
	WORD w[2];

	memset(&item_template, 0, sizeof(item_template));
	item_template.style = style;
	item_template.x = x;
	item_template.y = y;
	item_template.cx = cx;
	item_template.cy = cy;
	item_template.id = di->item_count + 1;

	if (dialog_write(di, &item_template, sizeof(item_template), 4))
		return 1;

	w[0] = 0xffff;
	w[1] = class_atom;
	if (dialog_write(di, w, sizeof(w), 2))
		return 1;

	if (dialog_write_string(di, str))
		return 1;

	w[0] = 0;
	if (dialog_write(di, w, sizeof(w[0]), 2))
		return 1;

	di->item_count++;
	return 0;
}

//============================================================
//	dialog_add_trigger
//============================================================

static int dialog_add_trigger(struct dialog_info *di, WORD dialog_item,
	WORD trigger_flags, UINT message, trigger_function trigger_proc,
	WPARAM wparam, LPARAM lparam, UINT16 *result)
{
	struct dialog_info_trigger *trigger;

	assert(di);
	assert(trigger_flags);

	trigger = (struct dialog_info_trigger *) pool_malloc(&di->memory_pool, sizeof(struct dialog_info_trigger));
	if (!trigger)
		return 1;

	trigger->next = NULL;
	trigger->trigger_flags = trigger_flags;
	trigger->dialog_item = dialog_item;
	trigger->message = message;
	trigger->trigger_proc = trigger_proc;
	trigger->wparam = wparam;
	trigger->lparam = lparam;
	trigger->result = result;

	if (di->trigger_last)
		di->trigger_last->next = trigger;
	else
		di->trigger_first = trigger;
	di->trigger_last = trigger;
	return 0;
}

//============================================================
//	dialog_prime
//============================================================

static void dialog_prime(struct dialog_info *di)
{
	DLGTEMPLATE *dlg_template;

	dlg_template = (DLGTEMPLATE *) GlobalLock(di->handle);
	dlg_template->cdit = di->item_count;
	dlg_template->cx = di->cx;
	dlg_template->cy = di->cy;
	GlobalUnlock(di->handle);
}

//============================================================
//	dialog_get_combo_value
//============================================================

static LRESULT dialog_get_combo_value(HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam)
{
	int idx;
	idx = SendMessage(dialog_item, CB_GETCURSEL, 0, 0);
	if (idx == CB_ERR)
		return 0;
	return SendMessage(dialog_item, CB_GETITEMDATA, idx, 0);
}

//============================================================
//	win_dialog_init
//============================================================

void *win_dialog_init(const char *title)
{
	struct dialog_info *di;
	DLGTEMPLATE dlg_template;
	WORD w[2];

	// create the dialog structure
	di = malloc(sizeof(struct dialog_info));
	if (!di)
		goto error;
	memset(di, 0, sizeof(*di));

	pool_init(&di->memory_pool);

	di->cx = 0;
	di->cy = 0;

	memset(&dlg_template, 0, sizeof(dlg_template));
	dlg_template.style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT;
	dlg_template.x = 10;
	dlg_template.y = 10;
	if (dialog_write(di, &dlg_template, sizeof(dlg_template), 4))
		goto error;

	w[0] = 0;
	w[1] = 0;
	if (dialog_write(di, w, sizeof(w), 2))
		goto error;

	if (dialog_write_string(di, title))
		goto error;

	w[0] = FONT_SIZE;
	if (dialog_write(di, w, sizeof(w[0]), 2))
		goto error;
	if (dialog_write_string(di, FONT_FACE))
		goto error;

	return (void *) di;

error:
	if (di)
		win_dialog_exit(di);
	return NULL;
}


//============================================================
//	win_dialog_add_combobox
//============================================================

int win_dialog_add_combobox(void *dialog, const char *item_label, UINT16 *value)
{
	struct dialog_info *di = (struct dialog_info *) dialog;
	short x;
	short y;

	assert(item_label);

	x = DIM_HORIZONTAL_SPACING;
	y = di->cy + DIM_VERTICAL_SPACING;

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y, DIM_LABEL_WIDTH, DIM_COMBO_ROW_HEIGHT, item_label, DLGITEM_STATIC))
		return 1;

	x += DIM_LABEL_WIDTH + DIM_HORIZONTAL_SPACING;
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
			x, y, DIM_COMBO_WIDTH, DIM_COMBO_ROW_HEIGHT * 8, "", DLGITEM_COMBOBOX))
		return 1;
	di->combo_string_count = 0;
	di->combo_default_value = *value;

	if (dialog_add_trigger(di, di->item_count, TRIGGER_APPLY, 0, dialog_get_combo_value, 0, 0, value))
		return 1;

	x += DIM_COMBO_WIDTH + DIM_HORIZONTAL_SPACING;

	if (x > di->cx)
		di->cx = x;
	di->cy += DIM_COMBO_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;
	return 0;
}

//============================================================
//	win_dialog_add_combobox_item
//============================================================

int win_dialog_add_combobox_item(void *dialog, const char *item_label, int item_data)
{
	struct dialog_info *di = (struct dialog_info *) dialog;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, CB_ADDSTRING, NULL, 0, (LPARAM) item_label, NULL))
		return 1;
	di->combo_string_count++;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, CB_SETITEMDATA, NULL, di->combo_string_count-1, (LPARAM) item_data, NULL))
		return 1;
	if (item_data == di->combo_default_value)
	{
		if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, CB_SETCURSEL, NULL, di->combo_string_count-1, 0, NULL))
			return 1;
	}
	return 0;
}

//============================================================
//	seqselect_wndproc
//============================================================

struct seqselect_stuff
{
	WNDPROC oldwndproc;
	InputSeq *code;
	InputSeq newcode;
	UINT_PTR timer;
	WORD pos;
};

static INT_PTR CALLBACK seqselect_wndproc(HWND editwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct seqselect_stuff *stuff;
	INT_PTR result;
	int dlgitem;
	HWND dlgwnd;
	HWND dlgitemwnd;
	InputCode code;

	stuff = (struct seqselect_stuff *) GetWindowLongPtr(editwnd, GWLP_USERDATA);

	switch(msg) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_CHAR:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		result = 1;
		break;

	case WM_TIMER:
		if (wparam == TIMER_ID)
		{
			win_poll_input();
			code = code_read_async();
			if (code != CODE_NONE)
			{
				seq_set_1(&stuff->newcode, code);
				SetWindowText(editwnd, A2T(code_name(code)));

				dlgwnd = GetParent(editwnd);

				dlgitem = stuff->pos;
				do
				{
					dlgitem++;
					dlgitemwnd = GetDlgItem(dlgwnd, dlgitem);
				}
				while(dlgitemwnd && (GetWindowLongPtr(dlgitemwnd, GWLP_WNDPROC) != (LONG_PTR) seqselect_wndproc));
				if (dlgitemwnd)
				{
					SetFocus(dlgitemwnd);
					SendMessage(dlgitemwnd, EM_SETSEL, 0, -1);
				}
				else
				{
					SetFocus(dlgwnd);
				}
			}
			result = 0;
		}
		else
		{
			result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		}
		break;

	case WM_SETFOCUS:
		if (stuff->timer)
			KillTimer(editwnd, stuff->timer);
		stuff->timer = SetTimer(editwnd, TIMER_ID, 100, (TIMERPROC) NULL);
		result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		break;

	case WM_KILLFOCUS:
		if (stuff->timer)
		{
			KillTimer(editwnd, stuff->timer);
			stuff->timer = 0;
		}
		result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		SetFocus(editwnd);
		SendMessage(editwnd, EM_SETSEL, 0, -1);
		result = 0;
		break;

	default:
		result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		break;
	}
	return result;
}

//============================================================
//	seqselect_setup
//============================================================

static LRESULT seqselect_setup(HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	char buf[256];
	struct seqselect_stuff *stuff = (struct seqselect_stuff *) lparam;

	memcpy(stuff->newcode, *(stuff->code), sizeof(stuff->newcode));
	seq_name(stuff->code, buf, sizeof(buf) / sizeof(buf[0]));
	SetWindowText(editwnd, A2T(buf));
	stuff->oldwndproc = (WNDPROC) SetWindowLongPtr(editwnd, GWLP_WNDPROC, (LONG) seqselect_wndproc);
	SetWindowLongPtr(editwnd, GWLP_USERDATA, lparam);
	return 0;
}

//============================================================
//	seqselect_apply
//============================================================

static LRESULT seqselect_apply(HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct seqselect_stuff *stuff;
	stuff = (struct seqselect_stuff *) GetWindowLongPtr(editwnd, GWLP_USERDATA);
	memcpy(*(stuff->code), stuff->newcode, sizeof(*(stuff->code)));
	return 0;
}

//============================================================
//	dialog_add_single_seqselect
//============================================================

static int dialog_add_single_seqselect(struct dialog_info *di, short x, short y,
	short cx, short cy, struct InputPort *port)
{
	struct seqselect_stuff *stuff;
	InputSeq *code;

	code = input_port_seq(port);

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | ES_CENTER,
			x, y, cx, cy, "", DLGITEM_EDIT))
		return 1;
	stuff = (struct seqselect_stuff *) pool_malloc(&di->memory_pool, sizeof(struct seqselect_stuff));
	if (!stuff)
		return 1;
	stuff->code = code;
	stuff->pos = di->item_count;
	stuff->timer = 0;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, 0, seqselect_setup, di->item_count, (LPARAM) stuff, NULL))
		return 1;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_APPLY, 0, seqselect_apply, 0, 0, NULL))
		return 1;
	return 0;
}

//============================================================
//	win_dialog_add_seqselect
//============================================================

int win_dialog_add_portselect(void *dialog, struct InputPort *port, int *span)
{
	struct dialog_info *di = (struct dialog_info *) dialog;
	short x;
	short y;
	struct InputPort *arranged_ports[4];
	char buf[256];
	int i;
	int rows;
	const char *port_name;
	const char *last_port_name = NULL;

	*span = inputx_orient_ports(port, arranged_ports);

	// Come up with 
	if (*span == 1)
	{
		port_name = input_port_name(port);
	}
	else
	{
		for (i = 0; i < sizeof(arranged_ports) / sizeof(arranged_ports[0]); i++)
		{
			if (arranged_ports[i])
			{
				port_name = input_port_name(arranged_ports[i]);
				if (i == 0)
				{
					strncpyz(buf, port_name, sizeof(buf) / sizeof(buf[0]));
					last_port_name = port_name;
				}
				else if (strcmp(port_name, last_port_name))
				{
					strncatz(buf, " / ", sizeof(buf) / sizeof(buf[0]));
					strncatz(buf, port_name, sizeof(buf) / sizeof(buf[0]));
					last_port_name = port_name;
				}
			}
		}
		port_name = buf;
	}

	rows = arranged_ports[2] ? (arranged_ports[0] ? 3 : 2) : 1;

	x = DIM_HORIZONTAL_SPACING;
	y = di->cy + DIM_VERTICAL_SPACING;

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y + (rows - 1) * (DIM_NORMAL_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2)/2,
			DIM_LABEL_WIDTH, DIM_NORMAL_ROW_HEIGHT, port_name, DLGITEM_STATIC))
		return 1;
	x += DIM_LABEL_WIDTH + DIM_HORIZONTAL_SPACING;

	switch(*span) {
	case 1:
		if (dialog_add_single_seqselect(di, x, y, DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, port))
			return 1;
		y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
		x += DIM_SEQ_WIDTH + DIM_HORIZONTAL_SPACING;
		break;

	case 2:
		switch(rows) {
		case 1:
			/* left */
			if (dialog_add_single_seqselect(di, x, y,
					DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[0]))
				return 1;

			/* right */
			if (dialog_add_single_seqselect(di, x, y,
					DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[1]))
				return 1;
			y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
			x += DIM_SEQ_WIDTH*2 + DIM_HORIZONTAL_SPACING*2;
			break;

		case 2:
			/* up */
			if (dialog_add_single_seqselect(di, x, y, DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[2]))
				return 1;
			y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;

			/* down */
			if (dialog_add_single_seqselect(di, x, y, DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[3]))
				return 1;
			y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
			x += DIM_SEQ_WIDTH + DIM_HORIZONTAL_SPACING;
			break;

		default:
			assert(0);
			return 1;
		}
		break;

	case 4:
		/* up */
		if (dialog_add_single_seqselect(di, x + (DIM_SEQ_WIDTH + DIM_HORIZONTAL_SPACING) / 2, y,
				DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[2]))
			return 1;
		y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;

		/* left */
		if (dialog_add_single_seqselect(di, x, y,
				DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[0]))
			return 1;

		/* right */
		if (dialog_add_single_seqselect(di, x + DIM_SEQ_WIDTH + DIM_HORIZONTAL_SPACING, y,
				DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[1]))
			return 1;
		y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;

		/* down */
		if (dialog_add_single_seqselect(di, x + (DIM_SEQ_WIDTH + DIM_HORIZONTAL_SPACING) / 2, y,
				DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, arranged_ports[3]))
			return 1;
		y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
		x += DIM_SEQ_WIDTH*2 + DIM_HORIZONTAL_SPACING*2;
		break;

	default:
		assert(0);
		return 1;
	}
	port++;

	/* update dialog size */
	if (x > di->cx)
		di->cx = x;
	di->cy = y;
	return 0;
}

//============================================================
//	win_dialog_add_standard_buttons
//============================================================

int win_dialog_add_standard_buttons(void *dialog)
{
	struct dialog_info *di = (struct dialog_info *) dialog;
	short x;
	short y;

	x = di->cx - DIM_HORIZONTAL_SPACING - DIM_BUTTON_WIDTH;
	y = di->cy + DIM_VERTICAL_SPACING;

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y, DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_CANCEL, DLGITEM_BUTTON))
		return 1;

	x -= DIM_HORIZONTAL_SPACING + DIM_BUTTON_WIDTH;
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y, DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_OK, DLGITEM_BUTTON))
		return 1;
	di->cy += DIM_BUTTON_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;
	return 0;
}

//============================================================
//	win_dialog_exit
//============================================================

void win_dialog_exit(void *dialog)
{
	struct dialog_info *di = (struct dialog_info *) dialog;

	assert(di);
	if (di->handle)
		GlobalFree(di->handle);

	pool_exit(&di->memory_pool);
	free(di);
}

//============================================================
//	win_dialog_runmodal
//============================================================

void win_dialog_runmodal(void *dialog)
{
	struct dialog_info *di;
	extern void win_timer_enable(int enabled);
	
	di = (struct dialog_info *) dialog;
	assert(di);

	// finishing touches on the dialog
	dialog_prime(di);

	// disable sound while in the dialog
	osd_sound_enable(0);

	// disable the timer while in the dialog
	win_timer_enable(0);

#ifdef UNDER_CE
	// on WinCE, suspend GAPI
	gx_suspend();
#endif

	DialogBoxIndirectParam(NULL, di->handle, win_video_window, dialog_proc, (LPARAM) di);

#ifdef UNDER_CE
	// on WinCE, resume GAPI
	gx_resume();
#endif

	// reenable timer
	win_timer_enable(1);

	// reenable sound
	osd_sound_enable(1);
}

