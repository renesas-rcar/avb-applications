#!/bin/sh

#
# avblauncher parameter
#
TYPE=$1
PRIORITY=$2
PACKETIER=cvf_h264

if [ "x$TYPE" = "xtalker" ]; then
  TALKER_MAC_ADDR=`cat /sys/class/net/eth0/address | tr -d ":"`
  UNIQUE_ID=$3
  DEST_ADDR=`echo $4 | tr -d ":"`
  MSE=mse3

  echo ${DEST_ADDR} > /sys/module/mse_core/${MSE}/dst_mac
else
  TALKER_MAC_ADDR=`echo $3 | tr -d ":" | cut -c 1-12`
  UNIQUE_ID=`echo $3 | tr -d ":" | cut -c 13-16`
  MSE=mse2
fi

echo ${TALKER_MAC_ADDR} > /sys/module/mse_core/${MSE}/src_mac
echo ${PRIORITY} > /sys/module/mse_core/${MSE}/priority
echo ${UNIQUE_ID} > /sys/module/mse_core/${MSE}/unique_id
echo ${PACKETIER} > /sys/module/mse_core/${MSE}/packetizer_name

export XDG_RUNTIME_DIR=/run/user/root

#
# mse v4l2 parameter
#
V4L2_WIDTH=640
V4L2_HEIGHT=480
V4L2_FRAMERATE=30/1
V4L2_MSE_DEVICE=`cat /sys/module/mse_core/${MSE}/device`

if [ "x$TYPE" = "xtalker" ]; then
  gst-launch-1.0 -v \
    videotestsrc ! \
    video/x-raw,width=${V4L2_WIDTH},height=${V4L2_HEIGHT},framerate=${V4L2_FRAMERATE} ! \
    omxh264enc target-bitrate=15000000 ! \
    h264parse ! \
    identity ! \
    video/x-h264,width=${V4L2_WIDTH},height=${V4L2_HEIGHT},framerate=${V4L2_FRAMERATE},stream-format=byte-stream,alignment=au ! \
    identity ! \
    v4l2sink sync=false device=${V4L2_MSE_DEVICE}
else
  gst-launch-1.0 \
    v4l2src device=${V4L2_MSE_DEVICE} ! \
    video/x-h264,width=${V4L2_WIDTH},height=${V4L2_HEIGHT},framerate=${V4L2_FRAMERATE} ! \
    h264parse ! \
    omxh264dec ! \
    waylandsink sync=false
fi
