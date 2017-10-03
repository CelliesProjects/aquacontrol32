void tftTask( void * pvParameters )
{
  const bool     TFT_SHOW_RAW     = true;            /* show raw PWM values */
  const uint16_t TFT_TEXT_COLOR   = ILI9341_YELLOW;
  const uint16_t TFT_DATE_COLOR   = ILI9341_WHITE;
  const uint16_t TFT_TEMP_COLOR   = ILI9341_WHITE;
  const uint16_t TFT_BACK_COLOR   = ILI9341_BLACK;

  SPI.begin( SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN );
  SPI.setFrequency( 60000000 );

  tft.begin( 35000000, SPI );
  uint8_t x = tft.readcommand8( ILI9341_RDSELFDIAG );
  Serial.print( "ILI9341 TFT Self Diagnostic: 0x" ); Serial.println( x, HEX );

  tft.fillScreen( TFT_BACK_COLOR );

  //setup backlight pwm
  const uint8_t BACKLIGHT_BITDEPTH = 16; /*max 16 bits */

  ledcAttachPin( TFT_BACKLIGHT_PIN, NUMBER_OF_CHANNELS );
  double backlightFrequency = ledcSetup( NUMBER_OF_CHANNELS , 10000, BACKLIGHT_BITDEPTH );

  uint16_t backlightMaxvalue = ( 0x00000001 << BACKLIGHT_BITDEPTH ) - 1;

  ( readStringNVS( "tftorientation", "normal" ) == "normal" ) ? tftOrientation = TFT_ORIENTATION_NORMAL : tftOrientation = TFT_ORIENTATION_UPSIDEDOWN;
  tft.setRotation( tftOrientation );

  int tftTaskdelayTime = 1000 / UPDATE_FREQ_TFT;

  while (1)
  {
    const uint16_t BARS_BOTTOM      = 205;
    const uint16_t BARS_HEIGHT      = BARS_BOTTOM;
    const uint16_t BARS_BORDER      = 10;
    const uint16_t BARS_WIDTH       = ILI9341_TFTHEIGHT / 5;
    const float    HEIGHT_FACTOR    = BARS_HEIGHT / 100.0;

    ledcWrite( NUMBER_OF_CHANNELS, map( tftBrightness, 0, 100, 0, backlightMaxvalue ) );

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

      tft.setCursor( thisChannel * BARS_WIDTH + 9, BARS_BOTTOM + 4 );
      tft.setTextSize( 1 );
      tft.setTextColor( TFT_TEXT_COLOR , TFT_BACK_COLOR );

      char buffer [8];
      if ( TFT_SHOW_RAW )
      {
        snprintf( buffer, sizeof( buffer ), " 0x%04X", ledcRead( thisChannel ) );
      }
      else
      {
        snprintf( buffer, sizeof( buffer ), "%*" ".3f%%", 7, channel[thisChannel].currentPercentage );
      }
      tft.print( buffer );
    }

    tft.setTextSize( 2 );
    if ( numberOfFoundSensors )
    {
      for ( byte thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++ )
      {
        tft.setCursor( 0, BARS_BOTTOM + 15 );
        tft.setTextColor( TFT_TEMP_COLOR , TFT_BACK_COLOR );
        tft.print( " " + String( sensor[thisSensor].temp / 16.0 ) + (char)247 + "C " );
      }
    }
    else
    {
      struct tm timeinfo;
      getLocalTime( &timeinfo );
      tft.setCursor( 18, BARS_BOTTOM + 15 );
      tft.setTextColor( TFT_DATE_COLOR , TFT_BACK_COLOR );
      tft.print( asctime( &timeinfo ) );
    }
    vTaskDelay( tftTaskdelayTime / portTICK_PERIOD_MS );
  }
}
