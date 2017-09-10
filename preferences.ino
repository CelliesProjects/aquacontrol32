#define WRITE_ENABLED  false
#define WRITE_DISABLED true

String readChannelName( const int channelNumber )
{
  preferences.begin( "aquacontrol32", WRITE_DISABLED );
  String NVSvalue = preferences.getString( "channelname" + channelNumber, "Channel" + String( channelNumber + 1 ) );
  preferences.end();
  return NVSvalue;
}

void saveChannelName( const int channelNumber )
{
  //save the channelcolors in preferences
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

void saveChannelColor()
{
  
}


