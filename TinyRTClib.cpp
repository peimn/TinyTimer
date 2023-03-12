#include "WString.h"
// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
// Adafruit modifications for Aradruit DS3231 breakout
//  board Product 268, Arduino Due and Trinket/Gemma

#include <avr/pgmspace.h>
#include <TinyWireM.h>
#define WIRE TinyWireM
#include "TinyRTClib.h"

#define DS3231_ADDRESS 0x68
#define DS3231_CONTROL  0x0E
#define DS3231_STATUSREG 0x0F

// Control Register bits
#define DS3231_EN32kHz (3)

#define SECONDS_PER_DAY 86400L

#define SECONDS_FROM_1970_TO_2000 946684800

#include <Arduino.h>


static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  WIRE.beginTransmission(addr);
  WIRE.write(reg);
  WIRE.endTransmission();

  WIRE.requestFrom(addr, (byte)1);
  return WIRE.read();
}

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  WIRE.beginTransmission(addr);
  WIRE.write(reg);
  WIRE.write(val);
  WIRE.endTransmission();
}



////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (uint32_t t) {
  t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

    ss = t % 60;
    t /= 60;
    mm = t % 60;
    t /= 60;
    hh = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (yOff = 0; ; ++yOff) {
        leap = yOff % 4 == 0;
        if (days < 365U + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1;
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (year >= 2000)
        year -= 2000;
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint
DateTime::DateTime (const char* date, const char* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    yOff = conv2d(date + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (date[0]) {
        case 'J':
            if ( date[1] == 'a' ) {
                m = 1;
            } else if ( date[2] == 'n' ) {
                m = 6;
            } else {
                m = 7;
            }
            break;
        case 'F': m = 2; break;
        case 'A': m = date[2] == 'r' ? 4 : 8; break;
        case 'M': m = date[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(date + 4);
    hh = conv2d(time);
    mm = conv2d(time + 3);
    ss = conv2d(time + 6);
}

uint8_t DateTime::dayOfWeek() const {
    uint16_t day = date2days(yOff, m, d);
    return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const {
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

long DateTime::secondstime(void) const {
  long t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  return t;
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS3231 implementation

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

uint8_t RTC_DS3231::begin(void) {
    TinyWireM.begin();
    uint8_t reg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
    reg &= ~_BV(DS3231_EN32kHz); //disable 32kHz to save power.
    write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, reg);
    return 1;
}


bool RTC_DS3231::lostPower(void) {
    return (read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG) >> 7);
}

void RTC_DS3231::adjust(const DateTime& dt) {
    WIRE.beginTransmission(DS3231_ADDRESS);
    WIRE.write(0);
    WIRE.write(bin2bcd(dt.second()));
    WIRE.write(bin2bcd(dt.minute()));
    WIRE.write(bin2bcd(dt.hour()));
    WIRE.write(bin2bcd(0));
    WIRE.write(bin2bcd(dt.day()));
    WIRE.write(bin2bcd(dt.month()));
    WIRE.write(bin2bcd(dt.year() - 2000));
    WIRE.write(0);
    WIRE.endTransmission();

    uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
    statreg &= ~0x80; // flip OSF bit
    write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, statreg);

}

DateTime RTC_DS3231::now() {
  WIRE.beginTransmission(DS3231_ADDRESS);
  WIRE.write(0);
  WIRE.endTransmission();

  WIRE.requestFrom(DS3231_ADDRESS, 7);
  uint8_t ss = bcd2bin(WIRE.read() & 0x7F);
  uint8_t mm = bcd2bin(WIRE.read());
  uint8_t hh = bcd2bin(WIRE.read());
  WIRE.read();
  uint8_t d = bcd2bin(WIRE.read());
  uint8_t m = bcd2bin(WIRE.read());
  uint16_t y = bcd2bin(WIRE.read()) + 2000;

  return DateTime (y, m, d, hh, mm, ss);
}

// https://how2electronics.com/arduino-ds3231-real-time-clock-alarm-temperature/

String tempAndAlarm(byte mode) {
  // Variables declaration
  bool alarm1_status, alarm2_status;
  String alarm1 = "A1=   :  ", alarm2 = "A2=   :  ", temperature = "TIn:   .   C";
  byte alarm1_minute, alarm1_hour, alarm2_minute, alarm2_hour, status_reg;
  byte control_reg, temperature_lsb;
  char temperature_msb;
  WIRE.beginTransmission(0x68);                 // Start I2C protocol with DS3231 address
  WIRE.write(0x08);                             // Send register address
  WIRE.endTransmission(false);                  // I2C restart
  WIRE.requestFrom(0x68, 11);                   // Request 11 bytes from DS3231 and release I2C bus at end of reading
  alarm1_minute = WIRE.read();                  // Read alarm1 minutes
  alarm1_hour   = WIRE.read();                  // Read alarm1 hours
  WIRE.read();                                  // Skip alarm1 day/date register
  alarm2_minute = WIRE.read();                  // Read alarm2 minutes
  alarm2_hour   = WIRE.read();                  // Read alarm2 hours
  WIRE.read();                                  // Skip alarm2 day/date register
  control_reg = WIRE.read();                    // Read the DS3231 control register
  status_reg  = WIRE.read();                    // Read the DS3231 status register
  WIRE.read();                                  // Skip aging offset register
  temperature_msb = WIRE.read();                // Read temperature MSB
  temperature_lsb = WIRE.read();                // Read temperature LSB
  // Convert BCD to decimal
  alarm1_minute = (alarm1_minute >> 4) * 10 + (alarm1_minute & 0x0F);
  alarm1_hour   = (alarm1_hour   >> 4) * 10 + (alarm1_hour & 0x0F);
  alarm2_minute = (alarm2_minute >> 4) * 10 + (alarm2_minute & 0x0F);
  alarm2_hour   = (alarm2_hour   >> 4) * 10 + (alarm2_hour & 0x0F);
  // End conversion
  alarm1.setCharAt(8, alarm1_minute % 10  + 48);
  alarm1.setCharAt(7, alarm1_minute / 10  + 48);
  alarm1.setCharAt(5, alarm1_hour   % 10  + 48);
  alarm1.setCharAt(4, alarm1_hour   / 10  + 48);
  alarm2.setCharAt(8, alarm2_minute % 10  + 48);
  alarm2.setCharAt(7, alarm2_minute / 10  + 48);
  alarm2.setCharAt(5, alarm2_hour   % 10  + 48);
  alarm2.setCharAt(4, alarm2_hour   / 10  + 48);
  alarm1_status = bitRead(control_reg, 0);      // Read alarm1 interrupt enable bit (A1IE) from DS3231 control register
  alarm2_status = bitRead(control_reg, 1);      // Read alarm2 interrupt enable bit (A2IE) from DS3231 control register
  if(temperature_msb < 0){
    temperature_msb = abs(temperature_msb);
    temperature.setCharAt(4, '-');
  }
  else
    temperature.setCharAt(4, ' ');
  temperature_lsb >>= 6;
  temperature.setCharAt(6, temperature_msb % 10  + 48);
  temperature.setCharAt(5, temperature_msb / 10  + 48);
  if(temperature_lsb == 0 || temperature_lsb == 2) {
    temperature.setCharAt(9, '0');
    if(temperature_lsb == 0) temperature.setCharAt(8, '0');
    else                     temperature.setCharAt(8, '5');
  }
  if(temperature_lsb == 1 || temperature_lsb == 3) {
    temperature.setCharAt(9, '5');
    if(temperature_lsb == 1) temperature.setCharAt(8, '2');
    else                     temperature.setCharAt(8, '7');
  }
  temperature.setCharAt(10, 223);                        // Put the degree symbol

  switch (mode) {
    case 1:
      return alarm1;
    case 2:
      return alarm2;
    case 3:
      return alarm1_status ? "y" : "n";
    case 4:
      return alarm2_status ? "y" : "n";
    case 5:
    default:
      return temperature;
  }
}

String RTC_DS3231::temp() {
  return tempAndAlarm(5);
}
String RTC_DS3231::alarm1() {
  return tempAndAlarm(1);
}
String RTC_DS3231::alarm2() {
  return tempAndAlarm(2);
}
String RTC_DS3231::a1status() {
  return tempAndAlarm(3);
}
String RTC_DS3231::a2status() {
  return tempAndAlarm(4);
}

