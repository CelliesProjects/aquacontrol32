void IRAM_ATTR oledTask( void * pvParameters )
{
  const TickType_t oledTaskdelayTime = 1000 / UPDATE_FREQ_OLED / portTICK_PERIOD_MS;

  OLED.init();
  ESP_LOGI( TAG, "OLED initialized." );

  if ( preferences.getString( "oledorientation", "normal" ) == "upsidedown" )
  {
    oledOrientation = OLED_ORIENTATION_UPSIDEDOWN;
    OLED.flipScreenVertically();
  }

  OLED.setTextAlignment( TEXT_ALIGN_CENTER );
  OLED.setFont( ArialMT_Plain_16 );
  OLED.drawString( 64, 10, F( "AquaControl32" ) );
  OLED.drawString( 64, 30, F( "Starting WiFi..." ) );
  OLED.display();

  while ( !xDimmerTaskHandle )
  {
    vTaskDelay( 10 / portTICK_PERIOD_MS );
  }

  oledContrast = preferences.getUInt( "oledcontrast", 15 );
  OLED.setContrast( oledContrast << 0x04 );

  OLED.setFont( ArialMT_Plain_10 );

  while (1)
  {
    static char content[64];

    OLED.clear();

    if ( OLED_SHOW_SYSTEMDATA )
    {
      //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/main/sntp_example_main.c
      struct tm timeinfo;
      getLocalTime( &timeinfo );

      OLED.drawString( 64, 0, asctime( &timeinfo ) );

      snprintf( content, sizeof( content ), "%.2f kB RAM", esp_get_free_heap_size() / 1024.0 );
      OLED.drawString( 64, 10, content );

      OLED.drawString( 64, 20, "IP: " +  WiFi.localIP().toString() );

      snprintf( content, sizeof( content ), "%i Dallas sensors", sensor.count() );
      OLED.drawString( 64, 30, content );

      snprintf( content, sizeof( content ), "%s", sketchVersion );
      OLED.drawString( 64, 45, content );
    }
    else
    {
      const uint8_t BARS_BOTTOM      = OLED.getHeight() - 11;
      const uint8_t BARS_HEIGHT      = BARS_BOTTOM;
      const uint8_t BARS_BORDER      = 4;
      const uint8_t BARS_WIDTH       = OLED.getWidth() / NUMBER_OF_CHANNELS;

      OLED.drawString( 64, 0, WiFi.localIP().toString() );

      for ( uint8_t thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
      {
        uint8_t x1 = BARS_WIDTH * thisChannel + BARS_BORDER;
        uint8_t y1 = ( BARS_BOTTOM ) - ( BARS_BOTTOM * ( channel[thisChannel].currentPercentage / 100 ) ) * 0.6;
        uint8_t x2 = BARS_WIDTH - BARS_BORDER;
        uint8_t y2 = BARS_BOTTOM - y1;
        OLED.fillRect( x1, y1, x2, y2 );

        threeDigitPercentage( content, sizeof( content ), channel[thisChannel].currentPercentage, NO_PERCENTSIGN );

        OLED.drawString( x1 + ( BARS_WIDTH / 2 ) - 1, y1 - 11, content );
      }

      if ( sensor.count() )
      {
        uint8_t charCount = 0;
        for ( uint8_t sensorNumber = 0; sensorNumber < sensor.count(); sensorNumber++ )
        {
          if ( !sensor.error(sensorNumber) )
            charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.1f°C  " , sensor.temp( sensorNumber ) );
          else
            charCount += snprintf( content + charCount, sizeof( content ) - charCount, "ERROR  " , sensor.temp( sensorNumber ) );
        }
      }
      else
      {
        snprintf( content, sizeof( content ), "%.2f kB RAM", esp_get_free_heap_size() / 1024.0 );
      }
      OLED.drawString( 64, BARS_BOTTOM, content );
    }

    OLED.display();
    vTaskDelay( oledTaskdelayTime );
  }
}
