### Aquacontrol32

Aquacontrol32 is software used to program and control 5 led strips to create more natural sunrises and sunsets in your aquarium.

Aquacontrol32 runs on hardware based on a [MH-ET LIVE MiniKit ESP32](http://mh.nodebb.com/topic/8/new-mh-et-live-minikit-for-esp32).
<br>You can connect a cheap 128x64 I2C OLED and/or a ILI9341 SPI tft display to have some feedback on the display(s).
<br>The ILI9341 displays optionally come with a XPT2046 touch controller which is supported.

#### Features:

- 5 channels led dimming (common anode) at 1.22kHz with 16 bit (65535 steps) resolution.

- 50 timers per channel.

- Password protected web interface to control the device.
<br>See it in action at my [fish](http://vissen.wasietsmet.nl/) and my [salamanders](http://salamanders.wasietsmet.nl/) tank.

- Automatic NTP timekeeping.

- Timezone support.

- OneWire Maxim ( Dallas ) DS18B20 sensor support.

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

- Source are compiled for `ESP32 Dev Module` which has support for ESP_LOGX macros.
<br>`Core debug level` (in `Tools` menu) should be set to `None` in the Arduino IDE for production use.
<br>When you are still testing your hardware and setup, debug level can be set to anything depending on your needs.
<br>(`Info` is probably what you need, `Verbose` gives the most info)

- OneWire is rather buggy at the moment, so until fixed you can use [stickbreakers modified OneWire library](https://github.com/stickbreaker/OneWire) to have troublefree DS18B20 sensors.

#### Connecting the hardware:

- Read the [file](tft_board_pins.md) on connecting a ILI9341 display. Pull-ups are not optional!

- The ILI9341 boards from AliExpress, DealExtreme or any other supplier are not all equal.
<br>Among the tested boards I encountered some that have no MISO pin connected, so they can't respond to read commands.
<br>For these boards you can enable `TFT_HAS_NO_MISO` (set it to 1) in `aquacontrol32.ino`.

- Some ILI9341 boards have their touch coordinates inverted.
<br>For these boards you can enable `TOUCH_IS_INVERTED` (set it to 1) in `aquacontrol32.ino`.

- Don't forget to connect the tft LED to 3.3V. (default: GPIO PIN 2)
<br>To be on the safe side, I use a BC547 transistor (and a 330R resistor) between the ESP32 pin and the LED connector on the tft board.
<br>If you connect the LED directly to a ESP32 pin, connect it through a 330R resistor in series to prevent burning up your ESP32.

#### SmartConfig:

- If your ESP32 has connected to your WiFi router before you flash Aquacontrol to your device, it will probably connect automagically .

- If you try to connect to an unknown WiFi network or changed your WiFi router settings, Aquacontrol will fail to connect and start SmartConfig.
<br>If you have no oled or tft connected, the onboard led will blink at 1Hz to show you the device is in SmartConfig mode.
<br>You can then use the Espressif SmartConfig app or the [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl) to setup your Aquacontrol WiFi connction.

#### Known issues:

SmartConfig is not working properly, probably needs some action from the Espressif dev team.

See [issue #1001](https://github.com/espressif/arduino-esp32/issues/1001).

To connect your device while this is sorted out, change line 7 in `wifitask.ino` from

````c
    WiFi.begin();
````
to

````c
    WiFi.begin( "YOURSSID", "YOURPSK" );
````

#### The hardware:

<a href="https://user-images.githubusercontent.com/24290108/33763793-1df0fe98-dc12-11e7-82a5-853e5a1d07d1.JPG"><img src="https://user-images.githubusercontent.com/24290108/33763798-2385a69c-dc12-11e7-81c4-2429f2fb88fd.JPG" height="320" width="512" ></a>
