#include "SPI.h"
#include <SD.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
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

// use 10kHz as a LEDC base request frequency
#define LEDC_BASE_FREQ    10000

// PWM depth is the number of discrete steps between fully on and off
#define LEDC_PWM_DEPTH     pow( 2, LEDC_NUMBER_OF_BIT ) - 1

// the number of LED channels
#define NUMBER_OF_CHANNELS 5

// Dallas sensors are connected to this pin
#define ONEWIRE_PIN        5

// maximum number of Dallas sensors
#define MAX_NUMBER_OF_SENSORS 3

//HWSPI pin definitions
#define _cs   0   // Goes to TFT CS
#define _dc   2   // Goes to TFT DC
#define _mosi 32  // Goes to TFT MOSI
#define _sclk 12  // Goes to TFT SCK/CLK
#define _rst  0  // ESP RST goes to TFT RESET
#define _miso 4   // Goes to TFT MISO
//       3.3V     // Goes to TFT LED
//       5v       // Goes to TFT Vcc-
//       Gnd      // Goes to TFT Gnd

//LED pins
const byte PROGMEM ledPin[NUMBER_OF_CHANNELS] =  { 22, 21, 17, 16, 26 } ;        //pin numbers of the channels !!!!! should contain [numberOfChannels] entries. D1 through D8 are the exposed pins on 'Wemos D1 mini'

// Use hardware SPI
Adafruit_ILI9341 tft = Adafruit_ILI9341( _cs, _dc, _rst );

Preferences preferences;

OneWire  ds( ONEWIRE_PIN );  // on pin 5 (a 4.7K resistor is necessary)

/*
       To get from temp saved as float in SensorStruct do:
       celsius = (float)temp / 16.0;
       fahrenheit = celsius * 1.8 + 32.0;
*/
struct sensorStruct
{
  byte addr[8];
  float temp;
  String name;
} sensor[MAX_NUMBER_OF_SENSORS];

byte numberOfFoundSensors;

unsigned long sensorReadTime;

// I2C OLED @ SDA=pin 23, SCL= pin 19
SSD1306  OLED( 0x3c, 23, 19 );

// TCP server at port 80 will respond to HTTP requests
ESP32WebServer server(80);

double brightness = 0;    // how bright the LED is
int fadeAmount = 1;    // how many points to fade the LED by

int ledcActualFrequency;

#define SD_CS 27

void setup()
{
  //TODO:
  //make pins low/high or whatever they should be

  pinMode(ledPin[0], OUTPUT);
  pinMode(ledPin[1], OUTPUT);
  pinMode(ledPin[2], OUTPUT);
  pinMode(ledPin[3], OUTPUT);
  pinMode(ledPin[4], OUTPUT);

  btStop();

  OLED.init();
  OLED.clear();
  OLED.setTextAlignment( TEXT_ALIGN_CENTER );
  OLED.setFont( ArialMT_Plain_16 );
  OLED.drawString( 64, 10, F( "AquaControl32" ) );
  OLED.drawString( 64, 30, F( "Booting..." ) );
  OLED.display();



  Serial.begin(115200);

  //http://marekburiak.github.io/ILI9341_due/
  //SPI.setHwCs(true);
  SPI.begin( _sclk, _miso, _mosi );
  SPI.setFrequency(1000000);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS, SPI, 1000000 )) {
    Serial.println("failed!");
  }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }


    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    listDir(SD, "/", 0);
/*
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/", 2);
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    testFileIO(SD, "/test.txt");  
*/  
  Serial.println( F( "aquacontrol32" ) );

  Serial.print( "ESP32 SDK: " );
  Serial.println( ESP.getSdkVersion() );
  Serial.println();
   
  tft.begin( 10000000 );

  uint8_t x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("ILI9341 TFT Self Diagnostic: 0x"); Serial.println(x, HEX);

  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.println( "TFT started.");

  
  //sensor setup
  byte currentAddr[8];
  while ( ds.search(currentAddr) && numberOfFoundSensors <= MAX_NUMBER_OF_SENSORS )
  {
    numberOfFoundSensors++;
    Serial.write( "Sensor "); Serial.print( numberOfFoundSensors ); Serial.print( ":" );
    for ( byte i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(currentAddr[i], HEX);
      sensor[numberOfFoundSensors].addr[i] = currentAddr[i];
    }
    Serial.println();
  }

  Serial.print(numberOfFoundSensors); Serial.println( " sensors found." );
  for ( byte thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++)
  {
    ds.reset();
    ds.select( sensor[thisSensor].addr );
    ds.write( 0x44, 0);        // start conversion, with parasite power off at the end
  }

  tft.print( numberOfFoundSensors );  tft.println( " Dallas temperature sensors found." );

  if ( numberOfFoundSensors > 0 )
  {
    sensorReadTime = millis() + 750;
  }

  tft.println( "Starting WiFi..." );

  setupWiFi();

  tft.println( WiFi.localIP() );

  // Set up RTC with NTP
  String NTPpoolAdress = COUNTRY_CODE_ISO_3166;
  NTPpoolAdress += ".pool.ntp.org";

  tft.print( "Getting time from " );  tft.println( NTPpoolAdress );


  configTime( -3600, 3600, NTPpoolAdress.c_str() );  //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/README.md
  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c
  printLocalTime();
  printLocalTimeTFT();

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

  tft.println( "Starting webserver. " );

  setupWebServer();

  //WiFi.printDiag( Serial );

  for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
  {
    // Setup timers and attach timer to a led pin
    ledcActualFrequency = ledcSetup(thisChannel, LEDC_BASE_FREQ, LEDC_NUMBER_OF_BIT);
    Serial.print( "\nChannel: " ); Serial.println( thisChannel + 1 );
    Serial.print( "PWM frequency requested: " ); Serial.print( LEDC_BASE_FREQ / 1000.0 ); Serial.println( "kHz." );
    Serial.print( "PWM frequency actual:    " ); Serial.print( ledcActualFrequency / 1000.0 ); Serial.println( "kHz." );
    Serial.print( "PWM depth:               " ); Serial.print( LEDC_NUMBER_OF_BIT ); Serial.print( "bit - "); Serial.print( (int)LEDC_PWM_DEPTH ); Serial.println( " steps." );

    ledcAttachPin( ledPin[thisChannel], thisChannel );

  }
  tft.println( "Setup done." );

}

void loop()
{
  server.handleClient();

  // set the brightness on LEDC channel 0
  ledcWrite(LEDC_CHANNEL_0, brightness);
  for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
  {
    ledcWrite( thisChannel, brightness );
  }

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

