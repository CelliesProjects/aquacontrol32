void oledTask( void * pvParameters )
{
  OLED.init();
  OLED.clear();
  OLED.setTextAlignment( TEXT_ALIGN_CENTER );
  OLED.setFont( ArialMT_Plain_16 );
  OLED.drawString( 64, 10, F( "AquaControl32" ) );
  OLED.drawString( 64, 30, F( "Booting..." ) );
  OLED.display();

  int oledTaskdelayTime = 1000 / UPDATE_FREQ_OLED;

  /* wait for setup to end, but don't block */
  while ( lightStatus == "" )
  {
    vTaskDelay( 100 / portTICK_PERIOD_MS);
  }

  OLED.setFont( ArialMT_Plain_10 );

  while (1)
  {
    char content[30];

    if ( OLED_SHOW_SYSTEMDATA )
    {
      //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/main/sntp_example_main.c
      struct tm timeinfo;
      getLocalTime( &timeinfo );

      OLED.clear();
      OLED.setFont( ArialMT_Plain_10 );
      OLED.drawString( 64, 0, asctime( &timeinfo ) );

      snprintf( content, sizeof( content ), "%.2f kB RAM", esp_get_free_heap_size() / 1024.0 );
      OLED.drawString( 64, 10, content );

      OLED.drawString( 64, 20, "IP: " +  WiFi.localIP().toString() );

      snprintf( content, sizeof( content ), "%i Dallas sensors", numberOfFoundSensors );
      OLED.drawString( 64, 30, content );

      snprintf( content, sizeof( content ), "PWM: %.2f Khz - %i bits", ledcActualFrequency / 1000, ledcNumberOfBits );
      OLED.drawString( 64, 40, content );

      OLED.display();
    }
    else
    {
      const uint8_t BARS_BOTTOM      = DISPLAY_HEIGHT - 11;
      const uint8_t BARS_HEIGHT      = BARS_BOTTOM;
      const uint8_t BARS_BORDER      = 4;
      const uint8_t BARS_WIDTH       = DISPLAY_WIDTH  / NUMBER_OF_CHANNELS;

      OLED.clear();
      OLED.drawString( 64, 0, WiFi.localIP().toString() );

      for ( uint8_t thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
      {
        uint8_t x1 = BARS_WIDTH * thisChannel + BARS_BORDER;
        uint8_t y1 = ( BARS_BOTTOM ) - ( BARS_BOTTOM * ( channel[thisChannel].currentPercentage / 100 ) ) * 0.6;
        uint8_t x2 = BARS_WIDTH - BARS_BORDER;
        uint8_t y2 = BARS_BOTTOM - y1;
        OLED.fillRect( x1, y1, x2, y2 );

        OLED.setFont( ArialMT_Plain_10 );
        OLED.setTextAlignment( TEXT_ALIGN_CENTER );

        if ( channel[thisChannel].currentPercentage == 0 || channel[thisChannel].currentPercentage == 100 )
        {
          snprintf( content, sizeof( content ), "%.0f", channel[thisChannel].currentPercentage );
        }
        else if ( channel[thisChannel].currentPercentage < 10 )
        {
          snprintf( content, sizeof( content ), "%.2f", channel[thisChannel].currentPercentage );
        }
        else
        {
          snprintf( content, sizeof( content ), "%.1f", channel[thisChannel].currentPercentage );
        }

        OLED.drawString( x1 + ( BARS_WIDTH / 2 ) - 1, y1 - 11, content );
      }

      if ( numberOfFoundSensors )
      {
        uint8_t charCount = 0;
        for ( uint8_t sensorNumber = 0; sensorNumber < numberOfFoundSensors; sensorNumber++ )
        {
          charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.1f°C" , sensor[sensorNumber].temp / 16.0 );
        }
      }
      else
      {
        snprintf( content, sizeof( content ), "%.2f kB RAM", esp_get_free_heap_size() / 1024.0 );
      }
      OLED.drawString( 64, BARS_BOTTOM, content );
      OLED.display();
    }
    vTaskDelay( oledTaskdelayTime / portTICK_PERIOD_MS);
  }
}

