#include "SPI.h"                   //should be installed together with ESP32 Arduino install
#include "SPIFFS.h"
#include <ESPmDNS.h>               //should be installed together with ESP32 Arduino install
#include <Preferences.h>           //should be installed together with ESP32 Arduino install
#include "apps/sntp/sntp.h"        //should be installed together with ESP32 Arduino install
#include "Adafruit_GFX.h"          //Install via 'Manage Libraries' in Arduino IDE
#include "Adafruit_ILI9341.h"      //Install via 'Manage Libraries' in Arduino IDE
#include "OneWire.h"               //https://github.com/CelliesProjects/OneWire
#include "MHDS18B20.h"             //https://github.com/CelliesProjects/ESP32-MINI-KIT
#include "SSD1306.h"               //https://github.com/squix78/esp8266-oled-ssd1306
#include <WebServer.h>             //https://github.com/CelliesProjects/WebServer_tng


/**************************************************************************
       1 = oled is enabled   0 = oled is disabled
**************************************************************************/
#define OLED_ENABLED                      1


/**************************************************************************
       1 = show system data on oled   0 = show light and temps on oled
**************************************************************************/
#define OLED_SHOW_SYSTEMDATA             0


/**************************************************************************
       defines for OLED display orientation
**************************************************************************/
#define OLED_ORIENTATION_NORMAL           1
#define OLED_ORIENTATION_UPSIDEDOWN       2


/**************************************************************************
       1 = tft is enabled   0 = tft is disabled
**************************************************************************/
#define TFT_ENABLED                      1


/**************************************************************************
       defines for TFT display orientation
**************************************************************************/
#define TFT_ORIENTATION_NORMAL           1
#define TFT_ORIENTATION_UPSIDEDOWN       3


/**************************************************************************
       country code for ntp server selection
       https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
**************************************************************************/
#define COUNTRY_CODE_ISO_3166 "nl"


/**************************************************************************
       update frequency for LEDS in Hz
**************************************************************************/
#define UPDATE_FREQ_LEDS      100


/**************************************************************************
       update frequency for TFT display in Hz
**************************************************************************/
#define UPDATE_FREQ_TFT       10


/**************************************************************************
       update frequency for OLED display in Hz
**************************************************************************/
#define UPDATE_FREQ_OLED      4


/**************************************************************************
       number of bit precission for LEDC timer
**************************************************************************/
#define LEDC_NUMBER_OF_BIT    16


/**************************************************************************
       maximum allowable pwm frequency in Hz
       -remember the rise and fall times of a 330R gate resistor!
**************************************************************************/
#define LEDC_MAXIMUM_FREQ     1300


/**************************************************************************
       the number of LED channels
**************************************************************************/
#define NUMBER_OF_CHANNELS    5


/**************************************************************************
       the maximum number of timers allowed for each channel
**************************************************************************/
#define MAX_TIMERS            50


/**************************************************************************
       LED pin numbers
**************************************************************************/
#define LED0_PIN              22
#define LED1_PIN              21
#define LED2_PIN              17
#define LED3_PIN              16
#define LED4_PIN              26


/**************************************************************************
      HW SPI pin definitions
**************************************************************************/
#define SPI_TFT_DC_PIN            27  // Goes to TFT DC
#define SPI_SCK_PIN               25  // Goes to TFT SCK/CLK
#define SPI_MOSI_PIN              32  // Goes to TFT MOSI
#define SPI_TFT_RST_PIN           12  // Goes to TFT RESET
#define SPI_TFT_CS_PIN             4  // Goes to TFT CS
#define SPI_SD_CS_PIN              0  // Goes to SD CS
#define SPI_MISO_PIN              14  // Goes to TFT MISO

//       3.3V                     // Goes to TFT LED
//       5v                       // Goes to TFT Vcc-
//       Gnd                      // Goes to TFT Gnd


/**************************************************************************
       TFT display backlight control
**************************************************************************/
#define TFT_BACKLIGHT_PIN       2


/**************************************************************************
      i2c pin definitions for oled
**************************************************************************/
#define I2C_SCL_PIN            19
#define I2C_SDA_PIN            23


/**************************************************************************
       OneWire Dallas sensors are connected to this pin
**************************************************************************/
#define ONEWIRE_PIN           5


/**************************************************************************
       maximum number of Dallas sensors
**************************************************************************/
#define MAX_NUMBER_OF_SENSORS 3


/**************************************************************************
      Setup included libraries
 *************************************************************************/
Adafruit_ILI9341 tft = Adafruit_ILI9341( SPI_TFT_CS_PIN, SPI_TFT_DC_PIN, SPI_TFT_RST_PIN );

Preferences preferences;

OneWire  ds( ONEWIRE_PIN );  /* a 4.7K pull-up resistor is necessary */

SSD1306  OLED( 0x3c, I2C_SDA_PIN, I2C_SCL_PIN );

WebServer server(80);


/**************************************************************************
       start of global variables
**************************************************************************/


/**************************************************************************
       Username and password for web interface
 *************************************************************************/
const char* www_username    = "admin";  //change me!
const char* www_password    = "esp32";  //change me!

const char* defaultTimerFile = "/default.aqu";

struct lightTimer
{
  time_t      time;               /* time in seconds since midnight so range is 0-86400 */
  byte        percentage;         /* in percentage so range is 0-100 */
};

struct lightTable
{
  lightTimer timer[MAX_TIMERS];
  String     name;                /* initially set to 'channel 1' 'channel 2' etc. */
  String     color;               /* interface color, not light color! in hex format*/
  /* Example: '#ff0000' for bright red */
  float      currentPercentage;   /* what percentage is this channel set to */
  byte       pin;                 /* which ESP32 pin is this channel on */
  byte       numberOfTimers;      /* actual number of timers for this channel */
  float      minimumLevel;        /* never dim this channel below this percentage */
} channel[NUMBER_OF_CHANNELS];


/******************************************************************************************
        struct to keep track of Dallas DS18B20 sensors
        To get from temp saved as float in SensorStruct do:
        celsius    = temp / 16.0;
        fahrenheit = celsius * 1.8 + 32.0;
******************************************************************************************/
struct sensorStruct
{
  byte   addr[8];
  float  temp;
  char   name[5];
} sensor[MAX_NUMBER_OF_SENSORS];

String mDNSname = "aquacontrol32";

TaskHandle_t x_dimmerTaskHandle = NULL;
TaskHandle_t x_tftTaskHandle    = NULL;

double   ledcActualFrequency;
uint16_t ledcMaxValue;
uint8_t  ledcNumberOfBits;

byte numberOfFoundSensors;

//Boot time is saved
struct tm systemStart;

String lightStatus;

float   tftBrightness   = 80;                        /* in percent */
uint8_t tftOrientation  = TFT_ORIENTATION_NORMAL;

int8_t  oledContrast;                                /* 0 .. 15 */
uint8_t oledOrientation = OLED_ORIENTATION_NORMAL;

/*****************************************************************************************

       end of global variables

*****************************************************************************************/


void setup()
{
  pinMode( LED0_PIN, OUTPUT );
  pinMode( LED1_PIN, OUTPUT );
  pinMode( LED2_PIN, OUTPUT );
  pinMode( LED3_PIN, OUTPUT );
  pinMode( LED4_PIN, OUTPUT );
  pinMode( TFT_BACKLIGHT_PIN, OUTPUT );

  btStop();

  Serial.begin( 115200 );
  Serial.println();
  Serial.println( "aquacontrol32" );
  Serial.print( "ESP32 SDK: " );
  Serial.println( ESP.getSdkVersion() );
  Serial.println();

  SPI.begin( SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN );
  SPI.setFrequency( 60000000 );

  xTaskCreatePinnedToCore(
    tempTask,                       /* Function to implement the task */
    "tempTask ",                    /* Name of the task */
    4000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    5,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  if ( OLED_ENABLED )
  {
    xTaskCreatePinnedToCore(
      oledTask,                       /* Function to implement the task */
      "oledTask ",                    /* Name of the task */
      2000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      1,                              /* Priority of the task */
      NULL,                           /* Task handle. */
      1);                             /* Core where the task should run */
  }

  Serial.println( "Starting and possibly formatting SPIFFS. ( Just be patient... )" );

  if ( !SPIFFS.begin( true ) )
  {
    Serial.println( "No SPIFFS!" );
  }
  else
  {
    Serial.println( "SPIFFS started." );
  }

  setupWiFi();

  setupNTP();

  Serial.print( "Local time:" ); Serial.println( getLocalTime( &systemStart ) ? asctime( &systemStart ) : "Failed to obtain time" );

  if ( defaultTimersLoaded() )
  {
    Serial.println("Default timers loaded." );
  }
  else
  {
    Serial.println( "No timers loaded." );
    setEmptyTimers();
  }

  setupMDNS();

  //WiFi.printDiag( Serial );

  //setup channels
  channel[ 0 ].pin = LED0_PIN;
  channel[ 1 ].pin = LED1_PIN;
  channel[ 2 ].pin = LED2_PIN;
  channel[ 3 ].pin = LED3_PIN;
  channel[ 4 ].pin = LED4_PIN;
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    channel[ channelNumber ].name          = readStringNVS( "channelname" +  char( channelNumber ), "Channel" + char( channelNumber + 1 ) );
    channel[ channelNumber ].color         = readStringNVS( "channelcolor" + char( channelNumber ), "#fffe7a" );
    channel[ channelNumber ].minimumLevel  = readFloatNVS( "channelminimum" + char( channelNumber ), 0 );
    ledcAttachPin( channel[channelNumber].pin, channelNumber);
  }

  setupDimmerPWMfrequency( readDoubleNVS( "pwmfrequency", LEDC_MAXIMUM_FREQ ), readInt8NVS( "pwmdepth", LEDC_NUMBER_OF_BIT ) );



  /*****************************************************************************************

         start the different tasks
         http://exploreembedded.com/wiki/Task_Switching

  *****************************************************************************************/


  xTaskCreatePinnedToCore(
    webServerTask,                  /* Function to implement the task */
    "webServerTask ",               /* Name of the task */
    3000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    4,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  xTaskCreatePinnedToCore(
    dimmerTask,                     /* Function to implement the task */
    "dimmerTask ",                  /* Name of the task */
    1000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    6,                              /* Priority of the task */
    &x_dimmerTaskHandle,            /* Task handle. */
    1);                             /* Core where the task should run */

  if ( TFT_ENABLED )
  {
    xTaskCreatePinnedToCore(
      tftTask,                        /* Function to implement the task */
      "tftTask ",                     /* Name of the task */
      2000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      1,                              /* Priority of the task */
      &x_tftTaskHandle,               /* Task handle. */
      1);                             /* Core where the task should run */
  }
}


/*****************************************************************************************

       loopTask start

       http://www.iotsharing.com/2017/06/how-to-apply-freertos-in-arduino-esp32.html
       http://www.iotsharing.com/2017/05/how-to-apply-finite-state-machine-to-arduino-esp32-avoid-blocking.html

*****************************************************************************************/
bool setupEnded = false;

void loop()
{
  setupEnded = true;
  vTaskDelete( NULL );
}
