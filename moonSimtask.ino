void IRAM_ATTR moonSimtask ( void * pvParameters )
{
  const TickType_t moonSimdelayTime  = ( 1000 * 60 * 3 ) / portTICK_PERIOD_MS;

  while (1)
  {
    moonData = moonPhase.getInfo();

    ESP_LOGI( TAG, "Moon phase updated: %i degrees %.6f%% lit", moonData.angle, moonData.percentLit * 100 );

    vTaskDelay( moonSimdelayTime / portTICK_PERIOD_MS );
  }
}
