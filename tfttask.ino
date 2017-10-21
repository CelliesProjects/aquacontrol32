void tftTask( void * pvParameters )
{
  const bool     TFT_SHOW_RAW           = true;            /* show raw PWM values */
  const uint16_t TFT_TEXT_COLOR         = ILI9341_YELLOW;
  const uint16_t TFT_DATE_COLOR         = ILI9341_WHITE;
  const uint16_t TFT_TEMP_COLOR         = ILI9341_WHITE;
  const uint16_t TFT_BACK_COLOR         = ILI9341_BLACK;
  const uint8_t  TFT_BACKLIGHT_BITDEPTH = 16;               /*min 11 bits, max 16 bits */

  const uint32_t tftTaskdelayTime = 1000 / UPDATE_FREQ_TFT;

  tft.begin( 20000000, SPI );
  uint8_t x = tft.readcommand8( ILI9341_RDSELFDIAG );
  Serial.printf( "ILI9341 TFT Self Diagnostic: 0x%x\n", x );
  if ( x == 0xff )
  {
    Serial.println( "No ILI9341 found. Quitting task..." );
    vTaskDelete( NULL );
  }

  tft.fillScreen( TFT_BACK_COLOR );

  //setup backlight pwm

  ledcAttachPin( TFT_BACKLIGHT_PIN, NUMBER_OF_CHANNELS );
  double backlightFrequency = ledcSetup( NUMBER_OF_CHANNELS , LEDC_MAXIMUM_FREQ, TFT_BACKLIGHT_BITDEPTH );

  uint16_t backlightMaxvalue = ( 0x00000001 << TFT_BACKLIGHT_BITDEPTH ) - 1;

  ( readStringNVS( "tftorientation", "normal" ) == "normal" ) ? tftOrientation = TFT_ORIENTATION_NORMAL : tftOrientation = TFT_ORIENTATION_UPSIDEDOWN;
  tft.setRotation( tftOrientation );

  tftBrightness = readInt8NVS( "tftbrightness", tftBrightness );

  while (1)
  {
    const uint16_t BARS_BOTTOM      = 205;
    const uint16_t BARS_HEIGHT      = BARS_BOTTOM;
    const uint16_t BARS_BORDER      = 10;
    const uint16_t BARS_WIDTH       = ILI9341_TFTHEIGHT / 5;
    const float    HEIGHT_FACTOR    = BARS_HEIGHT / 100.0;

    uint32_t average = 0;

    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      average += ledcRead( channelNumber );
    }
    average = average / NUMBER_OF_CHANNELS;

    uint16_t rawBrightness = map( tftBrightness, 0, 100, 0, ( 0x00000001 << TFT_BACKLIGHT_BITDEPTH ) - 1 );
    ledcWrite( NUMBER_OF_CHANNELS, ( average > rawBrightness ) ? rawBrightness : average );

    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      uint8_t r, g, b;
      uint32_t color = strtol( &channel[channelNumber].color[1], NULL, 16 );
      r = ( color & 0xFF0000 ) >> 16; // Filter the 'red' bits from color. Then shift right by 16 bits.
      g = ( color & 0x00FF00 ) >> 8;  // Filter the 'green' bits from color. Then shift right by 8 bits.
      b = ( color & 0x0000FF );       // Filter the 'blue' bits from color. No shift needed.

      // redraw the top part of the bar
      tft.fillRect( channelNumber * BARS_WIDTH + BARS_BORDER,
                    BARS_BOTTOM - BARS_HEIGHT,
                    BARS_WIDTH - BARS_BORDER * 2,
                    BARS_HEIGHT - channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                    TFT_BACK_COLOR );
      /*
            //high water mark
            tft.drawFastHLine( channelNumber * BARS_WIDTH + BARS_BORDER,
                               BARS_BOTTOM - BARS_HEIGHT - 1,
                               BARS_WIDTH - BARS_BORDER * 2,
                               tft.color565( r, g, b ) );
      */
      tft.fillRect( channelNumber * BARS_WIDTH + BARS_BORDER,
                    BARS_BOTTOM - channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                    BARS_WIDTH - BARS_BORDER * 2,
                    channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                    tft.color565( r, g, b ) );

      tft.setCursor( channelNumber * BARS_WIDTH + 9, BARS_BOTTOM + 4 );
      tft.setTextSize( 1 );
      tft.setTextColor( tft.color565( r, g, b ) , TFT_BACK_COLOR );

      char content[8];
      if ( TFT_SHOW_RAW )
      {
        snprintf( content, sizeof( content ), " 0x%04X", ledcRead( channelNumber ) );
      }
      else
      {
        snprintf( content, sizeof( content ), "%*" ".3f%%", 7, channel[channelNumber].currentPercentage );
      }
      tft.print( content );
    }

    tft.setTextSize( 2 );
    if ( numberOfFoundSensors )
    {
      for ( uint8_t thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++ )
      {
        tft.setCursor( thisSensor * 90, BARS_BOTTOM + 15 );
        tft.setTextColor( TFT_TEMP_COLOR , TFT_BACK_COLOR );
        tft.printf( " %.1f%cC", sensor[thisSensor].temp / 16.0, char(247) );
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
