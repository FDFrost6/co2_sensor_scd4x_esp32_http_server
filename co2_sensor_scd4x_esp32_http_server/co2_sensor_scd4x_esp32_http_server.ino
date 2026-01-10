/*
 Arduino ESP32 WiFi Web Server for the Adafruit SCD4X and SCD30 CO2 sensors.
 Responds to http requests with prometheus.io syntax responses.

 # HELP ambient_temperature Ambient temperature minus compensation
 # TYPE ambient_temperature gauge
 ambient_temperature 26.52
 # HELP temperature Measured temperature
 # TYPE temperature gauge
 temperature 31.52
 # HELP ambient_humidity Ambient humidity
 # TYPE ambient_humidity gauge
 ambient_humidity 52.83
 # HELP co2 CO2
 # TYPE co2 gauge
 co2 670
 # HELP battery_voltage Battery voltage
 # TYPE battery_voltage gauge
 battery_voltage 3.15

 Based on:
 * ESP32 example code SimpleWiFiServer by Jan Hendrik Berlin
 * Sensirion I2C SCD4X/SCD30 example code exampleUsage Copyright (c) 2021, Sensirion AG
 * Sensirion Example8_SCD4x_BLE_Gadget_with_RHT
 * Sensirion Example2_SCD30_BLE_Gadget

 Written by Simon Loffler on invasion/survival day 26/1/2022
*/

#include "secrets.h"
#include "vpd.h"

// OTA Updates
#include <ArduinoOTA.h>
#define OTA_HOSTNAME "growcontroller"

// mDNS for local network discovery
#include <ESPmDNS.h>

// Firmware version
#define FIRMWARE_VERSION "2.0.0"

// Uncomment which sensor you're using
// #define USESCD30
#define USESCD4X

// === CANNABIS GROW CONFIGURATION ===
// Set to GrowStage::VEG or GrowStage::FLOWER
GrowStage currentGrowStage = GrowStage::VEG;

// Optional: Disable BLE to reduce complexity and memory usage
// Comment out the next line to enable BLE
#define DISABLE_BLE

// If on an ESP32-C3 set this
// int LED_BUILTIN = 13;

// if on an ESP32-S3 Adafruit Qt Py
// #define ESP32S3QTPY

// For generic ESP32
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#ifdef ESP32S3QTPY
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
#endif

// Task scheduler
#include <TaskScheduler.h>
void readSensorCallback();
Task readSensorTask(5000, -1, &readSensorCallback);  // Read sensor every 5 seconds
Scheduler runner;

#ifndef DISABLE_BLE
// BLE
#include "DataProvider.h"
#include "NimBLELibraryWrapper.h"
#include "Sensirion_Gadget_BLE.h"
NimBLELibraryWrapper lib;

#ifdef USESCD30
  // SCD30
  DataProvider provider(lib, DataType::T_RH_CO2_ALT);
#endif
#ifdef USESCD4X
  // SCD4X
  DataProvider provider(lib, DataType::T_RH_CO2);
#endif
#endif // DISABLE_BLE

#ifdef USESCD30
  // SCD30 sensor init
  #include <SensirionI2cScd30.h>
  SensirionI2cScd30 sensor;
#endif
#ifdef USESCD4X
  // SCD4X sensor init
  #include <SensirionI2cScd4x.h>
  SensirionI2cScd4x sensor;
#endif

#include <Arduino.h>
#include <Wire.h>
#include <time.h>

// LittleFS for data logging
#include <LittleFS.h>
#define FORMAT_LITTLEFS_IF_FAILED true

// Data logging configuration
#define LOG_INTERVAL 900000  // 15 minutes in milliseconds
#define MAX_DATA_POINTS 672  // 7 days * 24 hours * 4 (15-min intervals)
unsigned long lastLogTime = 0;
bool firstLogDone = false;

// Structure for sensor data
struct SensorData {
  unsigned long timestamp;
  float temperature;
  float humidity;
  uint16_t co2;
  float vpd;
  float plantAgeDays;
  bool lightOn;
};

uint16_t error;

#ifdef USESCD30
  // SCD30
  float co2;
  char errorMessage[128];
#endif
#ifdef USESCD4X
  // SCD4X
  uint16_t co2;
  char errorMessage[256];
#endif
float temperature;
float humidity;
float voltage;
float temperatureCompensation;
float temperatureOffset = 0.0;
float humidityOffset = 0.0;

// VPD (Vapor Pressure Deficit) variables for cannabis grow monitoring
float currentVpd = 0.0;           // Current VPD in kPa
VpdStatus currentVpdStatus = VpdStatus::OPTIMAL;  // VPD classification

// Plant timer variables
unsigned long plantStartTime = 0;  // Timestamp when plant timer started (0 = not started)
bool plantTimerActive = false;     // Whether plant timer is running

// Light schedule variables (hours in 24h format)
int lightOnHour = 6;    // Default: lights on at 6:00 AM
int lightOffHour = 22;  // Default: lights off at 10:00 PM
int timeZoneOffsetHours = 0;  // Set to your local timezone offset from UTC
int daylightOffsetHours = 0;  // Set to 1 if daylight saving time is active
bool timeSynced = false;

// Function to check if lights should be on based on schedule
bool isLightOn() {
  int currentHour = -1;
  if (timeSynced) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100)) {
      currentHour = timeinfo.tm_hour;
    }
  }
  if (currentHour < 0) {
    // Fallback to uptime-based hours if time isn't synced.
    currentHour = (millis() / 3600000) % 24;
  }
  
  if (lightOnHour < lightOffHour) {
    // Normal day schedule (e.g., 6:00 - 22:00)
    return (currentHour >= lightOnHour && currentHour < lightOffHour);
  } else {
    // Overnight schedule (e.g., 22:00 - 6:00)
    return (currentHour >= lightOnHour || currentHour < lightOffHour);
  }
}

// Task callback
void readSensorCallback() {
    // Read the voltage (ESP32-C3 plugged into laptop 4.2V reads 3342)
    // int sensorValue = analogRead(A2);
    // printToSerial((String)"Analog read: " + sensorValue);
    // voltage = sensorValue * (4.2 / 3342.0);

#ifdef USESCD30
    // Read the SCD30 CO2 sensor
    uint16_t data_ready = 0;
    sensor.getDataReady(data_ready);
    if (bool(data_ready)) {
        error = sensor.readMeasurementData(co2, temperature, humidity);
        if (error != NO_ERROR) {
            Serial.print("Error trying to execute readMeasurementData(): ");
            errorToString(error, errorMessage, sizeof errorMessage);
            Serial.println(errorMessage);
            return;
        }
        // Provide the sensor values for Tools -> Serial Monitor or Serial
        // Plotter
        Serial.print("CO2[ppm]:");
        Serial.print(co2);
        Serial.print("\t");
        Serial.print("Temperature[℃]:");
        Serial.print(temperature);
        Serial.print("\t");
        Serial.print("Humidity[%]:");
        Serial.println(humidity);
    }
#endif
#ifdef USESCD4X
    // Read the SCD4X CO2 sensor
    error = sensor.readMeasurement(co2, temperature, humidity);
    if (error) {
        printToSerial("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        printToSerial(errorMessage);
    } else if (co2 == 0) {
        printToSerial("Invalid sample detected, skipping.");
    } else {
        temperature = temperature + temperatureOffset;
        humidity = humidity + humidityOffset;
        if (humidity < 0.0) humidity = 0.0;
        if (humidity > 100.0) humidity = 100.0;
        
        printToSerial((String)"Co2: " + co2);
        printToSerial((String)"Temperature: " + temperature);
        printToSerial((String)"Humidity: " + humidity);
        printToSerial((String)"Voltage: " + voltage);
        
        // Calculate VPD for cannabis grow monitoring with plant age
        currentVpd = computeVPD(temperature, humidity);
        
        // Calculate plant age for VPD optimization
        float plantAge = -1;
        if (plantTimerActive && plantStartTime > 0) {
            plantAge = (millis() - plantStartTime) / 86400000.0;
        }
        
        currentVpdStatus = classifyVpd(currentVpd, currentGrowStage, plantAge);
        printToSerial((String)"VPD: " + currentVpd + " kPa (" + vpdStatusToString(currentVpdStatus) + ")");
        if (plantAge >= 0) {
            printToSerial((String)"Plant Age: " + plantAge + " days");
        }
        printToSerial("");
    }
#endif

#ifndef DISABLE_BLE
    // Report sensor readings via BLE
    provider.writeValueToCurrentSample(co2, SignalType::CO2_PARTS_PER_MILLION);
    provider.writeValueToCurrentSample(temperature, SignalType::TEMPERATURE_DEGREES_CELSIUS);
    provider.writeValueToCurrentSample(humidity, SignalType::RELATIVE_HUMIDITY_PERCENTAGE);
    provider.commitSample();
    provider.handleDownload();
#endif // DISABLE_BLE

    // Pulse blue LED
#ifdef ESP32S3QTPY
    pixels.setPixelColor(0, pixels.Color(0, 0, 10));
    pixels.show();
    pixels.clear();
    pixels.show();
#else
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
#endif
}

// Data logging functions
void logSensorData() {
  if (co2 == 0) return; // Skip invalid readings
  
  File file = LittleFS.open("/data.csv", "a");
  if (!file) {
    printToSerial("Failed to open data file for writing");
    return;
  }
  
  // Calculate plant age in days
  float plantAgeDays = 0.0;
  if (plantTimerActive && plantStartTime > 0) {
    plantAgeDays = (millis() - plantStartTime) / 86400000.0; // Convert ms to days
  }
  
  bool lightStatus = isLightOn();
  
  // Format: timestamp,temperature,humidity,co2,vpd,plantAgeDays,lightOn
  file.print(millis());
  file.print(",");
  file.print(temperature, 2);
  file.print(",");
  file.print(humidity, 2);
  file.print(",");
  file.print(co2);
  file.print(",");
  file.print(currentVpd, 2);
  file.print(",");
  file.print(plantAgeDays, 2);
  file.print(",");
  file.println(lightStatus ? "1" : "0");
  
  file.close();
  printToSerial("Data logged");
  
  // Check file size and trim if needed
  trimDataFile();
}

void trimDataFile() {
  File file = LittleFS.open("/data.csv", "r");
  if (!file) return;
  
  // Count lines
  int lineCount = 0;
  while (file.available()) {
    if (file.read() == '\n') lineCount++;
  }
  file.close();
  
  // If more than MAX_DATA_POINTS, remove oldest entries
  if (lineCount > MAX_DATA_POINTS) {
    File oldFile = LittleFS.open("/data.csv", "r");
    File newFile = LittleFS.open("/data_temp.csv", "w");
    
    int linesToSkip = lineCount - MAX_DATA_POINTS;
    int currentLine = 0;
    
    while (oldFile.available()) {
      String line = oldFile.readStringUntil('\n');
      currentLine++;
      if (currentLine > linesToSkip) {
        newFile.println(line);
      }
    }
    
    oldFile.close();
    newFile.close();
    
    LittleFS.remove("/data.csv");
    LittleFS.rename("/data_temp.csv", "/data.csv");
    printToSerial("Data file trimmed");
  }
}

void saveMetadata() {
  File file = LittleFS.open("/metadata.txt", "w");
  if (!file) {
    printToSerial("Failed to save metadata");
    return;
  }
  file.print("plantStartTime=");
  file.println(plantStartTime);
  file.print("plantTimerActive=");
  file.println(plantTimerActive ? "1" : "0");
  file.print("growStage=");
  file.println(currentGrowStage == GrowStage::VEG ? "VEG" : "FLOWER");
  file.print("lightOnHour=");
  file.println(lightOnHour);
  file.print("lightOffHour=");
  file.println(lightOffHour);
  file.close();
  printToSerial("Metadata saved");
}

void loadMetadata() {
  File file = LittleFS.open("/metadata.txt", "r");
  if (!file) {
    printToSerial("No metadata file found");
    return;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.startsWith("plantStartTime=")) {
      plantStartTime = line.substring(15).toInt();
    } else if (line.startsWith("plantTimerActive=")) {
      plantTimerActive = line.substring(17).toInt() == 1;
    } else if (line.startsWith("growStage=")) {
      String stage = line.substring(10);
      stage.trim();
      currentGrowStage = (stage == "FLOWER") ? GrowStage::FLOWER : GrowStage::VEG;
    } else if (line.startsWith("lightOnHour=")) {
      lightOnHour = line.substring(12).toInt();
    } else if (line.startsWith("lightOffHour=")) {
      lightOffHour = line.substring(13).toInt();
    }
  }
  file.close();
  printToSerial("Metadata loaded");
}

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}

void printToSerial(String message) {
  // Check for Serial to avoid the ESP32-C3 hanging attempting to Serial.print
  if (Serial) {
    Serial.println(message);
  }
}

// Helper function to send CORS headers for mobile app access
void sendCorsHeaders(WiFiClient& client) {
  client.println("Access-Control-Allow-Origin: *");
  client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");
  client.println("Access-Control-Allow-Headers: Content-Type");
}

// Helper function to send JSON response headers with CORS
void sendJsonHeaders(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: application/json; charset=UTF-8");
  sendCorsHeaders(client);
  client.println("Connection: close");
  client.println();
}

// WiFi init
#include <WiFi.h>
char* ssid     = SECRET_SSID;
char* password = SECRET_PASSWORD;
WiFiServer server(80);

void setup() {

    Serial.begin(115200);
    delay(100);

    pinMode(LED_BUILTIN, OUTPUT);

    // Initialize LittleFS
    printToSerial("Mounting LittleFS...");
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
      printToSerial("LittleFS Mount Failed");
    } else {
      printToSerial("LittleFS Mounted Successfully");
      // Print file system info
      printToSerial((String)"Total space: " + LittleFS.totalBytes() + " bytes");
      printToSerial((String)"Used space: " + LittleFS.usedBytes() + " bytes");
      
      // Load metadata (plant timer info)
      loadMetadata();
    }

    // Sensor setup
#ifdef ESP32S3QTPY
    #define SDA 41
    #define SCL 40
    #define WIRE Wire1
    Wire1.setPins(SDA, SCL);
    Wire1.begin(SDA, SCL);
#else
    Wire.begin();
#endif

    uint16_t error;

#ifdef USESCD30
    // SCD30
#ifdef ESP32S3QTPY
    sensor.begin(Wire1, SCD30_I2C_ADDR_61);
    temperatureCompensation = 5;
#else
    sensor.begin(Wire, SCD30_I2C_ADDR_61);
#endif
    char errorMessage[128];
#endif
#ifdef USESCD4X
    // SCD4X
    sensor.begin(Wire, SCD40_I2C_ADDR_62);
    char errorMessage[256];
#endif

    // stop potentially previously started measurement
    error = sensor.stopPeriodicMeasurement();
    if (error) {
      printToSerial("Error trying to execute stopPeriodicMeasurement(): ");
#ifdef USESCD30
      errorToString(error, errorMessage, 128);
#endif
#ifdef USESCD4X
      errorToString(error, errorMessage, 256);
#endif
      printToSerial(errorMessage);
    }
#ifdef USESCD30
    // SCD30
    sensor.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    error = sensor.readFirmwareVersion(major, minor);
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute readFirmwareVersion(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        //return;
    }
    Serial.print("firmware version major: ");
    Serial.print(major);
    Serial.print("\t");
    Serial.print("minor: ");
    Serial.print(minor);
    Serial.println();
    sensor.activateAutoCalibration(1);
#endif

#ifdef USESCD4X
    uint64_t serialNumber = 0;
    error = sensor.getSerialNumber(serialNumber);
    if (error) {
      printToSerial("Error trying to execute getSerialNumber(): ");
      errorToString(error, errorMessage, 256);
      printToSerial(errorMessage);
    } else {
      if (Serial) {
        Serial.print("Serial: 0x");
        Serial.println(serialNumber, HEX);
      }
    }
#endif

    // Start Measurement
#ifdef USESCD30
    // SCD30
    error = sensor.startPeriodicMeasurement(0);
#endif
#ifdef USESCD4X
    // SCD4X
    error = sensor.startPeriodicMeasurement();
#endif

    if (error) {
      printToSerial("Error trying to execute startPeriodicMeasurement(): ");
#ifdef USESCD30
      errorToString(error, errorMessage, 128);
#endif
#ifdef USESCD4X
      errorToString(error, errorMessage, 256);
#endif
      printToSerial(errorMessage);
    }

    printToSerial("Waiting for first measurement... (5 sec)");

    // Setup read sensor task
    readSensorTask.enable();
    runner.addTask(readSensorTask);
    runner.enableAll();

#ifndef DISABLE_BLE
    // Setup BLE
    provider.begin();
    printToSerial("Sensirion BLE Lib initialized with deviceId: " + provider.getDeviceIdString());
#endif

    // WiFi setup
#ifdef ESP32S3QTPY
    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(10, 0, 0));
    pixels.show();
    delay(500);
    pixels.clear();
    pixels.show();
#else
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
#endif

    delay(10);

    // Set WiFi power
    // Max: WIFI_POWER_19_5dBm ~150mA
    // Min: WIFI_POWER_MINUS_1dBm ~120mA
    // WiFi.setTxPower(WIFI_POWER_2dBm);

    // We start by connecting to a WiFi network
    printToSerial((String)"Connecting to " + ssid);

    WiFi.begin(ssid, password);

    // Wait for a WiFi connection for up to 10 seconds
    for (int i = 0; i < 10; i++) {
      if (WiFi.status() != WL_CONNECTED) {
#ifdef ESP32S3QTPY
        pixels.setPixelColor(0, pixels.Color(0, 0, 10));
        pixels.show();
        delay(500);
        pixels.clear();
        pixels.show();
#else
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
#endif
        printToSerial(".");
        delay(500);
      } else {
        printToSerial("WiFi connected.");
        printToSerial("IP address: ");
        printToSerial((String)WiFi.localIP());

        // Setup mDNS responder
        if (MDNS.begin(OTA_HOSTNAME)) {
          printToSerial("mDNS responder started: http://growcontroller.local");
          MDNS.addService("http", "tcp", 80);
          MDNS.addService("growcontroller", "tcp", 80);  // Custom service for app discovery
        } else {
          printToSerial("Error setting up mDNS responder!");
        }

        // Setup OTA Updates
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        ArduinoOTA.onStart([]() {
          String type;
          if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
          } else {
            type = "filesystem";
          }
          printToSerial("OTA Start updating " + type);
        });
        ArduinoOTA.onEnd([]() {
          printToSerial("\nOTA End");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
          Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
          Serial.printf("OTA Error[%u]: ", error);
          if (error == OTA_AUTH_ERROR) printToSerial("Auth Failed");
          else if (error == OTA_BEGIN_ERROR) printToSerial("Begin Failed");
          else if (error == OTA_CONNECT_ERROR) printToSerial("Connect Failed");
          else if (error == OTA_RECEIVE_ERROR) printToSerial("Receive Failed");
          else if (error == OTA_END_ERROR) printToSerial("End Failed");
        });
        ArduinoOTA.begin();
        printToSerial("OTA ready. Hostname: " + String(OTA_HOSTNAME));

        configTime(timeZoneOffsetHours * 3600, daylightOffsetHours * 3600, "pool.ntp.org", "time.nist.gov");
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 10000)) {
          timeSynced = true;
          printToSerial("Time synced via NTP.");
        } else {
          printToSerial("NTP time sync failed. Using uptime-based clock.");
        }

#ifdef ESP32S3QTPY
        pixels.setPixelColor(0, pixels.Color(0, 10, 0));
        pixels.show();
        delay(500);
        pixels.clear();
        pixels.show();
#else
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
#endif
        break;
      }
    }
    server.begin();
    printToSerial("HTTP server started on port 80");
    printToSerial("Firmware version: " + String(FIRMWARE_VERSION));
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();
  
  runner.execute();

  // Log first data point immediately after 30 seconds
  if (!firstLogDone && millis() > 30000 && co2 > 0) {
    logSensorData();
    lastLogTime = millis();
    firstLogDone = true;
    printToSerial("First data point logged");
  }
  
  // Log data every 15 minutes
  if (firstLogDone && millis() - lastLogTime >= LOG_INTERVAL) {
    logSensorData();
    lastLogTime = millis();
  }

  // WiFi server
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    printToSerial("New Client.");           // print a message out the serial port
    printToSerial("");
    String currentLine = "";                // make a String to hold incoming data from the client
    String requestPath = "";                 // Track the HTTP request path
    bool headersParsed = false;
    
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // printToSerial("" + c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // Pulse the LED to show a connection has been made
#ifdef ESP32S3QTPY
            pixels.setPixelColor(0, pixels.Color(0, 10, 0));
            pixels.show();
#else
            digitalWrite(LED_BUILTIN, HIGH);
#endif

            // Route based on request path
            if (requestPath.indexOf("/timer/start") >= 0) {
              // === START PLANT TIMER ===
              plantStartTime = millis();
              plantTimerActive = true;
              saveMetadata();
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println();
            } else if (requestPath.indexOf("/timer/stop") >= 0) {
              // === STOP PLANT TIMER ===
              plantTimerActive = false;
              saveMetadata();
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println();
            } else if (requestPath.indexOf("/timer/reset") >= 0) {
              // === RESET PLANT TIMER ===
              plantStartTime = millis();
              plantTimerActive = true;
              saveMetadata();
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println();
            } else if (requestPath.indexOf("/stage/veg") >= 0) {
              // === SET VEG STAGE ===
              currentGrowStage = GrowStage::VEG;
              saveMetadata();
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println();
            } else if (requestPath.indexOf("/stage/flower") >= 0) {
              // === SET FLOWER STAGE ===
              currentGrowStage = GrowStage::FLOWER;
              saveMetadata();
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println();
            } else if (requestPath.indexOf("/light/set") >= 0) {
              // === SET LIGHT SCHEDULE ===
              // Parse parameters from URL: /light/set?on=6&off=22
              int onIdx = requestPath.indexOf("on=");
              int offIdx = requestPath.indexOf("off=");
              if (onIdx >= 0 && offIdx >= 0) {
                String onStr = requestPath.substring(onIdx + 3, requestPath.indexOf('&', onIdx));
                String offStr = requestPath.substring(offIdx + 4);
                int spaceIdx = offStr.indexOf(' ');
                if (spaceIdx >= 0) offStr = offStr.substring(0, spaceIdx);
                
                int newOnHour = onStr.toInt();
                int newOffHour = offStr.toInt();
                
                if (newOnHour >= 0 && newOnHour <= 23 && newOffHour >= 0 && newOffHour <= 23) {
                  lightOnHour = newOnHour;
                  lightOffHour = newOffHour;
                  saveMetadata();
                }
              }
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println();
            } else if (requestPath.indexOf("/export") >= 0) {
              // === CSV EXPORT ENDPOINT (Download data as CSV file) ===
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/csv");
              client.println("Content-Disposition: attachment; filename=co2_sensor_data.csv");
              client.println();
              
              // CSV Header
              client.println("Timestamp,Date Time,Plant Age (days),Temperature (C),Humidity (%),CO2 (ppm),VPD (kPa),Light On");
              
              File file = LittleFS.open("/data.csv", "r");
              if (file) {
                while (file.available()) {
                  String line = file.readStringUntil('\n');
                  if (line.length() > 0) {
                    // Parse CSV: timestamp,temperature,humidity,co2,vpd,plantAgeDays
                    int idx1 = line.indexOf(',');
                    unsigned long timestamp = line.substring(0, idx1).toInt();
                    
                    // Convert timestamp to readable date/time
                    unsigned long seconds = timestamp / 1000;
                    unsigned long days = seconds / 86400;
                    unsigned long hours = (seconds % 86400) / 3600;
                    unsigned long minutes = (seconds % 3600) / 60;
                    unsigned long secs = seconds % 60;
                    
                    client.print(timestamp);
                    client.print(",");
                    client.print(days);
                    client.print("d ");
                    if (hours < 10) client.print("0");
                    client.print(hours);
                    client.print(":");
                    if (minutes < 10) client.print("0");
                    client.print(minutes);
                    client.print(":");
                    if (secs < 10) client.print("0");
                    client.print(secs);
                    client.print(",");
                    client.println(line.substring(idx1 + 1)); // Rest of the CSV data
                  }
                }
                file.close();
              }
            } else if (requestPath.indexOf("/data") >= 0) {
              // === DATA ENDPOINT (CSV historical data) ===
              sendJsonHeaders(client);
              
              client.print("[");
              File file = LittleFS.open("/data.csv", "r");
              if (file) {
                bool first = true;
                while (file.available()) {
                  String line = file.readStringUntil('\n');
                  if (line.length() > 0) {
                    if (!first) client.print(",");
                    first = false;
                    
                    // Parse CSV: timestamp,temperature,humidity,co2,vpd
                    int idx1 = line.indexOf(',');
                    int idx2 = line.indexOf(',', idx1 + 1);
                    int idx3 = line.indexOf(',', idx2 + 1);
                    int idx4 = line.indexOf(',', idx3 + 1);
                    
                    client.print("{\"t\":");
                    client.print(line.substring(0, idx1));
                    client.print(",\"temp\":");
                    client.print(line.substring(idx1 + 1, idx2));
                    client.print(",\"hum\":");
                    client.print(line.substring(idx2 + 1, idx3));
                    client.print(",\"co2\":");
                    client.print(line.substring(idx3 + 1, idx4));
                    client.print(",\"vpd\":");
                    client.print(line.substring(idx4 + 1));
                    client.print("}");
                  }
                }
                file.close();
              }
              client.print("]");
            } else if (requestPath.indexOf("/api/info") >= 0) {
              // === DEVICE INFO ENDPOINT (for mobile app discovery) ===
              sendJsonHeaders(client);
              
              // Calculate plant age
              float plantAgeDays = 0.0;
              if (plantTimerActive && plantStartTime > 0) {
                plantAgeDays = (millis() - plantStartTime) / 86400000.0;
              }
              
              client.print("{");
              client.print("\"device_name\":\"GrowController\",");
              client.print("\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\",");
              client.print("\"hostname\":\"" + String(OTA_HOSTNAME) + "\",");
              client.print((String)"\"ip_address\":\"" + WiFi.localIP().toString() + "\",");
              client.print((String)"\"mac_address\":\"" + WiFi.macAddress() + "\",");
              client.print((String)"\"rssi\":" + WiFi.RSSI() + ",");
              client.print((String)"\"uptime_ms\":" + millis() + ",");
              client.print((String)"\"free_heap\":" + ESP.getFreeHeap() + ",");
              client.print("\"sensor_type\":\"SCD4X\",");
              client.print((String)"\"plant_timer_active\":" + (plantTimerActive ? "true" : "false") + ",");
              client.print((String)"\"plant_age_days\":" + plantAgeDays + ",");
              client.print((String)"\"light_on_hour\":" + lightOnHour + ",");
              client.print((String)"\"light_off_hour\":" + lightOffHour + ",");
              client.print((String)"\"time_synced\":" + (timeSynced ? "true" : "false"));
              client.print("}\n");
            } else if (requestPath.indexOf("/status") >= 0) {
              // === JSON STATUS ENDPOINT ===
              sendJsonHeaders(client);
              
              // Calculate plant age for status response
              float plantAgeDays = 0.0;
              if (plantTimerActive && plantStartTime > 0) {
                plantAgeDays = (millis() - plantStartTime) / 86400000.0;
              }
              
              // Get optimal VPD range for current stage and age
              VpdRange optimalRange = getVpdRangeForStage(currentGrowStage, plantAgeDays);
              
              client.print("{");
              client.print((String)"\"temperature_c\":" + temperature + ",");
              client.print((String)"\"ambient_temperature_c\":" + (temperature - temperatureCompensation) + ",");
              client.print((String)"\"humidity_percent\":" + humidity + ",");
              client.print((String)"\"co2_ppm\":" + co2 + ",");
              client.print((String)"\"vpd_kpa\":" + currentVpd + ",");
              client.print((String)"\"vpd_status\":\"" + vpdStatusToString(currentVpdStatus) + "\",");
              client.print((String)"\"vpd_min\":" + optimalRange.min_kPa + ",");
              client.print((String)"\"vpd_max\":" + optimalRange.max_kPa + ",");
              client.print((String)"\"grow_stage\":\"" + growStageToString(currentGrowStage) + "\",");
              client.print((String)"\"plant_timer_active\":" + (plantTimerActive ? "true" : "false") + ",");
              client.print((String)"\"plant_age_days\":" + plantAgeDays + ",");
              client.print((String)"\"light_on\":" + (isLightOn() ? "true" : "false") + ",");
              client.print((String)"\"battery_voltage\":" + voltage + ",");
              client.print("\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\"");
              client.print("}\n");
            } else if (requestPath == "/" || requestPath.indexOf("/dashboard") >= 0) {
              // === HTML DASHBOARD - TERMINAL STYLE ===
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              client.print("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
              client.print("<title>CO2 Control Center</title>");
              client.print("<style>");
              client.print("body{font-family:Consolas,monospace;background:#0d1117;color:#00ff66;margin:0;padding:20px}");
              client.print(".hdr{border:1px solid #00ff66;padding:15px;margin-bottom:20px;background:#1a1e24;box-shadow:0 0 10px rgba(0,255,102,0.5)}");
              client.print(".hdr h1{font-size:2em;margin:0 0 5px;color:#fff;text-shadow:0 0 5px #00ff66}");
              client.print(".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px;margin-bottom:20px}");
              client.print(".grid2{display:grid;grid-template-columns:1fr 2fr;gap:20px;margin-bottom:20px}");
              client.print(".card{background:#22272e;border:2px solid #373e47;padding:20px;text-align:center;margin-bottom:0}");
              client.print(".chart{height:220px}");
              client.print(".axis-labels{display:flex;justify-content:space-between;color:#8b949e;font-size:0.9em;margin-top:6px}");
              client.print("canvas{width:100%;height:180px;background:#161a20;border:1px solid #373e47}");
              client.print(".card h2{font-size:1em;color:#ccc;text-transform:uppercase;border-bottom:1px dashed #373e47;padding-bottom:10px;margin:0 0 15px}");
              client.print(".val{font-size:4em;font-weight:bold;color:#fff;text-shadow:0 0 10px rgba(0,255,102,0.8);line-height:1}");
              client.print(".unit{font-size:1.2em;color:#00ff66;margin-top:5px}");
              client.print(".vpd .val{color:#ffcc00}");
              client.print(".co2 .val{color:#00c3ff}");
              client.print(".plant{text-align:left}");
              client.print(".info{margin-top:15px;font-size:1.3em;line-height:2.2}");
              client.print(".info p{margin:0;color:#ccc;display:flex;justify-content:space-between;border-bottom:1px dashed #373e47;padding-bottom:5px}");
              client.print(".info span{color:#fff;font-weight:bold}");
              client.print(".rec{border:3px solid #ffcc00;padding:15px;margin-top:15px;text-align:center;background:rgba(255,204,0,0.05)}");
              client.print(".rec-ok{border-color:#00ff66}");
              client.print(".rec-bad{border-color:#ff004c}");
              client.print(".icon{font-size:2.5em;margin-bottom:5px}");
              client.print(".msg{font-size:1.3em;color:#fff;margin:5px 0}");
              client.print(".act{font-size:1.1em;font-weight:bold}");
              client.print(".ok{color:#00ff66}");
              client.print(".bad{color:#ff004c}");
              client.print(".btn{display:inline-block;background:#51CF66;color:#fff;padding:10px 20px;border-radius:8px;text-decoration:none;font-weight:600;margin:10px 5px}");
              client.print(".btn-start{background:#00ff66;color:#000}");
              client.print(".btn-stop{background:#ff004c}");
              client.print(".btn-veg{background:#00c3ff}");
              client.print(".btn-flower{background:#ff6b9d}");
              client.print("@media(max-width:768px){.grid2{grid-template-columns:1fr}.val{font-size:3em}}");
              client.print("</style></head><body>");
              client.print("<div class='hdr'><h1>// CANNABIS GROW MONITORING SYSTEM [V1.0]</h1>");
              client.print("<p style='font-size:0.9em;color:#ff004c;margin:5px 0 0'>STATUS: ONLINE // IP: ");
              client.print(WiFi.localIP());
              client.print("</p></div>");
              client.print("<div class='grid'>");
              client.print("<div class='card temp'><h2>TEMPERATURE</h2><div class='val' id='tempValue'>");
              client.print(temperature);
              client.print("</div><span class='unit'>&deg;C</span></div>");
              client.print("<div class='card hum'><h2>HUMIDITY</h2><div class='val' id='humValue'>");
              client.print(humidity);
              client.print("</div><span class='unit'>%</span></div>");
              client.print("<div class='card vpd'><h2>VPD</h2><div class='val' id='vpdValue'>");
              client.print(currentVpd);
              client.print("</div><span class='unit'>kPa</span></div>");
              client.print("<div class='card co2'><h2>CO2</h2><div class='val' id='co2Value'>");
              client.print(co2);
              client.print("</div><span class='unit'>PPM</span></div>");
              client.print("</div>");
              client.print("<div class='grid2'>");
              client.print("<div class='card plant'><h2>PLANT STATUS</h2><div class='info'>");
              client.print("<p>Growth Phase: <span style='color:#00ff66' id='stageValue'>");
              client.print(growStageToString(currentGrowStage));
              client.print("</span></p>");
              
              // Calculate and display plant age
              if (plantTimerActive && plantStartTime > 0) {
                float plantAgeDays = (millis() - plantStartTime) / 86400000.0;
                client.print("<p>Plant Age: <span id='plantAgeValue'>");
                client.print(plantAgeDays, 1);
                client.print(" days</span></p>");
              } else {
                client.print("<p>Plant Age: <span id='plantAgeValue'>Not Started</span></p>");
              }
              
              client.print("<p>Uptime: <span>");
              unsigned long uptimeSeconds = millis() / 1000;
              unsigned long days = uptimeSeconds / 86400;
              unsigned long hours = (uptimeSeconds % 86400) / 3600;
              client.print(days);
              client.print("d ");
              client.print(hours);
              client.print("h</span></p></div></div>");
              client.print("<div class='card'><h2>VPD RECOMMENDATIONS</h2><div class='rec ");
              if (currentVpdStatus == VpdStatus::OPTIMAL) {
                client.print("rec-ok' id='vpdRec'><span class='icon ok' id='vpdRecIcon'>&#9989;</span><p class='msg' id='vpdRecMsg'>VPD OPTIMAL</p><p class='act ok' id='vpdRecAct'>Continue monitoring</p>");
              } else if (currentVpdStatus == VpdStatus::TOO_LOW) {
                client.print("rec-bad' id='vpdRec'><span class='icon bad' id='vpdRecIcon'>&#10060;</span><p class='msg' id='vpdRecMsg'>VPD TOO LOW</p><p class='act bad' id='vpdRecAct'>INCREASE TEMP OR DECREASE HUMIDITY</p>");
              } else {
                client.print("rec-bad' id='vpdRec'><span class='icon bad' id='vpdRecIcon'>&#9888;</span><p class='msg' id='vpdRecMsg'>VPD TOO HIGH</p><p class='act bad' id='vpdRecAct'>INCREASE HUMIDITY OR DECREASE TEMP</p>");
              }
              client.print("</div></div></div>");
              client.print("<div class='card chart'><h2>VPD TREND (LIVE)</h2><canvas id='vpdChart' width='600' height='180'></canvas>");
              client.print("<div class='axis-labels'><span>VPD (kPa)</span><span>Time</span></div></div>");
              client.print("<div class='card'><h2>PLANT TIMER CONTROL</h2>");
              if (!plantTimerActive) {
                client.print("<a href='/timer/start' class='btn btn-start'>START TIMER</a>");
              } else {
                client.print("<a href='/timer/stop' class='btn btn-stop'>STOP TIMER</a>");
                client.print("<a href='/timer/reset' class='btn btn-stop'>RESET</a>");
              }
              client.print("<br><a href='/stage/veg' class='btn btn-veg'>SET VEG</a>");
              client.print("<a href='/stage/flower' class='btn btn-flower'>SET FLOWER</a></div>");
              
              // Light schedule control
              client.print("<div class='card'><h2>LIGHT SCHEDULE</h2>");
              client.print("<p style='color:#ccc;margin-bottom:10px'>Lights: ");
              if (isLightOn()) {
                client.print("<span style='color:#00ff66;font-weight:bold'>ON</span>");
              } else {
                client.print("<span style='color:#ff004c;font-weight:bold'>OFF</span>");
              }
              client.print("</p>");
              client.print("<p style='color:#ccc;margin-bottom:10px'>Schedule: ");
              if (lightOnHour < 10) client.print("0");
              client.print(lightOnHour);
              client.print(":00 - ");
              if (lightOffHour < 10) client.print("0");
              client.print(lightOffHour);
              client.print(":00</p>");
              client.print("<form method='GET' action='/light/set' style='margin-top:10px'>");
              client.print("<label style='color:#ccc;font-size:0.9em'>ON:</label>");
              client.print("<input type='number' name='on' min='0' max='23' value='");
              client.print(lightOnHour);
              client.print("' style='width:60px;margin:0 10px;padding:5px'>");
              client.print("<label style='color:#ccc;font-size:0.9em'>OFF:</label>");
              client.print("<input type='number' name='off' min='0' max='23' value='");
              client.print(lightOffHour);
              client.print("' style='width:60px;margin:0 10px;padding:5px'>");
              client.print("<button type='submit' class='btn' style='margin-left:10px'>SET</button>");
              client.print("</form></div>");
              client.print("<div class='card'><a href='/export' class='btn'>DOWNLOAD CSV</a><br>");
              client.print("<p style='margin-top:15px;color:#ccc'>LOGGING: 15min | <a href='/data' style='color:#00ff66'>Data</a> | <a href='/status' style='color:#00ff66'>JSON</a></p></div>");
              client.print("<script>");
              client.print("const vpdPoints=[];");
              client.print("const maxPoints=120;");
              client.print("const canvas=document.getElementById('vpdChart');");
              client.print("const ctx=canvas.getContext('2d');");
              client.print("function updateRec(status){");
              client.print("const rec=document.getElementById('vpdRec');");
              client.print("const msg=document.getElementById('vpdRecMsg');");
              client.print("const act=document.getElementById('vpdRecAct');");
              client.print("const icon=document.getElementById('vpdRecIcon');");
              client.print("rec.classList.remove('rec-ok','rec-bad');");
              client.print("if(status==='optimal'){rec.classList.add('rec-ok');icon.className='icon ok';icon.textContent='\\u2705';msg.textContent='VPD OPTIMAL';act.className='act ok';act.textContent='Continue monitoring';}");
              client.print("else if(status==='too_low'){rec.classList.add('rec-bad');icon.className='icon bad';icon.textContent='\\u274C';msg.textContent='VPD TOO LOW';act.className='act bad';act.textContent='INCREASE TEMP OR DECREASE HUMIDITY';}");
              client.print("else{rec.classList.add('rec-bad');icon.className='icon bad';icon.textContent='\\u26A0';msg.textContent='VPD TOO HIGH';act.className='act bad';act.textContent='INCREASE HUMIDITY OR DECREASE TEMP';}");
              client.print("}");
              client.print("function drawChart(){");
              client.print("const w=canvas.width,h=canvas.height;");
              client.print("ctx.clearRect(0,0,w,h);");
              client.print("ctx.fillStyle='#8b949e';ctx.font='12px Consolas,monospace';");
              client.print("ctx.save();ctx.translate(10,h/2);ctx.rotate(-Math.PI/2);ctx.fillText('VPD (kPa)',0,0);ctx.restore();");
              client.print("ctx.fillText('Time',w-40,h-6);");
              client.print("ctx.strokeStyle='#373e47';ctx.lineWidth=1;");
              client.print("for(let i=1;i<=3;i++){const y=(h/4)*i;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}");
              client.print("if(vpdPoints.length<2)return;");
              client.print("let min=Infinity,max=-Infinity;for(const p of vpdPoints){if(p.v<min)min=p.v;if(p.v>max)max=p.v;}"); 
              client.print("if(min===max){min-=0.1;max+=0.1;}");
              client.print("ctx.strokeStyle='#ffcc00';ctx.lineWidth=2;ctx.beginPath();");
              client.print("vpdPoints.forEach((p,i)=>{const x=i*(w/(maxPoints-1));const y=h-((p.v-min)/(max-min))*h; if(i===0)ctx.moveTo(x,y); else ctx.lineTo(x,y);});");
              client.print("ctx.stroke();");
              client.print("}");
              client.print("async function fetchStatus(){");
              client.print("try{const res=await fetch('/status');if(!res.ok)return;const s=await res.json();");
              client.print("document.getElementById('tempValue').textContent=s.temperature_c.toFixed(2);");
              client.print("document.getElementById('humValue').textContent=s.humidity_percent.toFixed(2);");
              client.print("document.getElementById('vpdValue').textContent=s.vpd_kpa.toFixed(2);");
              client.print("document.getElementById('co2Value').textContent=Math.round(s.co2_ppm);");
              client.print("document.getElementById('stageValue').textContent=s.grow_stage;");
              client.print("updateRec(s.vpd_status);");
              client.print("vpdPoints.push({t:Date.now(),v:s.vpd_kpa});if(vpdPoints.length>maxPoints)vpdPoints.shift();");
              client.print("drawChart();}catch(e){}");
              client.print("}");
              client.print("fetchStatus();setInterval(fetchStatus,5000);");
              client.print("</script></body></html>");
            } else if (requestPath.indexOf("/metrics") >= 0) {
              // === PROMETHEUS METRICS ENDPOINT (default) ===
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain; charset=UTF-8");
              client.println();
              
              // Original Prometheus metrics
              client.print("# HELP ambient_temperature Ambient temperature\n");
              client.print("# TYPE ambient_temperature gauge\n");
              client.print((String)"ambient_temperature " + (temperature - temperatureCompensation)  + "\n");
              client.print("# HELP temperature Measured temperature\n");
              client.print("# TYPE temperature gauge\n");
              client.print((String)"temperature " + temperature + "\n");
              client.print("# HELP ambient_humidity Ambient humidity\n");
              client.print("# TYPE ambient_humidity gauge\n");
              client.print((String)"ambient_humidity " + humidity + "\n");
              client.print("# HELP co2 CO2 concentration in ppm\n");
              client.print("# TYPE co2 gauge\n");
              client.print((String)"co2 " + co2 + "\n");
              client.print("# HELP battery_voltage Battery voltage\n");
              client.print("# TYPE battery_voltage gauge\n");
              client.print((String)"battery_voltage " + voltage + "\n");
              
              // Cannabis VPD metrics
              client.print("# HELP vpd_kpa Vapor Pressure Deficit in kPa\n");
              client.print("# TYPE vpd_kpa gauge\n");
              client.print((String)"vpd_kpa " + currentVpd + "\n");
              client.print("# HELP vpd_status VPD status classification (-1=too_low, 0=optimal, 1=too_high)\n");
              client.print("# TYPE vpd_status gauge\n");
              client.print((String)"vpd_status{stage=\"" + growStageToString(currentGrowStage) + "\",status=\"" + vpdStatusToString(currentVpdStatus) + "\"} " + vpdStatusToNumeric(currentVpdStatus) + "\n");
            }

#ifdef ESP32S3QTPY
            pixels.clear();
            pixels.show();
#else
            digitalWrite(LED_BUILTIN, LOW);
#endif

            // The HTTP response ends with another blank line:
            client.print("\n");
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            // Parse the first line to get the HTTP request path
            if (!headersParsed && currentLine.startsWith("GET ")) {
              int pathStart = 4;  // After "GET "
              int pathEnd = currentLine.indexOf(" ", pathStart);
              if (pathEnd > pathStart) {
                requestPath = currentLine.substring(pathStart, pathEnd);
                printToSerial((String)"Request path: " + requestPath);
              }
              headersParsed = true;
            }
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    printToSerial("Client Disconnected.");
  }
}
