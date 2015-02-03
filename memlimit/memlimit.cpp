// memlimit.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <Psapi.h>
#include "resource.h"


#define THIS_CLASSNAME      "Memlimit hidden window"
#define THIS_TITLE          "Memlimit"

#define	HELP_ABOUT			THIS_TITLE " V1.0\n(C) 2014 Paul Qureshi"


enum {
	ID_TRAYICON			= 1,
	APPWM_TRAYICON		= WM_APP,
	APPWM_NOP			= WM_APP + 1,

	ID_ABOUT			= 2000,
	ID_EXIT,

	IDT_CHECKPROC		= 1,
};

static BOOL g_bModalState		= FALSE;
static unsigned long limit_mb	= 0;
static char proc_name[255]		= "\0";


void RegisterMainWndClass(HINSTANCE hInstance);
void MonitorProcess(char *proc_name, unsigned long limit_mb, HWND hWnd, UINT uID);


// ------------------------------------------------------------------------------------------------
// Parse command line
//
BOOL ParseCommandLine(LPSTR cmdline)
{
	if (__argc != 3)
	{
		MessageBox(NULL, "Usage: <process name> <limit MB>", THIS_TITLE, MB_ICONINFORMATION | MB_OK);
		return(FALSE);
	}
	
	limit_mb = strtol(__argv[2], NULL, 10);
	if (limit_mb == 0)
	{
		MessageBox(NULL, "Invalid limit", THIS_TITLE, MB_ICONINFORMATION | MB_OK);
		return(FALSE);
	}

	strncpy(proc_name, __argv[1], sizeof(proc_name));

	return(TRUE);
}

// ------------------------------------------------------------------------------------------------
// Main entry point
//
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE prev, LPSTR cmdline, int show)
{
	HMENU	hSysMenu	= NULL;
	HWND	hWnd		= NULL;
	HWND	hPrev		= NULL;
	MSG		msg;

	if (!ParseCommandLine(cmdline))
		return(1);

	// create invisible window to receive messages
	RegisterMainWndClass(hInst);
	hWnd = CreateWindow(THIS_CLASSNAME, THIS_TITLE, 0, 0, 0, 100, 100, NULL, NULL, hInst, NULL);
	if (!hWnd)
	{
		MessageBox(NULL, "Unable to create main window!", THIS_TITLE, MB_ICONERROR | MB_OK | MB_TOPMOST);
		return(1);
	}

	// set up process checking timer
	SetTimer(hWnd, IDT_CHECKPROC, 5000, (TIMERPROC)NULL);

	// message loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnregisterClass(THIS_CLASSNAME, hInst);

	return(0);
}

// ------------------------------------------------------------------------------------------------
// Add system tray icon
//
void AddTrayIcon(HWND hWnd, UINT uID, UINT uCallbackMsg, UINT uIcon, LPSTR pszToolTip)
{
	NOTIFYICONDATA	nid;

	memset(&nid, 0, sizeof(nid));

	nid.cbSize				= sizeof(nid);
	nid.hWnd				= hWnd;
	nid.uID					= uID;
	nid.uFlags				= NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage	= uCallbackMsg;
	nid.hIcon				= LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(uIcon));
	
	strcpy(nid.szTip, pszToolTip);

	Shell_NotifyIcon(NIM_ADD, &nid);
}

// ------------------------------------------------------------------------------------------------
// Update tray icon
//
void ModifyTrayIcon(HWND hWnd, UINT uID, UINT uIcon, LPSTR pszToolTip)
{
	NOTIFYICONDATA	nid;

	memset(&nid, 0, sizeof(nid));

	nid.cbSize				= sizeof(nid);
	nid.hWnd				= hWnd;
	nid.uID					= uID;

	if (uIcon != NULL)
	{
		nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(uIcon));
		nid.uFlags |= NIF_ICON;
	}

	if (pszToolTip)
	{
		strcpy(nid.szTip, pszToolTip);
		nid.uFlags |= NIF_TIP;
	}

	if ((uIcon != NULL) || (pszToolTip))
		Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// ------------------------------------------------------------------------------------------------
// Remove system tray icon
//
void RemoveTrayIcon(HWND hWnd, UINT uID)
{
	NOTIFYICONDATA	nid;

	memset(&nid, 0, sizeof(nid));

	nid.cbSize	= sizeof(nid);
	nid.hWnd	= hWnd;
	nid.uID		= uID;

	Shell_NotifyIcon(NIM_DELETE, &nid);
}

// ------------------------------------------------------------------------------------------------
// Display a pop-up balloon message
//
void DisplayBalloon(HWND hWnd, UINT uID, LPSTR pszMessage)
{
	NOTIFYICONDATA	nid;

	memset(&nid, 0, sizeof(nid));

	nid.cbSize				= sizeof(nid);
	nid.hWnd				= hWnd;
	nid.uID					= uID;
	nid.uFlags				= NIF_INFO;
	strcpy(nid.szInfoTitle, THIS_TITLE);
	strcpy(nid.szInfo, pszMessage);
	nid.uTimeout			= 5000;

	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// ------------------------------------------------------------------------------------------------
void OnClose(HWND hWnd)
{
	RemoveTrayIcon(hWnd, ID_TRAYICON);
	PostQuitMessage(0);
}

// ------------------------------------------------------------------------------------------------
// Create pop-up menu when tray icon right-clicked
//
BOOL ShowPopupMenu(HWND hWnd, POINT *curpos, int wDefaultItem)
{
	HMENU	hPop		= NULL;
	int		i			= 0;
	WORD	cmd;
	POINT	pt;

	if (g_bModalState)	// already displayed
		return(false);

	hPop = CreatePopupMenu();

	if (!curpos)
	{
		GetCursorPos(&pt);
		curpos = &pt;
	}

	InsertMenu(hPop, i++, MF_BYPOSITION | MF_STRING, ID_ABOUT, "About");
	InsertMenu(hPop, i++, MF_BYPOSITION | MF_STRING, ID_EXIT, "Exit");
	//SetMenuDefaultItem(hPop, ID_ABOUT, FALSE);
	
	SetFocus(hWnd);
	SendMessage(hWnd, WM_INITMENUPOPUP, (WPARAM)hPop, 0);

	cmd = TrackPopupMenu(hPop, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, curpos->x, curpos->y, 0, hWnd, NULL);
	SendMessage(hWnd, WM_COMMAND, cmd, 0);

	DestroyMenu(hPop);
	return(cmd);
}

// ------------------------------------------------------------------------------------------------
void OnTrayIconRBtnUp(HWND hWnd)
{
	SetForegroundWindow(hWnd);
	ShowPopupMenu(hWnd, NULL, -1);
	PostMessage(hWnd, APPWM_NOP, 0, 0);
}

// ------------------------------------------------------------------------------------------------
// Message handler
//
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_CREATE:
			AddTrayIcon(hWnd, ID_TRAYICON, APPWM_TRAYICON, IDI_MEMORY, "Starting up...");
			MonitorProcess(proc_name, limit_mb, hWnd, ID_TRAYICON);
			return(0);

		case APPWM_NOP:			// see OnTrayIconRBtnUp()
			return(0);

		case APPWM_TRAYICON:	// mouse events involving our tray icon
			SetForegroundWindow(hWnd);

			switch(lParam)
			{
				case WM_RBUTTONUP:	// see OnTrayIconRBtnUp()
					OnTrayIconRBtnUp(hWnd);
					return(0);

			}
			return(0);

		case WM_COMMAND:
			//return(OnCommand(hWnd, LOWORD(wParam), (HWND)lParam);
			switch (LOWORD(wParam))
			{
				case ID_ABOUT:
					g_bModalState = TRUE;
					MessageBox(hWnd, HELP_ABOUT, THIS_TITLE, MB_ICONINFORMATION | MB_OK);
					g_bModalState = FALSE;
					break;
				case ID_EXIT:
					PostMessage(hWnd, WM_CLOSE, 0, 0);
					break;
			}
			return(0);

		case WM_TIMER:			// time to check the monitored process
			if (wParam == IDT_CHECKPROC)
				MonitorProcess(proc_name, limit_mb, hWnd, ID_TRAYICON);
				//DisplayBalloon(hWnd, ID_TRAYICON, APPWM_TRAYICON, "test");
			return(0);

		case WM_CLOSE:
			OnClose(hWnd);
			// fall through
		default:
			return(DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
}

// ------------------------------------------------------------------------------------------------
void RegisterMainWndClass(HINSTANCE hInstance)
{
	WNDCLASSEX	wc;
	memset(&wc, 0, sizeof(wc));

	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);	// special magic
	wc.lpszMenuName = NULL;
	wc.lpszClassName = THIS_CLASSNAME;

	RegisterClassEx(&wc);
}


// ------------------------------------------------------------------------------------------------
// Find a process by it's name
//
DWORD FindProcessByName(char *name)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return(1);
	
	cProcesses = cbNeeded / sizeof(DWORD);

	for (i = 0; i < cProcesses; i++)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
		char szProcessName[MAX_PATH];

		// get process name
		if (hProcess != NULL)
		{
			HMODULE hMod;
			DWORD cbNeeded;

			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
				GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(char));
		}

		CloseHandle(hProcess);

		if (strcmp(szProcessName, name) == 0)
			return(aProcesses[i]);
	}

	return(0);
}

// ------------------------------------------------------------------------------------------------
// Check memory use of a process
//
void MonitorProcess(char *proc_name, unsigned long limit_mb, HWND hWnd, UINT uID)
{
	char tooltip[100];
	UINT icon = NULL;

	DWORD processID;
	processID = FindProcessByName(proc_name);
	if (processID == 0)
	{
		sprintf(tooltip, "%s: not running KB\nLimit: %u KB", proc_name, limit_mb * 1024);
		icon = IDI_BLOCK;
		goto set_tray;
	}


	HANDLE hProcess;
	PROCESS_MEMORY_COUNTERS_EX pmc;

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, processID);
	if (hProcess == NULL)
	{
		sprintf(tooltip, "%s: error KB\nLimit: %u KB", proc_name, limit_mb * 1024);
		icon = IDI_EXCLAMATIONZ;
		goto set_tray;
	}

	if (!GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
	{
		sprintf(tooltip, "%s: error KB\nLimit: %u KB", proc_name, limit_mb * 1024);
		icon = IDI_EXCLAMATIONZ;
		goto set_tray;
	}


	sprintf(tooltip, "%s: %lu KB\nLimit: %u KB", proc_name, pmc.WorkingSetSize / 1024, limit_mb * 1024);
	if ((pmc.WorkingSetSize / (1024 * 1024)) > limit_mb)	// need to kill task
	{
		sprintf(tooltip, "%s: %lu KB\nLimit: %u KB\nProcess terminated.", proc_name, pmc.WorkingSetSize / 1024, limit_mb * 1024);
		DisplayBalloon(hWnd, uID, tooltip);
		icon = IDI_MINE;
		TerminateProcess(hProcess, 0);
	}
	else
	{
		sprintf(tooltip, "%s: %lu KB\nLimit: %u KB", proc_name, pmc.WorkingSetSize / 1024, limit_mb * 1024);
		icon = IDI_TICK;
	}

set_tray:
	ModifyTrayIcon(hWnd, uID, icon, tooltip);
	return;
}
