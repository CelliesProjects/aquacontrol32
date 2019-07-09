#include "webif/index_htm.h"
#include "webif/channels_htm.h"
#include "webif/setup_htm.h"
#include "webif/editor_htm.h"
#include "webif/logs_htm.h"
#include "webif/fileman_htm.h"

#define INVALID_CHANNEL 100

void webServerTask ( void * pvParameters )
{
  static const char * WWW_USERNAME             = "admin";
  static const char * WWW_DEFAULT_PASSWD       = "esp32";

  static const char * NOT_PRESENT_ERROR_501    = "Not present";
  static const char * PASSWD_KEY_NVS           = "wwwpassword";  //the (changed) admin password is saved in NVS under this key
  static const char * HEADER_LASTMODIFIED      = "Last-Modified";
  static const char * HEADER_HTML              = "text/html";

  static AsyncWebServer server(80);

  while ( !systemStart.tv_sec )
  {
    delay(100);
  }
  static char date[30];

  strftime( date, sizeof( date ), "%a, %d %b %Y %X GMT", gmtime( &systemStart.tv_sec ) );

  server.on( "/robots.txt", HTTP_GET, []( AsyncWebServerRequest * request )
  {
    request->send( 200, HEADER_HTML, "User-agent: *\nDisallow: /\n" );
  });

  server.on( "/api/login", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( WWW_USERNAME, preferences.getString( PASSWD_KEY_NVS, WWW_DEFAULT_PASSWD ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    request->send( 200 );
  });

  server.on( "/", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    if ( htmlUnmodified( request, date ) ) return request->send(304);
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HEADER_HTML, index_htm, index_htm_len );
    response->addHeader( HEADER_LASTMODIFIED, date );
    request->send( response );
  });

  //  /channels or 'channels.htm'
  server.on( "/channels", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    if ( htmlUnmodified( request, date ) ) return request->send(304);
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HEADER_HTML, channels_htm, channels_htm_len );
    response->addHeader( HEADER_LASTMODIFIED, date );
    request->send( response );
  });

  //  /editor or 'editor.htm'
  server.on( "/editor", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    if ( htmlUnmodified( request, date ) ) return request->send(304);
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HEADER_HTML, editor_htm, editor_htm_len );
    response->addHeader( HEADER_LASTMODIFIED, date );
    request->send( response );
  });

  //  /logs or 'logs.htm'
  server.on( "/logs", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    if ( htmlUnmodified( request, date ) ) return request->send(304);
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HEADER_HTML, logs_htm, logs_htm_len );
    response->addHeader( HEADER_LASTMODIFIED, date );
    request->send( response );
  });

  //  /setup or 'setup.htm'
  server.on( "/setup", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    if ( htmlUnmodified( request, date ) ) return request->send(304);
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HEADER_HTML, setup_htm, setup_htm_len );
    response->addHeader( HEADER_LASTMODIFIED, date );
    request->send( response );
  });

  //  /filemanager or 'fileman.htm'
  server.on( "/filemanager", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    if ( htmlUnmodified( request, date ) ) return request->send(304);
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HEADER_HTML, fileman_htm, fileman_htm_len );
    response->addHeader( HEADER_LASTMODIFIED, date );
    request->send( response );
  });

  /**********************************************************************************************
      api calls
  **********************************************************************************************/

  server.on( "/api/deletefile", HTTP_POST, []( AsyncWebServerRequest * request)
  {
    if ( !request->authenticate( WWW_USERNAME, preferences.getString( PASSWD_KEY_NVS, WWW_DEFAULT_PASSWD ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    if ( !request->hasArg( "filename" ) )
    {
      return request->send( 400, HEADER_HTML, "Invalid filename." );
    }
    String path;
    if ( !request->arg( "filename" ).startsWith( "/" ) )
    {
      path = "/" + request->arg( "filename" );
    }
    else
    {
      path = request->arg( "filename" );
    }

    if ( !FFat.exists( path ) )
    {
      path = request->arg( "filename" ) + " not found.";
      return request->send( 404, HEADER_HTML, path );
    }
    FFat.remove( path );
    path = request->arg( "filename" ) + " deleted.";
    request->send( 200, HEADER_HTML, path );
  });

  server.on( "/api/getdevice", HTTP_GET, []( AsyncWebServerRequest * request)
  {
    AsyncResponseStream *response;
    if ( request->hasArg( "boottime" ) )
    {
      char response[25];

      strftime ( response, sizeof( response ), "%c", localtime ( &systemStart.tv_sec ) );
      return request->send( 200, HEADER_HTML, response );
    }

    else if ( request->hasArg( "channelcolors" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        response->printf( "%s\n", channel[channelNumber].color );
      }
      return request->send( response );
    }

    else if ( request->hasArg( "channelnames" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        response->printf( "%s\n", channel[channelNumber].name );
      }
      return request->send( response );
    }

    else if ( request->hasArg( "diskspace" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%lu", FFat.freeBytes() );
      return request->send( response );
    }

    else if ( request->hasArg( "files" ) )
    {
      File root = FFat.open( "/" );
      if ( !root )
      {
        return request->send( 503, HEADER_HTML, "Storage not available." );
      }
      if ( !root.isDirectory() )
      {
        return request->send( 400, HEADER_HTML, "No root on Storage.");
      }
      File file = root.openNextFile();
      if ( !file )
      {
        return request->send( 404 );
      }
      response = request->beginResponseStream( HEADER_HTML );
      while ( file )
      {
        if ( !file.isDirectory() )
        {
          response->printf( "%s,%s\n", file.name(), humanReadableSize( file.size() ).c_str() );
        }
        file = root.openNextFile();
      }
      return request->send( response );
    }

    else if ( request->hasArg( "hostname" ) )
    {
      return request->send( 200, HEADER_HTML, hostName );
    }

    else if ( request->hasArg( "minimumlevels" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        response->printf( "%.2f\n", channel[channelNumber].minimumLevel );
      }
      return request->send( response );
    }

    else if ( request->hasArg( "moonphase" ) )
    {
      if ( !MOON_SIMULATOR )
      {
        return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      }
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%i\n%.4f\n", moonData.angle, moonData.percentLit );
      return request->send( response );
    }

    else if ( request->hasArg( "oledcontrast" ) )
    {
      if ( !xOledTaskHandle )
      {
        return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      }
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%i", oledContrast );
      return request->send( response );
    }

    else if ( request->hasArg( "oledorientation" ) )
    {
      if ( !xOledTaskHandle )
      {
        return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      }
      return request->send( 200, HEADER_HTML, oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" );
    }

    else if ( request->hasArg( "pwmdepth" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%i", ledcNumberOfBits );
      return request->send( response );
    }

    else if ( request->hasArg( "pwmfrequency" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%.0f", ledcActualFrequency );
      return request->send( response );
    }

    else if ( request->hasArg( "sensor" ) )
    {
      if ( !sensor.count() ) return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      if ( !request->hasArg( "number" ) ) return request->send( 400, HEADER_HTML, "No sensornumber" );
      uint8_t num = request->arg( "number" ).toInt();
      if ( num >= sensor.count() ) return request->send( 400, HEADER_HTML, "Invalid sensornumber" );
      response = request->beginResponseStream( HEADER_HTML );
      sensorName_t name;
      sensorId_t id;
      response->printf( "%s\n%.3f\n%s\n", sensor.getName( num, name ), sensor.temp( num ), sensor.getId( num, id ) );
      return request->send( response );
    }

    else if ( request->hasArg( "sensors" ) )
    {
      if ( !sensor.count() ) return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      sensorName_t name;
      sensorId_t id;
      response = request->beginResponseStream( HEADER_HTML );
      for ( uint8_t num = 0; num < sensor.count(); num++ )
        response->printf( "%s,%.3f,%s\n", sensor.getName( num, name ), sensor.temp( num ), sensor.getId( num, id ) );

      return request->send( response );
    }

    else if ( request->hasArg( "sensorlogging" ) )
    {
      if ( !sensor.count() ) return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      return request->send( 200, HEADER_HTML, sensor.logging() ? "ON" : "OFF" );
    }

    else if ( request->hasArg( "sensorerrorlogging" ) )
    {
      if ( !sensor.count() ) return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      return request->send( 200, HEADER_HTML, sensor.errorLogging() ? "ON" : "OFF" );
    }

    else if ( request->hasArg( "sensorname" ) )
    {
      if ( !sensor.count() ) return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      if ( !request->hasArg( "number" ) ) return request->send( 400, HEADER_HTML, "No sensornumber" );
      uint8_t num = request->arg( "number" ).toInt();
      if ( num >= sensor.count() ) return request->send( 400, HEADER_HTML, "Invalid sensornumber" );
      sensorName_t name;
      return request->send( 200, HEADER_HTML, sensor.getName( num, name ) );
    }

    else if ( request->hasArg( "status" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        char content[8];
        threeDigitPercentage( content, sizeof( content ), channel[channelNumber].currentPercentage, SHOW_PERCENTSIGN );
        response->printf( "%s\n", content );
      }
      static char timeStr[6];
      static time_t then, now;
      now = time(0);
      if ( then != now )
      {
        strftime( timeStr, sizeof( timeStr ),  "%H:%M", localtime( &now ) );
        then = now;
      }
      response->printf( "%s\n%s\n", timeStr, leds.stateString() );

      sensorName_t name;
      for ( uint8_t num = 0; num < sensor.count(); num++ )
        response->printf( "%s,%.3f\n", sensor.getName( num, name ), sensor.temp( num ) );

      return request->send( response );
    }

    else if ( request->hasArg( "tftbrightness" ) )
    {
      if ( !xTftTaskHandle )
      {
        return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      }
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%.2f", tftBrightness );
      return request->send( response );
    }

    else if ( request->hasArg( "tftorientation" ) )
    {
      if ( !xTftTaskHandle )
      {
        return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );
      }
      return request->send( 200, HEADER_HTML, ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
    }

    else if ( request->hasArg( "timezone" ) )
    {
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%s", getenv( "TZ" ) );
      return request->send( response );
    }

    else if ( request->hasArg( "version" ) )
    {
      return request->send( 200, HEADER_HTML, sketchVersion );
    }

    else if ( request->hasArg( "wifissid" ) )
    {
      return request->send( 200, HEADER_HTML, WiFi.SSID().c_str() );
    }

    else
    {
      return request->send( 400, HEADER_HTML, "Invalid option" );
    }
  });

  server.on( "/api/setchannel", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( WWW_USERNAME, preferences.getString( PASSWD_KEY_NVS, WWW_DEFAULT_PASSWD ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    AsyncResponseStream *response;
    uint8_t channelNumber;
    char   nvsKeyname[16];
    channelNumber = checkChannelNumber( request);
    if ( channelNumber == INVALID_CHANNEL )
    {
      return request->send( 400, HEADER_HTML, "Invalid channel" );
    }
    if ( request->hasArg( "color" ) )
    {
      for ( uint8_t currentChar = 0; currentChar < request->arg( "color" ).length(); currentChar++ )
      {
        if ( !isxdigit( request->arg( "color" )[currentChar] ) )
        {
          return request->send( 400, HEADER_HTML, "Invalid char" );
        }
      }
      snprintf( channel[ channelNumber ].color, sizeof( channel[ channelNumber ].color ), "#%s", request->arg( "color" ).c_str() );
      snprintf( nvsKeyname, sizeof( nvsKeyname ), "channelcolor%i", channelNumber );
      preferences.putString( nvsKeyname, channel[channelNumber].color );
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "channel %i color set to %s", channelNumber + 1, channel[ channelNumber ].color );
      return request->send( response );
    }

    else if ( request->hasArg( "minimum" ) )
    {
      float minLevel = request->arg( "minimum" ).toFloat();
      if ( minLevel < 0 || minLevel > 0.991 )
      {
        return request->send( 400, HEADER_HTML, "Invalid level" );
      }
      channel[ channelNumber ].minimumLevel = minLevel;
      snprintf( nvsKeyname, sizeof( nvsKeyname ), "channelminimum%i", channelNumber );
      preferences.putFloat( nvsKeyname, channel[channelNumber].minimumLevel );
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "channel %i minimum set to %.2f%%", channelNumber + 1, channel[ channelNumber ].minimumLevel );
      return request->send( response );
    }

    else if ( request->hasArg( "name" ) )
    {
      for ( uint8_t currentChar = 0; currentChar < request->arg( "name" ).length(); currentChar++ )
      {
        if ( request->arg( "name" )[currentChar] != 0x20  && !isalnum( request->arg( "name" )[currentChar] ) )
        {
          char buffer[30];
          snprintf( buffer, sizeof( buffer ), "Invalid character '%c'.", request->arg( "name" )[currentChar]  );
          return request->send( 400, HEADER_HTML, buffer );
        }
      }
      if ( request->arg( "name" ) != "" )
      {
        strncpy( channel[ channelNumber ].name, request->arg( "name" ).c_str(), sizeof( channel[ channelNumber ].name ) );
      }
      else
      {
        snprintf( channel[ channelNumber ].name, sizeof( channel[ channelNumber ].name ), "Channel%i", channelNumber + 1 );
      }
      snprintf( nvsKeyname, sizeof( nvsKeyname ), "channelname%i", channelNumber );
      preferences.putString( nvsKeyname, channel[channelNumber].name );
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "channel %i name set to '%s'", channelNumber + 1, channel[ channelNumber ].name );
      return request->send( response );
    }

    else
    {
      return request->send( 400, HEADER_HTML, "Invalid option" );
    }
  });

  server.on( "/api/setdevice", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    AsyncResponseStream *response;
    if ( !request->authenticate( WWW_USERNAME, preferences.getString( PASSWD_KEY_NVS, WWW_DEFAULT_PASSWD ).c_str() ) )
    {
      return request->requestAuthentication();
    }

    if ( request->hasArg( "clearnvs" ) )
    {
      preferences.clear();
      return request->send( 200, HEADER_HTML, "NVS cleared" );
    }

    if ( request->hasArg( "clearwifi" ) )
    {
      request->send( 200, HEADER_HTML, "WiFi data erased" );
      WiFi.disconnect(true);
      return;
    }

    else if ( request->hasArg( "hostname" ) )
    {
      if ( !setupMDNS( request->arg( "hostname" ).c_str() ) )
      {
        char wrongName[81];
        snprintf( wrongName , sizeof( wrongName ), "ERROR: %s is already present.", request->arg( "hostname" ).c_str() );
        return request->send( 400, HEADER_HTML, wrongName );
      }
      snprintf( hostName , sizeof( hostName ), "%s", request->arg( "hostname" ).c_str() );
      preferences.putString( "hostname", hostName );
      return request->send( 200, HEADER_HTML, hostName );
    }

    else if ( request->hasArg( "lightsoff" ) )
    {
      leds.setState( LIGHTS_OFF );
      return request->send( 200, HEADER_HTML, leds.stateString() );
    }

    else if ( request->hasArg( "lightson" ) )
    {
      leds.setState( LIGHTS_ON );
      return request->send( 200, HEADER_HTML, leds.stateString() );
    }

    else if ( request->hasArg( "lightsprogram" ) )
    {
      leds.setState( LIGHTS_AUTO );
      return request->send( 200, HEADER_HTML, leds.stateString() );
    }

    else if ( request->hasArg( "loadtimers" ) )
    {
      return request->send( 200, HEADER_HTML, defaultTimersLoaded() ? "Timers loaded." : "Not loaded." );
    }

    else if ( request->hasArg( "oledcontrast" ) )
    {
      request->arg( "oledcontrast" );
      uint8_t contrast = request->arg( "oledcontrast" ).toInt();
      if ( contrast < 0 || contrast > 15 )
      {
        return request->send( 400, HEADER_HTML, "Invalid contrast." );
      }
      oledContrast = contrast;
      OLED.setContrast( contrast << 4 );
      preferences.putUInt( "oledcontrast", oledContrast );
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%i", contrast );
      return request->send( response );
    }

    else if ( request->hasArg( "oledorientation" ) )
    {
      if ( request->arg( "oledorientation" ) == "upsidedown" )
      {
        oledOrientation = OLED_ORIENTATION_UPSIDEDOWN;
      }
      else if ( request->arg( "oledorientation" ) == "normal" )
      {
        oledOrientation = OLED_ORIENTATION_NORMAL;
      }
      else
      {
        return request->send( 400, HEADER_HTML, "Invalid orientation" );
      }
      OLED.end();
      OLED.init();
      OLED.setContrast( oledContrast << 0x04 );
      oledOrientation == OLED_ORIENTATION_NORMAL ? OLED.normalDisplay() : OLED.flipScreenVertically();
      preferences.putString( "oledorientation", ( oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" ) );
      return request->send( 200, HEADER_HTML, preferences.getString( "oledorientation" ) );
    }

    else if ( request->hasArg( "password" ) )
    {
      //TODO:check password is valid
      if ( request->arg( "password") == "" )
      {
        return request->send( 400, HEADER_HTML, "Supply a password. Password not changed." );
      }
      //some more tests...
      preferences.putString( PASSWD_KEY_NVS, request->arg( "password") );
      return request->send( 200, HEADER_HTML, "Password saved." );
    }

    else if ( request->hasArg( "pwmdepth" ) )
    {
      uint8_t newPWMDepth = request->arg( "pwmdepth" ).toInt();
      if ( newPWMDepth < 11 || newPWMDepth > 16 )
      {
        return request->send( 400, HEADER_HTML, "Invalid PWM depth" );
      }
      if ( ledcNumberOfBits != newPWMDepth )
      {
        setupDimmerPWMfrequency( LEDC_MAXIMUM_FREQ, newPWMDepth );
        preferences.putUInt( "pwmdepth" , ledcNumberOfBits );
      }
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%i", ledcNumberOfBits );
      return request->send( response );
    }

    else if ( request->hasArg( "pwmfrequency" ) )
    {
      double tempPWMfrequency = request->arg( "pwmfrequency" ).toFloat();
      if ( tempPWMfrequency < 100 || tempPWMfrequency > LEDC_MAXIMUM_FREQ )
      {
        return request->send( 400, HEADER_HTML, "Invalid PWM frequency" );;
      }
      if ( tempPWMfrequency != ledcActualFrequency )
      {
        setupDimmerPWMfrequency( tempPWMfrequency, ledcNumberOfBits );
        preferences.putDouble( "pwmfrequency", ledcActualFrequency );
      }
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%.0f", ledcActualFrequency );
      return request->send( response );
    }

    else if ( request->hasArg( "sensorlogging" ) )
    {
      if ( !sensor.count() ) return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );

      if ( request->arg( "sensorlogging").equalsIgnoreCase( "on" ) )
      {
        sensor.setLogging( true );
        if ( !xLoggerTaskHandle )
        {
          BaseType_t xReturned = startLogger();
          ESP_LOGI( TAG, "LoggerTask %s.", ( xReturned == pdPASS ) ? "started" : "failed" );
        }
        return request->send( 200, HEADER_HTML, "ON" );
      }
      else if ( request->arg( "sensorlogging" ).equalsIgnoreCase( "off" ) )
      {
        sensor.setLogging( false );
        return request->send( 200, HEADER_HTML, "OFF" );
      }
      else return request->send( 400, HEADER_HTML, "Invalid option." );
    }

    else if ( request->hasArg( "sensorerrorlogging" ) )
    {
      if ( !sensor.count() ) return request->send( 501, HEADER_HTML, NOT_PRESENT_ERROR_501 );

      if ( request->arg( "sensorerrorlogging").equalsIgnoreCase( "on" ) )
      {
        sensor.setErrorLogging( true );
        return request->send( 200, HEADER_HTML, "ON" );
      }
      else if ( request->arg( "sensorerrorlogging" ).equalsIgnoreCase( "off" ) )
      {
        sensor.setErrorLogging( false );
        return request->send( 200, HEADER_HTML, "OFF" );
      }
      else return request->send( 400, HEADER_HTML, "Invalid option." );
    }

    else if ( request->hasArg( "sensorname" ) )
    {
      if ( request->arg( "sensorname" ).length() > sizeof( sensorName_t ) )
        return request->send( 400, HEADER_HTML, "Sensorname too long" );

      if ( !request->hasArg( "number" ) ) return request->send( 400, HEADER_HTML, "No sensornumber" );

      uint8_t num = request->arg( "number" ).toInt();
      if ( num > sensor.count() ) return request->send( 400, HEADER_HTML, "Invalid sensornumber" );
      sensorId_t id;
      sensor.getId( num, id );
      if ( !sensor.setName( id, request->arg( "sensorname" ).c_str() ) )
        ESP_LOGE( TAG, " Error saving name '%s' for DS18B20 sensor id: '%s' in NVS.", request->arg( "sensorname" ).c_str(), id );
      else
        ESP_LOGD( TAG, " Saved name '%s' for DS18B20 sensor id: '%s' in NVS.", request->arg( "sensorname" ).c_str(), id );
      return request->send( 200, HEADER_HTML, request->arg( "sensorname" ).c_str() );
    }

    else if ( request->hasArg( "sensorscan" ) )
    {
      sensor.rescan();
      return request->send( 200, HEADER_HTML );
    }

    else if ( request->hasArg( "tftorientation" ) )
    {
      if (  request->arg( "tftorientation" ) == "normal" )
      {
        tftOrientation = TFT_ORIENTATION_NORMAL;
      }
      else if ( request->arg( "tftorientation" ) == "upsidedown" )
      {
        tftOrientation = TFT_ORIENTATION_UPSIDEDOWN;
      }
      else
      {
        return request->send( 400, HEADER_HTML, "Invalid tft orientation." );
      }
      tft.setRotation( tftOrientation );
      tftClearScreen = true;
      preferences.putString( "tftorientation", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%s", preferences.getString( "tftorientation", "" ).c_str() );
      return request->send( response );
    }

    else if ( request->hasArg( "tftbrightness" ) )
    {
      float brightness = request->arg( "tftbrightness" ).toFloat();
      if ( brightness < 0 || brightness > 100 )
      {
        return request->send( 400, HEADER_HTML, "Invalid tft brightness." );
      }
      tftBrightness = brightness;
      preferences.putFloat( "tftbrightness", brightness );
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%.2f", tftBrightness );
      return request->send( response );
    }

    else if ( request->hasArg( "timezone" ) )
    {
      if ( 0 == setenv( "TZ",  request->arg( "timezone" ).c_str(), 1 )  )
      {
        preferences.putString( "timezone", getenv( "TZ" ) );
      }
      else
      {
        return request->send( 400, HEADER_HTML, "Error setting timezone." );
      }
      response = request->beginResponseStream( HEADER_HTML );
      response->printf( "%s", getenv( "TZ" ) );
      return request->send( response );
    }

    else
    {
      return request->send( 400, HEADER_HTML, "Invalid option" );
    }
  });

  server.on( "/api/upload", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( WWW_USERNAME, preferences.getString( PASSWD_KEY_NVS, WWW_DEFAULT_PASSWD ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse( 200, HEADER_HTML );
    //response->addHeader("Connection", "close");
    request->send(response);
  },
  []( AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final )
  {
    static bool   _authenticated;
    static time_t startTimer;

    if ( !index )
    {
      _authenticated = false;
      if ( request->authenticate( WWW_USERNAME, preferences.getString( PASSWD_KEY_NVS, WWW_DEFAULT_PASSWD ).c_str() ) )
      {
        startTimer = millis();
        ESP_LOGI( TAG, "Starting upload. filename = %s\n", filename.c_str() );
        if ( !filename.startsWith( "/" ) )
        {
          filename = "/" + filename;
        }
        request->_tempFile = FFat.open( filename, "w" );
        _authenticated = true;
      }
      else
      {
        ESP_LOGI( TAG, "Unauthorized access." );
        return request->send( 401, "text/plain", "Not logged in." );
      }
    }

    if ( _authenticated && request->_tempFile && len )
    {
      request->_tempFile.write( data, len );
    }

    if ( _authenticated && final )
    {
      if ( request->_tempFile )
      {
        request->_tempFile.close();
      }
      if ( filename == defaultTimerFile )
      {
        defaultTimersLoaded();
      }

      ESP_LOGI( TAG, "Upload %iBytes in %.2fs which is %.2ikB/s.\n", index, ( millis() - startTimer ) / 1000.0, index / ( millis() - startTimer ) );
    }
  });

  server.serveStatic( "/", FFat, "/" );

  server.onNotFound( []( AsyncWebServerRequest * request )
  {
    ESP_LOGI( TAG, "Not found http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send( 404 );
  });

  DefaultHeaders::Instance().addHeader( "Access-Control-Allow-Origin", "*" );

  server.begin();
  ESP_LOGI( TAG, "HTTP server started." );

  vTaskDelete( NULL );
}

static inline __attribute__((always_inline)) uint8_t checkChannelNumber( const AsyncWebServerRequest *request )
{
  if ( !request->hasArg( "channel" ) )
  {
    return INVALID_CHANNEL;
  }
  else
  {
    uint8_t channelNumber = request->arg( "channel" ).toInt();
    if ( channelNumber < 1 || channelNumber > NUMBER_OF_CHANNELS )
    {
      return INVALID_CHANNEL;
    }
    return channelNumber - 1;
  }
}

/* format bytes as KB, MB or GB string */
static inline __attribute__((always_inline)) String humanReadableSize( const size_t bytes )
{
  if ( bytes < 1024)
  {
    return String( bytes ) + "&nbsp;&nbsp;B";
  } else if ( bytes < ( 1024 * 1024 ) )
  {
    return String( bytes / 1024.0 ) + " KB";
  } else if (bytes < ( 1024 * 1024 * 1024 ) )
  {
    return String( bytes / 1024.0 / 1024.0 ) + " MB";
  } else {
    return String( bytes / 1024.0 / 1024.0 / 1024.0 ) + " GB";
  }
}

bool setupMDNS( const char *hostname )
{
  for ( uint8_t currentChar = 0; currentChar < strlen( hostname ); currentChar++ )
  {
    if ( hostname[currentChar] != 0x20  && !isalnum( hostname[currentChar] ) )
      return false;
  }

  struct ip4_addr addr;
  addr.addr = 0;
  ESP_LOGI( TAG, "Looking for: %s.local...", hostname );
  esp_err_t res = mdns_query_a( hostname, 2000, &addr );

  if ( res != ESP_ERR_NOT_FOUND )
  {
    ESP_LOGI( TAG, "Hostname %s not set because %s is already present!", hostname, hostname );
    return false;
  }

  ESP_LOGI( TAG, "Setting up %s in mDNS.", hostname );
  mdns_hostname_set( hostname );
  mdns_service_add( NULL, "_http", "_tcp", 80, NULL, 0 );
  mdns_service_instance_name_set( "_http", "_tcp", hostname );
  mdns_txt_item_t serviceTxtData[3] = {
    { (char *)"description", (char *)"5 Channel LED controller" },
    { (char *)"source url", (char *)"https://github.com/CelliesProjects/aquacontrol32" },
    { (char *)"name", (char *)"Aquacontrol32" }
  };
  //set txt data for service (will free and replace current data)
  mdns_service_txt_set( "_http", "_tcp", serviceTxtData, 3 );
  preferences.putString( "hostname", hostname );
  return true;
}

static inline __attribute__((always_inline)) bool htmlUnmodified( AsyncWebServerRequest * request, const char * date )
{
  return request->hasHeader( "If-Modified-Since" ) && strcmp( request->header( "If-Modified-Since" ).c_str(), date ) == 0;
}
