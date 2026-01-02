
#pragma once

typedef enum class _IBM{
	GET_SMARTBATT = 0x92a,
	GET_SMARTBATTB,
	SET_USBPOWER = 0x96d,
	GET_FIRMWAREINFO = 0x970,
	GET_STATE_9	= 0x989,
	GET_LOWER = 0x98b,
	SET_LOWER,
	GET_UPPER,
	SET_UPPER,
	GET_DISCHARGESTATUS,
	GET_AIRPLANE = 0x994,
	SET_AIRPLANE,
	GET_SMARTBATT6 = 0xa06,
	GET_FLIPTOBOOT = 0xa09
}IBM;

typedef enum class _IBMRW{
	READ = 0,
	WRITE = 1
}IBMRW;

typedef struct _IBMFLAGS{
	BYTE l5 : 4;
	BYTE airplane : 1;
	BYTE h1 : 3;
}IBMFLAGS;

typedef union _IBMDW{
	DWORD raw;
	struct{
		union{
			BYTE value;
			IBMFLAGS flags;
		};
		BYTE flag;
		WORD w;
	};
}IBMDW;

typedef enum class _TPI{
	T1 = 1,
	PRIMARY,
	SECONDARY,
	MANUFACTURER,
	FRU,
	CHEMISTRY,
	BARCODE,
	FIRMWARE,
	CELL
}TPI;

typedef struct _TPPWRIFIN{
	union{
		DWORD raw;
		struct{
			BYTE flag;
			BYTE index;
			WORD w;
		};
	};
	DWORD zero[6];
}TPPWRIFIN;

typedef union _FLAGS{
	WORD raw[2];
	struct{
		WORD l0 : 6;
		WORD l1 : 1;
		WORD acattached : 1;
		WORD h0 : 3;
		WORD empty08 : 1;
		WORD discharge : 1;
		WORD charge : 1;
		WORD h3 : 2;
		WORD x80;
	};
}FLAGS;

typedef union _PRIMARYFLAGS{
	WORD raw;
	struct{
		WORD l0 : 4;
		WORD zero20 : 1;
		WORD full : 1;
		WORD off : 1;
		WORD l1 : 1;
		WORD dischargebelow5 : 1;
		WORD below12 : 1;
		WORD h0 : 6;
	};
}PRIMARYFLAGS;

typedef union _FIRMWAREFLAGS{
	WORD raw;
	struct{
		WORD l0 : 3;
		WORD enable : 1;
		WORD l1 : 1;
		WORD h0 : 11;
	};
}FIRMWAREFLAGS;

typedef union _TPDATE{
	WORD raw;
	struct{
		WORD day : 5;
		WORD month : 4;
		WORD year : 7;
	};
}TPDATE;

typedef struct _PRIMARY{
	WORD xe024;
	WORD temperature;
	WORD voltage;
	short current;
	short lowpasscurrent;
	WORD chargelevel;
	WORD remainingcapacity;
	WORD fullcapacity;
	short dischargeminutes[2];
	short chargeminutes;
	PRIMARYFLAGS flags;
	WORD cycles;
	WORD x0;
}PRIMARY;

typedef struct _SECONDARY{
	WORD temperature;
	WORD designcapacity;
	WORD designvoltage;
	WORD ufo49;
	TPDATE manufacturedate;
	WORD serialnumber;
	WORD minimalcapacity;
	TPDATE firstuseddate;
	WORD ufovoltage;
	WORD x80;
	WORD acwattage;
	WORD acattached;
	WORD x0[2];
}SECONDARY;

typedef struct _FIRMWARE{
	WORD firmwareversion[4];
	FIRMWAREFLAGS flags;
	WORD subcycle;
	WORD currentlimit;
	WORD endofchargevoltage;
	WORD x0[6];
}FIRMWARE;

typedef struct _CELL{
	WORD x6c05;
	WORD x6c01;
	WORD x0;
	WORD cellvoltage[3];
	WORD totalcharge;
	WORD operatinghours;
	WORD ufovoltage;
	WORD ufo121;
	WORD ufo61;
	WORD xx0[3];
}CELL;

typedef union _TPPWRIF{
	short raw[16];
	struct{
		FLAGS flags;
		union{
			PRIMARY p;
			SECONDARY s;
			char text[28];
			FIRMWARE f;
			CELL c;
		};
	};
}TPPWRIF, * PTPPWRIF;

BOOL tppwrif(PTPPWRIF out, TPI index, BYTE flag = 0);
IBMDW ibmpmdrv(IBM id, BYTE value = 1, IBMRW flag = IBMRW::READ);
