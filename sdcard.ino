void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

bool defaultTimersLoaded() {                                                      //this function loads the timers or returns FALSE
  //find 'default.aqu' on SD card and if present load the timerdata from this file
  //return false on error
  File f = SD.open( defaultTimerFile, "r" );
  if (!f) {
    Serial.println( F("No default timer file found.") );
    return false;
  }
  String lineBuf;
  byte currentTimer = 0;
  byte thisChannel;
  while ( f.position() < f.size() ) {
    lineBuf = f.readStringUntil( '\n' );
    if ( lineBuf.indexOf( "[" ) > -1 ) {
      String thisChannelStr;
      thisChannelStr = lineBuf.substring( lineBuf.indexOf("[") + 1, lineBuf.indexOf("]") );
      thisChannel = thisChannelStr.toInt();
      currentTimer = 0;
    } else {
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
  for (thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ ) {
    channel[thisChannel].timer[channel[thisChannel].numberOfTimers].time = 86400;
    channel[thisChannel].timer[channel[thisChannel].numberOfTimers].percentage = channel[thisChannel].timer[0].percentage;
  }
/*
  for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
  {
    Serial.print( "Channel " ); Serial.print( thisChannel ); Serial.println( " " );
    for ( byte thisTimer = 0; thisTimer < channel[thisChannel].numberOfTimers; thisTimer++ )
    {
      Serial.print( thisTimer ); Serial.print( " " ); Serial.print( String( channel[thisChannel].timer[thisTimer].time ) ); Serial.print( "'" ); Serial.println( String( channel[thisChannel].timer[thisTimer].percentage ) );
    }
  }
*/
  return true;
}

bool cardReaderPresent()
{
  Serial.print("Initializing SD card...");
  if (!SD.begin( SD_CS, SPI, 2000000 ) ) {
    Serial.println("failed!");
    return false;
  }
  uint8_t cardType = SD.cardType();
  if ( cardType == CARD_NONE ) {
    Serial.println("No SD card attached");
    tft.println("No SD card attached");
    return false;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
    return false;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  tft.printf("SD Card Size: %lluMB\n", cardSize);
  return true;
}
