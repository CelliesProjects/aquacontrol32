//include all web interface header files
//https://stackoverflow.com/questions/8707183/script-tool-to-convert-file-to-c-c-source-code-array/8707241#8707241
#include "index_htm.h"
#include "editor_htm.h"
#include "setup_htm.h"
#include "fileman_htm.h"
#include "channels_htm.h"

const char* textPlainHeader  = "text/plain";
const char* textHtmlHeader   = "text/html";

const char* contentSecurityHeader      = "Content-Security-Policy";
const char* contentSecurityHeaderValue = "script-src 'unsafe-inline' https: https://code.jquery.com;";

void webServerTask ( void * pvParameters )
{
  // Set up the web server
  Serial.println( "Starting webserver setup. " );

  //home page or 'index.html'
  server.on( "/", []()
  {
    server.sendHeader( contentSecurityHeader, contentSecurityHeaderValue );
    server.send_P( 200, textHtmlHeader, index_htm, index_htm_len );                // 'index_htm' & 'index_htm_len' are included with 'index_htm.h'
  });

  //channel setup or 'channels.htm'
  server.on( "/channels", []()
  {
    server.sendHeader( contentSecurityHeader, contentSecurityHeaderValue );
    server.send_P( 200, textHtmlHeader, channels_htm, channels_htm_len );          // 'channels_htm' & 'channels_htm_len' are included with 'channels_htm.h'
  });

  //editor or 'editor.htm'
  server.on( "/editor", []()
  {
    server.sendHeader( contentSecurityHeader, contentSecurityHeaderValue );
    server.send_P( 200, textHtmlHeader, editor_htm, editor_htm_len );              // 'editor_htm' & 'editor_htm_len' are included with 'editor_htm.h'
  });

  //editor or 'setup.htm'
  server.on( "/setup", []()
  {
    server.sendHeader( contentSecurityHeader, contentSecurityHeaderValue );
    server.send_P( 200, textHtmlHeader, setup_htm, setup_htm_len );                // 'setup_htm' & 'setup_htm_len' are included with 'setup_htm.h'
  });

  //filemanager or 'fileman.htm'
  server.on( "/filemanager", []()
  {
    server.sendHeader( contentSecurityHeader, contentSecurityHeaderValue );
    server.send_P( 200, textHtmlHeader, fileman_htm, fileman_htm_len );            // 'filemanager_htm' & 'filemanager_htm_len' are included with 'fileman_htm.h'
  });

  /***************************************************************************
      API calls
   **************************************************************************/

  server.on( "/api/upload", HTTP_OPTIONS, []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      server.requestAuthentication();
      return;
    }
    else
    {
      server.send( 200, textPlainHeader, "" );
    }
  });

  server.on( "/api/upload", HTTP_POST, []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      server.requestAuthentication();
      return;
    }
    else
    {
      server.send( 200 );
    }
  }, [] ()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      server.requestAuthentication();
      return;
    }

    static File fsUploadFile;
    HTTPUpload& upload = server.upload();

    String filename = upload.filename;
    if ( !filename.startsWith("/") )
    {
      filename = "/" + filename;
    }
    if ( filename.length() > 30 )
    {
      Serial.println( "Upload filename too long!" );
      return;
    }
    if ( upload.status == UPLOAD_FILE_START )
    {
      fsUploadFile = SPIFFS.open( filename, "w");
    }
    else if ( upload.status == UPLOAD_FILE_WRITE )
    {
      if ( fsUploadFile )
      {
        fsUploadFile.write( upload.buf, upload.currentSize );
        //showUploadProgressOLED( String( (float) fsUploadFile.position() / server.header( "Content-Length" ).toInt() * 100 ), upload.filename );
      }
    }
    else if ( upload.status == UPLOAD_FILE_END)
    {
      if ( fsUploadFile )
      {
        fsUploadFile.close();
      }
    }
  });


  /**********************************************************************************************/
  /*   api device get calls
    /**********************************************************************************************/

  server.on( "/api/getdevice", []()
  {
    char content[100];

    if ( server.hasArg( "boottime" ) )
    {
      snprintf( content, sizeof( content ), "%s", asctime( &systemStart ) );
    }
    else if ( server.hasArg( "channelcolors" ) )
    {
      uint8_t charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", channel[channelNumber].color.c_str() );
      }
    }
    else if ( server.hasArg( "channelnames" ) )
    {
      uint8_t charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", channel[channelNumber].name.c_str() );
      }
    }
    else if ( server.hasArg( "diskspace" ) )
    {
      snprintf( content, sizeof( content ), "%d" , SPIFFS.totalBytes() - SPIFFS.usedBytes() );
    }
    else if ( server.hasArg( "files" ) )
    {
      File root = SPIFFS.open( "/" );
      if ( !root )
      {
        server.send( 404, textPlainHeader, "Folder not found." );
        return;
      }
      if ( !root.isDirectory() )
      {
        server.send( 401, textPlainHeader, "Not a directory");
        return;
      }

      File file = root.openNextFile();
      byte charCount = 0;
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
    else if ( server.hasArg( "hostname" ) )
    {
      snprintf( content, sizeof( content ), "%s", mDNSname.c_str() );
    }
    else if ( server.hasArg( "minimumlevels" ) )
    {
      uint8_t charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.2f\n", channel[channelNumber].minimumLevel );
      }
    }
    else if ( server.hasArg( "oledcontrast" ) )
    {
      snprintf( content, sizeof( content ), "%i", oledContrast );
    }
    else if ( server.hasArg( "oledorientation" ) )
    {
      snprintf( content, sizeof( content ), "%s", oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" );
    }
    else if ( server.hasArg( "pwmdepth" ) )
    {
      snprintf( content, sizeof( content ), "%i", ledcNumberOfBits );
    }
    else if ( server.hasArg( "pwmfrequency" ) )
    {
      snprintf( content, sizeof( content ), "%.0f", ledcActualFrequency );
    }
    else if ( server.hasArg( "tftorientation" ) )
    {
      snprintf( content, sizeof( content ), "%s", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
    }
    else if ( server.hasArg( "status" ) )
    {
      int charCount = 0;
      for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.2f,", channel[channelNumber].currentPercentage );
      }
      time_t now = time(0);
      char buff[10];
      strftime( buff, sizeof( buff ), "%T", localtime( &now ) );
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s,", buff );
      snprintf( content + charCount, sizeof( content ) - charCount, "%s", lightStatus.c_str() );
    }
    else if ( server.hasArg( "tftbrightness" ) )
    {
      snprintf( content, sizeof( content ), "%.2f", tftBrightness );
    }
    else if ( server.hasArg( "timezone" ) )
    {
      snprintf( content, sizeof( content ), "%s", timeZone.c_str() );
    }
    else
    {
      return server.send( 400, textPlainHeader, "Invalid option" );
    }
    server.send( 200, textPlainHeader, content );
  });


  /**********************************************************************************************/
  /*   api device set calls
    /**********************************************************************************************/

  server.on( "/api/setdevice", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    char content[100];

    if ( server.hasArg( "clearnvs" ) )
    {
      clearNVS();
      snprintf( content, sizeof( content ), "NVS cleared" );
    }



    else if ( server.hasArg( "hostname" ) )
    {
      //set hostname NOT READY
      snprintf( content, sizeof( content ), "%s", mDNSname );

      //for now return an error
      return server.send( 400, textPlainHeader, "Not implemented yet." );
    }



    else if ( server.hasArg( "lightsoff" ) )
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



    else if ( server.hasArg( "lightson" ) )
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



    else if ( server.hasArg( "lightsprogram" ) )
    {
      lightStatus = "LIGHTS AUTO";
      snprintf( content, sizeof( content ), "%s", lightStatus.c_str() );
      vTaskResume( x_dimmerTaskHandle );
    }



    else if ( server.hasArg( "oledcontrast" ) )
    {
      server.arg( "oledcontrast" ).trim();
      int8_t contrast = server.arg( "oledcontrast" ).toInt();
      if ( contrast < 0 || contrast > 15 )
      {
        return server.send( 400, textPlainHeader, "Invalid contrast." );
      }
      oledContrast = contrast;
      OLED.setContrast( contrast << 4 );
      saveInt8NVS( "oledcontrast", oledContrast );
      snprintf( content, sizeof( content ), "%i", contrast );
    }



    else if ( server.hasArg( "oledorientation" ) )
    {
      if ( server.arg( "oledorientation" ) == "upsidedown" )
      {
        oledOrientation = OLED_ORIENTATION_UPSIDEDOWN;
      }
      else if ( server.arg( "oledorientation" ) == "normal" )
      {
        oledOrientation = OLED_ORIENTATION_NORMAL;
      }
      else
      {
        return server.send( 400, textPlainHeader, "Invalid orientation" );
      }
      OLED.end();
      OLED.init();
      OLED.setContrast( oledContrast << 0x04 );
      oledOrientation == OLED_ORIENTATION_NORMAL ? OLED.normalDisplay() : OLED.flipScreenVertically();
      saveStringNVS( "oledorientation", ( oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" ) );
      snprintf( content, sizeof( content ), "%s", oledOrientation == OLED_ORIENTATION_NORMAL ? "normal" : "upsidedown" );
    }



    else if ( server.hasArg( "pwmdepth" ) )
    {
      uint8_t newPWMDepth = server.arg( "pwmdepth" ).toInt();
      if ( newPWMDepth < 11 || newPWMDepth > 16 )
      {
        return server.send( 400, textPlainHeader, "Invalid PWM depth" );
      }
      if ( ledcNumberOfBits != newPWMDepth )
      {
        setupDimmerPWMfrequency( LEDC_REQUEST_FREQ, newPWMDepth );
        saveInt8NVS( "pwmdepth" , ledcNumberOfBits );
      }
      snprintf( content, sizeof( content ), "%i", ledcNumberOfBits );
    }



    else if ( server.hasArg( "pwmfrequency" ) )
    {
      double tempPWMfrequency = server.arg( "pwmfrequency" ).toFloat();
      if ( tempPWMfrequency < 100 || tempPWMfrequency > 20000 )
      {
        server.send( 200, textPlainHeader, "Invalid PWM frequency" );
        return;
      }
      if ( tempPWMfrequency != ledcActualFrequency )
      {
        setupDimmerPWMfrequency( tempPWMfrequency, ledcNumberOfBits );
        saveDoubleNVS( "pwmfrequency", ledcActualFrequency );
      }
      snprintf( content, sizeof( content ), "%.0f", ledcActualFrequency );
    }



    else if ( server.hasArg( "tftorientation" ) )
    {
      if (  server.arg( "tftorientation" ) == "normal" )
      {
        tftOrientation = TFT_ORIENTATION_NORMAL;
      }
      else if ( server.arg( "tftorientation" ) == "upsidedown" )
      {
        tftOrientation = TFT_ORIENTATION_UPSIDEDOWN;
      }
      else
      {
        return server.send( 400, textPlainHeader, "Invalid tft orientation." );
      }
      tft.setRotation( tftOrientation );
      tft.fillScreen( ILI9341_BLACK );
      saveStringNVS( "tftorientation", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
      snprintf( content, sizeof( content ), "%s", ( tftOrientation == TFT_ORIENTATION_NORMAL ) ? "normal" : "upsidedown" );
    }



    else if ( server.hasArg( "tftbrightness" ) )
    {
      float brightness = server.arg( "tftbrightness" ).toFloat();
      if ( brightness < 0 || brightness > 100 )
      {
        return server.send( 400, textPlainHeader, "Invalid tft brightness." );
      }
      tftBrightness = brightness;
      saveInt8NVS( "tftbrightness", brightness );
      snprintf( content, sizeof( content ), "%.2f", tftBrightness );
    }



    else if ( server.hasArg( "timezone" ) )
    {
      if ( 0 == setenv( "TZ",  server.arg( "timezone" ).c_str(), 1 )  )
      {
        timeZone = server.arg( "timezone" );
        saveStringNVS( "timezone", server.arg( "timezone" ) );
      }
      else
      {
        return server.send( 400, textPlainHeader, "Error setting timezone." );
      }
      snprintf( content, sizeof( content ), "%s", timeZone.c_str() );
    }



    else
    {
      return server.send( 400, textPlainHeader, "Invalid option" );
    }

    server.send( 200, textPlainHeader, content );
  });


  /**********************************************************************************************/
  /*   api channel set calls
    /**********************************************************************************************/

  server.on( "/api/setchannel", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    char content[100];
    int8_t channelNumber;

    channelNumber = checkChannelNumber();
    if ( channelNumber == -1 )
    {
      return server.send( 400, textPlainHeader, "Invalid channel" );
    }



    if ( server.hasArg( "color" ) )
    {
      for ( byte currentChar = 0; currentChar < server.arg( "color" ).length(); currentChar++ )
      {
        if ( !isxdigit( server.arg( "color" )[currentChar] ) )
        {
          return server.send( 400, textPlainHeader, "Invalid char" );
        }
      }
      channel[ channelNumber ].color = "#" + server.arg( "color" );
      snprintf( content, sizeof( content ), "channel %i color set to %s", channelNumber + 1, channel[ channelNumber ].color.c_str() );
    }



    else if ( server.hasArg( "minimum" ) )
    {
      float minLevel = server.arg( "minimum" ).toFloat();
      if ( minLevel < 0 || minLevel > 0.991 )
      {
        return server.send( 400, textPlainHeader, "Invalid level" );
      }
      channel[ channelNumber ].minimumLevel = minLevel;
      saveFloatNVS( "channelminimum" + channelNumber, channel[channelNumber].minimumLevel );
      snprintf( content, sizeof( content ), "channel %i minimum set to %.2f", channelNumber + 1, channel[ channelNumber ].minimumLevel );
    }



    else if ( server.hasArg( "name" ) )
    {
      for ( byte currentChar = 0; currentChar < server.arg( "name" ).length(); currentChar++ )
      {
        if ( !isalnum( server.arg( "name" )[currentChar] ) )
        {
          return server.send( 400, textPlainHeader, "Invalid char" );
        }
      }
      if ( server.arg( "name" ) != "" )
      {
        channel[ channelNumber ].name = server.arg( "name" );
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
      return server.send( 400, textPlainHeader, "Invalid option" );
    }

    server.send( 200, textPlainHeader, content );
  });



  /**********************************************************************************************/
  /*   api channel get calls
    /**********************************************************************************************/

  server.on( "/api/getchannel", []()
  {
    char content[30];
    int8_t channelNumber;

    channelNumber = checkChannelNumber();
    if ( channelNumber == -1 )
    {
      return server.send( 400, textPlainHeader, "Missing or invalid channel" );
    }
    if ( server.hasArg( "color" ) )
    {
      snprintf( content, sizeof( content ), "%s", channel[ channelNumber ].color.c_str() );
    }
    else if ( server.hasArg( "minimum" ) )
    {
      snprintf( content, sizeof( content ), "%.2f", channel[ channelNumber ].minimumLevel );
    }
    else if ( server.hasArg( "name" ) )
    {
      snprintf( content, sizeof( content ), "%s", channel[ channelNumber ].name.c_str() );
    }
    else
    {
      return server.send( 400, textPlainHeader, "Missing or invalid request" );
    }
    server.send( 200, textPlainHeader, content );
  });


  /**********************************************************************************************/


  server.onNotFound( handleNotFound );

  //start the web server
  server.begin();
  Serial.println("HTTP server setup done.");


  /**********************************************************************/
  /*    webserver main loop
    /***********************************************************************/

  while (1)
  {
    server.handleClient();
    vTaskDelay( 1 / portTICK_PERIOD_MS );
  }
}

int8_t checkChannelNumber()
{
  if ( !server.hasArg( "channel" ) )
  {
    return -1;
  }
  else
  {
    int8_t channelNumber = server.arg( "channel" ).toInt();
    if ( channelNumber < 1 || channelNumber > NUMBER_OF_CHANNELS )
    {
      return -1;
    }

    return channelNumber - 1;
  }
}

void handleNotFound()
{
  /////////////////////////////////////////////////////////////////////////////////////
  // if the request is not handled by any of the defined handlers
  // try to use the argument as filename and serve from SD
  // if no matching file is found, throw an error.
  int error;
  if ( !handleSDfile( server.uri(), error ) )
  {
    if ( error == 401 )
    {
      server.send( 401, textPlainHeader, "Unauthorized" );
      return;
    }
    server.send( 404, textPlainHeader, "404 - File not found." );
  }
}

bool handleSDfile( String path , int fileError )
{
  fileError = 200;
  path = server.urlDecode( path );
  if ( path.endsWith( "/" ) )
  {
    path += "index.htm";
  }

  if ( SPIFFS.exists( path ) )
  {
    if ( server.arg( "action" ) == "delete" )
    {
      if ( !server.authenticate( www_username, www_password ) )
      {
        server.requestAuthentication();
        fileError = 401;
        return false;
      }

      SPIFFS.remove( path );
      server.send( 200,  textPlainHeader, path.substring(1) + " deleted" );
      return true;
    };
    File file = SPIFFS.open( path, "r" );
    size_t sent = server.streamFile( file, getContentType( path ) );
    file.close();
    return true;
  }
  return false;
}

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

