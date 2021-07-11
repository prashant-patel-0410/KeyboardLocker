// SysTrayDemo.cpp : Defines the entry point for the application.
//

// Windows Header Files:
#include <windows.h>
#include <shellapi.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "KeyboardLocker.h"

#define MAX_LOADSTRING						100
#define ID_TRAY_LOCK_KEYBOARD				1001
#define ID_TRAY_UNLOCK_KEYBOARD				1002
#define ID_TRAY_EXIT					    1003
#define	WM_USER_LOCKED						WM_USER + 1
#define	WM_USER_UNLOCKED					WM_USER + 2

// Global Variables:
HINSTANCE hInst;                                // current instance
HMENU hLockedMenu;								// locked menu
HMENU hUnlockedMenu;							// unlocked menu
NOTIFYICONDATA nidApp;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szApplicationToolTip[MAX_LOADSTRING];	    // the main window class name
BOOL bDisable = FALSE;							// keep application state
HHOOK KeyboardHook;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL                ChangeNotificationIcon(HWND hwnd, BOOL locked);
void				LockKeyboard();
void				UnlockKeyboard();
LRESULT CALLBACK	LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
void				Cleanup();

int APIENTRY wWinMain(
	_In_		HINSTANCE hInstance,
	_In_opt_	HINSTANCE hPrevInstance,
	_In_		LPTSTR    lpCmdLine,
	_In_		int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_KEYBOARDLOCKER, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYBOARDLOCKER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOCKED_ICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_KEYBOARDLOCKER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_LOCKED_ICON));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	// locked menu
	hLockedMenu = CreatePopupMenu();
	AppendMenu(hLockedMenu, MF_STRING, ID_TRAY_UNLOCK_KEYBOARD, L"Unlock");

	// unlocked menu
	hUnlockedMenu = CreatePopupMenu();
	AppendMenu(hUnlockedMenu, MF_STRING, ID_TRAY_LOCK_KEYBOARD, L"Lock");
	AppendMenu(hUnlockedMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hUnlockedMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

	nidApp.cbSize = sizeof(NOTIFYICONDATA); // sizeof the struct in bytes 
	nidApp.hWnd = (HWND)hWnd;              //handle of the window which will process this app. messages 
	nidApp.uID = IDI_UNLOCKED_ICON;           //ID of the icon that willl appear in the system tray 
	nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; //ORing of all the flags 
	nidApp.hIcon = LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_UNLOCKED_ICON)); // handle of the Icon to be displayed, obtained from LoadIcon 
	nidApp.uCallbackMessage = WM_USER_UNLOCKED;
	LoadString(hInstance, IDS_UNLOCKED_TOOLTIP, nidApp.szTip, MAX_LOADSTRING);
	Shell_NotifyIcon(NIM_ADD, &nidApp);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT lpClickPoint;

	switch (message)
	{
	case WM_USER_LOCKED:
		switch (LOWORD(lParam))
		{
		case WM_RBUTTONDOWN:
			UINT uFlag = MF_BYPOSITION | MF_STRING;
			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hLockedMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
			return TRUE;
		}
		break;
	case WM_USER_UNLOCKED:
		switch (LOWORD(lParam))
		{
		case WM_RBUTTONDOWN:
			UINT uFlag = MF_BYPOSITION | MF_STRING;
			GetCursorPos(&lpClickPoint);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hUnlockedMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
			return TRUE;
		}
	break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_TRAY_LOCK_KEYBOARD:
		{
			ChangeNotificationIcon(hWnd, true);
			LockKeyboard();
			break;
		}
		case ID_TRAY_UNLOCK_KEYBOARD:
		{
			ChangeNotificationIcon(hWnd, false);
			UnlockKeyboard();
			break;
		}
		case ID_TRAY_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

BOOL ChangeNotificationIcon(HWND hwnd, BOOL locked)
{
	if (locked)
	{
		nidApp.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOCKED_ICON));
		nidApp.uCallbackMessage = WM_USER_LOCKED;
		LoadString(hInst, IDS_LOCKED_TOOLTIP, nidApp.szTip, ARRAYSIZE(nidApp.szTip));
	}
	else
	{
		nidApp.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_UNLOCKED_ICON));
		nidApp.uCallbackMessage = WM_USER_UNLOCKED;
		LoadString(hInst, IDS_UNLOCKED_TOOLTIP, nidApp.szTip, ARRAYSIZE(nidApp.szTip));
	}

	return Shell_NotifyIcon(NIM_MODIFY, &nidApp);
}
void LockKeyboard()
{
	if (KeyboardHook != NULL)
		UnhookWindowsHookEx(KeyboardHook);

	KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
}

void UnlockKeyboard()
{
	if (KeyboardHook != NULL)
	{
		BOOL isDone = UnhookWindowsHookEx(KeyboardHook);
		if (isDone)
			KeyboardHook = NULL;
	}
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		switch (wParam)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			return 1;
			break;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Cleanup()
{
	UnlockKeyboard();
	DestroyMenu(hLockedMenu);
	DestroyMenu(hUnlockedMenu);
	Shell_NotifyIcon(NIM_DELETE, &nidApp);
}
