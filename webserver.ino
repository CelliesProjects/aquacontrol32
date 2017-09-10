//include all web interface htm files
//https://stackoverflow.com/questions/8707183/script-tool-to-convert-file-to-c-c-source-code-array/8707241#8707241
#include "index_htm.h"
#include "editor_htm.h"
#include "setup_htm.h"
#include "filemanager_htm.h"
#include "channels_htm.h"

void webServerTask ( void * pvParameters )
{
  while (1)
  {
    server.handleClient();
    vTaskDelay( 1 / portTICK_PERIOD_MS);
  }
}

static const char textplainHEADER[]  = "text/plain";
static const char texthtmlHEADER[]  = "text/html";

void setupWebServer()
{
  // Set up the web server

  //home page or 'index.html'
  server.on( "/", []()
  {
    server.send_P( 200, texthtmlHEADER, index_htm, index_htm_len );
  });

  //channel setup or 'channels.htm'
  server.on( "/channels", []()
  {
    server.send_P( 200, texthtmlHEADER, channels_htm, channels_htm_len );
  });

  //editor or 'editor.htm'
  server.on( "/editor", []()
  {
    server.send_P( 200, texthtmlHEADER, editor_htm, editor_htm_len );
  });

  //editor or 'setup.htm'
  server.on( "/setup", []()
  {
    server.send_P( 200, texthtmlHEADER, setup_htm, setup_htm_len );
  });

  //filemanager or 'fileman.htm'
  server.on( "/filemanager", []()
  {
    server.send_P( 200, texthtmlHEADER, filemanager_htm, filemanager_htm_len );
  });

  /***************************************************************************
      API calls
   **************************************************************************/

  server.on( "/api/diskspace", []() {
    // https://stackoverflow.com/questions/8323159/how-to-convert-uint64-t-value-in-const-char-string
    // cardsize = uint64_t
    // length of 2**64 - 1, +1 for nul.
    char buff[21];
    // copy to buffer
    sprintf( buff, "%" PRIu64, SD.cardSize() );
    server.send( 200, FPSTR( textplainHEADER ), buff );
  });

  server.on( "/api/files", []() {
    String HTTPresponse;
    {
      File root = SD.open("/");
      if (!root)
      {
        Serial.println("Failed to open directory");
        server.send( 404, FPSTR( textplainHEADER ), "Folder not found." );
        return;
      }
      if (!root.isDirectory())
      {
        Serial.println("Not a directory");
        server.send( 401, FPSTR( textplainHEADER ), "Not a directory");
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
    server.send( 200, FPSTR( textplainHEADER ), HTTPresponse );
  });

  server.on( "/api/boottime", []()
  {
    String response = "0";
    size_t response_length = response.length();
    server.setContentLength( response_length );
    server.send( 200, texthtmlHEADER, response );
  });

  server.on( "/api/getminimumlevels", []()
  {
    String html;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      html += String( channel[thisChannel].minimumLevel ) + "\n";
    }
    server.setContentLength( html.length() );
    server.send( 200, texthtmlHEADER, html );
  });

  server.on( "/api/hostname", []()
  {
    String response = WiFi.getHostname();
    size_t response_length = response.length();
    server.setContentLength( response_length );
    server.send( 200, texthtmlHEADER, response );
  });


  server.on( "/api/loadtimers", []() {
    server.send( 200, FPSTR( textplainHEADER ), loadDefaultTimers() ? F( "Succes" ) : F( "Failed" ) );
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time_t secondsToday = ( timeinfo.tm_hour * 3600 ) + ( timeinfo.tm_min * 60 ) + timeinfo.tm_sec;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      setPercentageFromProgram( thisChannel, secondsToday );
    }
  });

  server.on( "/api/pwmdepth", []() {
    byte newPWMDepth;
    if ( server.arg( "newpwmdepth" ) != "" ) {
      newPWMDepth = server.arg( "newpwmdepth" ).toInt();
      if ( newPWMDepth < 10 || newPWMDepth > 16 ) {
        server.send( 200, FPSTR( textplainHEADER ), F( "ERROR - Invalid PWM depth" ) );
        return;
      }
      if ( ledcActualBitDepth != newPWMDepth )
      {
        for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
        {
          ledcSetup( thisChannel, ledcActualFrequency, newPWMDepth );
        }
      }
      ledcActualBitDepth = newPWMDepth;
      //TODO: Save in preferences
    }
    server.send( 200, FPSTR( textplainHEADER ), String( ledcActualBitDepth ) );
  });


  server.on( "/api/pwmfrequency", []() {
    double actualFreq = ledcActualFrequency;
    if ( server.arg( "newpwmfrequency" ) != "" ) {
      double tempPWMfrequency = server.arg( "newpwmfrequency" ).toFloat();
      if ( tempPWMfrequency < 100 || tempPWMfrequency > 10000 ) {
        server.send( 200, FPSTR( textplainHEADER ), F( "Invalid PWM frequency" ) );
        return;
      }
      for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
      {
        actualFreq = ledcSetup( thisChannel, tempPWMfrequency, LEDC_NUMBER_OF_BIT );
      }
      //TODO: Save in preferences
    }
    server.send( 200, FPSTR( textplainHEADER ), String( actualFreq ) );
  });

  server.on( "/api/minimumlevel", []()
  {
    if ( server.arg( "channel" ) != "" && server.arg( "percentage" != "" ) )
    {
      int thisChannel = server.arg( "channel" ).toInt();
      if ( thisChannel < 0 || thisChannel > NUMBER_OF_CHANNELS )
      {
        server.send( 400, FPSTR( textplainHEADER ), F( "Invalid channel." ) );
        return;
      }
      float thisPercentage = server.arg( "percentage" ).toFloat();
      if ( thisPercentage < 0 || thisPercentage > 0.99 )
      {
        server.send( 400, FPSTR( textplainHEADER ), F( "Invalid percentage." ) );
        return;
      }
      channel[thisChannel].minimumLevel = thisPercentage;
      //writeMinimumLevelFile();
      server.send( 200, FPSTR( textplainHEADER ), F( "Minimum level set." ) );
      return;
    }
    server.send( 400, FPSTR( textplainHEADER ), F( "Invalid input." ) );
  });

  server.on( "/api/setchannelcolor", []() {
    int thisChannel;
    if ( server.hasArg( "channel" ) ) {
      thisChannel = server.arg( "channel" ).toInt();
      Serial.println(thisChannel);
      if ( thisChannel < 0 || thisChannel > NUMBER_OF_CHANNELS ) {
        server.send( 400,  textplainHEADER, "Invalid channel." );
        return;
      }
    }
    if ( server.hasArg( "newcolor" ) ) {
      String newColor = "#" + server.arg( "newcolor" );
      newColor.trim();
      channel[thisChannel].color = newColor;
      saveChannelColor();
      server.send( 200, textplainHEADER , "Success" );
      return;
    }
    server.send( 400, textplainHEADER , "Invalid input." );
  });

  server.on( "/api/setchannelname", []() {
    int thisChannel;
    if ( server.hasArg( "channel" ) ) {
      thisChannel = server.arg( "channel" ).toInt();
      if ( thisChannel < 0 || thisChannel > NUMBER_OF_CHANNELS ) {
        server.send( 400, FPSTR( textplainHEADER ), F( "Invalid channel." ) );
        return;
      }
    }
    if ( server.hasArg( "newname" ) ) {
      String newName = server.arg( "newname" );
      newName.trim();
      //TODO: check if illegal cahrs present and get out if so
      channel[thisChannel].name = newName;
      saveChannelName( thisChannel );
      server.send( 200, FPSTR( textplainHEADER ), F( "Success" ) );
      return;
    }
    server.send( 400, FPSTR( textplainHEADER ), F( "Invalid input." ) );
  });

  server.on( "/api/setpercentage", []()
  {
    server.arg( "percentage" ).trim();
    float percentage = server.arg( "percentage" ).toFloat();
    programOverride = true;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      channel[thisChannel].currentPercentage = percentage;
      ledcWrite( thisChannel, mapFloat( channel[thisChannel].currentPercentage, 0, 100, 0, LEDC_PWM_DEPTH_NOMATH ) );
    }
    lightStatus = "All lights at " + String( percentage ) + "%";
    server.setContentLength( lightStatus.length() );
    server.send( 200, textplainHEADER, lightStatus );
  });

  server.on( "/api/status", []()
  {
    String HTML;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      HTML += String( channel[thisChannel].currentPercentage ) + ",";
    }
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    HTML +=  String( timeinfo.tm_hour) + ":" + String( timeinfo.tm_min ) + ":" + String( timeinfo.tm_sec ) + "," + lightStatus;
    server.setContentLength( HTML.length() );
    server.send( 200, textplainHEADER, HTML );
  });

  server.on( "/api/upload", HTTP_POST, []() {
    server.send( 200, FPSTR( textplainHEADER ), "" );
  }, []() {
    static File fsUploadFile;
    HTTPUpload& upload = server.upload();
    String filename = upload.filename;
    if ( !filename.startsWith("/") ) filename = "/" + filename;
    if ( filename.length() > 30 ) {
      Serial.println( "Upload filename too long!" );
      return;
    }
    if ( upload.status == UPLOAD_FILE_START ) {
      fsUploadFile = SD.open( filename, "w");
    } else if ( upload.status == UPLOAD_FILE_WRITE ) {
      if ( fsUploadFile ) {
        fsUploadFile.write( upload.buf, upload.currentSize );
        //showUploadProgressOLED( String( (float) fsUploadFile.position() / server.header( "Content-Length" ).toInt() * 100 ), upload.filename );
      }
    } else if ( upload.status == UPLOAD_FILE_END) {
      if ( fsUploadFile ) {
        fsUploadFile.close();
      }
    }
  });

  server.on( "/api/getchannelcolors", []()
  {
    String response;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      response += channel[thisChannel].color + "\n";
    }
    server.setContentLength( response.length() );
    server.send( 200, textplainHEADER, response );
  });

  server.on( "/api/getchannelnames", []()
  {
    String response;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      response += channel[thisChannel].name + "\n";
    }
    server.setContentLength( response.length()  );
    server.send( 200, textplainHEADER, response );
  });

  server.onNotFound( handleNotFound );

  //start the web server
  server.begin();
  Serial.println("TCP server started");
}

void handleNotFound() {
  /////////////////////////////////////////////////////////////////////////////////////
  // if the request is not handled by any of the defined handlers
  // try to use the argument as filename and serve from SD
  // if no matching file is found, throw an error.
  if ( !handleSDfile( server.uri() ) ) {
    Serial.println( F( "404 File not found." ) );
    server.send( 404, FPSTR( textplainHEADER ), F( "404 - File not found." ) );
  }
}

bool handleSDfile( String path ) {
  path = server.urlDecode( path );
  if ( path.endsWith( "/" ) ) path += F( "index.htm" );

  if ( SD.exists( path ) ) {
    if ( server.arg( "action" ) == "delete" ) {
      Serial.println( F( "Delete request. Deleting..." ) );
      //showDeleteOLED( path.substring(1) );
      SD.remove( path );
      Serial.println( path + F( " deleted" ) );
      server.send( 200, FPSTR( textplainHEADER ), path.substring(1) + F( " deleted" ) );
      return true;
    };
    File file = SD.open( path, "r" );
    size_t sent = server.streamFile( file, getContentType( path ) );
    file.close();
    return true;
  }
  return false;
}

String getContentType( const String& path) {
  if (path.endsWith(".html")) return "text/html";
  else if (path.endsWith(".htm")) return F( "text/html" );
  else if (path.endsWith(".css")) return F( "text/css" );
  else if (path.endsWith(".txt")) return F( "text/plain" );
  else if (path.endsWith(".js")) return F( "application/javascript" );
  else if (path.endsWith(".png")) return F( "image/png" );
  else if (path.endsWith(".gif")) return F( "image/gif" );
  else if (path.endsWith(".jpg")) return F( "image/jpeg" );
  else if (path.endsWith(".ico")) return F( "image/x-icon" );
  else if (path.endsWith(".svg")) return F( "image/svg+xml" );
  else if (path.endsWith(".ttf")) return F( "application/x-font-ttf" );
  else if (path.endsWith(".otf")) return F( "application/x-font-opentype" );
  else if (path.endsWith(".woff")) return F( "application/font-woff" );
  else if (path.endsWith(".woff2")) return F( "application/font-woff2" );
  else if (path.endsWith(".eot")) return F( "application/vnd.ms-fontobject" );
  else if (path.endsWith(".sfnt")) return F( "application/font-sfnt" );
  else if (path.endsWith(".xml")) return F( "text/xml" );
  else if (path.endsWith(".pdf")) return F( "application/pdf" );
  else if (path.endsWith(".zip")) return F( "application/zip" );
  else if (path.endsWith(".gz")) return F( "application/x-gzip" );
  else if (path.endsWith(".appcache")) return F( "text/cache-manifest" );
  return F( "application/octet-stream" );
}

