### Aquacontrol32

Aquacontrol32 is software used to program and control 5 led strips to create more natural sunrises and sunsets in your aquarium.

Aquacontrol32 runs on hardware based on a [MH-ET LIVE MiniKit ESP32](http://mh.nodebb.com/topic/8/new-mh-et-live-minikit-for-esp32).
<br><br>With some modifications and provided there are enough pins broken out, Aquacontrol32 should run on basically any ESP32 based board.
<br><br>The minimum hardware would be a ESP32 board with at least 5 free output pins connected via 330R gate resistors to 5 NPN mosfets. I use IRLZ44N mosfets as these are cheap and have the right ratings for my setup.
<br><br>You can connect a 128x64 I2C OLED and/or a ILI9341 SPI tft display to have some feedback on the display(s).
<br>The ILI9341 displays optionally come with a XPT2046 touch controller which is supported.

#### Features:

- 5 channels led dimming (common anode) at 1.22kHz with 16 bit (65535 steps) resolution.

- Lunar cycle night light.

- 50 timers per channel.

- Password protected web interface to control the device.
<br>See it in action at my [fish](https://vissen.wasietsmet.nl/) and my [salamanders](https://salamanders.wasietsmet.nl/) tank.

- Automatic NTP timekeeping.

- Timezone support.

- OneWire Maxim ( Dallas ) DS18B20 sensor support.

- I2C 128x64 SSD1306 OLED support.

- SPI 320x240 ILI9341 TFT support.

- XPT2046 touchscreen support.

- SPIFFS storage support.

- Temperature logging on SPIFFS.

- All device settings are saved in NVS.

- Easily connect your controller to WiFi with the [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl).

- Get a notification in the web interface if a new release is available.


#### You will need:

- The latest [aquacontrol32 release](https://github.com/CelliesProjects/aquacontrol32/releases/latest).

- The [Arduino IDE](https://arduino.cc/) 1.8.7.

- The [ESP32 Arduino Core @ commit 85032b226c7775bae510e954112823a2ae32181a](https://github.com/espressif/arduino-esp32/commit/85032b226c7775bae510e954112823a2ae32181a).


Aquacontrol32 can run happily with or without OLED or TFT display.

#### Compile notes:

- Check your device options in `deviceSetup.h`.
- Compiling from the Arduino IDE does not work if you just cloned the repo.
<br>You have to use the script `compile.sh` to verify your sketch and `flash.sh` to verify/upload the sketch to the controller.
<br>Read [this blog post](https://wasietsmet.nl/arduino/add-git-tag-and-version-number-to-an-arduino-sketch/) to see why I choose this method.
<br>You can however still compile from the Arduino IDE if you manually add a file named `gitTagVersion.h` to your sketch folder with the following content:
<br>`const char * sketchVersion = "change this to a version string";`
<br>This file will be overwritten and deleted if you use the `flash.sh` or `compile.sh` script.

- Source are compiled for `mhetesp32minikit` which has support for ESP_LOGX macros.
<br>This can be changed to a particular esp32 board by changing the `--board` option in the `compile.sh` and `flash.sh` scripts. Look in `~/Arduino/hardware/espressif/esp32/boards.txt` to find the relevant board desciption.
<br>`custom_DebugLevel` should be set to `esp32_none` in the `flash.sh` script for production use.
<br>When you are still testing your hardware and setup, debug level can be set to anything depending on your needs.
<br>(`esp32_info` is probably what you need, `esp32_verbose` gives the most info)

- Check your [compile settings](compile_options.md) and [used libraries](libraries.md).

- If your controller has a problem after flashing (no Wifi or stuck/not properly booting) the most probable cause is corrupted NVS.
<br>Erasing the complete flash memory will solve most of these problems.
<br>Use this command to erase flash (SPIFFS INCLUDED!) in Linux:
<br>`~/Arduino/hardware/espressif/esp32/tools/esptool.py --port /dev/ttyUSB1 erase_flash`
<br>Backup your `default.aqu` in the file manager before erasing and upload it after you flash your controller.

#### Connecting the hardware:

- Check the [Aquacontrol hardware GitHub repo](https://github.com/CelliesProjects/aquacontrol-hardware).

- Read the [file](tft_board_pins.md) on connecting a ILI9341 display. Pull-ups are not optional!

- The ILI9341 boards from AliExpress, DealExtreme or any other supplier are not all equal.
<br>Among the tested boards I encountered some that have no MISO pin connected, so they can't respond to read commands.
<br>For these boards you can enable `TFT_HAS_NO_MISO` (set it to `true`) in `deviceSetup.h`.

- Some ILI9341 boards have their touch coordinates inverted.
<br>For these boards you can enable `TOUCH_IS_INVERTED` (set it to `true`) in `deviceSetup.h`.

- Don't forget to connect the tft LED to 3.3V. (default: GPIO PIN 2)
<br>To be on the safe side, I use a BC547 transistor (and a 330R resistor) between the ESP32 pin and the LED connector on the tft board.
<br>If you connect the LED directly to a ESP32 pin, connect it through a 330R resistor in series to prevent burning up your ESP32.

#### Lunar cycle night light:

When enabled, the settings for the minimum levels in the channel section of the webinterface become the full moon light value.
Can be disabled in `deviceSetup.h`. (set it to `false`)

The lunar images used in the web interface are rendered by Jay Tanner and licenced under the [Creative Commons Attribution-ShareAlike 3.0 license](docs/near_side_256x256x8/README.md).

The [moon phase library](https://github.com/CelliesProjects/MoonPhase) is adapted from code kindly licensed by Hugh from [voidware.com](http://www.voidware.com/). Thanks Hugh!

#### SmartConfig:

- If your ESP32 has connected to your WiFi router before you flash Aquacontrol to your device, it will probably connect automagically .

- If you try to connect to an unknown WiFi network or changed your WiFi router settings, Aquacontrol will fail to connect and start SmartConfig.
<br>If you have no oled or tft connected, the onboard led will blink at 1Hz to show you the device is in SmartConfig mode.
<br>You can then use the Espressif SmartConfig app or the [ESP8266 SmartConfig Android app](https://play.google.com/store/apps/details?id=com.cmmakerclub.iot.esptouch&hl=nl) to setup your Aquacontrol WiFi connction.

- If after 5 minutes SmartConfig has not connected your device will reboot. This way Aquacontrol32 will reconnect after a powerout when the modem is not yet online when Aquacontrol32 has booted the first time.

#### Log files:

By default log files are not generated.
<br>Log files saved on SPIFFS could reduce the lifetime of the flash memory.
<br>To log the temperature sensor values enable `LOG_FILES` (set it to `true`) in `deviceSetup.h`.

#### Known issues:

1. Use the master branch from [PaulStoffregen's XPT2046 Touchscreen library](https://github.com/paulstoffregen/XPT2046_Touchscreen) until the next release. 
<br>( Current release is 1.2, which does not have the IRAM fix.) See [this issue](https://github.com/PaulStoffregen/XPT2046_Touchscreen/issues/14).

2. Use the [stickbreaker OneWire library](https://github.com/stickbreaker/OneWire) for troublefree temperature sensors.

#### The test hardware:

<a href="https://user-images.githubusercontent.com/24290108/33763793-1df0fe98-dc12-11e7-82a5-853e5a1d07d1.JPG"><img src="https://user-images.githubusercontent.com/24290108/33763798-2385a69c-dc12-11e7-81c4-2429f2fb88fd.JPG" height="320" width="512" ></a>
