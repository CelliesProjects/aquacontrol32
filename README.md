### Aquacontrol32

Aquacontrol32 is software used to program and control 5 led strips to create more natural sunrises and sunsets in your aquarium.

Aquacontrol32 runs on hardware based on a [MH-ET LIVE MiniKit ESP32](http://mh.nodebb.com/topic/8/new-mh-et-live-minikit-for-esp32).
<br>You can connect a cheap 128x64 I2C OLED and/or a ILI9341 SPI tft display to have some feedback on the display(s). 
<br>The ILI9341 displays optionally come with a XPT2046 touch controller which is supported.

#### The hardware:

<a href="https://user-images.githubusercontent.com/24290108/33763793-1df0fe98-dc12-11e7-82a5-853e5a1d07d1.JPG"><img src="https://user-images.githubusercontent.com/24290108/33763798-2385a69c-dc12-11e7-81c4-2429f2fb88fd.JPG" height="320" width="512" ></a>

#### Features:

- 5 channels led dimming (common anode) at 1.22kHz with 16 bit (65535 steps) resolution.

- 50 timers per channel.

- Password protected web interface to control the device.
<br>See it in action at my [fish](http://thuis.wasietsmet.nl:99/) and my [salamanders](http://thuis.wasietsmet.nl:88/) tank.

- Automatic NTP timekeeping.

- Timezone support.

- 1-Wire Maxim ( Dallas ) DS18B20 sensor support.

- I2C 128x64 SSD1306 OLED support.

- SPI 320x240 ILI9341 TFT support.

- XPT2046 touchscreen support.

- SPIFFS storage support.

- Temperature logging on SPIFFS.

- All settings are saved in NVS.

- Easily connect your controller to WiFi with the [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl).


#### You will need:

To compile or install Aquacontrol32, you will need the latest version [Arduino IDE](https://arduino.cc/), with the latest [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) installed.

Aquacontrol32 can run happily with or without OLED or TFT display.

#### Compile notes:

Core debug level (in 'Tools' menu) should be set to 'None' in the Arduino IDE for production use.

OneWire is rather buggy at the moment, so until fixed by Espressif you can use [stickbreakers modified OneWire library](https://github.com/stickbreaker/OneWire) to have troublefree DS18B20 sensors.
