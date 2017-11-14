void tftTask( void * pvParameters )
{
  const bool     TFT_SHOW_RAW           = true;            /* show raw PWM values */
  const uint16_t TFT_TEXT_COLOR         = ILI9341_YELLOW;
  const uint16_t TFT_DATE_COLOR         = ILI9341_WHITE;
  const uint16_t TFT_TEMP_COLOR         = ILI9341_WHITE;
  const uint16_t TFT_BACK_COLOR         = ILI9341_BLACK;
  const uint8_t  TFT_BACKLIGHT_BITDEPTH = 16;               /*min 11 bits, max 16 bits */
  const uint8_t  TFT_BACKLIGHT_CHANNEL  = NUMBER_OF_CHANNELS;

  const time_t tftTaskdelayTime         =   ( 1000 / UPDATE_FREQ_TFT) / portTICK_PERIOD_MS;
  const time_t SPI_MutexMaxWaitTime     =                         100 / portTICK_PERIOD_MS;

  bool firstRun                         = true;

  //setup backlight pwm
  ledcAttachPin( TFT_BACKLIGHT_PIN, TFT_BACKLIGHT_CHANNEL );
  double backlightFrequency = ledcSetup( TFT_BACKLIGHT_CHANNEL , LEDC_MAXIMUM_FREQ, TFT_BACKLIGHT_BITDEPTH );

  uint16_t backlightMaxvalue = ( 0x00000001 << TFT_BACKLIGHT_BITDEPTH ) - 1;

  if ( !xSemaphoreTake( x_SPI_Mutex, SPI_MutexMaxWaitTime ) )
  {
    Serial.println( "No tft init. - No SPI bus available. Exiting task." );
    vTaskDelete( NULL );
  }
  else
  {
    tft.fillScreen( TFT_BACK_COLOR );

    tftBrightness = readInt8NVS( "tftbrightness", tftBrightness );
    ledcWrite( TFT_BACKLIGHT_CHANNEL, map( tftBrightness, 0, 100, 0, backlightMaxvalue ) );

    ( readStringNVS( "tftorientation", "normal" ) == "normal" ) ? tftOrientation = TFT_ORIENTATION_NORMAL : tftOrientation = TFT_ORIENTATION_UPSIDEDOWN;
    tft.setRotation( tftOrientation );
    while ( !x_dimmerTaskHandle )
    {
      vTaskDelay( 10 / portTICK_PERIOD_MS );
    }
    ledcWrite( TFT_BACKLIGHT_CHANNEL, 0 );
    xSemaphoreGive( x_SPI_Mutex );
  }

  while (1)
  {

    if ( !xSemaphoreTake( x_SPI_Mutex, SPI_MutexMaxWaitTime ) )
    {
      Serial.println( "Skipped tft update. SPI bus not available.");
    }
    else
    {
      const uint16_t BARS_BOTTOM      = 205;
      const uint16_t BARS_HEIGHT      = BARS_BOTTOM;
      const uint16_t BARS_BORDER      = 10;
      const uint16_t BARS_WIDTH       = ILI9341_TFTHEIGHT / 5;
      const float    HEIGHT_FACTOR    = BARS_HEIGHT / 100.0;

      if ( firstRun )
      {
        tft.fillScreen( TFT_BACK_COLOR );
        firstRun = false;
      }

      uint32_t averageLedBrightness = 0;

      uint16_t channelColor565[NUMBER_OF_CHANNELS];

      tft.startWrite();
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        averageLedBrightness += ledcRead( channelNumber );

        // redraw the top part of the bar
        tft.writeFillRect( channelNumber * BARS_WIDTH + BARS_BORDER,
                           BARS_BOTTOM - BARS_HEIGHT,
                           BARS_WIDTH - BARS_BORDER * 2,
                           BARS_HEIGHT - channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                           TFT_BACK_COLOR );
        /*
              //100% water mark
              tft.drawFastHLine( channelNumber * BARS_WIDTH + BARS_BORDER,
                                 BARS_BOTTOM - BARS_HEIGHT - 1,
                                 BARS_WIDTH - BARS_BORDER * 2,
                                 tft.color565( r, g, b ) );
        */

        uint32_t color = strtol( &channel[channelNumber].color[1], NULL, 16 );
        channelColor565[channelNumber] = tft.color565( ( color & 0xFF0000 ) >> 16, ( color & 0x00FF00 ) >> 8, color & 0x0000FF  );

        // redraw the bottom part of the bar
        tft.writeFillRect( channelNumber * BARS_WIDTH + BARS_BORDER,
                           BARS_BOTTOM - channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                           BARS_WIDTH - BARS_BORDER * 2,
                           channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                           channelColor565[channelNumber]);
      }
      tft.endWrite();

      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        tft.setCursor( channelNumber * BARS_WIDTH + 9, BARS_BOTTOM + 4 );
        tft.setTextSize( 1 );
        tft.setTextColor( channelColor565[channelNumber] , TFT_BACK_COLOR );

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
        tft.setTextColor( TFT_TEMP_COLOR , TFT_BACK_COLOR );
        for ( uint8_t thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++ )
        {
          tft.setCursor( thisSensor * 90, BARS_BOTTOM + 15 );
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

      averageLedBrightness = averageLedBrightness / NUMBER_OF_CHANNELS;

      uint16_t rawBrightness = map( tftBrightness, 0, 100, 0, backlightMaxvalue );

      ledcWrite( TFT_BACKLIGHT_CHANNEL, ( averageLedBrightness > rawBrightness ) ? rawBrightness : averageLedBrightness );

      xSemaphoreGive( x_SPI_Mutex );
    }
    vTaskDelay( tftTaskdelayTime / portTICK_PERIOD_MS );
  }
}
