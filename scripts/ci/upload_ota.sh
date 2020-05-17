#!/bin/bash

set -e

IP=$1
DATA=$2

# OTA - over-the-air

[ "$DATA" == "iaqua" ] && {

[ -d "./_build/tmp/data/iaqua" ] || { echo "iaqua directory unavailable." ; exit 0; }

for i in $(find ./_build/tmp/data/iaqua -maxdepth 1 -type f)
do
    echo $i
    curl -F "data=@$i" http://$IP/upload?src=/iaqua/
done ;

}

[ "$DATA" == "sett" ] && {

[ -d "./_build/tmp/data/sett" ] || { echo "sett directory unavailable." ; exit 0; }

for i in $(find ./_build/tmp/data/sett -maxdepth 1 -type f)
do
    echo $i
    curl -F "data=@$i" http://$IP/upload?src=/sett/
done ;

}

[ "$DATA" == "www" ] && {

[ -d "./_build/tmp/data/www" ] || { echo "www directory unavailable." ; exit 0; }

for i in $(find ./_build/tmp/data/www -maxdepth 1 -type f \( -name "*.html" -or -name "*.gz" \))
do
    echo $i
    curl -F "data=@$i" -# http://$IP/upload?src=/www/
done ;

}
