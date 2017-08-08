#include "MHDS18B20.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "time.h"
#include <Preferences.h>
#include "SSD1306.h"              //https://github.com/squix78/esp8266-oled-ssd1306
#include <ESP32WebServer.h>

#define mDNSname "aquacontrol32"

#define COUNTRY_CODE_ISO_3166 "nl"  //https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0

// number of bit precission for LEDC timer
#define LEDC_NUMBER_OF_BIT  16

// use 5000 Hz as a LEDC base request frequency
#define LEDC_BASE_FREQ    10000

// PWM depth is the number of discrete steps between fully on and off
#define LEDC_PWM_DEPTH     pow( 2, LEDC_NUMBER_OF_BIT ) - 1

// fade LED PIN (replace with LED_BUILTIN constant for built-in LED)
#define LED_PIN            22

Preferences preferences;

OneWire  ds(5);  // on pin 5 (a 4.7K resistor is necessary)

/*
       To get from temp saved as float in SensorStruct do:
       celsius = (float)temp / 16.0;
       fahrenheit = celsius * 1.8 + 32.0;
*/
struct sensorStruct
{
  byte addr[8];
  float temp;
} sensor[3];

int  numberOfSensors;

unsigned long sensorReadTime;

// I2C OLED @ SDA=pin 23, SCL= pin 19
SSD1306  OLED( 0x3c, 23, 19 );

// TCP server at port 80 will respond to HTTP requests
ESP32WebServer server(80);

double brightness = 0;    // how bright the LED is
int fadeAmount = 1;    // how many points to fade the LED by

int ledcActualFrequency;

void setup()
{
  //TODO:
  //make pins low/high or whatever they should be

  pinMode(22, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(26, OUTPUT);

  btStop();

  OLED.init();
  OLED.clear();
  OLED.setTextAlignment( TEXT_ALIGN_CENTER );
  OLED.setFont( ArialMT_Plain_16 );
  OLED.drawString( 64, 10, F( "AquaControl32" ) );
  OLED.drawString( 64, 30, F( "Booting..." ) );
  OLED.display();

  Serial.begin(115200);

  Serial.println( F( "aquacontrol32" ) );

  Serial.print( "ESP32 SDK: " );
  Serial.println( ESP.getSdkVersion() );
  Serial.println();

  //sensor setup
  byte currentAddr[8];
  while ( ds.search(currentAddr) )
  {
    numberOfSensors++;
    Serial.write( "Sensor "); Serial.print( numberOfSensors ); Serial.print( ":" );
    for ( byte i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(currentAddr[i], HEX);
      sensor[numberOfSensors].addr[i] = currentAddr[i];
    }
    Serial.println();
  }

  Serial.print(numberOfSensors); Serial.println( " sensors found." );
  for ( byte thisSensor = 0; thisSensor < numberOfSensors; thisSensor++)
  {
    ds.reset();
    ds.select( sensor[thisSensor].addr );
    ds.write( 0x44, 0);        // start conversion, with parasite power off at the end
  }

  if ( numberOfSensors > 0 ) {
    sensorReadTime = millis() + 750;
  }

  setupWiFi();

  // Set up RTC with NTP
  String NTPpoolAdress = COUNTRY_CODE_ISO_3166;
  NTPpoolAdress += ".pool.ntp.org";
  configTime( -3600, 3600, NTPpoolAdress.c_str() );  //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/README.md
  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c
  printLocalTime();

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin( mDNSname ))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  Serial.println("mDNS responder started");
  Serial.print( "mDNS name: ");  Serial.print( mDNSname );  Serial.println( ".local" );

  setupWebServer();

  //WiFi.printDiag( Serial );

  // Setup timer and attach timer to a led pin
  ledcActualFrequency = ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_NUMBER_OF_BIT);

  Serial.print( "PWM frequency requested: " ); Serial.print( LEDC_BASE_FREQ / 1000.0 ); Serial.println( "kHz." );
  Serial.print( "PWM frequency actual:    " ); Serial.print( ledcActualFrequency / 1000.0 ); Serial.println( "kHz." );
  Serial.print( "PWM depth:               " ); Serial.print( LEDC_NUMBER_OF_BIT ); Serial.print( "bit - "); Serial.print( (int)LEDC_PWM_DEPTH ); Serial.println( " steps." );

  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);
}


void loop()
{
  server.handleClient();

  // set the brightness on LEDC channel 0
  ledcWrite(LEDC_CHANNEL_0, brightness);

  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= LEDC_PWM_DEPTH )
  {
    fadeAmount = -fadeAmount;
  }
  OLEDprintLocalTime();

  if ( (long)( millis() - sensorReadTime ) >= 0 )
  {
    readTempSensors();
    sensorReadTime = millis() + 750;
  }
}

