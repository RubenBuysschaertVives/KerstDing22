#include "stm32f091xc.h"

#if !defined(PWM_DEFINED)
	#define PWM_DEFINED
	
	#define PWM_OFFSET 1000									// Het servosignaal moet minstens 1000µs = 1ms aan zijn.
	#define PWM_MAXIMUM 1750								// 1750 niet overschrijden, anders botsing met behuizing.

	void InitPwm(void);
	void SetPwm(uint16_t pPwm);
//	void EnablePWM(void);
//	void DisablePWM(void);
#endif
