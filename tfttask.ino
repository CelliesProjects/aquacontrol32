const bool     TFT_SHOW_RAW           = false;            /* show raw PWM values */
const bool     HIGH_VIZ_PERCENTAGE    = true;             /* show channel percent values in TFT_TEXT_COLOR instead of the channel color to improve visibility */

enum tftFontsize_t {
  SMALL, NORMAL, LARGE
};

enum displayState {
  normal, menu
};

struct tftPoint_t {
  uint16_t x;
  uint16_t y;
};

class tftButton {
  public:

    struct button_t {
      uint16_t       x;
      uint16_t       y;
      uint16_t       w;
      uint16_t       h;
      uint16_t       color;
      uint16_t       bordercolor;
      uint16_t       labelcolor;
      tftFontsize_t  fontsize;
      char           text[25];
      char           label[15];
    };

    void draw( const button_t &button );

    void updateText( const button_t &button );

    inline bool pressed( const button_t &button, const tftPoint_t &location ) {
      return ( location.x > button.x && location.x < button.x + button.w ) && ( location.y > button.y && location.y < button.y + button.h );
    }

    void drawSlider( const button_t &area );

    void updateSlider( const button_t &area, const float &value, const float &rangeLow, const float &rangeHigh );

    void updateSensorLabel( const button_t &tempArea, const char * newLabel );
};

tftButton button;

const tftButton::button_t LIGHTSON_BUTTON {
  10, 60, TFT_BUTTON_WIDTH, TFT_BUTTON_HEIGHT, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_YELLOW, SMALL, "ON"
};

const tftButton::button_t LIGHTSOFF_BUTTON {
  10, 115, TFT_BUTTON_WIDTH, TFT_BUTTON_HEIGHT, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_YELLOW, SMALL, "OFF"
};

const tftButton::button_t LIGHTSAUTO_BUTTON {
  10, 170, TFT_BUTTON_WIDTH, TFT_BUTTON_HEIGHT, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_YELLOW, SMALL, "AUTO"
};

const tftButton::button_t EXIT_BUTTON {
  210, 170, TFT_BUTTON_WIDTH, TFT_BUTTON_HEIGHT, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_YELLOW, SMALL, "EXIT"
};

/* slider for backlight control - note: XPOS is center of slider*/
const uint16_t BL_SLIDER_XPOS = 160;
const uint16_t BL_SLIDER_YPOS = 40;
const uint16_t BL_SLIDER_WIDTH = 60;
const uint16_t BL_SLIDER_HEIGHT = 160;

/* area to check for touch to control backlight */
const tftButton::button_t BL_SLIDER_AREA {
  BL_SLIDER_XPOS - BL_SLIDER_WIDTH / 2, BL_SLIDER_YPOS, BL_SLIDER_WIDTH, BL_SLIDER_HEIGHT, 0, 0, ILI9341_YELLOW, SMALL, "", "back light"
};

/* buttons used on main screen */
const tftButton::button_t MENU_BUTTON {
  210, 10, TFT_BUTTON_WIDTH, TFT_BUTTON_HEIGHT, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_YELLOW, SMALL, "MENU"
};

tftButton::button_t tempArea[NUMBER_OF_SENSORS];


displayState tftState = normal;

bool tftClearScreen = true;

void drawSensors();

void tftTask( void * pvParameters ) {
  const TickType_t tftTaskdelayTime = ( 1000 / UPDATE_FREQ_TFT) / portTICK_PERIOD_MS;

  tft.setTextSize( 2 );
  tft.fillScreen( TFT_BACK_COLOR );

  //* setup backlight pwm *
  ledcAttachPin( TFT_BACKLIGHT_PIN, TFT_BACKLIGHT_CHANNEL );
  double backlightFrequency = ledcSetup( TFT_BACKLIGHT_CHANNEL , LEDC_MAXIMUM_FREQ, TFT_BACKLIGHT_BITDEPTH );

  tftBrightness = preferences.getFloat( "tftbrightness", tftBrightness );
  ledcWrite( TFT_BACKLIGHT_CHANNEL, map( tftBrightness, 0, 100, 0, TFT_BACKLIGHT_MAXPWM ) );

  ( preferences.getString( "tftorientation", "normal" ).equals( "normal" ) ) ? tftOrientation = TFT_ORIENTATION_NORMAL : tftOrientation = TFT_ORIENTATION_UPSIDEDOWN;
  tft.setRotation( tftOrientation );

  touch.begin();

  tft.println( "Aquacontrol32");

  ESP_LOGI( TAG, "started a ILI9341 display on SPI." );

  while ( !xDimmerTaskHandle ) vTaskDelay( 10 / portTICK_PERIOD_MS );

  ledcWrite( TFT_BACKLIGHT_CHANNEL, 0 );

  while (1) {
    switch ( tftState ) {
      case normal: {
          showStatus();
          break;
        }
      case menu: {
          showMenu();
          break;
        }
      default:
        break;
    }
    vTaskDelay( tftTaskdelayTime );
  }
}

void showMenu() {
  static lightState_t displayedLightStatus;

  if ( tftClearScreen ) {
    tft.fillScreen( TFT_BACK_COLOR );
    drawMenuButtons();
    button.drawSlider( BL_SLIDER_AREA );
    button.updateSlider( BL_SLIDER_AREA, tftBrightness, 0, 100 );

    tftButton::button_t versionString = { 0, ILI9341_TFTWIDTH - 20, ILI9341_TFTHEIGHT, 20, TFT_BACK_COLOR, ILI9341_RED, ILI9341_YELLOW, LARGE };

    snprintf( versionString.text, sizeof( versionString.text ), sketchVersion );
    button.updateText( versionString );
    displayedLightStatus = leds.state();
    ledcWrite( TFT_BACKLIGHT_CHANNEL, map( tftBrightness, 0, 100, 0, TFT_BACKLIGHT_MAXPWM ) );
    tftClearScreen = false;
  }

  /* check if tftBrightness has changed */
  if ( map( tftBrightness, 0, 100, 0, TFT_BACKLIGHT_MAXPWM ) != ledcRead( TFT_BACKLIGHT_CHANNEL ) ) {
    ledcWrite( TFT_BACKLIGHT_CHANNEL, map( tftBrightness, 0, 100, 0, TFT_BACKLIGHT_MAXPWM ) ); /* set new backlight value */
    button.updateSlider( BL_SLIDER_AREA, tftBrightness, 0, 100 );
  }

  /* check if light status has changed */
  if ( displayedLightStatus != leds.state() ) {
    drawMenuButtons();
    displayedLightStatus = leds.state();
  }

  /* process touch screen input */
  if ( touch.tirqTouched() ) {
    TS_Point p = touch.getPoint();
    tftPoint_t clickedLocation = mapToTft( p.x, p.y );

    if ( button.pressed( LIGHTSON_BUTTON , clickedLocation ) )
      leds.setState( LIGHTS_ON );

    else if ( button.pressed( LIGHTSOFF_BUTTON , clickedLocation ) )
      leds.setState( LIGHTS_OFF );

    else if ( button.pressed( LIGHTSAUTO_BUTTON , clickedLocation ) )
      leds.setState( LIGHTS_AUTO );

    else if ( button.pressed( BL_SLIDER_AREA , clickedLocation ) ) {
      static uint16_t oldLocation;
      if (  clickedLocation.y != oldLocation ) {
        tftBrightness = map( clickedLocation.y , BL_SLIDER_YPOS, BL_SLIDER_HEIGHT + BL_SLIDER_YPOS, 100, 0 );
        preferences.putFloat( "tftbrightness", tftBrightness );
        oldLocation = clickedLocation.y;
      }
    }

    else if ( button.pressed( EXIT_BUTTON , clickedLocation ) ) {
      ledcWrite( TFT_BACKLIGHT_CHANNEL, 0 );
      tftClearScreen = true;
      tftState = normal;
    }
  }
}

void newSensors() {
  tft.startWrite();
  tft.writeFillRect( 210, 60, TFT_BUTTON_WIDTH, 120, TFT_BACK_COLOR );
  tft.endWrite();

  sensorName_t name;
  for ( uint8_t num = 0; num < logger.sensorCount(); num++ ) {
    tempArea[num].x = 220;
    tempArea[num].y = 70 + num * 40;
    tempArea[num].w = TFT_BUTTON_WIDTH - 20;
    tempArea[num].h = 30;
    tempArea[num].color = TFT_BACK_COLOR;
    tempArea[num].labelcolor = ILI9341_WHITE;
    tempArea[num].fontsize = LARGE;
    button.updateSensorLabel( tempArea[num], logger.getSensorName( num, name ) );
  }
}

void showStatus() {
  const uint16_t BARS_BOTTOM      = 190;
  const uint16_t BARS_HEIGHT      = BARS_BOTTOM - 10;
  const uint16_t BARS_BORDER      = 12;
  const uint16_t BARS_WIDTH       = 210 / 5; /* note: total width is 210 px */
  const float    HEIGHT_FACTOR    = BARS_HEIGHT / 100.0;

  static wl_status_t   displayedWiFiStatus;
  static lightState_t displayedLightStatus;

  uint16_t channelColor565[NUMBER_OF_CHANNELS];

  if ( tftClearScreen ) {
    tft.fillScreen( TFT_BACK_COLOR );
    button.draw( MENU_BUTTON );
    showIPAddress();
    displayedWiFiStatus = WiFi.status();
  }

  static float oldPercentage[NUMBER_OF_CHANNELS];
  static uint16_t oldColor565[NUMBER_OF_CHANNELS];

  tft.startWrite();
  for ( uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++ ) {
    uint32_t color = strtol( &channel[num].color[1], NULL, 16 );
    channelColor565[num] = tft.color565( ( color & 0xFF0000 ) >> 16, ( color & 0x00FF00 ) >> 8, color & 0x0000FF  );

    /* only draw channels that changed percentage or color */
    if ( tftClearScreen || oldPercentage[ num ] != channel[num].currentPercentage ||
         oldColor565[ num ] != channelColor565[ num ] ) {
      // redraw the top part of the bar
      tft.writeFillRect( num * BARS_WIDTH + BARS_BORDER,
                         BARS_BOTTOM - BARS_HEIGHT,
                         BARS_WIDTH - BARS_BORDER * 2,
                         BARS_HEIGHT - channel[num].currentPercentage * HEIGHT_FACTOR + 1, /* + 1 is a fudge factor to delete the bottom pixel TODO: fix it... */
                         TFT_BACK_COLOR );
      // redraw the bottom part of the bar
      tft.writeFillRect( num * BARS_WIDTH + BARS_BORDER,
                         BARS_BOTTOM - channel[num].currentPercentage * HEIGHT_FACTOR,
                         BARS_WIDTH - BARS_BORDER * 2,
                         channel[num].currentPercentage * HEIGHT_FACTOR,
                         channelColor565[num]);
    }
  }
  tft.endWrite();

  uint32_t averageLedBrightness = 0;

  for ( uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++ ) {
    /* only write percentage if changed */
    if ( tftClearScreen || channel[num].currentPercentage != oldPercentage[num] ) {
      tftButton::button_t label;

      if ( TFT_SHOW_RAW )
        snprintf( label.text, sizeof( label.text ), "%04X", ledcRead( num ) );
      else
        threeDigitPercentage( label.text, sizeof( label.text ), channel[num].currentPercentage, SHOW_PERCENTSIGN );
      label.x = num * BARS_WIDTH;
      label.y = BARS_BOTTOM + 4;
      label.w = BARS_WIDTH;
      label.h = 10;
      label.fontsize = SMALL;
      label.color = TFT_BACK_COLOR;
      label.bordercolor = 0;
      label.labelcolor = channelColor565[num];
      int16_t x, y;
      uint16_t w, h;

      tft.setTextSize( SMALL );
      tft.getTextBounds( label.text, 0, 0, &x, &y, &w, &h );
      tft.setCursor( ( label.x + label.w / 2 ) - ( w / 2 ),
                     ( label.y + label.h / 2 ) - ( h / 2 ) );
      if ( HIGH_VIZ_PERCENTAGE )
        tft.setTextColor( TFT_TEXT_COLOR, TFT_BACK_COLOR );
      else
        tft.setTextColor( channelColor565[num], TFT_BACK_COLOR );
      tft.print(label.text);
    }
    oldColor565[ num ] = channelColor565[ num];
    oldPercentage[ num ] = channel[num].currentPercentage;
    averageLedBrightness += ledcRead( num ); //backlight will be set later on...
  }

  static uint8_t      lastCount{0};
  static sensorName_t displayedName[NUMBER_OF_SENSORS];
  static float        displayedTemp[NUMBER_OF_SENSORS];

  if ( tftClearScreen || logger.sensorCount() != lastCount ) {
    memset( displayedName, 0, sizeof( displayedName ) );
    newSensors();
    lastCount = logger.sensorCount();
  }

  for ( uint8_t num = 0; num < logger.sensorCount(); num++ ) {
    //if the name changed update the display
    sensorName_t name;
    logger.getSensorName( num, name );
    if ( tftClearScreen || strcmp( displayedName[num], name ) ) {
      button.updateSensorLabel( tempArea[num], name );
      memcpy( displayedName[num], name, sizeof( sensorName_t ) );
    }

    // if the temperature changed update the display
    if ( tftClearScreen || ( displayedTemp[num] != logger.sensorTemp( num ) ) ) {
      snprintf( tempArea[num].text, sizeof( tempArea[num].text ), " %.1f%c ", logger.sensorTemp( num ), char(247) );
      button.updateText( tempArea[num] );
      displayedTemp[num] = logger.sensorTemp( num );
    }
  }

  tftClearScreen = false;

  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_r( &now, &timeinfo );

  const tftButton::button_t clockArea {
    10, 205, 110, 30, TFT_BACK_COLOR, ILI9341_YELLOW, TFT_TEXT_COLOR, LARGE
  };
  static uint16_t oldtimeinfo;
  if ( timeinfo.tm_sec != oldtimeinfo ) {
    char buff[15];
    strftime( buff, sizeof( buff ), "%T", &timeinfo );
    int16_t x, y;
    uint16_t w, h;
    tft.setTextSize( clockArea.fontsize );
    tft.getTextBounds( buff, 0, 0, &x, &y, &w, &h);
    tft.setCursor( ( clockArea.x + clockArea.w / 2 ) - ( w / 2 ),
                   ( clockArea.y + clockArea.h / 2 ) - ( h / 2 ) );
    tft.setTextColor( TFT_TEXT_COLOR, TFT_BACK_COLOR );
    tft.print( buff );
    oldtimeinfo = timeinfo.tm_sec;
  }

  if ( displayedWiFiStatus != WiFi.status() ) {
    showIPAddress( );
    displayedWiFiStatus = WiFi.status();
  }
  //all screen items are refreshed, now lets set the backlight

  averageLedBrightness = averageLedBrightness / NUMBER_OF_CHANNELS;

  uint16_t rawBrightness = map( tftBrightness, 0, 100, 0, TFT_BACKLIGHT_MAXPWM );

  ledcWrite( TFT_BACKLIGHT_CHANNEL, ( averageLedBrightness > rawBrightness ) ? rawBrightness : averageLedBrightness );

  if ( touch.tirqTouched() ) {
    tftPoint_t clickedLocation;

    TS_Point p = touch.getPoint();
    clickedLocation = mapToTft( p.x, p.y );
    if ( button.pressed( MENU_BUTTON , clickedLocation ) ) {
      ledcWrite( TFT_BACKLIGHT_CHANNEL, 0 );
      tftState = menu;
      tftClearScreen = true;
    }
  }
}

void drawMenuButtons() {
  tftButton::button_t tempButton;

  tempButton = LIGHTSON_BUTTON;
  tempButton.color = ( leds.state() == LIGHTS_ON ) ? ILI9341_RED : ILI9341_BLUE;
  button.draw( tempButton );

  tempButton = LIGHTSOFF_BUTTON;
  tempButton.color = ( leds.state() == LIGHTS_OFF ) ? ILI9341_RED : ILI9341_BLUE;
  button.draw( tempButton );

  tempButton = LIGHTSAUTO_BUTTON;
  tempButton.color = ( leds.state() == LIGHTS_AUTO ) ? ILI9341_RED : ILI9341_BLUE;
  button.draw( tempButton );

  button.draw( EXIT_BUTTON );
}

uint16_t mapUint16( const uint16_t &x, const uint16_t &in_min, const uint16_t &in_max, const uint16_t &out_min, const uint16_t &out_max) {
  return ( x - in_min ) * ( out_max - out_min ) / ( in_max - in_min ) + out_min;
}

struct tftPoint_t mapToTft( const uint16_t &touchX, const uint16_t &touchY ) {
  const uint16_t XPT_RES = 4096;

  if ( !TOUCH_IS_INVERTED ) {
    if (  tftOrientation == TFT_ORIENTATION_UPSIDEDOWN )
      return { mapUint16( touchX, 0, XPT_RES, 0, 320 ),
               mapUint16( touchY, 0, XPT_RES, 0, 240 ) };

    else if ( tftOrientation == TFT_ORIENTATION_NORMAL )
      return { mapUint16( touchX, 0, XPT_RES, 320, 0 ),
               mapUint16( touchY, 0, XPT_RES, 240, 0 ) };
  }
  else {
    if (  tftOrientation == TFT_ORIENTATION_UPSIDEDOWN )
      return { mapUint16( touchX, 0, XPT_RES, 320, 0 ),
               mapUint16( touchY, 0, XPT_RES, 240, 0 ) };

    else if ( tftOrientation == TFT_ORIENTATION_NORMAL )
      return { mapUint16( touchX, 0, XPT_RES, 0, 320 ),
               mapUint16( touchY, 0, XPT_RES, 0, 240 ) };
  }
}

void showIPAddress() {
  const tftButton::button_t networkArea {
    140, 205, 170, 30, 0, ILI9341_YELLOW, ILI9341_YELLOW, LARGE, "", ""
  };
  char buff[15];
  int16_t x, y;
  uint16_t w, h;
  if (!WiFi.isConnected())
    snprintf(buff, sizeof(buff), "%s", "WiFi ERROR");
  else
    snprintf(buff, sizeof(buff), "%s", WiFi.localIP().toString().c_str());
  tft.fillRect(networkArea.x, networkArea.y, networkArea.w, networkArea.h, TFT_BACK_COLOR);
  tft.setTextSize(networkArea.fontsize);
  tft.getTextBounds(buff, 0, 0, &x, &y, &w, &h);
  tft.setCursor((networkArea.x + networkArea.w / 2) - w / 2,
                (networkArea.y + ( networkArea.h / 2) - h / 2));
  tft.setTextColor(TFT_TEXT_COLOR, TFT_BACK_COLOR);
  tft.print(buff);
}

void drawSensors() {
  sensorName_t name;
  for ( uint8_t num = 0; num < logger.sensorCount(); num++ )
    button.updateSensorLabel( tempArea[num], logger.getSensorName( num, name ) );
}

//tftButton:: functions

void tftButton::drawSlider( const button_t &area ) {
  tft.startWrite();
  tft.writeFillRect( area.x, area.y, area.w, area.h, TFT_BACK_COLOR );
  tft.endWrite();

  int16_t x, y;
  uint16_t w, h;

  if ( area.label ) {
    tft.setTextSize(SMALL);
    tft.getTextBounds( (char *)area.label, 0, 0, &x, &y, &w, &h );
    tft.setCursor( ( area.x + area.w / 2 ) - w / 2,
                   ( area.y + area.h + 3 ) );
    tft.setTextColor( area.labelcolor, TFT_BACK_COLOR );
    tft.print( area.label );
  }

  if ( area.text ) {
    tft.setTextSize(SMALL);
    tft.getTextBounds( (char *)area.text, 0, 0, &x, &y, &w, &h );
    tft.setCursor( ( area.x + area.w / 2 ) - w / 2,
                   ( area.y - 17 ) );
    tft.setTextColor( area.labelcolor, area.color );
    tft.print( area.text );
  }
}

void tftButton::updateSlider( const button_t &area, const float &value, const float &rangeLow, const float &rangeHigh ) {
  static int16_t oldpos = 0;
  //delete (overwrite) old knob
  tftButton::button_t knob = { area.x, 0, BL_SLIDER_WIDTH, 10, TFT_BACK_COLOR, TFT_BACK_COLOR };
  knob.y = oldpos - 5;
  button.draw( knob );

  //redraw vertical slider bar
  tft.startWrite();
  tft.writeFillRect( area.x + area.w / 2  - 1, area.y, 2, area.h, ILI9341_YELLOW );
  tft.endWrite();

  int16_t ypos = map( value, rangeLow, rangeHigh, area.h + area.y, area.y );
  knob = { area.x, 0, BL_SLIDER_WIDTH, 10, ILI9341_YELLOW };
  knob.y = ypos - 5;
  button.draw( knob );
  oldpos = ypos;
  snprintf( knob.text, sizeof( knob.text ), " %i%% ", int(value) );
  int16_t x, y;
  uint16_t w, h;
  tft.setTextSize(LARGE);
  tft.getTextBounds( (char *)knob.text, 0, 0, &x, &y, &w, &h );
  tft.setCursor( ( area.x + area.w / 2 ) - ( w / 2 ),
                 ( area.y - 20 ) );
  tft.setTextColor( TFT_TEXT_COLOR, TFT_BACK_COLOR );
  tft.print( knob.text );
}

void tftButton::updateText( const button_t &button ) {
  int16_t x, y;
  uint16_t w, h;
  tft.setTextSize( button.fontsize );
  tft.getTextBounds( (char *)button.text, 0, 0, &x, &y, &w, &h );
  tft.setCursor( ( button.x + button.w / 2 ) - ( w / 2 ),
                 ( button.y + button.h / 2 ) - ( h / 2 ) );
  tft.setTextColor( button.labelcolor, button.color );
  tft.print( button.text );
}

void tftButton::draw( const tftButton::button_t &button ) {
  tft.startWrite();
  tft.writeFillRect( button.x, button.y, button.w, button.h, button.color );
  tft.writeFastHLine( button.x, button.y, button.w, button.bordercolor );
  tft.writeFastHLine( button.x, button.y + button.h, button.w, button.bordercolor );
  tft.writeFastVLine( button.x, button.y, button.h, button.bordercolor );
  tft.writeFastVLine( button.x + button.w, button.y, button.h, button.bordercolor );
  tft.endWrite();
  if ( button.text )
    updateText( button );
  if ( button.label ) {
    int16_t x, y;
    uint16_t w, h;
    tft.getTextBounds( (char *)button.text, 0, 0, &x, &y, &w, &h );
    tft.setCursor( ( button.x + button.w / 2 ) - w / 2,
                   ( button.y - 10 ) );
    tft.print( button.label );
  }
}

void tftButton::updateSensorLabel( const tftButton::button_t &tempArea, const char * newLabel ) {
  int16_t x, y;
  uint16_t w, h;

  tft.fillRect( tempArea.x - 2, tempArea.y - 6, tempArea.w + 4, 8, TFT_BACK_COLOR );
  tft.setTextSize( SMALL );
  tft.getTextBounds( newLabel, 0, 0, &x, &y, &w, &h);
  tft.setCursor( ( tempArea.x + tempArea.w / 2 ) - w / 2,
                 ( tempArea.y - 6 ) );
  tft.print( newLabel );
}
