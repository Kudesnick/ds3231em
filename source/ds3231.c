#include <stdint.h>
#include <time.h>

#include "ds3231.h"
#include "rtc.h"
#include "RTE_Components.h"
#include CMSIS_device_header

// @todo optimize datetime conversion
// @todo 12:00 am it's 1:2 or 0:0 value?

typedef struct
{
    struct
    {
        union // 00h
        {
            uint8_t sec;
            struct 
            {
                uint8_t sec_l :4;
                uint8_t sec_h :3;
                uint8_t r0    :1;
            };

        };
        union // 01h
        {
            uint8_t min;
            struct 
            {
                uint8_t min_l :4;
                uint8_t min_h :3;
                uint8_t r1    :1;
            };
        };
        union // 02h
        {
            uint8_t hour;
            struct 
            {
                uint8_t hour_l  :4;
                uint8_t hour_h  :2;
                uint8_t hour_12 :1;
                uint8_t r3      :1;
            };
        };
    } time;
    
    struct
    {
        union // 03h
        {
            uint8_t w;
            struct
            {
                uint8_t week :3;
                uint8_t r4   :5;
            };

        };
        union // 04h
        {
            uint8_t day;
            struct 
            {
                uint8_t day_l :4;
                uint8_t day_h :2;
                uint8_t r5    :2;
            };
        };
        union // 05h
        {
            uint8_t month;
            struct 
            {
                uint8_t month_l :4;
                uint8_t month_h :1;
                uint8_t r6      :2;
                uint8_t century :1;
            };
        };
        union // 06h
        {
            uint8_t year;
            struct 
            {
                uint8_t year_l :4;
                uint8_t year_h :4;
            };
        };
    } date;

    struct
    {
        union // 07h
        {
            uint8_t sec;
            struct 
            {
                uint8_t sec_l :4;
                uint8_t sec_h :3;
                uint8_t A1M1  :1;
            };

        };
        union // 08h
        {
            uint8_t min;
            struct 
            {
                uint8_t min_l :4;
                uint8_t min_h :3;
                uint8_t A1M2  :1;
            };
        };
        union // 09h
        {
            uint8_t hour;
            struct 
            {
                uint8_t hour_l  :4;
                uint8_t hour_h  :2;
                uint8_t hour_12 :1;
                uint8_t A1M3    :1;
            };
        };
        union // 0Ah
        {
            uint8_t date;
            struct
            {
                uint8_t week :3;
                uint8_t r7   :5;
            };
            struct
            {
                uint8_t day_l :4;
                uint8_t day_h :2;
                uint8_t r8    :1;
                uint8_t A1M4  :1;
            };
        };
    } alarm1;

    struct
    {
        union // 0Bh
        {
            uint8_t min;
            struct 
            {
                uint8_t min_l :4;
                uint8_t min_h :3;
                uint8_t A2M2  :1;
            };
        };
        union // 0Ch
        {
            uint8_t hour;
            struct 
            {
                uint8_t hour_l  :4;
                uint8_t hour_h  :2;
                uint8_t hour_12 :1;
                uint8_t A2M3    :1;
            };
        };
        union // 0Dh
        {
            uint8_t date;
            struct
            {
                uint8_t week :3;
                uint8_t r9   :5;
            };
            struct
            {
                uint8_t day_l :4;
                uint8_t day_h :2;
                uint8_t r10   :1;
                uint8_t A2M4  :1;
            };
        };
    } alarm2;

    union // 0Eh
    {
        uint8_t ctl;
        struct
        {
            uint8_t A1IE  :1;
            uint8_t A2IE  :1;
            uint8_t INTCN :1;
            uint8_t RS1   :1;
            uint8_t RS2   :1;
            uint8_t CONV  :1;
            uint8_t BBSQW :1;
            uint8_t EOSC  :1;
        };
        
    };
    union // 0Fh
    {
        uint8_t ctl_sts;
        struct
        {
            uint8_t A1F     :1;
            uint8_t A2F     :1;
            uint8_t BSY     :1;
            uint8_t EN32kHz :1;
            uint8_t r11     :3;
            uint8_t OSF     :1;
        };
    };

    int8_t aging_offs; // 10h
    int8_t temp_h;
    uint8_t temp_l;
} state_t;

typedef union
{
    state_t state;
    uint8_t raw[sizeof(state_t)];
} ds3231_t;

ds3231_t state;

uint8_t wr_buf[sizeof(state_t)];

//**************************************************************************************************

typedef enum
{
    STEP_WAIT,
    STEP_RECEIVE,
    STEP_TRANSMIT,
} step_t;

step_t curr_step = STEP_WAIT;
uint8_t addr;
uint8_t ptr;

#define TIME_OFFSET ((time_t)946684799UL) // 31 dec 1999 23:59:59 

void ds_init()
{
    RTC_Init();
}

void ds_set_addr(uint8_t _addr)
{
    if (curr_step != STEP_WAIT)
    {
        ds_end_transaction();
    }
    addr = _addr;
}

void ds_write(uint8_t _byte)
{
    if (curr_step == STEP_TRANSMIT)
    {
        ds_end_transaction();
    }
    curr_step = STEP_RECEIVE;

    if (ptr < sizeof(state))
    {
        state.raw[addr + ptr++] = _byte;
    }
}

uint8_t ds_read(void)
{
    static bool time_upd = false;

    if (curr_step != STEP_TRANSMIT)
    {
        time_upd = false;
    }
    if (curr_step == STEP_RECEIVE)
    {
        ds_end_transaction();
    }
    curr_step = STEP_TRANSMIT;
    
    if (!time_upd && ((addr + ptr) < 0x07))
    {
        time_t time = RTC_GetCounter();
        struct tm dt = *localtime(&time);
        
        state.state.time.sec_l = dt.tm_sec % 10;
        state.state.time.sec_h = dt.tm_sec / 10;
        state.state.time.min_l = dt.tm_min % 10;
        state.state.time.min_h = dt.tm_min / 10;
        if (state.state.time.hour_12)
        {
            if (dt.tm_hour == 00)
            {
                dt.tm_hour = 24;
            }
            if (dt.tm_hour > 12)
            {
                dt.tm_hour = dt.tm_hour - 12 + 20;
            }
        }
        state.state.time.hour_l = dt.tm_hour % 10;
        state.state.time.hour_h = dt.tm_hour / 10;
            
        state.state.date.week = dt.tm_wday + 1;
        state.state.date.day_l = dt.tm_mday % 10;
        state.state.date.day_h = dt.tm_mday / 10;
        dt.tm_mon += 1;
        state.state.date.month_l = dt.tm_mon % 10;
        state.state.date.month_h = dt.tm_mon / 10;
        if (dt.tm_year > 100)
        {
            dt.tm_year -= 100;
        }
        state.state.date.year_l = dt.tm_year % 10;
        state.state.date.year_h = dt.tm_year / 10;
    
        time_upd = true;
    }
    else if ((addr + ptr) == 0x0e)
    {
        state.state.EOSC = 0;
        state.state.CONV = 0;
    }
    else if ((addr + ptr) == 0x0f)
    {
        state.state.OSF = 0;
        state.state.BSY = 0;
    }
    
    return state.raw[addr + ptr++];
}

void ds_end_transaction(void)
{
    if (curr_step == STEP_RECEIVE)
    {
        if ((addr < 0x07) && (ptr > 0))
        {
            struct tm dt;
            
            dt.tm_sec = state.state.time.sec_h * 10 + state.state.time.sec_l;
            dt.tm_min = state.state.time.min_h * 10 + state.state.time.min_l;
            dt.tm_hour = state.state.time.hour_h * 10 + state.state.time.hour_l;
            if (state.state.time.hour_12 && dt.tm_hour >= 20)
            {
                dt.tm_hour = dt.tm_hour - 20 + 12;
                if (dt.tm_hour >= 24)
                {
                    dt.tm_hour = 0;
                }
            }
            dt.tm_mday = state.state.date.day_h * 10 + state.state.date.day_l;
            dt.tm_mon = state.state.date.month_h * 10 + state.state.date.month_l - 1;
            dt.tm_year = state.state.date.year_h * 10 + state.state.date.year_l + 100;
            
            RTC_SetCounter(mktime(&dt));
        }
    }

    curr_step = STEP_WAIT;
    ptr = 0;
}
