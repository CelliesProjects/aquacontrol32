void IRAM_ATTR moonSimtask ( void * pvParameters )
{
  const TickType_t moonSimdelayTime  = ( 1000 * 60 * 3 ) / portTICK_PERIOD_MS;

  while (1)
  {
    time_t now;
    time( &now );
    struct tm timeinfo;
    gmtime_r( &now, &timeinfo ); /* moon phase is calculated using UTC! */

    double hour = timeinfo.tm_hour + mapFloat( ( timeinfo.tm_min * 60 ) + timeinfo.tm_sec, 1, 3600, 0, 1 );

    moonData = MoonPhase.getInfo( 1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday, hour );

    ESP_LOGI( TAG, "Moon phase updated: %i degrees %.6f%% lit", moonData.angle, moonData.percentLit * 100 );

    vTaskDelay( moonSimdelayTime / portTICK_PERIOD_MS );
  }
}

