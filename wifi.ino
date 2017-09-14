
void setupWiFi()
{
  WiFi.onEvent( WiFiEvent );

  tft.println( "Starting WiFi..." );
  String wifiSSID = readWifiSSID();
  String wifiPSK = readWifiPSK();

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
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("SmartConfig received.");
    tft.println("SmartConfig received.");
  }

  Serial.print( F( "Connecting to SSID:" ) ); Serial.println( wifiSSID );
  tft.print( F( "Connecting to SSID:" ) ); tft.println( wifiSSID );
  //Serial.print( F( "With password:" ) ); Serial.println( F( "*********" ) /* wifiPSK */ );

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for connection...");
  WiFi.mode( WIFI_STA );
  WiFi.begin( wifiSSID.c_str(), wifiPSK.c_str() );

  unsigned long WiFiStartTime = millis();
  while ( WiFi.status() != WL_CONNECTED && millis() - WiFiStartTime <= 10000 )
  {
    //pick nose while WiFi connects...
  }

  if ( WiFi.status() == WL_CONNECTED )
  {
    //We have succesfully connected...
    tft.invertDisplay( false );
    tft.println( "WiFi connected.\nLocal IP: " + WiFi.localIP() );

    saveWifiData();

  }
  else
  {
    Serial.println( F ("WiFi Connection failed. Check supplied password." ) );
    tft.println( F ("WiFi Connection failed. Check supplied password." ) );
    Serial.println( WiFi.status() );
    tft.println( WiFi.status() );
    // restart the AP to try again
    delay(5000);
    ESP.restart();
  }
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
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
      WiFi.setHostname( mDNSname.c_str() );
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

