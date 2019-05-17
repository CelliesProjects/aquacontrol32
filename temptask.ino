void IRAM_ATTR tempTask( void * pvParameters )
{
  OneWire  ds( ONEWIRE_PIN );  /* a 4.7K pull-up resistor is necessary */

  numberOfFoundSensors = 0;
  byte currentAddr[8];

  while ( ds.search( currentAddr ) && numberOfFoundSensors < MAX_NUMBER_OF_SENSORS )
  {
    for ( uint8_t i = 0; i < sizeof( currentAddr ); i++)
    {
      sensor[numberOfFoundSensors].addr[i] = currentAddr[i];
    }
    /* make a key field -in sensorUniqueId- for NVS */
    char sensorUniqueId[17];
    snprintf( sensorUniqueId, sizeof( sensorUniqueId ), "%02x%02x%02x%02x%02x%02x%02x",
              currentAddr[1], currentAddr[2], currentAddr[3], currentAddr[4], currentAddr[5], currentAddr[6], currentAddr[7]  );

    ESP_LOGI( TAG, "Finding saved name for sensor ID: %s", sensorUniqueId );

    /* and read value from NVS or use default name */
    snprintf( sensor[numberOfFoundSensors].name, sizeof( sensor[numberOfFoundSensors].name ),
              preferences.getString( sensorUniqueId, "temp sensor" ).c_str(), numberOfFoundSensors  );

    ESP_LOGD( TAG, "Sensor %i ID: %s - Name: '%s'", numberOfFoundSensors, sensorUniqueId, sensor[numberOfFoundSensors].name );

    numberOfFoundSensors++;
  }
  ESP_LOGI( TAG, "%i Dallas sensors found.", numberOfFoundSensors );

  if ( !numberOfFoundSensors )
  {
    vTaskDelete( NULL );
  }

  /* main temptask loop */

  while (1)
  {
    ds.reset();
    ds.write( 0xCC, 0); /* Skip ROM - All sensors */
    ds.write( 0x44, 0); /* start conversion, with parasite power off at the end */

    vTaskDelay( 750 / portTICK_PERIOD_MS); //wait for conversion ready


    for ( uint8_t thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++)
    {
      byte data[12];

      ds.reset();
      ds.select( sensor[thisSensor].addr );
      ds.write( 0xBE );         /* Read Scratchpad */
      for ( byte i = 0; i < 9; i++)
      { // we need 9 bytes
        data[i] = ds.read(  );
      }


      ESP_LOGD( TAG, "Sensor %i '%s' data=%02x%02x%02x%02x%02x%02x%02x%02x%02x", thisSensor, sensor[thisSensor].name,
                data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8] );

      byte type_s;
      // the first ROM byte indicates which chip
      switch ( sensor[thisSensor].addr[0] )
      {
        case 0x10:
          ESP_LOGD( TAG, "Dallas sensor type : DS18S20" );  /* or old DS1820 */
          type_s = 1;
          break;
        case 0x28:
          ESP_LOGD( TAG, "Dallas sensor type : DS18B20");
          type_s = 0;
          break;
        case 0x22:
          ESP_LOGD( TAG, "Dallas sensor type : DS1822");
          type_s = 0;
          break;
        default:
          ESP_LOGE( TAG, "OneWire device is not a DS18x20 family device.");
          return;
      }

      int16_t raw;
      if ( OneWire::crc8(data, 8) != data[8])
      {
        // CRC of temperature reading indicates an error, so we generate an error log and discard this reading
        writeSensorErrorLog( thisSensor, "BAD_CRC", data );
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
        sensor[thisSensor].tempCelcius = raw / 16.0;

        if ( sensor[ thisSensor ].tempCelcius < -55 || sensor[ thisSensor ].tempCelcius > 125 )    /* temp is outside DS18B20 specs which should never happen */
        {
          writeSensorErrorLog( thisSensor, "BAD_TMP", data );
        }
      }
    }
  }
}
