#ifndef SENSORSTATE_H
#define SENSORSTATE_H

#ifndef ESP32
#warning Sorry, sensorState will only work on ESP32 MCUs
#endif

#include "Task.h"

#define SAVED_LOGFILES          30
#define SENSOR_PIN              5
#define MAX_NUMBER_OF_SENSORS   3

class sensorState: public Task {

  public:

    struct sensorState_t                  /* struct to keep track of Dallas DS18B20 sensors */
    {
      byte     addr[8] = {};
      float    tempCelcius = NAN;
      char     name[15] = "";
      bool     error = true;
    };

    sensorState();
    virtual ~sensorState();

    bool                  startTask();
    uint8_t               count() const;
    float                 temp( uint8_t num ) const;
    float                 tempFromId( const char * sensorId ) ;
    bool                  error( uint8_t num ) const;
    const char *          name( uint8_t num ) const;
    const char *          nameFromId( const char * id ) const;
    char *                id( uint8_t num );
    bool                  setName( const char * id, const char * name ) const;
    bool                  logging();
    bool                  setLogging( bool state );
    bool                  errorLogging();
    void                  setErrorLogging( bool state );

  private:

    void                  run( void * data );
    uint8_t               _count = 0;
    uint8_t               _scanSensors();
    sensorState_t          _state[MAX_NUMBER_OF_SENSORS];
    sensorState_t         _tempState[MAX_NUMBER_OF_SENSORS];
    sensorState *         _pSensorState = nullptr;
    char                  _idStr[15];
    bool                  _errorlogging = false;
    bool                  _logError( uint8_t num, const char * path, const char * message, const byte data[9] );
};

#endif
