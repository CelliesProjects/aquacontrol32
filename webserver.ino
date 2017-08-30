 const String PROGMEM indexHTML  = R"#####(<p>You just loaded the root of your ESP WebServer.</p>
<a href="wasietsmet.nl/">test</a>
)#####";

void setupWebServer()
{
  static const char textplainHEADER[] PROGMEM = "text/plain";
  static const char texthtmlHEADER[] PROGMEM = "text/html";

  // Start TCP (HTTP) server

  server.on( "/", []() {
    server.setContentLength( indexHTML.length());
    server.send( 200, texthtmlHEADER, indexHTML );
  });
  
  server.begin();
  Serial.println("TCP server started");  
}

