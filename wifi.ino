/* https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/esp32/esp_smartconfig.h */
void wifiTask( void * pvParameters )
{
  /* trying last accesspoint */
  WiFi.begin();
  unsigned long WiFiStartTime = millis();
  Serial.println( "Connecting to last known network");
  tft.println( "Connecting to last known network");
  while ( WiFi.status() != WL_CONNECTED && millis() - WiFiStartTime <= 15000 )
  {
    tft.print( "." );
    vTaskDelay( 500 / portTICK_PERIOD_MS );
  }
  tft.println();

  /* check if we are connected */
  if ( WiFi.status() != WL_CONNECTED )
  {
    /* wait for SC */
    tft.println("\n\nNo WiFi connection.\nWaiting for SmartConfig. Use the app. RTM.");
    tft.invertDisplay( true );
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();
    while ( !WiFi.smartConfigDone() )
    {
      vTaskDelay( 500 / portTICK_PERIOD_MS );
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("SmartConfig received.");
    tft.println("SmartConfig received.");
    ESP.restart();
  }

  /* We have succesfully connected */
  tft.invertDisplay( false );
  tft.println( "WiFi connected.\nLocal IP: " + WiFi.localIP().toString() );
  Serial.println( "WiFi connected.\nLocal IP: " + WiFi.localIP().toString() );

  /* setup mDNS */
  if ( readStringNVS( "hostname", "" ) == "" )
  {
    snprintf( hostName, sizeof( hostName ), "%s%c%c%c%c%c%c", DEFAULT_HOSTNAME_PREFIX,
              WiFi.macAddress()[9], WiFi.macAddress()[10],
              WiFi.macAddress()[12], WiFi.macAddress()[13],
              WiFi.macAddress()[15], WiFi.macAddress()[16]
            );
  }
  else
  {
    snprintf( hostName, sizeof( hostName ), "%s", readStringNVS( "hostname", "" ).c_str() );
  }
  Serial.println( "MAC address = " + WiFi.macAddress() );
  Serial.printf( "Unique name = %s\n", hostName );

  if ( !MDNS.begin( readStringNVS( "hostname", hostName ).c_str() ) )
  {
    Serial.println( "Error setting up mDNS." );
    memset( hostName, 0, sizeof( hostName ) );
  }
  else
  {
    Serial.printf(  "MDNS name set to %s.\n", hostName );
  }

  /* start network dependent tasks */
  xTaskCreatePinnedToCore(
    ntpTask,                       /* Function to implement the task */
    "ntpTask",                     /* Name of the task */
    2000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    1,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);

  xTaskCreatePinnedToCore(
    webServerTask,                  /* Function to implement the task */
    "webServerTask",                /* Name of the task */
    1000,                           /* Stack size in words */
    NULL,                           /* Task input parameter */
    6,                              /* Priority of the task */
    NULL,                           /* Task handle. */
    1);                             /* Core where the task should run */

  while (1)
  {
    if ( !WiFi.isConnected() )
    {
      Serial.println( "No Wifi. Reconnecting.." );
      WiFi.begin();
    }
    vTaskDelay( 10000 / portTICK_PERIOD_MS );
  }
}

void doSmartConfig()
{
  tft.println("\n\nNo WiFi connection.\nWaiting for SmartConfig. Use the app. RTM.");
  tft.invertDisplay( true );
  WiFi.beginSmartConfig();
  while ( !WiFi.smartConfigDone() )
  {
    //vTaskDelay( 500 / portTICK_PERIOD_MS );
    delay( 500  );
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("SmartConfig received.");
  tft.println("SmartConfig received.");
}
