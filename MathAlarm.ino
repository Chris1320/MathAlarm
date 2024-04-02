/*
 *
 * MathAlarm - A CAPTCHA-enabled alarm clock.
 *
 * ========== MODULES USED ==========
 *
 * - 2x LED Lights
 * - 1x Passive Buzzer
 * - 1x LCD1602 Module
 * - 1x Membrane Switch Module
 * - 1x DS3231 Real-Time Clock Module
 *
 * ========== THIRD-PARTY LIBRARIES INCLUDED ==========
 *
 * - Keypad v3.1.1 (https://playground.arduino.cc/Code/Time/)
 * - LiquidCrystal_I2C v1.1.2
 * (https://github.com/marcoschwartz/LiquidCrystal_I2C)
 * - DS3231 v1.1.2 (https://github.com/NorthernWidget/DS3231)
 *
 * ========== PIN LAYOUT ==========
 *
 * | Pin Number/Code | Description              |
 * | --------------- | ------------------------ |
 * | 2               | Green LED anode          |
 * | 3               | Passive buzzer anode     |
 * | 4               | Red LED anode            |
 * | 6               | Membrane switch column 4 |
 * | 7               | Membrane switch column 3 |
 * | 8               | Membrane switch column 2 |
 * | 9               | Membrane switch column 1 |
 * | 10              | Membrane switch row 4    |
 * | 11              | Membrane switch row 3    |
 * | 12              | Membrane switch row 2    |
 * | 13              | Membrane switch row 1    |
 * | A4              | DS3231 SDA               |
 * | A5              | DS3231 I2C SCL           |
 *
 */

#include <DS3231.h>
#include <EEPROM.h>  // NOTE: see reseedRandom() for why
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
// #include <Wire.h>
// #include <time.h>

#define BAUD_RATE 9600  // 115200 is too fast

// Alarm configuration
// formula: x * y + z
#define ALARM_X_MIN 5
#define ALARM_Y_MIN 5
#define ALARM_Z_MIN 5
#define ALARM_X_MAX 10
#define ALARM_Y_MAX 10
#define ALARM_Z_MAX 20

#define PIN_LED_GREEN 2
#define PIN_LED_RED 4

#define PIN_BUZ 3

// Keypad configuration
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
#define KEYPAD_BACKSPACE_CHAR keypad_keys[3][0]  // use this key as a backspace
#define KEYPAD_ENTER_CHAR keypad_keys[3][2]      // use this key as enter button
// keypad pinout: R1,R2,R3,R4,C1,C2,C3,C4
byte PIN_KEYPAD_ROWS[KEYPAD_ROWS] = {13, 12, 11, 10};
byte PIN_KEYPAD_COLS[KEYPAD_COLS] = {9, 8, 7, 6};
char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},  // map keypad buttons to matrix.
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
Keypad keypad = Keypad(makeKeymap(keypad_keys), PIN_KEYPAD_ROWS,
                       PIN_KEYPAD_COLS, KEYPAD_ROWS, KEYPAD_COLS);

// LCD I2C configuration
// walang documentation, idk how to do this...
// like, why do I not have to set the SCA/SDL pins???
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DS3231 RTC configuration
DS3231 rtc;
bool rtc_is_year_overflow, rtc_is_12hr, rtc_is_pm;

// This function allows us to get a random number everytime,
// instead of having the same sequence of numbers since random()
// is a pseudo-random number generator (pRNG).
// source:
// https://forum.arduino.cc/t/the-reliable-but-not-very-sexy-way-to-seed-random/65872/53
void reseedRandom(void) {
  static const uint32_t HappyPrime = 937;
  union {
    uint32_t i;
    uint8_t b[4];
  } raw;
  int8_t i;
  unsigned int seed;

  for (i = 0; i < sizeof(raw.b); ++i) {
    raw.b[i] = EEPROM.read(i);
  }

  do {
    raw.i += HappyPrime;
    seed = raw.i & 0x7FFFFFFF;
  } while ((seed < 1) || (seed > 2147483646));

  randomSeed(seed);

  for (i = 0; i < sizeof(raw.b); ++i) {
    EEPROM.write(i, raw.b[i]);
  }
}

// Set lines 1 and 2 of the LCD to the contents of <line1> and <line2>,
// overwriting its current contents.
void setLCDContent(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// left-pad the string using the specified character.
String leftPadString(String str_to_pad, String padchar, int length) {
  String padded_string = "";
  int pad_length = length - str_to_pad.length();
  // Add padding to the left
  for (int i = 0; i < pad_length; i++) padded_string += padchar;
  padded_string += str_to_pad;
  return padded_string;
}

// set the date.
void setDate(byte* current_datetime, String prompt) {
  byte date_cols[] = {0, 1, 3, 4, 6, 7};  // for LCD
  char date_cols_placeholder[] = {'Y', 'Y', 'M', 'M', 'D', 'D'};

  Serial.println("Current date: " + String(current_datetime[0]) + '-' +
                 current_datetime[1] + '-' + current_datetime[2]);

  Serial.println("Asking the user the date.");
  setLCDContent(prompt,
                leftPadString(String(current_datetime[0]), "0", 2) + "-" +
                    leftPadString(String(current_datetime[1]), "0", 2) + "-" +
                    leftPadString(String(current_datetime[2]), "0", 2));
  byte input_len = 6;  // YYYYMMDD
  byte pointer = 0;
  char input[input_len + 1] = {};  // +1 to account for null byte
  while (true) {
    char keypress = keypad.getKey();
    if (keypress == NULL) continue;  // wait for user input.

    // backspace
    if (keypress == KEYPAD_BACKSPACE_CHAR) {
      if (pointer != 0) {
        input[--pointer] = 0;
        lcd.setCursor(date_cols[pointer], 1);
        lcd.print(date_cols_placeholder[pointer]);
      }
      continue;
    }

    if (keypress == KEYPAD_ENTER_CHAR) {
      // convert char into int...
      // ...what even is this
      // subtract the ASCII value of '0'
      byte year1 = input[0] - '0';
      byte year2 = input[1] - '0';
      byte month1 = input[2] - '0';
      byte month2 = input[3] - '0';
      byte date1 = input[4] - '0';
      byte date2 = input[5] - '0';

      byte year = (year1 * 10) + year2;
      byte month = (month1 * 10) + month2;
      byte date = (date1 * 10) + date2;
      Serial.println("YYYY-MM-DD == " + String(year) + '-' + String(month) +
                     '-' + String(date));
      current_datetime[0] = year;
      current_datetime[1] = month;
      current_datetime[2] = date;
      return;
    }
    input[pointer] = keypress;
    if (pointer == input_len) continue;
    if (pointer < input_len - 1) {
      lcd.setCursor(date_cols[pointer + 1], 1);
      lcd.print('_');
    }
    lcd.setCursor(date_cols[pointer], 1);
    lcd.print(input[pointer++]);

    Serial.println(String(pointer) + " | " + String(input));
  }
}

// set the time.
void setTime(byte* current_datetime, String prompt) {
  byte time_cols[] = {0, 1, 3, 4, 6, 7};  // for LCD
  char time_cols_placeholder[] = {'H', 'H', 'M', 'M', 'S', 'S'};

  Serial.println("Current time: " + String(current_datetime[3]) + ':' +
                 String(current_datetime[4]) + ':' +
                 String(current_datetime[5]));

  Serial.println("Asking the user to change/verify the time.");
  setLCDContent(prompt,
                leftPadString(String(current_datetime[3]), "0", 2) + ":" +
                    leftPadString(String(current_datetime[4]), "0", 2) + ":" +
                    leftPadString(String(current_datetime[5]), "0", 2));
  byte input_len = 6;  // HHMMSS
  byte pointer = 0;
  char input[input_len + 1] = {};  // +1 to account for null byte
  while (true) {
    char keypress = keypad.getKey();
    if (keypress == NULL) continue;  // wait for user input.

    // backspace
    if (keypress == KEYPAD_BACKSPACE_CHAR) {
      if (pointer != 0) {
        input[--pointer] = 0;
        lcd.setCursor(time_cols[pointer], 1);
        lcd.print(time_cols_placeholder[pointer]);
      }
      continue;
    }

    if (keypress == KEYPAD_ENTER_CHAR) {
      // convert char into int...
      // ...what even is this
      // subtract the ASCII value of '0'
      byte hour1 = input[0] - '0';
      byte hour2 = input[1] - '0';
      byte minute1 = input[2] - '0';
      byte minute2 = input[3] - '0';
      byte second1 = input[4] - '0';
      byte second2 = input[5] - '0';

      byte hour = (hour1 * 10) + hour2;
      byte minute = (minute1 * 10) + minute2;
      byte second = (second1 * 10) + second2;
      Serial.println("HH:MM:SS == " + String(hour) + ':' + String(minute) +
                     ':' + String(second));
      current_datetime[3] = hour;
      current_datetime[4] = minute;
      current_datetime[5] = second;
      return;
    }
    input[pointer] = keypress;
    if (pointer == input_len) continue;
    if (pointer < input_len - 1) {
      lcd.setCursor(time_cols[pointer + 1], 1);
      lcd.print('_');
    }
    lcd.setCursor(time_cols[pointer], 1);
    lcd.print(input[pointer++]);

    Serial.println(String(pointer) + " | " + String(input));
  }
}

// Set the date and time at startup.
void firstRun() {
  Serial.println("Showing the first run wizard.");
  byte current_datetime[] = {
      rtc.getYear(),   rtc.getMonth(rtc_is_year_overflow),
      rtc.getDate(),   rtc.getHour(rtc_is_12hr, rtc_is_pm),
      rtc.getMinute(), rtc.getSecond()};
  setDate(current_datetime, "Set the date:");
  setTime(current_datetime, "Set the time: ");
  Serial.println(
      "Datetime: " + String(current_datetime[0]) + "-" +
      String(current_datetime[1]) + "-" + String(current_datetime[2]) + " " +
      String(current_datetime[3]) + ":" + String(current_datetime[4]) + ":" +
      String(current_datetime[5]));

  rtc.setYear(current_datetime[0]);
  rtc.setMonth(current_datetime[1]);
  rtc.setDate(current_datetime[2]);
  rtc.setHour(current_datetime[3]);
  rtc.setMinute(current_datetime[4]);
  rtc.setSecond(current_datetime[5]);
  Serial.println("RTC updated.");
}

void modeSelect() {
  Serial.println("Asking user for mode...");
  setLCDContent("[A] Alarm Clock Mode", "[B] Timer Mode");
  while (true) {
    char selected_mode = keypad.getKey();
    if (selected_mode == NULL) continue;
    Serial.println("Selected:" + selected_mode);
    if (selected_mode == keypad_keys[0][3]) {
      setLCDContent("Selected Mode:", "Alarm Clock Mode");
      startAlarmClockMode();
      break;
    } else if (selected_mode == keypad_keys[1][4]) {
      setLCDContent("Selected Mode:", "Timer Mode");
      startTimerMode();
      break;
    }
  }
}

void startAlarmClockMode() {
  // NOTE: pad with 0s to conform
  byte target_time_tmp[] = {0,
                            0,
                            0,
                            rtc.getHour(rtc_is_12hr, rtc_is_pm),
                            rtc.getMinute(),
                            rtc.getSecond()};

  Serial.println("Alarm Clock Mode selected.");
  setTime(target_time_tmp, "Alarm Time:");
  byte target_time[] = {target_time_tmp[3], target_time_tmp[4],
                        target_time_tmp[5]};
  Serial.println("Alarm time: " + String(target_time[0]) + ":" +
                 String(target_time[1]) + ":" + String(target_time[2]));
  while (!(rtc.getHour(rtc_is_12hr, rtc_is_pm) == target_time[0] &&
           rtc.getMinute() == target_time[1] &&
           rtc.getSecond() == target_time[2])) {
    int total_rem =
        (target_time[2] - rtc.getSecond()) +
        ((target_time[1] - rtc.getMinute()) * 60) +
        ((target_time[0] - rtc.getHour(rtc_is_12hr, rtc_is_pm)) * 3600);
    int rem_hh = total_rem / 3600;
    int rem_mm = (total_rem % 3600) / 60;
    int rem_ss = (total_rem % 3600) % 60;

    lcd.setCursor(0, 1);
    lcd.print(leftPadString(String(rem_hh), "0", 2) + "h ");
    lcd.print(leftPadString(String(rem_mm), "0", 2) + "m ");
    lcd.print(leftPadString(String(rem_ss), "0", 2) + "s");

    Serial.print(leftPadString(String(rem_hh), "0", 2) + "h ");
    Serial.print(leftPadString(String(rem_mm), "0", 2) + "m ");
    Serial.println(leftPadString(String(rem_ss), "0", 2) + "s");
  }

  // generate a random equation
  int x = random(ALARM_X_MIN, ALARM_X_MAX),
      y = random(ALARM_Y_MIN, ALARM_Y_MAX),
      z = random(ALARM_Z_MIN, ALARM_Z_MAX);
  int answer = x * y + z;
  setLCDContent(String(x) + " * " + String(y) + " + " + String(z), "=      ");
  byte time_cols[] = {2, 3, 4, 5, 6};  // for LCD
  char time_cols_placeholder[] = {'x', 'x', 'x', 'x', 'x'};

  byte input_len = 5;  // HHMMSS
  byte pointer = 0;
  char input[input_len + 1] = {};  // +1 to account for null byte
  while (true) {
    char keypress = keypad.getKey();
    if (keypress == NULL) continue;  // wait for user input.

    // backspace
    if (keypress == KEYPAD_BACKSPACE_CHAR) {
      if (pointer != 0) {
        input[--pointer] = 0;
        lcd.setCursor(time_cols[pointer], 1);
        lcd.print(time_cols_placeholder[pointer]);
      }
      continue;
    }

    if (keypress == KEYPAD_ENTER_CHAR) {
      // convert char into int...
      // ...what even is this
      // subtract the ASCII value of '0'
      if (String(input) == String(answer))
        return;
      else {
        int x = random(ALARM_X_MIN, ALARM_X_MAX),
            y = random(ALARM_Y_MIN, ALARM_Y_MAX),
            z = random(ALARM_Z_MIN, ALARM_Z_MAX);
        int answer = x * y + z;
        setLCDContent(String(x) + " * " + String(y) + " + " + String(z),
                      "=      ");
        pointer = 0;
        for (int i = 0; i < input_len; i++) input[i] = NULL;

        continue;
      }
    }
    input[pointer] = keypress;
    if (pointer == input_len) continue;
    if (pointer < input_len - 1) {
      lcd.setCursor(time_cols[pointer + 1], 1);
      lcd.print('_');
    }
    lcd.setCursor(time_cols[pointer], 1);
    lcd.print(input[pointer++]);

    Serial.println(String(pointer) + " | " + String(input));
  }
}

void startTimerMode() { Serial.println("Timer Mode selected."); }

// the entrypoint
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println("Reseeding RNG...");
  reseedRandom();

  Serial.println("Initializing LED indicators...");
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  Serial.println("Initializing buzzer...");
  pinMode(PIN_BUZ, OUTPUT);

  Serial.println("Initializing LCD module...");
  lcd.init();
  lcd.backlight();

  Serial.println("Initializing DS3231 module...");
  rtc.setClockMode(false);

  Serial.println("Initializing I2C communication...");
  Wire.begin();

  Serial.println("Ready.");
  firstRun();
}

void loop() { modeSelect(); }
