void setupNTP()
{
  String NTPpoolAdress = COUNTRY_CODE_ISO_3166;

  NTPpoolAdress += ".pool.ntp.org";

  timeZone = readStringNVS( "timezone", "CET-1CEST,M3.5.0/2,M10.5.0/3" );

  tft.println( "Getting time from " + NTPpoolAdress );

  configTzTime( timeZone.c_str(), NTPpoolAdress.c_str() );

  //https://www.ibm.com/developerworks/aix/library/au-aix-posix/index.html#artdownload
  //https://github.com/espressif/esp-idf/blob/master/examples/protocols/sntp/README.md
  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-time.c
  //https://www.di-mgt.com.au/wclock/tz.html
  //http://www.catb.org/esr/time-programming/
  //http://www.lucadentella.it/en/2017/05/11/esp32-17-sntp/
  //https://github.com/espressif/esp-idf/blob/master/components/lwip/apps/sntp/sntp.c

  /* https://stackoverflow.com/questions/3673226/how-to-print-time-in-format-2009-08-10-181754-811 */

}
