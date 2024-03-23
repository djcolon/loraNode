/*
  Code Base from RadioLib: https://github.com/jgromes/RadioLib/tree/master/examples/SX126x

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define LoRa_MOSI 3
#define LoRa_MISO 4
#define LoRa_SCK 5

#define LoRa_nss 8
#define LoRa_dio1 14
#define LoRa_nrst 12
#define LoRa_busy 13

#define PIN_VBAT 11
#define SCL 6
#define SDA 7

#define SEALEVELPRESSURE_HPA (1013.25)

//SX1262 radio = new Module(LoRa_nss, LoRa_dio1, LoRa_nrst, LoRa_busy);
SX1262 radio = new Module(RADIOLIB_BUILTIN_MODULE);
Adafruit_BME280 bme; // I2C

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  pinMode(PIN_VBAT, INPUT);

  // Sensor
  //unsigned status = bme.begin();  
  // You can also pass in a Wire library object like &Wire2
  unsigned status = bme.begin(0x76);
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1) delay(10);
  }

  // Radio
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI);
  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
}

void loop()
{
  // Get the battery voltage.
  int vbat = 2*analogRead(PIN_VBAT);
  // And BME data.
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F;
  float humidity = bme.readHumidity();
  // Build a string.
  char buff[100];
  int size = sprintf(buff, "v%dt%.1fp%.1fh%.1f", vbat, temperature, pressure, humidity);
  buff[size] = '\0';
  Serial.println(buff);

  Serial.print(F("[SX1262] Transmitting packet ... "));

  int state = radio.transmit(buff);

  if (state == RADIOLIB_ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));
  }
  else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
  {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));
  }
  else if (state == RADIOLIB_ERR_TX_TIMEOUT)
  {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait for a second before transmitting again
  delay(1000);
}