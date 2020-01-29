#!/usr/bin/env bash
if [ $(grep GIT_TAG deviceSetup.h | grep -ic TRUE) != "1" ]
then
  echo -e "\e[31mERROR\e[0m GIT_TAG not set to true in deviceSetup.h."
  exit 1
fi
currentVersion=""
if [ $(git branch | grep \* | cut -d ' ' -f2) == "master" ]
then
  currentVersion=$(git describe --tags --always)
else
  currentVersion=$(git branch | grep \* | cut -d ' ' -f2):$(git rev-parse --short HEAD)
fi
echo "const char * sketchVersion = \"$currentVersion\";" > gitTagVersion.h
echo -e "Compiling aquacontrol32 version: \e[36m$currentVersion\e[0m"
~/arduino-1.8.11/arduino \
  --board espressif:esp32:mhetesp32minikit \
  --pref custom_DebugLevel=esp32_none \
  --port /dev/ttyUSB0 \
  --pref upload.speed=921600 \
  --preserve-temp-files \
  --pref build.path=temp \
  --pref build.partitions=default_ffat \
  --pref build.flash_freq=80m -v \
  --verify aquacontrol32.ino \
  | grep  -e 'Using core' -e 'Using board' -e ' uses ' -e 'Using library' -e 'Compiling' -e 'Global'
rm gitTagVersion.h
