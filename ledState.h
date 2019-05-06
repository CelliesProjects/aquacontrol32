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
      lightState = LIGHTS_OFF;
    };

    const char *          stateString() {                                 // return the current state
      return lightStr[lightState];
    }

    const char *          stateString( const lightState_t state ) {
      return lightStr[state];
    }

    void                  setState( lightState_t state ) {
      lightState = state;
    };

    const lightState_t   getState() {
      return lightState;
    };

  private:

    const char * lightStr[3] = { "LIGHTS OFF", "LIGHTS ON", "LIGHTS AUTO" };

    lightState_t lightState;
};

#endif
