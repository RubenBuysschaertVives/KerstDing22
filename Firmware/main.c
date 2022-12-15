// KerstDing22 - Vives - Elektronica-ICT Kortrijk.
//
// Hardware:
// APA102C LED-strip met 72 LED's.
// HC-SR04 ultrasone sensor.
// Servomotor.
// MPR121 capacitive touch module.
// Nucleo Extension Shield.
// Nucleo F091RC
// 
// Versie: 20221215
// R.Buysschaert

// Includes.
#include "stm32f091xc.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "leds.h"
#include "buttons.h"
#include "usart2.h"
#include "ad.h"
#include "spi1.h"
#include "apa102c.h"
#include "i2c1.h"
#include "mpr121.h"
#include "timer6.h"
#include "pwm.h"

// Defines.
#define TRIGGER_DISTANCE 100															// Afstand in cm vanwaar de servo bediend moet worden.
#define NUMBER_OF_SUCCESSIVE_DISTANCE_MEASUREMENTS 6			// De inferieure HC-SR04 sensor verschillende keren inlezen, zodat foute metingen gefilterd kunnen worden...
#define PWM_VARIATION_SPEED 3															// Snelheid waarmee de PWM voor de servo mag wijzigen.

// Functieprototypes.
void SystemClock_Config(void);
void InitIo(void);
void WaitForMs(uint32_t timespan);

// Variabelen aanmaken. 
static bool sw1Help = false, sw2Help = false, sw3Help = false;
static bool touch1Help = false, touch2Help = false, touch3Help = false;
static volatile bool newDistanceAvailable = false;
static char text[101];
static uint8_t counter = 0, randomIndex = 0;
static volatile uint8_t distanceIndex = 0, distanceSampleDowncounter = NUMBER_OF_SUCCESSIVE_DISTANCE_MEASUREMENTS;
static uint8_t brightness = 0;
uint8_t arrayBrightness = MAXIMUM_LED_BRIGHTNESS, arrayRed = 0, arrayGreen = 255, arrayBlue = 0;
static uint16_t touchStatus = 0, touchStatusOld = 0;
static volatile uint16_t pwm = PWM_OFFSET;
static volatile uint32_t ticks = 0;
APA102C rgbLeds[NUMBER_OF_APA102C_LEDS];
static volatile uint8_t newDistance = 0, distanceCopy = 0;


// Entry point.
int main(void)
{
	// Initialisaties.
	SystemClock_Config();
	InitButtons();
	InitLeds();
	InitUsart2(9600);
	InitAd();
	InitSpi1();
	InitI2C1();
	InitMpr121();
	InitIo();
	InitTimer6();
	InitPwm();
	
	// Laten weten dat we opgestart zijn, via de USART2 (USB).
	StringToUsart2("Reboot - KerstDing22\r\n");
	
	// Alle RGB-LED's resetten.
	for(counter = 0; counter < NUMBER_OF_APA102C_LEDS; counter++)
	{
		rgbLeds[counter].brightness = arrayBrightness;
		rgbLeds[counter].red = 0;
		rgbLeds[counter].green = 0;
		rgbLeds[counter].blue = 0;	
	}
	UpdateAPA102CLeds(rgbLeds);		// Verzend de data via SPI naar de LED's.
	
	
	// Oneindige lus starten.
	while (1)
	{	
		// Tijdens debugging, kleur laten bepalen a.d.h.v. SW1 t.e.m. SW3 (flankdetectie).
		if(SW1Active() && (sw1Help == false))
		{
			sw1Help = true;
			
			arrayRed = 255;
			arrayGreen = 0;
			arrayBlue = 0;		
		}
		if(!SW1Active())
			sw1Help = false;
		
		if(SW2Active() && (sw2Help == false))
		{
			sw2Help = true;
			
			arrayRed = 0;
			arrayGreen = 255;
			arrayBlue = 0;	
		}
		if(!SW2Active())
			sw2Help = false;
		
		if(SW3Active() && (sw3Help == false))
		{
			sw3Help = true;
			
			arrayRed = 0;
			arrayGreen = 0;
			arrayBlue = 255;			
		}
		if(!SW3Active())
			sw3Help = false;	
		
		
		// Touch statussen inlezen van de MPR121 via I²C.
		touchStatus = Mpr121GetTouchStatus();		
		if(touchStatus != touchStatusOld)
		{			
			sprintf(text, "Received touch data via I2C: %d.\r\n", touchStatus);
			StringToUsart2(text);	

			touchStatusOld = touchStatus;
		}		

		// Touch data (flankdetectie) omzetten naar een kleur met MPR121 input 0,1 en 2.
		if((touchStatus & 0x01) && (touch1Help == false))
		{
			StringToUsart2("Red.\r\n");
			
			arrayRed = 255;
			arrayGreen = 0;
			arrayBlue = 0;
			
			touch1Help = true;
		}
		if(!(touchStatus & 0x01))
			touch1Help = false;
		
		if((touchStatus & 0x02) && (touch2Help == false))
		{
			StringToUsart2("Green.\r\n");
			
			arrayRed = 0;
			arrayGreen = 255;
			arrayBlue = 0;
			
			touch2Help = true;
		}
		if(!(touchStatus & 0x02))
			touch2Help = false;
		
		if((touchStatus & 0x04) && (touch3Help == false))
		{
			StringToUsart2("Blue.\r\n");
			
			arrayRed = 0;
			arrayGreen = 0;
			arrayBlue = 255;
			
			touch3Help = true;
		}
		if(!(touchStatus & 0x04))
			touch3Help = false;
		
		
		// 5-bit helderheid opvragen via de AD-converter en de on board trimmer (Nucleo Extension Shield V2).
		brightness = GetAdValue() >> 7;
		// Beperken tot 2-bit om algemene helderheid niet te overdrijven.
		brightness >>= 3;		
		
		
		// Oude willekeurig gekozen RGB-LED resetten.
		rgbLeds[randomIndex].red = 0;
		rgbLeds[randomIndex].green = 0;
		rgbLeds[randomIndex].blue = 0;	
		
		// Nieuwe willekeurig gekozen LED kiezen om te laten flikkeren.
		randomIndex = rand() % NUMBER_OF_APA102C_LEDS;
		
		// Alle RGB-LED's resetten naar het basiskleur, met basis lichtsterkte.
		for(counter = 0; counter < NUMBER_OF_APA102C_LEDS; counter++)
		{
			rgbLeds[counter].brightness = 0b11100000 | brightness;
			rgbLeds[counter].red = arrayRed;
			rgbLeds[counter].green = arrayGreen;
			rgbLeds[counter].blue = arrayBlue;	
		}
		
		// Nieuwe willekeurig gekozen RGB-LED, instellen op maximum helderheid en correcte kleur (optioneel).
		rgbLeds[randomIndex].brightness = 0b11100000 | 31;
		rgbLeds[randomIndex].red = arrayRed;
		rgbLeds[randomIndex].green = arrayGreen;
		rgbLeds[randomIndex].blue = arrayBlue;
		
		// Verzend de LED-data via SPI naar de LED's.
		UpdateAPA102CLeds(rgbLeds);
		
		// De gemeten afstand met de (inferieure) HC-SR04 ultrasone sensor tonen.
		if(newDistanceAvailable)
		{			
			newDistanceAvailable = false;
			sprintf(text, "New distance: %d.\r\n", newDistance);
			StringToUsart2(text);			
		}		
		
		// Even wachten (anti-dender).
		WaitForMs(10);
	}
}

// Pinnen van de ultrasone sensor instellen.
void InitIo(void)
{
	//Echo staat als input na reset (PA10 = 5V-tolerant).
	GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10);
	
	// Trigger als output zetten (PA6 NIET 5V-tolerant).
	GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER6) | GPIO_MODER_MODER6_0;	
	
	// SYSCFG clock enable.
	RCC->APB2ENR |=RCC_APB2ENR_SYSCFGEN;
	
	// PA10 koppelen aan EXTI 10 (eerst doen, anders foute interruptgeneratie).
	SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI10_PA;
	
	// Rising edge detecteren
	EXTI->RTSR = EXTI->RTSR | EXTI_RTSR_TR10;
	
	// Falling edge detecteren
	EXTI->FTSR = EXTI->FTSR | EXTI_FTSR_TR10;
	
	// Interrupt toelaten
	EXTI->IMR = EXTI->IMR | EXTI_IMR_MR10;
	
	// Eén van de 4 prioriteiten kiezen.
	NVIC_SetPriority(EXTI4_15_IRQn,1);
	
	// Interrupt effectief toelaten.
	NVIC_EnableIRQ(EXTI4_15_IRQn);
}

// Afstand verhogen van de meting van de ultrasone sensor.
// Interrupt van Timer 6 opvangen
void TIM6_IRQHandler(void)
{
	if((TIM6->SR & TIM_SR_UIF) == TIM_SR_UIF)
	{
		// Interruptvlag resetten
		TIM6->SR &= ~TIM_SR_UIF;
		
		// Afstand bijwerken. Eén tick van Timer 6 is 1 cm verder.
		newDistance++;
		
		// Verder dan 1 meter, stop de meting.
		if(newDistance >= TRIGGER_DISTANCE)
			StopTimer6();
	}
}

// Trigger input van de ultrasone sensor opvangen.
void EXTI4_15_IRQHandler(void)
{
	// Als het een interrupt is van PA10...
	if((EXTI->PR & EXTI_PR_PR10) == EXTI_PR_PR10)
	{
		// Interrupt (pending) vlag wissen.
		EXTI->PR |= EXTI_PR_PR10;
		
		// Status van PA10 checken.
		if((GPIOA->IDR & GPIO_IDR_10) == GPIO_IDR_10)
		{
			// Rising edge van Echo gedetecteerd. Start de timer voor de meting.
			StartTimer6();
			newDistance = 0;
		}
		else
		{
			// Falling edge van Echo gedetecteerd. Stop de timer voor de meting.
			StopTimer6();
			distanceCopy = newDistance;
			
			// Nieuwe meting, verlaag de afteller.
			if(distanceSampleDowncounter > 0)
				distanceSampleDowncounter--;
			
			// Als de nieuwe meting er één is die meer dan 1 meter mat, zet de afteller terug op zijn maximum.
			// We willen immers voldoende correcte metingen na elkaar.
			if(distanceCopy >= TRIGGER_DISTANCE)
				distanceSampleDowncounter = NUMBER_OF_SUCCESSIVE_DISTANCE_MEASUREMENTS;
			
			// Melding doorgeven naar main().
			newDistanceAvailable = true;
		}
	}
}

// Handler die iedere 1ms afloopt. Ingesteld met SystemCoreClockUpdate() en SysTick_Config().
void SysTick_Handler(void)
{
	ticks++;
	
	// Iedere 300ms een ultrasone meting starten.
	if(ticks % 300 == 0)
		// Trigger starten voor de HC-SR04 (PA6).
		GPIOA->ODR |= GPIO_ODR_6;			// Trigger op 1 zetten.
	else
		GPIOA->ODR &= ~GPIO_ODR_6;		// Trigger op 0 zetten.
	
	// Servo aansturen met SW4 of de ultrasone sensor.
	if((SW4Active() || (distanceSampleDowncounter == 0)) && (pwm < (PWM_MAXIMUM - PWM_VARIATION_SPEED)))
	{
		pwm += PWM_VARIATION_SPEED;
		SetPwm(pwm);
	}
	else
		if(pwm > (PWM_OFFSET + PWM_VARIATION_SPEED))
		{
			pwm -= PWM_VARIATION_SPEED;
			SetPwm(pwm);	
		}
}

// Wachtfunctie via de SysTick.
void WaitForMs(uint32_t timespan)
{
	uint32_t startTime = ticks;
	
	while(ticks < startTime + timespan);
}

// Klokken instellen.
void SystemClock_Config(void)
{
	RCC->CR |= RCC_CR_HSITRIM_4;														// HSITRIM op 16 zetten, dit is standaard (ook na reset).
	RCC->CR  |= RCC_CR_HSION;																// Internal high speed oscillator enable (8MHz).
	while((RCC->CR & RCC_CR_HSIRDY) == 0);									// Wacht tot HSI zeker ingeschakeld is.
	
	RCC->CFGR &= ~RCC_CFGR_SW;															// System clock op HSI zetten (SWS is status geupdatet door hardware).
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);	// Wachten to effectief HSI in actie is getreden.
	
	RCC->CR &= ~RCC_CR_PLLON;																// Eerst PLL uitschakelen.
	while((RCC->CR & RCC_CR_PLLRDY) != 0);									// Wacht tot PLL zeker uitgeschakeld is.
	
	RCC->CFGR |= RCC_CFGR_PLLSRC_HSI_PREDIV;								// 01: HSI/PREDIV selected as PLL input clock.
	RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV2;										// prediv = /2		=> 4MHz.
	RCC->CFGR |= RCC_CFGR_PLLMUL12;													// PLL multiplied by 12 => 48MHz.
	
	FLASH->ACR |= FLASH_ACR_LATENCY;												// Meer dan 24 MHz, dus latency op 1 (p 67).
	
	RCC->CR |= RCC_CR_PLLON;																// PLL inschakelen.
	while((RCC->CR & RCC_CR_PLLRDY) == 0);									// Wacht tot PLL zeker ingeschakeld is.

	RCC->CFGR |= RCC_CFGR_SW_PLL; 													// PLLCLK selecteren als SYSCLK (48MHz).
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);	// Wait until the PLL is switched on.
		
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;												// SYSCLK niet meer delen, dus HCLK = 48MHz.
	RCC->CFGR |= RCC_CFGR_PPRE_DIV1;												// HCLK niet meer delen, dus PCLK = 48MHz.
	
	SystemCoreClockUpdate();																// Nieuwe waarde van de core frequentie opslaan in SystemCoreClock variabele.
	SysTick_Config(48000);																	// Interrupt genereren. Zie core_cm0.h, om na ieder 1ms een interrupt.
																													// te hebben op SysTick_Handler().
}
