#include "stm32f091xc.h"
#include "i2c1.h"
#include "mpr121.h"

void InitMpr121(void)
{
	uint8_t index = 0;
	
	// Clock voor GPIOA inschakelen.
	RCC->AHBENR = RCC->AHBENR | RCC_AHBENR_GPIOAEN;
	
	// Clock voor GPIOB inschakelen.
	RCC->AHBENR = RCC->AHBENR | RCC_AHBENR_GPIOBEN;
	
//	// Interrupt request pin op input zetten (pin 10 = PA5). Op dit moment nog niet geïmplementeerd.
//	GPIOA->MODER = GPIOA->MODER & ~GPIO_MODER_MODER5;
	
	// MPR121 instellen voor het gebruik van 12 touch inputs.
	I2C1WriteRegister(MPR121, ECR, 0x00);
	
	I2C1WriteRegister(MPR121, MHDR, 0x01);
	I2C1WriteRegister(MPR121, NHDR, 0x01);
	I2C1WriteRegister(MPR121, NCLR, 0x00);
	I2C1WriteRegister(MPR121, FDLR, 0x00);
	
	I2C1WriteRegister(MPR121, MHDF, 0x01);
	I2C1WriteRegister(MPR121, NHDF, 0x01);
	I2C1WriteRegister(MPR121, NCLF, 0xFF);
	I2C1WriteRegister(MPR121, FDLF, 0x02);
	
	for(index = 0; index < 12; index++)
	{
		I2C1WriteRegister(MPR121, E0TTH + (index * 2), TOUCH_THRESHOLD);
		I2C1WriteRegister(MPR121, E0RTH + (index * 2), RELEASE_THRESHOLD);
	}
	
	I2C1WriteRegister(MPR121, AFEC1, 0x10);
	I2C1WriteRegister(MPR121, AFEC2, 0x24);
	
	I2C1WriteRegister(MPR121, ECR, 0x0C);
}

// Vraag de 12 touch statussen op. De resultaten komen terecht in de 12 LSB's.
uint16_t Mpr121GetTouchStatus(void)
{
	uint16_t touchStatus = 0;
	
	touchStatus = I2C1ReadRegister(MPR121, TSH);
	touchStatus &= 0x0F;
	touchStatus <<= 8;
	touchStatus += I2C1ReadRegister(MPR121, TSL);
	
	return touchStatus;
}
