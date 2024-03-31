#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

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

// Keypad configuration
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
// keypad pinout: R1,R2,R3,R4,C1,C2,C3,C4
// NOTE: for some reason PIN1 doesn't work with
// keypad so I moved it to pins 2-9.
byte PIN_KEYPAD_ROWS[KEYPAD_ROWS] = {2, 3, 4, 5};
byte PIN_KEYPAD_COLS[KEYPAD_COLS] = {6, 7, 8, 9};
char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},  // map keypad buttons to matrix.
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
Keypad keypad = Keypad(makeKeymap(keypad_keys), PIN_KEYPAD_ROWS,
                       PIN_KEYPAD_COLS, KEYPAD_ROWS, KEYPAD_COLS);

// LCD I2C configuration
// walang documentation, idk how to do this... YOLO
LiquidCrystal_I2C lcd(0x27, 16, 2);

// LED indicators configuration
byte PIN_LED_GREEN = 10;
byte PIN_LED_RED = 11;
byte PIN_BUZ = 12;

// the time in 24hr format
byte hh = 0, mm = 0, ss = 0;
byte alarm_hh = 0, alarm_mm = 0, alarm_ss = 0;
int time_cols[] = {0, 1, 3, 4, 6, 7};  // for LCD
char time_cols_placeholder[] = {'H', 'H', 'M', 'M', 'S', 'S'};

/* always overwrite LCD contents */
void setLCDContent(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void firstRun() {
  setLCDContent("Enter time:", "HH:MM:SS");
  byte input_len = 6;
  byte pointer = 0;
  char input[input_len + 1] = {};
  while (true) {
    do {
      input[pointer] = keypad.getKey();
    } while (input[pointer] == NULL);

    // backspace
    if (input[pointer] == '*') {
      if (pointer != 0) {
        input[pointer] = NULL;
        lcd.setCursor(time_cols[--pointer], 1);
        lcd.print(time_cols_placeholder[pointer]);
      }
    }

    else if (input[pointer] == '#') {
      // convert char into int...
      // ...what even is this
      int hh1 = input[0] - '0';
      int hh2 = input[1] - '0';
      int mm1 = input[2] - '0';
      int mm2 = input[3] - '0';
      int ss1 = input[4] - '0';
      int ss2 = input[5] - '0';
      hh = (hh1 * 10) + hh2;
      mm = (mm1 * 10) + mm2;
      ss = (ss1 * 10) + ss2;
      Serial.print("HH:MM:SS == ");
      Serial.print(hh);
      Serial.print(':');
      Serial.print(mm);
      Serial.print(':');
      Serial.println(ss);
      break;
    }

    // input array is full
    else if (pointer == input_len) {
      continue;
    } else {
      lcd.setCursor(time_cols[pointer], 1);
      lcd.print(input[pointer++]);
    }
    Serial.print(pointer);
    Serial.print(" | ");
    Serial.println(input);
  }
}

void modeSelect() {
  setLCDContent("[A] Alarm Clock Mode", "[B] Timer Mode");
  while (true) {
    char selected_mode = keypad.getKey();
    if (selected_mode == NULL) continue;
    Serial.println("Checking selection");
    Serial.print("Selected:");
    Serial.println(selected_mode);
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
  setLCDContent("Alarm Time:", "HH:MM:SS");
  byte input_len = 6;
  byte pointer = 0;
  char input[input_len + 1] = {};
  while (true) {
    do {
      input[pointer] = keypad.getKey();
    } while (input[pointer] == NULL);

    // backspace
    if (input[pointer] == '*') {
      if (pointer != 0) {
        input[pointer] = NULL;
        lcd.setCursor(time_cols[--pointer], 1);
        lcd.print(time_cols_placeholder[pointer]);
      }
    }

    else if (input[pointer] == '#') {
      int hh1 = input[0] - '0';
      int hh2 = input[1] - '0';
      int mm1 = input[2] - '0';
      int mm2 = input[3] - '0';
      int ss1 = input[4] - '0';
      int ss2 = input[5] - '0';
      alarm_hh = (hh1 * 10) + hh2;
      alarm_mm = (mm1 * 10) + mm2;
      alarm_ss = (ss1 * 10) + ss2;
      Serial.print("HH:MM:SS == ");
      Serial.print(hh);
      Serial.print(':');
      Serial.print(mm);
      Serial.print(':');
      Serial.println(ss);
      break;
    }

    // input array is full
    else if (pointer == input_len) {
      continue;
    } else {
      lcd.setCursor(time_cols[pointer], 1);
      lcd.print(input[pointer++]);
    }
    Serial.print(pointer);
    Serial.print(" | ");
    Serial.println(input);
  }
}

void startTimerMode() {
  // WIP
}

void incrementTime() {
  if (ss++ == 60) {
    ss = 0;
    if (mm++ == 60) {
      mm = 0;
      if (hh++ == 24) {
        hh = 0;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);  // Set up serial connection.

  Serial.println("Reseeding RNG...");
  reseedRandom();

  Serial.println("Initializing LED indicators...");
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  Serial.println("Initializing LCD Module...");
  lcd.init();
  lcd.backlight();

  Serial.println("Ready.");
  firstRun();
  modeSelect();
}

void loop() {
  Serial.println("Start loop");
  while (!(hh == alarm_hh && mm == alarm_mm && ss == alarm_ss)) {
    delay(1000);
    incrementTime();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(hh);
    lcd.print(':');
    lcd.print(mm);
    lcd.print(':');
    lcd.print(ss);

    Serial.print(hh);
    Serial.print(':');
    Serial.print(mm);
    Serial.print(':');
    Serial.print(ss);
    Serial.print(" == ");
    Serial.print(alarm_hh);
    Serial.print(':');
    Serial.print(alarm_mm);
    Serial.print(':');
    Serial.println(alarm_ss);
  }
  // generate a random equation
  int x = random(5, 10), y = random(5, 10), z = random(5, 20);
  int answer = x * y + z;
  setLCDContent(String(x) + " * " + String(y) + " + " + String(z), "=");

  byte input_len = 4;
  byte pointer = 0;
  char input[input_len + 1] = {};
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_BUZ, HIGH);
  while (true) {
    do {
      input[pointer] = keypad.getKey();
    } while (input[pointer] == NULL);

    // backspace
    if (input[pointer] == '*') {
      if (pointer != 0) {
        input[pointer] = NULL;
        lcd.setCursor(time_cols[--pointer], 1);
        lcd.print(time_cols_placeholder[pointer]);
      }
    }

    else if (input[pointer] == '#') {
      if (String(input) == String(answer) + '#') {
        digitalWrite(PIN_LED_RED, LOW);
        digitalWrite(PIN_BUZ, LOW);
        digitalWrite(PIN_LED_GREEN, HIGH);
        delay(3000);
        digitalWrite(PIN_LED_GREEN, LOW);
        modeSelect();
        break;
      }
      continue;
    }

    // input array is full
    else if (pointer == input_len) {
      continue;
    } else {
      lcd.setCursor(time_cols[pointer], 1);
      lcd.print(input[pointer++]);
    }
    Serial.print(pointer);
    Serial.print(" | ");
    Serial.println(input);
  }
}
