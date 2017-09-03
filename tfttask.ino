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
    tft.setTextSize(1);
    tft.setTextColor( ILI9341_YELLOW , ILI9341_BLACK );
    tft.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    tft.setTextSize(2);
    tft.println( ledcRead( 0 ) );
    if ( numberOfFoundSensors )
    {
      for ( byte thisSensor = 1; thisSensor <= numberOfFoundSensors; thisSensor++ )
      {
        tft.print( String( sensor[thisSensor].temp / 16.0 ) + "C  " );
      }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
