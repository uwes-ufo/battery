
#include <Windows.h>
#include <corecrt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <ios>
#include <iostream>
#include "battery.h"
#include "driver.h"

static BOOL diff(PBATTERY oldbattery, PBATTERY newbattery, double* newvalue, double threshold = 0, BOOL relative = false){
	if (relative) threshold *= abs(*newvalue);
	double* oldvalue = (double*)((char*)newvalue + ((char*)oldbattery - (char*)newbattery));
	return abs(*oldvalue - *newvalue) > threshold;
}
static BOOL isnew(PBATTERY o, PBATTERY b){
	if (o->state != b->state){ setverbosity(b->state != STATE::INACTIVE); return true; }
	if (o->operatinghours != b->operatinghours) return true;
	if (o->ufos != b->ufos) return true;
	if (memcmp(&o->flags, &b->flags, sizeof(BATTERYFLAGS))) return true;
	for (int j = 0; j < countof(BATTERY::temperature); j++) if (diff(o, b, &b->temperature[j], 200e-3)) return true;
	if (diff(o, b, &b->voltage.series, 200e-3)) return true;
	//	if (diff(o, b, &b->capacity.threshold)) return true;
	if (diff(o, b, &b->power, 200e-3, true)) return true;
	return false;
}
static BOOL isplausible(PBATTERY b){
	for (int j = 0; j < countof(BATTERY::temperature); j++) if (b->temperature[j] < -50 || 150 < b->temperature[j]) return false;
	if (b->voltage.series < 5 || 15 < b->voltage.series) return false;
	return true;
}
static void writelog(PBATTERY b, const char* filename){
	const char s[] = ",";
	BOOL h = !std::filesystem::exists(filename);
	std::ofstream file(filename, std::ios_base::app);
	if (h) file << "date,°C,°C,V,Uq,cell,cell,cell,off,%,v2%,full,Wh,Limit,A,W,AC,cycles,hours,time,t,total,ufo,sub,ufo,off,fu,en,z,12,d5,e,air,ufos\n";
	file << b->time.text << s;
	for (int j = 0; j < countof(BATTERY::temperature); j++) file << b->temperature[j] << s;
	file << b->voltage.single << s;
	file << b->voltage.idle << s;
	for (int j = 0; j < countof(BATTERY::voltage.cell); j++) file << b->voltage.cell[j] << s;
	file << b->capacity.threshold << s;
	file << b->capacity.level << s;
	file << b->capacity.v2level << s;
	file << b->capacity.fullCharge << s;
	file << b->capacity.remaining << s;
	file << b->current.limit << s;
	file << b->current.lowpass << s;
	file << b->power << s;
	file << b->acAdapter << s;
	file << b->cycles << s;
	file << b->operatinghours << s;
	file << b->duration.minutes << s;
	file << b->duration.driver << s;
	file << b->capacity.total << s;
	file << b->ufovoltage << s;
	file << b->subcycle << s;
	file << b->ufo121 << s;
	file << b->flags.off << s;
	file << b->flags.full << s;
	file << b->flags.enable << s;
	file << b->flags.zero20 << s;
	file << b->flags.below12 << s;
	file << b->flags.dischargebelow5 << s;
	file << b->flags.empty08 << s;
	file << b->flags.airplane << s;
	file << b->ufos << "\n";
}
static void logger(PBATTERY o, PBATTERY b){
	if (isnew(o, b)){
		*o = *b;
		char logfile[256];
		sprintf_s(logfile, LOG"battery.%d.csv", b->serialnumber);
		writelog(b, isplausible(b) ? logfile : LOG"badbatt.csv");
	}
}
void batteryinfo(PBATTERY b){
	char filename[256];
	sprintf_s(filename, LOG"battery.%d.info", b->serialnumber);
	if (!std::filesystem::exists(filename)){
		std::ofstream file(filename);
		file << "chemistry:              " << b->chemistry << "\n";
		file << "manufacturer:           " << b->manufacturer << "\n";
		file << "fru:                    " << b->fru << "\n";
		file << "firmwareversion:        " << b->firmwareversion << "\n";
		file << "manufacturedate:        " << b->manufacturedate << "\n";
		file << "barcode:                " << b->barcode << "\n";
		file << "serialnumber:           " << b->serialnumber << "\n";
		file << "firstuseddate:          " << b->firstuseddate << "\n";
		file << "cycles:                 " << b->cycles << "\n";
		file << "operatinghours:         " << b->operatinghours << " h\n";
		file << "total charge:           " << b->capacity.total << " Wh\n";
		file << "capacity.design:        " << b->capacity.design << " Wh\n";
		file << "capacity.fullCharge:    " << b->capacity.fullCharge << " Wh\n";
		file << "voltage.design:         " << b->voltage.design << " V\n";
		file << "current.limit:          " << b->current.limit << " A\n";
	}
}
static void thresholdstep(void){
	int u = ibmpmdrv(IBM::GET_UPPER).value, v = u;
	if (!u) u = 100;
	u = max(1, min(100, u + 10));
	if (u == 100) u = 0;
	if (u != v){
		int l = max(0, u - 1);
		ibmpmdrv(IBM::SET_UPPER, u, IBMRW::WRITE);
		ibmpmdrv(IBM::SET_LOWER, l, IBMRW::WRITE);
	}
}
void periodlogger(PBATTERY b){
	constexpr int PERIOD = 24 * 60 * 60;
	time_t now = b->time.raw / PERIOD;
	static time_t next = now;	// +1;
	if (now >= next){
		next = now + 1;
		//		thresholdstep();
		char logfile[256];
		sprintf_s(logfile, LOG"battery.%d.csv", b->serialnumber);
		writelog(b, logfile);
	}
}
