void wifiTask( void * pvParameters ) {
  /* trying last accesspoint */
  WiFi.mode( WIFI_STA );
  WiFi.setSleep( false );

  if ( wifi_network != "" ) WiFi.begin( wifi_network, wifi_password );
  else WiFi.begin();

  if ( xTftTaskHandle ) {
    tft.setTextColor( TFT_TEXT_COLOR , TFT_BACK_COLOR );
    tft.println( "Connecting WiFi");
  }

  WiFi.waitForConnectResult();

  if ( WiFi.status() != WL_CONNECTED && ( WiFi.status() != WL_CONNECT_FAILED ) ) {
    if ( xTftTaskHandle ) {
      tft.println( "\nNo WiFi connection.\nWaiting for SmartConfig." );
      tft.invertDisplay( true );
    }

    WiFi.mode( WIFI_AP_STA );
    WiFi.beginSmartConfig();

    const time_t DELAY_SECONDS = 60 * 5;
    const time_t END_TIME = time( NULL ) + DELAY_SECONDS;

    ESP_LOGI( TAG, "Waiting %i seconds for SmartConfig.", DELAY_SECONDS );

    while ( !WiFi.smartConfigDone() && time( NULL ) < END_TIME ) {
      char remainingSCTime[12];
      if ( xOledTaskHandle ) {
        OLED.clear();
        OLED.setFont( ArialMT_Plain_10 );
        OLED.drawString( 64, 10, "Waiting for SmartConfig." );
        snprintf( remainingSCTime, sizeof( remainingSCTime), "%3.i seconds", END_TIME - time( NULL ) );
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

  const IPAddress NO_IP(0, 0, 0, 0);
  if (!WiFi.config(STATIC_IP, GATEWAY, SUBNET, PRIMARY_DNS, SECONDARY_DNS)) {
    Serial.print( "STA Failed to configure" );
    while (1) delay(10);
  }

  //WiFi.setAutoReconnect( true );
  /* We have succesfully connected */
  WiFi.onEvent( WiFiEvent );
  ESP_LOGI( TAG, "WiFi connected to '%s' %s %s",
            WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str() );
  if ( xTftTaskHandle ) {
    tft.invertDisplay( false );
    tft.printf( "WiFi connected.\nIP: %s\nmac: %s\n", WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str() );
  }
  if ( xOledTaskHandle )
    OLED.normalDisplay();
  strncpy( hostName, preferences.getString( "hostname", "" ).c_str(), sizeof( hostName ) );

  if ( hostName[0] ==  0 ) {
    snprintf( hostName, sizeof( hostName ), "%s-%c%c%c%c%c%c", DEFAULT_HOSTNAME_PREFIX,
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
  /*
    while (1) {
      if ( !WiFi.isConnected() ) {
        ESP_LOGI( TAG, "No Wifi. Reconnecting.." );
        WiFi.reconnect();
        vTaskDelay( 9000 / portTICK_PERIOD_MS );
      }
      vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
  */
  vTaskDelete(NULL);
}

/* https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiType.h */
void waitForWifi() {
  unsigned long timeOut = millis() + 10000UL;
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
      WiFi.begin();
      break;
    case SYSTEM_EVENT_STA_STOP:
      ESP_LOGI( TAG, "STA Stopped");
      break;
    default:
      break;
  }
}
