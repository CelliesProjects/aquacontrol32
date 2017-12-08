const uint16_t TFT_BACK_COLOR         = ILI9341_BLACK;
const bool     TFT_SHOW_RAW           = false;            /* show raw PWM values */

const uint16_t TFT_TEXT_COLOR         = ILI9341_YELLOW;
const uint16_t TFT_DATE_COLOR         = ILI9341_WHITE;
const uint16_t TFT_TEMP_COLOR         = ILI9341_WHITE;
const uint8_t  TFT_BACKLIGHT_BITDEPTH = 16;               /*min 11 bits, max 16 bits */
const uint8_t  TFT_BACKLIGHT_CHANNEL  = NUMBER_OF_CHANNELS;

enum displayState {
  normal, menu
};

struct button_t
{
  uint16_t x;
  uint16_t y;
  int16_t w;
  int16_t h;
  char text[10];
};

const uint16_t BUTTON_WIDTH  = 100;
const uint16_t BUTTON_HEIGHT =  40;

const button_t LIGHTON_BUTTON
{
  10, 10, BUTTON_WIDTH, BUTTON_HEIGHT, "ON"
};

const button_t LIGHTOFF_BUTTON
{
  10, 90, BUTTON_WIDTH, BUTTON_HEIGHT, "OFF"
};

const button_t LIGHTAUTO_BUTTON
{
  10, 170, BUTTON_WIDTH, BUTTON_HEIGHT, "AUTO"
};

const button_t EXIT_BUTTON
{
  210, 170, BUTTON_WIDTH, BUTTON_HEIGHT, "EXIT"
};

const button_t MENU_BUTTON
{
  210, 10, BUTTON_WIDTH, BUTTON_HEIGHT, "MENU"
};

bool clearScreen                       = true;

uint16_t backlightMaxvalue;

displayState tftState = normal;

void tftTask( void * pvParameters )
{
  const time_t tftTaskdelayTime         =   ( 1000 / UPDATE_FREQ_TFT) / portTICK_PERIOD_MS;

  tft.fillScreen( TFT_BACK_COLOR );

  /* setup backlight pwm */
  ledcAttachPin( TFT_BACKLIGHT_PIN, TFT_BACKLIGHT_CHANNEL );
  double backlightFrequency = ledcSetup( TFT_BACKLIGHT_CHANNEL , LEDC_MAXIMUM_FREQ, TFT_BACKLIGHT_BITDEPTH );

  backlightMaxvalue = ( 0x00000001 << TFT_BACKLIGHT_BITDEPTH ) - 1;

  tftBrightness = readInt8NVS( "tftbrightness", tftBrightness );
  ledcWrite( TFT_BACKLIGHT_CHANNEL, map( tftBrightness, 0, 100, 0, backlightMaxvalue ) );

  ( readStringNVS( "tftorientation", "normal" ) == "normal" ) ? tftOrientation = TFT_ORIENTATION_NORMAL : tftOrientation = TFT_ORIENTATION_UPSIDEDOWN;
  tft.setRotation( tftOrientation );
  while ( !xDimmerTaskHandle )
  {
    vTaskDelay( 10 / portTICK_PERIOD_MS );
  }
  ledcWrite( TFT_BACKLIGHT_CHANNEL, 0 );

  while (1)
  {
    switch ( tftState )
    {
      case normal: showStatus();
        break;
      case menu:   showMenu();
        break;
      default:     break;
    }
    vTaskDelay( tftTaskdelayTime / portTICK_PERIOD_MS );
  }
}

void drawMenuButtons()
{
  drawButton( LIGHTON_BUTTON,     lightStatus == LIGHTS_ON  ? ILI9341_RED : ILI9341_BLUE );
  drawButton( LIGHTOFF_BUTTON,    lightStatus == LIGHTS_OFF ? ILI9341_RED : ILI9341_BLUE );
  drawButton( LIGHTAUTO_BUTTON,   lightStatus == LIGHTS_AUTO ? ILI9341_RED : ILI9341_BLUE );
  drawButton( EXIT_BUTTON, ILI9341_BLUE );
}

void showMenu()
{
  if ( clearScreen )
  {
    ledcWrite( TFT_BACKLIGHT_CHANNEL, 0 );
    tft.fillScreen( ILI9341_BLACK );
    drawMenuButtons();
    ledcWrite( TFT_BACKLIGHT_CHANNEL, map( tftBrightness, 0, 100, 0, backlightMaxvalue ) );
    clearScreen = false;
  }

  if ( touch.touched() )
  {
    TS_Point p = touch.getPoint();

    if ( buttonPressed( LIGHTON_BUTTON , p ) )
    {
      lightsOn();
      drawMenuButtons();
    }
    else if ( buttonPressed( LIGHTOFF_BUTTON , p ) )
    {
      lightsOff();
      drawMenuButtons();
    }
    else if ( buttonPressed( LIGHTAUTO_BUTTON , p ) )
    {
      lightsAuto();
      drawMenuButtons();
    }
    else if ( buttonPressed( EXIT_BUTTON , p ) )
    {
      ledcWrite( TFT_BACKLIGHT_CHANNEL, 0 );
      clearScreen = true;
      tftState = normal;
    }
  }
}

void showStatus()
{
  const uint16_t BARS_BOTTOM      = 205;
  const uint16_t BARS_HEIGHT      = BARS_BOTTOM;
  const uint16_t BARS_BORDER      = 10;
  const uint16_t BARS_WIDTH       = 200 / 5;
  const float    HEIGHT_FACTOR    = BARS_HEIGHT / 100.0;

  uint16_t channelColor565[NUMBER_OF_CHANNELS];

  if ( clearScreen )
  {
    tft.fillScreen( ILI9341_BLACK );
    clearScreen = false;
    drawButton( MENU_BUTTON, ILI9341_BLUE );
  }

  tft.startWrite();
  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    // redraw the top part of the bar
    tft.writeFillRect( channelNumber * BARS_WIDTH + BARS_BORDER,
                       BARS_BOTTOM - BARS_HEIGHT,
                       BARS_WIDTH - BARS_BORDER * 2,
                       BARS_HEIGHT - channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                       TFT_BACK_COLOR );
    /*
          //100% water mark
          tft.drawFastHLine( channelNumber * BARS_WIDTH + BARS_BORDER,
                             BARS_BOTTOM - BARS_HEIGHT - 1,
                             BARS_WIDTH - BARS_BORDER * 2,
                             tft.color565( r, g, b ) );
    */

    uint32_t color = strtol( &channel[channelNumber].color[1], NULL, 16 );
    channelColor565[channelNumber] = tft.color565( ( color & 0xFF0000 ) >> 16, ( color & 0x00FF00 ) >> 8, color & 0x0000FF  );

    // redraw the bottom part of the bar
    tft.writeFillRect( channelNumber * BARS_WIDTH + BARS_BORDER,
                       BARS_BOTTOM - channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                       BARS_WIDTH - BARS_BORDER * 2,
                       channel[channelNumber].currentPercentage * HEIGHT_FACTOR,
                       channelColor565[channelNumber]);
  }
  tft.endWrite();

  uint32_t averageLedBrightness = 0;

  for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
  {
    tft.setCursor( channelNumber * BARS_WIDTH + 2, BARS_BOTTOM + 4 );
    tft.setTextSize( 1 );
    tft.setTextColor( channelColor565[channelNumber] , TFT_BACK_COLOR );

    char content[8];
    if ( TFT_SHOW_RAW )
    {
      snprintf( content, sizeof( content ), " 0x%04X", ledcRead( channelNumber ) );
    }
    else
    {
      snprintf( content, sizeof( content ), "%*" ".1f%%", 5, channel[channelNumber].currentPercentage );
    }
    tft.print( content );
    averageLedBrightness += ledcRead( channelNumber );
  }
  averageLedBrightness = averageLedBrightness / NUMBER_OF_CHANNELS;

  uint16_t rawBrightness = map( tftBrightness, 0, 100, 0, backlightMaxvalue );

  ledcWrite( TFT_BACKLIGHT_CHANNEL, ( averageLedBrightness > rawBrightness ) ? rawBrightness : averageLedBrightness );

  //draw temps under the menu button
  tft.setTextSize( 2 );
  if ( numberOfFoundSensors )
  {
    tft.setTextColor( TFT_TEMP_COLOR , TFT_BACK_COLOR );
    for ( uint8_t thisSensor = 0; thisSensor < numberOfFoundSensors; thisSensor++ )
    {
      tft.setCursor( 200, 40 + thisSensor * 30 );
      //char content[10];
      //snprintf( content, sizeof( content ), "%.1f%cC", sensor[thisSensor].tempCelcius, char(247) );
      //tft.printf( " %.1f%cC", sensor[thisSensor].tempCelcius, char(247) );
      button_t tempArea;
      tempArea.x = 210;
      tempArea.y = 90 + thisSensor * 30;
      tempArea.w = BUTTON_WIDTH;
      tempArea.h = 20;
      snprintf( tempArea.text, sizeof( tempArea.text ), "%.1f%c", sensor[thisSensor].tempCelcius, char(247) );

      drawButton( tempArea, 0 );
    }
  }

  struct tm timeinfo;
  getLocalTime( &timeinfo );
  tft.setCursor( 18, BARS_BOTTOM + 15 );
  tft.setTextColor( TFT_DATE_COLOR , TFT_BACK_COLOR );
  tft.print( asctime( &timeinfo ) );


  if ( touch.touched() )
  {
    TS_Point p = touch.getPoint();
    if ( buttonPressed( MENU_BUTTON , p ) )
    {
      tftState = menu;
      clearScreen = true;
    }

  }
}

static inline __attribute__((always_inline)) void drawButton( struct button_t button, uint16_t color )
{
  tft.setTextColor( ILI9341_YELLOW, color );
  if ( color )
  {
    tft.fillRect( button.x, button.y, button.w, button.h, color );
  }
  int16_t x, y;
  uint16_t w, h;
  tft.getTextBounds( button.text, 0, 0, &x, &y, &w, &h);
  tft.setCursor( ( button.x + button.w / 2 ) - w / 2,
                 ( button.y + button.h / 2 ) - h / 2 );
  tft.print( button.text );
}

static inline __attribute__((always_inline)) bool buttonPressed( struct button_t button, const TS_Point p )
{
  uint16_t x, y;

  if ( tftOrientation == TFT_ORIENTATION_UPSIDEDOWN )
  {
    x = mapFloat( p.x, 340, 3900, 0, 320 );
    y = mapFloat( p.y, 200, 3850, 0, 240 );
  }
  else if ( tftOrientation == TFT_ORIENTATION_NORMAL )
  {
    x = mapFloat( p.x, 340, 3900, 320, 0 );
    y = mapFloat( p.y, 200, 3850, 240, 0 );
  }
  //tft.drawPixel( x, y, ILI9341_GREEN );  //debug
  return ( x > button.x && x < button.x + button.w ) && ( y > button.y && y < button.y + button.h );
}
