// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
// No changes from RTClib.h

#ifndef _RTCLIB_H_
#define _RTCLIB_H_

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime (uint32_t t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const char* date, const char* time);
    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long secondstime() const;   
    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};

// RTC based on the DS3231 chip connected via I2C and the Wire library
class RTC_DS3231 {
public:
    static uint8_t begin(void);
    static void adjust(const DateTime& dt);
    static void adjustAlarm(const DateTime& at1, const DateTime& at2);
    bool lostPower(void);
    static DateTime now();
    static String temp();
    static String alarm1();
    static String alarm2();
    static String a1status();
    static String a2status();
};


#endif // _RTCLIB_H_
