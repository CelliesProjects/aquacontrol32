echo "const char * sketchVersion = \"$(git describe --tags --always --dirty)\";" > gitTagVersion.h
~/arduino-1.8.5/arduino --verify test.ino
rm gitTagVersion.h
