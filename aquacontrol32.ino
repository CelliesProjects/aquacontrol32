#include <rom/rtc.h>
#include <SPI.h>                   /* should be installed together with ESP32 Arduino install */
#include <SPIFFS.h>                /* should be installed together with ESP32 Arduino install */
#include <ESPmDNS.h>               /* should be installed together with ESP32 Arduino install */
#include <Preferences.h>           /* should be installed together with ESP32 Arduino install */
#include "Adafruit_ILI9341.h"      /* Install via 'Manage Libraries' in Arduino IDE */
#include "OneWire.h"               /* Install via 'Manage Libraries' in Arduino IDE */
/* or use stickbreakers library:      https://github.com/stickbreaker/OneWire */
#include "SSD1306.h"               /* https://github.com/squix78/esp8266-oled-ssd1306 */
#include <AsyncTCP.h>              /* https://github.com/me-no-dev/ESPAsyncTCP */
#include <ESPAsyncWebServer.h>     /* https://github.com/me-no-dev/ESPAsyncWebServer */
#include <XPT2046_Touchscreen.h>   /* https://github.com/PaulStoffregen/XPT2046_Touchscreen */

#include "gitTagVersion.h"
#include "deviceSetup.h"
#include "devicePinSetup.h"

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
       maximum number of Dallas sensors
**************************************************************************/
#define MAX_NUMBER_OF_SENSORS               3


/**************************************************************************
       default hostname if no hostname is set
**************************************************************************/
#define DEFAULT_HOSTNAME_PREFIX             "aquacontrol32_"


/**************************************************************************
      Setup included libraries
 *************************************************************************/
XPT2046_Touchscreen touch( TOUCH_CS_PIN, TOUCH_IRQ_PIN );

Adafruit_ILI9341 tft = Adafruit_ILI9341( SPI_TFT_CS_PIN, SPI_TFT_DC_PIN, SPI_TFT_RST_PIN );

Preferences preferences;

OneWire  ds( ONEWIRE_PIN );  /* a 4.7K pull-up resistor is necessary */

SSD1306  OLED( OLED_ADDRESS, I2C_SDA_PIN, I2C_SCL_PIN );

AsyncWebServer server(80);

/**************************************************************************
       type definitions
**************************************************************************/
enum lightStatus_t
{
  LIGHTS_OFF, LIGHTS_ON, LIGHTS_AUTO
};

static inline __attribute__((always_inline)) const char *ToString( const lightStatus_t status )
{
  switch ( status )
  {
    case LIGHTS_OFF:   return " LIGHTS OFF";
    case LIGHTS_ON:    return " LIGHTS ON ";
    case LIGHTS_AUTO:  return "LIGHTS AUTO";
    default:           return " UNDEFINED ";
  }
}

struct lightTimer_t
{
  time_t      time;                    /* time in seconds since midnight so range is 0-86400 */
  byte        percentage;              /* in percentage so range is 0-100 */
};

struct channelData_t
{
  lightTimer_t    timer[MAX_TIMERS];
  char            name[15];            /* initially set to 'channel 1' 'channel 2' etc. */
  char            color[8];            /* interface color, not light color! in hex format*/
  /*                                      Example: '#ff0000' for bright red */
  float           currentPercentage;   /* what percentage is this channel set to */
  byte            pin;                 /* which ESP32 pin is this channel on */
  byte            numberOfTimers;      /* actual number of timers for this channel */
  float           minimumLevel;        /* never dim this channel below this percentage */
};

struct sensorData_t                    /* struct to keep track of Dallas DS18B20 sensors */
{
  byte   addr[8];
  float  tempCelcius;
  char   name[15];
};

/* const */
const char* defaultTimerFile   = "/default.aqu";

/* task priorities */
const uint8_t dimmerTaskPriority       = 8;
const uint8_t tempTaskPriority         = 7;
const uint8_t webserverTaskPriority    = 6;
const uint8_t tftTaskPriority          = 5;
const uint8_t ntpTaskPriority          = 4;
const uint8_t oledTaskPriority         = 3;
const uint8_t wifiTaskPriority         = 2;
const uint8_t loggerTaskPriority       = 1;
const uint8_t versionCheckPriority     = 1;
const uint8_t spiffsTaskPriority       = 0;


/**************************************************************************
       start of global variables
**************************************************************************/
channelData_t           channel[NUMBER_OF_CHANNELS];

sensorData_t            sensor[MAX_NUMBER_OF_SENSORS];

lightStatus_t           lightStatus;

TaskHandle_t            xDimmerTaskHandle            = NULL;
TaskHandle_t            xTftTaskHandle               = NULL;
TaskHandle_t            xOledTaskHandle              = NULL;
TaskHandle_t            xLoggerTaskHandle            = NULL;

//Boot time is saved
timeval                 systemStart;

char                    hostName[30];

double                  ledcActualFrequency;
uint16_t                ledcMaxValue;
uint8_t                 ledcNumberOfBits;

byte                    numberOfFoundSensors;

float                   tftBrightness                 = 80;                         /* in percent */
uint8_t                 tftOrientation                = TFT_ORIENTATION_NORMAL;

uint8_t                 oledContrast;                                               /* 0 .. 15 */
uint8_t                 oledOrientation               = OLED_ORIENTATION_NORMAL;

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

  pinMode( I2C_SCL_PIN, INPUT_PULLUP );
  pinMode( I2C_SDA_PIN, INPUT_PULLUP );

  preferences.begin( "aquacontrol32", false );

  btStop();
  //Serial.begin( 115200 );
  ESP_LOGI( TAG, "aquacontrol32 %s", sketchVersion );
  ESP_LOGI( TAG, "ESP32 SDK: %s", ESP.getSdkVersion() );

  SPI.begin( SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN );
  SPI.setFrequency( 80000000 );

  tft.begin( 20000000, SPI );

  if ( TFT_HAS_NO_MISO || tft.readcommand8( ILI9341_RDSELFDIAG ) == 0xE0 )
  {
    touch.begin();
    if ( TFT_HAS_NO_MISO ) ESP_LOGI( TAG, "Forced ILI9341 start." );
    else                   ESP_LOGI( TAG, "ILI9341 display found." );
    xTaskCreatePinnedToCore(
      tftTask,                        /* Function to implement the task */
      "tftTask",                      /* Name of the task */
      3000,                           /* Stack size in words */
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
  uint8_t err = Wire.endTransmission();
  if ( err == 0 )
  {
    ESP_LOGI( TAG, "Found I2C device at address 0x%x.", OLED_ADDRESS );
    xTaskCreatePinnedToCore(
      oledTask,                       /* Function to implement the task */
      "oledTask",                     /* Name of the task */
      2000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      oledTaskPriority,               /* Priority of the task */
      &xOledTaskHandle,               /* Task handle. */
      1);                             /* Core where the task should run */
  }
  else
  {
    ESP_LOGI( TAG, "No I2C device found." );
  }

  xTaskCreatePinnedToCore(
    wifiTask,                       /* Function to implement the task */
    "wifiTask",                     /* Name of the task */
    2000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    wifiTaskPriority,               /* Priority of the task */
    NULL,                           /* Task handle. */
    1);

  xTaskCreatePinnedToCore(
    tempTask,                       /* Function to implement the task */
    "tempTask",                     /* Name of the task */
    4000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    tempTaskPriority,               /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  xTaskCreatePinnedToCore(
    spiffsTask,                     /* Function to implement the task */
    "spiffsTask",                   /* Name of the task */
    2000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    spiffsTaskPriority,             /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */
}

void loop()
{
  vTaskDelete( NULL );
}
