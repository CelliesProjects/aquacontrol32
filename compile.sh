echo -n 'const char * sketchVersion = "' > gitTagVersion.h
git --no-pager describe --tags --always --dirty | tr --delete '\n' >>gitTagVersion.h
echo  -n '";' >> gitTagVersion.h
~/arduino-1.8.5/arduino --verify test.ino
rm gitTagVersion.h
