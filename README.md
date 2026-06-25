# ESP32-S3 RFID Attendance System

A smart attendance system using ESP32-S3 and MFRC522 RFID reader that 
automatically logs employee check-in and check-out times to Google Sheets 
via WiFi in real time.

## Features
- RFID card scan for employee identification
- Auto Check-IN / Check-OUT toggle
- Real time clock via NTP (IST UTC+5:30)
- Google Sheets logging over WiFi
- 16x2 I2C LCD display for status
- Green/Red LED indicators
- Active buzzer feedback

## Hardware Used
- ESP32-S3
- MFRC522 RFID Reader
- 16x2 LCD with I2C module
- Active Buzzer
- Green & Red LEDs

## Pin Configuration
| Component  | Pin        |
|------------|------------|
| RFID SS    | GPIO10     |
| RFID RST   | GPIO4      |
| RFID SCK   | GPIO12     |
| RFID MOSI  | GPIO11     |
| RFID MISO  | GPIO13     |
| LCD SDA    | GPIO8      |
| LCD SCL    | GPIO9      |
| Green LED  | GPIO16     |
| Red LED    | GPIO17     |
| Buzzer     | GPIO21     |

## Libraries Required
- MFRC522 by GithubCommunity
- LiquidCrystal I2C
- WiFi (built-in)

## Setup
1. Clone this repository
2. Install required libraries in Arduino IDE
3. Update WiFi credentials in the code
4. Update Google Sheets script URL
5. Add your staff names and card UIDs
6. Upload to ESP32-S3
   
<img width="1536" height="1024" alt="image" src="https://github.com/user-attachments/assets/97464851-8b8d-49f4-9bf1-a89316f8e956" />

   
