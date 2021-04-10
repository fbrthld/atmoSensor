#include <Wire.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>


/* should contain
const char *ssid = "your network ssid";
const char *password = "your network password";
const char *hostname = "hostname of this device";
const char *mqttHostname = "mqtt hostname or address";
 */
#include "secret.h"


#define SEND_INTERVAL 5 * 60 * 1000
#define TEMP_COMPENSATION -2.5


unsigned long lastMillis = - SEND_INTERVAL; // send once at start

WiFiClient net;
MQTTClient client;

Adafruit_BME280 bme;
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();


void messageReceived(String &topic, String &payload) {

    // Note: Do not use the client in the callback to publish, subscribe or
    // unsubscribe as it may cause deadlocks when other things arrive while
    // sending and receiving acknowledgments. Instead, change a global variable,
    // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void connectWiFi() {

    delay(10);

    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
    WiFi.hostname(hostname);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    randomSeed(micros());

}

void connectMqtt() {
    delay(10);

    client.begin(mqttHostname, net);
    client.onMessage(messageReceived);

    while (!client.connect("terrariumAtmoSensor")) {
        delay(1000);
    }
}

void setupSensor() {
    Wire.begin(0, 2);

    if (!bme.begin(0x76, &Wire)) {
        client.publish("error", "Could not find a valid BME280 sensor, check wiring!");
        while (1) delay(10);
    }

    bme.setTemperatureCompensation(TEMP_COMPENSATION);
}

void setup() {
    connectWiFi();
    connectMqtt();



    setupSensor();
}

void loop() {
    client.loop();

    if (millis() - lastMillis > SEND_INTERVAL) {
        lastMillis = millis();

        sensors_event_t temp_event, pressure_event, humidity_event;
        bme_temp->getEvent(&temp_event);
        bme_pressure->getEvent(&pressure_event);
        bme_humidity->getEvent(&humidity_event);

        StaticJsonDocument<200> json;
        json["temperature"] = temp_event.temperature;
        json["pressure"] = pressure_event.pressure;
        json["humidity"] = humidity_event.relative_humidity;

        char jsonStr[200];
        serializeJson(json, jsonStr);

        client.publish("terrarium/atmo", jsonStr);
    }
    delay(5000);
}



