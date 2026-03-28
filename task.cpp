
#include <Windows.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "battery.h"
#include "driver.h"

BATTERY batt, logged;

static double celsius(WORD t){ return 100e-3 * t - 273.2; }
static double v2level(double v){ return max(0, min(1, (v - 3.576) * 1.144 + tanh((v - 3.829) * 14.79) * 0.144)); }
static void ufosight(PBATTERY b, UFOS u){ b->ufos |= 1 << (UINT)u; }
static void ufoflags(PBATTERY b, PTPPWRIF ti){
	if (ti->flags.l0 || !ti->flags.l1 || ti->flags.h0 || ti->flags.h3 != 3 || ti->flags.x80 != 0x80) ufosight(b, UFOS::FLAGS);
}
static void primary(PBATTERY b){
	const double internalresistance = 60e-3;
	PCAPACITY cap = &b->capacity;
	TPPWRIF ti = { 0 };
	tppwrif(&ti, TPI::T1);
	tppwrif(&ti, TPI::T1);
	if (ti.flags.raw[0] != 0x102 || ti.flags.raw[1] != 0xf0) ufosight(b, UFOS::T1FLAGS);
	for (int j = 2; j < countof(TPPWRIF::raw); j++) if (ti.raw[j]) ufosight(b, UFOS::T1);
	tppwrif(&ti, TPI::PRIMARY);
	ufoflags(b, &ti);
	b->state = ti.flags.discharge ? STATE::DISCHARGE : ti.flags.charge ? STATE::CHARGE : STATE::INACTIVE;
	b->flags.empty08 = ti.flags.empty08;
	if (ti.p.xe024 != 0xe024) ufosight(b, UFOS::XE024);
	b->temperature[0] = celsius(ti.p.temperature);
	b->voltage.series = 1e-3 * ti.p.voltage;
	b->voltage.single = b->voltage.series / 3;
	b->current.value = 1e-3 * ti.p.current;
	b->current.lowpass = 1e-3 * ti.p.lowpasscurrent;
	b->voltage.idle = b->voltage.single - internalresistance * b->current.lowpass;
	b->power = b->voltage.series * b->current.lowpass;
	cap->v2level = v2level(b->voltage.idle);
	cap->remaining = 10e-3 * ti.p.remainingcapacity;
	cap->fullCharge = 10e-3 * ti.p.fullcapacity;
	cap->level = cap->remaining / cap->fullCharge;
	b->duration.discharge = ti.p.dischargeminutes[0] < 0 ? 0 : ti.p.dischargeminutes[0] / 60.0;
	b->duration.driver = ti.p.chargeminutes >= 0 ? ti.p.chargeminutes : ti.p.dischargeminutes[1] >= 0 ? ti.p.dischargeminutes[1] / 60.0 : 0;
	b->flags.off = ti.p.flags.off;
	b->flags.full = ti.p.flags.full;
	b->flags.dischargebelow5 = ti.p.flags.dischargebelow5;
	b->flags.below12 = ti.p.flags.below12;
	b->flags.zero20 = ti.p.flags.zero20;
	if (ti.p.flags.l0 || !ti.p.flags.l1 || ti.p.flags.h0) ufosight(b, UFOS::PFLAGS);
	b->cycles = ti.p.cycles;
	if (ti.p.x0) ufosight(b, UFOS::P0);
}
static void tpdate(PCHAR text, TPDATE d){ sprintf_s(text, sizeof(BATTERY::firstuseddate), "%04d-%02d-%02d", 1980 + d.year, d.month, d.day); }
static void secondary(PBATTERY b){
	TPPWRIF ti = { 0 };
	tppwrif(&ti, TPI::SECONDARY);
	ufoflags(b, &ti);
	b->temperature[1] = celsius(ti.s.temperature);
	b->capacity.design = 10e-3 * ti.s.designcapacity;
	b->voltage.design = 1e-3 * ti.s.designvoltage;
	if (ti.s.ufo49 != 49) ufosight(b, UFOS::D49);
	tpdate(b->manufacturedate, ti.s.manufacturedate);
	b->serialnumber = ti.s.serialnumber;
	b->capacity.minimal = ti.s.minimalcapacity;
	tpdate(b->firstuseddate, ti.s.firstuseddate);
	if (ti.s.ufovoltage != 11137) ufosight(b, UFOS::D11137);
	b->acAdapter = ti.s.acwattage;
	if (ti.s.x80 != 0x80) ufosight(b, UFOS::X80);
	for (int j = 0; j < countof(TPPWRIF::s.x0); j++) if (ti.s.x0[j]) ufosight(b, UFOS::S0);
}
static void firmware(PCHAR text, WORD f[]){ sprintf_s(text, sizeof(BATTERY::firmwareversion), "%04x-%04x-%04x-%04x", f[0], f[1], f[2], f[3]); }
static void firmware(PBATTERY b){
	TPPWRIF ti = { 0 };
	tppwrif(&ti, TPI::FIRMWARE);
	ufoflags(b, &ti);
	firmware(b->firmwareversion, ti.f.firmwareversion);
	b->flags.enable = ti.f.flags.enable;
	if (ti.f.flags.l0 || !ti.f.flags.l1 || ti.p.flags.h0) ufosight(b, UFOS::FFLAGS);
	b->subcycle = ti.f.subcycle;
	b->current.limit = 1e-3 * ti.f.currentlimit;
	b->voltage.endofcharge = 1e-3 / 3.0 * ti.f.endofchargevoltage;
	for (int j = 0; j < countof(TPPWRIF::f.x0); j++) if (ti.f.x0[j]) ufosight(b, UFOS::F0);
}
static void cell(PBATTERY b){
	TPPWRIF ti = { 0 };
	tppwrif(&ti, TPI::CELL);
	ufoflags(b, &ti);
	for (int j = 0; j < countof(BATTERY::voltage.cell); j++) b->voltage.cell[j] = 1e-3 * ti.c.cellvoltage[j];
	if (ti.c.x6c05 != 0x6c05) ufosight(b, UFOS::X6C05);
	if (ti.c.x6c01 != 0x6c01) ufosight(b, UFOS::X6C01);
	if (ti.c.x0) ufosight(b, UFOS::C0);
	b->capacity.total = ti.c.totalcharge;
	b->operatinghours = ti.c.operatinghours;
	b->ufovoltage = ti.c.ufovoltage;
	b->ufo121 = ti.c.ufo121;
	b->ufo61 = ti.c.ufo61;
	for (int j = 0; j < countof(TPPWRIF::c.xx0); j++) if (ti.c.xx0[j]) ufosight(b, UFOS::XX0);
}
static void text(PBATTERY b){
	if (strlen(b->manufacturer)) return;
	TPPWRIF ti = { 0 };
	if (tppwrif(&ti, TPI::MANUFACTURER)) strcpy_s(b->manufacturer, ti.text);
	if (tppwrif(&ti, TPI::FRU)) strcpy_s(b->fru, ti.text);
	if (tppwrif(&ti, TPI::CHEMISTRY)) strcpy_s(b->chemistry, ti.text);
	if (tppwrif(&ti, TPI::BARCODE)) strcpy_s(b->barcode, ti.text);
}
static void threshold(PBATTERY b){
	IBMFLAGS ibm = ibmpmdrv(IBM::GET_AIRPLANE).flags;
	b->flags.airplane = ibm.airplane;
	if (ibm.l5 != 5 || ibm.h1 != 1) ufosight(b, UFOS::IBMFLAGS);
	int u = ibmpmdrv(IBM::GET_UPPER).value;
	if (u) b->capacity.threshold = b->capacity.memorizedthreshold = 10e-3 * u; else b->capacity.threshold = 1;
	if (b->capacity.memorizedthreshold < 5e-3) b->capacity.memorizedthreshold = 30e-2;
}
static void duration(PBATTERY b){
	PCAPACITY cap = &b->capacity;
	double v = 0, thres = cap->threshold, reserve = cap->level < 50e-3 ? 0 : 50e-3;
	if (thres < 1) thres -= 5e-3;
	if (b->power != 0) switch (b->state){
		case STATE::DISCHARGE: v = (reserve * cap->fullCharge - cap->remaining) / b->power; break;
		case STATE::CHARGE: v = 60 * (thres * cap->fullCharge - cap->remaining) / b->power; break;
	}
	b->duration.value = v;
	int minutes = (int)round(60 * abs(v)), hours = minutes / 60; minutes %= 60;
	sprintf_s(b->duration.tip, "%s%d:%02d", v < 0 ? "-" : "", hours, minutes);
}
static void time(PBATTERY b){
	time(&b->time);
}
static void tiptext(PBATTERY b){
	char temp[32], charge[32], tip[64];
	sprintf_s(temp, "%1.1f°C", b->temperature[0]);
	strcpy_s(tip, temp);
	if (getverbosity()){
		char watt[32];
		sprintf_s(temp, "%s  %1.2f V", temp, b->voltage.single);
		sprintf_s(charge, "%1.0f%%  :  %1.1f%%", 100 * b->capacity.threshold, 100 * b->capacity.level);
		sprintf_s(watt, "%s %s %1.0f W", b->duration.tip, b->flags.airplane ? "#" : "@", abs(b->power));
		sprintf_s(tip, "%s\n%s\n%s", temp, charge, watt);
		menutext(IDM::LEVEL, charge);
		menutext(IDM::WATT, watt);
	}
	menutext(IDM::TEMPERATURE, temp);
	tiptext(tip);
}
void setthreshold(int d){
	PBATTERY b = &batt;
	int u = ibmpmdrv(IBM::GET_UPPER).value, v = u;
	if (d == ACTION::TOGGLE) u = v ? 0 : (int)round(100 * b->capacity.memorizedthreshold);
	else{
		if (!u) u = 100;
		while (d && u % d) d = abs(d) < 10 ? COPYSIGN(1, d) : d / 2;
		u = max(1, min(100, u + d));
		if (u == 100) u = 0;
	}
	int l = max(0, u - 5);
	if (u < v) ibmpmdrv(IBM::SET_LOWER, l, IBMRW::WRITE);
	if (u != v) ibmpmdrv(IBM::SET_UPPER, u, IBMRW::WRITE);
	if (u > v) ibmpmdrv(IBM::SET_LOWER, l, IBMRW::WRITE);
}
void setairplane(BOOL a){
	IBMFLAGS ibm = ibmpmdrv(IBM::GET_AIRPLANE).flags;
	if (a == ACTION::TOGGLE) a = !ibm.airplane; else a = a < 0;
	if (ibm.airplane != a){
		ibm.airplane = a;
		ibmpmdrv(IBM::SET_AIRPLANE, *(BYTE*)&ibm);
	}
}
void time(PTIME t){
	time(&t->raw);
	localtime_s(&t->info, &t->raw);
	strftime(t->text, sizeof(TIME::text), "%F %T", &t->info);
}
void battery(void){
	PBATTERY b = &batt;
	b->ufos = 0;
	primary(b);
	secondary(b);
	firmware(b);
	cell(b);
	text(b);
	threshold(b);
	duration(b);
	time(b);
	tiptext(b);
	statusled(b);
	write2registry(b);
	batteryinfo(b);
//	logger(&logged, b);
	periodlogger(b);
}
