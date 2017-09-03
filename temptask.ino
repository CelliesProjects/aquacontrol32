void readTempSensors()
{
  for ( byte thisSensor = 1; thisSensor <= numberOfFoundSensors; thisSensor++)
  {
    byte data[12];

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
    //Serial.println();

    byte type_s;
    // the first ROM byte indicates which chip
    switch ( sensor[thisSensor].addr[0] )
    {
      case 0x10:
        //Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
      case 0x28:
        //Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
      case 0x22:
        //Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
      default:
        Serial.println("Device is not a DS18x20 family device.");
        return;
    }

    if ( OneWire::crc8(data, 8) != data[8])
    {
      // CRC of temperature reading indicates an error, so we print a error message and discard this reading
      Serial.print( millis() / 1000.0 ); Serial.print( " - CRC error from device " ); Serial.println( thisSensor );
      ds.reset();
      ds.select( sensor[thisSensor].addr  );
      ds.write( 0x44, 0);        // start conversion, with parasite power off at the end
      return;
    }

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
    sensor[thisSensor].temp = raw;
    ds.reset();
    ds.select( sensor[thisSensor].addr  );
    ds.write( 0x44, 0);        // start conversion, with parasite power off at the end
  }
}

