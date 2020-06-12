/**
 * @file arduino_tests.ino
 * @author Kudesnick (kudesnick@inbox.ru)
 * @brief base test of i2c RTC clock module, such as DS1307, DS3231, DS3232, DS32C35
 * @date 2020-05-23
 *
 * @see https://tronixstuff.com/2014/12/01/tutorial-using-ds1307-and-ds3231-real-time-clock-modules-with-arduino/
 *
 */

#include <Wire.h>

/// Address of i2c device
#define DS3231_I2C_ADDRESS 0x68
#define PIN_LED 13

enum al_t
{
    AL_1,
    AL_2
};

/// structure of time
struct ds_time
{
    byte second;     ///< seconds (0-59)
    byte minute;     ///< minutes (0-59)
    byte hour;       ///< hours (0-23)
    byte dayOfWeek;  ///< day of week (1=Sunday - 7=Saturday)
    byte dayOfMonth; ///< day of month (1-31)
    byte month;      ///< month (1-12)
    word year;       ///< year (1900-2099)

    friend bool operator==(const ds_time& left, const ds_time& right)
    {
        return (memcmp(&left, &right, sizeof(ds_time)) == 0);
    };
};

/// array of name of week's days
const char *const dayofweek[] =
{
    "unknown",
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
};

/**
 * @brief Convert normal decimal numbers to binary coded decimal
 *
 * @param val - normal byte number
 * @return - binary decimal code
 */
byte decToBcd(byte val)
{
    return (val / 10 << 4) + (val % 10);
}

/**
 * @brief Convert binary coded decimal to normal decimal numbers
 *
 * @param val - binary decimal code
 * @return byte - normal byte number
 */
byte bcdToDec(byte val)
{
    return ((val / 16 * 10) + (val % 16));
}

/// Hardware settings and time set
void setup()
{
    pinMode(PIN_LED, INPUT_PULLUP);

    Wire.begin();
    Serial.begin(9600);

    // Test of year and day of week increment
    Serial.print("test 01 - ");
    ds_time tm = {59,59,23,5,31,12,1999};
    setDS3231time(tm);
    delay(1500);
    readDS3231time(tm);
    if (tm == ds_time{0, 0, 0, 6, 1, 1, 2000})
    {
        Serial.println("Ok");  
    }
    else
    {
        Serial.println("Err");
    }
    
    Serial.print("test 02 - ");
    setDS3231alarm(AL_1, ds_time{2, 0, 0, 6, 1, 1, 2000});
    bool tmp = true;
    if (!digitalRead(PIN_LED))
    {
        tmp = false;  
    }
    delay(1000);
    if (!digitalRead(PIN_LED))
    {
        tmp = false;  
    }
    delay(1000);
    if (!digitalRead(PIN_LED) && tmp)
    {
        Serial.println("Ok");  
    }
    else
    {
        Serial.println("Err");
    }

    Serial.print("test 03 - ");
    setDS3231time(ds_time{59, 0, 0, 6, 1, 1, 2000});
    setDS3231alarm(AL_2, ds_time{0, 1, 0, 6, 1, 1, 2000});
    tmp = true;
    if (!digitalRead(PIN_LED))
    {
        tmp = false;  
    }
    delay(500);
    if (!digitalRead(PIN_LED))
    {
        tmp = false;  
    }
    delay(1000);
    if (!digitalRead(PIN_LED) && tmp)
    {
        Serial.println("Ok");  
    }
    else
    {
        Serial.println("Err");
    }

    Serial.print("test 04 - ");
    setDS3231alarm(AL_2, ds_time{0, 1, 0, 6, 1, 1, 2000});
    if (digitalRead(PIN_LED))
    {
        Serial.println("Ok");  
    }
    else
    {
        Serial.println("Err");
    }
}

/**
 * @brief settings time registers
 *
 * @param _tm - structure of time
 */
void setDS3231time(const ds_time &_tm)
{
    // sets time and date data to DS3231
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set next input to start at the seconds register
    Wire.write(decToBcd(_tm.second)); // set seconds
    Wire.write(decToBcd(_tm.minute)); // set minutes
    Wire.write(decToBcd(_tm.hour)); // set hours
    Wire.write(decToBcd(_tm.dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
    Wire.write(decToBcd(_tm.dayOfMonth)); // set date (1 to 31)
    if (_tm.year >= 2000)
    {
        Wire.write(decToBcd(_tm.month) | 0x80); // set month
    }
    else
    {
        Wire.write(decToBcd(_tm.month)); // set month    
    }
    Wire.write(decToBcd(_tm.year % 100)); // set year (1900 to 2099)
    Wire.endTransmission();
}

void setDS3231alarm(const enum al_t _al_num, const ds_time &_tm)
{
    // sets time and date data to DS3231
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    if (_al_num == AL_1)
    {
        Wire.write(0x07);
        Wire.write(decToBcd(_tm.second));
    }
    else
    {
        Wire.write(0x0B);
    }
    Wire.write(decToBcd(_tm.minute)); // set minutes
    Wire.write(decToBcd(_tm.hour)); // set hours
    if (_tm.dayOfWeek == 0)
    {
        Wire.write(decToBcd(_tm.dayOfMonth));
    }
    else
    {
        Wire.write(decToBcd(_tm.dayOfWeek) | 0x40);
    }
    Wire.endTransmission();
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
        Wire.write(0x0E);
        Wire.write(0b00000100 | ((_al_num == AL_1) ? 1 : 2));
        Wire.write(0b00000000);
    Wire.endTransmission();
}

/**
 * @brief reading time registers
 *
 * @param _tm - structure of time
 */
void readDS3231time(ds_time &_tm)
{
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(0); // set DS3231 register pointer to 00h
    Wire.endTransmission();
    Wire.requestFrom(DS3231_I2C_ADDRESS, 7);

    // request seven bytes of data from DS3231 starting from register 00h
    _tm.second = bcdToDec(Wire.read() & 0x7f);
    _tm.minute = bcdToDec(Wire.read());
    _tm.hour = bcdToDec(Wire.read() & 0x3f);
    _tm.dayOfWeek = bcdToDec(Wire.read());
    _tm.dayOfMonth = bcdToDec(Wire.read());
    byte tmp = Wire.read();
    _tm.month = bcdToDec(tmp & 0x1F);
    _tm.year = bcdToDec(Wire.read()) + 1900;
    if (tmp & 0x80)
    {
        _tm.year += 100;
    }
}

void printDec(byte _val, char _flood = '\0')
{
    if (_flood != '\0' && _val < 10)
    {
        Serial.print(_flood);
    }
    Serial.print(_val, DEC);
}

/// output time to terminal
void displayTime(ds_time &_tm)
{
    // send it to the serial monitor
    printDec(_tm.hour, ' ');
    Serial.print(":");
    printDec(_tm.minute, '0');
    Serial.print(":");
    printDec(_tm.second, '0');
    Serial.print(" ");
    printDec(_tm.dayOfMonth, ' ');
    Serial.print("/");
    printDec(_tm.month, '0');
    Serial.print("/");
    Serial.print(_tm.year, DEC);
    Serial.print(" ");
    byte wi = _tm.dayOfWeek < sizeof(dayofweek) / sizeof(dayofweek[0]) ? _tm.dayOfWeek : 0;
    Serial.println(dayofweek[wi]);
}

/// main loop
void loop()
{
    ds_time tm;
    readDS3231time(tm);
    displayTime(tm); // display the real-time clock data on the Serial Monitor,
    delay(1000); // every second
}
