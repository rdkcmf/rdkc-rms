/**
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
##########################################################################
**/



#ifndef _STREAMSTYPES_H
#define	_STREAMSTYPES_H

#define ST_IN					MAKE_TAG1('I')
#define ST_IN_NET				MAKE_TAG2('I','N')
#define ST_IN_DEVICE			MAKE_TAG2('I','D')
#define ST_IN_NET_RTMP			MAKE_TAG3('I','N','R')
#define ST_IN_NET_LIVEFLV		MAKE_TAG6('I','N','L','F','L','V')
#define ST_IN_NET_TS			MAKE_TAG4('I','N','T','S')
#define ST_IN_NET_RTP			MAKE_TAG3('I','N','P')
#define ST_IN_NET_EXT			MAKE_TAG5('I','N','E','X','T')
#define ST_IN_NET_PASSTHROUGH	MAKE_TAG3('I','N','S')
#define ST_IN_NET_RAW			MAKE_TAG5('I','N','R', 'A', 'W')
#define ST_IN_FILE				MAKE_TAG2('I','F')
#define ST_IN_FILE_RTMP			MAKE_TAG3('I','F','R')
#define ST_IN_FILE_TS			MAKE_TAG3('I','F','T')
#define ST_IN_FILE_PASSTHROUGH	MAKE_TAG3('I','F','S')
#define ST_IN_FILE_RTSP			MAKE_TAG3('I','F','P')
#define ST_OUT					MAKE_TAG1('O')
#define ST_OUT_NET				MAKE_TAG2('O','N')
#define ST_OUT_NET_EXT			MAKE_TAG5('O','N','E','X','T')
#define ST_OUT_NET_FMP4			MAKE_TAG6('O','N','F','M','P','4')
#define ST_OUT_NET_MP4			MAKE_TAG5('O','N','M','P','4')
#define ST_OUT_NET_RTMP			MAKE_TAG3('O','N','R')
#define ST_OUT_NET_RTP			MAKE_TAG3('O','N','P')
#define	ST_OUT_NET_TS			MAKE_TAG4('O','N','T','S')
#define ST_OUT_NET_RAW			MAKE_TAG6('O','U','T','R', 'A', 'W')
#define ST_OUT_NET_PASSTHROUGH	MAKE_TAG3('O','N','S')
#define ST_OUT_FILE				MAKE_TAG2('O','F')
#define ST_OUT_FILE_RTMP		MAKE_TAG3('O','F','R')
#define ST_OUT_FILE_RTMP_FLV	MAKE_TAG6('O','F','R','F','L','V')
#define ST_OUT_FILE_HLS			MAKE_TAG5('O','F','H','L','S')
#define ST_OUT_FILE_HDS			MAKE_TAG5('O','F','H','D','S')
#define ST_OUT_FILE_MSS			MAKE_TAG5('O','F','M','S','S')
#define ST_OUT_FILE_DASH		MAKE_TAG6('O','F','D','A','S','H')
#define ST_OUT_FILE_TS			MAKE_TAG4('O','F','T','S')
#define ST_OUT_FILE_MP4			MAKE_TAG5('O','F','M','P','4')
#define ST_OUT_META				MAKE_TAG5('O','M','E','T','A')
#define ST_OUT_VMF				MAKE_TAG4('O','V','M','F')
#endif	/* _STREAMSTYPES_H */


