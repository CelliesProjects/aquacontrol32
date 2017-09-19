void tftTask( void * pvParameters )
{
  int tftTaskdelayTime = 1000 / UPDATE_FREQ_LEDS;

  tft.fillScreen( ILI9341_BLACK );
  while (1)
  {
    const uint16_t TFT_TEXT_COLOR   = ILI9341_YELLOW;
    const uint16_t TFT_DATE_COLOR   = ILI9341_BLUE;
    const uint16_t TFT_TEMP_COLOR   = ILI9341_WHITE;
    const uint16_t TFT_BACK_COLOR   = ILI9341_BLACK;

    const uint16_t BARS_BOTTOM      = 181;
    const uint16_t BARS_HEIGHT      = 180;
    const uint16_t BARS_BORDER      = 10;
    const uint16_t BARS_WIDTH       = ILI9341_TFTHEIGHT / 5;
    const float    HEIGHT_FACTOR    = BARS_HEIGHT / 100.0;

    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      //convert from String to RGB color
      //https://stackoverflow.com/questions/2342114/extracting-rgb-color-components-from-integer-value/2342149#2342149
      int r, g, b;
      uint32_t color = strtol( &channel[thisChannel].color[1], NULL, 16 );
      r = ( color & 0xFF0000 ) >> 16; // Filter the 'red' bits from color. Then shift right by 16 bits.
      g = ( color & 0x00FF00 ) >> 8;  // Filter the 'green' bits from color. Then shift right by 8 bits.
      b = ( color & 0x0000FF );       // Filter the 'blue' bits from color. No shift needed.

      // redraw the top part of the bar
      tft.fillRect( thisChannel * BARS_WIDTH + BARS_BORDER,
                    BARS_BOTTOM - BARS_HEIGHT,
                    BARS_WIDTH - BARS_BORDER * 2,
                    BARS_HEIGHT - channel[thisChannel].currentPercentage * HEIGHT_FACTOR,
                    TFT_BACK_COLOR );
/*
      //high water mark
      tft.drawFastHLine( thisChannel * BARS_WIDTH + BARS_BORDER,
                         BARS_BOTTOM - BARS_HEIGHT - 1,
                         BARS_WIDTH - BARS_BORDER * 2,
                         tft.color565( r, g, b ) );
*/
      tft.fillRect( thisChannel * BARS_WIDTH + BARS_BORDER,
                    BARS_BOTTOM - channel[thisChannel].currentPercentage * HEIGHT_FACTOR,
                    BARS_WIDTH - BARS_BORDER * 2,
                    channel[thisChannel].currentPercentage * HEIGHT_FACTOR,
                    tft.color565( r, g, b ) );

      tft.setCursor( thisChannel * BARS_WIDTH + 12, BARS_BOTTOM + 4 );
      tft.setTextSize( 1 );
      tft.setTextColor( TFT_TEXT_COLOR , TFT_BACK_COLOR );

      char buffer [9];
      snprintf( buffer, sizeof( buffer ), "%*" ".3f%%", 7, channel[thisChannel].currentPercentage );
      tft.print( buffer );
    }

    tft.setCursor( 0, BARS_BOTTOM + 20 );
    tft.setTextSize( 2 );
    tft.setTextColor( TFT_TEMP_COLOR , TFT_BACK_COLOR );

    if ( numberOfFoundSensors )
    {
      for ( byte thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++ )
      {
        tft.print( " " + String( sensor[thisSensor].temp / 16.0 ) + (char)247 + "C " );
      }
    }

    tft.setCursor( 60, BARS_BOTTOM + 40 );
    tft.setTextColor( TFT_TEMP_COLOR , TFT_BACK_COLOR );

    tft.print( lightStatus );

    vTaskDelay( tftTaskdelayTime / portTICK_PERIOD_MS );
  }
}

void setupTFT()
{
  tft.begin( 40000000, SPI );
  uint8_t x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("ILI9341 TFT Self Diagnostic: 0x"); Serial.println(x, HEX);

  tft.fillScreen(ILI9341_BLACK);
  ( readStringNVS( "tftorientation", "normal" ) == "normal" ) ? TFTorientation = TFTnormal : TFTorientation = TFTupsidedown;
  tft.setRotation( TFTorientation );
  tft.setCursor( 0, 0 );
  tft.setTextColor( ILI9341_WHITE );
  tft.setTextSize(1);
  tft.println( "TFT started.");
}
