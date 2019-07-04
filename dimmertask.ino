void IRAM_ATTR dimmerTask ( void * pvParameters )
{
  const TickType_t dimmerTaskdelayTime = 1000 / UPDATE_FREQ_LEDS / portTICK_PERIOD_MS;

  TickType_t xLastWakeTime;

  if ( defaultTimersLoaded() )
  {
    ESP_LOGI( TAG, "Default timers loaded." );
  }
  else
  {
    ESP_LOGI( TAG, "No timers loaded." );
    setEmptyTimers();
  }

  /* setup pwm on leds */
  channel[ 0 ].pin = LED0_PIN;
  channel[ 1 ].pin = LED1_PIN;
  channel[ 2 ].pin = LED2_PIN;
  channel[ 3 ].pin = LED3_PIN;
  channel[ 4 ].pin = LED4_PIN;
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    char NVSKeyName[32];

    snprintf( NVSKeyName, sizeof( NVSKeyName ), "channelname%i", channelNumber );
    snprintf( channel[ channelNumber ].name, sizeof( channel[ channelNumber ].name ), preferences.getString( NVSKeyName, "channelname" ).c_str() );

    snprintf( NVSKeyName, sizeof( NVSKeyName ), "channelcolor%i", channelNumber );
    snprintf( channel[ channelNumber ].color, sizeof( channel[ channelNumber ].color ), preferences.getString( NVSKeyName, "#fffe7a" ).c_str() );

    snprintf( NVSKeyName, sizeof( NVSKeyName ), "channelminimum%i", channelNumber );
    channel[ channelNumber ].minimumLevel  = preferences.getFloat( NVSKeyName, 0  );

    ledcAttachPin( channel[channelNumber].pin, channelNumber);
  }

  setupDimmerPWMfrequency( preferences.getDouble( "pwmfrequency", LEDC_MAXIMUM_FREQ ),
                           preferences.getUInt( "pwmdepth", LEDC_NUMBER_OF_BIT ) );

  leds.setState( LIGHTS_AUTO );

  xLastWakeTime = xTaskGetTickCount();

  ESP_LOGI( TAG, "Lights running after %.1f seconds.", millis() / 1000.0 );

  while (1)
  {
    if ( leds.state() != LIGHTS_AUTO )
    {
      uint16_t pwmValue = ( leds.state() == LIGHTS_OFF ) ? 0 : ledcMaxValue;
      float percentage = ( pwmValue == 0 ) ? 0 : 100;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        channel[channelNumber].currentPercentage = percentage;
        ledcWrite( channelNumber, pwmValue );
      }
    }
    else
    {
      struct timeval microSecondTime;

      gettimeofday( &microSecondTime, NULL );

      struct tm *localTime;

      localTime = localtime( &microSecondTime.tv_sec );

      suseconds_t milliSecondsToday = ( localTime->tm_hour       * 3600000U ) +
                                      ( localTime->tm_min        * 60000U ) +
                                      ( localTime->tm_sec        * 1000U ) +
                                      ( microSecondTime.tv_usec  / 1000U );

      if ( milliSecondsToday ) /* to solve flashing at 00:00:000 due to the fact that the first timer has no predecessor */
      {
        for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
        {
          uint8_t thisTimer = 0;

          while ( channel[channelNumber].timer[thisTimer].time * 1000U < milliSecondsToday )
          {
            thisTimer++;
          }

          float newPercentage;

          /* only do a lot of math if really neccesary */
          if ( channel[channelNumber].timer[thisTimer].percentage != channel[channelNumber].timer[thisTimer - 1].percentage )
          {
            newPercentage = mapFloat( milliSecondsToday,
                                      channel[channelNumber].timer[thisTimer - 1].time * 1000U,
                                      channel[channelNumber].timer[thisTimer].time * 1000U,
                                      channel[channelNumber].timer[thisTimer - 1].percentage,
                                      channel[channelNumber].timer[thisTimer].percentage );
          }
          else
          {
            /* timers are equal so no math neccesary */
            newPercentage = channel[channelNumber].timer[thisTimer].percentage;
          }

          /* check if channel has a minimum set */
          if ( !MOON_SIMULATOR && newPercentage < channel[channelNumber].minimumLevel )
            newPercentage = channel[channelNumber].minimumLevel;

          /* calculate moon light */
          if ( MOON_SIMULATOR && newPercentage < ( channel[channelNumber].minimumLevel * moonData.percentLit ) )
            newPercentage = channel[channelNumber].minimumLevel * moonData.percentLit;

          /* done, set the channel */
          channel[channelNumber].currentPercentage = newPercentage;
          ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage,
                                              0,
                                              100,
                                              0,
                                              ledcMaxValue ) );
        }
      }
    }
    vTaskDelayUntil( &xLastWakeTime, dimmerTaskdelayTime );
  }
}

bool defaultTimersLoaded()
{
  //find 'default.aqu' on selected storage and if present load the timerdata from this file
  //return true on success
  //return false on error

  if ( !FFat.exists( defaultTimerFile ) )
  {
    ESP_LOGI( TAG, "No default timer file found. [%s]", defaultTimerFile );
    return false;
  }

  File f = FFat.open( defaultTimerFile, "r" );

  if ( !f.available() )
  {
    ESP_LOGI( TAG, "Error opening default timer file. [%s]", defaultTimerFile );
    return false;
  }
  String lineBuf;
  byte currentTimer = 0;
  byte thisChannel;
  while ( f.position() < f.size() )
  {
    lineBuf = f.readStringUntil( '\n' );
    if ( lineBuf.indexOf( "[" ) > -1 )
    {
      String thisChannelStr;
      thisChannelStr = lineBuf.substring( lineBuf.indexOf("[") + 1, lineBuf.indexOf("]") );
      thisChannel = thisChannelStr.toInt();
      currentTimer = 0;
    }
    else if ( currentTimer < MAX_TIMERS - 1 )
    {
      String thisPercentage;
      String thisTime;
      thisTime = lineBuf.substring( 0, lineBuf.indexOf(",") );
      thisPercentage = lineBuf.substring( lineBuf.indexOf(",") + 1 );
      channel[thisChannel].timer[currentTimer].time = thisTime.toInt();
      channel[thisChannel].timer[currentTimer].percentage = thisPercentage.toInt();
      currentTimer++;
      channel[thisChannel].numberOfTimers = currentTimer;
    }

  }
  f.close();
  //add the 24:00 timers ( copy of timer percentage no: 0 )
  for ( thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
  {
    channel[thisChannel].timer[channel[thisChannel].numberOfTimers].time = 86400;
    channel[thisChannel].timer[channel[thisChannel].numberOfTimers].percentage = channel[thisChannel].timer[0].percentage;
    currentTimer++;
    channel[thisChannel].numberOfTimers = currentTimer;
  }
  return true;
}

void setEmptyTimers()
{
  for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++)
  {
    channel[channelNumber].timer[0].time = 0;
    channel[channelNumber].timer[0].percentage = 0;
    channel[channelNumber].timer[1].time = 86400;
    channel[channelNumber].timer[1].percentage = 0;
    channel[channelNumber].numberOfTimers = 1;
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
  ESP_LOGI( TAG, "PWM frequency set to %.2f kHz.", ledcActualFrequency / 1000);
  ESP_LOGI( TAG, "PWM bit depth set to %i bits.", ledcNumberOfBits);
  ESP_LOGI( TAG, "Maximum raw value set to 0x%x or %i decimal.", ledcMaxValue, ledcMaxValue);
}
