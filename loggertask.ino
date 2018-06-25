void IRAM_ATTR loggerTask ( void * pvParameters )
{
  const TickType_t loggerTaskdelayTime  = ( 1000 * 120 ) / portTICK_PERIOD_MS;

  while (1)
  {
    const char *    appendError = "Failed to write to log file.";
    char            content[60];
    uint8_t           charCount = 0;
    static bool systemHasBooted = true;
    time_t                  now;
    struct tm          timeinfo;
    char           fileName[17];

    time( &now );
    localtime_r( &now, &timeinfo );
    strftime( fileName , sizeof( fileName ), "/%F.log", &timeinfo );

    ESP_LOGI( TAG, "Logger task writing to %s", fileName );

    deleteOldLogfiles( SPIFFS, "/", 0 );

    if ( systemHasBooted )
    {
      snprintf( content, sizeof( content ), "#%i Aquacontrol32 start", now );
      if ( !writelnFile( SPIFFS, fileName, content ) )
      {
        ESP_LOGE( TAG, "%s", appendError );
      }
      systemHasBooted = false;
      /* add reset reasons for both cores to log file */
      snprintf( content, sizeof( content ), "#Reset reasons: CPU0:%s CPU1:%s ", reset_reason( rtc_get_reset_reason(0) ), reset_reason( rtc_get_reset_reason(1) ) );
      if ( !writelnFile( SPIFFS, fileName, content ) )
      {
        ESP_LOGE( TAG, "%s", appendError );
      }
    }

    if ( numberOfFoundSensors )
    {
      charCount += snprintf( content, sizeof( content ), "%i,", now );
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%3.2f", sensor[ 0 ].tempCelcius);

      for  ( uint8_t sensorNumber = 1; sensorNumber < numberOfFoundSensors; sensorNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, ",%3.2f", sensor[ sensorNumber ].tempCelcius );
      }
      if ( !writelnFile( SPIFFS, fileName, content ) )
      {
        ESP_LOGE( TAG, "%s", appendError );
      }
    }
    else
    {
      vTaskDelete( NULL );
    }
    vTaskDelay( loggerTaskdelayTime );
  }
}

void deleteOldLogfiles( fs::FS &fs, const char * dirname, uint8_t levels )
{
  File root = fs.open( dirname );

  if ( !root )
  {
    ESP_LOGI( TAG, "Failed to open %s", dirname );
    return;
  }

  if ( !root.isDirectory() )
  {
    ESP_LOGI( TAG, "Not a directory" );
    return;
  }

  std::list<String> logFiles;

  File file = root.openNextFile();

  while ( file )
  {
    if ( file.isDirectory() )
    {
      if ( levels )
      {
        deleteOldLogfiles( fs, file.name(), levels - 1 );
      }
    }
    else
    {
      if ( strstr( file.name(), ".log" ) )
      {
        logFiles.push_back( file.name() );
      }
    }
    file = root.openNextFile();
  }

  if ( logFiles.size() > SAVED_LOGFILES )
  {
    logFiles.sort();
  }

  while ( logFiles.size() > SAVED_LOGFILES )
  {
    std::list<String>::iterator thisFile;

    thisFile = logFiles.begin();

    String filename = *thisFile;

    ESP_LOGI( TAG, "Deleting oldest (by numeric compare) log file %s", filename.c_str() );

    fs.remove( filename.c_str() );
    logFiles.erase( thisFile );
  }
}

static inline __attribute__((always_inline)) bool writelnFile( fs::FS &fs, const char * path, const char * message )
{
  File file = fs.open( path, FILE_APPEND );
  if ( !file )
  {
    return false;
  }
  if ( !file.println( message ) )
  {
    file.close();
    return false;
  }
  file.close();
  return true;
}

static inline __attribute__((always_inline)) const char * reset_reason( const RESET_REASON reason )
{
  switch ( reason)
  {
    case 1 : return PSTR("POWERON_RESET"); break;         /**<1, Vbat power on reset*/
    case 3 : return PSTR("SW_RESET"); break;              /**<3, Software reset digital core*/
    case 4 : return PSTR("OWDT_RESET"); break;            /**<4, Legacy watch dog reset digital core*/
    case 5 : return PSTR("DEEPSLEEP_RESET"); break;       /**<5, Deep Sleep reset digital core*/
    case 6 : return PSTR("SDIO_RESET"); break;            /**<6, Reset by SLC module, reset digital core*/
    case 7 : return PSTR("TG0WDT_SYS_RESET"); break;      /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : return PSTR("TG1WDT_SYS_RESET"); break;      /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : return PSTR("RTCWDT_SYS_RESET"); break;      /**<9, RTC Watch dog Reset digital core*/
    case 10 : return PSTR("INTRUSION_RESET"); break;      /**<10, Instrusion tested to reset CPU*/
    case 11 : return PSTR("TGWDT_CPU_RESET"); break;      /**<11, Time Group reset CPU*/
    case 12 : return PSTR("SW_CPU_RESET"); break;         /**<12, Software reset CPU*/
    case 13 : return PSTR("RTCWDT_CPU_RESET"); break;     /**<13, RTC Watch dog Reset CPU*/
    case 14 : return PSTR("EXT_CPU_RESET"); break;        /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : return PSTR("RTCWDT_BROWN_OUT_RESET"); break; /**<15, Reset when the vdd voltage is not stable*/
    case 16 : return PSTR("RTCWDT_RTC_RESET"); break;     /**<16, RTC Watch dog reset digital core and rtc module*/
    default : return PSTR("NO_MEAN");
  }
}

