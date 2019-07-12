#include <rom/rtc.h>               /* should be installed together with ESP32 Arduino install */
#include <list>                    /* should be installed together with ESP32 Arduino install */
#include <SPI.h>                   /* should be installed together with ESP32 Arduino install */
#include <Wire.h>                  /* should be installed together with ESP32 Arduino install */
#include <FS.h>                    /* should be installed together with ESP32 Arduino install */
#include <FFat.h>                  /* should be installed together with ESP32 Arduino install */
#include <ESPmDNS.h>               /* should be installed together with ESP32 Arduino install */
#include <Preferences.h>           /* should be installed together with ESP32 Arduino install */
#include <WiFi.h>                  /* should be installed together with ESP32 Arduino install */
#include <Adafruit_ILI9341.h>      /* Install 1.5.1 via 'Manage Libraries' in Arduino IDE */
#include <Adafruit_GFX.h>          /* Install 1.5.6 via 'Manage Libraries' in Arduino IDE */
#include <SSD1306.h>               /* Install 4.0.0 via 'Manage Libraries' in Arduino IDE -> https://github.com/ThingPulse/esp8266-oled-ssd1306 */
#include <XPT2046_Touchscreen.h>   /* Install 1.3 via 'Manage Libraries' in Arduino IDE */
#include <AsyncTCP.h>              /* Reports as 1.0.3 https://github.com/me-no-dev/AsyncTCP */
#include <ESPAsyncWebServer.h>     /* Reports as 1.2.2 https://github.com/me-no-dev/ESPAsyncWebServer */
#include <MoonPhase.h>             /* https://github.com/CelliesProjects/MoonPhase */
#include <sensorState.h>           /* https://github.com/CelliesProjects/sensorState */
#include "ledState.h"

#include "deviceSetup.h"
#include "devicePinSetup.h"

#if GIT_TAG
#include "gitTagVersion.h"
#else
const char * sketchVersion = "ARDUINO IDE";
#endif

/**************************************************************************
       defines for OLED display orientation
**************************************************************************/
#define OLED_ORIENTATION_NORMAL            1
#define OLED_ORIENTATION_UPSIDEDOWN        2


/**************************************************************************
       defines for TFT display orientation
**************************************************************************/
#define TFT_ORIENTATION_NORMAL             1
#define TFT_ORIENTATION_UPSIDEDOWN         3


/**************************************************************************
       update frequency for LEDS in Hz
**************************************************************************/
#define UPDATE_FREQ_LEDS                   100


/**************************************************************************
       update frequency for TFT display in Hz
**************************************************************************/
#define UPDATE_FREQ_TFT                    5


/**************************************************************************
       update frequency for OLED display in Hz
**************************************************************************/
#define UPDATE_FREQ_OLED                   4


/**************************************************************************
       number of bit precission for LEDC timer
**************************************************************************/
#define LEDC_NUMBER_OF_BIT                 16


/**************************************************************************
       maximum allowable pwm frequency in Hz
       -remember the rise and fall times of a 330R gate resistor!
**************************************************************************/
#define LEDC_MAXIMUM_FREQ                  1300


/**************************************************************************
       the number of LED channels
**************************************************************************/
#define NUMBER_OF_CHANNELS                 5


/**************************************************************************
       the maximum number of timers allowed for each channel
**************************************************************************/
#define MAX_TIMERS                         50


/**************************************************************************
       default hostname if no hostname is set
**************************************************************************/
#define DEFAULT_HOSTNAME_PREFIX             "aquacontrol32_"


/**************************************************************************
       defines for threeDigitPercentage()
**************************************************************************/
#define SHOW_PERCENTSIGN                    true
#define NO_PERCENTSIGN                      false

/**************************************************************************
      Setup included libraries
 *************************************************************************/
ledState                leds;

sensorState             sensor;

MoonPhase               MoonPhase;

XPT2046_Touchscreen     touch( TOUCH_CS_PIN, TOUCH_IRQ_PIN );

Adafruit_ILI9341        tft = Adafruit_ILI9341( SPI_TFT_CS_PIN, SPI_TFT_DC_PIN, SPI_TFT_RST_PIN );

SSD1306                 OLED( OLED_ADDRESS, I2C_SDA_PIN, I2C_SCL_PIN );

Preferences             preferences;

/**************************************************************************
       type definitions
**************************************************************************/
struct lightTimer_t
{
  time_t      time;                    /* time in seconds since midnight so range is 0-86400 */
  uint8_t     percentage;              /* in percentage so range is 0-100 */
};

struct channelData_t
{
  lightTimer_t    timer[MAX_TIMERS];
  char            name[15];            /* initially set to 'channel 1' 'channel 2' etc. */
  char            color[8];            /* interface color, not light color! in hex format*/
  /*                                      Example: '#ff0000' for bright red */
  float           currentPercentage;   /* what percentage is this channel set to */
  uint8_t         pin;                 /* which ESP32 pin is this channel on */
  uint8_t         numberOfTimers;      /* actual number of timers for this channel */
  float           minimumLevel;        /* never dim this channel below this percentage */
};

/* const */
const char* defaultTimerFile   = "/default.aqu";

/* task priorities */
const uint8_t dimmerTaskPriority       = 8;
const uint8_t tftTaskPriority          = 6;
const uint8_t ntpTaskPriority          = 5;
const uint8_t oledTaskPriority         = 4;
const uint8_t wifiTaskPriority         = 3;
const uint8_t webserverTaskPriority    = 1;
const uint8_t moonSimtaskPriority      = 0;

/**************************************************************************
       start of global variables
**************************************************************************/
channelData_t           channel[NUMBER_OF_CHANNELS];

MoonPhase::moonData     moonData;

TaskHandle_t            xDimmerTaskHandle            = NULL;
TaskHandle_t            xTftTaskHandle               = NULL;
TaskHandle_t            xOledTaskHandle              = NULL;

//Boot time is saved
timeval                 systemStart;

char                    hostName[30];

double                  ledcActualFrequency;
uint16_t                ledcMaxValue;
uint8_t                 ledcNumberOfBits;

float                   tftBrightness                 = 80;                         /* in percent */
uint8_t                 tftOrientation                = TFT_ORIENTATION_NORMAL;

uint8_t                 oledContrast;                                               /* 0 .. 15 */
uint8_t                 oledOrientation               = OLED_ORIENTATION_NORMAL;

/*****************************************************************************************
       end of global variables
*****************************************************************************************/

/* forward declarations  */
void tftTask( void * pvParameters );
void oledTask( void * pvParameters );
void wifiTask( void * pvParameters );

/* global functions */
float mapFloat( const float &x, const float &in_min, const float &in_max, const float &out_min, const float &out_max) {
  return ( x - in_min ) * ( out_max - out_min ) / ( in_max - in_min ) + out_min;
}

bool logLineToFile( fs::FS &fs, const char * path, const char * message ) {
  File file = fs.open( path, FILE_APPEND );
  if ( !file ) return false;

  if ( !file.println( message ) ) {
    file.close();
    return false;
  }
  file.close();
  return true;
}

const char * resetString( const uint8_t core ) {
  const char * resetStr[] =
  {
    "",
    "POWERON_RESET",
    "",
    "SW_RESET",
    "OWDT_RESET",
    "DEEPSLEEP_RESET",
    "SDIO_RESET",
    "TG0WDT_SYS_RESET",
    "TG1WDT_SYS_RESET",
    "RTCWDT_SYS_RESET",
    "INTRUSION_RESET",
    "TGWDT_CPU_RESET",
    "SW_CPU_RESET",
    "RTCWDT_CPU_RESET",
    "EXT_CPU_RESET",
    "RTCWDT_BROWN_OUT_RESET",
    "RTCWDT_RTC_RESET"
  };
  return resetStr[rtc_get_reset_reason( core )];
}

void threeDigitPercentage( char *buffer, const uint8_t &bufferSize, const float &percentage, const bool &addPercentSign )
{
  if ( percentage < 0.005 )
    snprintf( buffer, bufferSize, addPercentSign ? "  0%%  " : "  0  " );
  else if ( percentage > 99.9 )
    snprintf( buffer, bufferSize, addPercentSign ? " 100%% " : " 100 " );
  else if ( percentage < 10 )
    snprintf( buffer,  bufferSize , addPercentSign ? " %1.2f%% " : " %1.2f ", percentage );
  else
    snprintf( buffer,  bufferSize , addPercentSign ? " %2.1f%% " : " %2.1f ", percentage );
}

void setup()
{
  pinMode( LED0_PIN, OUTPUT );
  pinMode( LED1_PIN, OUTPUT );
  pinMode( LED2_PIN, OUTPUT );
  pinMode( LED3_PIN, OUTPUT );
  pinMode( LED4_PIN, OUTPUT );
  pinMode( TFT_BACKLIGHT_PIN, OUTPUT );

  pinMode( I2C_SCL_PIN, INPUT_PULLUP );
  pinMode( I2C_SDA_PIN, INPUT_PULLUP );

  gpio_set_drive_capability( (gpio_num_t)LED0_PIN, GPIO_DRIVE_CAP_3 );
  gpio_set_drive_capability( (gpio_num_t)LED1_PIN, GPIO_DRIVE_CAP_3 );
  gpio_set_drive_capability( (gpio_num_t)LED2_PIN, GPIO_DRIVE_CAP_3 );
  gpio_set_drive_capability( (gpio_num_t)LED3_PIN, GPIO_DRIVE_CAP_3 );
  gpio_set_drive_capability( (gpio_num_t)LED4_PIN, GPIO_DRIVE_CAP_3 );

  gpio_set_drive_capability( (gpio_num_t)ONEWIRE_PIN, GPIO_DRIVE_CAP_3 );

  btStop();

  ESP_LOGI( TAG, "aquacontrol32 %s", sketchVersion );
  ESP_LOGI( TAG, "ESP32 SDK: %s", ESP.getSdkVersion() );

  if ( FFat.begin() )
    ESP_LOGI( TAG, "FFat partition mounted. Total space: %lu kB. Free space: %lu kB.", FFat.totalBytes() / 1024, FFat.freeBytes() / 1024 );
  else
    ESP_LOGE( TAG, "FATAL ERROR! Could not find FFat. Did you select the right partition scheme? (something with ffat)" );

  preferences.begin( "aquacontrol32", false );

  SPI.begin( SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN );

  tft.begin( TFT_SPI_CLOCK );

  if ( TFT_HAS_NO_MISO || tft.readcommand8( ILI9341_RDSELFDIAG ) == 0xE0 )
  {
    touch.begin();

    ESP_LOGI( TAG, "%s an ILI9341 display on SPI.", TFT_HAS_NO_MISO ? "Forced" : "Found" );

    xTaskCreatePinnedToCore(
      tftTask,                        /* Function to implement the task */
      "tftTask",                      /* Name of the task */
      4000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      tftTaskPriority,                /* Priority of the task */
      &xTftTaskHandle,                /* Task handle. */
      1);                             /* Core where the task should run */
  }
  else
  {
    ESP_LOGI( TAG, "No ILI9341 found" );
  }

  Wire.begin( I2C_SDA_PIN, I2C_SCL_PIN, 400000 );

  Wire.beginTransmission( OLED_ADDRESS );
  uint8_t error = Wire.endTransmission();
  if ( error )
  {
    ESP_LOGI( TAG, "No SSD1306 OLED found." );
  }
  else
  {
    ESP_LOGI( TAG, "Found SSD1306 OLED at address 0x%x.", OLED_ADDRESS );
    xTaskCreatePinnedToCore(
      oledTask,                       /* Function to implement the task */
      "oledTask",                     /* Name of the task */
      3000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      oledTaskPriority,               /* Priority of the task */
      &xOledTaskHandle,               /* Task handle. */
      1);                             /* Core where the task should run */
  }

  xTaskCreatePinnedToCore(
    wifiTask,                       /* Function to implement the task */
    "wifiTask",                     /* Name of the task */
    3000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    wifiTaskPriority,               /* Priority of the task */
    NULL,                           /* Task handle. */
    1);
}

void loop()
{
  vTaskDelete( NULL );
}
