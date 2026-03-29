
#include <Windows.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <ioapiset.h>
#include <iostream>
#include <usbiodef.h>
#include "driver.h"

static HANDLE CreateFile(LPCSTR lpFileName){
	return CreateFile(lpFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}
BOOL tppwrif(PTPPWRIF out, TPI index, BYTE flag){
	DWORD dwOut = 0;
	TPPWRIFIN in = { 0 };
	in.index = (BYTE)index;
	in.flag = flag;
	HANDLE h = CreateFile("\\\\.\\TPPWRIF");
	return DeviceIoControl(h, USB_CTL(0x810), &in, sizeof(in), out, sizeof(TPPWRIF), &dwOut, NULL);
}
IBMDW ibmpmdrv(IBM id, BYTE value, IBMRW flag){
	DWORD dwOut = 0;
	IBMDW out = { 0 }, in = { 0 };
	in.value = value;
	in.flag = (BYTE)flag;
	HANDLE h = CreateFile("\\\\.\\IBMPmDrv");
	if (!DeviceIoControl(h, USB_CTL((UINT)id), &in, sizeof(in), &out, sizeof(out), &dwOut, NULL)) 
		std::cout << "mecker: " << GetLastError() << "\n";
	return out;
}
