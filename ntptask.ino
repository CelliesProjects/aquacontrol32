void setupNTP( void * pvParameters )
{
  String NTPpoolAdress = COUNTRY_CODE_ISO_3166;

  NTPpoolAdress += ".pool.ntp.org";

  tft.println( "Getting time from " + NTPpoolAdress );

  configTzTime( readStringNVS( "timezone", "CET-1CEST,M3.5.0/2,M10.5.0/3" ).c_str(),
                NTPpoolAdress.c_str(), "0.pool.ntp.org", "1.pool.ntp.org" );

  time_t now;
  struct tm timeinfo;

  while ( timeinfo.tm_year < ( 2016 - 1900 ) ) {
    vTaskDelay( 50 / portTICK_PERIOD_MS );
    time( &now );
    localtime_r( &now, &timeinfo );
  }
  gettimeofday( &systemStart, NULL );

  Serial.printf( "NTP sync @ %s\n", asctime( localtime( &systemStart.tv_sec ) ) );

  /* start time dependent tasks */

  xTaskCreatePinnedToCore(
    dimmerTask,                     /* Function to implement the task */
    "dimmerTask",                   /* Name of the task */
    2000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    7,                              /* Priority of the task */
    &x_dimmerTaskHandle,            /* Task handle. */
    1);                             /* Core where the task should run */

  xTaskCreatePinnedToCore(
    loggerTask,                     /* Function to implement the task */
    "loggerTask",                   /* Name of the task */
    3000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    0,                              /* Priority of the task */
    &x_loggerTaskHandle,            /* Task handle. */
    1);                             /* Core where the task should run */

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
