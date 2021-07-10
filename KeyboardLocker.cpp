// KeyboardLocker.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <shellapi.h>
#include <commctrl.h>
#include "KeyboardLocker.h"

#define MAX_LOADSTRING						0x0100
#define ID_TRAY_LOCK_KEYBOARD				0x1235
#define ID_TRAY_UNLOCK_KEYBOARD				0x1236
#define ID_TRAY_EXIT					    0x1238

// Use a guid to uniquely identify our icon
class __declspec(uuid("9D0B8B92-4E1C-488e-A1E1-2331AFCE2CB5")) KeyboardIcon;

// Global Variables:
HINSTANCE hInst;                                // current instance
HMENU hLockedMenu;								// locked menu
HMENU hUnlockedMenu;							// unlocked menu
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL                AddNotificationIcon(HWND hwnd);
BOOL                ChangeNotificationIcon(HWND hwnd, BOOL locked);
BOOL                DeleteNotificationIcon();
void                ShowContextMenu(HWND hwnd, POINT pt, BOOL locked);
void				LockKeyboard();
void				UnlockKeyboard();
void				Cleanup();
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
HHOOK KeyboardHook;
UINT const WMAPP_NOTIFYCALLBACK_LOCKED = WM_APP + 1;
UINT const WMAPP_NOTIFYCALLBACK_UNLOCKED = WM_APP + 2;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_KEYBOARDLOCKER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, SW_HIDE))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYBOARDLOCKER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KEYBOARDLOCKER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_KEYBOARDLOCKER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // locked menu
   hLockedMenu = CreatePopupMenu();
   AppendMenu(hLockedMenu, MF_STRING, ID_TRAY_UNLOCK_KEYBOARD, L"Unlock");
   //AppendMenu(hLockedMenu, MF_SEPARATOR, 0, NULL);
   //AppendMenu(hLockedMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

   // unlocked menu
   hUnlockedMenu = CreatePopupMenu();
   AppendMenu(hUnlockedMenu, MF_STRING, ID_TRAY_LOCK_KEYBOARD, L"Lock");
   AppendMenu(hUnlockedMenu, MF_SEPARATOR, 0, NULL);
   AppendMenu(hUnlockedMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool isDone = false;
	
	switch (message)
    {
	case WM_CREATE:
		// add the notification icon
		AddNotificationIcon(hWnd);
		break;
    case WM_COMMAND:
        {
			int wmId = LOWORD(wParam);
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
        }
        break;
	case WMAPP_NOTIFYCALLBACK_LOCKED:
		switch (LOWORD(lParam))
		{
			case WM_CONTEXTMENU:
			{
				POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
				ShowContextMenu(hWnd, pt, true);
			}
			break;
		}
		break;	
	case WMAPP_NOTIFYCALLBACK_UNLOCKED:
		switch (LOWORD(lParam))
		{
			case WM_CONTEXTMENU:
			{
				POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
				ShowContextMenu(hWnd, pt, false);
			}
			break;
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

BOOL AddNotificationIcon(HWND hwnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hwnd;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = __uuidof(KeyboardIcon);
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK_UNLOCKED;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2));
	LoadString(hInst, IDS_UNLOCKED_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	Shell_NotifyIcon(NIM_ADD, &nid);

	// NOTIFYICON_VERSION_4 is prefered
	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL ChangeNotificationIcon(HWND hwnd, BOOL locked)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hwnd;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = __uuidof(KeyboardIcon);

	if (locked)
	{
		nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
		nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK_LOCKED;
		LoadString(hInst, IDS_LOCKED_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	}
	else
	{
		nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2));
		nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK_UNLOCKED;
		LoadString(hInst, IDS_UNLOCKED_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	}

	return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

BOOL DeleteNotificationIcon()
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_GUID;
	nid.guidItem = __uuidof(KeyboardIcon);
	return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hWnd, POINT pt, BOOL locked)
{
	// Get current mouse position.
	POINT curPoint;
	GetCursorPos(&curPoint);

	// should SetForegroundWindow according
	// to original poster so the popup shows on top
	SetForegroundWindow(hWnd);

	// TrackPopupMenu blocks the app until TrackPopupMenu returns
	if(locked)
		TrackPopupMenu(hLockedMenu, 0, curPoint.x, curPoint.y, 0, hWnd, NULL);
	else
		TrackPopupMenu(hUnlockedMenu, 0, curPoint.x, curPoint.y, 0, hWnd, NULL);
}

void Cleanup()
{
	UnlockKeyboard();
	DestroyMenu(hLockedMenu);
	DestroyMenu(hUnlockedMenu);
	DeleteNotificationIcon();
}