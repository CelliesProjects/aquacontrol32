echo "const char * sketchVersion = \"$(git describe --tags --always --dirty)\";" > gitTagVersion.h
~/arduino-1.8.5/arduino --upload test.ino --pref custom_DebugLevel=esp32_none
rm gitTagVersion.h
