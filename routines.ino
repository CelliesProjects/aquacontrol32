float mapFloat( float x, const float in_min, const float in_max, const float out_min, const float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//format bytes as KB, MB or GB with indicator
String humanReadableSize( size_t bytes )
{
  if ( bytes < 1024)
  {
    return String( bytes ) + "B";
  } else if ( bytes < ( 1024 * 1024 ) )
  {
    return String( bytes / 1024.0 ) + "KB";
  } else if (bytes < ( 1024 * 1024 * 1024 ) )
  {
    return String( bytes / 1024.0 / 1024.0 ) + "MB";
  } else {
    return String( bytes / 1024.0 / 1024.0 / 1024.0 ) + "GB";
  }
}

bool setupMDNS()
{
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (MDNS.begin( mDNSname.c_str() ))
  {
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started");
    Serial.print( "mDNS name: ");  Serial.print( mDNSname );  Serial.println( ".local" );
    return true;
  }
  else
  {
    Serial.println("Error setting up MDNS responder!");
    return false;
  }
}
