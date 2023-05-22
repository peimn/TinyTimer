#include <Wire.h>
#include <DS3231.h>
#include "LiquidCrystal_attiny.h"

#define TEMP_PIN  1

LiquidCrystal_I2C lcd(0x27,16,2);

// Interrupt frequency, in seconds
#define INT_FREQ 3UL // 3 seconds, characterized as unsigned long

// myRTC interrupt pin
#define CLINT 2

DS3231 myRTC;
byte turn = 0;

bool century = false;
bool h12Flag;
bool pmFlag;

// Variables for use in method parameter lists
byte alarmDay;
byte alarmHour;
byte alarmMinute;
byte alarmSecond;
byte alarmBits;
bool alarmDayIsDay;
bool alarmH12;
bool alarmPM;

// Interrupt signaling byte
volatile byte tick = 1;

void print_time() {
  unsigned int Y = myRTC.getYear();
  unsigned int M = myRTC.getMonth(century);
  unsigned int D = myRTC.getDate();
  unsigned int h = myRTC.getHour(h12Flag, pmFlag);
  unsigned int m = myRTC.getMinute();
  unsigned int s = myRTC.getSecond();
  lcd.clear();
  lcd.home();
  lcd.print(Y);
  lcd.print('-');
  lcd.print(M);
  lcd.print('-');
  lcd.print(D);
  lcd.setCursor(0, 1); // go to position
  lcd.print(h);
  lcd.print(":");
  lcd.print(m);
  lcd.print(":");
  lcd.print(s);
}

void set_alarm_1() {
  // Assign parameter values for Alarm 1
  alarmDay = myRTC.getDate();
  alarmHour = myRTC.getHour(alarmH12, alarmPM);
  alarmMinute = myRTC.getMinute();
  alarmSecond = INT_FREQ; // initialize to the interval length
  alarmBits = 0b00001110; // Alarm 1 when seconds match
  alarmDayIsDay = false; // using date of month

  // Upload initial parameters of Alarm 1
  myRTC.turnOffAlarm(1);
  myRTC.setA1Time(
      alarmDay, alarmHour, alarmMinute, alarmSecond,
      alarmBits, alarmDayIsDay, alarmH12, alarmPM);
  // clear Alarm 1 flag after setting the alarm time
  myRTC.checkIfAlarm(1);
  // now it is safe to enable interrupt output
  myRTC.turnOnAlarm(1);

  // When using interrupt with only one of the DS3231 alarms, as in this example,
  // it may be possible to prevent the other alarm entirely,
  // so it will not covertly block the outgoing interrupt signal.

  // Try to prevent Alarm 2 altogether by assigning a 
  // nonsensical alarm minute value that cannot match the clock time,
  // and an alarmBits value to activate "when minutes match".
  alarmMinute = 0xFF; // a value that will never match the time
  alarmBits = 0b01100000; // Alarm 2 when minutes match, i.e., never
  
  // Upload the parameters to prevent Alarm 2 entirely
  myRTC.setA2Time(
      alarmDay, alarmHour, alarmMinute,
      alarmBits, alarmDayIsDay, alarmH12, alarmPM);
  // disable Alarm 2 interrupt
  myRTC.turnOffAlarm(2);
  // clear Alarm 2 flag
  myRTC.checkIfAlarm(2);
}

void print_alarm_1_status() {
  // static variable to keep track of on/off state
  static byte state = false;

  // Do when alarm interrupt received:
  if (tick) {
    // right away, capture the current time in a DateTime variable
    // for later processing
    DateTime alarmDT = RTClib::now();

    // disable Alarm 1 interrupt
    myRTC.turnOffAlarm(1);
    
    // Clear Alarm 1 flag
    myRTC.checkIfAlarm(1);
    
    tick = 0; // reset the local interrupt-received flag
    state = ~state; // reverse the state

    // optional serial output
    lcd.clear();
    lcd.home();
    lcd.print("Turning ");
    lcd.print((state ? "ON" : "OFF"));
    lcd.print(" at:");
    lcd.setCursor(0, 1); // go to position
    lcd.print(alarmDT.hour());
    lcd.print(":");
    lcd.print(alarmDT.minute());
    lcd.print(":");
    lcd.println(alarmDT.second());

    // extract the DateTime values as a timestamp 
    uint32_t nextAlarm = alarmDT.unixtime();
    // add the INT_FREQ number of seconds
    nextAlarm += INT_FREQ;
    // update the DateTime with the new timestamp
    alarmDT = DateTime(nextAlarm);

    // upload the new time to Alarm 1
    myRTC.setA1Time(
      alarmDT.day(), alarmDT.hour(), alarmDT.minute(), alarmDT.second(),
    alarmBits, alarmDayIsDay, alarmH12, alarmPM);
    
    // enable Alarm 1 interrupts
    myRTC.turnOnAlarm(1);
    // clear Alarm 1 flag again after enabling interrupts
    myRTC.checkIfAlarm(1);
  }
}

void print_temp() {
  unsigned int T = myRTC.getTemperature();
  lcd.setCursor(12, 0); // go to position
  lcd.print(T);
}

void setup() {
  // Set the DS3231 clock mode to 24-hour
  myRTC.setClockMode(false); // false = not using the alternate, 12-hour mode

  set_alarm_1();

  // NOTE: both of the alarm flags must be clear
  // to enable output of a FALLING interrupt

  // attach clock interrupt
  pinMode(CLINT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLINT), isr_TickTock, FALLING);

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
      print_temp();
      break;
    case 1:
      print_alarm_1_status();
      break;
    case 2:
    default:
      print_alarm_1_status();
      break;       
  }
  turn++;
  delay(2000);
  if (turn == 3) turn = 0;
}

void isr_TickTock() {
    // interrupt signals to loop
    tick = 1;
    return;
}