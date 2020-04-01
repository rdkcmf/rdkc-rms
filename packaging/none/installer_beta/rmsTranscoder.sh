#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#########################################################################


#echo "---------- RMS Transcoder Script ----------"
# This is a transcoder script example used by transcode API of RMS

# Transcoder binary, change as needed
TRANSCODER_BIN=__RMS_AVCONV_BIN__
if [ ! -x $TRANSCODER_BIN ]
then
	echo "$TRANSCODER_BIN not found or wrong permissions"
	exit 1
fi

# We're using AVCONV for now, add the preset files path as well
AVCONV_DATADIR=__RMS_AVCONV_PRESETS__

# Encoders being used
ENC_VIDEO="libx264"
ENC_AUDIO="libfaac"

########## MAKE ONLY NECESSARY CHANGES BEYOND THIS POINT ##########

# Function to form the needed parameter
# $1 - value to be evaluated
# $2 - prepended to $1 if it is not '0'
# $3 - value to replace $1 if IT IS '0'
# $4 - trigger flag
formParams() {
  # Check for triggers first
  if [ -n "$4" ] && [ "$4" = "0" ]; then
    # Do nothing and just return since trigger is 0 (disabled)
    echo ""
  else
    # Trigger is passed and not "0"
    case "$1" in
      "na")
        # Do nothing
        echo ""
        ;;

      "copy")
        # Do nothing
        echo ""
        ;;

      "0")
        # Check if this $3 is not also '0'
        if [ "$3" = "0" ]; then
          # Do nothing
          echo ""
        else
          # Disable the track
          echo " -$3"
        fi
        ;;

      *)
        # Default option
        echo " -$2 $1"
        ;;
    esac
  fi
}

# -- parameters passed to this script --
RMS_SRC_URI=$1
RMS_VIDEO_BITRATE=$2
RMS_VIDEO_SIZE=$3
RMS_VIDEO_APP=$4
RMS_AUDIO_BITRATE=$5
RMS_AUDIO_CHANNELS=$6
RMS_AUDIO_FREQ=$7
RMS_AUDIO_APP=$8
RMS_OVERLAY=$9
RMS_CROP_X_POS=${10}
RMS_CROP_Y_POS=${11}
RMS_CROP_WIDTH=${12}
RMS_CROP_HEIGHT=${13}
RMS_DST_URI=${14}
RMS_DST_STREAM_NAME=${15}

TRANSCODE=""

# Add the extra param
if [ -n "$RMS_RTSP_TRANSPORT" ]; then
  TRANSCODE="-rtsp_transport tcp"
fi

# Add the source
TRANSCODE="$TRANSCODE -i $RMS_SRC_URI"

# Add the overlay and cropping as a single video filter
HAS_OVERLAY=$(formParams $RMS_OVERLAY 0 0)
HAS_CROPPING=$(formParams $RMS_CROP_WIDTH 0 0)

# If we have an overlay, setup the parameter
if [ -n "$HAS_OVERLAY" ]; then
  OVERLAY_CROPPING=" -i $RMS_OVERLAY -filter_complex overlay"
fi

# If we have cropping, setup the parameter
if [ -n "$HAS_CROPPING" ]; then
  CROPPING_PARAM="$RMS_CROP_WIDTH:$RMS_CROP_HEIGHT"

  # Check if we also have x and y positions
  if [ $RMS_CROP_X_POS != "na" ]; then
    CROPPING_PARAM="$CROPPING_PARAM:$RMS_CROP_X_POS:$RMS_CROP_Y_POS"
  fi

  OVERLAY_CROPPING=" -filter_complex crop=$CROPPING_PARAM"
fi

# If we have both, use a single filter_complex param
if [ -n "$HAS_OVERLAY" ] && [ -n "$HAS_CROPPING" ]; then
  OVERLAY_CROPPING=" -i $RMS_OVERLAY -filter_complex overlay,crop=$CROPPING_PARAM"
fi

# Add the overlay/cropping
TRANSCODE="$TRANSCODE$OVERLAY_CROPPING"

# Flag to indicate if we need to reencode video or not
REENCODE=0

# Check if we need reencoding
if [ -n "$OVERLAY_CROPPING" ]; then
  REENCODE=1
fi

# Add the video bitrate
TMP_VALUE=$(formParams $RMS_VIDEO_BITRATE b:v vn)
TRANSCODE="$TRANSCODE$TMP_VALUE"

# Check if we need reencoding
if [ -n "$TMP_VALUE" ] && [ "$TMP_VALUE" != " -vn" ]; then
  REENCODE=1
fi

# Add the video size
TMP_VALUE=$(formParams $RMS_VIDEO_SIZE s vn $RMS_VIDEO_BITRATE)
TRANSCODE="$TRANSCODE$TMP_VALUE"

# Check if we need reencoding
if [ -n "$TMP_VALUE" ]; then
  REENCODE=1
fi

# Add the video preset profile
TMP_VALUE=$(formParams $RMS_VIDEO_APP pre:v vn $RMS_VIDEO_BITRATE)
TRANSCODE="$TRANSCODE$TMP_VALUE"

# Check if we need reencoding
if [ -n "$TMP_VALUE" ]; then
  REENCODE=1
fi

# Add the output video codec
if [ $REENCODE = 1 ]; then
  TRANSCODE="$TRANSCODE -c:v $ENC_VIDEO"
else
  TRANSCODE="$TRANSCODE -c:v copy"
fi

# Flag to indicate if we need to reencode audio or not
REENCODE=0

# Add the audio bitrate
TMP_VALUE=$(formParams $RMS_AUDIO_BITRATE b:a an)
TRANSCODE="$TRANSCODE$TMP_VALUE"

# Check if we need reencoding
if [ -n "$TMP_VALUE" ] && [ "$TMP_VALUE" != " -an" ]; then
  REENCODE=1
fi

# Add the audio channel count
TMP_VALUE=$(formParams $RMS_AUDIO_CHANNELS ac an $RMS_AUDIO_BITRATE)
TRANSCODE="$TRANSCODE$TMP_VALUE"

# Check if we need reencoding
if [ -n "$TMP_VALUE" ]; then
  REENCODE=1
fi

# Add the audio frequency
TMP_VALUE=$(formParams $RMS_AUDIO_FREQ ar an $RMS_AUDIO_BITRATE)
TRANSCODE="$TRANSCODE$TMP_VALUE"

# Check if we need reencoding
if [ -n "$TMP_VALUE" ]; then
  REENCODE=1
fi

# Add the audio preset profile
TMP_VALUE=$(formParams $RMS_AUDIO_APP pre:a an $RMS_AUDIO_BITRATE)
TRANSCODE="$TRANSCODE$TMP_VALUE"

# Check if we need reencoding
if [ -n "$TMP_VALUE" ]; then
  REENCODE=1
fi

# Add the output audio codec
if [ $REENCODE = 1 ]; then
  TRANSCODE="$TRANSCODE -c:a $ENC_AUDIO"
else
  TRANSCODE="$TRANSCODE -c:a copy"
fi

# Add the target streamName
TRANSCODE="$TRANSCODE -metadata streamName=$RMS_DST_STREAM_NAME"

# Add the destination
TRANSCODE="$TRANSCODE $RMS_DST_URI"

# Run the binary
#echo "$TRANSCODER_BIN $TRANSCODE"
exec $TRANSCODER_BIN -loglevel error $TRANSCODE

