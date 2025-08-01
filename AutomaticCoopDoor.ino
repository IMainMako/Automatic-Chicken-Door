#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <TimeLib.h>              
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "SSID";
const char* password = "Password";

// OpenWeatherMap API
String openWeatherMapApiKey = "OpenWeather API Key";
String city = "Nashville";
String countryCode = "US";

// Time zone offset (CDT = UTC-5)
const long timeZoneOffset = -5 * 3600;

// GPIO pins
const int motor1Pin1 = 27;
const int motor1Pin2 = 26;
const int limitSwitchOpen = 32;
const int limitSwitchClosed = 33;
const int buttonOpen = 25;
const int buttonClose = 14;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Timers
unsigned long timerDelay = 60 * 60 * 1000UL;  // 1 hour
unsigned long lastSunUpdate = 0;

// Sunrise/sunset times
time_t localSunriseTime;
time_t localSunsetTime;

// Flags
bool doorOpenedToday = false;
bool doorClosedTonight = false;
bool wifiConnected = false;

String doorState = "Idle";

void setup() {
  Serial.begin(115200);

  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(limitSwitchOpen, INPUT_PULLUP);
  pinMode(limitSwitchClosed, INPUT_PULLUP);
  pinMode(buttonOpen, INPUT_PULLUP);
  pinMode(buttonClose, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  connectWiFi();
  updateSunTimes();
  updateLCD();

  time_t nowTime = now();
  Serial.print("Time now: ");
  printDateTime(nowTime);
  Serial.print("Open time: ");
  printDateTime(localSunriseTime - 2700);

  if (nowTime >= localSunriseTime && digitalRead(limitSwitchOpen) == HIGH && nowTime < localSunsetTime) {
    Serial.println("Boot: Missed open time. Opening door now...");
    openDoor();
    doorOpenedToday = true;
  } else if (digitalRead(limitSwitchOpen) == LOW) {
    Serial.println("Boot: Door already open.");
    doorOpenedToday = true;
  }

  if (nowTime >= localSunsetTime && digitalRead(limitSwitchClosed) == HIGH || nowTime < localSunriseTime) {
    Serial.println("Boot: Missed close time. Closing door now...");
    closeDoor();
    doorClosedTonight = true;
  } else if (digitalRead(limitSwitchClosed) == LOW) {
    Serial.println("Boot: Door already closed.");
    doorClosedTonight = true;
  }
}

void loop() {
  checkWiFi();
  time_t nowTime = now();

  if (millis() - lastSunUpdate > timerDelay) {
    updateSunTimes();
    updateLCD();
    lastSunUpdate = millis();
    doorOpenedToday = false;
    doorClosedTonight = false;
  }

  if (!doorOpenedToday && nowTime >= localSunriseTime) {
    Serial.println("Auto: Opening door before sunrise.");
    openDoor();
    doorOpenedToday = true;
  }

  if (!doorClosedTonight && nowTime > localSunsetTime && digitalRead(limitSwitchClosed) == HIGH) {
    Serial.println("Auto: Closing door after sunset.");
    closeDoor();
    doorClosedTonight = true;
  }

  if (digitalRead(buttonOpen) == LOW) {
    Serial.println("Manual: Open button pressed.");
    openDoor();
    delay(500);
  }

  if (digitalRead(buttonClose) == LOW) {
    Serial.println("Manual: Close button pressed.");
    closeDoor();
    delay(500);
  }

  delay(100);
}

// ------ DOOR CONTROL ------

void openDoor() {
  if (digitalRead(limitSwitchOpen) == LOW) {
    Serial.println("Door already open.");
    doorState = "Open";
    updateLCD();
    stopMotor();
    return;
  }

  doorState = "Opening...";
  updateLCD();

  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  while (digitalRead(limitSwitchOpen) == HIGH) delay(10);

  stopMotor();
  doorState = "Open";
  updateLCD();
  Serial.println("Door opened.");
}

void closeDoor() {
  if (digitalRead(limitSwitchClosed) == LOW) {
    Serial.println("Door already closed.");
    doorState = "Closed";
    updateLCD();
    stopMotor();
    return;
  }

  doorState = "Closing...";
  updateLCD();

  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  while (digitalRead(limitSwitchClosed) == HIGH) delay(10);

  stopMotor();
  doorState = "Closed";
  updateLCD();
  Serial.println("Door closed.");
}

void stopMotor() {
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
}

// ------ WIFI / TIME / API ------

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 40) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    wifiConnected = true;
  } else {
    Serial.println("\nWiFi failed.");
    wifiConnected = false;
  }
}

void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost. Reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nReconnected.");
      wifiConnected = true;
      updateSunTimes();
      updateLCD();
    } else {
      Serial.println("\nReconnect failed.");
      wifiConnected = false;
    }
  }
}

void updateSunTimes() {
  if (WiFi.status() == WL_CONNECTED) {
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
    String jsonBuffer = httpGETRequest(url.c_str());
    JSONVar obj = JSON.parse(jsonBuffer);

    if (JSON.typeof(obj) == "undefined") {
      Serial.println("JSON parsing failed.");
      return;
    }

    time_t sunriseUTC = (time_t)(long)obj["sys"]["sunrise"];
    time_t sunsetUTC = (time_t)(long)obj["sys"]["sunset"];
    localSunriseTime = sunriseUTC + timeZoneOffset;
    localSunsetTime = sunsetUTC + timeZoneOffset;

    setTime((time_t)(long)obj["dt"] + timeZoneOffset);

    Serial.print("Sunrise: ");
    printDateTime(localSunriseTime);
    Serial.print("Sunset : ");
    printDateTime(localSunsetTime);
  } else {
    Serial.println("Can't update sun times. No WiFi.");
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);

  int responseCode = http.GET();
  String payload = "{}";

  if (responseCode > 0) {
    payload = http.getString();
  } else {
    Serial.print("HTTP error: ");
    Serial.println(responseCode);
  }

  http.end();
  return payload;
}

void printDateTime(time_t t) {
  char buf[20];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", year(t), month(t), day(t), hour(t), minute(t), second(t));
  Serial.println(buf);
}

void updateLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("WiFi: ");
  lcd.print(wifiConnected ? "Connected" : "FAIL");

  lcd.setCursor(0, 1);
  if (doorState == "Opening..." || doorState == "Closing...") {
    lcd.print(doorState);
  } else {
    char buf1[6], buf2[6];
    sprintf(buf1, "%02d:%02d", hour(localSunriseTime), minute(localSunriseTime));
    sprintf(buf2, "%02d:%02d", hour(localSunsetTime) - 12, minute(localSunsetTime));
    lcd.print(buf1);
    lcd.print("AM ");
    lcd.print(buf2);
    lcd.print("PM");
  }
}
