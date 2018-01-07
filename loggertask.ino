void loggerTask ( void * pvParameters )
{
  const uint64_t loggerTaskdelayTime  = ( 1000 * 119 ) / portTICK_PERIOD_MS;

  while (1)
  {
    const char *    appendError = "Failed to write to log file.";
    char            content[40];
    uint8_t           charCount = 0;
    static bool systemHasBooted = true;
    time_t                  now;
    struct tm          timeinfo;
    char           fileName[17];

    time( &now );
    localtime_r( &now, &timeinfo );
    strftime( fileName , sizeof( fileName ), "/%F.log", &timeinfo );

    if ( systemHasBooted )
    {
      snprintf( content, sizeof( content ), "#%i Aquacontrol32 start", now );
      if ( !writelnFile( SPIFFS, fileName, content ) )
      {
        ESP_LOGE( TAG, "%s", appendError );
      }
      systemHasBooted = false;
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
