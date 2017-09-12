
void setupWiFi()
{
  WiFi.onEvent( WiFiEvent );

  //if no NVS data is found start an AP
  preferences.begin( "aquacontrol32", false );
  if ( preferences.getString( "ssid" ) != "" )
  {
    Serial.println( F( "Preferences found." ) );
  }
  else
  {
    Serial.println( "No WiFi preferences found. Starting SmartConfig." );

    //Init WiFi as Station, start SmartConfig
    WiFi.mode( WIFI_STA );
    WiFi.beginSmartConfig();

    //Wait for SmartConfig packet from mobile
    Serial.println("Waiting for SmartConfig. Use the app. RTM.");
    while ( !WiFi.smartConfigDone() )
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("SmartConfig received.");
  }

  Serial.print( F( "Connecting to SSID:" ) ); Serial.println( preferences.getString( "ssid" ) );
  //Serial.print( F( "With password:" ) ); Serial.println( F( "*********" ) /* preferences.getString( "psk" ) */ );

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for connection...");
  WiFi.mode( WIFI_STA );
  WiFi.begin( preferences.getString( "ssid" ).c_str(), preferences.getString( "psk" ).c_str() );

  unsigned long WiFiStartTime = millis();
  while ( WiFi.status() != WL_CONNECTED && millis() - WiFiStartTime <= 10000 )
  {
    //pick nose while WiFi connects...
  }

  if ( WiFi.status() == WL_CONNECTED )
  {
    //We have succesfully connected...
    tft.println( "WiFi connected.\nLocal IP: " + WiFi.localIP() );

    saveWifiData();

  }
  else
  {
    Serial.println( F ("WiFi Connection failed. Check supplied password." ) );
    Serial.println( WiFi.status() );
    // restart the AP to try again
    delay(5000);
    ESP.restart();
  }
  preferences.end();
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
      //WiFi.setHostname(AP_SSID);
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

