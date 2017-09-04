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
    tft.setCursor( 56, 0 );
    tft.setTextSize( 1 );
    tft.setTextColor( ILI9341_YELLOW , ILI9341_BLACK );
    tft.println( &timeinfo, "%A, %B %d %Y %H:%M:%S" );
    tft.setCursor( 0, 120 );
    tft.setTextSize( 2 );
    if ( numberOfFoundSensors )
    {
      for ( byte thisSensor = 1; thisSensor <= numberOfFoundSensors; thisSensor++ )
      {
        tft.print( " " + String( sensor[thisSensor].temp / 16.0 ) + "C " );
      }
    }
    #define BARS_BOTTOM 110
    int barWidth = ILI9341_TFTHEIGHT / 5;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      tft.fillRect( thisChannel * barWidth + 5,
                    BARS_BOTTOM - 100,
                    barWidth - 10,
                    100 - channel[thisChannel].currentPercentage,
                    ILI9341_WHITE );
      tft.fillRect( thisChannel * barWidth + 5,
                    BARS_BOTTOM - channel[thisChannel].currentPercentage,
                    barWidth - 10,
                    channel[thisChannel].currentPercentage,
                    ILI9341_BLUE );
      tft.setTextSize(1);
      tft.setCursor( thisChannel * barWidth + 10, BARS_BOTTOM + 1 );
      char buffer [9];
      snprintf( buffer, sizeof(buffer), "%*6.3f%%", 6, channel[thisChannel].currentPercentage );
      tft.print( buffer );
    }

    vTaskDelay( 233 / portTICK_PERIOD_MS );
  }
}
