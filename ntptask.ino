void ntpTask( void * pvParameters )
{
  const char * NTPpool         = "pool.ntp.org";
  const char * defaultTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
  static char NTPpoolAdress[20];

  if ( COUNTRY_CODE_ISO_3166 )
  {
    snprintf( NTPpoolAdress, sizeof( NTPpoolAdress ), "%s.%s", COUNTRY_CODE_ISO_3166, NTPpool );
  }
  else
  {
    snprintf( NTPpoolAdress, sizeof( NTPpoolAdress ), "0.%s", NTPpool );
  }

  ESP_LOGI( TAG, "NTP syncing with %s.", NTPpoolAdress );

  configTzTime( preferences.getString( "timezone", defaultTimezone ).c_str(), NTPpoolAdress );

  struct tm timeinfo = {};

  while ( !timeinfo.tm_year )
  {
    vTaskDelay( 50 / portTICK_PERIOD_MS );
    getLocalTime( &timeinfo );
  }

  gettimeofday( &systemStart, NULL );

  ESP_LOGI( TAG, "NTP sync @ %s", asctime( localtime( &systemStart.tv_sec ) ) );

  /* start time dependent tasks */

  sensor.startSensors();

  char timestr[20];
  char content[100];

  strftime( timestr , sizeof( timestr ), "%x %X", &timeinfo );
  snprintf( content, sizeof( content ), "%s %s %s ", timestr, sensor.resetString( 0 ), sensor.resetString( 1 ) );

  sensor.appendToFile( FFat, "/resetreasons.txt", content );

  BaseType_t xReturned;

  xReturned = xTaskCreatePinnedToCore(
                dimmerTask,                     /* Function to implement the task */
                "dimmerTask",                   /* Name of the task */
                3000,                           /* Stack size in words */
                NULL,                           /* Task input parameter */
                dimmerTaskPriority,             /* Priority of the task */
                &xDimmerTaskHandle,             /* Task handle. */
                1);                             /* Core where the task should run */

  if ( MOON_SIMULATOR )
  {
    xReturned = xTaskCreatePinnedToCore(
                  moonSimtask,                    /* Function to implement the task */
                  "moonSimtask",                  /* Name of the task */
                  2200,                           /* Stack size in words */
                  NULL,                           /* Task input parameter */
                  moonSimtaskPriority,            /* Priority of the task */
                  NULL,                           /* Task handle. */
                  1);                             /* Core where the task should run */
  }

  vTaskDelete( NULL );
}
