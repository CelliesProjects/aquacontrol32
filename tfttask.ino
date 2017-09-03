void tftTask( void * pvParameters )
{
  while (1)
  {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
      tft.println("Failed to obtain time");
      return;
    }
    tft.setCursor(0, 0);
    tft.setTextColor( ILI9341_YELLOW , ILI9341_BLACK );
    tft.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    tft.println( ledcRead( 0 ) );
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
