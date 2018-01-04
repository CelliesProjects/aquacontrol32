//include all web interface header files
//https://stackoverflow.com/questions/8707183/script-tool-to-convert-file-to-c-c-source-code-array/8707241#8707241
#include "webif/index_htm.h"
#include "webif/channels_htm.h"
#include "webif/setup_htm.h"
#include "webif/editor_htm.h"
#include "webif/logs_htm.h"
#include "webif/fileman_htm.h"

/**************************************************************************
       Username and password for web interface
 *************************************************************************/
const char* www_username       = "admin";
const char* www_default_passw  = "esp32";

//the (changed) admin password is saved in NVS under this key
const char* passwordKeyNVS   = "wwwpassword";

const char* textPlainHeader  = "text/plain";
const char* textHtmlHeader   = "text/html";

void webServerTask ( void * pvParameters )
{
  Serial.println( "Starting webserver setup. " );

  server.on( "/api/login", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( www_username, readStringNVS( passwordKeyNVS, www_default_passw ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    request->send( 200 );
  });

  server.on( "/", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, textHtmlHeader, index_htm, index_htm_len );
    request->send( response );
  });

  //  /channels or 'channels.htm'
  server.on( "/channels", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, textHtmlHeader, channels_htm, channels_htm_len );
    request->send( response );
  });

  //  /editor or 'editor.htm'
  server.on( "/editor", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, textHtmlHeader, editor_htm, editor_htm_len );
    request->send( response );
  });

  //  /logs or 'logs.htm'
  server.on( "/logs", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, textHtmlHeader, logs_htm, logs_htm_len );
    request->send( response );
  });

  //  /setup or 'setup.htm'
  server.on( "/setup", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, textHtmlHeader, setup_htm, setup_htm_len );
    request->send( response );
  });

  //  /filemanager or 'fileman.htm'
  server.on( "/filemanager", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, textHtmlHeader, fileman_htm, fileman_htm_len );
    request->send( response );
  });

  /**********************************************************************************************
      api calls
  **********************************************************************************************/

  server.on( "/api/deletefile", HTTP_POST, []( AsyncWebServerRequest * request)
  {
    if ( !request->authenticate( www_username, readStringNVS( passwordKeyNVS, www_default_passw ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    if ( !request->hasArg( "filename" ) )
    {
      return request->send( 400, textPlainHeader, "Invalid filename." );
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

    if ( !SPIFFS.exists( path ) )
    {
      path = request->arg( "filename" ) + " not found.";
      return request->send( 404, textPlainHeader, path );
    }
    SPIFFS.remove( path );
    path = request->arg( "filename" ) + " deleted.";
    request->send( 200, textPlainHeader, path );
  });

  server.on( "/api/getdevice", HTTP_GET, []( AsyncWebServerRequest * request)
  {
    char content[1024];

    if ( request->hasArg( "boottime" ) )
    {
      snprintf( content, sizeof( content ), "%s", asctime( localtime( &systemStart.tv_sec ) ) );
    }
    else if ( request->hasArg( "channelcolors" ) )
    {
      uint8_t charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", channel[channelNumber].color.c_str() );
      }
    }
    else if ( request->hasArg( "channelnames" ) )
    {
      uint8_t charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", channel[channelNumber].name.c_str() );
      }
    }
    else if ( request->hasArg( "diskspace" ) )
    {
      snprintf( content, sizeof( content ), "%i" , SPIFFS.totalBytes() - SPIFFS.usedBytes() );
    }
    else if ( request->hasArg( "files" ) )
    {
      File root = SPIFFS.open( "/" );
      if ( !root )
      {
        return request->send( 503, textPlainHeader, "SD card not available." );
      }
      if ( !root.isDirectory() )
      {
        return request->send( 400, textPlainHeader, "Not a directory");
      }
      File file = root.openNextFile();
      uint16_t charCount = 0;
      content[0] = 0;        /* solves webif filemanager showing garbage when no files on sd */
      while ( file )
      {
        if ( !file.isDirectory() )
        {
          size_t fileSize = file.size();
          charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s,%s\n", file.name(), humanReadableSize( fileSize ).c_str() );
        }
        file = root.openNextFile();
      }
    }
    else if ( request->hasArg( "hostname" ) )
    {
      snprintf( content, sizeof( content ), "%s", hostName );
    }
    else if ( request->hasArg( "minimumlevels" ) )
    {
      uint8_t charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.2f\n", channel[channelNumber].minimumLevel );
      }
    }
    else if ( request->hasArg( "oledcontrast" ) )
    {
      snprintf( content, sizeof( content ), "%i", oledContrast );
    }
    else if ( request->hasArg( "oledorientation" ) )
    {
      if ( xOledTaskHandle )
      {
        snprintf( content, sizeof( content ), "%s", oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" );
      }
      else
      {
        return request->send( 501, textPlainHeader, "Not present." );
      }
    }
    else if ( request->hasArg( "pwmdepth" ) )
    {
      snprintf( content, sizeof( content ), "%i", ledcNumberOfBits );
    }
    else if ( request->hasArg( "pwmfrequency" ) )
    {
      snprintf( content, sizeof( content ), "%.0f", ledcActualFrequency );
    }

    else if ( request->hasArg( "tftorientation" ) )
    {
      if ( xTftTaskHandle )
      {
        snprintf( content, sizeof( content ), "%s", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
      }
      else
      {
        return request->send( 501, textPlainHeader, "Not present." );
      }
    }

    else if ( request->hasArg( "sensor" ) )
    {
      if ( !numberOfFoundSensors )
      {
        return request->send( 501, textPlainHeader, "No sensors present." );
      }

      if ( !request->hasArg( "number" ) )
      {
        return request->send( 400, textPlainHeader, "No sensornumber" );
      }

      uint8_t sensorNumber = request->arg( "number" ).toInt();

      if ( sensorNumber >= numberOfFoundSensors )
      {
        return request->send( 400, textPlainHeader, "Invalid sensornumber" );
      }

      uint8_t charCount = 0;

      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", sensor[sensorNumber].name );
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%f\n", sensor[sensorNumber].tempCelcius );
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%x\n", sensor[sensorNumber].addr );
    }

    else if ( request->hasArg( "sensorname" ) )
    {
      if ( !numberOfFoundSensors )
      {
        return request->send( 501, textPlainHeader, "No sensors present." );
      }

      if ( !request->hasArg( "number" ) )
      {
        return request->send( 400, textPlainHeader, "No sensornumber" );
      }

      uint8_t sensorNumber = request->arg( "number" ).toInt();

      if ( sensorNumber >= numberOfFoundSensors )
      {
        return request->send( 400, textPlainHeader, "Invalid sensornumber" );
      }
      snprintf( content, sizeof( content ), "%s", sensor[sensorNumber].name );
    }

    else if ( request->hasArg( "status" ) )
    {
      uint8_t charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.2f\n", channel[channelNumber].currentPercentage );
      }
      time_t now = time(0);
      charCount += strftime( content + charCount, sizeof( content ) - charCount, "%T\n", localtime( &now ) );
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", ToString( lightStatus ) );
      if ( numberOfFoundSensors )
      {
        for ( uint8_t sensorNumber = 0; sensorNumber < numberOfFoundSensors; sensorNumber++ )
        {
          charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s,%.3f\n", sensor[sensorNumber].name, sensor[sensorNumber].tempCelcius );
        }
      }
    }

    else if ( request->hasArg( "tftbrightness" ) )
    {
      if ( xTftTaskHandle )
      {
        snprintf( content, sizeof( content ), "%.2f", tftBrightness );
      }
      else
      {
        return request->send( 501, textPlainHeader, "Not present." );
      }
    }

    else if ( request->hasArg( "timezone" ) )
    {
      snprintf( content, sizeof( content ), "%s", getenv( "TZ" ) );
    }


    else if ( request->hasArg( "version" ) )
    {
      snprintf( content, sizeof( content ), "Version %s - compile date: %s", SKETCH_VERSION, __DATE__ );
    }

    else
    {
      return request->send( 400, textPlainHeader, "Invalid option" );
    }


    request->send( 200, textPlainHeader, content );
  });

  server.on( "/api/setchannel", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( www_username, readStringNVS( passwordKeyNVS, www_default_passw ).c_str() ) )
    {
      return request->requestAuthentication();
    }

    uint8_t channelNumber;
    char   content[100];

    channelNumber = checkChannelNumber( request);
    if ( channelNumber == -1 )
    {
      return request->send( 400, textPlainHeader, "Invalid channel" );
    }

    if ( request->hasArg( "color" ) )
    {
      for ( uint8_t currentChar = 0; currentChar < request->arg( "color" ).length(); currentChar++ )
      {
        if ( !isxdigit( request->arg( "color" )[currentChar] ) )
        {
          return request->send( 400, textPlainHeader, "Invalid char" );
        }
      }
      channel[ channelNumber ].color = "#" + request->arg( "color" );

      //pre-use 'content' buffer
      snprintf( content, sizeof( content ), "channelcolor%i", channelNumber );
      saveStringNVS( content, channel[channelNumber].color );

      snprintf( content, sizeof( content ), "channel %i color set to %s", channelNumber + 1, channel[ channelNumber ].color.c_str() );
    }



    else if ( request->hasArg( "minimum" ) )
    {
      float minLevel = request->arg( "minimum" ).toFloat();
      if ( minLevel < 0 || minLevel > 0.991 )
      {
        return request->send( 400, textPlainHeader, "Invalid level" );
      }
      channel[ channelNumber ].minimumLevel = minLevel;

      //pre-use 'content' buffer
      snprintf( content, sizeof( content ), "channelminimum%i", channelNumber );
      saveFloatNVS( content, channel[channelNumber].minimumLevel );

      snprintf( content, sizeof( content ), "channel %i minimum set to %.2f%%", channelNumber + 1, channel[ channelNumber ].minimumLevel );
    }

    else if ( request->hasArg( "name" ) )
    {
      for ( uint8_t currentChar = 0; currentChar < request->arg( "name" ).length(); currentChar++ )
      {
        if ( request->arg( "name" )[currentChar] != 0x20  && !isalnum( request->arg( "name" )[currentChar] ) )
        {
          snprintf( content, sizeof( content ), "Invalid character '%c'.", request->arg( "name" )[currentChar] );
          return request->send( 400, textPlainHeader, content );
        }
      }
      if ( request->arg( "name" ) != "" )
      {
        channel[ channelNumber ].name = request->arg( "name" );
      }
      else
      {
        channel[ channelNumber ].name = "Channel" + String( channelNumber + 1 );
      }

      //pre-use 'content' buffer
      snprintf( content, sizeof( content ), "channelname%i", channelNumber );
      saveStringNVS( content, channel[channelNumber].name );

      snprintf( content, sizeof( content ), "channel %i name set to '%s'", channelNumber + 1, channel[ channelNumber ].name.c_str() );
    }



    else
    {
      return request->send( 400, textPlainHeader, "Invalid option" );
    }

    request->send( 200, textPlainHeader, content );
  });

  server.on( "/api/setdevice", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( www_username, readStringNVS( passwordKeyNVS, www_default_passw ).c_str() ) )
    {
      return request->requestAuthentication();
    }

    char content[100];

    if ( request->hasArg( "clearnvs" ) )
    {
      clearNVS();
      snprintf( content, sizeof( content ), "NVS cleared" );
    }



    else if ( request->hasArg( "hostname" ) )
    {
      if ( !setupMDNS( request->arg( "hostname" ) ) )
      {
        return request->send( 400, textPlainHeader, "name not available." );
      }
      snprintf( hostName , sizeof( hostName ), "%s", request->arg( "hostname" ).c_str() );
      saveStringNVS( "hostname", hostName );
      snprintf( content, sizeof( content ), "%s", hostName );
    }



    else if ( request->hasArg( "lightsoff" ) )
    {
      lightsOff();
      snprintf( content, sizeof( content ), "%s", ToString( lightStatus ) );
    }



    else if ( request->hasArg( "lightson" ) )
    {
      lightsOn();
      snprintf( content, sizeof( content ), "%s", ToString( lightStatus ) );
    }



    else if ( request->hasArg( "lightsprogram" ) )
    {
      lightsAuto();
      snprintf( content, sizeof( content ), "%s", ToString( lightStatus ) );
    }



    else if ( request->hasArg( "loadtimers" ) )
    {
      snprintf( content, sizeof( content ), "%s", defaultTimersLoaded() ? "Timers loaded." : "Not loaded." );
    }



    else if ( request->hasArg( "oledcontrast" ) )
    {
      request->arg( "oledcontrast" );
      int8_t contrast = request->arg( "oledcontrast" ).toInt();
      if ( contrast < 0 || contrast > 15 )
      {
        return request->send( 400, textPlainHeader, "Invalid contrast." );
      }
      oledContrast = contrast;
      OLED.setContrast( contrast << 4 );
      saveUint8NVS( "oledcontrast", oledContrast );
      snprintf( content, sizeof( content ), "%i", contrast );
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
        return request->send( 400, textPlainHeader, "Invalid orientation" );
      }
      OLED.end();
      OLED.init();
      OLED.setContrast( oledContrast << 0x04 );
      oledOrientation == OLED_ORIENTATION_NORMAL ? OLED.normalDisplay() : OLED.flipScreenVertically();
      saveStringNVS( "oledorientation", ( oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" ) );
      snprintf( content, sizeof( content ), "%s", oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" );
    }



    else if ( request->hasArg( "password" ) )
    {
      //TODO:check password is valid
      if ( request->arg( "password") == "" )
      {
        return request->send( 400, textPlainHeader, "Supply a password. Password not changed." );
      }

      //some more tests...

      saveStringNVS( passwordKeyNVS, request->arg( "password") );

      strncpy( content, "Password saved.", sizeof( content ) );
    }



    else if ( request->hasArg( "pwmdepth" ) )
    {
      uint8_t newPWMDepth = request->arg( "pwmdepth" ).toInt();
      if ( newPWMDepth < 11 || newPWMDepth > 16 )
      {
        return request->send( 400, textPlainHeader, "Invalid PWM depth" );
      }
      if ( ledcNumberOfBits != newPWMDepth )
      {
        setupDimmerPWMfrequency( LEDC_MAXIMUM_FREQ, newPWMDepth );
        saveInt8NVS( "pwmdepth" , ledcNumberOfBits );
      }
      snprintf( content, sizeof( content ), "%i", ledcNumberOfBits );
    }



    else if ( request->hasArg( "pwmfrequency" ) )
    {
      double tempPWMfrequency = request->arg( "pwmfrequency" ).toFloat();
      if ( tempPWMfrequency < 100 || tempPWMfrequency > LEDC_MAXIMUM_FREQ )
      {
        request->send( 200, textPlainHeader, "Invalid PWM frequency" );
        return;
      }
      if ( tempPWMfrequency != ledcActualFrequency )
      {
        setupDimmerPWMfrequency( tempPWMfrequency, ledcNumberOfBits );
        saveDoubleNVS( "pwmfrequency", ledcActualFrequency );
      }
      snprintf( content, sizeof( content ), "%.0f", ledcActualFrequency );
    }



    else if ( request->hasArg( "sensorname" ) )
    {
      if ( !request->hasArg( "number" ) )
      {
        return request->send( 400, textPlainHeader, "No sensornumber" );
      }

      uint8_t sensorNumber = request->arg( "number" ).toInt();

      if ( sensorNumber > numberOfFoundSensors )
      {
        return request->send( 400, textPlainHeader, "Invalid sensornumber" );
      }

      snprintf( sensor[sensorNumber].name, sizeof( sensor[sensorNumber].name ), "%s", request->arg( "sensorname" ).c_str() );

      snprintf( content, sizeof( content ), "sensorname%i", sensorNumber );
      saveStringNVS( content, request->arg( "sensorname" ).c_str() );

      Serial.printf( " Saved '%s' as '%s'\n", request->arg( "sensorname" ).c_str(), content );

      snprintf( content , sizeof( content ), "%s", request->arg( "sensorname" ).c_str() );
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
        return request->send( 400, textPlainHeader, "Invalid tft orientation." );
      }
      tft.setRotation( tftOrientation );
      tft.fillScreen( ILI9341_BLACK );
      tftClearScreen = true;
      saveStringNVS( "tftorientation", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
      snprintf( content, sizeof( content ), "%s", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
    }



    else if ( request->hasArg( "tftbrightness" ) )
    {
      float brightness = request->arg( "tftbrightness" ).toFloat();
      if ( brightness < 0 || brightness > 100 )
      {
        return request->send( 400, textPlainHeader, "Invalid tft brightness." );
      }
      tftBrightness = brightness;
      saveInt8NVS( "tftbrightness", brightness );
      snprintf( content, sizeof( content ), "%.2f", tftBrightness );
    }



    else if ( request->hasArg( "timezone" ) )
    {
      //Serial.printf( request->arg( "timezone" ) );
      urlDecode( request->arg( "timezone" ) );
      if ( 0 == setenv( "TZ",  request->arg( "timezone" ).c_str(), 1 )  )
      {
        saveStringNVS( "timezone", getenv( "TZ" ) );
      }
      else
      {
        return request->send( 400, textPlainHeader, "Error setting timezone." );
      }
      snprintf( content, sizeof( content ), "%s", getenv( "TZ" ) );
    }



    else
    {
      return request->send( 400, textPlainHeader, "Invalid option" );
    }

    request->send( 200, textPlainHeader, content );
  });

  server.on( "/api/upload", HTTP_POST, []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( www_username, readStringNVS( passwordKeyNVS, www_default_passw ).c_str() ) )
    {
      return request->requestAuthentication();
    }
    request->send( 200 );
  },
  []( AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final )
  {
    static File   newFile;
    static bool   _authenticated;
    static time_t startTimer;

    if ( !index )
    {
      _authenticated = false;
      if ( request->authenticate( www_username, readStringNVS( passwordKeyNVS, www_default_passw ).c_str() ) )
      {
        startTimer = millis();
        Serial.printf( "Starting upload. filename = %s\n", filename.c_str() );
        if ( !filename.startsWith( "/" ) )
        {
          filename = "/" + filename;
        }
        request->_tempFile = SPIFFS.open( filename, "w" );
        _authenticated = true;
      }
      else
      {
        Serial.println( "Unauthorized access." );
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
      Serial.printf( "Upload %iBytes in %.2fs which is %.2ikB/s.\n", index, ( millis() - startTimer ) / 1000.0, index / ( millis() - startTimer ) );
    }
  });

  server.serveStatic( "/", SPIFFS, "/" );

  server.onNotFound( []( AsyncWebServerRequest * request )
  {
    const char* notFound = "NOT_FOUND: ";

    if ( request->method() == HTTP_GET )
      Serial.printf( "%s GET", notFound );
    else if (request->method() == HTTP_POST)
      Serial.printf("%s POST", notFound );
    else if (request->method() == HTTP_DELETE)
      Serial.printf("%s DELETE", notFound );
    else if (request->method() == HTTP_PUT)
      Serial.printf("%s PUT", notFound );
    else if (request->method() == HTTP_PATCH)
      Serial.printf("%s PATCH", notFound );
    else if (request->method() == HTTP_HEAD)
      Serial.printf("%s HEAD", notFound );
    else if (request->method() == HTTP_OPTIONS)
      Serial.printf("%s OPTIONS", notFound );
    else
      Serial.printf("%s UNKNOWN", notFound );
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send( 404 );
  });

  DefaultHeaders::Instance().addHeader( "Access-Control-Allow-Origin", "*" );

  server.begin();
  Serial.println("HTTP server setup done.");

  vTaskDelete( NULL );
}

int8_t checkChannelNumber( const AsyncWebServerRequest *request )
{
  if ( !request->hasArg( "channel" ) )
  {
    return -1;
  }
  else
  {
    int8_t channelNumber = request->arg( "channel" ).toInt();
    if ( channelNumber < 1 || channelNumber > NUMBER_OF_CHANNELS )
    {
      return -1;
    }
    return channelNumber - 1;
  }
}

/* DOES NOT RETURN IF INVALID str OFFERED!    */
/* valid str contain only alphanumeric en spaces */
void abortOnInvalidChar( const String str, AsyncWebServerRequest * request )
{
  for ( uint8_t currentChar = 0; currentChar < str.length(); currentChar++ )
  {
    if ( str[currentChar] != 0x20  && !isalnum( str[currentChar] ) )
    {
      char content[25];
      snprintf( content, sizeof( content ), "Invalid character '%c'.", str[currentChar] );
      return request->send( 400, textPlainHeader, content );
    }
  }
}
/*
  Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  Modified 8 May 2015 by Hristo Gochkov (proper post and file upload handling)
*/

String getContentType( const String& path )
{
  if (path.endsWith(".html")) return "text/html";
  else if (path.endsWith(".htm")) return "text/html";
  else if (path.endsWith(".css")) return "text/css";
  else if (path.endsWith(".txt")) return "text/plain";
  else if (path.endsWith(".js")) return "application/javascript";
  else if (path.endsWith(".png")) return "image/png";
  else if (path.endsWith(".gif")) return "image/gif";
  else if (path.endsWith(".jpg")) return "image/jpeg";
  else if (path.endsWith(".ico")) return "image/x-icon";
  else if (path.endsWith(".svg")) return "image/svg+xml";
  else if (path.endsWith(".ttf")) return "application/x-font-ttf";
  else if (path.endsWith(".otf")) return "application/x-font-opentype";
  else if (path.endsWith(".woff")) return "application/font-woff";
  else if (path.endsWith(".woff2")) return "application/font-woff2";
  else if (path.endsWith(".eot")) return "application/vnd.ms-fontobject";
  else if (path.endsWith(".sfnt")) return "application/font-sfnt";
  else if (path.endsWith(".xml")) return "text/xml";
  else if (path.endsWith(".pdf")) return "application/pdf";
  else if (path.endsWith(".zip")) return "application/zip";
  else if (path.endsWith(".gz")) return "application/x-gzip";
  else if (path.endsWith(".appcache")) return "text/cache-manifest";
  return "application/octet-stream";
}

String urlDecode( const String& text )
{
  String decoded = "";
  char temp[] = "0x00";
  unsigned int len = text.length();
  unsigned int i = 0;
  while (i < len)
  {
    char decodedChar;
    char encodedChar = text.charAt(i++);
    if ( ( encodedChar == '%' ) && ( i + 1 < len ) )
    {
      temp[2] = text.charAt(i++);
      temp[3] = text.charAt(i++);

      decodedChar = strtol( temp, NULL, 16 );
    }
    else {
      if ( encodedChar == '+')
      {
        decodedChar = ' ';
      }
      else {
        decodedChar = encodedChar;  // normal ascii char
      }
    }
    decoded += decodedChar;
  }
  return decoded;
}

/* format bytes as KB, MB or GB string */
String humanReadableSize( size_t bytes )
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

bool setupMDNS( const String hostname )
{
  Serial.printf( "Check if %s is already present...\n", hostname.c_str() );
  uint32_t serviceIp = MDNS.queryHost( hostname.c_str() );
  if ( serviceIp != 0 )
  {
    Serial.printf( "%s already on network.\n", hostname.c_str() );
    return false;
  }
  Serial.printf( "%s is available.\nSetting new hostname...\n", hostname.c_str() );

  MDNS.end();

  if ( MDNS.begin( hostname.c_str() ) )
  {
    // Add service to MDNS-SD
    MDNS.addService( "http", "tcp", 80 );

    Serial.printf( "mDNS responder started\nmDNS name: %s.local.\n", hostname.c_str() );
    saveStringNVS( "hostname", hostname );
    Serial.printf( "mDNS hostname set to %s.\n", hostname.c_str() );
    return true;
  }
  else
  {
    Serial.println( "Error setting up MDNS responder!" );
    return false;
  }
}
