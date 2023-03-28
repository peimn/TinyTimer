#include <TinyWireM.h>
#include "LiquidCrystal_attiny.h"
#include "TinyRTClib.h" // https://github.com/ryo1kato/arduino-clock/blob/master/clock.ino

#define TEMP_PIN  1

LiquidCrystal_I2C lcd(0x27,16,2);

RTC_DS3231 rtc;
byte turn = 0;

void OneWireReset(int Pin);//See Note 2
void OneWireOutByte(int Pin, byte d);
byte OneWireInByte(int Pin);

void setup_rtc() {
  TinyWireM.begin();
  rtc.begin();
  if (rtc.lostPower()) {
      lcd.clear ();
      lcd.home ();
      lcd.print("RTC power lost");
      DateTime saved_time = DateTime(2023, 3, 15, 22, 50, 0);
      rtc.adjust( saved_time );
  }

  // set alarm
  DateTime alarm_1 = DateTime(0, 0, 0, 1, 15, 0);
  DateTime alarm_2 = DateTime(0, 0, 0, 1, 20, 0);
  rtc.adjustAlarm(alarm_1, alarm_2);
}

void print_time() {
  DateTime now = rtc.now();
  unsigned int Y = now.year();
  unsigned int M = now.month();
  unsigned int D = now.day();
  unsigned int h = now.hour();
  unsigned int m = now.minute();
  unsigned int s = now.second();
  lcd.clear ();
  lcd.home ();
  lcd.print(Y);
  lcd.print('-');
  lcd.print(M);
  lcd.print('-');
  lcd.print(D);
  lcd.setCursor ( 0, 1 ); // go to position
  lcd.print(h);
  lcd.print(":");
  lcd.print(m);
  lcd.print(":");
  lcd.print(s);
}

void print_alarm() {
  String alarm1 = rtc.alarm1(), alarm2 = rtc.alarm2(), a1status = rtc.a1status(),
    a2status = rtc.a2status();
  lcd.clear ();
  lcd.home ();
  lcd.print(alarm1);
  lcd.print(" > ");
  lcd.print(a1status);
  lcd.setCursor ( 0, 1 ); // go to position
  lcd.print(alarm2);
  lcd.print(" > ");
  lcd.print(a2status);
};

void print_temp() {
  String rtc_temp = rtc.temp();
  lcd.clear();
  lcd.home();
  lcd.print(rtc_temp);
  lcd.setCursor ( 0, 1 ); // go to position
  lcd.print("TOut: ");
  print_temp_DS18B20();
};

void print_temp_DS18B20() {
  int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;

  OneWireReset(TEMP_PIN);
  OneWireOutByte(TEMP_PIN, 0xcc);
  OneWireOutByte(TEMP_PIN, 0x44); // perform temperature conversion, strong pullup for one sec

  OneWireReset(TEMP_PIN);
  OneWireOutByte(TEMP_PIN, 0xcc);
  OneWireOutByte(TEMP_PIN, 0xbe);

  LowByte = OneWireInByte(TEMP_PIN);
  HighByte = OneWireInByte(TEMP_PIN);
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

  Whole = Tc_100 / 100;  // separate off the whole and fractional portions
  Fract = Tc_100 % 100;


  if (SignBit) // If its negative
  {
     lcd.print("-");
  }
  lcd.print(Whole);
  lcd.print(".");
  if (Fract < 10)
  {
     lcd.print("0");
  }
  lcd.print(Fract);
  char deg = 223;
  lcd.print(deg);
  lcd.print('C');  
};

void setup() {
  digitalWrite(TEMP_PIN, LOW);
  pinMode(TEMP_PIN, INPUT);
  // Now set up the LCD     
  // initialize the lcd
  lcd.init();
  lcd.backlight();
  setup_rtc();
}

void loop() {
  switch (turn) {
    case 0:
      print_time();
      break;
    case 1:
      print_alarm();
      break;
    case 2:
    default:
      print_temp();
      break;       
  }
  turn++;
  delay(2000);
  if (turn == 3) turn = 0;
}

void OneWireReset(int Pin) // reset.  Should improve to act as a presence pulse
{
     digitalWrite(Pin, LOW);
     pinMode(Pin, OUTPUT); // bring low for 500 us
     delayMicroseconds(500);
     pinMode(Pin, INPUT);
     delayMicroseconds(500);
}

void OneWireOutByte(int Pin, byte d) // output byte d (least sig bit first).
{
   byte n;

   for(n=8; n!=0; n--)
   {
      if ((d & 0x01) == 1)  // test least sig bit
      {
         digitalWrite(Pin, LOW);
         pinMode(Pin, OUTPUT);
         delayMicroseconds(5);
         pinMode(Pin, INPUT);
         delayMicroseconds(60);
      }
      else
      {
         digitalWrite(Pin, LOW);
         pinMode(Pin, OUTPUT);
         delayMicroseconds(60);
         pinMode(Pin, INPUT);
      }

      d=d>>1; // now the next bit is in the least sig bit position.
   }

}

byte OneWireInByte(int Pin) // read byte, least sig byte first
{
    byte d, n, b;

    d=0;//This critical line added 04 Oct 16
    //I hate to think how many derivatives of
    //this code exist elsewhere on my web pages
    //which have NOT HAD this. You may "get away"
    //with not setting d to zero here... but it
    //is A Very Bad Idea to trust to "hidden"
    //initializations!
    //The matter was brought to my attention by
    //a kind reader who was THINKING OF YOU!!!
    //If YOU spot an error, please write in, bring
    //it to my attention, to save the next person
    //grief.

    for (n=0; n<8; n++)
    {
        digitalWrite(Pin, LOW);
        pinMode(Pin, OUTPUT);
        delayMicroseconds(5);
        pinMode(Pin, INPUT);
        delayMicroseconds(5);
        b = digitalRead(Pin);
        delayMicroseconds(50);
        d = (d >> 1) | (b<<7); // shift d to right and insert b in most sig bit position
    }
    return(d);
}