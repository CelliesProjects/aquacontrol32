void loggerTask ( void * pvParameters )
{
  const uint64_t loggerTaskdelayTime  = ( 1000 * 60 ) / portTICK_PERIOD_MS;

  if ( !numberOfFoundSensors )
  {
    vTaskDelete( NULL );
  }

  TickType_t xLastWakeTime;

  xLastWakeTime = xTaskGetTickCount();

  while (1)
  {
    char            content[40];
    uint8_t           charCount = 0;
    static bool systemHasBooted = true;
    time_t                  now;
    struct tm          timeinfo;

    time( &now );

    charCount += snprintf( content, sizeof( content ), "%i,", now );

    if ( numberOfFoundSensors )
    {
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%3.2f", sensor[ 0 ].tempCelcius);

      for  ( uint8_t sensorNumber = 1; sensorNumber < numberOfFoundSensors; sensorNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, ",%3.2f", sensor[ sensorNumber ].tempCelcius );
      }
    }
    else
    {
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, ",%s", "No sensors." );
    }

    char fileName[17];

    localtime_r( &now, &timeinfo );
    strftime( fileName , sizeof( fileName ), "/%F.log", &timeinfo );

    const char * appendError = "Failed to open log file.";

    if ( systemHasBooted )
    {
      char buffer[40];

      snprintf( buffer, sizeof( buffer ), "#%i Aquacontrol32 start", now );
      if ( !writelnFile( SPIFFS, fileName, buffer ) )
      {
        Serial.println( appendError );
      }
      systemHasBooted = false;
    }

    if ( !writelnFile( SPIFFS, fileName, content ) )
    {
      Serial.println( appendError );
    }
    vTaskDelayUntil( &xLastWakeTime, loggerTaskdelayTime / portTICK_PERIOD_MS );
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
