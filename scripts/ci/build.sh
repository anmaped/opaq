#!/bin/bash

# check git, curl
command -v curl >/dev/null 2>&1 || { echo >&2 "Curl is not installed.  Aborting."; exit 1; }

BUILDBIN=_build/bin
ESP8266URL=https://arduino.esp8266.com/stable/package_esp8266com_index.json

mkdir -p _build
mkdir -p $BUILDBIN


export PATH=$BUILDBIN:$PATH

command -v arduino-cli >/dev/null 2>&1 || { echo >&2 "arduino-cli is not installed.  Installing..."; curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=$BUILDBIN sh; }


arduino-cli config init
arduino-cli core update-index --additional-urls $ESP8266URL
arduino-cli core search esp8266 --additional-urls $ESP8266URL
arduino-cli core install esp8266:esp8266 --additional-urls $ESP8266URL

# add libraries

ARDUINO_SKETCHBOOK_DIR=. arduino-cli compile -v --fqbn esp8266:esp8266:nodemcu opaq.ino


