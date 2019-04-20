echo Compiling and flashing aquacontrol32 version: $(git describe --tags --always)
echo Branch: $(git branch | grep \* | cut -d ' ' -f2)
echo
if [ $(git branch | grep \* | cut -d ' ' -f2) == "master" ]
then
  echo "const char * sketchVersion = \"$(git describe --tags --always)\";" > gitTagVersion.h
else
  echo "const char * sketchVersion = \"$(git describe --tags --always):$(git branch | grep \* | cut -d ' ' -f2)\";" > gitTagVersion.h
fi
echo $(git describe --tags --always):$(git branch | grep \* | cut -d ' ' -f2)
~/arduino-1.8.9/arduino --board espressif:esp32:mhetesp32minikit --upload test.ino --pref custom_DebugLevel=esp32_none --port /dev/ttyUSB0 --pref upload.speed=921600 --preserve-temp-files --pref build.path=temp --pref build.partition=default_ffat --pref build.flash_freq=80m
rm gitTagVersion.h
