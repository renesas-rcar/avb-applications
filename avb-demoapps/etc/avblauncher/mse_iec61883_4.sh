#!/bin/sh

#
# avblauncher parameter
#
TYPE=$1
PRIORITY=$2
PACKETIER=iec61883-4
MSE_SYSFS=/sys/class/ravb_mse

if [ "x$TYPE" = "xtalker" ]; then
  TALKER_MAC_ADDR=`cat /sys/class/net/eth0/address | tr -d ":"`
  UNIQUE_ID=$3
  DEST_ADDR=`echo $4 | tr -d ":"`
  MSE=mse4

  echo ${DEST_ADDR} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/dst_mac
  echo ${TALKER_MAC_ADDR} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/src_mac
  echo ${PRIORITY}  > ${MSE_SYSFS}/${MSE}/avtp_tx_param/priority
  echo ${UNIQUE_ID} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/uniqueid
  echo ${PACKETIER} > ${MSE_SYSFS}/${MSE}/packetizer/name
else
  STREAMID=`echo $3 | tr -d ":"`
  MSE=mse4

  echo ${STREAMID} > ${MSE_SYSFS}/${MSE}/avtp_rx_param/streamid
  echo ${PACKETIER} > ${MSE_SYSFS}/${MSE}/packetizer/name
fi

#
# mse v4l2/alsa parameter (MPEG2TS)
#
ALSA_SAMPLERATE=44100
ALSA_CHANNELS=2
ALSA_FORMAT=S16LE
ALSA_DEVICE=hw:0,0
V4L2_MSE_DEVICE=`cat ${MSE_SYSFS}/${MSE}/info/device`

if [ "x$TYPE" = "xtalker" ]; then
  gst-launch-1.0 \
    filesrc location=./movie.ts ! \
    video/mpegts,systemstream=true ! \
    v4l2sink sync=false device=${V4L2_MSE_DEVICE}
else
  gst-launch-1.0 \
    v4l2src device=${V4L2_MSE_DEVICE} ! queue ! \
    tsdemux name=demux ! queue ! \
      aacparse ! \
      omxaaclcdec ! \
      audioconvert ! \
      audioresample ! \
      audio/x-raw,format=${ALSA_FORMAT},rate=${ALSA_SAMPLERATE},channels=${ALSA_CHANNELS} ! \
      alsasink device="${ALSA_DEVICE}" \
    demux. ! queue ! \
      h264parse ! \
      omxh264dec ! \
      waylandsink sync=false
fi
