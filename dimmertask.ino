void dimmerTask ( void * pvParameters )
{
  const uint32_t dimmerTaskdelayTime = 1000 / UPDATE_FREQ_LEDS;

  TickType_t xLastWakeTime;

  if ( defaultTimersLoaded() )
  {
    Serial.println("Default timers loaded." );
  }
  else
  {
    Serial.println( "No timers loaded." );
    setEmptyTimers();
  }

  //setup channels
  channel[ 0 ].pin = LED0_PIN;
  channel[ 1 ].pin = LED1_PIN;
  channel[ 2 ].pin = LED2_PIN;
  channel[ 3 ].pin = LED3_PIN;
  channel[ 4 ].pin = LED4_PIN;
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    char buff[32];

    snprintf( buff, sizeof( buff ), "channelname%i", channelNumber );
    channel[ channelNumber ].name          = readStringNVS( buff, "channelname" );

    snprintf( buff, sizeof( buff ), "channelcolor%i", channelNumber );
    channel[ channelNumber ].color         = readStringNVS( buff, "#fffe7a" );

    snprintf( buff, sizeof( buff ), "channelminimum%i", channelNumber );
    channel[ channelNumber ].minimumLevel  = readFloatNVS( buff, 0 );

    ledcAttachPin( channel[channelNumber].pin, channelNumber);
  }

  setupDimmerPWMfrequency( readDoubleNVS( "pwmfrequency", LEDC_MAXIMUM_FREQ ), readInt8NVS( "pwmdepth", LEDC_NUMBER_OF_BIT ) );

  lightStatus = "LIGHTS AUTO";

  xLastWakeTime = xTaskGetTickCount();

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

    if ( milliSecondsToday ) /* to solve flashing at 00:00:000 due to the fact that the first timer has no predecessor */
    {
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
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
    vTaskDelayUntil( &xLastWakeTime, dimmerTaskdelayTime / portTICK_PERIOD_MS );
  }
}

void setupDimmerPWMfrequency( const double frequency, const uint8_t numberOfBits )
{
  /* Setup timers and pwm bit depth */
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    ledcActualFrequency = ledcSetup( channelNumber, frequency, numberOfBits );
  }
  ledcMaxValue = ( 0x00000001 << numberOfBits ) - 1;
  ledcNumberOfBits = numberOfBits;
  Serial.printf( "\nPWM frequency set to %.2f kHz\n", ledcActualFrequency / 1000);
  Serial.printf( "PWM bit depth set to %i bits\n", ledcNumberOfBits);
  Serial.printf( "Maximum raw value set to 0x%x or %i decimal\n\n", ledcMaxValue, ledcMaxValue);
}
