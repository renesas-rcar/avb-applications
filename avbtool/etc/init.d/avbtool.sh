#!/bin/sh

while read LINE
do
  case "${LINE}" in
    \#*)
      ;;
    *)
      echo "avbtool ${LINE}"
      avbtool ${LINE} > /dev/null
      ;;
  esac
done < /etc/avbtool.conf
