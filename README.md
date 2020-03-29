[![Codacy Badge](https://api.codacy.com/project/badge/Grade/e743c6ad2f0f416e9d43cfb87965b89e)](https://www.codacy.com/manual/CelliesProjects/aquacontrol32?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=CelliesProjects/aquacontrol32&amp;utm_campaign=Badge_Grade)

### Aquacontrol32

Aquacontrol32 can control 5 led strips to create gradual sunrises and sunsets in your aquarium. It is developed for and tested on [MH-ET LIVE MiniKit ESP32](http://mh.nodebb.com/topic/8/new-mh-et-live-minikit-for-esp32) MCUs. Other than the led dimming hardware, no additional hardware is needed to control the device.

The minimum hardware would be a ESP32 board with at least 5 free output pins connected via 100R gate resistors to 5 NPN mosfets that drive the led lights. You will need logic level mosfets for example [IRLZ44N mosfets](http://www.irf.com/product-info/datasheets/data/irlz44n.pdf).

With some modifications and provided there are enough pins broken out, Aquacontrol32 should run on basically any ESP32 based board. The built-in web interface already gives access to most functions. The goal is v2.0 as a single binary with all functions accessible via the web interface.

You can connect a 128x64 I2C OLED and/or a ILI9341 SPI tft display to have some feedback on the display(s). The ILI9341 displays usually come with a XPT2046 touch controller which is supported (and assumed).

Another cool feature is support for 3 Dallas DS18B20 temperature sensors, with temperature logging to FATFS and a 30 day temperature history.

### Index

- [Video](#aquacontrol32-dimming-down-youtube-video)
- [Features](#features)
- [Requirements](#requirements)
- [Libraries needed for compiling](#libraries)
- [Quick start](#quick-start)
- [Compile options](#compile-options)
- [Compile notes](#compile-notes)
- [Connecting a ILI9341](#connecting-a-ili9341)
- [Lunar cycle night light](#lunar-cycle-night-light)
- [SmartConfig and WiFi setup](#smartconfig-and-wifi-setup)
- [DHCP or static IP](#DHCP-or-static-IP)
- [Log files](#log-files)
- [Known issues](#known-issues)
- [Libraries in the web interface](#Libraries-used-in-web-interface)

#### Aquacontrol32 dimming down YouTube video

[![VIDEO](https://img.youtube.com/vi/o2aeSjKm6FA/0.jpg  "Click to watch the video")](https://www.youtube.com/watch?v=o2aeSjKm6FA)

#### Features

  - 5 channels led dimming (common anode) through 1.22kHz PWM at 16 bit (65535 steps) resolution. The dimming control task runs at 100Hz to ensure smooth dimming.
  - Lunar cycle night light.
  - 50 timers per channel with a 1 minute resolution.
  - Password protected web interface to control the device. (default login is user:admin password:esp32)<br>See it in action at my [fish](https://vissen.wasietsmet.nl/) and my [salamanders](https://salamanders.wasietsmet.nl/) tank.
  - SNTP timekeeping with timezone support.
  - 3x OneWire DS18B20 sensor support and FFat storage with a 30 day temperature history.
  - SSD1306 128x64 OLED over I<sup>2</sup>C support.
  - ILI9341 320x240 TFT with XPT2046 touchscreen over SPI support.
  - All device settings are saved in NVS.
  - Easily connect your controller to WiFi with the [Espressif EsptouchForAndroid app](https://github.com/EspressifApp/EsptouchForAndroid/releases/latest).
  - Get a notification in the web interface if a new release is available.

#### Requirements

  - The latest [aquacontrol32 release](https://github.com/CelliesProjects/aquacontrol32/releases/latest).
  - The [Arduino IDE](https://arduino.cc/) 1.8.12.
  - The [ESP32 Arduino Core 1.0.4](https://github.com/espressif/arduino-esp32/releases/tag/1.0.4).

#### Libraries

Most libraries can be installed with the Arduino library Manager `Sketch > Include Library > Manage Libraries`.

A few have to be downloaded from GitHub:

  - [OneWire](https://github.com/stickbreaker/OneWire) 2.3.3 - Use this library instead of the standard Arduino OneWire library.
  - [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) 1.0.3
  - [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) 1.2.3
  - [MoonPhase](https://github.com/CelliesProjects/MoonPhase) 1.0.0
  - [Task](https://github.com/CelliesProjects/Task) 1.0.0
  - [FFatSensor](https://github.com/CelliesProjects/FFatSensor) 1.0.2

Install in the Arduino libraries or ESP32 libraries folder.

#### Quick start

  1. Download and unpack the latest release.
  2. Check if all libraries are installed and the correct version. (in `aquacontrol32.ino`)
  3. (Optional) Adjust the `wifi_password` and `wifi_network`. (in `aquacontrol32.ino`)
  4. Check and adjust device specific setup in `deviceSetup.h` and `devicePinSetup.h`.
  5. Flash your device. Remember to use a FFat partition!
  6. On the first boot ( or after a flash erase ) the internal flash drive will be formatted so first boot will take a little longer. Updating aquacontrol to a new version will not format the drive.

The sensors and displays should be plug and play, except the ILI9341 when it has no MISO pin connected. For these displays you can enable `TFT_HAS_NO_MISO` (set it to `true`) in `deviceSetup.h`.

#### Compile options

  - Board: MH ET LIVE ESP32MiniKit (A lot of other boards work just fine without any code changes. Check/adjust the particular pin setup if not.)
  - Partition scheme: Default with ffat

#### Compile notes

  - Check your device options in `deviceSetup.h` and `devicePinSetup.h`.
  - Source are compiled for `mhetesp32minikit` which has support for ESP_LOGX macros.
<br>This can be changed to a particular esp32 board by changing the `--board` option in the `compile.sh` and `flash.sh` scripts.
<br>Look in `~/Arduino/hardware/espressif/esp32/boards.txt` to find the relevant board desciption.
<br>`custom_DebugLevel` should be set to `esp32_none` in the `flash.sh` script for production use.
<br>When you are still testing your hardware and setup, debug level can be set to anything depending on your needs.
<br>(`esp32_info` is probably what you need, `esp32_verbose` gives the most info)

Toggle the `GIT_TAG` option in `deviceSetup.h` to enable or disable version information.
<br>Setting `GIT_TAG` to `true` makes that the Arduino IDE can no longer compile or flash your script.
<br>You then have to use the script `compile.sh` to verify your sketch and `flash.sh` to verify/upload the sketch to the controller. You might have to adjust these scripts for your particular OS or setup.
<br>Read [this blog post](https://wasietsmet.nl/arduino/add-git-tag-and-version-number-to-an-arduino-sketch/) to see why I choose this method.

#### Connecting a ILI9341

  - Check the [Aquacontrol hardware GitHub repo](https://github.com/CelliesProjects/aquacontrol-hardware).
  - Read the [file](tft_board_pins.md) on connecting a ILI9341 display. Pull-ups are not optional!
  - The ILI9341 boards from AliExpress, DealExtreme or any other supplier are not all equal.
<br>Among the tested boards I encountered some that have no MISO pin connected, so they can't respond to read commands.
<br>For these boards you can enable `TFT_HAS_NO_MISO` (set it to `true`) in `deviceSetup.h`.
  - Some ILI9341/XPT2046 boards have their touch coordinates inverted.
<br>For these boards you can enable `TOUCH_IS_INVERTED` (set it to `true`) in `deviceSetup.h`.
  - Don't forget to connect the tft LED to 3.3V. (default: GPIO PIN 2)
<br>To be on the safe side, I use a BC547 transistor (and a 100R resistor) between the ESP32 pin and the LED connector on the tft board.
<br>If you connect the LED directly to a ESP32 pin, connect it through a 100R resistor in series to prevent burning up your ESP32.

To override the device detection for ILI9341 displays you can use the `ILI9341 force` button in the `setup` page of the web interface. This will force a ILI9341 display until the device reboots.

#### Lunar cycle night light

  - Moon light settings can be adjusted in the `channels` area of the web-interface.
  - The lunar images used in the web interface are rendered by Jay Tanner and licenced under the [Creative Commons Attribution-ShareAlike 3.0 license](docs/near_side_256x256x8/README.md).
  - The [moon phase library](https://github.com/CelliesProjects/MoonPhase) is adapted from code kindly licensed by Hugh from [voidware.com](http://www.voidware.com/). Thanks Hugh!

#### SmartConfig and WiFi setup

Set your `wifi_network` and `wifi_password` in `aquacontrol32.ino` before you flash your device.
Double check because setting a wrong `wifi_network` or `wifi_password` will result in a boot loop!

Or use SmartConfig and take note of the next couple of things.

  - If your ESP32 has connected to your WiFi router before you flash Aquacontrol to your device, it will probably connect automagically .
  - If you try to connect to an unknown WiFi network or changed your WiFi router settings, Aquacontrol will fail to connect and start SmartConfig.
<br>If you have no oled or tft connected, the onboard led will blink at 1Hz to show you the device is in SmartConfig mode.
<br>You can then use the Android [Espressif SmartConfig app](https://github.com/EspressifApp/EsptouchForAndroid/releases/latest) or the Android [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl) or the IOS [EsptouchForIOS app](https://github.com/EspressifApp/EsptouchForIOS) to setup your WiFi connection.
- If after 5 minutes SmartConfig has not connected your device will reboot. This is a failsafe for home power outs and slow booting modems/routers.

##### Note: ESP32s can only connect to a 2.4Ghz WiFi network. Connect your phone to a 2.4Ghz network before starting the SmartConfig app.

##### Note 2: Since Android 9 only the latest Espressif app seems to work. Android apps now requires location access to probe WiFi.

#### DHCP or static IP

By default aquacontrol will get an ip address from the DHCP server.

Follow these steps to  to set a static ip address:

1. Enable `SET_STATIC_IP` (set it to `true`) in `aquacontrol32.ino`.
2. Change `STATIC_IP`, `GATEWAY`, `SUBNET`, `PRIMARY_DNS` and `SECONDARY_DNS` to the desired values.

#### Log files

By default log files are not generated.
<br>This is because log files saved on FFat could reduce the lifetime of the flash memory.
<br>Sensor logging can be enabled in the web interface.

#### Known issues

  - SmartConfig (actually the whole esp32) does not work on 5Ghz Wifi. Make sure you try to connect to 2.4Ghz Wifi.
  - Boards without a 4.7K pull-up on the `ONEWIRE_PIN` will in some cases display ghost sensors.
  - The `OneWire` library that comes with the Arduino IDE does not work with esp32 MCUs. Use the [stickbreaker OneWire library](https://github.com/stickbreaker/OneWire) for troublefree temperature sensors.
  - Some ILI9341/XPT2046 boards have their touch coordinates inverted.
<br>For these boards you can enable `TOUCH_IS_INVERTED` (set it to `true`) in `deviceSetup.h`.

Not really an issue but if your controller has a problem after flashing (no Wifi or stuck/not properly booting) reflashing after a full flash erase will solve it almost always.
<br>Always backup your `default.aqu` in the file manager before flashing and upload it back to the controller after you succesfully flashed your controller.
<br>Use this command to erase flash (FFat INCLUDED!) in Debian based Linux:
<br>`~/Arduino/hardware/espressif/esp32/tools/esptool.py --port /dev/ttyUSBx erase_flash`

#### Libraries used in web interface

- [jQuery 3.4.1](https://code.jquery.com/jquery-3.4.1.js) which is [available under MIT license](https://jquery.org/license/).
- [jCanvas 21.0.1](https://cdnjs.cloudflare.com/ajax/libs/jcanvas/21.0.1/min/jcanvas.min.js) which is [available under MIT license](https://github.com/caleb531/jcanvas/blob/master/LICENSE.txt).
- [Google Roboto font](https://fonts.google.com/specimen/Roboto) which is [available under Apache2.0 license](https://www.apache.org/licenses/LICENSE-2.0.html).

#### The test hardware

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

