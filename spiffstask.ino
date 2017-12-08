void spiffsTask( void * pvParameters )
{
  Serial.println( "Starting SPIFFS." );

  if ( !SPIFFS.begin( true ) )
  {
    Serial.println( "Error starting SPIFFS." );
  }
  else
  {
    Serial.println( "SPIFFS started." );
  }
  vTaskDelete( NULL );
}

bool defaultTimersLoaded()
{
  //find 'default.aqu' on SD card and if present load the timerdata from this file
  //return true on success
  //return false on error
  File f = SPIFFS.open( defaultTimerFile, "r" );
  if (!f)
  {
    Serial.println( F("No default timer file found.") );
    return false;
  }
  String lineBuf;
  byte currentTimer = 0;
  byte thisChannel;
  while ( f.position() < f.size() )
  {
    lineBuf = f.readStringUntil( '\n' );
    if ( lineBuf.indexOf( "[" ) > -1 )
    {
      String thisChannelStr;
      thisChannelStr = lineBuf.substring( lineBuf.indexOf("[") + 1, lineBuf.indexOf("]") );
      thisChannel = thisChannelStr.toInt();
      currentTimer = 0;
    }
    else
    {
      String thisPercentage;
      String thisTime;
      thisTime = lineBuf.substring( 0, lineBuf.indexOf(",") );
      thisPercentage = lineBuf.substring( lineBuf.indexOf(",") + 1 );
      channel[thisChannel].timer[currentTimer].time = thisTime.toInt();
      channel[thisChannel].timer[currentTimer].percentage = thisPercentage.toInt();
      currentTimer++;
      channel[thisChannel].numberOfTimers = currentTimer;
    }
  }
  f.close();
  //add the 24:00 timers ( copy of timer percentage no: 0 )
  for ( thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
  {
    channel[thisChannel].timer[channel[thisChannel].numberOfTimers].time = 86400;
    channel[thisChannel].timer[channel[thisChannel].numberOfTimers].percentage = channel[thisChannel].timer[0].percentage;
  }
  return true;
}

void setEmptyTimers()
{
  for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++)
  {
    channel[channelNumber].timer[0].time = 0;
    channel[channelNumber].timer[0].percentage = 0;
    channel[channelNumber].timer[1].time = 86400;
    channel[channelNumber].timer[1].percentage = 0;
    channel[channelNumber].numberOfTimers = 1;
  }
}
