void oledTask( void * pvParameters )
{
  int oledTaskdelayTime = 1000 / UPDATE_FREQ_OLED;

  while (1)
  {
    //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/main/sntp_example_main.c
    struct tm timeinfo;
    getLocalTime( &timeinfo );

    OLED.clear();
    OLED.setFont( ArialMT_Plain_10 );
    OLED.drawString( 64, 0, asctime( &timeinfo ) );
    OLED.drawString( 64, 10, String( esp_get_free_heap_size() / 1024.0 ) + " kB RAM FREE" );
    OLED.drawString( 64, 20, "IP: " +  WiFi.localIP().toString() );
    OLED.drawString( 64, 30, String( numberOfFoundSensors ) + " Dallas sensors" );
    OLED.drawString( 64, 40, "SYSTEM START:" );
    OLED.drawString( 64, 50, asctime( &systemStart ) );
    OLED.display();
    vTaskDelay( oledTaskdelayTime / portTICK_PERIOD_MS);
  }
}

void setupOLED()
{
  btStop();
  OLED.init();
  OLED.clear();
  OLED.setTextAlignment( TEXT_ALIGN_CENTER );
  OLED.setFont( ArialMT_Plain_16 );
  OLED.drawString( 64, 10, F( "AquaControl32" ) );
  OLED.drawString( 64, 30, F( "Booting..." ) );
  OLED.display();
}
