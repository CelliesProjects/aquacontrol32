//include all web interface header files
//https://stackoverflow.com/questions/8707183/script-tool-to-convert-file-to-c-c-source-code-array/8707241#8707241
#include "index_htm.h"
#include "editor_htm.h"
#include "setup_htm.h"
#include "fileman_htm.h"
#include "channels_htm.h"

const char* textPlainHeader  = "text/plain";
const char* textHtmlHeader   = "text/html";

void webServerTask ( void * pvParameters )
{

  // Set up the web server
  Serial.println( "Starting webserver setup. " );
  /*
    server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request){
      if(!request->authenticate(www_username, www_password))
          return request->requestAuthentication();
      request->redirect( "/api/upload");
    });

    server.on( "/api/upload", HTTP_POST,
    []( AsyncWebServerRequest *request )
    {
      request->send(200);
    },
    []( AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final )
    {
      if(!request->authenticate( www_username, www_password ) )
          return request->redirect( "/login" );

      static File newFile;
      static bool  _authenticated;
      static size_t receivedBytes;
      static time_t startTimer;

      if( !index )
      {
        startTimer = millis();
        Serial.printf( "Starting upload. filename = %s\n", filename.c_str() );
        if ( !filename.startsWith( "/" ) )
        {
          filename = "/" + filename;
        }
        newFile = SPIFFS.open( filename, "w");
        receivedBytes = 0;
      }

      receivedBytes += len;
      newFile.write( data, len );
      //Serial.printf( "received %i bytes\n",receivedBytes );

      if ( final )
      {
        newFile.close();
        //Serial.println( "Upload done." );
        Serial.printf( "Got %i bytes in %i ms.\n\n", receivedBytes, millis() - startTimer );
      }
    });
  */

  server.on( "/api/upload", HTTP_POST,
  []( AsyncWebServerRequest * request )
  {
    request->send( 404 );
   },
  []( AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final )
  {
    static File   newFile;
    static bool   _authenticated;
    static time_t startTimer;

    if ( !index )
    {
      _authenticated = false;
      if ( request->authenticate( www_username, www_password ) )
      {
        startTimer = millis();
        Serial.printf( "Starting upload. filename = %s\n", filename.c_str() );
        if ( !filename.startsWith( "/" ) )
        {
          filename = "/" + filename;
        }
        newFile = SPIFFS.open( filename, "w" );
        _authenticated = true;
      }
      else
      {
        Serial.println( "Unauthorized access." );
        return request->requestAuthentication();
      }
    }

    if ( _authenticated )
    {
      newFile.write( data, len );
    }

    if ( _authenticated && final )
    {
      newFile.close();
    }
  });

  // / or 'index.htm'
  server.on( "/", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    request->send_P( 200, textHtmlHeader, index_htm );
  });

  //  /channels or 'channels.htm'
  server.on( "/channels", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    request->send_P( 200, textHtmlHeader, channels_htm);
  });

  //  /editor or 'editor.htm'
  server.on( "/editor", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    request->send_P( 200, textHtmlHeader, editor_htm );
  });

  //  /setup or 'setup.htm'
  server.on( "/setup", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    request->send_P( 200, textHtmlHeader, setup_htm );
  });

  //  /filemanager or 'fileman.htm'
  server.on( "/filemanager", HTTP_GET, [] ( AsyncWebServerRequest * request )
  {
    request->send_P( 200, textHtmlHeader, fileman_htm );
  });

  /**********************************************************************************************
      api device get calls
  **********************************************************************************************/

  server.on( "/api/getdevice", []( AsyncWebServerRequest * request)
  {
    char content[100];

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
      snprintf( content, sizeof( content ), "%d" , SPIFFS.totalBytes() - SPIFFS.usedBytes() );
    }
    else if ( request->hasArg( "files" ) )
    {
      File root = SPIFFS.open( "/" );
      if ( !root )
      {
        request->send( 404, textPlainHeader, "Folder not found." );
        return;
      }
      if ( !root.isDirectory() )
      {
        request->send( 400, textPlainHeader, "Not a directory");
        return;
      }

      File file = root.openNextFile();
      uint8_t charCount = 0;
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
      snprintf( content, sizeof( content ), "%s", oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" );
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
      snprintf( content, sizeof( content ), "%s", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
    }
    else if ( request->hasArg( "status" ) )
    {
      int charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.2f\n", channel[channelNumber].currentPercentage );
      }
      time_t now = time(0);
      charCount += strftime( content + charCount, sizeof( content ) - charCount, "%T\n", localtime( &now ) );
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", lightStatus.c_str() );
      if ( numberOfFoundSensors )
      {
        for ( uint8_t sensorNumber = 0; sensorNumber < numberOfFoundSensors; sensorNumber++ )
        {
          charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s,%.1f\n", sensor[sensorNumber].name, sensor[sensorNumber].temp / 16 );
        }
      }
    }
    else if ( request->hasArg( "tftbrightness" ) )
    {
      snprintf( content, sizeof( content ), "%.2f", tftBrightness );
    }
    else if ( request->hasArg( "timezone" ) )
    {
      snprintf( content, sizeof( content ), "%s", getenv( "TZ" ) );
    }
    else
    {
      return request->send( 400, textPlainHeader, "Invalid option" );
    }
    request->send( 200, textPlainHeader, content );
  });


  /**********************************************************************************************
      api device set calls
  **********************************************************************************************/

  server.on( "/api/setdevice", []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( www_username, www_password ) )
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
      vTaskSuspend( x_dimmerTaskHandle );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        channel[channelNumber].currentPercentage = 0;
        ledcWrite( channelNumber, 0 );
      }
      lightStatus = "LIGHTS OFF ";
      snprintf( content, sizeof( content ), "%s", lightStatus.c_str() );
    }



    else if ( request->hasArg( "lightson" ) )
    {
      vTaskSuspend( x_dimmerTaskHandle );
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        channel[channelNumber].currentPercentage = 100;
        ledcWrite( channelNumber, ledcMaxValue );
      }
      lightStatus = " LIGHTS ON ";
      snprintf( content, sizeof( content ), "%s", lightStatus.c_str() );
    }



    else if ( request->hasArg( "lightsprogram" ) )
    {
      lightStatus = "LIGHTS AUTO";
      snprintf( content, sizeof( content ), "%s", lightStatus.c_str() );
      vTaskResume( x_dimmerTaskHandle );
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
      saveInt8NVS( "oledcontrast", oledContrast );
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


  /**********************************************************************************************
      api channel set calls
  **********************************************************************************************/

  server.on( "/api/setchannel", []( AsyncWebServerRequest * request )
  {
    if ( !request->authenticate( www_username, www_password ) )
    {
      return request->requestAuthentication();
    }

    int8_t channelNumber;
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

      saveStringNVS( "channelcolor" + channelNumber, channel[channelNumber].color );
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
      saveFloatNVS( "channelminimum" + channelNumber, channel[channelNumber].minimumLevel );
      snprintf( content, sizeof( content ), "channel %i minimum set to %.2f", channelNumber + 1, channel[ channelNumber ].minimumLevel );
    }



    else if ( request->hasArg( "name" ) )
    {
      for ( uint8_t currentChar = 0; currentChar < request->arg( "name" ).length(); currentChar++ )
      {
        if ( !isalnum( request->arg( "name" )[currentChar] ) )
        {
          return request->send( 400, textPlainHeader, "Invalid char" );
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
      saveStringNVS( "channelname" + char( channelNumber ), channel[channelNumber].name );
      snprintf( content, sizeof( content ), "channel %i name set to %s", channelNumber + 1, channel[ channelNumber ].name.c_str() );
    }



    else
    {
      return request->send( 400, textPlainHeader, "Invalid option" );
    }

    request->send( 200, textPlainHeader, content );
  });



  /**********************************************************************************************/
  /*   api channel get calls
    /**********************************************************************************************/
  /*
    server.on( "/api/getchannel", HTTP_GET, []( AsyncWebServerRequest * request)
    {
      int8_t channelNumber;
      char   content[30];

      channelNumber = checkChannelNumber( request );
      if ( channelNumber == -1 )
      {
        return request->send( 400, textPlainHeader, "Missing or invalid channel" );
      }
      if ( request->hasArg( "color" ) )
      {
        snprintf( content, sizeof( content ), "%s", channel[ channelNumber ].color.c_str() );
      }
      else if ( request->hasArg( "minimum" ) )
      {
        snprintf( content, sizeof( content ), "%.2f", channel[ channelNumber ].minimumLevel );
      }
      else if ( request->hasArg( "name" ) )
      {
        snprintf( content, sizeof( content ), "%s", channel[ channelNumber ].name.c_str() );
      }
      else
      {
        return request->send( 400, textPlainHeader, "Missing or invalid request" );
      }
      request->send( 200, textPlainHeader, content );
    });
  */

  /**********************************************************************************************/

  server.on( "/api/deletefile", HTTP_POST, []( AsyncWebServerRequest * request)
  {
    if ( !request->authenticate( www_username, www_password ) )
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

  server.serveStatic( "/", SPIFFS, "/" );

  server.onNotFound( []( AsyncWebServerRequest * request )
  {
    Serial.printf("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      Serial.printf("GET");
    else if (request->method() == HTTP_POST)
      Serial.printf("POST");
    else if (request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if (request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if (request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if (request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
  });

  //start the web server
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
  if (path.endsWith(".html")) return textHtmlHeader;
  else if (path.endsWith(".htm")) return textHtmlHeader;
  else if (path.endsWith(".css")) return "text/css";
  else if (path.endsWith(".txt")) return textPlainHeader;
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

