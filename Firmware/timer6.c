#include "stm32f091xc.h"

// Timer 6 instellen
// 1cm (moet dubbel afgelegd worden) = 0.02m/340m/s = ~59 µs
// Gemeten met een klok van 48MHz is dit 59µs/20,8ns = ~2832 stappen
void InitTimer6(void)
{
	// EERST klok voorzien!!
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;	// clock voorzien voor de timer
	
	TIM6->PSC = 0;												// prescaler op nul, dus de timer telt op ritme van 48MHz
	TIM6->ARR = 2832;											// na 2832 stappen is er 1 cm afstand gemeten (heen en terug)
	
	TIM6->DIER |= TIM_DIER_UIE;					// Interrupt enable voor timer 6	
//	TIM6->CR1 |= TIM_CR1_CEN;						// counter enable
	
	//TIM6->CR1 |= TIM_CR1_OPM;			// one pulse mode kiezen, dan stopt de timer na één overflow.
	
	NVIC_SetPriority(TIM6_IRQn, 0);
	NVIC_EnableIRQ(TIM6_IRQn);
}

void StartTimer6(void)
{
	// Volgorde van (groot) belang.		
	TIM6->CNT = 0;											// Timer resetten	
//	TIM6->EGR |= TIM_EGR_UG;						// registers updaten		(in one pulse mode zorgt dit voor het uitzetten van CEN).
	TIM6->CR1 |= TIM_CR1_CEN;						// counter enable	
//	TIM6->SR &= ~TIM_SR_UIF;						// Interruptvlag resetten	
//	TIM6->DIER |= TIM_DIER_UIE;					// Interrupt enable voor timer 6.
}

void StopTimer6(void)
{	
//	TIM6->DIER &= ~TIM_DIER_UIE;				// Interrupt disable voor timer 6.
//	TIM6->SR &= ~TIM_SR_UIF;						// Interruptvlag resetten
	TIM6->CR1 &= ~TIM_CR1_CEN;					// counter disable
}
