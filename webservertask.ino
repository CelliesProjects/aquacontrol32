#include "webif/index_htm.h"
#include "webif/channels_htm.h"
#include "webif/setup_htm.h"
#include "webif/editor_htm.h"
#include "webif/logs_htm.h"
#include "webif/fileman_htm.h"

#define INVALID_CHANNEL 100

const char* HTML_HEADER                       = "text/html";

void webServerTask ( void * pvParameters )
{
  static const char* WWW_USERNAME             = "admin";
  static const char* WWW_DEFAULT_PASSWD       = "esp32";

  static const char* NOT_PRESENT_ERROR_501    = "Not present";

  static const char* PASSWD_KEY_NVS           = "wwwpassword";  //the (changed) admin password is saved in NVS under this key

  static fs::FS &fs = SPIFFS;

  server.on( "/robots.txt", HTTP_GET, []( AsyncWebServerRequest * request )
  {
    request->send( 200, HTML_HEADER, "User-agent: *\nDisallow: /\n" );
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
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HTML_HEADER, index_htm, index_htm_len );
    request->send( response );
  });

  //  /channels or 'channels.htm'
  server.on( "/channels", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HTML_HEADER, channels_htm, channels_htm_len );
    request->send( response );
  });

  //  /editor or 'editor.htm'
  server.on( "/editor", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HTML_HEADER, editor_htm, editor_htm_len );
    request->send( response );
  });

  //  /logs or 'logs.htm'
  server.on( "/logs", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HTML_HEADER, logs_htm, logs_htm_len );
    request->send( response );
  });

  //  /setup or 'setup.htm'
  server.on( "/setup", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HTML_HEADER, setup_htm, setup_htm_len );
    request->send( response );
  });

  //  /filemanager or 'fileman.htm'
  server.on( "/filemanager", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, HTML_HEADER, fileman_htm, fileman_htm_len );
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
      return request->send( 400, HTML_HEADER, "Invalid filename." );
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

    if ( !fs.exists( path ) )
    {
      path = request->arg( "filename" ) + " not found.";
      return request->send( 404, HTML_HEADER, path );
    }
    fs.remove( path );
    path = request->arg( "filename" ) + " deleted.";
    request->send( 200, HTML_HEADER, path );
  });

  server.on( "/api/getdevice", HTTP_GET, []( AsyncWebServerRequest * request)
  {
    AsyncResponseStream *response;
    if ( request->hasArg( "boottime" ) )
    {
      time_t rawtime;
      struct tm * timeinfo;
      char buffer [25];

      time (&rawtime);
      timeinfo = localtime ( &rawtime );
      strftime ( buffer, sizeof(buffer), "%c", timeinfo );
      return request->send( 200, HTML_HEADER, buffer );
    }

    else if ( request->hasArg( "channelcolors" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        response->printf( "%s\n", channel[channelNumber].color );
      }
      return request->send( response );
    }

    else if ( request->hasArg( "channelnames" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        response->printf( "%s\n", channel[channelNumber].name );
      }
      return request->send( response );
    }

    else if ( request->hasArg( "diskspace" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%i" , SPIFFS.totalBytes() - SPIFFS.usedBytes() );
      return request->send( response );
    }

    else if ( request->hasArg( "files" ) )
    {
      File root = fs.open( "/" );
      if ( !root )
      {
        return request->send( 503, HTML_HEADER, "Storage not available." );
      }
      if ( !root.isDirectory() )
      {
        return request->send( 400, HTML_HEADER, "No root on Storage.");
      }
      File file = root.openNextFile();
      if ( !file )
      {
        return request->send( 404 );
      }
      response = request->beginResponseStream( HTML_HEADER );
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
      return request->send( 200, HTML_HEADER, hostName );
    }

    else if ( request->hasArg( "minimumlevels" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
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
        return request->send( 501, HTML_HEADER, NOT_PRESENT_ERROR_501 );
      }
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%i\n%.4f\n", moonData.angle, moonData.percentLit );
      return request->send( response );
    }

    else if ( request->hasArg( "oledcontrast" ) )
    {
      if ( !xOledTaskHandle )
      {
        return request->send( 501, HTML_HEADER, NOT_PRESENT_ERROR_501 );
      }
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%i", oledContrast );
      return request->send( response );
    }

    else if ( request->hasArg( "oledorientation" ) )
    {
      if ( !xOledTaskHandle )
      {
        return request->send( 501, HTML_HEADER, NOT_PRESENT_ERROR_501 );
      }
      return request->send( 200, HTML_HEADER, oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" );
    }

    else if ( request->hasArg( "pwmdepth" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%i", ledcNumberOfBits );
      return request->send( response );
    }

    else if ( request->hasArg( "pwmfrequency" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%.0f", ledcActualFrequency );
      return request->send( response );
    }

    else if ( request->hasArg( "sensor" ) )
    {
      if ( !numberOfFoundSensors )
      {
        return request->send( 501, HTML_HEADER, NOT_PRESENT_ERROR_501 );
      }
      if ( !request->hasArg( "number" ) )
      {
        return request->send( 400, HTML_HEADER, "No sensornumber" );
      }
      uint8_t sensorNumber = request->arg( "number" ).toInt();
      if ( sensorNumber >= numberOfFoundSensors )
      {
        return request->send( 400, HTML_HEADER, "Invalid sensornumber" );
      }
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%s\n%.3f\n%x\n",
                        sensor[sensorNumber].name,
                        sensor[sensorNumber].tempCelcius,
                        sensor[sensorNumber].addr );
      return request->send( response );
    }

    else if ( request->hasArg( "sensorname" ) )
    {
      if ( !numberOfFoundSensors )
      {
        return request->send( 501, HTML_HEADER, "No sensors present." );
      }
      if ( !request->hasArg( "number" ) )
      {
        return request->send( 400, HTML_HEADER, "No sensornumber" );
      }
      uint8_t sensorNumber = request->arg( "number" ).toInt();
      if ( sensorNumber >= numberOfFoundSensors )
      {
        return request->send( 400, HTML_HEADER, "Invalid sensornumber" );
      }
      return request->send( 200, HTML_HEADER, sensor[sensorNumber].name );
    }

    else if ( request->hasArg( "status" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
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
      response->printf( "%s\n%s\n", timeStr, lightStatusToString( lightStatus ) );
      if ( numberOfFoundSensors )
      {
        for ( uint8_t sensorNumber = 0; sensorNumber < numberOfFoundSensors; sensorNumber++ )
        {
          response->printf( "%s,%.3f\n", sensor[sensorNumber].name, sensor[sensorNumber].tempCelcius );
        }
      }
      return request->send( response );
    }

    else if ( request->hasArg( "tftbrightness" ) )
    {
      if ( !xTftTaskHandle )
      {
        return request->send( 501, HTML_HEADER, NOT_PRESENT_ERROR_501 );
      }
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%.2f", tftBrightness );
      return request->send( response );
    }

    else if ( request->hasArg( "tftorientation" ) )
    {
      if ( !xTftTaskHandle )
      {
        return request->send( 501, HTML_HEADER, NOT_PRESENT_ERROR_501 );
      }
      return request->send( 200, HTML_HEADER, ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
    }

    else if ( request->hasArg( "timezone" ) )
    {
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%s", getenv( "TZ" ) );
      return request->send( response );
    }

    else if ( request->hasArg( "version" ) )
    {
      return request->send( 200, HTML_HEADER, sketchVersion );
    }

    else if ( request->hasArg( "wifissid" ) )
    {
      return request->send( 200, HTML_HEADER, WiFi.SSID().c_str() );
    }

    else
    {
      return request->send( 400, HTML_HEADER, "Invalid option" );
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
      return request->send( 400, HTML_HEADER, "Invalid channel" );
    }
    if ( request->hasArg( "color" ) )
    {
      for ( uint8_t currentChar = 0; currentChar < request->arg( "color" ).length(); currentChar++ )
      {
        if ( !isxdigit( request->arg( "color" )[currentChar] ) )
        {
          return request->send( 400, HTML_HEADER, "Invalid char" );
        }
      }
      snprintf( channel[ channelNumber ].color, sizeof( channel[ channelNumber ].color ), "#%s", request->arg( "color" ).c_str() );
      snprintf( nvsKeyname, sizeof( nvsKeyname ), "channelcolor%i", channelNumber );
      preferences.putString( nvsKeyname, channel[channelNumber].color );
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "channel %i color set to %s", channelNumber + 1, channel[ channelNumber ].color );
      return request->send( response );
    }

    else if ( request->hasArg( "minimum" ) )
    {
      float minLevel = request->arg( "minimum" ).toFloat();
      if ( minLevel < 0 || minLevel > 0.991 )
      {
        return request->send( 400, HTML_HEADER, "Invalid level" );
      }
      channel[ channelNumber ].minimumLevel = minLevel;
      snprintf( nvsKeyname, sizeof( nvsKeyname ), "channelminimum%i", channelNumber );
      preferences.putFloat( nvsKeyname, channel[channelNumber].minimumLevel );
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "channel %i minimum set to %.2f%%", channelNumber + 1, channel[ channelNumber ].minimumLevel );
      return request->send( response );
    }

    else if ( request->hasArg( "name" ) )
    {
      for ( uint8_t currentChar = 0; currentChar < request->arg( "name" ).length(); currentChar++ )
      {
        if ( request->arg( "name" )[currentChar] != 0x20  && !isalnum( request->arg( "name" )[currentChar] ) )
        {
          response = request->beginResponseStream( HTML_HEADER );
          response->printf( "Invalid character '%c'.", request->arg( "name" )[currentChar] );
          return request->send( response );
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
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "channel %i name set to '%s'", channelNumber + 1, channel[ channelNumber ].name );
      return request->send( response );
    }

    else
    {
      return request->send( 400, HTML_HEADER, "Invalid option" );
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
      return request->send( 200, HTML_HEADER, "NVS cleared" );
    }

    if ( request->hasArg( "clearwifi" ) )
    {
      request->send( 200, HTML_HEADER, "WiFi data erased" );
      WiFi.disconnect(true);
      return;
    }

    else if ( request->hasArg( "hostname" ) )
    {
      if ( !setupMDNS( request->arg( "hostname" ).c_str() ) )
      {
        char wrongName[81];
        snprintf( wrongName , sizeof( wrongName ), "ERROR: %s is already present.", request->arg( "hostname" ).c_str() );
        return request->send( 400, HTML_HEADER, wrongName );
      }
      snprintf( hostName , sizeof( hostName ), "%s", request->arg( "hostname" ).c_str() );
      preferences.putString( "hostname", hostName );
      return request->send( 200, HTML_HEADER, hostName );
    }

    else if ( request->hasArg( "lightsoff" ) )
    {
      lightsOff();
      return request->send( 200, HTML_HEADER, lightStatusToString( lightStatus ) );
    }

    else if ( request->hasArg( "lightson" ) )
    {
      lightsOn();
      return request->send( 200, HTML_HEADER, lightStatusToString( lightStatus ) );
    }

    else if ( request->hasArg( "lightsprogram" ) )
    {
      lightsAuto();
      return request->send( 200, HTML_HEADER, lightStatusToString( lightStatus ) );
    }

    else if ( request->hasArg( "loadtimers" ) )
    {
      return request->send( 200, HTML_HEADER, defaultTimersLoaded() ? "Timers loaded." : "Not loaded." );
    }

    else if ( request->hasArg( "oledcontrast" ) )
    {
      request->arg( "oledcontrast" );
      uint8_t contrast = request->arg( "oledcontrast" ).toInt();
      if ( contrast < 0 || contrast > 15 )
      {
        return request->send( 400, HTML_HEADER, "Invalid contrast." );
      }
      oledContrast = contrast;
      OLED.setContrast( contrast << 4 );
      preferences.putUInt( "oledcontrast", oledContrast );
      response = request->beginResponseStream( HTML_HEADER );
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
        return request->send( 400, HTML_HEADER, "Invalid orientation" );
      }
      OLED.end();
      OLED.init();
      OLED.setContrast( oledContrast << 0x04 );
      oledOrientation == OLED_ORIENTATION_NORMAL ? OLED.normalDisplay() : OLED.flipScreenVertically();
      preferences.putString( "oledorientation", ( oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" ) );
      return request->send( 200, HTML_HEADER, preferences.getString( "oledorientation" ) );
    }

    else if ( request->hasArg( "password" ) )
    {
      //TODO:check password is valid
      if ( request->arg( "password") == "" )
      {
        return request->send( 400, HTML_HEADER, "Supply a password. Password not changed." );
      }
      //some more tests...
      preferences.putString( PASSWD_KEY_NVS, request->arg( "password") );
      return request->send( 200, HTML_HEADER, "Password saved." );
    }

    else if ( request->hasArg( "pwmdepth" ) )
    {
      uint8_t newPWMDepth = request->arg( "pwmdepth" ).toInt();
      if ( newPWMDepth < 11 || newPWMDepth > 16 )
      {
        return request->send( 400, HTML_HEADER, "Invalid PWM depth" );
      }
      if ( ledcNumberOfBits != newPWMDepth )
      {
        setupDimmerPWMfrequency( LEDC_MAXIMUM_FREQ, newPWMDepth );
        preferences.putUInt( "pwmdepth" , ledcNumberOfBits );
      }
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%i", ledcNumberOfBits );
      return request->send( response );
    }

    else if ( request->hasArg( "pwmfrequency" ) )
    {
      double tempPWMfrequency = request->arg( "pwmfrequency" ).toFloat();
      if ( tempPWMfrequency < 100 || tempPWMfrequency > LEDC_MAXIMUM_FREQ )
      {
        return request->send( 400, HTML_HEADER, "Invalid PWM frequency" );;
      }
      if ( tempPWMfrequency != ledcActualFrequency )
      {
        setupDimmerPWMfrequency( tempPWMfrequency, ledcNumberOfBits );
        preferences.putDouble( "pwmfrequency", ledcActualFrequency );
      }
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%.0f", ledcActualFrequency );
      return request->send( response );
    }

    else if ( request->hasArg( "sensorname" ) )
    {
      if ( request->arg( "sensorname" ).length() >= sizeof( sensor->name ) )
      {
        return request->send( 400, HTML_HEADER, "Sensorname too long" );
      }
      if ( !request->hasArg( "number" ) )
      {
        return request->send( 400, HTML_HEADER, "No sensornumber" );
      }

      uint8_t sensorNumber = request->arg( "number" ).toInt();

      if ( sensorNumber > numberOfFoundSensors )
      {
        return request->send( 400, HTML_HEADER, "Invalid sensornumber" );
      }
      snprintf( sensor[sensorNumber].name, sizeof( sensor->name ), "%s", request->arg( "sensorname" ).c_str() );

      //get the sensor id and save under that key
      char nvsKeyname[16];

      snprintf( nvsKeyname, sizeof( nvsKeyname ), "%02x%02x%02x%02x%02x%02x%02x", sensor[sensorNumber].addr[1], sensor[sensorNumber].addr[2], sensor[sensorNumber].addr[3], sensor[sensorNumber].addr[4], sensor[sensorNumber].addr[5], sensor[sensorNumber].addr[6], sensor[sensorNumber].addr[7] );
      preferences.putString( nvsKeyname, request->arg( "sensorname" ).c_str() );

      ESP_LOGI( TAG, " Saved name '%s' for DS18B20 sensor id: '%s' in NVS.", request->arg( "sensorname" ).c_str(), nvsKeyname );

      return request->send( 200, HTML_HEADER, request->arg( "sensorname" ).c_str() );
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
        return request->send( 400, HTML_HEADER, "Invalid tft orientation." );
      }
      tft.fillScreen( ILI9341_BLACK );
      tft.setRotation( tftOrientation );
      tftClearScreen = true;
      preferences.putString( "tftorientation", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%s", preferences.getString( "tftorientation", "" ).c_str() );
      return request->send( response );
    }

    else if ( request->hasArg( "tftbrightness" ) )
    {
      float brightness = request->arg( "tftbrightness" ).toFloat();
      if ( brightness < 0 || brightness > 100 )
      {
        return request->send( 400, HTML_HEADER, "Invalid tft brightness." );
      }
      tftBrightness = brightness;
      preferences.putFloat( "tftbrightness", brightness );
      response = request->beginResponseStream( HTML_HEADER );
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
        return request->send( 400, HTML_HEADER, "Error setting timezone." );
      }
      response = request->beginResponseStream( HTML_HEADER );
      response->printf( "%s", getenv( "TZ" ) );
      return request->send( response );
    }

    else
    {
      return request->send( 400, HTML_HEADER, "Invalid option" );
    }
  });

  server.on( "/api/upload", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( WWW_USERNAME, preferences.getString( PASSWD_KEY_NVS, WWW_DEFAULT_PASSWD ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse( 200, HTML_HEADER );
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
        request->_tempFile = fs.open( filename, "w" );
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

  server.serveStatic( "/", fs, "/" );

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
