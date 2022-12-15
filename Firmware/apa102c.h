#include "stm32f091xc.h"

#if !defined(APA102C_DEFINED)
	#define APA102C_DEFINED
	
	#define MAXIMUM_LED_BRIGHTNESS 31			// Helderheid van de RGB-LED's bij opstarten.
	#define NUMBER_OF_APA102C_LEDS 72
	
	typedef struct{
		uint8_t brightness;
		uint8_t red;
		uint8_t green;
		uint8_t blue;		
	} APA102C;		
	
	void UpdateAPA102CLeds(APA102C led[]);
#endif
