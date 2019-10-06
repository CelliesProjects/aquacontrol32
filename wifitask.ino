void wifiTask( void * pvParameters ) {
  /* trying last accesspoint */
  WiFi.mode( WIFI_STA );
  WiFi.begin();
  //WiFi.onEvent( WiFiEvent );

  ESP_LOGI( TAG, "Connecting WiFi" );

  if ( xTftTaskHandle ) {
    tft.setTextColor( TFT_TEXT_COLOR , TFT_BACK_COLOR );
    tft.println( "Connecting WiFi");
  }

  WiFi.waitForConnectResult();

  if ( WiFi.status() != WL_CONNECTED ) {
    if ( xTftTaskHandle ) {
      tft.println( "\nNo WiFi connection.\nWaiting for SmartConfig." );
      tft.invertDisplay( true );
    }

    WiFi.mode( WIFI_AP_STA );
    WiFi.beginSmartConfig();

    const unsigned long rebootDelayMs = 60 * 5 * 1000;

    const unsigned long smartConfigStartMs = millis();

    ESP_LOGI( TAG, "Waiting %i seconds for SmartConfig.", rebootDelayMs / 1000 );

    while ( !WiFi.smartConfigDone() && ( (unsigned long)millis() - smartConfigStartMs ) < rebootDelayMs ) {
      char remainingSCTime[12];
      if ( xOledTaskHandle ) {
        OLED.clear();
        OLED.setFont( ArialMT_Plain_10 );
        OLED.drawString( 64, 10, "Waiting for SmartConfig." );
        snprintf( remainingSCTime, sizeof( remainingSCTime), "%i seconds", ( (unsigned long)( smartConfigStartMs + rebootDelayMs ) - millis() ) / 1000 );
        OLED.drawString( 64, 30, remainingSCTime );
        OLED.invertDisplay();
        OLED.display();
      }

      if ( xTftTaskHandle ) {
        tft.setCursor( 70, 100 );
        tft.print( remainingSCTime );
      }
      else
        digitalWrite( LED_BUILTIN, !gpio_get_level( gpio_num_t( LED_BUILTIN ) ) );
      vTaskDelay( 500 / portTICK_PERIOD_MS );
    }

    if ( !WiFi.smartConfigDone() )
      ESP.restart();

    if ( !xTftTaskHandle )
      digitalWrite( LED_BUILTIN, LOW );
  }

  waitForWifi();

  /* We have succesfully connected */
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  ESP_LOGI( TAG, "WiFi '%s' %s %02x:%02x:%02x:%02x:%02x:%02x",
            WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
  if ( xTftTaskHandle ) {
    tft.invertDisplay( false );
    tft.printf( "WiFi connected.\nLocal IP: %s\n", WiFi.localIP().toString() );
  }
  if ( xOledTaskHandle )
    OLED.normalDisplay();
  strncpy( hostName, preferences.getString( "hostname", "" ).c_str(), sizeof( hostName ) );

  if ( hostName[0] ==  0 ) {
    snprintf( hostName, sizeof( hostName ), "%s%c%c%c%c%c%c", DEFAULT_HOSTNAME_PREFIX,
              WiFi.macAddress()[9], WiFi.macAddress()[10],
              WiFi.macAddress()[12], WiFi.macAddress()[13],
              WiFi.macAddress()[15], WiFi.macAddress()[16]
            );
  }

  if ( !MDNS.begin( "" ) && !setupMDNS( hostName ) )
    ESP_LOGE( TAG, "Error setting up %s as hostname. ", hostName );

  /* start network dependent tasks */
  xTaskCreatePinnedToCore(
    ntpTask,                        /* Function to implement the task */
    "ntpTask",                      /* Name of the task */
    3000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    ntpTaskPriority,                /* Priority of the task */
    NULL,                           /* Task handle. */
    1);

  xTaskCreatePinnedToCore(
    webServerTask,                  /* Function to implement the task */
    "webServerTask",                /* Name of the task */
    2000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    webserverTaskPriority,          /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  while (1) {
    if ( !WiFi.isConnected() ) {
      ESP_LOGI( TAG, "No Wifi. Reconnecting.." );
      WiFi.reconnect();
      vTaskDelay( 9000 / portTICK_PERIOD_MS );
    }
    vTaskDelay( 1000 / portTICK_PERIOD_MS );
  }
}

/* https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiType.h */
void waitForWifi() {
  unsigned long timeOut = millis() + 10000;
  while ( !WiFi.isConnected() && millis() < timeOut )
    vTaskDelay( 20 / portTICK_PERIOD_MS );
}

void WiFiEvent( WiFiEvent_t event ) {
  switch ( event ) {
    case SYSTEM_EVENT_AP_START:
      ESP_LOGI( TAG, "AP Started");
      //WiFi.softAPsetHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_AP_STOP:
      ESP_LOGI( TAG, "AP Stopped");
      break;
    case SYSTEM_EVENT_STA_START:
      ESP_LOGI( TAG, "STA Started");
      //WiFi.setHostname( DEFAULT_HOSTNAME_PREFIX.c_str( );
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      ESP_LOGI( TAG, "STA Connected");
      //WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      ESP_LOGI( TAG, "STA IPv6: ");
      //ESP_LOGI( TAG, "%s", WiFi.localIPv6().toString());
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      //ESP_LOGI( TAG, "STA IPv4: ");
      //ESP_LOGI( TAG, "%s", WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGI( TAG, "STA Disconnected");
      break;
    case SYSTEM_EVENT_STA_STOP:
      ESP_LOGI( TAG, "STA Stopped");
      break;
    default:
      break;
  }
}
