echo Compiling aquacontrol32 version: $(git describe --tags --always)
echo We are on branch: $(git branch | grep \* | cut -d ' ' -f2)
echo
echo "const char * sketchVersion = \"$(git describe --tags --always)\";" > gitTagVersion.h
~/arduino-1.8.9/arduino --board espressif:esp32:mhetesp32minikit --verify test.ino --pref custom_DebugLevel=esp32_none --port /dev/ttyUSB0 --pref upload.speed=921600 --pref build.path=temp --preserve-temp-files
rm gitTagVersion.h
