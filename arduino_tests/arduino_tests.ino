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

/// structure of time
struct ds_time
{
    byte second;     ///< seconds (0-59)
    byte minute;     ///< minutes (0-59)
    byte hour;       ///< hours (0-23)
    byte dayOfWeek;  ///< day of week (1=Sunday - 7=Saturday)
    byte dayOfMonth; ///< day of month (1-31)
    byte month;      ///< month (1-12)
    byte year;       ///< year (1-99)
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
    Wire.begin();
    Serial.begin(9600);

    // set the initial time here:
    // DS3231 seconds, minutes, hours, day, date, month, year
    // setDS3231time((ds_time){30,42,21,4,26,11,14});
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
    Wire.write(decToBcd(_tm.month)); // set month
    Wire.write(decToBcd(_tm.year)); // set year (0 to 99)
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
    _tm.month = bcdToDec(Wire.read());
    _tm.year = bcdToDec(Wire.read());
}

/// output time to terminal
void displayTime()
{
    ds_time tm;
    // retrieve data from DS3231
    readDS3231time(tm);
    // send it to the serial monitor
    Serial.print(tm.hour, DEC);
    // convert the byte variable to a decimal number when displayed
    Serial.print(":");
    if (tm.minute < 10)
    {
        Serial.print("0");
    }
    Serial.print(tm.minute, DEC);
    Serial.print(":");
    if (tm.second < 10)
    {
        Serial.print("0");
    }
    Serial.print(tm.second, DEC);
    Serial.print(" ");
    Serial.print(tm.dayOfMonth, DEC);
    Serial.print("/");
    Serial.print(tm.month, DEC);
    Serial.print("/");
    Serial.print(tm.year, DEC);
    Serial.print(" Day of week: ");
    byte wi = tm.dayOfWeek < sizeof(dayofweek) / sizeof(dayofweek[0]) ? tm.dayOfWeek : 0;
    Serial.println(dayofweek[wi]);
}

/// main loop
void loop()
{
    displayTime(); // display the real-time clock data on the Serial Monitor,
    delay(1000); // every second
}
