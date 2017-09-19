bool defaultTimersLoaded()
{
  //find 'default.aqu' on SD card and if present load the timerdata from this file
  //return false on error
  File f = SD.open( defaultTimerFile, "r" );
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

bool cardReaderPresent()
{
  Serial.println("Initializing SD card...");
  if (!SD.begin( SD_CS, SPI, 2000000 ) )
  {
    Serial.println("failed!");
    return false;
  }
  uint8_t cardType = SD.cardType();
  if ( cardType == CARD_NONE )
  {
    Serial.println("No SD card attached");
    tft.println("No SD card attached");
    return false;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  } else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
    return false;
  }

  uint64_t cardSize = SD.cardSize() / ( 1024 * 1024 );
  Serial.printf( "SD Card Size: %lluMB\n", cardSize );
  tft.printf( "SD Card Size: %lluMB\n", cardSize );
  return true;
}
