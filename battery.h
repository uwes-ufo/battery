
#pragma once

#include <windows.h>
#include <corecrt.h>
#include <ctime>
#include <debugapi.h>
#include <sstream>
#include "resource.h"

#define countof(a) (sizeof(a)/sizeof(a[0]))
#define COPYSIGN(a,b) (((b)<0)?-(a):((b)>0)?(a):0)

#define LOG "c:\\users\\uwe\\.uwe\\log\\"
#define UFOLOG(msg) { TIME t; time(&t); std::ofstream file(LOG"ufo.log", std::ios_base::app); file << t.text << " : " << msg << "\n"; }
#define DEBUG(msg) { std::ostringstream os; os << msg; OutputDebugString(os.str().c_str()); }

enum _WM_USER { WM_ICON = WM_USER + IDI_ICON1 };
constexpr double CAPACITY_LEVEL_WHITE = 89.5e-2;

struct ACTION { 
	enum _ACTION{ 
		KEEP = 0xcafe,
		TOGGLE = 0xaffe
	};
};

constexpr struct _TEMPERATURE{
	double BLUE = 25; 
	double THROTTLE = 41.8; 
	double RED = 45; 
}TEMP;

typedef enum class _COLOR{
	DARKBLUE = 0x0000ff,
	BLUE = 0x0080ff,
	ORANGE = 0xff8000,
	RED = 0xff4040,
	PURPLE = 0x8000ff,
	GRAY = 0xcccc9a,
	WHITE = 0xffffc0,
	FRAME = 0xffffff
}COLOR, * PCOLOR;

typedef struct _LED{
	COLOR led;
	COLOR thermo;
	COLOR frame;
}LED;

typedef enum class _TIMER{
	FAST = 100,
	SLOW = 2000,
	DELAY = 4000
}TIMER;

typedef enum class _UFOS{
	T1FLAGS,
	T1,
	FLAGS,
	XE024,
	PFLAGS,
	P0,
	D49,
	D11137,
	X80,
	S0,
	FFLAGS,
	F0,
	X6C05,
	X6C01,
	C0,
	XX0,
	IBMFLAGS
}UFOS;

typedef enum class _IDM{
	ZERO,
	QUIT,
	SEPARATOR,
	TEMPERATURE,
	LEVEL,
	WATT
}IDM;

typedef enum class _STATE{
	INIT,
	INACTIVE,
	CHARGE,
	DISCHARGE
}STATE, * PSTATE;

typedef struct _VOLTAGE{
	double cell[3];
	double series;
	double single;
	double idle;
	double design;
	double endofcharge;
}VOLTAGE, * PVOLTAGE;

typedef struct _CURRENT{
	double value;
	double lowpass;
	double limit;
}CURRENT, * PCURRENT;

typedef struct _CAPACITY{
	double minimal;
	double design;
	double fullCharge;
	double remaining;
	double level;
	double v2level;
	double total;
	double threshold;
	double memorizedthreshold;
}CAPACITY, * PCAPACITY;

typedef struct _DURATION{
	double minutes;
	char tip[32];
	double discharge;
	double driver;
}DURATION, * PDURATION;

typedef struct _TIME{
	time_t raw;
	struct tm info;
	char text[32];
}TIME, * PTIME;

typedef struct _BATTERYFLAGS{
	BOOL off;
	BOOL full;
	BOOL enable;
	BOOL zero20;
	BOOL below12;
	BOOL dischargebelow5; // 5%
	BOOL empty08;
	BOOL airplane;
}BATTERYFLAGS;

typedef struct _BATTERY{
	TIME time;
	double temperature[2];
	VOLTAGE voltage;
	CURRENT current;
	double power;
	double acAdapter;
	STATE state;
	CAPACITY capacity;
	DURATION duration;
	int cycles;
	int operatinghours;
	CHAR manufacturer[32];	
	CHAR fru[32];			
	CHAR chemistry[32];		
	CHAR barcode[32];		
	CHAR firmwareversion[32];
	CHAR manufacturedate[32];
	CHAR firstuseddate[32];
	BATTERYFLAGS flags;
	int serialnumber;
	int subcycle;
	int ufovoltage;
	int ufo121;
	int ufo61;
	int ufos;
}BATTERY, * PBATTERY;

typedef struct _BITMAPINFO3{
	BITMAPINFO bi;
	RGBQUAD bmiColors[2];
}BITMAPINFO3;

extern BATTERY batt;

void batteryinfo(PBATTERY b);
void periodlogger(PBATTERY b);

void write2registry(PBATTERY battery);

void time(PTIME t);
void battery(void);
void statusled(PBATTERY b);
void tiptext(char* x);
void menutext(IDM item, LPSTR text);
void setairplane(BOOL a);
void setthreshold(int delta);
BOOL getverbosity(void);
void setverbosity(BOOL v);
