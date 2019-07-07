#ifndef SENSORSTATE_H
#define SENSORSTATE_H

#ifndef ESP32
#warning sensorState will only work on ESP32 MCUs
#endif

#include "Task.h"

#define SAVED_LOGFILES          30
#define SENSOR_PIN              5
#define MAX_NUMBER_OF_SENSORS   3

#define VALID_ID_LENGTH         14

typedef char sensorId_t[VALID_ID_LENGTH + 1];
typedef char sensorName_t[15];

class sensorState: public Task {

  public:

    struct sensorState_t                  /* struct to keep track of Dallas DS18B20 sensors */
    {
      byte             addr[8] = {};
      float            tempCelsius = NAN;
      bool             error = true;
    };

    sensorState();
    virtual ~sensorState();

    bool                  startSensors();
    void                  scan();
    uint8_t               count();
    float                 temp( const uint8_t num );
    bool                  error( const uint8_t num );
    const char *          readNVS( const sensorName_t &id, sensorName_t &name );
    const char *          getName( const uint8_t num, sensorName_t &name );
    const char *          getId( const uint8_t num, sensorId_t &id );
    bool                  setName( const sensorId_t &id, const char * name );
    bool                  logging();
    bool                  setLogging( const bool state );
    bool                  errorLogging();
    void                  setErrorLogging( const bool state );

  private:

    void                  run( void * data );
    uint8_t               _count = 0;
    uint8_t               _scanSensors();
    sensorState_t         _state[MAX_NUMBER_OF_SENSORS];
    sensorState_t         _tempState[MAX_NUMBER_OF_SENSORS];
    sensorState *         _pSensorState = nullptr;
    bool                  _errorlogging = false;
    bool                  _logError( const uint8_t num, const char * path, const char * message, const byte data[9] );
    bool                  _rescan = false;
};

#endif //SENSORSTATE_H
