#include <stdint.h>
#include <time.h>
#include <string.h>

#include "ds3231.h"
#include "rtc.h"
#include "RTE_Components.h"
#include CMSIS_device_header

#define DS3231
// #define DS1307
// #define DS3232
// #define DS32C35

/// @note this unit work with years from 2000 to 2100  

typedef struct 
{
    uint8_t low :4;
    uint8_t hi  :3;
    uint8_t r   :1;
} sm_t;

static uint8_t _sm_bcd_to_int(const sm_t _val)
{
    return (_val.hi * 10 + _val.low);
}

static void _sm_int_to_bcd(const uint8_t _val, sm_t *const _sm)
{
    _sm->low = _val % 10;
    _sm->hi  = _val / 10;
}

typedef struct 
{
    uint8_t low   :4;
    uint8_t hi    :2;
    uint8_t fl_12 :1;
    uint8_t r     :1;
} hr_t;

static uint8_t _hr_bcd_to_int(const hr_t _val)
{
    uint8_t result = (_val.hi * 10 + _val.low);
    if (_val.fl_12 && result >= 20)
    {
        result -= 20 + 12;
        if (result >= 24)
        {
            result = 0;
        }
    }
    
    return result;
}

static hr_t _hr_int_to_bcd(uint8_t _val, hr_t *const _hr)
{
    if (_hr->fl_12)
    {
        if (_val == 00)
        {
            _val = 24;
        }
        if (_val > 12)
        {
            _val -= 12 + 20;
        }
    }
    _hr->low = _val % 10;
    _hr->hi  = _val / 10;
    
    return *_hr;
}

typedef struct
{
    uint8_t wday :3;
    uint8_t r    :5;
} wday_t;

static uint8_t _wday_bcd_to_int(const wday_t _val)
{
    return _val.wday - 1;
}

static void _wday_int_to_bcd(const uint8_t _val, wday_t *const _day)
{
    _day->wday = _val + 1;
}

typedef struct
{
    uint8_t low :4;
    uint8_t hi  :2;
    uint8_t r   :2;
} mday_t;

static uint8_t _mday_bcd_to_int(const mday_t _val)
{
    return (_val.hi * 10 + _val.low);
}

static void _mday_int_to_bcd(const uint8_t _val, mday_t *const _mday)
{
    _mday->low = (_val) % 10;
    _mday->hi  = (_val) / 10;
}

typedef struct
{
    uint8_t low :4;
    uint8_t hi  :1;
    uint8_t r   :3;
} mon_t;

static uint8_t _mon_bcd_to_int(const mon_t _val)
{
    return (_val.hi * 10 + _val.low - 1);
}

static void _mon_int_to_bcd(const uint8_t _val, mon_t *const _mon)
{
    _mon->low = (_val + 1) % 10;
    _mon->hi  = (_val + 1) / 10;
}

typedef struct
{
    uint8_t low :4;
    uint8_t hi  :4;
} year_t;

static uint8_t _year_bcd_to_int(const year_t _val)
{
    return (_val.hi * 10 + _val.low + 100);
}

static void _year_int_to_bcd(uint8_t _val, year_t *const _year)
{
    _val %= 100;
    _year->low = (_val) % 10;
    _year->hi  = (_val) / 10;
}

struct
{
    union
    {
        struct
        {
            sm_t sec;    // 00h
            sm_t min;    // 01h
            hr_t hour;   // 02h
            wday_t wday; // 03h
            mday_t mday; // 04h
            mon_t mon;   // 05h
            year_t year; // 06h
        };
        struct
        {
            uint8_t r0[5];
            uint8_t r1 :7;
            uint8_t century :1;
            uint8_t r2;
        };
    } time;
#ifdef DS1307
    union // 07h
    {
        uint8_t ctl;
        struct
        {
            uint8_t RS1   :1;
            uint8_t RS2   :1;
            uint8_t r3    :2;
            uint8_t BBSQW :1;
            uint8_t r4    :2;
            uint8_t OUT   :1;
        };
    };
    uint8_t ram[56];
#else
    union
    {
        struct
        {
            sm_t sec;
            sm_t min;
            hr_t hour;
            union
            {
                wday_t wday;
                mday_t mday;
            };
        };
        struct
        {
            uint8_t r10   :7;
            uint8_t A1M1  :1;
            uint8_t r11   :7;
            uint8_t A1M2  :1;
            uint8_t r12   :7;
            uint8_t A1M3  :1;
            uint8_t r13   :6;
            uint8_t dy_dt :1;
            uint8_t A1M4  :1;
        };
    } alarm1;

    union
    {
        struct
        {
            sm_t min;
            hr_t hour;
            union
            {
                wday_t wday;
                mday_t mday;
            };
        };
        struct
        {
            uint8_t r11   :7;
            uint8_t A2M2  :1;
            uint8_t r12   :7;
            uint8_t A2M3  :1;
            uint8_t r13   :6;
            uint8_t dy_dt :1;
            uint8_t A2M4  :1;
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
#ifdef DS3232
            uint8_t CRATE0  :1;
            uint8_t CRATE1  :1;
            uint8_t BB32kHz :1;
#else
            uint8_t r11     :3;
#endif
            uint8_t OSF     :1;
        };
    };

    int8_t aging_offs; // 10h
    __PACKED int16_t temp;  // 11h - 12h
#ifdef DS3232
    uint8_t r11; // 13h
    uint8_t ram[236];
#endif

#endif
} state;

//**************************************************************************************************

uint8_t* addr = NULL;

uint32_t curr_time;
bool must_upd_time = true;
bool must_upd_al_1 = true;
bool must_upd_al_2 = true;

void ds_init()
{
    RTC_Init();
}

void ds_set_addr(uint8_t _addr)
{
    addr = &((uint8_t *)&state)[_addr];
}

void ds_write(uint8_t _byte)
{
    if (addr >= (uint8_t *)&state.time && addr < ((uint8_t *)&state.time + sizeof(state.time)))
    {
        must_upd_time = true;
    }
    else if (addr >= (uint8_t *)&state.alarm1 && addr < ((uint8_t *)&state.alarm1 + sizeof(state.alarm1)))
    {
        must_upd_al_1 = true;
    }
    else if (addr >= (uint8_t *)&state.alarm2 && addr < ((uint8_t *)&state.alarm2 + sizeof(state.alarm2)))
    {
        must_upd_al_2 = true;
    }
    if (addr >= (uint8_t *)&state && addr < ((uint8_t *)&state + sizeof(state)))
    {
        *addr = _byte;
    }
    
    addr++;
}

void ds_upd_time_registers(void)
{
    static uint32_t last_time = 0;
    
    if (last_time != curr_time)
    {
        struct tm dt = *localtime(&curr_time);
        
        _sm_int_to_bcd(dt.tm_sec, &state.time.sec);
        _sm_int_to_bcd(dt.tm_min, &state.time.min);
        _hr_int_to_bcd(dt.tm_hour, &state.time.hour);
        _wday_int_to_bcd(dt.tm_wday, &state.time.wday);
        _mday_int_to_bcd(dt.tm_mday, &state.time.mday);
        _mon_int_to_bcd(dt.tm_mon, &state.time.mon);
#ifndef DS1307
        uint8_t prev_year = _year_bcd_to_int(state.time.year);
        if (prev_year > dt.tm_year)
        {
            state.time.century ^= 1;
        }
#endif
        _year_int_to_bcd(dt.tm_year, &state.time.year);

        last_time = curr_time;
    }
}

uint8_t ds_read(void)
{
    if (addr < (uint8_t *)&state || addr >= ((uint8_t *)&state + sizeof(state)))
    {
        return 0;
    }

    if (addr >= (uint8_t *)&state.time && addr < ((uint8_t *)&state.time + sizeof(state.time)))
    {
        ds_upd_time_registers();
    }
    
    else if (addr == &state.ctl)
    {
#ifndef DS1307
        state.EOSC = 0;
        state.CONV = 0;
#endif
    }
#ifndef DS1307
    else if (addr == &state.ctl_sts)
    {
        state.OSF = 0;
        state.BSY = 0;
    }
#endif
    
    return *addr++;
}

// alarm interrupt
void ds_alarm_irq()
{
    if (state.INTCN)
    {
        if ((state.A1IE && state.A1F) || (state.A2IE && state.A2F))
        {
            // @todo interrupt of alarm
        }
    }
}

static uint32_t _alarm_calc(const typeof(state.alarm2) *const _alarm, const uint8_t _sec)
{
    uint32_t result = _sec;
    
    struct tm dt;
    memset(&dt, 0, sizeof(dt));
    
    // Minute
    if (_alarm->A2M2)
    {
        dt.tm_min = _sm_bcd_to_int(state.time.min);
    }
    else
    {
        dt.tm_min = _sm_bcd_to_int(_alarm->min);
    }
    // Hour
    if (_alarm->A2M3)
    {
        dt.tm_hour = _hr_bcd_to_int(state.time.hour);
    }
    else
    {
        dt.tm_hour = _hr_bcd_to_int(_alarm->hour);
    }
    // Day
    if (_alarm->A2M4 || _alarm->dy_dt)
    {
        dt.tm_mday = _mday_bcd_to_int(state.time.mday);
    }
    else
    {
        dt.tm_mday = _mday_bcd_to_int(_alarm->mday);
    }
    // Month and year
    dt.tm_mon = _mon_bcd_to_int(state.time.mon);
    dt.tm_year = _year_bcd_to_int(state.time.year);
    
    result = mktime(&dt);
    // Correct if per week mode
    if (!_alarm->A2M4 && _alarm->dy_dt)
    {
        // Converted range 1-7 to range 0-6 and calculate distance in positive value
        uint8_t inc = (_alarm->wday.wday - 1 + 7 - state.time.wday.wday - 1) % 7;
        result += (24 * 60 * 60) * inc;
        if (result <= curr_time)
        {
            result += (24 * 60 * 60) * 7;
        }
    }
    
    return result;
}

// weakly callback
void rtc_callback(uint32_t _time)
{
    static uint32_t al_1_time, al_2_time;

    if (must_upd_time)
    {
        struct tm dt;
        
        dt.tm_sec = _sm_bcd_to_int(state.time.sec);
        dt.tm_min = _sm_bcd_to_int(state.time.min);
        dt.tm_hour = _hr_bcd_to_int(state.time.hour);
        dt.tm_mday = _mday_bcd_to_int(state.time.mday);
        dt.tm_mon = _mon_bcd_to_int(state.time.mon);
        dt.tm_year = _year_bcd_to_int(state.time.year);
        
        curr_time = mktime(&dt);
        RTC_SetCounter(curr_time);
        
        must_upd_time = false;
    }
    else
    {
        curr_time = _time;
    }
    
    if (al_1_time == curr_time && !must_upd_al_1)
    {
        state.A1F = 1;
        ds_alarm_irq();
        
        must_upd_al_1 = true;
    }
    if (al_2_time == curr_time && !must_upd_al_2)
    {
        state.A2F = 1;
        ds_alarm_irq();
        
        must_upd_al_1 = true;
    }
    
    if (must_upd_al_1)
    {
        ds_upd_time_registers();
        
        uint8_t sec;
        
        // Seconds
        if (state.alarm1.A1M1)
        {
            sec = (_sm_bcd_to_int(state.time.sec) + 1) % 60;
        }
        else
        {
            sec = _sm_bcd_to_int(state.alarm1.sec);
        }

        al_1_time = _alarm_calc((typeof(state.alarm2) *)&state.alarm1.min, sec);
    }
    
    if (must_upd_al_2)
    {
        ds_upd_time_registers();
        
        al_2_time = _alarm_calc(&state.alarm2, 0);
    }
    
    if (must_upd_al_1 || must_upd_al_2)
    {
        RTC_SetAlarm(al_1_time < al_2_time ? al_1_time : al_2_time);
    
        must_upd_al_1 = must_upd_al_2 = false;
    }
}
