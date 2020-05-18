
#include "RTE_Components.h"
#include CMSIS_device_header

#include "rtc.h"

#define TST_BLINK

//========================================================================================
bool RTC_Init(void)
{
    bool result = false;

	// Дозволити тактування модулів управління живленням і управлінням резервної областю
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	// Дозволити доступ до області резервних даних
	PWR_BackupAccessCmd(ENABLE);
	// Якщо годинник вимкнений - ініціалізувати
	if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN)
	{
		// Виконати скидання області резервних даних
		RCC_BackupResetCmd(ENABLE);
		RCC_BackupResetCmd(DISABLE);

		// Вибрати джерелом тактових імпульсів зовнішній кварц 32768 і подати тактування
		RCC_LSEConfig(RCC_LSE_ON);
		while ((RCC->BDCR & RCC_BDCR_LSERDY) != RCC_BDCR_LSERDY) {}
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

		RTC_SetPrescaler(0x7FFF); // Встановити поділювач щоб годинник рахував секунди

		// Вмикаємо годинник
		RCC_RTCCLKCmd(ENABLE);

		// Чекаємо на синхронізацію
		RTC_WaitForSynchro();
        
        // Magic delay
        for (volatile uint16_t i = UINT16_MAX; i > 0; i--);

		result = true;
	}
    
    NVIC_SetPriority(RTC_IRQn, 0);
    NVIC_EnableIRQ(RTC_IRQn);
    NVIC_SetPriority(RTCAlarm_IRQn, 0);
    NVIC_EnableIRQ(RTCAlarm_IRQn);
    
    RTC_ITConfig(RTC_IT_SEC, ENABLE);
    
	return result;
}

void rtc_set_alarm(uint32_t _time)
{
    RTC_SetAlarm(_time);
    
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
}

__WEAK void rtc_callback(uint32_t _time)
{
    (void)_time;
}

void RTC_IRQHandler(void)
{
#ifdef TST_BLINK
    // Test blinker
    static bool init = false;
    
    if (!init)
    {
        static GPIO_InitTypeDef  GPIO_InitStructure =
        {
            .GPIO_Pin =  GPIO_Pin_13,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP,
        };

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
        GPIO_Init(GPIOC, &GPIO_InitStructure);
        init = true;
    }
    
    BitAction new_val = GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) ? Bit_RESET : Bit_SET;
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, new_val);
#endif

    if (RTC_GetITStatus(RTC_IT_SEC))
    {
        RTC_ClearITPendingBit(RTC_IT_SEC);
    }

    rtc_callback(RTC_GetCounter());
}

void RTCAlarm_IRQHandler(void)
{
    if (RTC_GetITStatus(RTC_IT_ALR))
    {
        RTC_ClearITPendingBit(RTC_IT_ALR);
    }
    
    rtc_callback(RTC_GetCounter());
}
