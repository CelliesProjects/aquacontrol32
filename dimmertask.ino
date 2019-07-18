void IRAM_ATTR dimmerTask ( void * pvParameters )
{
  const TickType_t dimmerTaskdelayTime = 1000 / UPDATE_FREQ_LEDS / portTICK_PERIOD_MS;

  TickType_t xLastWakeTime;

  if ( defaultTimersLoaded() ) ESP_LOGI( TAG, "Default timers loaded." );
  else {
    ESP_LOGI( TAG, "No timers loaded." );
    setEmptyTimers();
  }

  /* setup pwm on leds */
  channel[ 0 ].pin = LED0_PIN;
  channel[ 1 ].pin = LED1_PIN;
  channel[ 2 ].pin = LED2_PIN;
  channel[ 3 ].pin = LED3_PIN;
  channel[ 4 ].pin = LED4_PIN;
  for ( uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++ ) {
    char NVSKeyName[32];

    snprintf( NVSKeyName, sizeof( NVSKeyName ), "channelname%i", num );
    snprintf( channel[ num ].name, sizeof( channel[ num ].name ), preferences.getString( NVSKeyName, "channelname" ).c_str() );

    snprintf( NVSKeyName, sizeof( NVSKeyName ), "channelcolor%i", num );
    snprintf( channel[ num ].color, sizeof( channel[ num ].color ), preferences.getString( NVSKeyName, "#fffe7a" ).c_str() );

    snprintf( NVSKeyName, sizeof( NVSKeyName ), "channelminimum%i", num );
    channel[ num ].minimumLevel  = preferences.getFloat( NVSKeyName, 0  );

    ledcAttachPin( channel[num].pin, num);
  }

  setupDimmerPWMfrequency( preferences.getDouble( "pwmfrequency", LEDC_MAXIMUM_FREQ ),
                           preferences.getUInt( "pwmdepth", LEDC_NUMBER_OF_BIT ) );
  leds.setState( LIGHTS_AUTO );

  ESP_LOGI( TAG, "Lights running after %i ms.", millis() );

  xLastWakeTime = xTaskGetTickCount();

  while (1) {

    if ( leds.state() != LIGHTS_AUTO ) {
      lightState_t currentState = leds.state();
      uint16_t pwmValue = ( currentState == LIGHTS_OFF ) ? 0 : ledcMaxValue;
      float percentage = ( pwmValue == 0 ) ? 0 : 100;
      for ( uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++ ) {
        channel[num].currentPercentage = percentage;
        ledcWrite( num, pwmValue );
      }
      while ( leds.state() == currentState ) delay( 100 );
    }
    else {
      struct timeval microSecondTime;
      gettimeofday( &microSecondTime, NULL );

      struct tm *localTime;
      localTime = localtime( &microSecondTime.tv_sec );

      suseconds_t milliSecondsToday = ( localTime->tm_hour       * 3600000U ) +
                                      ( localTime->tm_min        * 60000U ) +
                                      ( localTime->tm_sec        * 1000U ) +
                                      ( microSecondTime.tv_usec  / 1000U );

      if ( milliSecondsToday ) { /* to solve flashing at 00:00:000 due to the fact that the first timer has no predecessor */
        for ( uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++ ) {
          uint8_t thisTimer = 0;

          while ( channel[num].timer[thisTimer].time * 1000U < milliSecondsToday )
            thisTimer++;

          float newPercentage;

          /* only do a lot of math if really neccesary */
          if ( channel[num].timer[thisTimer].percentage != channel[num].timer[thisTimer - 1].percentage ) {
            newPercentage = mapFloat( milliSecondsToday,
                                      channel[num].timer[thisTimer - 1].time * 1000U,
                                      channel[num].timer[thisTimer].time * 1000U,
                                      channel[num].timer[thisTimer - 1].percentage,
                                      channel[num].timer[thisTimer].percentage );
          }
          else {
            /* timers are equal so no math neccesary */
            newPercentage = channel[num].timer[thisTimer].percentage;
          }

          /* check if channel has a minimum set */
          if ( !MOON_SIMULATOR && newPercentage < channel[num].minimumLevel )
            newPercentage = channel[num].minimumLevel;

          /* calculate moon light */
          if ( MOON_SIMULATOR && newPercentage < ( channel[num].minimumLevel * moonData.percentLit ) )
            newPercentage = channel[num].minimumLevel * moonData.percentLit;

          /* done, set the channel */
          channel[num].currentPercentage = newPercentage;
          ledcWrite( num, mapFloat( channel[num].currentPercentage,
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

bool defaultTimersLoaded() {
  //find 'default.aqu' on selected storage and if present load the timerdata from this file
  //return true on success
  //return false on error

  if ( !FFat.exists( defaultTimerFile ) ) {
    ESP_LOGI( TAG, "No default timer file found. [%s]", defaultTimerFile );
    return false;
  }

  File f = FFat.open( defaultTimerFile, "r" );
  if ( !f.available() ) {
    ESP_LOGI( TAG, "Error opening default timer file. [%s]", defaultTimerFile );
    return false;
  }
  byte currentTimer = 0;
  uint8_t chan;
  //String data;
  while ( f.position() < f.size() ) {
    String data = f.readStringUntil( '\n' );
    if ( 0 == data.indexOf( "[" ) ) {
      chan = data.substring( 1, 3 ).toInt();
      currentTimer = 0;
    }
    else if ( currentTimer < MAX_TIMERS - 1 ) {
      channel[chan].timer[currentTimer].time = data.substring( 0, data.indexOf(",") ).toInt();
      channel[chan].timer[currentTimer].percentage = data.substring( data.indexOf(",") + 1 ).toInt();
      currentTimer++;
      channel[chan].numberOfTimers = currentTimer;
    }

  }
  f.close();
  //add the 24:00 timers ( copy of timer percentage no: 0 )
  for (chan = 0; chan < NUMBER_OF_CHANNELS; chan++ ) {
    channel[chan].timer[channel[chan].numberOfTimers].time = 86400;
    channel[chan].timer[channel[chan].numberOfTimers].percentage = channel[chan].timer[0].percentage;
    currentTimer++;
    channel[chan].numberOfTimers = currentTimer;
  }
  return true;
}

void setEmptyTimers() {
  for ( uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++) {
    channel[num].timer[0] = {0, 0};
    channel[num].timer[1] = {86400, 0};
    channel[num].numberOfTimers = 1;
  }
}

void setupDimmerPWMfrequency( const double frequency, const uint8_t numberOfBits ) {
  /* Setup timers and pwm bit depth */
  for ( uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++ ) {
    ledcActualFrequency = ledcSetup( num, frequency, numberOfBits );
  }
  ledcMaxValue = ( 0x00000001 << numberOfBits ) - 1;
  ledcNumberOfBits = numberOfBits;
  ESP_LOGI( TAG, "PWM frequency set to %.2f kHz.", ledcActualFrequency / 1000);
  ESP_LOGI( TAG, "PWM bit depth set to %i bits.", ledcNumberOfBits);
  ESP_LOGI( TAG, "Maximum raw value set to 0x%x or %i decimal.", ledcMaxValue, ledcMaxValue);
}
