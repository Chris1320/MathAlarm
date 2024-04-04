# MathAlarm - A CAPTCHA-enabled alarm clock.

## Modules Used

- 2x LED Lights (Green/Red)
- 1x Active Buzzer
- 1x Passive Buzzer
- 1x LCD1602 Module
- 1x Membrane Switch Module
- 1x DS3231 Real-Time Clock Module

## Third-Party Libraries Included

| Library           | Version | Link                                               |
| ----------------- | ------- | -------------------------------------------------- |
| Keypad            | v3.1.1  | https://playground.arduino.cc/Code/Time            |
| LiquidCrystal_I2C | v1.1.2  | https://github.com/marcoschwartz/LiquidCrystal_I2C |
| DS3231            | v1.1.2  | https://github.com/NorthernWidget/DS3231           |

## Pin Layout

| Pin Number/Code | Description              |
| --------------- | ------------------------ |
| 2               | Green LED anode          |
| 3               | Active buzzer            |
| 4               | Red LED anode            |
| 5               | Passive buzzer           |
| 6               | Membrane switch column 4 |
| 7               | Membrane switch column 3 |
| 8               | Membrane switch column 2 |
| 9               | Membrane switch column 1 |
| 10              | Membrane switch row 4    |
| 11              | Membrane switch row 3    |
| 12              | Membrane switch row 2    |
| 13              | Membrane switch row 1    |
| A4              | DS3231 SDA               |
| A5              | DS3231 I2C SCL           |
| SDA             | LCD1602 SDA              |
| SCL             | LCD1602 I2C SCL          |
