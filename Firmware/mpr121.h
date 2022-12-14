#include "stm32f091xc.h"

#if !defined(MPR121_DEFINED)
	#define MPR121_DEFINED
	
	#define MPR121 	0x5A		// Standaard I²C-adres voor een MPR121.
	
	#define ECR 		0x5E
	
	#define MHDR 		0x2B
	#define NHDR 		0x2C
	#define NCLR 		0x2D
	#define FDLR 		0x2E
	#define MHDF 		0x2F
	#define NHDF 		0x30
	#define NCLF 		0x31
	#define FDLF 		0x32
	
	#define E0TTH		0x41
	#define E0RTH		0x42
	
	#define AFEC1		0x5C
	#define AFEC2		0x5D
	
	#define TSL			0x00
	#define TSH			0x01
	
	#define TOUCH_THRESHOLD 0x10			//0x35		// Steeds iets hoger dan release threshold.
	#define RELEASE_THRESHOLD 0x0C		//0x30		// Steeds iets lager dan touch threshold.
	
	void InitMpr121(void);
	uint16_t Mpr121GetTouchStatus(void);
#endif
