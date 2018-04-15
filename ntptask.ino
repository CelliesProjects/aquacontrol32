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

  time_t now;
  struct tm timeinfo;

  while ( timeinfo.tm_year < ( 2016 - 1900 ) ) {
    vTaskDelay( 50 / portTICK_PERIOD_MS );
    time( &now );
    localtime_r( &now, &timeinfo );
  }
  gettimeofday( &systemStart, NULL );

  ESP_LOGI( TAG, "NTP sync @ %s", asctime( localtime( &systemStart.tv_sec ) ) );

  /* start time dependent tasks */

  BaseType_t xReturned;

  xReturned = xTaskCreatePinnedToCore(
                dimmerTask,                     /* Function to implement the task */
                "dimmerTask",                   /* Name of the task */
                2500,                           /* Stack size in words */
                NULL,                           /* Task input parameter */
                dimmerTaskPriority,             /* Priority of the task */
                &xDimmerTaskHandle,             /* Task handle. */
                1);                             /* Core where the task should run */

  ESP_LOGI( TAG, "DimmerTask %s.", ( xReturned == pdPASS ) ? "started" : "failed" );

  if ( LOG_FILES )
  {
    xReturned = xTaskCreatePinnedToCore(
                  loggerTask,                     /* Function to implement the task */
                  "loggerTask",                   /* Name of the task */
                  3000,                           /* Stack size in words */
                  NULL,                           /* Task input parameter */
                  loggerTaskPriority,             /* Priority of the task */
                  &xLoggerTaskHandle,             /* Task handle. */
                  1);                             /* Core where the task should run */

    ESP_LOGI( TAG, "LoggerTask %s.", ( xReturned == pdPASS ) ? "started" : "failed" );
  }
  vTaskDelay( 10 / portTICK_PERIOD_MS );

  xReturned = xTaskCreatePinnedToCore(
                versionCheck,                     /* Function to implement the task */
                "versionCheck",                   /* Name of the task */
                6000,                             /* Stack size in words */
                NULL,                             /* Task input parameter */
                versionCheckPriority,             /* Priority of the task */
                NULL,                             /* Task handle. */
                1);                               /* Core where the task should run */

  ESP_LOGI( TAG, "VersionCheck %s.", ( xReturned == pdPASS ) ? "started" : "failed" );

  vTaskDelete( NULL );

  //https://www.ibm.com/developerworks/aix/library/au-aix-posix/index.html#artdownload
  //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/README.md
  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c
  //https://www.di-mgt.com.au/wclock/tz.html
  //http://www.catb.org/esr/time-programming/
  //http://www.lucadentella.it/en/2017/05/11/esp32-17-sntp/
  //https://github.com/espressif/esp-idf/blob/master/components/lwip/apps/sntp/sntp.c
  //https://github.com/hwstar/freertos-avr/blob/master/include/time.h

  /* https://stackoverflow.com/questions/3673226/how-to-print-time-in-format-2009-08-10-181754-811 */

}
