
#include "RTE_Components.h"
#include CMSIS_device_header

#include "rtc.h"

//========================================================================================
bool RTC_Init(void)
{
    bool result = false;

	// ��������� ���������� ������ ��������� ��������� � ���������� �������� �������
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	// ��������� ������ �� ������ ��������� �����
	PWR_BackupAccessCmd(ENABLE);
	// ���� �������� ��������� - ������������
	if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN)
	{
		// �������� �������� ������ ��������� �����
		RCC_BackupResetCmd(ENABLE);
		RCC_BackupResetCmd(DISABLE);

		// ������� �������� �������� �������� ������� ����� 32768 � ������ ����������
		RCC_LSEConfig(RCC_LSE_ON);
		while ((RCC->BDCR & RCC_BDCR_LSERDY) != RCC_BDCR_LSERDY) {}
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

		RTC_SetPrescaler(0x7FFF); // ���������� �������� ��� �������� ������� �������

		// ������� ��������
		RCC_RTCCLKCmd(ENABLE);

		// ������ �� ������������
		RTC_WaitForSynchro();
        
        // Magic delay
        for (volatile uint16_t i = UINT16_MAX; i > 0; i--);

		result = true;
	}
    
    NVIC_SetPriority(RTC_IRQn, 0);
    NVIC_EnableIRQ(RTC_IRQn);
    
    RTC_ITConfig(RTC_IT_SEC, ENABLE);
    
	return result;
}


void RTC_IRQHandler(void)
{
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

    if (RTC_GetITStatus(RTC_IT_SEC))
    {
        RTC_ClearITPendingBit(RTC_IT_SEC);
        
        BitAction new_val = GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) ? Bit_RESET : Bit_SET;
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, new_val);
    }
}
