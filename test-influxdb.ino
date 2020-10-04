#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>

#include <BME280Spi.h>
#include <BME280I2C.h>
#include <BME280SpiSw.h>
#include <BME280I2C_BRZO.h>
#include <EnvironmentCalculations.h>
#include <BME280.h>
#include <Wire.h>



// WiFi AP SSID
#define WIFI_SSID "TIM-46822155"
// WiFi password
#define WIFI_PASSWORD "gSQ2Ec7j5c56f8t2"

// InfluxDB  server url. Don't use localhost, always server name or ip address.
#define INFLUXDB_URL "http://myserver.local:8086"
// InfluxDB v1 database name 
#define INFLUXDB_DB_NAME "testweathersensor"
#define INFLUXDB_USER "testweathersensor"
#define INFLUXDB_PASSWORD "testweathersensor"

#define SENSOR_NAME "test-interno"

// InfluxDB client instance
// InfluxDB client instance for InfluxDB 1
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

// Data point
Point sensor(SENSOR_NAME);

#define SERIAL_BAUD 115200

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

const int sleepTimeS = 300;

void setup() {
  Serial.begin(SERIAL_BAUD);

  while(!Serial) {} // Wait

  wiFiSetup();

  influxDbSetup();

  bmeSetup();

  readSensorAndWriteInfluxdb();
}

void readSensorAndWriteInfluxdb() {
  float temp(NAN), hum(NAN), pres(NAN);
  
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  
  bme.read(pres, temp, hum, tempUnit, presUnit);

  // Store measured value into point
  sensor.clearFields();
  sensor.addField("temperature", temp);
  sensor.addField("preasure", pres);
  sensor.addField("humidity", hum);
  
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());
  // If no Wifi signal, try to reconnect it
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED))
    Serial.println("Wifi connection lost");
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  ESP.deepSleep(sleepTimeS * 1000000);
}

void loop() {
}

void wiFiSetup() {
  // Connect WiFi
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

void influxDbSetup() {
  // Set InfluxDB 1 authentication params
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void bmeSetup() {
  Wire.begin();

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch(bme.chipModel()) {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
}
