echo "const char * sketchVersion = \"$(git describe --tags --always)\";" > gitTagVersion.h
~/arduino-1.8.5/arduino --upload test.ino --pref custom_DebugLevel=esp32_info
rm gitTagVersion.h
