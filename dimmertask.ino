void dimmerTask ( void * pvParameters )
{
  int dimmerTaskdelayTime = 1000 / UPDATE_FREQ_LEDS;

  /* I probably have to read more of this thread: */
  /* https://stackoverflow.com/questions/3673226/how-to-print-time-in-format-2009-08-10-181754-811 */

  while (1)
  {
    struct timeval microSecondTime;

    gettimeofday( &microSecondTime, NULL );

    struct tm *localTime;

    localTime = localtime( &microSecondTime.tv_sec );

    suseconds_t milliSecondsToday = ( localTime->tm_hour * 3600000 )
                                  + ( localTime->tm_min * 60000 )
                                  + ( localTime->tm_sec * 1000 )
                                  + ( microSecondTime.tv_usec / 1000 );

    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      setPercentageFromProgram( channelNumber, milliSecondsToday );
    }
    vTaskDelay( dimmerTaskdelayTime / portTICK_PERIOD_MS );
  }
}

void setPercentageFromProgram( const uint8_t channelNumber, const suseconds_t milliSecondsToday )
{
  /* to solve flashing at midnight due to the fact that the first timer has no predecessor */
  if ( milliSecondsToday )
  {
    uint8_t thisTimer = 0;
    while ( channel[channelNumber].timer[thisTimer].time * 1000 < milliSecondsToday )
    {
      thisTimer++;
    }
    /* only do a lot of math if really neccesary */
    if ( channel[channelNumber].timer[thisTimer].time != channel[channelNumber].timer[thisTimer - 1].time )
    {
      float dimPercentage = channel[channelNumber].timer[thisTimer].percentage - channel[channelNumber].timer[thisTimer - 1].percentage;
      suseconds_t numberOfMilliSecondsBetween = ( channel[channelNumber].timer[thisTimer].time - channel[channelNumber].timer[thisTimer - 1].time ) * 1000;
      suseconds_t milliSecondsSinceLastTimer = milliSecondsToday - ( channel[channelNumber].timer[thisTimer - 1].time * 1000 );
      float changePerMilliSecond = dimPercentage / numberOfMilliSecondsBetween;
      channel[channelNumber].currentPercentage = channel[channelNumber].timer[thisTimer - 1].percentage + ( milliSecondsSinceLastTimer * changePerMilliSecond );
    }
    else
    {
      /* timers are equal so no math neccesary */
      channel[channelNumber].currentPercentage = channel[channelNumber].timer[thisTimer].percentage;
    }
    /* check if channel has a minimum set */
    if ( channel[channelNumber].currentPercentage < channel[channelNumber].minimumLevel )
    {
      channel[channelNumber].currentPercentage = channel[channelNumber].minimumLevel;
    }
    ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage, 0, 100, 0, ledcMaxValue ) );
  }
}

double setupDimmerPWMfrequency( const double frequency )
{
  /* Setup timers and attach timer to a led pin */
  double newFrequency;
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    newFrequency = ledcSetup( channelNumber, frequency, ledcNumberOfBits );
    ledcAttachPin( channel[channelNumber].pin, channelNumber);
  }
  return newFrequency;
}
