#include <FFat.h>
#include <OneWire.h>
#include <Preferences.h>

#include "sensorState.h"

static const char * ERROR_LOG_NAME = "/sensor_error.txt";
static const char * UNKNOWN_SENSOR = "unknown sensor";

OneWire ds( SENSOR_PIN );
Preferences sensorPreferences;

sensorState::sensorState() {}

sensorState::~sensorState() {}

bool sensorState::startTask()
{
  if ( nullptr != _pSensorState )
  {
    ESP_LOGE( TAG, "Sensors already running. Exiting." );
    return false;
  }
  _pSensorState = new sensorState();
  if ( nullptr == _pSensorState )
  {
    ESP_LOGE( TAG, "Sensors not created. (low mem?) Exiting." );
    return false;
  }
  sensorPreferences.begin( "sensors", false );
  _pSensorState->setStackSize(1800);
  _pSensorState->setCore(0);
  _pSensorState->setPriority(0);
  _pSensorState->start();
  ESP_LOGI( TAG, "Sensors started." );
  return true;
}

bool sensorState::logging()
{
  return sensorPreferences.getBool( "logging", false );
}

bool sensorState::setLogging( const bool state )
{
  return sensorPreferences.putBool( "logging", state );
};

bool sensorState::errorLogging()
{
  return ( nullptr == _pSensorState ) ? false : _pSensorState->_errorlogging;
};

void sensorState::setErrorLogging( const bool state )
{
  if ( nullptr != _pSensorState ) _pSensorState->_errorlogging = state;
};

uint8_t sensorState::count()
{
  return ( nullptr == _pSensorState ) ? 0 : _pSensorState->_count;
};

float sensorState::temp( const uint8_t num )
{
  return ( nullptr == _pSensorState ) ? NAN : _pSensorState->_state[num].tempCelcius;
};

bool sensorState::error( const uint8_t num )
{
  return ( nullptr == _pSensorState ) ? true : _pSensorState->_state[num].error;
};

bool sensorState::getName( const uint8_t num, sensorName_t &name )
{
  if ( nullptr == _pSensorState ) return false;
  sensorIdStr_t id;
  getId( num, id );
  String result = sensorPreferences.getString( id, UNKNOWN_SENSOR );
  if ( !result ) return false;
  strncpy( name, result.c_str(), sizeof( sensorName_t ) );
  return true;
};

uint8_t sensorState::_scanSensors()
{
  uint8_t currentSensor = 0;
  byte    currentAddr[ sizeof( sensorState_t::addr ) ];

  ds.reset_search();
  ds.target_search(0x28);

  while ( ds.search( currentAddr ) && ( currentSensor < MAX_NUMBER_OF_SENSORS ) )
  {
    memcpy( _tempState[currentSensor].addr, currentAddr, sizeof( sensorState_t::addr ) );
    sensorIdStr_t sensorId;
    snprintf( sensorId, sizeof( sensorId ), "%02x%02x%02x%02x%02x%02x%02x",
              currentAddr[1], currentAddr[2], currentAddr[3], currentAddr[4], currentAddr[5], currentAddr[6], currentAddr[7]  );
    /* and read value from NVS or use default name */
    snprintf( _tempState[currentSensor].name, sizeof( sensorName_t ),
              sensorPreferences.getString( sensorId, UNKNOWN_SENSOR ).c_str() );

    ESP_LOGD( TAG, "Found sensor %i id: %.15s name: '%s'", currentSensor, sensorUniqueId, _tempState[currentSensor].name );

    currentSensor++;
  }
  _rescan = false;
  return currentSensor;
}

bool sensorState::setName( const sensorIdStr_t &id, const char * name )  {
  if ( 0 == strlen( name ) ) return sensorPreferences.remove( id );
  if ( strlen( name ) > sizeof( sensorName_t ) ) return false;
  return sensorPreferences.putString( id, name );
}

void sensorState::getId( const uint8_t num, sensorIdStr_t &id ) {
  snprintf( id, sizeof( sensorIdStr_t ), "%02x%02x%02x%02x%02x%02x%02x",
            _pSensorState->_state[num].addr[1], _pSensorState->_state[num].addr[2], _pSensorState->_state[num].addr[3], _pSensorState->_state[num].addr[4],
            _pSensorState->_state[num].addr[5], _pSensorState->_state[num].addr[6], _pSensorState->_state[num].addr[7] );
}

void sensorState::run( void * data ) {
  uint8_t loopCounter = _scanSensors();

  while (1)
  {
    ESP_LOGD( TAG, "Stack words left: %i",uxTaskGetStackHighWaterMark(NULL) );
    if ( _rescan ) loopCounter = _scanSensors();

    ds.reset();
    ds.write( 0xCC, 0); /* Skip ROM - All sensors */
    ds.write( 0x44, 0); /* start conversion, with parasite power off at the end */

    vTaskDelay( 750 ); //wait for conversion ready

    uint8_t thisSensor = 0;
    while ( thisSensor < loopCounter )
    {
      byte data[12];

      _tempState[thisSensor].error = true; /* we start with an error, which will be cleared if the CRC checks out. */
      ds.reset();
      ds.select( _tempState[thisSensor].addr );
      ds.write( 0xBE );         /* Read Scratchpad */
      for ( byte i = 0; i < 9; i++)
      { // we need 9 bytes
        data[i] = ds.read(  );
      }

      ESP_LOGD( TAG, "Sensor %i '%s' data=%02x%02x%02x%02x%02x%02x%02x%02x%02x", thisSensor, _tempState[thisSensor].name,
                data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8] );

      byte type_s;
      // the first ROM byte indicates which chip
      switch ( _tempState[thisSensor].addr[0] )
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
      }

      if ( OneWire::crc8( data, 8 ) != data[8] )
      {
        _tempState[thisSensor].error = true;
        if ( _errorlogging && !_logError( thisSensor, ERROR_LOG_NAME, "BAD_CRC", data ) )
          ESP_LOGE( TAG, "%s", "Error writing errorlog.(disk full?)" );
      }
      else
      {
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
        _tempState[thisSensor].tempCelcius = raw / 16.0;

        if ( _tempState[thisSensor].tempCelcius <= -40.0 || _tempState[thisSensor].tempCelcius  >= 85.0 )
        {
          _tempState[thisSensor].error = true;
          if ( _errorlogging && !_logError( thisSensor, ERROR_LOG_NAME, "BAD_TMP", data ) )
            ESP_LOGE( TAG, "%s", "Error writing errorlog.(disk full?)" );
        }
        else
          _tempState[thisSensor].error = false;
      }
      ESP_LOGD( TAG, "sensor %i: %.1f %s", thisSensor, _tempState[thisSensor].tempCelcius, _tempState[thisSensor].error ? "invalid" : "valid" );
      thisSensor++;
    }
    memcpy( &_state, &_tempState, sizeof( sensorState_t[ MAX_NUMBER_OF_SENSORS ] ) );
    _count = loopCounter;
  }
}

bool sensorState::_logError( const uint8_t num, const char * path, const char * message, const byte data[9] )
{
  File file = FFat.open( path, FILE_APPEND );
  if ( !file ) return false;

  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  char timeBuff[20];
  strftime ( timeBuff, sizeof(timeBuff), "%x %X", timeinfo );
  char buffer[100];
  snprintf( buffer, sizeof( buffer ), "%s - sensor: '%s' %s %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", timeBuff, _tempState[num].name, message,
            data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8] );
  ESP_LOGE( TAG, "%s", buffer );

  if ( !file.println( buffer ) )
  {
    file.close();
    return false;
  }
  file.close();
  return true;
}
