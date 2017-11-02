//#define SHOW_DALLAS_ERROR  /* uncomment to show Dallas ( CRC ) errors on Serial. */

void tempTask( void * pvParameters )
{
  numberOfFoundSensors = 0;
  byte currentAddr[8];

  while ( ds.search( currentAddr ) && numberOfFoundSensors < MAX_NUMBER_OF_SENSORS )
  {
    //Serial.write( "Sensor "); Serial.print( counter ); Serial.print( ":" );
    for ( byte i = 0; i < 8; i++)
    {
      //Serial.write(' ');
      //Serial.print( currentAddr[i], HEX );
      sensor[numberOfFoundSensors].addr[i] = currentAddr[i];
    }

    /* make a key field -in buff- for NVS */
    char buff[15];
    snprintf( buff, sizeof( buff ), "sensorname%i", numberOfFoundSensors );

    /* and read value from NVS or use default name */ 
    snprintf( sensor[numberOfFoundSensors].name, sizeof( sensor[numberOfFoundSensors].name ), readStringNVS( buff, "temp sensor" ).c_str(), numberOfFoundSensors  );
    numberOfFoundSensors++;
  }
  Serial.printf( "%i Dallas sensors found.\n", numberOfFoundSensors );

  if ( !numberOfFoundSensors )
  {
    vTaskDelete( NULL );
  }

  /* main temptask loop */

  while (1)
  {
    for ( byte thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++ )
    {
      ds.reset();
      ds.select( sensor[thisSensor].addr );
      ds.write( 0x44, 0);        // start conversion, with parasite power off at the end
    }

    vTaskDelay( 750 / portTICK_PERIOD_MS); //wait for conversion ready

    for ( byte thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++)
    {
      byte data[12];
      ds.reset();
      ds.select( sensor[thisSensor].addr );
      ds.write( 0xBE );         // Read Scratchpad

      //Serial.print( "Sensor " );Serial.print( thisSensor ); Serial.print("  Data = ");
      //Serial.println( present, HEX );
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
#ifdef SHOW_DALLAS_ERROR
          Serial.println("Device is not a DS18x20 family device.");
#endif
          return;
      }

      int16_t raw;
      if ( OneWire::crc8(data, 8) != data[8])
      {
#ifdef SHOW_DALLAS_ERROR
        // CRC of temperature reading indicates an error, so we print a error message and discard this reading
        Serial.print( millis() / 1000.0 ); Serial.print( " - CRC error from device " ); Serial.println( thisSensor );
#endif
      }
      else
      {
        raw = (data[1] << 8) | data[0];
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
      }
    }
  }
}
