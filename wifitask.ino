void wifiTask( void * pvParameters )
{
  /* trying last accesspoint */
  WiFi.mode( WIFI_STA );
  WiFi.begin();
  //WiFi.onEvent( WiFiEvent );

  ESP_LOGI( TAG, "Connecting WiFi");
  tft.println( "Connecting WiFi");
  connectWiFi();
  tft.setTextColor( ILI9341_WHITE , ILI9341_BLACK );

  /* check if we are connected */
  if ( WiFi.status() != WL_CONNECTED )
  {
    /* wait for SC */
    Serial.printf( "\nWaiting %i seconds for SmartConfig.\n", 60 * 5 );
    tft.println( "\nNo WiFi connection.\nWaiting for SmartConfig." );
    tft.invertDisplay( true );
    WiFi.mode( WIFI_AP_STA );
    WiFi.beginSmartConfig();

    const time_t rebootTime = millis() + 60 * 5 * 1000; /* 5 min */

    while ( !WiFi.smartConfigDone() && millis() < rebootTime )
    {
      vTaskDelay( 100 / portTICK_PERIOD_MS );

      tft.setCursor( 70, 100 );
      tft.printf( "%2i seconds until reboot.", ( 100 + rebootTime - millis() ) / 1000 );
    }

    if ( !WiFi.smartConfigDone() )
    {
      SPI.end();
      /* set bootstrapping pins */
      /* https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection */
      digitalWrite( 0, HIGH );
      digitalWrite( 2, HIGH );
      ESP.restart();
    }
  }

  //Wait for WiFi to connect to AP
  connectWiFi();


  /* We have succesfully connected */
  tft.invertDisplay( false );
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  tft.println( "WiFi connected.\nLocal IP: " + WiFi.localIP().toString() );
  ESP_LOGI( TAG, "WiFi connected to %s", WiFi.SSID().c_str() );
  ESP_LOGI( TAG, "Local IP: %c", WiFi.localIP().toString()  );
  ESP_LOGI( TAG, "MAC address: %X", mac );

  strncpy( hostName, readStringNVS( "hostname", "" ).c_str(), sizeof( hostName ) );

  if ( hostName[0] ==  0 )
  {
    snprintf( hostName, sizeof( hostName ), "%s%c%c%c%c%c%c", DEFAULT_HOSTNAME_PREFIX,
              WiFi.macAddress()[9], WiFi.macAddress()[10],
              WiFi.macAddress()[12], WiFi.macAddress()[13],
              WiFi.macAddress()[15], WiFi.macAddress()[16]
            );
  }

  if ( !setupMDNS( hostName ) )
  {
    ESP_LOGE( TAG, "Error setting up mDNS." );
    memset( hostName, 0, sizeof( hostName ) );
  }

  /* start network dependent tasks */
  xTaskCreatePinnedToCore(
    ntpTask,                        /* Function to implement the task */
    "ntpTask",                      /* Name of the task */
    2000,                           /* Stack size in words */
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

  while (1)
  {
    if ( !WiFi.isConnected() )
    {
      ESP_LOGI( TAG, "No Wifi. Reconnecting.." );
      WiFi.reconnect();
    }
    vTaskDelay( 10000 / portTICK_PERIOD_MS );
  }
}

void connectWiFi()
{
  const time_t endTime = millis() +  15 * 1000; /* 15 sec */

  while ( WiFi.status() != WL_CONNECTED && millis() < endTime )
  {
    tft.print( "." );
    vTaskDelay( 500 / portTICK_PERIOD_MS );
  }
}
/*
void WiFiEvent( WiFiEvent_t event )
{
  switch ( event )
  {
    case SYSTEM_EVENT_AP_START:
      //ESP_LOGI( TAG, "AP Started");
      //WiFi.softAPsetHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_AP_STOP:
      ESP_LOGI( TAG, "AP Stopped");
      break;
    case SYSTEM_EVENT_STA_START:
      //ESP_LOGI( TAG, "STA Started");
      //WiFi.setHostname( DEFAULT_HOSTNAME_PREFIX.c_str( );
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      ESP_LOGI( TAG, "STA Connected");
      //WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      //ESP_LOGI( TAG, "STA IPv6: ");
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
*/
