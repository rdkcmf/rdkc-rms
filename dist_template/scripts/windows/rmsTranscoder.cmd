rem ##########################################################################
rem # If not stated otherwise in this file or this component's LICENSE
rem # file the following copyright and licenses apply:
rem #
rem # Copyright 2019 RDK Management
rem #
rem # Licensed under the Apache License, Version 2.0 (the "License");
rem # you may not use this file except in compliance with the License.
rem # You may obtain a copy of the License at
rem #
rem # http://www.apache.org/licenses/LICENSE-2.0
rem #
rem # Unless required by applicable law or agreed to in writing, software
rem # distributed under the License is distributed on an "AS IS" BASIS,
rem # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem # See the License for the specific language governing permissions and
rem # limitations under the License.
rem #########################################################################
@echo off
setlocal enableDelayedExpansion

rem echo ---------- RMS Transcoder Script ----------
@title This is a transcoder script example used by transcode API of RMS

rem # get all command line parameters
set A1=%1
set A2=%2
set A3=%3
set A4=%4
set A5=%5
set A6=%6
set A7=%7
set A8=%8
set A9=%9
shift
shift
shift
shift
shift
shift
set A10=%4
set A11=%5
set A12=%6
set A13=%7
set A14=%~8
set A15=%9
shift
set A16=%9
rem echo A1=%A1% A2=%A2% A3=%A3% A4=%A4% A5=%A5%
rem echo A6=%A6% A7=%A7% A8=%A8% A9=%A9% A10=%A10%
rem echo A11=%A11% A12=%A12% A13=%A13% A14=%A14% A15=%A15% A16=%A16%

rem # Transcoder binary, change as needed
set TRANSCODER_BIN=__RMS_AVCONV_BIN__

rem # We're using AVCONV for now, add the preset files path as well
set AVCONV_DATADIR=__RMS_AVCONV_PRESETS__

rem # Encoders being used
set ENC_VIDEO=libx264
set ENC_AUDIO=libfaac

rem ########## MAKE ONLY NECESSARY CHANGES BEYOND THIS POINT ##########

rem # -- parameters passed to this script --
set RMS_SRC_URI=%A1%
set RMS_VIDEO_BITRATE=%A2%
set RMS_VIDEO_SIZE=%A3%
set RMS_VIDEO_APP=%A4%
set RMS_AUDIO_BITRATE=%A5%
set RMS_AUDIO_CHANNELS=%A6%
set RMS_AUDIO_FREQ=%A7%
set RMS_AUDIO_APP=%A8%
set RMS_OVERLAY=%A9%
set RMS_CROP_X_POS=%A10%
set RMS_CROP_Y_POS=%A11%
set RMS_CROP_WIDTH=%A12%
set RMS_CROP_HEIGHT=%A13%
set RMS_DST_URI=%A14%
set RMS_DST_STREAM_NAME=%A15%
set RMS_CMD_FLAGS=%A16%

rem # Enable file overwrite by default
set TRANSCODE=-y

rem # Add the extra param
if not "%RMS_RTSP_TRANSPORT%" == "" (
  set TRANSCODE=%TRANSCODE% -rtsp_transport tcp
)

rem # Add -re param as needed
if not "%RMS_FORCE_RE%" == "" (
  set TRANSCODE=%TRANSCODE% -re
)

rem # Add the source
set TRANSCODE=%TRANSCODE% -i %RMS_SRC_URI%

rem # Add the overlay and cropping as a single video filter
call :formParams result %RMS_OVERLAY% 0 0
set HAS_OVERLAY=%result%
call :formParams result %RMS_CROP_WIDTH% 0 0
set HAS_CROPPING=%result%

rem # If we have an overlay, setup the parameter
if not "%HAS_OVERLAY%" == "" (
	set OVERLAY_CROPPING= -i %RMS_OVERLAY% -filter_complex overlay
)

rem # If we have cropping, setup the parameter
if not "%HAS_CROPPING%" == "" (
	set CROPPING_PARAM=%RMS_CROP_WIDTH%:%RMS_CROP_HEIGHT%

	rem # Check if we also have x and y positions
	if not "%RMS_CROP_X_POS%" == "na" (
		set CROPPING_PARAM=!CROPPING_PARAM!:%RMS_CROP_X_POS%:%RMS_CROP_Y_POS%
	)

	set OVERLAY_CROPPING= -filter_complex crop=!CROPPING_PARAM!
)

rem # If we have both, use a single filter_complex param
if not "%HAS_OVERLAY%" == "" if not "%HAS_CROPPING%" == "" (
	set OVERLAY_CROPPING= -i %RMS_OVERLAY% -filter_complex overlay,crop=%CROPPING_PARAM%
)

rem # Add the overlay/cropping
set TRANSCODE=%TRANSCODE%%OVERLAY_CROPPING%

rem # Flag to indicate if we need to reencode video or not
set REENCODE=0

rem # Check if we need reencoding
if not "%OVERLAY_CROPPING%" == "" (
  set REENCODE=1
)

rem # Add the video bitrate
call :formParams result %RMS_VIDEO_BITRATE% b:v vn
set TRANSCODE=%TRANSCODE%%result%

rem # Check if we need reencoding
if not "%result%" == "" if not "%result%" == " -vn" (
  set REENCODE=1
)

rem # Add the video size
call :formParams result %RMS_VIDEO_SIZE% s vn %RMS_VIDEO_BITRATE%
set TRANSCODE=%TRANSCODE%%result%

rem # Check if we need reencoding
if not "%result%" == "" (
  set REENCODE=1
)

rem # Add the video preset profile
call :formParams result %RMS_VIDEO_APP% pre:v vn %RMS_VIDEO_BITRATE%
set TRANSCODE=%TRANSCODE%%result%

rem # Check if we need reencoding
if not "%result%" == "" (
  set REENCODE=1
)

rem # Add the output video codec
if %REENCODE% equ 1 (
  set TRANSCODE=%TRANSCODE% -c:v %ENC_VIDEO%
) else (
  set TRANSCODE=%TRANSCODE% -c:v copy
)

rem # Flag to indicate if we need to reencode audio or not
set REENCODE=0

rem # Add the audio bitrate
call :formParams result %RMS_AUDIO_BITRATE% b:a an
set TRANSCODE=%TRANSCODE%%result%

rem # Check if we need reencoding
if not "%result%" == "" if not "%result%" == " -an" (
  set REENCODE=1
)

rem # Add the audio channel count
call :formParams result %RMS_AUDIO_CHANNELS% ac an %RMS_AUDIO_BITRATE%
set TRANSCODE=%TRANSCODE%%result%

rem # Check if we need reencoding
if not "%result%" == "" (
  set REENCODE=1
)

rem # Add the audio frequency
call :formParams result %RMS_AUDIO_FREQ% ar an %RMS_AUDIO_BITRATE%
set TRANSCODE=%TRANSCODE%%result%

rem # Check if we need reencoding
if not "%result%" == "" (
  set REENCODE=1
)

rem # Add the audio preset profile
call :formParams result %RMS_AUDIO_APP% pre:a an %RMS_AUDIO_BITRATE%
set TRANSCODE=%TRANSCODE%%result%

rem # Check if we need reencoding
if not "%result%" == "" (
  set REENCODE=1
)

rem # Add the output audio codec
if %REENCODE% equ 1 (
  set TRANSCODE=%TRANSCODE% -c:a %ENC_AUDIO%
) else (
  set TRANSCODE=%TRANSCODE% -c:a copy
)

rem # Add the target streamName
set TRANSCODE=%TRANSCODE% %RMS_CMD_FLAGS% -metadata streamName=%RMS_DST_STREAM_NAME%

rem # Add the destination
set TRANSCODE=%TRANSCODE% %RMS_DST_URI%

rem # Run the binary
rem echo %TRANSCODER_BIN% %TRANSCODE%
%TRANSCODER_BIN% -loglevel error %TRANSCODE%
rem pause
endlocal
goto :eof

rem #########################################################
rem # Function to form the needed parameter
rem # %~1 - name of variable to store result in
rem # %~2 - value to be evaluated
rem # %~3 - prepended to %~2 if it is not '0'
rem # %~4 - value to replace %~2 if IT IS '0'
rem # %~5 - trigger flag
:formParams
	rem echo %~0 %~1 %~2 %~3 %~4 %~5 
	rem # Check for triggers first
	if "%~5"== "0" (
		rem # Do nothing and just return since trigger is 0 (disabled)
		set "%~1="
	) else (
		rem # Trigger is passed and not "0"

		rem # case begin
		if "%~2" == "na" (
			rem # Do nothing
			set "%~1="
		) else if "%~2" == "copy" (
			rem # Do nothing
			set "%~1="
		) else if "%~2" == "0" (
			rem # Check if this %~4 is not also '0'
			if "%~4" == "0" (
				rem # Do nothing
				set "%~1="
			) else (
				rem # Disable the track
				set "%~1= -%~4"
			)
		) else (
			rem # Default option
			set "%~1= -%~3 %~2"
		)
		rem # case end

	)
goto :eof
