void dimmerTask ( void * pvParameters )
{
  while (1)
  {
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time_t secondsToday = ( timeinfo.tm_hour * 3600 ) + ( timeinfo.tm_min * 60 ) + timeinfo.tm_sec;
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      setPercentageFromProgram( channelNumber, secondsToday );
    }
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }
}

void  setPercentageFromProgram( const byte channelNumber, const time_t secondsToday ) {
  if ( secondsToday != 0 ) {     ///to solve flashing at midnight due to secondsToday which cant be smaller than 0 -- so at midnight there is no adjusting
    byte thisTimer = 0;
    while ( channel[channelNumber].timer[thisTimer].time < secondsToday ) {
      thisTimer++;
    }
    float dimPercentage = channel[channelNumber].timer[thisTimer].percentage - channel[channelNumber].timer[thisTimer - 1].percentage;
    time_t numberOfSecondsBetween = channel[channelNumber].timer[thisTimer].time - channel[channelNumber].timer[thisTimer - 1].time;
    time_t secondsSinceLastTimer = secondsToday - channel[channelNumber].timer[thisTimer - 1].time;
    float changePerSecond  = dimPercentage / numberOfSecondsBetween;
    channel[channelNumber].currentPercentage = channel[channelNumber].timer[thisTimer - 1].percentage + ( secondsSinceLastTimer * changePerSecond );

    //check if channel has a minimum set
    if ( channel[channelNumber].currentPercentage < channel[channelNumber].minimumLevel ) {
      channel[channelNumber].currentPercentage = channel[channelNumber].minimumLevel;
    }
    ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage, 0, 100, 0, LEDC_PWM_DEPTH_NOMATH ) );
  }
}
