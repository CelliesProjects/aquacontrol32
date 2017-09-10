#define WRITE_ENABLED                        false
#define WRITE_DISABLED                       true

#define DEFAULT_CHANNEL_NAME                 "Channel" + String( channelNumber + 1 )
#define DEFAULT_BACKGROUND_COLOR             "#f8ff7c"

String readChannelName( const int channelNumber )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  String NVSvalue = preferences.getString( "channelname" + channelNumber, DEFAULT_CHANNEL_NAME );
  preferences.end();
  return NVSvalue;
}

void saveChannelName( const int channelNumber )
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  if ( channel[channelNumber].name == "" )
  {
    preferences.remove( "channelname" + channelNumber );
    //Serial.print( "Channel "); Serial.print( channelNumber ); Serial.println( " name removed from NVS" );
  }
  else if ( preferences.getString( "channelname" + channelNumber ) != channel[channelNumber].name )
  {
    preferences.putString( "channelname" + channelNumber, channel[channelNumber].name );
    //Serial.print( "Channel "); Serial.print( channelNumber ); Serial.println( " name stored in NVS" );
  }
  preferences.end();
}

String readChannelColor( const int channelNumber )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  String NVSvalue = preferences.getString( "channelcolor" + channelNumber, DEFAULT_BACKGROUND_COLOR );
  preferences.end();
  return NVSvalue;
}

void saveChannelColor( const int channelNumber )
{
  preferences.begin( "aquacontrol32", WRITE_ENABLED );
  if ( channel[channelNumber].color == "undefined" )
  {
    preferences.remove( "channelcolor" + channelNumber );
    //Serial.print( "Channel "); Serial.print( channelNumber ); Serial.println( " color removed from NVS" );
  }
  else if ( preferences.getString( "channelcolor" + channelNumber ) != channel[channelNumber].color )
  {
    preferences.putString( "channelcolor" + channelNumber, channel[channelNumber].color );
    Serial.print( "Channel "); Serial.print( channelNumber ); Serial.println( " color stored in NVS" );
  }
  preferences.end();  
}


