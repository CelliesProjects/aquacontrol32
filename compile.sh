echo Compiling aquacontrol32 version: $(git describe --tags --always)
echo We are on branch: $(git branch | grep \* | cut -d ' ' -f2)
echo
echo "const char * sketchVersion = \"$(git describe --tags --always)\";" > gitTagVersion.h
~/arduino-1.8.5/arduino --board espressif:esp32:esp32 --verify test.ino
rm gitTagVersion.h
