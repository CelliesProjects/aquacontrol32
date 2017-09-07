//include all web interface htm files
//https://stackoverflow.com/questions/8707183/script-tool-to-convert-file-to-c-c-source-code-array/8707241#8707241
#include "index_htm.h"
#include "editor_htm.h"

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

  server.on( "/default.aqu", []()
  {
    if ( SD.exists( "/default.aqu" ) )
    {
      File file = SD.open( "/default.aqu", FILE_READ );
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

  server.on( "/api/hostname", []()
  {
    String response = WiFi.getHostname();
    size_t response_length = response.length();
    server.setContentLength( response_length );
    server.send( 200, texthtmlHEADER, response );
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
    float percentage = server.arg( "percentage" ).toFloat();
    programOverride = true;
    for ( byte thisChannel = 0; thisChannel < NUMBER_OF_CHANNELS; thisChannel++ )
    {
      channel[thisChannel].currentPercentage = percentage;
      ledcWrite( thisChannel, mapFloat( channel[thisChannel].currentPercentage, 0, 100, 0, LEDC_PWM_DEPTH ) );
    }
    String okString = "Ok";
    server.setContentLength( okString.length() );
    server.send( 200, textplainHEADER, okString );
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
    HTML +=  String( timeinfo.tm_hour) + ":" + String( timeinfo.tm_min ) + ":" + String( timeinfo.tm_sec ) + ",debug unit";
    server.setContentLength( HTML.length() );
    server.send( 200, textplainHEADER, HTML );
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


