/**************************************************************************
       country code for ntp server selection
       https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
**************************************************************************/
#define COUNTRY_CODE_ISO_3166              "nl"


/**************************************************************************
       set to 1 to produce log files
       set to 0 to produce no log files
**************************************************************************/
#define LOG_FILES                          0


/**************************************************************************
       OLED I2C address
**************************************************************************/
#define OLED_ADDRESS                       0x3C


/**************************************************************************
       1 = show system data on oled   0 = show light and temps on oled
**************************************************************************/
#define OLED_SHOW_SYSTEMDATA               0


/**************************************************************************
       Some tft/sdcard breakout boards have their TFT MISO pin unconnected.
       These displays will not be detected by aquacontrol.
       Set TFT_HAS_NO_MISO to 1 to override detection and use these tft boards.
       Set TFT_HAS_NO_MISO to 0 to use device detection.
**************************************************************************/
#define TFT_HAS_NO_MISO                    0


/**************************************************************************
       Some tft/sdcard breakout boards have their touch coordinates reversed.
       Set to 1 if your touchscreen has reversed coordinates.
**************************************************************************/
#define TOUCH_IS_INVERTED                  0

