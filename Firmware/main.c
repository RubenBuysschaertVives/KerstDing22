// Kerstboom voorbereidingen.
//
// APA102C LED-strip met 72 LED's.
// HC-SR04 ultrasone sensor.
// Servomotor.
// MPR121 capacitive touch module.
// Nucleo Extension Shield.
// 
// Versie: 20221214

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

// Functie prototypes.
void SystemClock_Config(void);
void InitIo(void);
void WaitForMs(uint32_t timespan);

// Variabelen aanmaken. 
// OPM: het keyword 'static', zorgt ervoor dat de variabele enkel binnen dit bestand gebruikt kan worden.
static bool sw1Help = false, sw2Help = false, sw3Help = false;
static bool touch1Help = false, touch2Help = false, touch3Help = false;
static char text[101];
static uint8_t counter = 0, index = 65;
static uint8_t brightness = 0, color = 0;	// 0b00000BGR
uint8_t arrayBrightness = MAXIMUM_LED_BRIGHTNESS, arrayRed = 0, arrayGreen = 255, arrayBlue = 0;
static uint16_t touchStatus = 0, touchStatusOld = 0;
static volatile uint32_t ticks = 0;
APA102C rgbLeds[NUMBER_OF_APA102C_LEDS];
static volatile uint8_t distance = 0;


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
	InitTimer6();
	InitPwm();
	InitIo();
	
	// Laten weten dat we opgestart zijn, via de USART2 (USB).
	StringToUsart2("Reboot\r\n");
	
	// RGB-LED's resetten.
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
		// Kleur bepalen a.d.h.v. SW1 t.e.m. SW3.
		if(SW1Active() && (sw1Help == false))
		{
			sw1Help = true;
			
			if(arrayRed == 0)
				arrayRed = 255;
			else
				arrayRed = 0;			
		}
		if(!SW1Active())
			sw1Help = false;
		
		if(SW2Active() && (sw2Help == false))
		{
			sw2Help = true;
			
			if(arrayGreen == 0)
				arrayGreen = 255;
			else
				arrayGreen = 0;			
		}
		if(!SW2Active())
			sw2Help = false;
		
		if(SW3Active() && (sw3Help == false))
		{
			sw3Help = true;
			
			if(arrayBlue == 0)
				arrayBlue = 255;
			else
				arrayBlue = 0;			
		}
		if(!SW3Active())
			sw3Help = false;
		
		
		// Servo testen met SW4.
		if(SW4Active())
			SetPwm(PWM_MAXIMUM);
		else
			SetPwm(PWM_OFFSET);		
		
		
		// Touch statussen inlezen.
		touchStatus = Mpr121GetTouchStatus();		
//		if(touchStatus != touchStatusOld)
//		{			
//			sprintf(text, "Received I2C-data: %d.\r\n", touchStatus);
//			StringToUsart2(text);	

//			touchStatusOld = touchStatus;
//		}
		
		
		// Gemeten afstand met HC-SR04 tonen indien touch-knop 11 is geactiveerd.
		if((touchStatus & 0x0800) == 0x0800)
		{			
			sprintf(text, "Distance: %d.\r\n", distance);
			StringToUsart2(text);			
		}
		
		
		// Touch data (flankdetectie) omzetten naar een kleur met input 0,1 en 2.
		if((touchStatus & 0x01) && (touch1Help == false))
		{
			//StringToUsart2("R\r\n");
			
			if(arrayRed == 0)
				arrayRed = 255;
			else
				arrayRed = 0;
			
			touch1Help = true;
		}
		if(!(touchStatus & 0x01))
			touch1Help = false;
		
		if((touchStatus & 0x02) && (touch2Help == false))
		{
			//StringToUsart2("G\r\n");
			
			if(arrayGreen == 0)
				arrayGreen = 255;
			else
				arrayGreen = 0;	
			
			touch2Help = true;
		}
		if(!(touchStatus & 0x02))
			touch2Help = false;
		
		if((touchStatus & 0x04) && (touch3Help == false))
		{
			//StringToUsart2("B\r\n");
			
			if(arrayBlue == 0)
				arrayBlue = 255;
			else
				arrayBlue = 0;
			
			touch3Help = true;
		}
		if(!(touchStatus & 0x04))
			touch3Help = false;
		
		
		// 5-bit helderheid opvragen.
		brightness = GetAdValue() >> 7;
		// Beperken tot 2-bit om algemene helderheid niet te overdrijven.
		brightness >>= 3;		
		
		
//		// Alle LED's overlopen.
//		index++;
//		if(index >= NUMBER_OF_APA102C_LEDS)
//			index = 0;

		// Oude RGB-LED resetten.
		rgbLeds[index].red = 0;
		rgbLeds[index].green = 0;
		rgbLeds[index].blue = 0;	
		
		// Nieuwe willekeurige LED kiezen genereren.
		index = rand() % NUMBER_OF_APA102C_LEDS;
		
		// RGB-LED's resetten.
		for(counter = 0; counter < NUMBER_OF_APA102C_LEDS; counter++)
		{
			rgbLeds[counter].brightness = 0b11100000 | brightness;
			rgbLeds[counter].red = arrayRed;
			rgbLeds[counter].green = arrayGreen;
			rgbLeds[counter].blue = arrayBlue;	
		}
		
		// TODO: brightness misschien regelen met RGB-waarden... Daarmee bekom je misschien een fijnere regeling.
		// Nieuwe RGB-LED setten.
		rgbLeds[index].brightness = 0b11100000 | 31;
		rgbLeds[index].red = arrayRed;
		rgbLeds[index].green = arrayGreen;
		rgbLeds[index].blue = arrayBlue;
		
		// Verzend de data via SPI naar de LED's.
		UpdateAPA102CLeds(rgbLeds);
		
		// Even wachten.
		WaitForMs(10);
	}
}

// Pinnen van de ultrasone sensor instellen.
void InitIo(void)
{
	//Echo staat als input na reset (PA10 = 5V-tolerant).
	GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10);
	
	// Trigger als output (PA6 NIET 5V-tolerant).
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
	
	// Eén van de 4 prioriteiten kiezen. Als er twee interrupts zijn met dezelfde
	// prioriteit, wordt eerst diegene afgehandeld die de laagste 'position' heeft (zie vector table NVIC in datasheet).
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
		
		// afstand bijwerken
		distance++;
		
		// verder dan 1,25 meter, stop de meting
		if(distance >= 125)
			StopTimer6();
	}
}

// Trigger input van de ultrasone sensor opvangen.
void EXTI4_15_IRQHandler(void)
{
	// Als het een interrupt is van PA10...
	if((EXTI->PR & EXTI_PR_PR10) == EXTI_PR_PR10)
	{
		// Interrupt (pending) vlag wissen door
		// een 1 te schrijven: EXTI->PR |= EXTI_PR_PR10;
		EXTI->PR |= EXTI_PR_PR10;
		
		if(GPIOA->IDR & GPIO_IDR_10)
		{
			// Rising edge van Echo gedetecteerd. Start de timer voor de meting.
			StartTimer6();
			distance = 0;
		}
		else
		{
			// Falling edge van Echo gedetecteerd. Stop de timer voor de meting.
			StopTimer6();
		}
	}
}


// Handler die iedere 1ms afloopt. Ingesteld met SystemCoreClockUpdate() en SysTick_Config().
void SysTick_Handler(void)
{
	ticks++;
	
	// Iedere halve seconde een ultrasone meting starten.
	if(ticks % 500 == 0)
	{
		// TODO: met timer werken...
		// Trigger starten voor de HC-SR04 (PA6).
		GPIOA->ODR |= GPIO_ODR_6;		//trigger op 1 zetten
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
		GPIOA->ODR &= ~GPIO_ODR_6;		//trigger op 0 zetten
	}
}

// Wachtfunctie via de SysTick.
void WaitForMs(uint32_t timespan)
{
	uint32_t startTime = ticks;
	
	while(ticks < startTime + timespan);
}

// Klokken instellen. Deze functie niet wijzigen, tenzij je goed weet wat je doet.
void SystemClock_Config(void)
{
	RCC->CR |= RCC_CR_HSITRIM_4;														// HSITRIM op 16 zetten, dit is standaard (ook na reset).
	RCC->CR  |= RCC_CR_HSION;																// Internal high speed oscillator enable (8MHz)
	while((RCC->CR & RCC_CR_HSIRDY) == 0);									// Wacht tot HSI zeker ingeschakeld is
	
	RCC->CFGR &= ~RCC_CFGR_SW;															// System clock op HSI zetten (SWS is status geupdatet door hardware)	
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);	// Wachten to effectief HSI in actie is getreden
	
	RCC->CR &= ~RCC_CR_PLLON;																// Eerst PLL uitschakelen
	while((RCC->CR & RCC_CR_PLLRDY) != 0);									// Wacht tot PLL zeker uitgeschakeld is
	
	RCC->CFGR |= RCC_CFGR_PLLSRC_HSI_PREDIV;								// 01: HSI/PREDIV selected as PLL input clock
	RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV2;										// prediv = /2		=> 4MHz
	RCC->CFGR |= RCC_CFGR_PLLMUL12;													// PLL multiplied by 12 => 48MHz
	
	FLASH->ACR |= FLASH_ACR_LATENCY;												//  meer dan 24 MHz, dus latency op 1 (p 67)
	
	RCC->CR |= RCC_CR_PLLON;																// PLL inschakelen
	while((RCC->CR & RCC_CR_PLLRDY) == 0);									// Wacht tot PLL zeker ingeschakeld is

	RCC->CFGR |= RCC_CFGR_SW_PLL; 													// PLLCLK selecteren als SYSCLK (48MHz)
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);	// Wait until the PLL is switched on
		
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;												// SYSCLK niet meer delen, dus HCLK = 48MHz
	RCC->CFGR |= RCC_CFGR_PPRE_DIV1;												// HCLK niet meer delen, dus PCLK = 48MHz	
	
	SystemCoreClockUpdate();																// Nieuwe waarde van de core frequentie opslaan in SystemCoreClock variabele
	SysTick_Config(48000);																	// Interrupt genereren. Zie core_cm0.h, om na ieder 1ms een interrupt 
																													// te hebben op SysTick_Handler()
}
