#!/bin/sh

#
# avblauncher parameter
#
TYPE=$1

if [ "x$TYPE" = "xtalker" ]; then
  UNIQUE_ID=$2
  DEST_ADDR=$3
else
  STREAM_ID=$2
fi

#
# simple application parameter
#
SEND_DATA=/dev/zero
AVB_RX_DEVICE=/dev/avb_rx0

if [ "x$TYPE" = "xtalker" ]; then
  simple_talker -f ${SEND_DATA} -m 0 -u ${UNIQUE_ID} -i eth0 -a ${DEST_ADDR}
else
  avbtool -r ${AVB_RX_DEVICE} streamid ${STREAM_ID}
  simple_listener -d ${AVB_RX_DEVICE} -m 0
fi
