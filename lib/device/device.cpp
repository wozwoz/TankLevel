#include "mqtt.h"
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "device.h"

#include <Adafruit_BMP280.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "../telnet/telnet.h"

#define On_Board_LED_PIN 2
#define SampleTime 60000
#define ONE_WIRE_BUS 4
#define INPIN 35

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

Device::Device() {}

Device device;

const int sensorIn = 34; // pin where the OUT pin from sensor is connected on Arduino
#define SampleCount 16

// Define static member variables
bool Device::payloadReady = false;
char Device::globalBuf[256] = {0};

void Device::setup()
{
    static const char *subscription_list[] = {
        "beacon"};
    mqtt.set_subscriptions(subscription_list, 1);
    mqtt.set_callback(message_handler);

    // Publish initial status
    mqtt.publish("tanklevel/status", "online");
    mqtt.publish("tanklevel/ota/state", "ready");

    // if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
    if (!bmp.begin())
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                         "try a different address!"));
    }
    else
        _BMP_Found = true;

    /* Default settings from datasheet. */
    bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}
void Device::TestData()
{
    telnet.println("Adding Testing data to Device for Data Readings...");
    _InsideTemp = 25.5;
    _OutsideTemp = 25.5;
    _Pressure = 1013.25;
    _Down = true;
}
void Device::loop()
{
    // Generate sensor data
    if(millis() < _SampleTime) {
        return;
    }
    
    _SampleTime = millis() + SampleTime;

    int rssi = WiFi.RSSI(); // Get WiFi signal strength

    if (_BMP_Found && bmp.takeForcedMeasurement())
    {
        // can now print out the new measurements
        Serial.print(F("Temperature = "));
        _InsideTemp = bmp.readTemperature();
        Serial.print(_InsideTemp);
        Serial.println(" *C");

        Serial.print(F("Pressure = "));
        _Pressure = bmp.readPressure() / 100 + 76.66;
        Serial.print(_Pressure); //- 1014.0 937.73.34 );
        Serial.println(" hPa");

        Serial.print(F("Approx altitude = "));
        Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! */
        Serial.println(" m");

        Serial.println();
    }
    else
    {
        Serial.println("Forced measurement failed!");
        // load test data
        TestData();
    }

    int AverageTankLevel = GetAverageTankLevel();
    // Build JSON payload using ArduinoJson
    JsonDocument doc;
    JsonDocument subdoc;
    doc["outsidetemperature"] = ReadTemp();
    doc["insidetemperature"] = _InsideTemp;
    doc["pressure"] = _Pressure;

    doc["tanklevel"] = AverageTankLevel;

    subdoc["rssi"] = rssi;
    subdoc["uptime"] = millis() / 1000;
    subdoc["down"] = IsDown() ? "offline" : "online";
    subdoc["ip"] = WiFi.localIP().toString();
    doc["subdata"] = subdoc;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    // Publish JSON to single topic
    mqtt.publish("tanklevel/sensors", jsonBuffer);
    Serial.println("Published sensor data to MQTT:");
    delay(2000);
    if (payloadReady)
    {
        telnet.print("\tprocessed payload: ");
        telnet.println(globalBuf);
        payloadReady = false;
    }
}

void Device::message_handler(char *topic, char *payload)
{
    telnet.print("\treceived topic: ");
    telnet.print(topic);
    telnet.print(" / payload: ");
    telnet.println(payload);

    // Handle OTA update trigger
    if (strcmp(payload, "OTA_UPDATE") == 0 || strcmp(payload, "UPDATE") == 0)
    {
        telnet.println("\tOTA Update triggered via MQTT");
        mqtt.publish("tanklevel/ota/state", "updating");
        telnet.println("\tWaiting for OTA upload...");
        telnet.println("\tDevice is ready for OTA updates");
        mqtt.publish("tanklevel/ota/state", "ready");
        return;
    }

    strcpy(globalBuf, payload);
    payloadReady = true;
}
bool Device::IsDown()
{
    return _Down;
}
/////////////////////////////////////////////////////////////////

int Device::GetTankLevel()
{
    // read the analog / millivolts value for pin 2:
    int analogVolts = analogReadMilliVolts(INPIN);
    long tanklevel = map(analogVolts, 600, 2840, 0, 1550);

    return tanklevel;
}
int Device::GetAverageTankLevel()
{
    int Average = 0;
    for (int i = 0; i < SampleCount; i++)
    {
        Average += GetTankLevel();
        delay(100); // delay(10 seconds);
    }
    // print out the values you read:
    telnet.println("ADC millivolts value = " + String(Average));

    return Average / SampleCount;
}
float Device::ReadTemp(void)
{
    float rv = 0.0;
    // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
    sensors.requestTemperatures();
    telnet.print("Celsius temperature: ");
    // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
    rv = sensors.getTempCByIndex(0);
    _OutsideTemp = rv;
    telnet.println(String(rv));
    return rv;
}