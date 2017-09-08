//include all web interface htm files
//https://stackoverflow.com/questions/8707183/script-tool-to-convert-file-to-c-c-source-code-array/8707241#8707241
#include "index_htm.h"
#include "editor_htm.h"
#include "setup_htm.h"

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
    server.send_P( 200, texthtmlHEADER, index_html, index_html_len );
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

  server.on( defaultTimerFile, []()
  {
    if ( SD.exists( defaultTimerFile ) )
    {
      File file = SD.open( defaultTimerFile, FILE_READ );
      if ( file )
      {
        server.streamFile( file, "application/octet-stream" );
        file.close();
      }
    }
    else
    {
      server.send( 401 );
    }
  });

  /***************************************************************************
      API calls
   **************************************************************************/

  server.on( "/api/boottime", []()
  {
    String response = "0";
    size_t response_length = response.length();
    server.setContentLength( response_length );
    server.send( 200, texthtmlHEADER, response );
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
      //      analogWriteFreq( PWMfrequency );
      //      updateChannels();
      //writeConfigFile();
    }
    server.send( 200, FPSTR( textplainHEADER ), String( actualFreq ) );
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
      saveChannelColors();
      server.send( 200, textplainHEADER , "Success" );
      return;
    }
    server.send( 400, textplainHEADER , "Invalid input." );
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

  server.on( "/channelcolors.txt", []()
  {
    String response = "red\ngreen\nblue\n\white\nwhite-blue\n";
    server.setContentLength( response.length() );
    server.send( 200, textplainHEADER, response );
  });

  server.on( "/channelnames.txt", []()
  {
    String response = "rood\ngroen\nblauw\n\wit\nwit-blauw\n";
    server.setContentLength( response.length()  );
    server.send( 200, textplainHEADER, response );
  });

  //start the web server
  server.begin();
  Serial.println("TCP server started");
}


