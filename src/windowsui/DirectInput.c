/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************
	
  directinput.c

  Direct Input routines.
 
 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dinput.h>
#include "DirectInput.h"
#include "M32Util.h"

/***************************************************************************
	function prototypes
 ***************************************************************************/

/***************************************************************************
	External variables
 ***************************************************************************/

LPDIRECTINPUT di = NULL;

/***************************************************************************
	Internal structures
 ***************************************************************************/

/***************************************************************************
	Internal variables
 ***************************************************************************/

static HANDLE hDLL = NULL;

/***************************************************************************
	External functions	
 ***************************************************************************/

/****************************************************************************
 *		DirectInputInitialize
 *
 *		Initialize the DirectInput variables.
 *
 *		This entails the following functions:
 *
 *			DirectInputCreate
 *
 ****************************************************************************/

typedef HRESULT (WINAPI *dica_proc)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI,
									LPUNKNOWN punkOuter);

BOOL DirectInputInitialize()
{
	HRESULT   hr;
	UINT	  error_mode;
	dica_proc dica;

	if (hDLL != NULL)
		return TRUE;

	hDLL = NULL;

	/* Turn off error dialog for this call */
	error_mode = SetErrorMode(0);
	hDLL = LoadLibrary("dinput.dll");
	SetErrorMode(error_mode);

	if (hDLL == NULL)
		return FALSE;

	dica = (dica_proc)GetProcAddress(hDLL, "DirectInputCreateA");
	if (dica == NULL)
		return FALSE;

	hr = dica(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &di, NULL);

	if (FAILED(hr)) 
	{
		hr = dica(GetModuleHandle(NULL), 0x0300, &di, NULL);

		if (FAILED(hr))
		{
			ErrorMsg("DirectInputCreate failed! error=%x\n", hr);
			di = NULL;
			return FALSE;
		}
	}
	return TRUE;
}

/****************************************************************************
 *
 *		DirectInputClose
 *
 *		Terminate our usage of DirectInput.
 *
 ****************************************************************************/

void DirectInputClose()
{
	/*
		Release any lingering IDirectInput object.
	*/
	if (di) 
	{
		IDirectInput_Release(di);
		di = NULL;
	}
}

BOOL CALLBACK inputEnumDeviceProc(LPCDIDEVICEINSTANCE pdidi, LPVOID pv)
{
	GUID *pguidDevice;

	/* report back the instance guid of the device we enumerated */
	if (pv)
	{
		pguidDevice  = (GUID *)pv;
		*pguidDevice = pdidi->guidInstance;
	}

	/* BUGBUG for now, stop after the first device has been found */
	return DIENUM_STOP;
}

HRESULT SetDIDwordProperty(LPDIRECTINPUTDEVICE2 pdev, REFGUID guidProperty,
						   DWORD dwObject, DWORD dwHow, DWORD dwValue)
{
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize		= sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwObj		= dwObject;
	dipdw.diph.dwHow		= dwHow;
	dipdw.dwData			= dwValue;

	return IDirectInputDevice2_SetProperty(pdev, guidProperty, &dipdw.diph);
}

/***************************************************************************
	Internal functions
 ***************************************************************************/
