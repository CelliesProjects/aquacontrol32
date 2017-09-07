void dimmerTask ( void * pvParameters )
{
  while (1)
  {
    if ( !programOverride )
    {
      struct tm timeinfo;
      getLocalTime(&timeinfo);
      time_t secondsToday = ( timeinfo.tm_hour * 3600 ) + ( timeinfo.tm_min * 60 ) + timeinfo.tm_sec;
      for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
      {
        setPercentageFromProgram( thisChannel, secondsToday );
      }
    }
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }
}

void  setPercentageFromProgram( const byte thisChannel, const time_t secondsToday ) {
  if ( secondsToday != 0 ) {     ///to solve flashing at midnight due to secondsToday which cant be smaller than 0 -- so at midnight there is no adjusting
    byte thisTimer = 0;
    while ( channel[thisChannel].timer[thisTimer].time < secondsToday ) {
      thisTimer++;
    }
    float dimPercentage = channel[thisChannel].timer[thisTimer].percentage - channel[thisChannel].timer[thisTimer - 1].percentage;
    time_t numberOfSecondsBetween = channel[thisChannel].timer[thisTimer].time - channel[thisChannel].timer[thisTimer - 1].time;
    time_t secondsSinceLastTimer = secondsToday - channel[thisChannel].timer[thisTimer - 1].time;
    float changePerSecond  = dimPercentage / numberOfSecondsBetween;
    channel[thisChannel].currentPercentage = channel[thisChannel].timer[thisTimer - 1].percentage + ( secondsSinceLastTimer * changePerSecond );

    //check if channel has a minimum set
    if ( channel[thisChannel].currentPercentage < channel[thisChannel].minimumLevel ) {
      channel[thisChannel].currentPercentage = channel[thisChannel].minimumLevel;
    }
    ledcWrite( thisChannel, mapFloat( channel[thisChannel].currentPercentage, 0, 100, 0, LEDC_PWM_DEPTH ) );
  }
}
