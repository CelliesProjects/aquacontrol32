#define WRITE_ENABLED                        false
#define WRITE_DISABLED                       true

void clearNVS()
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  preferences.clear();
  preferences.end();
  Serial.println( "NVS cleared from data" );
}

float readFloatNVS( char const* key , const float defaultValue)
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

String readStringNVS( const char* key, const String defaultStr )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  String NVSvalue = preferences.getString( key, defaultStr );
  preferences.end();
  return NVSvalue;
}

uint8_t readInt8NVS( const char* key,  const int8_t value )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  int8_t NVSvalue = preferences.getUChar( key, value );
  preferences.end();
  return NVSvalue;
}

void saveInt8NVS( const char* key, const int8_t value )
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  if ( preferences.getUChar( key ) != value )
  {
    preferences.putUChar( key, value );
  }
  preferences.end();
}

uint8_t readUint8NVS( const char* key,  const uint8_t value )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  uint8_t NVSvalue = preferences.getUChar( key, value );
  preferences.end();
  return NVSvalue;
}

void saveUint8NVS( const char* key, const uint8_t value )
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  if ( preferences.getUChar( key ) != value )
  {
    preferences.putUChar( key, value );
  }
  preferences.end();
}

double readDoubleNVS( const char* key, const double value )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  double NVSvalue = preferences.getDouble( key, value );
  preferences.end();
  return NVSvalue;
}

void saveDoubleNVS( const char* key, const double value )
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  if ( preferences.getDouble( key ) != value )
  {
    preferences.putDouble( key, value );
  }
  preferences.end();
}

char readCharNVS( const char* key, const char value )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  char NVSvalue = preferences.getChar( key, value );
  preferences.end();
  return NVSvalue;
}

