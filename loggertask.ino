void IRAM_ATTR loggerTask ( void * pvParameters )
{
  const TickType_t loggerTaskdelayTime  = ( 1000 * 120 ) / portTICK_PERIOD_MS;

  while (1)
  {
    const char *    appendError = "Failed to write to log file.";
    char            content[60];
    static bool systemHasBooted = true;
    time_t                  now;
    struct tm          timeinfo;
    char           fileName[17];

    time( &now );
    localtime_r( &now, &timeinfo );
    strftime( fileName , sizeof( fileName ), "/%F.log", &timeinfo );

    ESP_LOGI( TAG, "Logger task writing to %s", fileName );

    deleteOldLogfiles( FFat, "/", 0 );

    if ( systemHasBooted )
    {
      snprintf( content, sizeof( content ), "#%i Aquacontrol32 start", now );
      if ( !writelnFile( FFat, fileName, content ) )
      {
        ESP_LOGE( TAG, "%s", appendError );
      }
      systemHasBooted = false;
      /* add reset reasons for both cores to log file */
      snprintf( content, sizeof( content ), "#Reset reasons: CPU0:%s CPU1:%s ", resetString( rtc_get_reset_reason(0) ), resetString( rtc_get_reset_reason(1) ) );
      if ( !writelnFile( FFat, fileName, content ) )
      {
        ESP_LOGE( TAG, "%s", appendError );
      }
    }

    uint8_t charCount = 0;
    if ( sensor.count() )
    {
      charCount += snprintf( content, sizeof( content ), "%i,", now );
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%3.2f", sensor.temp( 0 ) );

      for  ( uint8_t sensorNumber = 1; sensorNumber < sensor.count(); sensorNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, ",%3.2f", sensor.temp( sensorNumber ) );
      }
      if ( !writelnFile( FFat, fileName, content ) )
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

const char * resetString( const RESET_REASON reason )
{
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
  return resetStr[reason];
}
