void oledTask( void * pvParameters )
{
  int oledTaskdelayTime = 1000 / UPDATE_FREQ_OLED;

  while (1)
  {
    //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/main/sntp_example_main.c
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    OLED.clear();
    OLED.setFont( ArialMT_Plain_10 );
    OLED.drawString( 64, 0, strftime_buf );
    //OLED.drawString( 64, 10, String( sensor[0].temp / 16 ) + "C  " + String( sensor[1].temp / 16 ) + "C" );
    OLED.drawString( 64, 10, String( esp_get_free_heap_size() / 1024.0 ) + " kB RAM FREE" );
    OLED.drawString( 64, 20, "IP: " +  WiFi.localIP().toString() );
    OLED.drawString( 64, 30, String( numberOfFoundSensors ) + " Dallas sensors" );
    OLED.drawString( 64, 40, "SYSTEM START:" );

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &systemStart);

    OLED.drawString( 64, 50, strftime_buf  );
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
