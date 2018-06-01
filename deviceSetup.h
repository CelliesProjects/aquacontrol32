/**************************************************************************
       country code for ntp server selection
       https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
**************************************************************************/
#define COUNTRY_CODE_ISO_3166              "nl"


/**************************************************************************
       set to 0 to produce no log files
       set to 1 to produce log files
**************************************************************************/
#define LOG_FILES                          1


/**************************************************************************
       how many log files are kept
**************************************************************************/
#define SAVED_LOGFILES                     30


/**************************************************************************
       OLED I2C address
**************************************************************************/
#define OLED_ADDRESS                       0x3C


/**************************************************************************
       0 = show light and temps on oled
       1 = show system data on oled
**************************************************************************/
#define OLED_SHOW_SYSTEMDATA               1


/**************************************************************************
       Some tft/sdcard breakout boards have their TFT MISO pin unconnected.
       These displays will not be auto-detected by aquacontrol.
       Set TFT_HAS_NO_MISO to 0 to use device detection.
       Set TFT_HAS_NO_MISO to 1 to override detection and use these tft boards.
**************************************************************************/
#define TFT_HAS_NO_MISO                    1


/**************************************************************************
       TFT SPI CLOCK SPEED (DEFAULT = 10MHz)
**************************************************************************/
#define TFT_SPI_CLOCK                      10 * 1000 * 1000


/**************************************************************************
       Some tft/sdcard breakout boards have their touch coordinates reversed.
       Set to 1 if your touchscreen has reversed coordinates.
**************************************************************************/
#define TOUCH_IS_INVERTED                  0


/**************************************************************************
       Set to 0 to disable the moon simulator.
**************************************************************************/
#define MOON_SIMULATOR                     1

