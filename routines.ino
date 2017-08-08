float mapFloat( double x, const double in_min, const double in_max, const double out_min, const double out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void OLEDprintLocalTime()
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
  OLED.drawString( 64, 10, String( sensor[1].temp / 16 ) + "C  " + String( sensor[2].temp / 16 ) + "C" );
  OLED.drawString( 64, 20, String( (int)brightness) + " - " + mapFloat( brightness, 0, LEDC_PWM_DEPTH, 0, 100 ) + "%" );

  OLED.drawString( 64, 30, String( (int)LEDC_PWM_DEPTH ) + " steps."  );
  OLED.drawString( 64, 40, String( LEDC_NUMBER_OF_BIT ) + "bit - " + String( ledcActualFrequency / 1000.0 ) + "kHz" );
  OLED.drawString( 64, 50, String( millis() / 1000.0 ) + " Sec" );
  OLED.display();

}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void readTempSensors()
{
  byte data[12];
  float celsius, fahrenheit;

  for ( byte thisSensor = 1; thisSensor <= numberOfSensors; thisSensor++)
  {
    ds.reset();
    ds.select( sensor[thisSensor].addr );
    ds.write(0xBE);         // Read Scratchpad

    //Serial.print( thisSensor ); Serial.print("  Data = ");
    //Serial.print(present, HEX);
    //Serial.print(" ");
    for ( byte i = 0; i < 9; i++)
    { // we need 9 bytes
      data[i] = ds.read(  );
      //Serial.print(data[i], HEX);
      //Serial.print(" ");
    }
    //Serial.print(" CRC=");
    //Serial.print(OneWire::crc8(data, 8), HEX);
    //Serial.println();
    byte type_s;
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s)
    {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10)
      {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    }
    else
    {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
    fahrenheit = celsius * 1.8 + 32.0;

    sensor[thisSensor].temp = raw;

    ds.reset();
    ds.select( sensor[thisSensor].addr  );
    ds.write( 0x44, 1);        // start conversion, with parasite power on at the end
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

