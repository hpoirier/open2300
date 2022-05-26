#!/usr/bin/env sh

set +e

if [ ! -f /etc/open2300/open2300.conf ]
then
    cp /open2300/open2300.conf.orig /etc/open2300/open2300.conf
fi
while [ true ]
do
   /open2300/mqtt2300 /etc/open2300/open2300.conf
   sleep 300
done
