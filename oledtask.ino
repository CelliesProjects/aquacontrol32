void oledTask( void * pvParameters ) {
  const TickType_t oledTaskdelayTime = 1000 / UPDATE_FREQ_OLED / portTICK_PERIOD_MS;

  while ( !xDimmerTaskHandle )
    vTaskDelay( 10 / portTICK_PERIOD_MS );

  OLED.setFont( ArialMT_Plain_10 );

  while (1) {
    static char content[64];
    OLED.clear();
    if ( OLED_SHOW_SYSTEMDATA ) {
      //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/main/sntp_example_main.c
      time_t now = time(NULL);
      struct tm timeinfo;
      localtime_r( &now, &timeinfo );
      OLED.drawString( 64, 0, asctime_r( &timeinfo, content ) );

      snprintf( content, sizeof( content ), "%.2f kB RAM", esp_get_free_heap_size() / 1024.0 );
      OLED.drawString( 64, 10, content );

      if (!WiFi.isConnected())
        OLED.drawString(64, 20, "WiFi ERROR");
      else
        OLED.drawString( 64, 20, "IP: " +  WiFi.localIP().toString() );

      snprintf( content, sizeof( content ), "%i Dallas sensors", logger.sensorCount() );
      OLED.drawString( 64, 30, content );

      snprintf( content, sizeof( content ), "%s", sketchVersion );
      OLED.drawString( 64, 45, content );
    }
    else {
      const uint8_t BARS_BOTTOM      = OLED.getHeight() - 11;
      const uint8_t BARS_HEIGHT      = BARS_BOTTOM;
      const uint8_t BARS_BORDER      = 4;
      const uint8_t BARS_WIDTH       = OLED.getWidth() / NUMBER_OF_CHANNELS;

      if (!WiFi.isConnected())
        OLED.drawString(64, 0, "WiFi ERROR");
      else
        OLED.drawString( 64, 0, "IP: " +  WiFi.localIP().toString() );

      for ( uint8_t thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ ) {
        uint8_t x1 = BARS_WIDTH * thisChannel + BARS_BORDER;
        uint8_t y1 = ( BARS_BOTTOM ) - ( BARS_BOTTOM * ( channel[thisChannel].currentPercentage / 100 ) ) * 0.6;
        uint8_t x2 = BARS_WIDTH - BARS_BORDER;
        uint8_t y2 = BARS_BOTTOM - y1;
        OLED.fillRect( x1, y1, x2, y2 );
        OLED.drawString( x1 + ( BARS_WIDTH / 2 ) - 1, y1 - 11,
                         threeDigitPercentage( content, sizeof( content ), channel[thisChannel].currentPercentage, NO_PERCENTSIGN ) );
      }

      if ( logger.sensorCount() ) {
        uint8_t charCount = 0;
        for ( uint8_t sensorNumber = 0; sensorNumber < logger.sensorCount(); sensorNumber++ )
          charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.1f°C  " , logger.sensorTemp( sensorNumber ) );
      }
      else
        snprintf( content, sizeof( content ), "%.2f kB RAM", esp_get_free_heap_size() / 1024.0 );
      OLED.drawString( 64, BARS_BOTTOM, content );
    }

    OLED.display();
    vTaskDelay( oledTaskdelayTime );
  }
}
