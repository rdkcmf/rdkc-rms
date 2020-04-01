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



#ifndef _PROTOCOLTYPES_H
#define	_PROTOCOLTYPES_H

#include "common.h"

//carrier protocols
#define PT_TCP					MAKE_TAG3('T','C','P')
#define PT_UDP					MAKE_TAG3('U','D','P')
#define PT_UDS					MAKE_TAG3('U','D','S')

//protocol for custom raw media
#define PT_RAW_MEDIA			MAKE_TAG3('R','A','W')

//variant protocols
#define PT_BIN_VAR				MAKE_TAG4('B','V','A','R')
#define PT_XML_VAR				MAKE_TAG4('X','V','A','R')
#define PT_JSON_VAR				MAKE_TAG4('J','V','A','R')

//RTMP protocols
#define PT_INBOUND_RTMP			MAKE_TAG2('I','R')
#define PT_INBOUND_RTMPS_DISC	MAKE_TAG3('I','R','S')
#define PT_OUTBOUND_RTMP		MAKE_TAG2('O','R')
#define PT_RTMP_DISSECTOR		MAKE_TAG2('R','S')

//encryption protocols
#define PT_RTMPE				MAKE_TAG2('R','E')
#define PT_INBOUND_SSL			MAKE_TAG4('I','S','S','L')
#define PT_OUTBOUND_SSL			MAKE_TAG4('O','S','S','L')
#define PT_INBOUND_DTLS			MAKE_TAG5('I','D','T','L','S')
#define PT_OUTBOUND_DTLS		MAKE_TAG5('O','D','T','L','S')

//MPEG-TS protocol
#define PT_INBOUND_TS			MAKE_TAG3('I','T','S')

//HTTP protocols
#define PT_INBOUND_HTTP				MAKE_TAG4('I','H','T','T')
#define PT_INBOUND_HTTP2			MAKE_TAG5('I','H','T','T','2')
#define PT_INBOUND_HTTP_FOR_RTMP	MAKE_TAG4('I','H','4','R')
#define PT_OUTBOUND_HTTP			MAKE_TAG4('O','H','T','T')
#define PT_OUTBOUND_HTTP2			MAKE_TAG5('O','H','T','T','2')
#define PT_OUTBOUND_HTTP_FOR_RTMP	MAKE_TAG4('O','H','4','R')
#define PT_HTTP_ADAPTOR				MAKE_TAG4('H','T','T','A')
#define PT_HTTP_MSS_LIVEINGEST		MAKE_TAG7('M','S','S','H','T','L','I')
#define PT_HTTP_MEDIA_RECV			MAKE_TAG5('I','H','T','M','R')
#define PT_INBOUND_HTTP_4_FMP4		MAKE_TAG4('I','H','4','F')

//Timer protocol
#define PT_TIMER				MAKE_TAG3('T','M','R')

//Live FLV protocols
#define PT_INBOUND_LIVE_FLV		MAKE_TAG4('I','L','F','L')
#define PT_OUTBOUND_LIVE_FLV	MAKE_TAG4('O','L','F','L')

//RTP/RTPS protocols
#define PT_RTSP					MAKE_TAG4('R','T','S','P')
#define PT_RTCP					MAKE_TAG4('R','T','C','P')
#define PT_INBOUND_RTP			MAKE_TAG4('I','R','T','P')
#define PT_OUTBOUND_RTP			MAKE_TAG4('O','R','T','P')
#define PT_RTP_NAT_TRAVERSAL	MAKE_TAG5('R','N','A','T','T')
#define PT_SDP					MAKE_TAG3('S','D','P')

//CLI protocols
#define PT_INBOUND_JSONCLI		MAKE_TAG8('I','J','S','O','N','C','L','I')
#define PT_HTTP_4_CLI			MAKE_TAG3('H','4','C')
#define PT_INBOUND_ASCIICLI		MAKE_TAG7('I','A','S','C','C','L','I')

#define PT_INBOUND_RPC			MAKE_TAG4('I','R','P','C')
#define PT_OUTBOUND_RPC			MAKE_TAG4('O','R','P','C')

//pass through protocol
#define PT_PASSTHROUGH MAKE_TAG2('P','T')

//DRM protocol
#define PT_DRM					MAKE_TAG3('D','R','M')

//Meta Data protocols
#define PT_INBOUND_NMEA         MAKE_TAG4('N','M','E','A')
#define PT_JSONMETADATA         MAKE_TAG8('J','S','O','N','M','E','T','A')
#define PT_OUTBOUND_VMF			MAKE_TAG6('O','U','T','V','M','F')
#define PT_INBOUND_VMFSCHEMA	MAKE_TAG8('I','N','V','M','S','K','M','A')

//WebSocket
#define PT_INBOUND_WS_FMP4		MAKE_TAG7('I','W','S','F','M','P','4')
#define PT_INBOUND_WS			MAKE_TAG3('I','W','S')
#define PT_OUTBOUND_WS			MAKE_TAG3('O','W','S')
#define PT_WS_METADATA			MAKE_TAG6('W','S','M','E','T','A')

//WebRTC Protocols
#define PT_WRTC_SIG				MAKE_TAG7('W','R','T','C','S','I','G')
#define PT_WRTC_CNX				MAKE_TAG7('W','R','T','C','C','N','X')

// Originally a carrier protocol, currently used for WebRTC
#define PT_SCTP					MAKE_TAG4('S','C','T','P')

#define PT_ICE_UDP					MAKE_TAG4('I','C','E','U')
#define PT_ICE_TCP					MAKE_TAG4('I','C','E','T')

#define PT_API_INTEGRATION		MAKE_TAG3('A','P','I')

#endif	/* _PROTOCOLTYPES_H */
