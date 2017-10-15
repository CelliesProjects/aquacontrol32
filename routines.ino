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

/* https://www.esp32.com/viewtopic.php?t=664 */

bool setupMDNS( const String hostname )
{
  Serial.printf( "Check if %s is already present...\n", hostname.c_str() );
  uint32_t serviceIp = MDNS.queryHost( hostname.c_str() );
  if ( serviceIp != 0 )
  {
    Serial.printf( "%s already on network.\n", hostname.c_str() );
    return false;
  }
  Serial.printf( "%s is available.\nSetting new hostname...\n", hostname.c_str() );

  MDNS.end();

  if ( MDNS.begin( hostname.c_str() ) )
  {
    // Add service to MDNS-SD
    MDNS.addService( "http", "tcp", 80 );

    Serial.printf( "mDNS responder started\nmDNS name: %s.local.\n", hostname.c_str() );
    saveStringNVS( "hostname", hostname );
    Serial.printf( "mDNS hostname set to %s.\n", hostname.c_str() );
    return true;
  }
  else
  {
    Serial.println( "Error setting up MDNS responder!" );
    return false;
  }
}
