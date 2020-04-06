#!/bin/bash

# check git, curl
command -v curl >/dev/null 2>&1 || { echo >&2 "Curl is not installed.  Aborting."; exit 1; }

GITTAG="$(git describe --tags)"
[ "$GITTAG" = "" ] && { GITTAG=dev; }
GITID="$(git rev-parse HEAD)_$(date +"%Y%m%d_%H%M")"
echo $GITTAG $GITID

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

# add libraries and compile

ARDUINO_SKETCHBOOK_DIR=. arduino-cli compile -v --build-path $(pwd)/_build/tmp  --fqbn esp8266:esp8266:nodemcuv2:eesz=4M3M opaq.ino -o opaqc1-$GITTAG-$GITID




