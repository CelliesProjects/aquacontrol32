//include all web interface header files
//https://stackoverflow.com/questions/8707183/script-tool-to-convert-file-to-c-c-source-code-array/8707241#8707241
#include "index_htm.h"
#include "editor_htm.h"
#include "setup_htm.h"
#include "fileman_htm.h"
#include "channels_htm.h"


void webServerTask ( void * pvParameters )
{
  while (1)
  {
    server.handleClient();
    vTaskDelay( 1 / portTICK_PERIOD_MS );
  }
}

const char* textPlainHeader  = "text/plain";
const char* textHtmlHeader   = "text/html";

const char* contentSecurityHeader      = "Content-Security-Policy";
const char* contentSecurityHeaderValue = "script-src 'unsafe-inline' https: https://code.jquery.com;";

void setupWebServer()                                            //https://github.com/espressif/esp-idf/blob/master/components/spi_flash/README.rst
{
  // Set up the web server
  tft.println( "Starting webserver. " );

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

  server.on( "/api/clearnvs", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    clearNVS();
    server.send( 200,  textPlainHeader, "NVS cleared" );
  });

  server.on( "/api/diskspace", []()
  {
    // https://stackoverflow.com/questions/8323159/how-to-convert-uint64-t-value-in-const-char-string
    // cardsize = uint64_t
    // length of 2**64 - 1, +1 for nul.
    char content[21];
    snprintf( content, sizeof( content ), "%" PRIu64, SD.cardSize() );
    server.send( 200,  textPlainHeader, content );
  });

  server.on( "/api/files", []()
  {
    String HTTPresponse;
    {
      File root = SD.open("/");
      if (!root)
      {
        server.send( 404, textPlainHeader, "Folder not found." );
        return;
      }
      if (!root.isDirectory())
      {
        server.send( 401, textPlainHeader, "Not a directory");
        return;
      }

      File file = root.openNextFile();
      while (file)
      {
        if (!file.isDirectory())
        {
          size_t fileSize = file.size();
          HTTPresponse += String( file.name() ) + "," + humanReadableSize( fileSize ) + "|";
        }
        file = root.openNextFile();
      }
    }
    server.send( 200, textPlainHeader, HTTPresponse );
  });

  server.on( "/api/boottime", []()
  {
    server.send( 200, textHtmlHeader, asctime( &systemStart ) );
  });

  server.on( "/api/getchannelcolors", []()
  {
    char content[42];
    uint8_t charCount = 0;
    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", channel[channelNumber].color.c_str() );
    }
    server.send( 200, textPlainHeader, content );
  });

  server.on( "/api/getchannelnames", []()
  {
    char content[100];
    uint8_t charCount = 0;
    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%s\n", channel[channelNumber].name.c_str() );
    }
    server.send( 200, textPlainHeader, content );
  });

  server.on( "/api/getminimumlevels", []()
  {
    char content[30];
    uint8_t charCount = 0;
    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%.2f\n", channel[channelNumber].minimumLevel );
    }
    server.send( 200, textPlainHeader, content );
  });

  server.on( "/api/hostname", HTTP_GET, []()
  {
    if ( server.hasArg( "hostname" ) )
    {
      server.arg( "hostname" ).trim();
      mDNSname = server.arg( "hostname" );
      if ( WiFi.setHostname( mDNSname.c_str() ) )
      {
        saveStringNVS( "hostname", mDNSname );
      }
      else
      {
        server.send( 200, textHtmlHeader, "ERROR setting hostname" );
        return;
      }
    }
    server.send( 200, textHtmlHeader, WiFi.getHostname() );
  });

  server.on( "/api/lightsoff", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    vTaskSuspend( x_dimmerTaskHandle );
    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      channel[channelNumber].currentPercentage = 0;
      ledcWrite( channelNumber, 0 );
    }
    lightStatus = "LIGHTS OFF ";
    server.send( 200, textHtmlHeader, lightStatus );
  });

  server.on( "/api/lightson", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    vTaskSuspend( x_dimmerTaskHandle );
    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      channel[channelNumber].currentPercentage = 100;
      ledcWrite( channelNumber, ledcMaxValue );
    }
    lightStatus = " LIGHTS ON ";
    server.send( 200, textHtmlHeader, lightStatus );
  });

  server.on( "/api/lightsprogram", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }
    vTaskResume( x_dimmerTaskHandle );
    lightStatus = "LIGHTS AUTO";
    server.send( 200, textHtmlHeader, lightStatus );
  });

  server.on( "/api/loadtimers", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    server.send( 200, textPlainHeader, defaultTimersLoaded() ? "Succes" : "Failed" );
  });

  server.on( "/api/pwmdepth", []()
  {
    if ( server.hasArg( "newpwmdepth" ) )
    {
      if ( !server.authenticate( www_username, www_password ) )
      {
        return server.requestAuthentication();
      }
      uint8_t newPWMDepth = server.arg( "newpwmdepth" ).toInt();
      if ( newPWMDepth < 11 || newPWMDepth > 16 )
      {
        server.send( 200, textPlainHeader, "ERROR - Invalid PWM depth" );
        return;
      }
      if ( ledcNumberOfBits != newPWMDepth )
      {
        for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
        {
          ledcSetup( channelNumber, ledcActualFrequency, newPWMDepth );
        }
      }
      ledcNumberOfBits = newPWMDepth;
    }

    char content[3];
    snprintf( content, sizeof( content ), "%i", ledcNumberOfBits );
    server.send( 200, textPlainHeader, content );
  });

  server.on( "/api/pwmfrequency", []()
  {
    if ( server.hasArg( "newpwmfrequency" ) )
    {
      if ( !server.authenticate( www_username, www_password ) )
      {
        return server.requestAuthentication();
      }
      double tempPWMfrequency = server.arg( "newpwmfrequency" ).toFloat();
      if ( tempPWMfrequency < 100 || tempPWMfrequency > 20000 )
      {
        server.send( 200, textPlainHeader, "Invalid PWM frequency" );
        return;
      }
      ledcActualFrequency = setupDimmerPWMfrequency( tempPWMfrequency, ledcNumberOfBits );
    }
    char content[16];
    snprintf( content, sizeof( content ), "%.0f", ledcActualFrequency );
    server.send( 200, textPlainHeader, content );
  });

  server.on( "/api/minimumlevel", []()
  {
    if ( server.hasArg( "channel" ) && server.hasArg( "percentage" ) )
    {
      if ( !server.authenticate( www_username, www_password ) )
      {
        return server.requestAuthentication();
      }

      int channelNumber = server.arg( "channel" ).toInt();
      if ( channelNumber < 0 || channelNumber >= NUMBER_OF_CHANNELS )
      {
        server.send( 400,  textPlainHeader, "Invalid channel." );
        return;
      }
      float thisPercentage = server.arg( "percentage" ).toFloat();
      if ( thisPercentage < 0 || thisPercentage > 1 )
      {
        server.send( 400,  textPlainHeader, "Invalid percentage." );
        return;
      }
      channel[channelNumber].minimumLevel = thisPercentage;
      saveFloatNVS( "channelminimum" + channelNumber, channel[channelNumber].minimumLevel );
      server.send( 200,  textPlainHeader, "Minimum level set." );
      return;
    }
    server.send( 400,  textPlainHeader, "Invalid input." );
  });

  server.on( "/api/ntpinterval", []()
  {
    server.send( 200,  textPlainHeader, String( SNTP_UPDATE_DELAY / 1000 ) );
  });

  server.on( "/api/setchannelcolor", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    int channelNumber;
    if ( server.hasArg( "channel" ) ) {
      channelNumber = server.arg( "channel" ).toInt();
      if ( channelNumber < 0 || channelNumber >= NUMBER_OF_CHANNELS ) {
        server.send( 400,  textPlainHeader, "Invalid channel." );
        return;
      }
    }
    if ( server.hasArg( "newcolor" ) ) {
      String newColor = "#" + server.arg( "newcolor" );
      newColor.trim();
      channel[channelNumber].color = newColor;
      saveStringNVS( "channelcolor" + char( channelNumber ), channel[channelNumber].color );
      server.send( 200, textPlainHeader , "Success" );
      return;
    }
    server.send( 400, textPlainHeader , "Invalid input." );
  });

  server.on( "/api/setchannelname", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    int channelNumber;
    if ( server.hasArg( "channel" ) ) {
      channelNumber = server.arg( "channel" ).toInt();
      if ( channelNumber < 0 || channelNumber >= NUMBER_OF_CHANNELS ) {
        server.send( 400, textPlainHeader, "Invalid channel." );
        return;
      }
    }
    if ( server.hasArg( "newname" ) ) {
      String newName = server.arg( "newname" );
      newName.trim();
      //TODO: check if illegal cahrs present and get out if so
      channel[channelNumber].name = newName;
      saveStringNVS( "channelname" + char( channelNumber ), channel[channelNumber].name );
      server.send( 200, textPlainHeader, "Success" );
      return;
    }
    server.send( 400, textPlainHeader, "Invalid input." );
  });

  server.on( "/api/setpercentage", []()
  {
    if ( !server.authenticate( www_username, www_password ) )
    {
      return server.requestAuthentication();
    }

    vTaskSuspend( x_dimmerTaskHandle );
    server.arg( "percentage" ).trim();
    float percentage = server.arg( "percentage" ).toFloat();
    for ( uint8_t channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      channel[channelNumber].currentPercentage = percentage;
      ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage, 0, 100, 0, ledcMaxValue ) );
    }
    lightStatus = "All lights at " + String( percentage ) + "%";
    server.send( 200, textPlainHeader, lightStatus );
  });

  server.on( "/api/status", []()
  {
    char content[50];
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
    server.send( 200, textPlainHeader, content );
  });

  server.on( "/api/tftorientation", HTTP_GET, []()
  {
    if ( server.hasArg( "tftorientation" ) )
    {
      if ( !server.authenticate( www_username, www_password ) )
      {
        return server.requestAuthentication();
      }

      if (  server.arg( "tftorientation" ) == "normal" )
      {
        TFTorientation = TFTnormal;
        //vTaskSuspend( x_tftTaskHandle );  //not needed since this task has a higher priority
        tft.setRotation( TFTorientation );
        tft.fillScreen( ILI9341_BLACK );
        //vTaskResume( x_tftTaskHandle );
      }
      else if ( server.arg( "tftorientation" ) == "upsidedown" )
      {
        TFTorientation = TFTupsidedown;
        //vTaskSuspend( x_tftTaskHandle );
        tft.setRotation( TFTorientation );
        tft.fillScreen( ILI9341_BLACK );
        //vTaskResume( x_tftTaskHandle );
      } else
      {
        server.send( 400, textPlainHeader, "ERROR No valid input." );
        return;
      }
    }
    saveStringNVS( "tftorientation", ( TFTorientation == TFTnormal ) ? "normal" : "upsidedown" );
    server.send( 200, textPlainHeader, ( TFTorientation == TFTnormal ) ? "normal" : "upsidedown" );
  });

  server.on( "/api/timezone", HTTP_GET, []()
  {
    if ( server.hasArg( "timezone" ) )
    {
      if ( !server.authenticate( www_username, www_password ) )
      {
        return server.requestAuthentication();
      }

      if ( 0 == setenv( "TZ",  server.arg( "timezone" ).c_str(), 1 )  )
      {
        saveStringNVS( "timezone", server.arg( "timezone" ) );
        server.send( 200, textPlainHeader, server.arg( "timezone" ) );
        return;
      }
      else
      {
        server.send( 400, textPlainHeader, "ERROR setting timezone" );
        return;
      }
    }
    else
    {
      char const* timeZone = getenv( "TZ" );
      if ( timeZone == NULL )
      {
        server.send( 200, textPlainHeader, "No timezone set." );
        return;
      }
      server.send( 200, textPlainHeader, timeZone );
      return;
    }
  });

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
      fsUploadFile = SD.open( filename, "w");
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

  server.onNotFound( handleNotFound );

  //start the web server
  server.begin();
  Serial.println("HTTP server setup done.");
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

  if ( SD.exists( path ) )
  {
    if ( server.arg( "action" ) == "delete" )
    {
      if ( !server.authenticate( www_username, www_password ) )
      {
        server.requestAuthentication();
        fileError = 401;
        return false;
      }

      SD.remove( path );
      server.send( 200,  textPlainHeader, path.substring(1) + " deleted" );
      return true;
    };
    File file = SD.open( path, "r" );
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

