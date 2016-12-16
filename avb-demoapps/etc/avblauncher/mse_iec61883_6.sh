#!/bin/sh

#
# avblauncher parameter
#
TYPE=$1
PRIORITY=$2
PACKETIER=iec61883-6
MSE_SYSFS=/sys/class/ravb_mse

if [ "x$TYPE" = "xtalker" ]; then
  TALKER_MAC_ADDR=`cat /sys/class/net/eth0/address | tr -d ":"`
  UNIQUE_ID=$3
  DEST_ADDR=`echo $4 | tr -d ":"`
  MSE=mse0

  echo ${DEST_ADDR} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/dst_mac
  echo ${TALKER_MAC_ADDR} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/src_mac
  echo ${PRIORITY}  > ${MSE_SYSFS}/${MSE}/avtp_tx_param/priority
  echo ${UNIQUE_ID} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/uniqueid
  echo ${PACKETIER} > ${MSE_SYSFS}/${MSE}/packetizer/name
else
  STREAMID=`echo $3 | tr -d ":"`
  MSE=mse1

  echo ${STREAMID} > ${MSE_SYSFS}/${MSE}/avtp_rx_param/streamid
  echo ${PACKETIER} > ${MSE_SYSFS}/${MSE}/packetizer/name
fi

#
# mse alsa parameter
#
ALSA_SAMPLERATE=48000
ALSA_CHANNELS=2
ALSA_FORMAT=S16_LE
ALSA_TEST=sine
ALSA_DEVICE=hw:0,0
ALSA_MSE_DEVICE=`cat ${MSE_SYSFS}/${MSE}/info/device`

if [ "x$TYPE" = "xtalker" ]; then
  speaker-test \
    -D ${ALSA_MSE_DEVICE} \
    -c ${ALSA_CHANNELS} \
    -r ${ALSA_SAMPLERATE} \
    -F ${ALSA_FORMAT} \
    -t ${ALSA_TEST}
else
  amixer set "DVC Out" 1%
  arecord \
    -D ${ALSA_MSE_DEVICE} \
    -c ${ALSA_CHANNELS} \
    -r ${ALSA_SAMPLERATE} \
    -f ${ALSA_FORMAT} | aplay -D ${ALSA_DEVICE}
fi
