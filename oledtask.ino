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

  while (1)
  {
    //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/main/sntp_example_main.c
    struct tm timeinfo;
    getLocalTime( &timeinfo );

    OLED.clear();
    OLED.setFont( ArialMT_Plain_10 );
    OLED.drawString( 64, 0, asctime( &timeinfo ) );

    char content[30];
    snprintf( content, sizeof( content ), "%.2f kB RAM", esp_get_free_heap_size() / 1024.0 );
    OLED.drawString( 64, 10, content );

    OLED.drawString( 64, 20, "IP: " +  WiFi.localIP().toString() );

    snprintf( content, sizeof( content ), "%i Dallas sensors", numberOfFoundSensors );
    OLED.drawString( 64, 30, content );

    snprintf( content, sizeof( content ), "PWM: %.2f Khz - %i bits", ledcActualFrequency / 1000, ledcNumberOfBits );
    OLED.drawString( 64, 40, content );

    OLED.display();
    vTaskDelay( oledTaskdelayTime / portTICK_PERIOD_MS);
  }
}

