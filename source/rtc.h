// see: https://blog.avislab.com/stm32-rtc/

#include <stdint.h>

/*
* �� ������������ ���� ����� ��������� ���:
* https://ru.m.wikipedia.org/wiki/���������_����
*/

// (UnixTime = 00:00:00 01.01.1970 = JD0 = 2440588)
#define JULIAN_DATE_BASE	2440588

typedef struct
{
	uint8_t RTC_Hours;
	uint8_t RTC_Minutes;
	uint8_t RTC_Seconds;
	uint8_t RTC_Date;
	uint8_t RTC_Wday;
	uint8_t RTC_Month;
	uint16_t RTC_Year;
} RTC_DateTimeTypeDef;

void RTC_GetDateTime(uint32_t RTC_Counter, RTC_DateTimeTypeDef* RTC_DateTimeStruct);
uint32_t RTC_GetRTC_Counter(RTC_DateTimeTypeDef* RTC_DateTimeStruct);

unsigned char RTC_Init(void);