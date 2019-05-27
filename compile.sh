currentVersion=""
if [ $(git branch | grep \* | cut -d ' ' -f2) == "master" ]
then
  currentVersion=$(git describe --tags --always)
else
  currentVersion=$(git branch | grep \* | cut -d ' ' -f2):$(git rev-parse --short HEAD)
fi
echo "const char * sketchVersion = \"$currentVersion\";" > gitTagVersion.h
echo Compiling aquacontrol32 version: $currentVersion
~/arduino-1.8.9/arduino --board espressif:esp32:mhetesp32minikit --verify test.ino --pref custom_DebugLevel=esp32_none --port /dev/ttyUSB0 --pref upload.speed=921600 --pref build.path=temp --preserve-temp-files --pref build.partition=default_ffat
rm gitTagVersion.h
