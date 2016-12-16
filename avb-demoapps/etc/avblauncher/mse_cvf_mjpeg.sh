#!/bin/sh

#
# avblauncher parameter
#
TYPE=$1
PRIORITY=$2
PACKETIER=cvf_mjpeg
MSE_SYSFS=/sys/class/ravb_mse

if [ "x$TYPE" = "xtalker" ]; then
  TALKER_MAC_ADDR=`cat /sys/class/net/eth0/address | tr -d ":"`
  UNIQUE_ID=$3
  DEST_ADDR=`echo $4 | tr -d ":"`
  MSE=mse2

  echo ${DEST_ADDR} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/dst_mac
  echo ${TALKER_MAC_ADDR} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/src_mac
  echo ${PRIORITY}  > ${MSE_SYSFS}/${MSE}/avtp_tx_param/priority
  echo ${UNIQUE_ID} > ${MSE_SYSFS}/${MSE}/avtp_tx_param/uniqueid
  echo ${PACKETIER} > ${MSE_SYSFS}/${MSE}/packetizer/name
else
  STREAMID=`echo $3 | tr -d ":"`
  MSE=mse3

  echo ${STREAMID} > ${MSE_SYSFS}/${MSE}/avtp_rx_param/streamid
  echo ${PACKETIER} > ${MSE_SYSFS}/${MSE}/packetizer/name
fi

export XDG_RUNTIME_DIR=/run/user/root

#
# mse v4l2 parameter
#
V4L2_WIDTH=640
V4L2_HEIGHT=480
V4L2_FRAMERATE_NUMERATOR=30
V4L2_FRAMERATE_DENOMINATOR=1
V4L2_FRAMERATE=${V4L2_FRAMERATE_NUMERATOR}/${V4L2_FRAMERATE_DENOMINATOR}
V4L2_MSE_DEVICE=`cat ${MSE_SYSFS}/${MSE}/info/device`

if [ "x$TYPE" = "xtalker" ]; then
  echo ${V4L2_FRAMERATE_NUMERATOR} > ${MSE_SYSFS}/${MSE}/media_video_config/fps_numerator
  echo ${V4L2_FRAMERATE_DENOMINATOR} > ${MSE_SYSFS}/${MSE}/media_video_config/fps_denominator

  gst-launch-1.0 \
    filesrc location=./movie.mjpeg ! \
    image/jpeg,width=${V4L2_WIDTH},height=${V4L2_HEIGHT},framerate=${V4L2_FRAMERATE} ! \
    v4l2sink sync=false device=${V4L2_MSE_DEVICE}
else
  gst-launch-1.0 \
    v4l2src device=${V4L2_MSE_DEVICE} ! \
    image/jpeg,width=${V4L2_WIDTH},height=${V4L2_HEIGHT},framerate=${V4L2_FRAMERATE} ! \
    jpegparse ! \
    jpegdec ! \
    vspfilter ! \
    video/x-raw,format=NV12,width=${V4L2_WIDTH},height=${V4L2_HEIGHT} ! \
    waylandsink sync=false
fi
