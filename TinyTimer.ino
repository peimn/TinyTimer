#include <Wire.h>
#include <DS3231.h>
#include "LiquidCrystal_attiny.h"

#define TEMP_PIN  1

LiquidCrystal_I2C lcd(0x27,16,2);

DS3231 myRTC;
byte turn = 0;

bool century = false;
bool h12Flag;
bool pmFlag;

void print_time() {
  unsigned int Y = myRTC.getYear();
  unsigned int M = myRTC.getMonth(century);
  unsigned int D = myRTC.getDate();
  unsigned int h = myRTC.getHour(h12Flag, pmFlag);
  unsigned int m = myRTC.getMinute();
  unsigned int s = myRTC.getSecond();
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

void setup() {
  // Now set up the LCD     
  // initialize the lcd
  lcd.init();
  lcd.backlight();
  Wire.begin();
}

void loop() {
  switch (turn) {
    case 0:
      print_time();
      break;
    case 1:
      print_time();
      break;
    case 2:
    default:
      print_time();
      break;       
  }
  turn++;
  delay(2000);
  if (turn == 3) turn = 0;
}