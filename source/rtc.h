// see: https://blog.avislab.com/stm32-rtc/

#include <stdint.h>
#include <stdbool.h>

bool RTC_Init(void);
void rtc_alarm_callback(uint32_t _time);
