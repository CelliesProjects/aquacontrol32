void dimmerTask ( void * pvParameters )
{
  int dimmerTaskdelayTime = 1000 / UPDATE_FREQ_LEDS;

  while (1)
  {
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    struct timeval microSecondTime;

    gettimeofday( &microSecondTime, NULL );

    time_t milliSecondsToday = ( timeinfo.tm_hour * 3600000 ) + ( timeinfo.tm_min * 60000 ) + ( timeinfo.tm_sec * 1000 ) + ( microSecondTime.tv_usec / 1000 );
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      setPercentageFromProgram( channelNumber, milliSecondsToday );
    }
    vTaskDelay( dimmerTaskdelayTime / portTICK_PERIOD_MS );
  }
}

void setPercentageFromProgram( const byte channelNumber, const time_t milliSecondsToday )
{
  if ( milliSecondsToday )
  { ///to solve flashing at midnight due to milliSecondsToday which cant be smaller than 0 -- so at midnight there is no adjusting
    byte thisTimer = 0;
    while ( channel[channelNumber].timer[thisTimer].time * 1000 < milliSecondsToday )
    {
      thisTimer++;
    }
    float dimPercentage = channel[channelNumber].timer[thisTimer].percentage - channel[channelNumber].timer[thisTimer - 1].percentage;
    time_t numberOfMilliSecondsBetween = ( channel[channelNumber].timer[thisTimer].time - channel[channelNumber].timer[thisTimer - 1].time ) * 1000;
    time_t milliSecondsSinceLastTimer = milliSecondsToday - channel[channelNumber].timer[thisTimer - 1].time * 1000;
    float changePerMilliSecond  = dimPercentage / numberOfMilliSecondsBetween;
    channel[channelNumber].currentPercentage = channel[channelNumber].timer[thisTimer - 1].percentage + ( milliSecondsSinceLastTimer * changePerMilliSecond );

    //check if channel has a minimum set
    if ( channel[channelNumber].currentPercentage < channel[channelNumber].minimumLevel )
    {
      channel[channelNumber].currentPercentage = channel[channelNumber].minimumLevel;
    }
    ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage, 0, 100, 0, ledcMaxValue ) );
  }
}

double setupDimmerPWMfrequency( const double frequency )
{
  // Setup timers and attach timer to a led pin
  double newFrequency;
  for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    newFrequency = ledcSetup( channelNumber, frequency, ledcNumberOfBits );
    ledcAttachPin( channel[channelNumber].pin, channelNumber);
  }
  return newFrequency;
}
