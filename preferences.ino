#define WRITE_ENABLED                        false
#define WRITE_DISABLED                       true

void clearNVS()
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  preferences.clear();
  preferences.end();
  Serial.println( "NVS cleared from data" );
}

float readFloatNVS( char const* key , float defaultValue)
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  float NVSvalue = preferences.getFloat( key , defaultValue );
  preferences.end();
  return NVSvalue;
}

void saveFloatNVS( char const* key, const float value )
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  if ( preferences.getFloat( key ) != value )
  {
    preferences.putFloat( key, value );
  }
  preferences.end();
}

void saveStringNVS( char const* key, const String value )
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  if ( preferences.getString( key ) != value )
  {
    preferences.putString( key, value );
  }
  preferences.end();
}

String readStringNVS( const char* key, String defaultStr )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  String NVSvalue = preferences.getString( key, defaultStr );
  preferences.end();
  return NVSvalue;
}

