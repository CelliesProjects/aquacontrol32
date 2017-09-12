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
    vTaskDelay( 5 / portTICK_PERIOD_MS );
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
    server.send_P( 200, texthtmlHEADER, index_htm, index_htm_len );                // 'index_htm' & 'index_htm_len' are included with 'index_htm.h'
  });

  //channel setup or 'channels.htm'
  server.on( "/channels", []()
  {
    server.send_P( 200, texthtmlHEADER, channels_htm, channels_htm_len );          // 'channels_htm' & 'channels_htm_len' are included with 'channels_htm.h'
  });

  //editor or 'editor.htm'
  server.on( "/editor", []()
  {
    server.send_P( 200, texthtmlHEADER, editor_htm, editor_htm_len );              // 'editor_htm' & 'editor_htm_len' are included with 'editor_htm.h'
  });

  //editor or 'setup.htm'
  server.on( "/setup", []()
  {
    server.send_P( 200, texthtmlHEADER, setup_htm, setup_htm_len );                // 'setup_htm' & 'setup_htm_len' are included with 'setup_htm.h'
  });

  //filemanager or 'fileman.htm'
  server.on( "/filemanager", []()
  {
    server.send_P( 200, texthtmlHEADER, fileman_htm, fileman_htm_len );            // 'filemanager_htm' & 'filemanager_htm_len' are included with 'fileman_htm.h'
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
    server.send( 200,  textplainHEADER, buff );
  });

  server.on( "/api/files", []() {
    String HTTPresponse;
    {
      File root = SD.open("/");
      if (!root)
      {
        Serial.println("Failed to open directory");
        server.send( 404, textplainHEADER, "Folder not found." );
        return;
      }
      if (!root.isDirectory())
      {
        Serial.println("Not a directory");
        server.send( 401, textplainHEADER, "Not a directory");
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
    server.send( 200, textplainHEADER, HTTPresponse );
  });

  server.on( "/api/boottime", []()
  {
    String response = "0";
    size_t response_length = response.length();
    server.setContentLength( response_length );
    server.send( 200, texthtmlHEADER, response );
  });

  server.on( "/api/getchannelcolors", []()
  {
    String response;
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      response += channel[channelNumber].color + "\n";
    }
    server.setContentLength( response.length() );
    server.send( 200, textplainHEADER, response );
  });

  server.on( "/api/getchannelnames", []()
  {
    String response;
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      response += channel[channelNumber].name + "\n";
    }
    server.setContentLength( response.length()  );
    server.send( 200, textplainHEADER, response );
  });

  server.on( "/api/getminimumlevels", []()
  {
    String html;
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      html += String( channel[channelNumber].minimumLevel ) + "\n";
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

  server.on( "/api/lightsoff", []()
  {
    vTaskSuspend( x_dimmerTaskHandle );
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      channel[channelNumber].currentPercentage = 0;
      ledcWrite( channelNumber, 0 );
    }
    lightStatus = "Lights are off";
    server.send( 200, texthtmlHEADER, lightStatus );
  });

  server.on( "/api/lightson", []()
  {
    vTaskSuspend( x_dimmerTaskHandle );
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      channel[channelNumber].currentPercentage = 100;
      ledcWrite( channelNumber, LEDC_PWM_DEPTH_NOMATH );
    }
    lightStatus = "Lights are on";
    server.send( 200, texthtmlHEADER, lightStatus );
  });

  server.on( "/api/lightsprogram", []()
  {
    vTaskResume( x_dimmerTaskHandle );
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time_t secondsToday = ( timeinfo.tm_hour * 3600 ) + ( timeinfo.tm_min * 60 ) + timeinfo.tm_sec;
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      setPercentageFromProgram( channelNumber, secondsToday );
    }
    lightStatus = "Program running.";
    server.send( 200, texthtmlHEADER, lightStatus );
  });

  server.on( "/api/loadtimers", []()
  {
    server.send( 200, textplainHEADER, loadDefaultTimers() ? "Succes" : "Failed" );
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time_t secondsToday = ( timeinfo.tm_hour * 3600 ) + ( timeinfo.tm_min * 60 ) + timeinfo.tm_sec;
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      setPercentageFromProgram( channelNumber, secondsToday );
    }
  });

  server.on( "/api/pwmdepth", []() {
    byte newPWMDepth;
    if ( server.arg( "newpwmdepth" ) != "" ) {
      newPWMDepth = server.arg( "newpwmdepth" ).toInt();
      if ( newPWMDepth < 10 || newPWMDepth > 16 ) {
        server.send( 200, textplainHEADER, "ERROR - Invalid PWM depth" );
        return;
      }
      if ( ledcActualBitDepth != newPWMDepth )
      {
        for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
        {
          ledcSetup( channelNumber, ledcActualFrequency, newPWMDepth );
        }
      }
      ledcActualBitDepth = newPWMDepth;
      //TODO: Save in preferences
    }
    server.send( 200, textplainHEADER, String( ledcActualBitDepth ) );
  });


  server.on( "/api/pwmfrequency", []() {
    double actualFreq = ledcActualFrequency;
    if ( server.arg( "newpwmfrequency" ) != "" ) {
      double tempPWMfrequency = server.arg( "newpwmfrequency" ).toFloat();
      if ( tempPWMfrequency < 100 || tempPWMfrequency > 10000 ) {
        server.send( 200, textplainHEADER, "Invalid PWM frequency" );
        return;
      }
      for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
      {
        actualFreq = ledcSetup( channelNumber, tempPWMfrequency, LEDC_NUMBER_OF_BIT );
      }
      //TODO: Save in preferences
    }
    server.send( 200, textplainHEADER, String( actualFreq ) );
  });

  server.on( "/api/minimumlevel", []()
  {
    if ( server.arg( "channel" ) != "" && server.arg( "percentage" != "" ) )
    {
      int channelNumber = server.arg( "channel" ).toInt();
      if ( channelNumber < 0 || channelNumber >= NUMBER_OF_CHANNELS )
      {
        server.send( 400,  textplainHEADER, "Invalid channel." );
        return;
      }
      float thisPercentage = server.arg( "percentage" ).toFloat();
      if ( thisPercentage < 0 || thisPercentage > 0.99 )
      {
        server.send( 400,  textplainHEADER, "Invalid percentage." );
        return;
      }
      channel[channelNumber].minimumLevel = thisPercentage;
      //writeMinimumLevelFile();
      server.send( 200,  textplainHEADER, "Minimum level set." );
      return;
    }
    server.send( 400,  textplainHEADER, "Invalid input." );
  });

  server.on( "/api/setchannelcolor", []() {
    int channelNumber;
    if ( server.hasArg( "channel" ) ) {
      channelNumber = server.arg( "channel" ).toInt();
      Serial.println(channelNumber);
      if ( channelNumber < 0 || channelNumber >= NUMBER_OF_CHANNELS ) {
        server.send( 400,  textplainHEADER, "Invalid channel." );
        return;
      }
    }
    if ( server.hasArg( "newcolor" ) ) {
      String newColor = "#" + server.arg( "newcolor" );
      newColor.trim();
      channel[channelNumber].color = newColor;
      saveChannelColor( channelNumber );
      server.send( 200, textplainHEADER , "Success" );
      return;
    }
    server.send( 400, textplainHEADER , "Invalid input." );
  });

  server.on( "/api/setchannelname", []() {
    int channelNumber;
    if ( server.hasArg( "channel" ) ) {
      channelNumber = server.arg( "channel" ).toInt();
      if ( channelNumber < 0 || channelNumber >= NUMBER_OF_CHANNELS ) {
        server.send( 400, textplainHEADER, "Invalid channel." );
        return;
      }
    }
    if ( server.hasArg( "newname" ) ) {
      String newName = server.arg( "newname" );
      newName.trim();
      //TODO: check if illegal cahrs present and get out if so
      channel[channelNumber].name = newName;
      saveChannelName( channelNumber );
      server.send( 200, textplainHEADER, "Success" );
      return;
    }
    server.send( 400, textplainHEADER, "Invalid input." );
  });

  server.on( "/api/setpercentage", []()
  {
    vTaskSuspend( x_dimmerTaskHandle );
    server.arg( "percentage" ).trim();
    float percentage = server.arg( "percentage" ).toFloat();
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      channel[channelNumber].currentPercentage = percentage;
      ledcWrite( channelNumber, mapFloat( channel[channelNumber].currentPercentage, 0, 100, 0, LEDC_PWM_DEPTH_NOMATH ) );
    }
    lightStatus = "All lights at " + String( percentage ) + "%";
    server.setContentLength( lightStatus.length() );
    server.send( 200, textplainHEADER, lightStatus );
  });

  server.on( "/api/status", []()
  {
    String HTML;
    for ( byte channelNumber = 0; channelNumber < NUMBER_OF_CHANNELS; channelNumber++ )
    {
      HTML += String( channel[channelNumber].currentPercentage ) + ",";
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
    server.send( 200, textplainHEADER, "" );
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
    Serial.println( "404 File not found." );
    server.send( 404, textplainHEADER, "404 - File not found." );
  }
}

bool handleSDfile( String path ) {
  path = server.urlDecode( path );
  if ( path.endsWith( "/" ) ) path += "index.htm";

  if ( SD.exists( path ) ) {
    if ( server.arg( "action" ) == "delete" ) {
      Serial.println( "Delete request. Deleting..." );
      SD.remove( path );
      Serial.println( path + " deleted" );
      server.send( 200,  textplainHEADER, path.substring(1) + " deleted" );
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

