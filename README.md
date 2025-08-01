# Automatic-Chicken-Door

This project uses an ESP32 to automatically control a motorized door based on sunrise and sunset times retrieved from the OpenWeatherMap API. The system also supports manual control and displays the current status on an I2C LCD.

## Features

- Automatically opens the door shortly before sunrise
- Automatically closes the door after sunset
- Manual open/close buttons
- Manual reset button
- Detects door position using limit switches
- Displays WiFi and door status on a 16x2 I2C LCD
- Recovers from power loss and re-syncs time

## Hardware Requirements

- ESP32
- H-Bridge Motor Driver (L298N)
- DC Motor
- Two limit switches
- Three momentary push buttons (normaly open)
- I2C 16x2 LCD display (address 0x27)
- 5v Power supply for ESP32 and motor

## Pin Assignments

- Motor IN1: GPIO 27  
- Motor IN2: GPIO 26  
- Limit Switch (Open): GPIO 32  
- Limit Switch (Closed): GPIO 33  
- Button (Open): GPIO 25  
- Button (Close): GPIO 14  
- I2C SDA: GPIO 21 
- I2C SCL: GPIO 22 

Adjust SDA and SCL pins if your ESP32 board uses different defaults.

## OpenWeatherMap API Setup

1. Sign up at https://openweathermap.org
2. Generate an API key
3. Replace the following lines in the code with your credentials:

const char* ssid = "YOUR_WIFI_SSID";

const char* password = "YOUR_WIFI_PASSWORD";

String openWeatherMapApiKey = "YOUR_API_KEY";

String city = "YourCity";

String countryCode = "YourCountryCode";


Time Zone Configuration:
OpenWeatherMap returns UTC time. You must manually adjust to your local time using the timeZoneOffset variable. This offset is in seconds:


Example for U.S. Central Daylight Time:
const long timeZoneOffset = -5 * 3600;


Common Offsets:

UTC: 0

Eastern Standard Time: -5 * 3600

Central Standard Time: -6 * 3600

Mountain Standard Time: -7 * 3600

Pacific Standard Time: -8 * 3600

Eastern Daylight Time: -4 * 3600

Central Daylight Time: -5 * 3600

UK (GMT/BST): 0 or +1 * 3600

Europe (CET/CEST): +1 * 3600 or +2 * 3600

India (IST): +5.5 * 3600

Japan (JST): +9 * 3600

Australia (AEST): +10 * 3600

Note: For locations with Daylight Saving Time, you will need to update the offset manually unless using a library that supports automatic DST handling.


Required Libraries:

WiFi (comes with ESP32 board support)

HTTPClient (comes with ESP32 board support)

Arduino_JSON by Arduino (https://github.com/arduino-libraries/Arduino_JSON)

TimeLib by Paul Stoffregen (https://github.com/PaulStoffregen/Time)

LiquidCrystal_I2C by Martin Kubovčík (https://github.com/markub3327/LiquidCrystal_I2C)

How It Works:

The system opens the door approximately 45 minutes before local sunrise.

The door closes after sunset.

If the door misses an event due to reboot or power loss, it will correct on the next boot.

Manual control is available via two buttons.

Manual reset is avalible via one button.

The LCD shows WiFi connection status and either the current door action or scheduled times.

License:
This project is open source and provided as-is under the MIT License.
