
void setupWiFi()
{
  //WiFi.onEvent( WiFiEvent );

  tft.println( "Starting WiFi..." );
  String wifiSSID = readStringNVS( "wifissid", "" );
  String wifiPSK = readStringNVS( "wifipsk" , "" );

  Serial.println( "wifidata: " + wifiSSID + " " + wifiPSK );

  //if no NVS data is found start an AP
  if ( wifiSSID != "" && wifiPSK != "" )
  {
    Serial.println( F( "WiFi preferences found." ) );
  }
  else
  {
    //Init WiFi as Station, start SmartConfig
    WiFi.mode( WIFI_STA );
    WiFi.beginSmartConfig();

    //Wait for SmartConfig packet from mobile
    Serial.println("\n\nNo WiFi preferences found.\nWaiting for SmartConfig. Use the app. RTM.");
    tft.println("\n\nNo WiFi preferences found.\nWaiting for SmartConfig. Use the app. RTM.");
    tft.invertDisplay( true );
    while ( !WiFi.smartConfigDone() )
    {
      //vTaskDelay( 500 / portTICK_PERIOD_MS );
      vTaskDelay( 500 / portTICK_PERIOD_MS );
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("SmartConfig received.");
    tft.println("SmartConfig received.");
    wifiSSID = WiFi.SSID();
    wifiPSK = WiFi.psk();
  }

  Serial.print( F( "Connecting to SSID:" ) ); Serial.println( wifiSSID );
  tft.print( F( "Connecting to SSID:" ) ); tft.println( wifiSSID );

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for connection...");
  WiFi.mode( WIFI_STA );

  WiFi.begin( wifiSSID.c_str(), wifiPSK.c_str() );

  unsigned long WiFiStartTime = millis();
  while ( WiFi.status() != WL_CONNECTED && millis() - WiFiStartTime <= 10000 )
  {
    Serial.print( "." );
    tft.print( "." );
    //vTaskDelay( 500 / portTICK_PERIOD_MS );
    delay( 500 / portTICK_PERIOD_MS );
  }
  tft.println();
  Serial.println();

  if ( WiFi.status() == WL_CONNECTED )
  {
    //We have succesfully connected...
    tft.invertDisplay( false );
    tft.println( "WiFi connected.\nLocal IP: " + WiFi.localIP().toString() );
    Serial.println( "WiFi connected.\nLocal IP: " + WiFi.localIP().toString() );
    saveStringNVS( "wifissid", wifiSSID.c_str() );
    saveStringNVS( "wifipsk", wifiPSK.c_str() );

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
    Serial.printf( "MAC ident = %s\n", hostName );

    if ( !MDNS.begin( readStringNVS( "hostname", hostName ).c_str() ) )
    {
      Serial.println( "Error setting up mDNS." );
      memset( hostName, 0, sizeof( hostName ) );
    }
    else
    {
      Serial.printf(  "MDNS name set to %s.\n", hostName );
    }
  }
  else
  {
    Serial.println( F ("WiFi Connection failed. Check supplied password." ) );
    tft.println( F ("WiFi Connection failed. Check supplied password." ) );
    Serial.println( WiFi.status() );
    tft.println( WiFi.status() );
    // restart the AP to try again
    vTaskDelay( 500 / portTICK_PERIOD_MS );
    ESP.restart();
  }
}
/*
  void WiFiEvent( WiFiEvent_t event )
  {
  switch ( event )
  {
    case SYSTEM_EVENT_AP_START:
      Serial.println("AP Started");
      //WiFi.softAPsetHostname(AP_SSID);
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("AP Stopped");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("STA Started");
      WiFi.setHostname( DEFAULT_HOSTNAME_PREFIX.c_str() );
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("STA Connected");
      //WiFi.enableIpV6();
      break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      Serial.print("STA IPv6: ");
      Serial.println(WiFi.localIPv6());
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("STA IPv4: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("STA Disconnected");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("STA Stopped");
      break;
    default:
      break;
  }
  }
*/
