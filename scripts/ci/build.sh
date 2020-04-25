#!/bin/bash

set -e

# check git, curl
command -v curl >/dev/null 2>&1 || { echo >&2 "Curl is not installed.  Aborting."; exit 1; }

GITTAG="$(git describe --tags | cut -c 1-5)"
[ "$GITTAG" = "" ] && { GITTAG=dev; }
GITID="$(git rev-parse --short HEAD)_$(date +"%Y%m%d_%H%M")"
echo "$GITTAG $GITID"

charvectortag=($(echo "${GITTAG//./}" | grep -o . ))
for i in ${charvectortag[@]}
do
  tagstream="$tagstream '$i', "
done

charvector=($(echo "$GITID" | grep -o . ))
for i in ${charvector[@]}
do
  idstream="$idstream '$i', "
done

echo "$tagstream |-| $idstream"

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

# apply scheduler patch to esp8266 core
CURRENT=$(pwd)
cd ~/.arduino15/packages/esp8266/hardware/esp8266/
patch -s -p0 --forward < $CURRENT/libraries/scheduler/core_esp8266_2.6.3.patch
cd $CURRENT


[ "$1" = "c1" ] || [ "$1" = "all" ] && {

# build tag
echo -e "
#ifndef VERSION_H
#define VERSION_H

static char id[25] = { $idstream '\0'};
static char version[4] = { $tagstream '\0'};

#endif // VERSION_H
" > version.h

# add libraries and compile opaq c1 firmware
ARDUINO_SKETCHBOOK_DIR=. arduino-cli compile -v --build-path "$BUILDTMP/opaqc1" --fqbn esp8266:esp8266:nodemcuv2:eesz=4M3M opaq.ino -o "$BUILDFW/opaqc1-$GITTAG-$GITID" ;
}

arduino-cli core install arduino:avr

[ "$1" = "coproc" ] || [ "$1" = "all" ] && {

# build tag
echo -e "
#ifndef VERSION_H
#define VERSION_H

#include \"protocol.h\"

static struct status st = {$tagstream $idstream 0x00};

#endif // VERSION_H
" > ./avr/coprocessor/version.h

# compile coprocessor firmware
ARDUINO_SKETCHBOOK_DIR=./avr/coprocessor arduino-cli compile -v --build-path "$BUILDTMP/opaqc1-coproc" --fqbn arduino:avr:nano:cpu=atmega328 ./avr/coprocessor/coprocessor.ino -o "$BUILDFW/opaqc1-coproc-$GITTAG-$GITID" ;

# to find avr-gcc path (ask arduino-cli for it)
AVRGCC=$(ARDUINO_SKETCHBOOK_DIR=./avr/coprocessor arduino-cli compile --show-properties --fqbn arduino:avr:nano:cpu=atmega328 ./avr/coprocessor/coprocessor.ino | grep "runtime.tools.avr-gcc.path=" | cut -d "=" -f 2- )

# use avr-objcopy to convert ihex to bin
$AVRGCC/bin/avr-objcopy -I ihex "$BUILDFW/opaqc1-coproc-$GITTAG-$GITID.hex" -O binary "$BUILDFW/opaqc1-coproc-$GITTAG-$GITID.bin" ;

}

[ "$1" = "n1" ] || [ "$1" = "all" ] && {
# compile n1 node firmware
ARDUINO_SKETCHBOOK_DIR=./avr/n1 arduino-cli compile -v --build-path "$BUILDTMP/opaqc1-n1" --fqbn arduino:avr:nano:cpu=atmega328 ./avr/n1/n1.ino -o "$BUILDFW/opaqn1-$GITTAG-$GITID" ;
}

[ "$1" = "" ] && { echo "Use 'build.sh [c1/coproc/n1/all]' to build acordingly." ; }

exit 0



