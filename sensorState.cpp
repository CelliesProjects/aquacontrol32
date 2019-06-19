#include <OneWire.h>
#include <Preferences.h>
#include "sensorState.h"

OneWire ds( SENSOR_PIN );
Preferences sensorPreferences;

sensorState::sensorState() {}

sensorState::~sensorState() {}

bool sensorState::startTask()
{
  if ( nullptr != _pSensorState )
  {
    ESP_LOGE( TAG, "Sensor task already running. Exiting." );
    return false;
  }
  //TODO: check if new succeeds
  _pSensorState = new sensorState();
  _pSensorState->setStackSize(2500);
  _pSensorState->setCore(0);
  _pSensorState->setPriority(0);
  _pSensorState->start();
  return true;
}










    uint8_t               sensorState::count() const { return ( nullptr == _pSensorState ) ? 0 : _pSensorState->_count; };
    float                 sensorState::temp( uint8_t num ) const { return ( nullptr == _pSensorState ) ? NAN :_pSensorState->_state[num].tempCelcius; };
    bool                  sensorState::error( uint8_t num ) const { return ( nullptr == _pSensorState ) ? true :  _pSensorState->_state[num].error; };
    const char *          sensorState::name( uint8_t num ) const { return ( nullptr == _pSensorState ) ? "" : _pSensorState->_state[num].name; };

















uint8_t sensorState::_scanSensors()
{
  uint8_t currentSensor = 0;
  byte    currentAddr[ sizeof( sensorState_t::addr ) ];

  ds.reset_search();
  ds.target_search(0x28);

  while ( ds.search( currentAddr ) && ( currentSensor < MAX_NUMBER_OF_SENSORS ) )
  {
    memcpy( _tempState[currentSensor].addr, currentAddr, sizeof( sensorState_t::addr ) );

    /* make a key field -in sensorUniqueId- for NVS */
    char sensorUniqueId[17];
    snprintf( sensorUniqueId, sizeof( sensorUniqueId ), "%02x%02x%02x%02x%02x%02x%02x",
              currentAddr[1], currentAddr[2], currentAddr[3], currentAddr[4], currentAddr[5], currentAddr[6], currentAddr[7]  );

    /* and read value from NVS or use default name */
    snprintf( _tempState[currentSensor].name, sizeof( sensorState_t::name ),
              sensorPreferences.getString( sensorUniqueId, "unknown sensor" ).c_str() );

    ESP_LOGD( TAG, "Found sensor %i id: %.15s name: '%s'", currentSensor, sensorUniqueId, _tempState[currentSensor].name );

    currentSensor++;
  }
  return currentSensor;
}

float sensorState::tempFromId( const char * sensorId )  {
  if ( nullptr == _pSensorState ) return NAN;
  // search id in connected sensors
  uint8_t num = 0;
  uint8_t found = count();
  while ( num < found && 0 != strcmp( sensorId, sensorState::id(num) ) ) num++;
  if ( num == found ) return NAN;
  return _pSensorState->_state[num].tempCelcius;
}

bool sensorState::setName( const char * id, const char * name ) const {
  if ( !id ) return false;
  if ( !name ) return sensorPreferences.remove( id );
  if ( strlen( name ) > sizeof( sensorState_t::name ) ) return false;
  bool result = sensorPreferences.putString( id, name );
  ESP_LOGD( TAG, "Sensor ID: %s new name: '%s' result: %s", id, name, result == true ? "success" : "failed" );
  return result;
}

char * sensorState::id( uint8_t num ) {
  snprintf( sensorState::_idStr, sizeof( sensorState::_idStr ), "%02x%02x%02x%02x%02x%02x%02x",
            _pSensorState->_state[num].addr[1], _pSensorState->_state[num].addr[2], _pSensorState->_state[num].addr[3], _pSensorState->_state[num].addr[4],
            _pSensorState->_state[num].addr[5], _pSensorState->_state[num].addr[6], _pSensorState->_state[num].addr[7] );
  return (char *)sensorState::_idStr;
}

const char * sensorState::nameFromId( const char * id ) const {
  if ( 14 != strlen( id ) ) return "";
  return sensorPreferences.getString( id, "unnamed sensor" ).c_str();
}

void sensorState::run( void * data ) {
  sensorPreferences.begin( "sensors", false );
  while (1)
  {
    uint8_t loopCounter = _scanSensors();
    ds.reset();
    ds.write( 0xCC, 0); /* Skip ROM - All sensors */
    ds.write( 0x44, 0); /* start conversion, with parasite power off at the end */

    vTaskDelay( 750 ); //wait for conversion ready

    uint8_t thisSensor = 0;
    while ( thisSensor < loopCounter )
    {
      ESP_LOGD( TAG, "current sensor: %i", thisSensor);
      byte data[12];

      byte present = ds.reset();
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

      if ( OneWire::crc8( data, 8) != data[8] )
      {
        _tempState[thisSensor].tempCelcius = NAN;
        _tempState[thisSensor].error = true;
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
        _tempState[thisSensor].error = false;
      }
      thisSensor++;
    }
    vTaskSuspendAll();
    memcpy( &_state, &_tempState, sizeof(sensorState_t[MAX_NUMBER_OF_SENSORS]) );
    _count = loopCounter;
    xTaskResumeAll();
  }
}
