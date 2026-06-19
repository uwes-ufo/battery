
#pragma once

#include <Windows.h>
#include <cstring>
#include <cmath>
#include <ctime>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <libloaderapi.h>
#include <sal.h>
#include <shellapi.h>
#include <hidusage.h>
#include "battery.h"
#include "resource.h"

constexpr int MAX_LOADSTRING = 100;

CHAR szTitle[MAX_LOADSTRING];
CHAR szWindowClass[MAX_LOADSTRING];
NOTIFYICONDATA nid;
HMENU hMenu;

static void darken(PCOLOR color, double brightness){
	PBYTE rgb = (PBYTE)color;
	for (int j = 0; j < 3; j++) rgb[j] = min(255, (BYTE)round(rgb[j] * brightness));
}
static COLOR colormixer(COLOR color0, COLOR color1, double ratio){
	ratio = max(0, min(1, ratio));
	darken(&color0, 1 - ratio);
	darken(&color1, ratio);
	return (COLOR)((UINT)color0 + (UINT)color1);
}
static COLOR thermometer(double temp){
	COLOR color = colormixer(COLOR::BLUE, COLOR::RED, (temp - TEMP.BLUE) / (TEMP.RED - TEMP.BLUE));
	PBYTE rgb = (PBYTE)&color;
	DEBUG(temp << "\t" << std::hex << (UINT)color << std::dec << "\n");
	return color;
}
static BOOL cursoronicon(void){
	NOTIFYICONIDENTIFIER ni = { sizeof(NOTIFYICONIDENTIFIER), nid.hWnd, nid.uID, nid.guidItem };
	POINT p;
	RECT r;
	GetCursorPos(&p);
	Shell_NotifyIconGetRect(&ni, &r);
	return r.left < p.x && p.x < r.right && r.top < p.y && p.y < r.bottom;
}
static BOOL getfocus(void){
	return GetFocus() || cursoronicon();
}
static BOOL isframe(PCOLOR color){
	PBYTE rgb = (PBYTE)color;
	for (int j = 1; j < 3; j++) if (rgb[0] != rgb[j]) return false;
	return true;
}
static void setled(LED led, LED thermo){
	HDC hdc = GetDC(NULL);
	HICON hIcon = nid.hIcon;
	ICONINFO ii;
	GetIconInfo(hIcon, &ii);
	BITMAP bm = { 0 };
	GetObject(ii.hbmColor, sizeof(BITMAP), &bm);
	SelectObject(hdc, &bm);
	BITMAPINFO3 b3 = { 0 };
	b3.bi.bmiHeader.biSize = sizeof(BITMAPINFO3);
	GetDIBits(hdc, ii.hbmColor, 0, 1, NULL, &b3.bi, DIB_PAL_COLORS);
	COLOR px[256];
	if (countof(px) < bm.bmWidth) return;
	for (int k = 0; k < bm.bmHeight; k++){
		GetDIBits(hdc, ii.hbmColor, k, 1, px, &b3.bi, DIB_PAL_COLORS);
		for (int j = 0; j < bm.bmWidth / 2; j++) if ((UINT)px[j]) px[j] = isframe(&px[j]) ? led.frame : led.face;
		for (int j = bm.bmWidth / 2; j < bm.bmWidth; j++) if ((UINT)px[j]) px[j] = isframe(&px[j]) ? thermo.frame : thermo.face;
		SetDIBits(hdc, ii.hbmColor, k, 1, px, &b3.bi, DIB_PAL_COLORS);
	}
	nid.hIcon = CreateIconIndirect(&ii);
	DestroyIcon(hIcon);
	ReleaseDC(NULL, hdc);
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}
static void getmenutext(IDM item, LPSTR text, UINT cch){
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	mii.fMask = MIIM_STRING;
	mii.cch = cch;
	mii.dwTypeData = text;
	GetMenuItemInfo(hMenu, (UINT)item, false, &mii);
}
void menutext(IDM item, LPSTR text){
	char otext[64];
	getmenutext(item, otext, sizeof(otext));
	if (strcmp(otext, text)){
		MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = text;
		SetMenuItemInfo(hMenu, (UINT)item, false, &mii);
	}
}
static BOOL getmenucheck(IDM item){
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	mii.fMask = MIIM_STATE;
	GetMenuItemInfo(hMenu, (UINT)item, false, &mii);
	return false != (mii.fState & MFS_CHECKED);
}
static void menucheck(IDM item, BOOL b){
	if (b != getmenucheck(item)){
		MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
		mii.fMask = MIIM_STATE;
		mii.fState = b ? MFS_CHECKED : MFS_UNCHECKED;
		SetMenuItemInfo(hMenu, (UINT)item, false, &mii);
	}
}
static BOOL menuhilite(IDM item){
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	mii.fMask = MIIM_STATE;
	GetMenuItemInfo(hMenu, (UINT)item, false, &mii);
	return false != (mii.fState & MFS_HILITE);
}
void tiptext(char* x){
	if (strcmp(nid.szTip, x)){
		strcpy_s(nid.szTip, x);
		Shell_NotifyIcon(NIM_MODIFY, &nid);
	}
}
static BOOL menuexists(IDM item){
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	mii.fMask = MIIM_ID;
	GetMenuItemInfo(hMenu, (UINT)item, false, &mii);
	return mii.wID == (UINT)item;
}
BOOL getverbosity(void){
	return menuexists(IDM::WATT);
}
void statusled(PBATTERY b){
	static LED oled, othermo;
	LED led = { COLOR::BLUE, COLOR::FRAME };
	LED thermo = { COLOR::BLUE, COLOR::FRAME };
	thermo.face = thermometer(b->temperature[0]);
	if (b->capacity.level < CAPACITY_LEVEL_WHITE) switch (b->state){
		case STATE::CHARGE: led.face = b->flags.airplane ? COLOR::PURPLE : COLOR::ORANGE; break;
	}
	else switch (b->state){
		case STATE::INACTIVE: led.face = COLOR::GRAY; break;
		case STATE::CHARGE: led.face = COLOR::WHITE; break;
	}
	if (GetFocus()) { darken(&led.face, 0.8); darken(&led.frame, 0.8); }
	if (memcmp(&oled, &led, sizeof(LED)) || memcmp(&othermo, &thermo, sizeof(LED))) { setled(led, thermo); oled = led; othermo = thermo; }
	menucheck(IDM::TEMPERATURE, b->temperature[0] > TEMP.THROTTLE);
	menucheck(IDM::LEVEL, b->state == STATE::CHARGE);
	menucheck(IDM::WATT, b->flags.airplane);
}
static void insertitem(IDM above, IDM wid, const char* text = "", HBITMAP bmp = NULL, UINT type = MFT_RADIOCHECK){
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	mii.fMask = MIIM_ID | MIIM_STRING | MIIM_BITMAP | MIIM_STATE | MIIM_FTYPE;
	mii.wID = (UINT)wid;
	mii.fType = type;
	mii.hbmpItem = bmp;
	mii.dwTypeData = (LPSTR)text;
	InsertMenuItem(hMenu, (UINT)above, false, &mii);
}
static void insertitem(IDM wid, const char* text = "", HBITMAP bmp = NULL){ insertitem(IDM::ZERO, wid, text, bmp); }

void setverbosity(BOOL b){
	BOOL v = getverbosity();
	if (b == ACTION::TOGGLE) b = !v;
	else { b = b > 0; if (!b && !menuhilite(IDM::TEMPERATURE) && getfocus()) return; }
	if (b != v){
		if (b) { insertitem(IDM::LEVEL); insertitem(IDM::WATT); }
		else { RemoveMenu(hMenu, (UINT)IDM::LEVEL, false); RemoveMenu(hMenu, (UINT)IDM::WATT, false); }
	}
}
static void setaccthreshold(int delta){
	if (delta != ACTION::TOGGLE){
		if (HIBYTE(GetAsyncKeyState(VK_LSHIFT))) delta *= 2;
		if (!HIBYTE(GetAsyncKeyState(VK_LCONTROL))) delta *= 5;
	}
	setthreshold(delta);
}
static void rawedit(int delta, USHORT usButtonFlags = RI_MOUSE_WHEEL){
	if (!getverbosity()) setverbosity(true);
	else if (menuhilite(IDM::TEMPERATURE)) setverbosity(delta);
	else if (menuhilite(IDM::LEVEL)) setaccthreshold(delta);
	else if (menuhilite(IDM::WATT)) setairplane(delta);
	else if (HIBYTE(GetAsyncKeyState(VK_RCONTROL))) setverbosity(delta);
	else if (cursoronicon()) switch (usButtonFlags){
		case RI_MOUSE_WHEEL: setaccthreshold(delta); break;
		case RI_MOUSE_HWHEEL: setairplane(delta); break;
	}
}
static void wmtimer(HWND hWnd, TIMER uElapse){
	static TIMER delay, elapse;
	delay = (TIMER)(uElapse < TIMER::SLOW ? 0 : (UINT)delay + (UINT)elapse);
	if (elapse != uElapse && !((UINT)delay && delay < TIMER::DELAY)) { elapse = uElapse; SetTimer(hWnd, 1, (UINT)elapse, NULL); }
	if (uElapse > TIMER::FAST) battery();
}
static void rawmouse(HWND hWnd, PRAWINPUT raw){
	if (getfocus()){
		int delta = COPYSIGN(1, *(short*)&raw->data.mouse.usButtonData);
		USHORT flags = raw->data.mouse.usButtonFlags;
		switch (flags) case RI_MOUSE_WHEEL: case RI_MOUSE_HWHEEL: rawedit(delta, flags);
	}
}
static void rawkeyboard(HWND hWnd, PRAWINPUT raw){
	if (raw->data.keyboard.Message == WM_KEYDOWN){
		if (getfocus()){
			if (HIBYTE(GetAsyncKeyState(VK_RCONTROL))) rawedit(ACTION::TOGGLE);
		}
		if (GetFocus()) switch (raw->data.keyboard.VKey){
			case VK_SPACE: rawedit(ACTION::TOGGLE); break;
			case VK_LEFT: rawedit(-1); break;
			case VK_RIGHT: rawedit(1); break;
			case 'A': setairplane(ACTION::TOGGLE); break;
			case 'T': setthreshold(ACTION::TOGGLE); break;
			case 'V': setverbosity(ACTION::TOGGLE); break;
			case 'Q': if (HIBYTE(GetAsyncKeyState(VK_LCONTROL))) SendMessage(hWnd, WM_CLOSE, 0, 0); break;
		}
	}
}
static void wminput(HWND hWnd, LPARAM lParam){
	RAWINPUT raw = { 0 };
	UINT size = sizeof(RAWINPUT);
	if (getfocus()) wmtimer(hWnd, TIMER::FAST);
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
	switch (raw.header.dwType){
		case RIM_TYPEMOUSE: rawmouse(hWnd, &raw); break;
		case RIM_TYPEKEYBOARD: rawkeyboard(hWnd, &raw); break;
	}
}
static void trackmenu(HWND hWnd, UINT uFlags){
	POINT p;
	if (SetForegroundWindow(hWnd) && GetCursorPos(&p)){
		switch ((IDM)TrackPopupMenuEx(hMenu, TPM_BOTTOMALIGN | TPM_RETURNCMD | uFlags, p.x, p.y, hWnd, NULL)){
			case IDM::QUIT: SendMessage(hWnd, WM_CLOSE, 0, 0); break;
			case IDM::TEMPERATURE: setverbosity(ACTION::TOGGLE); break;
			case IDM::LEVEL: setthreshold(ACTION::TOGGLE); break;
			case IDM::WATT: setairplane(ACTION::TOGGLE); break;
		}
	}
}
static void trackquitmenu(HWND hWnd){
	insertitem(IDM::TEMPERATURE, IDM::SEPARATOR, NULL);
	insertitem(IDM::SEPARATOR, IDM::QUIT, "Quit", HBMMENU_POPUP_CLOSE);
	trackmenu(hWnd, TPM_LEFTBUTTON);
	RemoveMenu(hMenu, (UINT)IDM::SEPARATOR, false);
	RemoveMenu(hMenu, (UINT)IDM::QUIT, false);
}
static void wmicon(HWND hWnd, LPARAM lParam){
	switch (LOWORD(lParam)){
		case WM_LBUTTONUP: trackmenu(hWnd, TPM_RIGHTBUTTON); break;
		case WM_RBUTTONUP: trackquitmenu(hWnd); break;
	}
}
static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message){
		case WM_CREATE: hMenu = CreatePopupMenu(); insertitem(IDM::TEMPERATURE); break;
		case WM_DESTROY: DestroyMenu(hMenu); PostQuitMessage(0); break;
		case WM_ICON: wmicon(hWnd, lParam); break;
		case WM_INPUT: wminput(hWnd, lParam); break;
		case WM_POWERBROADCAST: wmtimer(hWnd, TIMER::FAST); break;
		case WM_TIMER: wmtimer(hWnd, TIMER::SLOW); break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
static void icon(HINSTANCE hInstance, HWND hWnd){
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = IDI_ICON1;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = WM_ICON;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	Shell_NotifyIcon(NIM_ADD, &nid);
}
static void rawinput(HWND hWnd){
	RAWINPUTDEVICE rid = { 0 };
	rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid.hwndTarget = hWnd;
	rid.dwFlags = RIDEV_PAGEONLY | RIDEV_INPUTSINK;
	RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
}
static BOOL InitInstance(HINSTANCE hInstance){
	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) return FALSE;
	icon(hInstance, hWnd);
	rawinput(hWnd);
	wmtimer(hWnd, TIMER::SLOW);
	RegisterSuspendResumeNotification(hWnd, DEVICE_NOTIFY_WINDOW_HANDLE);
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);
	return TRUE;
}
static ATOM MyRegisterClass(HINSTANCE hInstance){
	WNDCLASSEX cex = { 0 };
	cex.cbSize = sizeof(WNDCLASSEX);
	cex.style = CS_HREDRAW | CS_VREDRAW;
	cex.lpfnWndProc = WndProc;
	cex.hInstance = hInstance;
	cex.hIconSm = cex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	cex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	cex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	cex.lpszMenuName = MAKEINTRESOURCE(IDC_BATTERY);
	cex.lpszClassName = szWindowClass;
	return RegisterClassEx(&cex);
}
int WinMain(_In_ HINSTANCE hi, _In_opt_ HINSTANCE hpi, _In_ LPSTR lpCmdLine, _In_ int nShowCmd){
	LoadString(hi, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hi, IDC_BATTERY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hi);
	if (!InitInstance(hi)) return 0;
	MSG msg = { 0 };
	while (GetMessage(&msg, nullptr, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
