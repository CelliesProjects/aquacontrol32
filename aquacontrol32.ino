#include "SPI.h"                   //should be installed together with ESP32 Arduino install
#include <SD.h>                    //should be installed together with ESP32 Arduino install
#include <ESPmDNS.h>               //should be installed together with ESP32 Arduino install
#include <Preferences.h>           //should be installed together with ESP32 Arduino install
#include "Adafruit_GFX.h"          //Install via 'Manage Libraries' in Arduino IDE
#include "Adafruit_ILI9341.h"      //Install via 'Manage Libraries' in Arduino IDE
#include "OneWire.h"               //https://github.com/CelliesProjects/OneWire
#include "MHDS18B20.h"             //https://github.com/CelliesProjects/ESP32-MINI-KIT
//#include "time.h"
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
  String     color;                                                   //!!interface color, not light color! Example: '#ff0000' for bright red
  float      currentPercentage;                                       //what percentage is this channel set to
  byte       pin;                                                     //which pin is this channel on
  byte       numberOfTimers;                                          //actual number of timers for this channel
  float      minimumLevel;                                            //never dim this channel below this percentage
};

//and make 5 instances
struct lightTable channel[NUMBER_OF_CHANNELS];                           //all channels are now memory allocated

String mDNSname = "aquacontrol32";

//LED pins
const byte ledPin[NUMBER_OF_CHANNELS] =  { LED0_PIN, LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN } ;        //pin numbers of the channels !!!!! should contain [numberOfChannels] entries.

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

TaskHandle_t x_dimmerTaskHandle;

double ledcActualFrequency;
byte ledcActualBitDepth;

byte numberOfFoundSensors;

String defaultTimerFile = "/default.aqu";

//Boot time is saved
struct tm systemStart;

String lightStatus;

void setup()
{
  pinMode(ledPin[0], OUTPUT);
  pinMode(ledPin[1], OUTPUT);
  pinMode(ledPin[2], OUTPUT);
  pinMode(ledPin[3], OUTPUT);
  pinMode(ledPin[4], OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println( F( "aquacontrol32" ) );
  Serial.print( "ESP32 SDK: " );
  Serial.println( ESP.getSdkVersion() );
  Serial.println();

  Wire.begin( I2C_SDA_PIN, I2C_SCL_PIN, 500000 );

  SPI.begin( _sclk, _miso, _mosi );
  SPI.setFrequency( 4000000 );

  setupOLED();

  setupTFT();

  setupWiFi();

  setupNTP();

  Serial.println( getLocalTime(&systemStart) ? "System start: " "%A, %B %d %Y %H:%M:%S" : "Failed to obtain time" );

  Serial.println( cardReaderPresent() ? "SD card found." : "No SD card found." );

  Serial.println( defaultTimersLoaded() ? "Default timers loaded" : "No timers loaded" );

  numberOfFoundSensors = searchDallasSensors();

  setupMDNS();

  setupWebServer();

  //WiFi.printDiag( Serial );

  ledcActualFrequency = setupDimmerPWMfrequency( LEDC_BASE_FREQ );

  //setup channels
  for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    channel[ channelNumber ].name          = readChannelName( channelNumber );
    channel[ channelNumber ].color         = readChannelColor( channelNumber ) ;
    channel[ channelNumber ].pin           = ledPin[ channelNumber ];
    channel[ channelNumber ].minimumLevel  = 0;
  }

  //http://exploreembedded.com/wiki/Task_Switching

  xTaskCreatePinnedToCore(
    webServerTask,                  /* Function to implement the task */
    "webServerTask ",               /* Name of the task */
    3000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    4,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  xTaskCreatePinnedToCore(
    oledTask,                       /* Function to implement the task */
    "oledTask ",                    /* Name of the task */
    2000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    2,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  xTaskCreatePinnedToCore(
    dimmerTask,                     /* Function to implement the task */
    "dimmerTask ",                  /* Name of the task */
    1000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    3,                              /* Priority of the task */
    &x_dimmerTaskHandle,            /* Task handle. */
    1);                             /* Core where the task should run */

  lightStatus = "Program running.";

  xTaskCreatePinnedToCore(
    tftTask,                        /* Function to implement the task */
    "tftTask ",                     /* Name of the task */
    2000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    1,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  if ( numberOfFoundSensors )
  {
    xTaskCreatePinnedToCore(
      tempTask,                       /* Function to implement the task */
      "tempTask ",                    /* Name of the task */
      4000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      5,                              /* Priority of the task */
      NULL,                           /* Task handle. */
      1);                             /* Core where the task should run */
  }
}

//http://www.iotsharing.com/2017/06/how-to-apply-freertos-in-arduino-esp32.html
//http://www.iotsharing.com/2017/05/how-to-apply-finite-state-machine-to-arduino-esp32-avoid-blocking.html
void loop()
{
}
