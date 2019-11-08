void ntpTask( void * pvParameters )
{
  const char * NTPpool         = "pool.ntp.org";
  const char * defaultTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
  static char NTPpoolAdress[20];

  if ( COUNTRY_CODE_ISO_3166 )
    snprintf( NTPpoolAdress, sizeof( NTPpoolAdress ), "%s.%s", COUNTRY_CODE_ISO_3166, NTPpool );
  else
    snprintf( NTPpoolAdress, sizeof( NTPpoolAdress ), "0.%s", NTPpool );

  configTzTime( preferences.getString( "timezone", defaultTimezone ).c_str(), NTPpoolAdress );

  struct tm timeinfo = {0};

  while ( !getLocalTime( &timeinfo, 0 ) )
    vTaskDelay( 10 / portTICK_PERIOD_MS );

  gettimeofday( &systemStart, NULL );

  /* save reset reason */
  if ( preferences.getString("bootlog").equalsIgnoreCase("on") ) {
    char content[60];
    snprintf( content, sizeof( content ), " %s,%s", resetString( 0 ), resetString( 1 ) );
    logger.appendToFile( "/reset_reasons.txt", HUMAN_TIME, content );
  }

  ESP_LOGI( TAG, "NTP sync from '%s'", NTPpoolAdress );

  /* start time dependent tasks */

  logger.startSensors( NUMBER_OF_SENSORS, ONEWIRE_PIN, SENSOR_HWTIMER );


  BaseType_t xReturned;

  xReturned = xTaskCreatePinnedToCore(
                dimmerTask,                     /* Function to implement the task */
                "dimmerTask",                   /* Name of the task */
                3000,                           /* Stack size in words */
                NULL,                           /* Task input parameter */
                dimmerTaskPriority,             /* Priority of the task */
                &xDimmerTaskHandle,             /* Task handle. */
                1);                             /* Core where the task should run */

  vTaskDelete( NULL );
}
