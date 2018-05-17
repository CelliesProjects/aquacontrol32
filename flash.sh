echo Compiling and flashing aquacontrol32 version: $(git describe --tags --always)
echo
echo "const char * sketchVersion = \"$(git describe --tags --always)\";" > gitTagVersion.h
~/arduino-1.8.5/arduino --upload test.ino --pref custom_DebugLevel=esp32_none --port /dev/ttyUSB0
rm gitTagVersion.h
