echo -n 'const char * sketchVersion = "' > gitTagVersion.h
git --no-pager describe --tags --always --dirty | tr --delete '\n' >>gitTagVersion.h
echo  -n '";' >> gitTagVersion.h
~/arduino-1.8.5/arduino --upload test.ino --pref custom_DebugLevel=esp32_none
rm gitTagVersion.h
