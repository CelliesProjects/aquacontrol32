void tftTask( void * pvParameters )
{
  while (1)
  {
    tft.setTextColor( ILI9341_YELLOW , ILI9341_BLACK );
    tft.setCursor( 15, 0 );
    tft.setTextSize( 2 );

    struct tm timeinfo;
    if ( !getLocalTime( &timeinfo ) )
    {
      tft.println( "Failed to obtain time" );
    }
    else
    {
      tft.println( asctime( &timeinfo ) );
    }

#define BARS_BOTTOM    170
#define BARS_HEIGHT    150
#define BARS_WIDTH     ILI9341_TFTHEIGHT / 5
#define HEIGHT_FACTOR  BARS_HEIGHT / 100

    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      // redraw the top part of the bar
      tft.fillRect( thisChannel * BARS_WIDTH + 5,
                    BARS_BOTTOM - BARS_HEIGHT,
                    BARS_WIDTH - 10,
                    BARS_HEIGHT - channel[thisChannel].currentPercentage * HEIGHT_FACTOR,
                    ILI9341_WHITE );

      //https://stackoverflow.com/questions/2342114/extracting-rgb-color-components-from-integer-value/2342149#2342149
      int r, g, b;
      //convert from String to color
      int color = strtol( &channel[thisChannel].color[1], NULL, 16 );
      r = ( color & 0xFF0000 ) >> 16; // Filter the 'red' bits from color. Then shift right by 16 bits.
      g = ( color & 0x00FF00 ) >> 8;  // Filter the 'green' bits from color. Then shift right by 8 bits.
      b = ( color & 0x0000FF );       // Filter the 'blue' bits from color. No shift needed.

      tft.fillRect( thisChannel * BARS_WIDTH + 5,
                    BARS_BOTTOM - channel[thisChannel].currentPercentage * HEIGHT_FACTOR,
                    BARS_WIDTH - 10,
                    channel[thisChannel].currentPercentage * HEIGHT_FACTOR,
                    tft.color565( r, g, b ) );
      tft.setTextSize( 1 );
      tft.setCursor( thisChannel * BARS_WIDTH + 10, BARS_BOTTOM + 4 );
      char buffer [9];
      snprintf( buffer, sizeof( buffer ), "%*" ".2f%%", 6, channel[thisChannel].currentPercentage );
      tft.print( buffer );
    }

    tft.setCursor( 0, BARS_BOTTOM + 20 );
    tft.setTextSize( 2 );
    if ( numberOfFoundSensors )
    {
      for ( byte thisSensor = 1; thisSensor <= numberOfFoundSensors; thisSensor++ )
      {
        tft.print( " " + String( sensor[thisSensor].temp / 16.0 ) + "C " );
      }
    }

    vTaskDelay( 500 / portTICK_PERIOD_MS );
  }
}
