#!/bin/bash
addr='192.168.4.1'
for ((j=0;j<10000;j++))
do
curl http://$addr/advset
curl http://$addr/light
echo "Itteration "$j
done
exit 0

