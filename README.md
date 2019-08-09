### Aquacontrol32

Aquacontrol32 can control 5 led strips to create more natural sunrises and sunsets in your aquarium.

Other than the led dimming hardware, no additional hardware is needed.

Aquacontrol32 is developed for and tested on [MH-ET LIVE MiniKit ESP32](http://mh.nodebb.com/topic/8/new-mh-et-live-minikit-for-esp32) MCUs.

The minimum hardware would be a ESP32 board with at least 5 free output pins connected via 100R gate resistors to 5 NPN mosfets.

With some modifications and provided there are enough pins broken out, Aquacontrol32 should run on basically any ESP32 based board. The built-in web interface already gives access to most functions. The goal is v2.0 as a single binary with all functions accessible via the web interface.

You can connect a 128x64 I2C OLED and/or a ILI9341 SPI tft display to have some feedback on the display(s). The ILI9341 displays usually come with a XPT2046 touch controller which is supported (and assumed).

Another feature is support for 3 Dallas DS18B20 temperature sensors, with temperature logging to FATFS and a 30 day temperature history.

### Index
- [Video](#aquacontrol32-dimming-down-youtube-video)
- [Features](#features)
- [Libraries](#used-libraries)
- [Software used](#you-will-need)
- [Compile options](#compile-options)
- [Compile notes](#compile-notes)
- [Connecting the hardware](#connecting-the-hardware)
- [Lunar cycle night light](#lunar-cycle-night-light)
- [Smart config](#smartconfig)
- [Log files](#log-files)
- [Known issues](#known-issues)

#### Aquacontrol32 dimming down YouTube video

[![VIDEO](https://img.youtube.com/vi/o2aeSjKm6FA/0.jpg  "Click to watch the video")](https://www.youtube.com/watch?v=o2aeSjKm6FA)

#### Features:

- 5 channels led dimming (common anode) at 1.22kHz with 16 bit (65535 steps) resolution.
- Lunar cycle night light.
- 50 timers per channel.
- Password protected web interface to control the device. (default login is user:admin password:esp32)
<br>See it in action at my [fish](https://vissen.wasietsmet.nl/) and my [salamanders](https://salamanders.wasietsmet.nl/) tank.
- Automatic NTP timekeeping.
- Timezone support.
- OneWire DS18B20 sensor support and FFat storage with a 30 day temperature history.
- I2C SSD1306 128x64 OLED support.
- SPI ILI9341 320x240 TFT with XPT2046 touchscreen support.
- All device settings are saved in NVS.
- Easily connect your controller to WiFi with the [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl).
- Get a notification in the web interface if a new release is available.

#### You will need:

- The latest [aquacontrol32 release](https://github.com/CelliesProjects/aquacontrol32/releases/latest).
- The [Arduino IDE](https://arduino.cc/) 1.8.9.
- The [ESP32 Arduino Core 1.0.2](https://github.com/espressif/arduino-esp32/releases/tag/1.0.2).

#### Used Libraries:

Most libraries can be installed with the Arduino library Manager `Sketch > Include Library > Manage Libraries`.

A few have to be downloaded from GitHub:

- [OneWire](https://github.com/stickbreaker/OneWire) 2.3.3
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) 1.0.3
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) 1.2.2
- [MoonPhase](https://github.com/CelliesProjects/MoonPhase) 1.0.0
- [Task](https://github.com/CelliesProjects/Task) 1.0.0
- [FFatSensor](https://github.com/CelliesProjects/FFatSensor) 0.99.0

Install these libraries in the esp32 libraries folder.

#### Compile options.

- Board: MH ET LIVE ESP32MiniKit
- Upload Speed: 921600 (or lower)
- Flash frequency: 80Mhz
- Partition scheme: Default with ffat

#### Compile notes:

- Compare your installed libraries versions against the libraries in `aquacontrol32.ino`.
- Check your device options in `deviceSetup.h`.
- Toggle the `GIT_TAG` option in `deviceSetup.h` to enable or disable version information.
<br>Setting `GIT_TAG` to `true` makes that the Arduino IDE can no longer compile or flash your script.
<br>You then have to use the script `compile.sh` to verify your sketch and `flash.sh` to verify/upload the sketch to the controller.
<br>Read [this blog post](https://wasietsmet.nl/arduino/add-git-tag-and-version-number-to-an-arduino-sketch/) to see why I choose this method.
- Source are compiled for `mhetesp32minikit` which has support for ESP_LOGX macros.
<br>This can be changed to a particular esp32 board by changing the `--board` option in the `compile.sh` and `flash.sh` scripts.
<br>Look in `~/Arduino/hardware/espressif/esp32/boards.txt` to find the relevant board desciption.
<br>`custom_DebugLevel` should be set to `esp32_none` in the `flash.sh` script for production use.
<br>When you are still testing your hardware and setup, debug level can be set to anything depending on your needs.
<br>(`esp32_info` is probably what you need, `esp32_verbose` gives the most info)
- If your controller has a problem after flashing (no Wifi or stuck/not properly booting) a reflash after a full flash erase will solve it almost always.
<br>Backup your `default.aqu` in the file manager before erasing and upload it back to the controller after you flashed your controller.
<br>Use this command to erase flash (FFat INCLUDED!) in Linux:
<br>`~/Arduino/hardware/espressif/esp32/tools/esptool.py --port /dev/ttyUSB1 erase_flash`

#### Connecting the hardware:

- Check the [Aquacontrol hardware GitHub repo](https://github.com/CelliesProjects/aquacontrol-hardware).
- Read the [file](tft_board_pins.md) on connecting a ILI9341 display. Pull-ups are not optional!
- The ILI9341 boards from AliExpress, DealExtreme or any other supplier are not all equal.
<br>Among the tested boards I encountered some that have no MISO pin connected, so they can't respond to read commands.
<br>For these boards you can enable `TFT_HAS_NO_MISO` (set it to `true`) in `deviceSetup.h`.
- Some ILI9341 boards have their touch coordinates inverted.
<br>For these boards you can enable `TOUCH_IS_INVERTED` (set it to `true`) in `deviceSetup.h`.
- Don't forget to connect the tft LED to 3.3V. (default: GPIO PIN 2)
<br>To be on the safe side, I use a BC547 transistor (and a 100R resistor) between the ESP32 pin and the LED connector on the tft board.
<br>If you connect the LED directly to a ESP32 pin, connect it through a 100R resistor in series to prevent burning up your ESP32.

#### Lunar cycle night light:

- Moon light settings can be adjusted in the `channels` area of the web-interface.
- The lunar images used in the web interface are rendered by Jay Tanner and licenced under the [Creative Commons Attribution-ShareAlike 3.0 license](docs/near_side_256x256x8/README.md).
- The [moon phase library](https://github.com/CelliesProjects/MoonPhase) is adapted from code kindly licensed by Hugh from [voidware.com](http://www.voidware.com/). Thanks Hugh!

#### SmartConfig:

- If your ESP32 has connected to your WiFi router before you flash Aquacontrol to your device, it will probably connect automagically .
- If you try to connect to an unknown WiFi network or changed your WiFi router settings, Aquacontrol will fail to connect and start SmartConfig.
<br>If you have no oled or tft connected, the onboard led will blink at 1Hz to show you the device is in SmartConfig mode.
<br>You can then use the Espressif SmartConfig app or the [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl) to setup your Aquacontrol WiFi connction.
- If after 5 minutes SmartConfig has not connected your device will reboot. This is a failsafe for home power outs and slow booting modems/routers.

#### Log files:

By default log files are not generated.
<br>That is because log files saved on FFat could reduce the lifetime of the flash memory.
<br>Sensor logging can be enabled in the web interface.

#### Known issues:

- The OneWire library that comes with the Arduino IDE does not work with esp32 MCUs. Use the [stickbreaker OneWire library](https://github.com/stickbreaker/OneWire) for troublefree temperature sensors.

#### The test hardware:

<a href="https://user-images.githubusercontent.com/24290108/33763793-1df0fe98-dc12-11e7-82a5-853e5a1d07d1.JPG"><img src="https://user-images.githubusercontent.com/24290108/33763798-2385a69c-dc12-11e7-81c4-2429f2fb88fd.JPG" height="320" width="512" ></a>

#### This program is beerware.

I develop Aquacontrol32 in my spare time for fun.
Although I like to code, my afk time is equally important.
If you like the project, you could buy me a beer for some moral support.

[![paypal](https://www.paypalobjects.com/en_US/NL/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=MSP53ANQ3VV6J)

````
MIT License

Copyright (c) 2017 Cellie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
````

