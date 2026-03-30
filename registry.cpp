
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <handleapi.h>
#include <iostream>
#include <libloaderapi.h>
#include <rpcndr.h>
#include <tlhelp32.h>
#include <winreg.h>
#include "battery.h"

typedef BOOL(WINAPI* TH32_PROCESS)
(HANDLE hSnapShot, LPPROCESSENTRY32 lppe);

static TH32_PROCESS pProcess32First = NULL, pProcess32Next = NULL;

static BOOL isProcessAlive(const char process_name[]){
	PROCESSENTRY32 pe32 = { 0 };
	HANDLE hSnapshot = NULL;
	HINSTANCE hDll = LoadLibrary("kernel32.dll");
	if (hDll){
		pProcess32First = (TH32_PROCESS)GetProcAddress(hDll, "Process32First");
		pProcess32Next = (TH32_PROCESS)GetProcAddress(hDll, "Process32Next");
		bool process_state = false;
		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != (HANDLE)-1){
			pe32.dwSize = sizeof(PROCESSENTRY32);
			if (pProcess32First(hSnapshot, &pe32)){
				do{
					if (!_strcmpi(pe32.szExeFile, process_name)) process_state = true;
				} while (pProcess32Next(hSnapshot, &pe32));
			}
			CloseHandle(hSnapshot);
		}
		return process_state;
	}
	return false;
}
static void w2r(PBATTERY b, LPCSTR sensor, LPCSTR name, double value, LPCSTR unit){
	char val[256], key[256] = "Software\\HWiNFO64\\Sensors\\Custom\\Battery";
	HKEY h;
	sprintf_s(key, "%s: %s %s %s %s %s %d %s\\%s", key, b->manufacturer, b->fru, b->firmwareversion, b->manufacturedate, b->barcode, b->serialnumber, b->firstuseddate, sensor);
	sprintf_s(val, "%f", value);
	if (RegCreateKeyA(HKEY_CURRENT_USER, key, &h)) std::cout << "fail RegCreateKeyA\n";
	if (RegSetValueExA(h, "Name", 0, REG_SZ, (byte*)name, (DWORD)strlen(name))) std::cout << "fail Name\n";
	if (RegSetValueExA(h, "Value", 0, REG_SZ, (byte*)val, (DWORD)strlen(val))) std::cout << "fail Value\n";
	if (unit && RegSetValueExA(h, "Unit", 0, REG_SZ, (byte*)unit, (DWORD)strlen(unit))) std::cout << "fail Unit\n";
	if (RegCloseKey(h)) std::cout << "fail RegCloseKey\n";
}
static void w2r(PBATTERY b, LPCSTR sensor, LPCSTR name, double value) {w2r(b, sensor, name, value, 0);}
void write2registry(PBATTERY b){
	if (!isProcessAlive("HWiNFO64.EXE")) return;
	w2r(b, "Temp0", "Temperature", b->temperature[0]);
	w2r(b, "Temp1", "Temperature #2", b->temperature[1]);
	w2r(b, "Volt0", "Cell #1", b->voltage.cell[0]);
	w2r(b, "Volt1", "Cell #2", b->voltage.cell[1]);
	w2r(b, "Volt2", "Cell #3", b->voltage.cell[2]);
	w2r(b, "Volt3", "EOC", b->voltage.endofcharge);
	w2r(b, "Volt4", "Voltage", b->voltage.series);
	w2r(b, "Current0", "Amperage", b->current.lowpass);
	w2r(b, "Current1", "Amperage Limit", b->current.limit);
	w2r(b, "Power0", "AC", b->acAdapter);
	w2r(b, "Power1", "Charge Rate", b->power);
	w2r(b, "Usage0", "Charge Level", 100 * b->capacity.level);
	w2r(b, "Usage1", "Charge Stop", 100 * b->capacity.threshold);
	w2r(b, "Usage2", "V2Level", 100 * b->capacity.v2level);
	w2r(b, "Usage3", "Wear Level", 100 * (1 - b->capacity.fullCharge / b->capacity.design));
	w2r(b, "Other0", "Remaining Capacity", b->capacity.remaining, "Wh");
	w2r(b, "Other1", "Design Capacity", b->capacity.design, "Wh");
	w2r(b, "Other2", "Full Charge Capacity", b->capacity.fullCharge, "Wh");
	w2r(b, "Other3", "Estimated Remaining Time", b->duration.driver, "min");
	w2r(b, "Other4", "Time", b->duration.minutes, "min");
	w2r(b, "Other5", "Cycles", b->cycles);
	w2r(b, "Other6", "Operating Hours", b->operatinghours, "h");
	w2r(b, "Other7", "Total Charge", b->capacity.total, "Wh");
}
