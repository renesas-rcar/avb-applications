#!/bin/sh

#
# avblauncher parameter
#
TYPE=$1
PRIORITY=$2
PACKETIER=aaf_pcm

if [ "x$TYPE" = "xtalker" ]; then
  TALKER_MAC_ADDR=`cat /sys/class/net/eth0/address | tr -d ":"`
  UNIQUE_ID=$3
  DEST_ADDR=`echo $4 | tr -d ":"`
  MSE=mse0

  echo ${DEST_ADDR} > /sys/module/mse_core/${MSE}/dst_mac
else
  TALKER_MAC_ADDR=`echo $3 | tr -d ":" | cut -c 1-12`
  UNIQUE_ID=`echo $3 | tr -d ":" | cut -c 13-16`
  MSE=mse1
fi

echo ${TALKER_MAC_ADDR} > /sys/module/mse_core/${MSE}/src_mac
echo ${PRIORITY} > /sys/module/mse_core/${MSE}/priority
echo ${UNIQUE_ID} > /sys/module/mse_core/${MSE}/unique_id
echo ${PACKETIER} > /sys/module/mse_core/${MSE}/packetizer_name

#
# mse alsa parameter
#
ALSA_SAMPLERATE=48000
ALSA_CHANNELS=2
ALSA_FORMAT=S16_LE
ALSA_TEST=sine
ALSA_DEVICE=hw:0,0
ALSA_MSE_DEVICE=`cat /sys/module/mse_core/${MSE}/device`

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
