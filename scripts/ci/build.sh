#!/bin/bash

# check git, curl
command -v curl >/dev/null 2>&1 || { echo >&2 "Curl is not installed.  Aborting."; exit 1; }

GITTAG="$(git describe --tags)"
[ "$GITTAG" = "" ] && { GITTAG=dev; }
GITID="$(git rev-parse HEAD)_$(date +"%Y%m%d_%H%M")"
echo $GITTAG $GITID

BUILDDIR=$(pwd)/_build
BUILDBIN=$BUILDDIR/bin
BUILDFW=$BUILDDIR/fw
BUILDTMP=$BUILDDIR/tmp
ESP8266URL=https://arduino.esp8266.com/stable/package_esp8266com_index.json

mkdir -p $BUILDDIR
mkdir -p $BUILDBIN
mkdir -p $BUILDFW


export PATH=$BUILDBIN:$PATH

command -v arduino-cli >/dev/null 2>&1 || { echo >&2 "arduino-cli is not installed.  Installing..."; curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=$BUILDBIN sh; }


arduino-cli config init
arduino-cli core update-index --additional-urls $ESP8266URL
#arduino-cli core search esp8266 --additional-urls $ESP8266URL
arduino-cli core install esp8266:esp8266 --additional-urls $ESP8266URL

[ "$1" = "c1" ] && {
# add libraries and compile opaq c1 firmware
ARDUINO_SKETCHBOOK_DIR=. arduino-cli compile -v --build-path "$BUILDTMP/opaqc1" --fqbn esp8266:esp8266:nodemcuv2:eesz=4M3M opaq.ino -o "$BUILDFW/opaqc1-$GITTAG-$GITID" ;
}

arduino-cli core install arduino:avr

[ "$1" = "coproc" ] && {
# compile coprocessor firmware
ARDUINO_SKETCHBOOK_DIR=./avr/coprocessor arduino-cli compile -v --build-path "$BUILDTMP/opaqc1-coproc" --fqbn arduino:avr:nano:cpu=atmega328old ./avr/coprocessor/coprocessor.ino -o "$BUILDFW/opaqc1-coproc-$GITTAG-$GITID" ;
}

[ "$1" = "n1" ] && {
# compile n1 node firmware
ARDUINO_SKETCHBOOK_DIR=./avr/n1 arduino-cli compile -v --build-path "$BUILDTMP/opaqc1-n1" --fqbn arduino:avr:nano:cpu=atmega328 ./avr/n1/n1.ino -o "$BUILDFW/opaqn1-$GITTAG-$GITID" ;
}

[ "$1" = "" ] && { echo "Use 'build.sh [c1/coproc/n1]' to build acordingly." ; }





