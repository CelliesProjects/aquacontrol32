void IRAM_ATTR dimmerTask ( void * pvParameters )
{
  const uint64_t dimmerTaskdelayTime = 1000U / UPDATE_FREQ_LEDS;

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
    char buff[32];

    snprintf( buff, sizeof( buff ), "channelname%i", channelNumber );
    channel[ channelNumber ].name          = preferences.getString( buff, "channelname" );

    snprintf( buff, sizeof( buff ), "channelcolor%i", channelNumber );
    channel[ channelNumber ].color         = preferences.getString( buff, "#fffe7a" );

    snprintf( buff, sizeof( buff ), "channelminimum%i", channelNumber );
    //channel[ channelNumber ].minimumLevel  = readFloatNVS( buff, 0 );
    channel[ channelNumber ].minimumLevel  = preferences.getFloat( buff, 0  );

    ledcAttachPin( channel[channelNumber].pin, channelNumber);
  }

  setupDimmerPWMfrequency( preferences.getDouble( "pwmfrequency", LEDC_MAXIMUM_FREQ ),
                           preferences.getUInt( "pwmdepth", LEDC_NUMBER_OF_BIT ) );

  lightStatus = LIGHTS_AUTO;

  xLastWakeTime = xTaskGetTickCount();

  while (1)
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
        if ( channel[channelNumber].timer[thisTimer].time != channel[channelNumber].timer[thisTimer - 1].time )
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
        if ( newPercentage < channel[channelNumber].minimumLevel )
        {
          newPercentage = channel[channelNumber].minimumLevel;
        }

        /* done, set the channel */
        channel[channelNumber].currentPercentage = newPercentage;
        ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage,
                                            0,
                                            100,
                                            0,
                                            ledcMaxValue ) );
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
  ESP_LOGI( TAG, "PWM frequency set to %.2f kHz.", ledcActualFrequency / 1000);
  ESP_LOGI( TAG, "PWM bit depth set to %i bits.", ledcNumberOfBits);
  ESP_LOGI( TAG, "Maximum raw value set to 0x%x or %i decimal.", ledcMaxValue, ledcMaxValue);
}

static inline __attribute__((always_inline)) float mapFloat( const float x, const float in_min, const float in_max, const float out_min, const float out_max)
{
  return ( x - in_min ) * ( out_max - out_min ) / ( in_max - in_min ) + out_min;
}

static inline __attribute__((always_inline)) void lightsOn()
{
  vTaskSuspend( xDimmerTaskHandle );
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    channel[channelNumber].currentPercentage = 100;
    ledcWrite( channelNumber, ledcMaxValue );
  }
  lightStatus = LIGHTS_ON;
}

static inline __attribute__((always_inline)) void lightsOff()
{
  vTaskSuspend( xDimmerTaskHandle );
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    channel[channelNumber].currentPercentage = 0;
    ledcWrite( channelNumber, 0 );
  }
  lightStatus = LIGHTS_OFF;
}

static inline __attribute__((always_inline)) void lightsAuto()
{
  lightStatus = LIGHTS_AUTO;
  vTaskResume( xDimmerTaskHandle );
}
