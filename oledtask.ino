void oledTask( void * pvParameters )
{
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
    //OLED.drawString( 64, 10, String( sensor[1].temp / 16 ) + "C  " + String( sensor[2].temp / 16 ) + "C" );
    OLED.drawString( 64, 10, String( esp_get_free_heap_size() / 1024.0 ) + " KB RAM FREE" );
    OLED.drawString( 64, 20, "IP: " +  WiFi.localIP().toString() );
    OLED.drawString( 64, 30, numberOfFoundSensors + " Dallas sensors"  );
    OLED.drawString( 64, 40, String( LEDC_NUMBER_OF_BIT ) + "bit - " + String( ledcActualFrequency / 1000.0 ) + "kHz" );
    OLED.drawString( 64, 50, String( millis() / 1000.0 ) + " Sec" );
    OLED.display();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

