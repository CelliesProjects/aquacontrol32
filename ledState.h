#ifndef LEDSTATE_H
#define LEDSTATE_H

enum lightState_t
{
  LIGHTS_OFF, LIGHTS_ON, LIGHTS_AUTO
};

class ledState
{
  public:

    ledState() {
      _lightState = LIGHTS_OFF;
    };

    const char *          stateString() {
      return _lightStr[_lightState];
    }

    void                  setState( lightState_t state ) {
      _lightState = state;
    };

    const lightState_t    state() {
      return _lightState;
    };

  private:

    const char * _lightStr[3] = { "LIGHTS OFF", "LIGHTS ON", "LIGHTS AUTO" };

    lightState_t _lightState;
};

#endif
