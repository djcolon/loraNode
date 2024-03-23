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

#define DEVICE_ID 0x01

// Low power setup.
//#define USE_SERIAL
#define SLEEP_TIME_MS 300000 // 5 mins
static TimerEvent_t wakeUp;
uint8_t lowpower = 0;

//SX1262 radio = new Module(LoRa_nss, LoRa_dio1, LoRa_nrst, LoRa_busy);
SX1262 radio = new Module(RADIOLIB_BUILTIN_MODULE);
Adafruit_BME280 bme; // I2C

/**
 * We don't want to use serial in production to save power. So unify this here.
*/
void serialPrintLn(String data) {
  #ifdef USE_SERIAL
  Serial.println(data);
  #endif
}

/**
 * We don't want to use serial in production to save power. So unify this here.
*/
void serialPrint(String data) {
  #ifdef USE_SERIAL
  Serial.print(data);
  #endif
}

void sleep() {
  serialPrintLn("Going into low power mode.");
  lowpower=1;
  //timetillwakeup ms later wake up;
  TimerSetValue( &wakeUp, SLEEP_TIME_MS );
  TimerStart( &wakeUp );
}

void onWakeUp() {
  lowpower=0;
  serialPrintLn("Woke up.");
}

void setup()
{
  #ifdef USE_SERIAL
  Serial.begin(115200);
  #endif
  Wire.begin();
  pinMode(PIN_VBAT, INPUT);

  // Sensor
  //unsigned status = bme.begin();  
  // You can also pass in a Wire library object like &Wire2
  unsigned status = bme.begin(0x76);
  if (!status) {
      serialPrintLn("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      while (1) delay(10);
  }

  // Radio
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI);
  // initialize SX1262 with default settings
  serialPrint(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    serialPrintLn(F("success!"));
  }
  else
  {
    serialPrint(F("failed, code "));
    serialPrintLn(String(state));
    while (true)
      ;
  }

  // Low power.
  TimerInit( &wakeUp, onWakeUp );
}

void loop()
{
  if(lowpower){
    lowPowerHandler();
  }
  // Get the battery voltage (according to datasheet).
  int vbat = 2*analogRead(PIN_VBAT);

  // And BME data.
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F;
  float humidity = bme.readHumidity();

  // Build a string.
  char buff[100];
  int size = sprintf(buff, "%dv%dt%.1fp%.1fh%.1f", DEVICE_ID, vbat, temperature, pressure, humidity);
  buff[size] = '\0';
  serialPrintLn(buff);

  int state = radio.transmit(buff);

  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    serialPrintLn(F("Sensor data sent."));
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    serialPrintLn(F("Sensor data too long to transmit."));
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    serialPrintLn(F("Timeout sending sensor data."));
  } else {
    // some other error occurred
    serialPrint(F("Failed to send sensor data with code: "));
    serialPrintLn(String(state));
  }

  // Go into deep sleep for a while.
  sleep();
}