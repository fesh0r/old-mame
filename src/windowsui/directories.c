/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Directories.c

***************************************************************************/

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <sys/stat.h>
#include "MAME32.h"
#include "Directories.h"
#include "resource.h"
#include "options.h"

#define MAX_DIRS 20

enum
{
	ROM = 0,
#if HASDIR_SAMPLE
	SAMPLE,
#endif
#if HASDIR_SOFTWARE
	SOFTWARE,
#endif
#ifdef MESS
	CRC,
#endif
	CFG,
	HI,
	IMG,
	INP,
	STATE,
	ART,
	MEMCARD,
	FLYER,
	CABINET,
	MARQUEE,
	NVRAM,
	CTRLR,
	DIFF,
	ICONS,
	LASTDIR
};


#define NUMPATHS 2
#define NUMDIRS  (LASTDIR - NUMPATHS)

char *dir_names[LASTDIR] =
{
#ifdef MESS
	"BIOSes",  /* path */
#else
	"ROMs",    /* path */
#endif
#if HASDIR_SAMPLE
	"Samples", /* path */
#endif
#if HASDIR_SOFTWARE
    "Software",/* path */
#endif
#ifdef MESS
	"CRC",
#endif
	"Config",
	"High Scores",
	"Snapshots",
	"Input files (*.inp)",
	"State",
	"Artwork",
	"Memory Card",
	"Flyers",
	"Cabinets",
	"Marquees",
	"NVRAM",
	"Controller Files",
	"Hard Drive Difference",
	"Icons"
};

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	char    m_Directories[MAX_DIRS][MAX_PATH];
	int     m_NumDirectories;
	BOOL    m_bModified;
} tPath;

typedef char tDirectory[MAX_PATH];

struct tDirInfo
{
	tPath     m_Paths[NUMPATHS];
	char      m_Directories[NUMDIRS][MAX_PATH];
};

/***************************************************************************
    Function prototypes
 ***************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
#ifdef MESS
BOOL BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult);
#else
static BOOL         BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult);
#endif

static void     DirInfo_SetDir(struct tDirInfo* pInfo, int nType, int nItem, const char* pText);
static char*    DirInfo_Dir(struct tDirInfo* pInfo, int nType);
static char*    DirInfo_Path(struct tDirInfo* pInfo, int nType, int nItem);
static char*    FixSlash(char *s);

static void     UpdateDirectoryList(HWND hDlg);

static void     Directories_OnSelChange(HWND hDlg);
static BOOL     Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void     Directories_OnDestroy(HWND hDlg);
static void     Directories_OnClose(HWND hDlg);
static void     Directories_OnOk(HWND hDlg);
static void     Directories_OnCancel(HWND hDlg);
static void     Directories_OnInsert(HWND hDlg);
static void     Directories_OnBrowse(HWND hDlg);
static void     Directories_OnDelete(HWND hDlg);
static BOOL     Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static BOOL     Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR);
static void     Directories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
static BOOL     Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

struct tDirInfo* pDirInfo;

/***************************************************************************
    External function definitions
 ***************************************************************************/

INT_PTR CALLBACK DirectoriesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL bReturn = FALSE;

	switch (Msg)
	{
	case WM_INITDIALOG:
		return (BOOL)HANDLE_WM_INITDIALOG(hDlg, wParam, lParam, Directories_OnInitDialog);

	case WM_COMMAND:
		HANDLE_WM_COMMAND(hDlg, wParam, lParam, Directories_OnCommand);
		bReturn = TRUE;
		break;

	case WM_NOTIFY:
		return (BOOL)HANDLE_WM_NOTIFY(hDlg, wParam, lParam, Directories_OnNotify);
		break;

	case WM_CLOSE:
		HANDLE_WM_CLOSE(hDlg, wParam, lParam, Directories_OnClose);
		break;

	case WM_DESTROY:
		HANDLE_WM_DESTROY(hDlg, wParam, lParam, Directories_OnDestroy);
		break;

	default:
		bReturn = FALSE;
	}
	return bReturn;
}

/***************************************************************************
	Internal function definitions
 ***************************************************************************/

static BOOL IsMultiDir(int nType)
{
	switch (nType)
	{
	case ROM:
#if HASDIR_SOFTWARE
    case SOFTWARE:
#endif
#if HASDIR_SAMPLE
	case SAMPLE:
#endif
		return TRUE;
	}
	return FALSE;
}

static void DirInfo_SetDir(struct tDirInfo* pInfo, int nType, int nItem, const char* pText)
{
	if (IsMultiDir(nType))
	{
		strcpy(DirInfo_Path(pInfo, nType, nItem), pText);
		pInfo->m_Paths[nType].m_bModified = TRUE;
	}
	else
	{
		if (nType < LASTDIR)
			strcpy(DirInfo_Dir(pInfo, nType), pText);
	}
}

static char* DirInfo_Dir(struct tDirInfo* pInfo, int nType)
{
	return pInfo->m_Directories[nType - NUMPATHS];
}

static char* DirInfo_Path(struct tDirInfo* pInfo, int nType, int nItem)
{
	return pInfo->m_Paths[nType].m_Directories[nItem];
}

#define DirInfo_NumDir(info, t) ((info)->m_Paths[(t)].m_NumDirectories)

/* lop off trailing backslash if it exists */
static char * FixSlash(char *s)
{
	int len = 0;
	
	if (s)
		len = strlen(s);

	if (len && s[len - 1] == '\\')
		s[len - 1] = '\0';

	return s;
}

static void UpdateDirectoryList(HWND hDlg)
{
	int 	i;
	int 	nType;
	LV_ITEM Item;
	HWND	hList  = GetDlgItem(hDlg, IDC_DIR_LIST);
	HWND	hCombo = GetDlgItem(hDlg, IDC_DIR_COMBO);

	/* Remove previous */

	ListView_DeleteAllItems(hList);

	/* Update list */

	memset(&Item, 0, sizeof(LV_ITEM));
	Item.mask = LVIF_TEXT;

	nType = ComboBox_GetCurSel(hCombo);
	if (IsMultiDir(nType))
	{
		Item.pszText = "<               >";
		ListView_InsertItem(hList, &Item);
		for (i = DirInfo_NumDir(pDirInfo, nType) - 1; 0 <= i; i--)
		{
			Item.pszText = DirInfo_Path(pDirInfo, nType, i);
			ListView_InsertItem(hList, &Item);
		}
		}
	else
	{
		if (nType < LASTDIR)
		{
			Item.pszText = DirInfo_Dir(pDirInfo, nType);
			ListView_InsertItem(hList, &Item);
		}
	}

	/* select first one */

	ListView_SetItemState(hList, 0, LVIS_SELECTED, LVIS_SELECTED);
}

static void Directories_OnSelChange(HWND hDlg)
{
	UpdateDirectoryList(hDlg);
	
	if (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO)) == 0
	||	ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO)) == 1)
	{
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_DELETE), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_INSERT), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_DELETE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_DIR_INSERT), FALSE);
	}
}

static void PopulateDirInfo(int nDir, const char *(*GetTheseDirs)(void))
{
	char *token;
	char buf[MAX_PATH * MAX_DIRS];

	strcpy(buf, GetTheseDirs());

	pDirInfo->m_Paths[nDir].m_NumDirectories = 0;
	token = strtok(buf, ";");
	while ((DirInfo_NumDir(pDirInfo, nDir) < MAX_DIRS) && token)
	{
		strcpy(DirInfo_Path(pDirInfo, nDir, DirInfo_NumDir(pDirInfo, nDir)), token);
		DirInfo_NumDir(pDirInfo, nDir)++;
		token = strtok(NULL, ";");
	}
	pDirInfo->m_Paths[nDir].m_bModified = FALSE;
}

static BOOL Directories_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	RECT		rectClient;
	LVCOLUMN	LVCol;
	int 		i;

	pDirInfo = (struct tDirInfo*) malloc(sizeof(struct tDirInfo));
	if (pDirInfo == NULL) /* bummer */
	{
		EndDialog(hDlg, -1);
		return FALSE;
	}

	for (i = LASTDIR - 1; i >= 0; i--)
	{
		ComboBox_InsertString(GetDlgItem(hDlg, IDC_DIR_COMBO), 0, dir_names[i]);
	}

	ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO), 0);

	GetClientRect(GetDlgItem(hDlg, IDC_DIR_LIST), &rectClient);

	memset(&LVCol, 0, sizeof(LVCOLUMN));
	LVCol.mask	  = LVCF_WIDTH;
	LVCol.cx	  = rectClient.right - rectClient.left - GetSystemMetrics(SM_CXHSCROLL);
	
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_DIR_LIST), 0, &LVCol);


	/* Keep a temporary copy of the directory strings in pDirInfo. */
	memset(pDirInfo, 0, sizeof(struct tDirInfo));

	PopulateDirInfo(ROM, GetRomDirs);
#if HASDIR_SOFTWARE
	PopulateDirInfo(SOFTWARE, GetSoftwareDirs);
#endif
#if HASDIR_SAMPLE
	PopulateDirInfo(SAMPLE, GetSampleDirs);
#endif
#ifdef MESS
	strcpy(DirInfo_Dir(pDirInfo, CRC),		GetCrcDir());
#endif

	strcpy(DirInfo_Dir(pDirInfo, CFG),		GetCfgDir());
	strcpy(DirInfo_Dir(pDirInfo, HI),		GetHiDir());
	strcpy(DirInfo_Dir(pDirInfo, IMG),		GetImgDir());
	strcpy(DirInfo_Dir(pDirInfo, INP),		GetInpDir());
	strcpy(DirInfo_Dir(pDirInfo, STATE),	GetStateDir());
	strcpy(DirInfo_Dir(pDirInfo, ART),		GetArtDir());
	strcpy(DirInfo_Dir(pDirInfo, MEMCARD),	GetMemcardDir());
	strcpy(DirInfo_Dir(pDirInfo, FLYER),	GetFlyerDir());
	strcpy(DirInfo_Dir(pDirInfo, CABINET),	GetCabinetDir());
	strcpy(DirInfo_Dir(pDirInfo, MARQUEE),	GetMarqueeDir());
	strcpy(DirInfo_Dir(pDirInfo, NVRAM),	GetNvramDir());
	strcpy(DirInfo_Dir(pDirInfo, CTRLR),	GetCtrlrDir());
	strcpy(DirInfo_Dir(pDirInfo, DIFF),	GetDiffDir());
	strcpy(DirInfo_Dir(pDirInfo, ICONS),	GetIconsDir());

	UpdateDirectoryList(hDlg);

	return TRUE;
}

static void Directories_OnDestroy(HWND hDlg)
{
	if (pDirInfo != NULL)
	{
		free(pDirInfo);
		pDirInfo = NULL;
	}
}

static void Directories_OnClose(HWND hDlg)
{
	EndDialog(hDlg, IDCANCEL);
}

static int RetrieveDirList(int nDir, int nFlagResult, void (*SetTheseDirs)(const char *s))
{
	int i;
	int nResult = 0;
	int nPaths;
	char buf[MAX_PATH * MAX_DIRS];

	if (pDirInfo->m_Paths[nDir].m_bModified == TRUE)
	{
		memset(buf, 0, MAX_PATH * MAX_DIRS);
		nPaths = DirInfo_NumDir(pDirInfo, nDir);
		for (i = 0; i < nPaths; i++)
		{

			strcat(buf, FixSlash(DirInfo_Path(pDirInfo, nDir, i)));

			if (i < nPaths - 1)
				strcat(buf, ";");
		}
		SetTheseDirs(buf);

		nResult |= nFlagResult;
    }
	return nResult;
}

static void Directories_OnOk(HWND hDlg)
{
	int 	nResult = 0;

	/* set path options */
	nResult |= RetrieveDirList(ROM, DIRDLG_ROMS, SetRomDirs);
#if HASDIR_SOFTWARE
	nResult |= RetrieveDirList(SOFTWARE, DIRDLG_SOFTWARE, SetSoftwareDirs);
#endif
#if HASDIR_SAMPLE
	nResult |= RetrieveDirList(SAMPLE, DIRDLG_SAMPLES, SetSampleDirs);
#endif
#ifdef MESS
	SetCrcDir(	  FixSlash(DirInfo_Dir(pDirInfo, CRC)));
#endif
	SetCfgDir(	  FixSlash(DirInfo_Dir(pDirInfo, CFG)));
	SetHiDir(	  FixSlash(DirInfo_Dir(pDirInfo, HI)));
	SetImgDir(	  FixSlash(DirInfo_Dir(pDirInfo, IMG)));
	SetInpDir(	  FixSlash(DirInfo_Dir(pDirInfo, INP)));
	SetStateDir(  FixSlash(DirInfo_Dir(pDirInfo, STATE)));
	SetArtDir(	  FixSlash(DirInfo_Dir(pDirInfo, ART)));
	SetMemcardDir(FixSlash(DirInfo_Dir(pDirInfo, MEMCARD)));
	SetFlyerDir(  FixSlash(DirInfo_Dir(pDirInfo, FLYER)));
	SetCabinetDir(FixSlash(DirInfo_Dir(pDirInfo, CABINET)));
	SetMarqueeDir(FixSlash(DirInfo_Dir(pDirInfo, MARQUEE)));
	SetNvramDir(  FixSlash(DirInfo_Dir(pDirInfo, NVRAM)));
	SetCtrlrDir(  FixSlash(DirInfo_Dir(pDirInfo, CTRLR)));
	SetDiffDir(   FixSlash(DirInfo_Dir(pDirInfo, DIFF)));
	SetIconsDir(  FixSlash(DirInfo_Dir(pDirInfo, ICONS)));

	EndDialog(hDlg, nResult);
}

static void Directories_OnCancel(HWND hDlg)
{
	EndDialog(hDlg, IDCANCEL);
}

static void Directories_OnInsert(HWND hDlg)
{
	int 	nItem;
	char	buf[MAX_PATH];
	HWND	hList;

	hList = GetDlgItem(hDlg, IDC_DIR_LIST);
	nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

	if (BrowseForDirectory(hDlg, NULL, buf) == TRUE)
	{
		int 	i;
		int 	nType;

		/* list was empty */
		if (nItem == -1)
			nItem = 0;

		nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
		if (IsMultiDir(nType))
		{
			if (MAX_DIRS <= DirInfo_NumDir(pDirInfo, nType))
				return;

			for (i = DirInfo_NumDir(pDirInfo, nType); nItem < i; i--)
				strcpy(DirInfo_Path(pDirInfo, nType, i),
					   DirInfo_Path(pDirInfo, nType, i - 1));

			strcpy(DirInfo_Path(pDirInfo, nType, nItem), buf);
			DirInfo_NumDir(pDirInfo, nType)++;

			pDirInfo->m_Paths[nType].m_bModified = TRUE;
		}

		UpdateDirectoryList(hDlg);

		ListView_SetItemState(hList, nItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

static void Directories_OnBrowse(HWND hDlg)
{
	int 	nType;
	int 	nItem;
	char	inbuf[MAX_PATH];
	char	outbuf[MAX_PATH];
	HWND	hList;

	hList = GetDlgItem(hDlg, IDC_DIR_LIST);
	nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

	if (nItem == -1)
		return;

	nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		/* Last item is placeholder for append */
		if (nItem == ListView_GetItemCount(hList) - 1)
		{
			Directories_OnInsert(hDlg); 
			return;
		}
	}

	ListView_GetItemText(hList, nItem, 0, inbuf, MAX_PATH);

	if (BrowseForDirectory(hDlg, inbuf, outbuf) == TRUE)
	{
		nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
		DirInfo_SetDir(pDirInfo, nType, nItem, outbuf);
		UpdateDirectoryList(hDlg);
	}
}

static void Directories_OnDelete(HWND hDlg)
{
	int 	nType;
	int 	nCount;
	int 	nSelect;
	int 	i;
	int 	nItem;
	HWND	hList = GetDlgItem(hDlg, IDC_DIR_LIST);

	nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED | LVNI_ALL);

	if (nItem == -1)
		return;

	/* Don't delete "Append" placeholder. */
	if (nItem == ListView_GetItemCount(hList) - 1)
		return;

	nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		for (i = nItem; i < DirInfo_NumDir(pDirInfo, nType) - 1; i++)
			strcpy(DirInfo_Path(pDirInfo, nType, i),
				   DirInfo_Path(pDirInfo, nType, i + 1));

		strcpy(DirInfo_Path(pDirInfo, nType, DirInfo_NumDir(pDirInfo, nType) - 1), "");
		DirInfo_NumDir(pDirInfo, nType)--;

		pDirInfo->m_Paths[nType].m_bModified = TRUE;
	}

	UpdateDirectoryList(hDlg);
	

	nCount = ListView_GetItemCount(hList);
	if (nCount <= 1)
		return;

	/* If the last item was removed, select the item above. */
	if (nItem == nCount - 1)
		nSelect = nCount - 2;
	else
		nSelect = nItem;

	ListView_SetItemState(hList, nSelect, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

static BOOL Directories_OnBeginLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	int 		  nType;
	BOOL		  bResult = FALSE;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEM* 	  pItem = &pDispInfo->item;

	nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
	if (IsMultiDir(nType))
	{
		/* Last item is placeholder for append */
		if (pItem->iItem == ListView_GetItemCount(GetDlgItem(hDlg, IDC_DIR_LIST)) - 1)
		{
			HWND hEdit;

			if (MAX_DIRS <= DirInfo_NumDir(pDirInfo, nType))
				return TRUE; /* don't edit */

			hEdit = (HWND)(LRESULT)SendMessage(GetDlgItem(hDlg, IDC_DIR_LIST), LVM_GETEDITCONTROL, 0, 0);
			Edit_SetText(hEdit, "");
		}
	}

	return bResult;
}

static BOOL Directories_OnEndLabelEdit(HWND hDlg, NMHDR* pNMHDR)
{
	BOOL		  bResult = FALSE;
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	LVITEM* 	  pItem = &pDispInfo->item;

	if (pItem->pszText != NULL)
	{
		struct stat file_stat;

		/* Don't allow empty entries. */
		if (!strcmp(pItem->pszText, ""))
		{
			return FALSE;
		}

		/* Check validity of edited directory. */
		if (stat(pItem->pszText, &file_stat) == 0
		&&	(file_stat.st_mode & S_IFDIR))
		{
			bResult = TRUE;
		}
		else
		{
			if (MessageBox(NULL, "Directory does not exist, continue anyway?", MAME32NAME, MB_OKCANCEL) == IDOK)
				bResult = TRUE;
		}
	}

	if (bResult == TRUE)
	{
		int nType;
		int i;

		nType = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIR_COMBO));
		if (IsMultiDir(nType))
		{
			/* Last item is placeholder for append */
			if (pItem->iItem == ListView_GetItemCount(GetDlgItem(hDlg, IDC_DIR_LIST)) - 1)
			{
				if (MAX_DIRS <= DirInfo_NumDir(pDirInfo, nType))
					return FALSE;

				for (i = DirInfo_NumDir(pDirInfo, nType); pItem->iItem < i; i--)
					strcpy(DirInfo_Path(pDirInfo, nType, i),
						   DirInfo_Path(pDirInfo, nType, i - 1));

				strcpy(DirInfo_Path(pDirInfo, nType, pItem->iItem), pItem->pszText);
				pDirInfo->m_Paths[nType].m_bModified = TRUE;

				DirInfo_NumDir(pDirInfo, nType)++;
			}
			else
				DirInfo_SetDir(pDirInfo, nType, pItem->iItem, pItem->pszText);
		}
		else
		{
			switch (nType)
			{
#ifdef MESS
			case CRC:
#endif
			case CFG:
			case HI:
			case IMG:
			case INP:
			case STATE:
			case ART:
			case MEMCARD:
			case FLYER:
			case CABINET:
			case MARQUEE:
			case NVRAM:
		case CTRLR:
		case DIFF:
		case ICONS:
				DirInfo_SetDir(pDirInfo, nType, pItem->iItem, pItem->pszText);
				break;
			}
		}

		UpdateDirectoryList(hDlg);
		ListView_SetItemState(GetDlgItem(hDlg, IDC_DIR_LIST), pItem->iItem,
							  LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

	}

	return bResult;
}

static void Directories_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		if (codeNotify == BN_CLICKED)
			Directories_OnOk(hDlg);
		break;

	case IDCANCEL:
		if (codeNotify == BN_CLICKED)
			Directories_OnCancel(hDlg);
		break;

	case IDC_DIR_BROWSE:
		if (codeNotify == BN_CLICKED)
			Directories_OnBrowse(hDlg);
		break;

	case IDC_DIR_INSERT:
		if (codeNotify == BN_CLICKED)
			Directories_OnInsert(hDlg);
		break;

	case IDC_DIR_DELETE:
		if (codeNotify == BN_CLICKED)
			Directories_OnDelete(hDlg);
		break;

	case IDC_DIR_COMBO:
		switch (codeNotify)
		{
		case CBN_SELCHANGE:
			Directories_OnSelChange(hDlg);
			break;
		}
		break;
	}
}

static BOOL Directories_OnNotify(HWND hDlg, int id, NMHDR* pNMHDR)
{
	switch (id)
	{
	case IDC_DIR_LIST:
		switch (pNMHDR->code)
		{
		case LVN_ENDLABELEDIT:
			return Directories_OnEndLabelEdit(hDlg, pNMHDR);

		case LVN_BEGINLABELEDIT:
			return Directories_OnBeginLabelEdit(hDlg, pNMHDR);
		}
	}
	return FALSE;
}

/**************************************************************************

	Use the shell to select a Directory.

 **************************************************************************/

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	/*
		Called just after the dialog is initialized
		Select the dir passed in BROWSEINFO.lParam
	*/
	if (uMsg == BFFM_INITIALIZED)
	{
		if ((const char*)lpData != NULL)
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

BOOL BrowseForDirectory(HWND hwnd, const char* pStartDir, char* pResult) 
{
	BOOL		bResult = FALSE;
	IMalloc*	piMalloc = 0;
	BROWSEINFO	Info;
	ITEMIDLIST* pItemIDList = NULL;
	char		buf[MAX_PATH];
	
	if (!SUCCEEDED(SHGetMalloc(&piMalloc)))
		return FALSE;

	Info.hwndOwner		= hwnd;
	Info.pidlRoot		= NULL;
	Info.pszDisplayName = buf;
	Info.lpszTitle		= (LPCSTR)"Directory name:";
	Info.ulFlags		= BIF_RETURNONLYFSDIRS;
	Info.lpfn			= BrowseCallbackProc;
	Info.lParam 		= (LPARAM)pStartDir;

	pItemIDList = SHBrowseForFolder(&Info);

	if (pItemIDList != NULL)
	{
		if (SHGetPathFromIDList(pItemIDList, buf) == TRUE)
		{
			strncpy(pResult, buf, MAX_PATH);
			bResult = TRUE;
		}
		IMalloc_Free(piMalloc, pItemIDList);
	}
	else
	{
		bResult = FALSE;
	}

	IMalloc_Release(piMalloc);
	return bResult;
}

/**************************************************************************/


