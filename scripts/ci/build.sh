#!/bin/bash

set -e

# check git, curl
command -v curl >/dev/null 2>&1 || { echo >&2 "Curl is not installed.  Aborting."; exit 1; }

GITTAG="$(git describe --tags | cut -c 1-5)"
[ "$GITTAG" = "" ] && { GITTAG=dev; }
GITID="$(git rev-parse --short HEAD)-$(date +"%Y%m%d-%H%M")"
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

VERSION=2.7.0
#VERSION=2.6.3

arduino-cli config init
arduino-cli core update-index --additional-urls $ESP8266URL
#arduino-cli core search esp8266 --additional-urls $ESP8266URL
arduino-cli core install "esp8266:esp8266@$VERSION" --additional-urls $ESP8266URL

# apply scheduler patch to esp8266 core
CURRENT=$(pwd)
cd ~/.arduino15/packages/esp8266/hardware/esp8266/
( patch -s -p0 --forward < $CURRENT/libraries/scheduler/core_esp8266_$VERSION.patch || true )
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
# esp8266:esp8266:nodemcuv2:eesz=4M3M
ARDUINO_SKETCHBOOK_DIR=. arduino-cli compile -v --build-path "$BUILDTMP/opaqc1" --fqbn esp8266:esp8266:nodemcuv2:xtal=80,vt=flash,exception=legacy,ssl=all,eesz=4M2M,led=2,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200 opaq.ino -o "$BUILDFW/opaqc1-${GITTAG//./}-$GITID" ;

MD5=($(md5sum "$BUILDFW/opaqc1-${GITTAG//./}-$GITID.bin"))
cp "$BUILDFW/opaqc1-${GITTAG//./}-$GITID.bin" "$BUILDFW/opaqc1-${GITTAG//./}-$GITID-md5-$MD5.bin" ;

# put tools available to the shell environment
export PATH=$PATH:$(pwd)/tools/

# backup current dir location
CURRENTDIR=$(pwd)

# create a new empty 'data' folder
mkdir -p $BUILDTMP/data/
cp $CURRENTDIR/data/www/   $BUILDTMP/data/ -r # copy www
cp $CURRENTDIR/data/iaqua/ $BUILDTMP/data/ -r # copy iaqua
cp $CURRENTDIR/data/sett/  $BUILDTMP/data/ -r # copy sett

cd $BUILDTMP/data/www/

npx google-closure-compiler --js=jquery.min.js \
jquery.mobile.min.js \
reconnecting-websocket.js \
jtsage-datebox.jqm.min.js \
jquery.jqplot.min.js \
jqplot.cursor.js \
jqplot.dateAxisRenderer.js \
jqplot.dragable.js \
jqplot.highlighter.js \
jqplot.mobile.js \
--js_output_file=opaqc1-all.js

gzip -9 -k -f "opaqc1-all.js"

for i in `find | grep -E "\.css$|\.js.map$"`; do gzip -9 -k -f "$i" ; done

cd $BUILDTMP/data/

# create www tar file
tar -cvf "$BUILDFW/opaqc1-files-www-${GITTAG//./}-$GITID.tar" --exclude='*.js' --exclude='*.css' --exclude='*.js.map' "www/"

# create iaqua tar file
tar -cvf "$BUILDFW/opaqc1-files-iaqua-${GITTAG//./}-$GITID.tar" "iaqua/"

# create sett tar file
tar -cvf "$BUILDFW/opaqc1-files-sett-${GITTAG//./}-$GITID.tar" "sett/"

cd $CURRENTDIR
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

[ "$1" = "" ] && { echo "Use 'build.sh [c1/coproc/n1/all]' to build acordingly." ; exit 1; }

echo "Build success."
exit 0



