/* https://github.com/espressif/arduino-esp32/issues/846 */

char newRelease[8];
bool newReleaseFound = false;

void versionCheck ( void * pvParameters )
{
  const uint64_t versionCheckDelayTime = ( 1000 * 3600 ) / portTICK_PERIOD_MS;

  const char* latestReleaseAPI = "https://api.github.com/repos/CelliesProjects/aquacontrol32/releases/latest";
  const char* github_root_ca = \
                               "-----BEGIN CERTIFICATE-----\n" \
                               "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
                               "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
                               "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
                               "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
                               "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
                               "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
                               "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
                               "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
                               "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
                               "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
                               "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
                               "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
                               "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
                               "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
                               "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
                               "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
                               "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
                               "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
                               "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
                               "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
                               "+OkuE6N36B9K\n" \
                               "-----END CERTIFICATE-----\n";

  while (1)
  {
    HTTPClient http;

    http.begin( latestReleaseAPI, github_root_ca );
    uint8_t httpCode = http.GET();

    ESP_LOGI( TAG, "GitHub https request result: %i", httpCode );

    if ( httpCode == 200 )
    {
      const size_t capacity = 2200;
      // Use arduinojson.org/assistant to compute the capacity.
      DynamicJsonBuffer jsonBuffer( capacity );
      JsonObject& root = jsonBuffer.parseObject( http.getString() );
      if ( !root.success() )
      {
        ESP_LOGI( TAG, "Parsing of %s failed!", latestReleaseAPI );
      }
      else
      {
        char installedRelease[8];
        uint8_t i = 0;
        while ( ( sketchVersion[i] != '-' ) && ( i < sizeof( installedRelease ) ) )
        {
          installedRelease[i] = sketchVersion[i];
          i++;
        }
        strncpy( newRelease, root["tag_name"], sizeof( newRelease ) );
        ESP_LOGI( TAG, "Local Version: '%s'", installedRelease );

        if ( strcmp ( installedRelease, newRelease) < 0 )
        {
          newReleaseFound = true;
          ESP_LOGI( TAG, "Found new version: '%s'", newRelease );
        }
        else
        {
          ESP_LOGI( TAG, "No new version found.");
        }
      }
    }
    else
    {
      ESP_LOGI( TAG, "Error %i on HTTPS request.", httpCode );
    }
    http.end();
    vTaskDelay( versionCheckDelayTime );
  }
}

