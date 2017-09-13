void dimmerTask ( void * pvParameters )
{
  int dimmerTaskdelayTime = 1000 / UPDATE_FREQ_HZ;

  while (1)
  {
    struct tm timeinfo;
    getLocalTime(&timeinfo);

    struct timeval milliSecondTime;

    gettimeofday( &milliSecondTime, NULL );

    time_t milliSecondsToday = ( timeinfo.tm_hour * 3600000 ) + ( timeinfo.tm_min * 60000 ) + ( timeinfo.tm_sec * 1000 ) + ( milliSecondTime.tv_usec / 1000 );
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
    ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage, 0, 100, 0, LEDC_PWM_DEPTH_NOMATH ) );
  }
}

double setupDimmerPWMfrequency( double frequency )
{
  // Setup timers and attach timer to a led pin
  double newFrequency;
  for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    ledcActualBitDepth = LEDC_NUMBER_OF_BIT;
    newFrequency = ledcSetup( channelNumber, frequency, LEDC_NUMBER_OF_BIT );
    Serial.print( "\nChannel: " ); Serial.println( channelNumber + 1 );
    Serial.print( "PWM frequency requested: " ); Serial.print( frequency / 1000.0 ); Serial.println( "kHz." );
    Serial.print( "PWM frequency actual:    " ); Serial.print( newFrequency / 1000.0 ); Serial.println( "kHz." );
    Serial.print( "PWM depth:               " ); Serial.print( LEDC_NUMBER_OF_BIT ); Serial.print( "bit - "); Serial.print( (int)LEDC_PWM_DEPTH_NOMATH ); Serial.println( " steps." );
    ledcAttachPin( ledPin[channelNumber], channelNumber );
  }
  return newFrequency;
}
