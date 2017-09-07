#include "SPI.h"
#include <SD.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "MHDS18B20.h"
#include <WiFi.h>
#include <ESPmDNS.h>
//#include "time.h"
#include <Preferences.h>
#include "SSD1306.h"                //https://github.com/squix78/esp8266-oled-ssd1306
#include <WebServer.h>              //https://github.com/CelliesProjects/WebServer_tng

#define COUNTRY_CODE_ISO_3166 "nl"  //https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2

// number of bit precission for LEDC timer
#define LEDC_NUMBER_OF_BIT    16

// use 10kHz as a LEDC base request frequency
#define LEDC_BASE_FREQ        10000

// PWM depth is the number of discrete steps between fully on and off
#define LEDC_PWM_DEPTH        pow( 2, LEDC_NUMBER_OF_BIT ) - 1

// the number of LED channels
#define NUMBER_OF_CHANNELS    5
#define MAX_TIMERS            50

// the pins for the LED channels
#define LED0_PIN              22
#define LED1_PIN              21
#define LED2_PIN              17
#define LED3_PIN              16
#define LED4_PIN              26

// OneWire Dallas sensors are connected to this pin
#define ONEWIRE_PIN           5

// maximum number of Dallas sensors
#define MAX_NUMBER_OF_SENSORS 3

// HW SPI pin definitions
#define _cs                   0   // Goes to TFT CS
#define _dc                   25  // Goes to TFT DC
#define _mosi                 32  // Goes to TFT MOSI
#define _sclk                 12  // Goes to TFT SCK/CLK
#define _rst                  0   // ESP RST goes to TFT RESET
#define _miso                 4   // Goes to TFT MISO
//       3.3V                     // Goes to TFT LED
//       5v                       // Goes to TFT Vcc-
//       Gnd                      // Goes to TFT Gnd
#define  SD_CS                 27 // Goes to SD CS

// i2c pin definitions for oled
#define I2C_SCL_PIN            19
#define I2C_SDA_PIN            23

//the beef of the program is constructed here
//first define a list of timers
struct lightTimer
{
  time_t      time;                                                 //time in seconds since midnight so range is 0-86400
  byte        percentage;                                           // in percentage so range is 0-100
};

//then a struct for general housekeeping of a ledstrip
struct lightTable
{
  lightTimer timer[MAX_TIMERS];
  String     name;                                                    //initially set to 'channel 1' 'channel 2' etc.
  String     color;                                                   //!!interface color, not light color! Can be 'red' or '#ff0000' or 'rgba(255,0,0,1)', basically anything a browser understands
  float      currentPercentage;                                       //what percentage is this channel set to
  byte       pin;                                                     //which pin is this channel on
  byte       numberOfTimers;                                          //actual number of timers for this channel
  float      minimumLevel;                                            //never dim this channel below this percentage
};

//and make 5 instances
struct lightTable channel[NUMBER_OF_CHANNELS];                           //all channels are now memory allocated

String mDNSname = "aquacontrol32";

//LED pins
const byte ledPin[NUMBER_OF_CHANNELS] =  { LED0_PIN, LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN } ;        //pin numbers of the channels !!!!! should contain [numberOfChannels] entries. D1 through D8 are the exposed pins on 'Wemos D1 mini'

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


SSD1306  OLED( 0x3c, I2C_SDA_PIN, I2C_SCL_PIN );

// TCP server at port 80 will respond to HTTP requests
WebServer server(80);

//global variables
uint16_t LEDC_PWM_DEPTH_NOMATH = LEDC_PWM_DEPTH; // Calculate once

byte numberOfFoundSensors;

String defaultTimerFile = "/default.aqu";

// if programOverride is set to true, the duty cycle of the leds is not updated
bool programOverride = false;

//Boot time is saved
struct tm systemStart;

String lightStatus;

void setup()
{
  //TODO:
  //make pins low/high or whatever they should be

  pinMode(ledPin[0], OUTPUT);
  pinMode(ledPin[1], OUTPUT);
  pinMode(ledPin[2], OUTPUT);
  pinMode(ledPin[3], OUTPUT);
  pinMode(ledPin[4], OUTPUT);

  Wire.begin( I2C_SDA_PIN, I2C_SCL_PIN, 500000 );

  btStop();
  OLED.init();
  OLED.clear();
  OLED.setTextAlignment( TEXT_ALIGN_CENTER );
  OLED.setFont( ArialMT_Plain_16 );
  OLED.drawString( 64, 10, F( "AquaControl32" ) );
  OLED.drawString( 64, 30, F( "Booting..." ) );
  OLED.display();

  //setup channel names
  for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
  {
    channel[thisChannel].name = "Channel " + ( thisChannel + 1 );
    channel[ thisChannel ].color = "undefined" ;
    channel[ thisChannel ].pin = ledPin[ thisChannel ];
    channel[ thisChannel ].minimumLevel = 0;
  }

  Serial.begin(115200);

  Serial.println( F( "aquacontrol32" ) );

  Serial.print( "ESP32 SDK: " );
  Serial.println( ESP.getSdkVersion() );
  Serial.println();

  SPI.begin( _sclk, _miso, _mosi );
  SPI.setFrequency(4000000);

  tft.begin( 38000000 );

  uint8_t x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("ILI9341 TFT Self Diagnostic: 0x"); Serial.println(x, HEX);

  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.println( "TFT started.");

  Serial.print("Initializing SD card...");
  if (!SD.begin( SD_CS, SPI, 2000000 ) ) {
    Serial.println("failed!");
  }
  uint8_t cardType = SD.cardType();
  if ( cardType == CARD_NONE ) {
    Serial.println("No SD card attached");
    tft.println("No SD card attached");
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("SD Card Size: %lluMB\n", cardSize);

  if ( loadDefaultTimers() )
  {
    Serial.println("Default timers loaded");
    tft.println("Default timers loaded");
  }

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

  tft.print( numberOfFoundSensors );  tft.println( " Dallas temperature sensors found." );

  tft.println( "Starting WiFi..." );

  setupWiFi();

  tft.println( WiFi.localIP() );

  // Set up RTC with NTP
  String NTPpoolAdress = COUNTRY_CODE_ISO_3166;
  NTPpoolAdress += ".pool.ntp.org";

  tft.print( "Getting time from " );  tft.println( NTPpoolAdress );

  configTime( -3600, 3600, NTPpoolAdress.c_str() );  //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/README.md
  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c

  if (!getLocalTime(&systemStart))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println( &systemStart, "System start: " "%A, %B %d %Y %H:%M:%S" );

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  bool mDNS = false;
  if (MDNS.begin( mDNSname.c_str() ))
  {
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started");
    Serial.print( "mDNS name: ");  Serial.print( mDNSname );  Serial.println( ".local" );
  }
  else
  {
    Serial.println("Error setting up MDNS responder!");
    bool mDNS = false;
  }

  WiFi.setHostname( mDNSname.c_str() );
  tft.println( "Starting webserver. " );

  setupWebServer();

  //WiFi.printDiag( Serial );


  //setup pwm
  for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
  {
    // Setup timers and attach timer to a led pin
    int ledcActualFrequency = ledcSetup(thisChannel, LEDC_BASE_FREQ, LEDC_NUMBER_OF_BIT);
    Serial.print( "\nChannel: " ); Serial.println( thisChannel + 1 );
    Serial.print( "PWM frequency requested: " ); Serial.print( LEDC_BASE_FREQ / 1000.0 ); Serial.println( "kHz." );
    Serial.print( "PWM frequency actual:    " ); Serial.print( ledcActualFrequency / 1000.0 ); Serial.println( "kHz." );
    Serial.print( "PWM depth:               " ); Serial.print( LEDC_NUMBER_OF_BIT ); Serial.print( "bit - "); Serial.print( (int)LEDC_PWM_DEPTH_NOMATH ); Serial.println( " steps." );

    ledcAttachPin( ledPin[thisChannel], thisChannel );

  }
  tft.fillScreen(ILI9341_BLACK);

  //http://exploreembedded.com/wiki/Task_Switching

  xTaskCreatePinnedToCore(
    webServerTask,                  /* Function to implement the task */
    "webServerTask ",               /* Name of the task */
    3000,                          /* Stack size in words */
    NULL,                           /* Task input parameter */
    4,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  xTaskCreatePinnedToCore(
    oledTask,                       /* Function to implement the task */
    "oledTask ",                    /* Name of the task */
    2000,                          /* Stack size in words */
    NULL,                           /* Task input parameter */
    2,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  xTaskCreatePinnedToCore(
    dimmerTask,                     /* Function to implement the task */
    "dimmerTask ",                  /* Name of the task */
    1000,                          /* Stack size in words */
    NULL,                           /* Task input parameter */
    3,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  lightStatus = "Program running.";

  xTaskCreatePinnedToCore(
    tftTask,                        /* Function to implement the task */
    "tftTask ",                     /* Name of the task */
    2000,                          /* Stack size in words */
    NULL,                           /* Task input parameter */
    1,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  if ( numberOfFoundSensors )
  {
    xTaskCreatePinnedToCore(
      tempTask,                        /* Function to implement the task */
      "tempTask ",                     /* Name of the task */
      4000,                          /* Stack size in words */
      NULL,                           /* Task input parameter */
      5,                              /* Priority of the task */
      NULL,                           /* Task handle. */
      1);                             /* Core where the task should run */
  }
}

void loop()
{
}
