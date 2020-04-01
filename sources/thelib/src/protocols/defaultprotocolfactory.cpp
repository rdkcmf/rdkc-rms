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


#include "protocols/defaultprotocolfactory.h"
#include "protocols/protocoltypes.h"
#include "protocols/tcpprotocol.h"
#include "protocols/rtmp/inboundrtmpprotocol.h"
#include "protocols/rtmp/outboundrtmpprotocol.h"
#include "protocols/ssl/inboundsslprotocol.h"
#include "protocols/ssl/outboundsslprotocol.h"
#include "protocols/ssl/inbounddtlsprotocol.h"
#include "protocols/ssl/outbounddtlsprotocol.h"
#include "protocols/ts/inboundtsprotocol.h"
#include "protocols/http/inboundhttpprotocol.h"
#include "protocols/rtmp/inboundhttp4rtmp.h"
#include "protocols/http/outboundhttpprotocol.h"
#include "protocols/liveflv/inboundliveflvprotocol.h"
#include "protocols/variant/xmlvariantprotocol.h"
#include "protocols/variant/binvariantprotocol.h"
#include "protocols/variant/jsonvariantprotocol.h"
#include "protocols/udpprotocol.h"
#include "protocols/rtp/rtspprotocol.h"
#include "protocols/rtp/inboundrtpprotocol.h"
#include "protocols/rtp/rtcpprotocol.h"
#include "protocols/cli/inboundjsoncliprotocol.h"
#ifdef HAS_PROTOCOL_ASCIICLI
#include "protocols/cli/inboundasciicliprotocol.h"
#endif /* HAS_PROTOCOL_ASCIICLI */
#include "protocols/rtmp/inboundrtmpsdiscriminatorprotocol.h"
#include "protocols/cli/http4cliprotocol.h"
#include "protocols/rtp/nattraversalprotocol.h"
#include "protocols/http/httpprotocol.h"
#include "protocols/http/httpadaptorprotocol.h"
#include "protocols/rpc/inboundrpcprotocol.h"
#include "protocols/rpc/outboundrpcprotocol.h"
#include "protocols/drm/verimatrixdrmprotocol.h"
#include "protocols/http/httpmssliveingest.h"
#include "protocols/http/httpmediareceiver.h"
#ifdef HAS_PROTOCOL_WEBRTC
#include "protocols/sctpprotocol.h"
#include "protocols/wrtc/wrtcsigprotocol.h"
#include "protocols/wrtc/wrtcconnection.h"
#include "protocols/wrtc/icetcpprotocol.h"
#endif // HAS_PROTOCOL_WEBRTC
#ifdef HAS_UDS
#include "protocols/udsprotocol.h"
#endif /* HAS_UDS */
#ifdef HAS_PROTOCOL_RAWMEDIA
#include "protocols/rawmedia/rawmediaprotocol.h"
#endif /* HAS_PROTOCOL_RAWMEDIA */
#ifdef HAS_PROTOCOL_WS
#include "protocols/ws/inboundwsprotocol.h"
#include "protocols/ws/outboundwsprotocol.h"
#include "protocols/ws/wsmetadataprotocol.h"
#ifdef HAS_PROTOCOL_WS_FMP4
#include "protocols/ws/inboundws4fmp4.h"
#endif /* HAS_PROTOCOL_WS_FMP4 */
#endif /* HAS_PROTOCOL_WS */
//#ifdef HAS_PROTOCOL_HTTP4FMP4
#include "protocols/fmp4/http4fmp4protocol.h"
//#endif
#ifdef HAS_PROTOCOL_API
#include "api3rdparty/apiprotocol.h"
#endif /* HAS_PROTOCOL_API */

DefaultProtocolFactory::DefaultProtocolFactory()
: BaseProtocolFactory() {

}

DefaultProtocolFactory::~DefaultProtocolFactory() {
}

vector<uint64_t> DefaultProtocolFactory::HandledProtocols() {
	vector<uint64_t> result;

	ADD_VECTOR_END(result, PT_TCP);
	ADD_VECTOR_END(result, PT_UDP);
	ADD_VECTOR_END(result, PT_SCTP);
	ADD_VECTOR_END(result, PT_INBOUND_SSL);
	ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
	ADD_VECTOR_END(result, PT_INBOUND_DTLS);
	ADD_VECTOR_END(result, PT_OUTBOUND_DTLS);
	ADD_VECTOR_END(result, PT_TIMER);
#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
	ADD_VECTOR_END(result, PT_INBOUND_TS);
#endif /* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */
#ifdef HAS_PROTOCOL_RTMP
	ADD_VECTOR_END(result, PT_INBOUND_RTMP);
	ADD_VECTOR_END(result, PT_INBOUND_RTMPS_DISC);
	ADD_VECTOR_END(result, PT_OUTBOUND_RTMP);
	ADD_VECTOR_END(result, PT_RTMPE);
#ifdef HAS_PROTOCOL_HTTP
	ADD_VECTOR_END(result, PT_INBOUND_HTTP_FOR_RTMP);
	ADD_VECTOR_END(result, PT_OUTBOUND_HTTP_FOR_RTMP);
#endif /* HAS_PROTOCOL_HTTP */
#endif /* HAS_PROTOCOL_RTMP */
#ifdef HAS_PROTOCOL_HTTP
	ADD_VECTOR_END(result, PT_INBOUND_HTTP);
	ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
#endif /* HAS_PROTOCOL_HTTP */
#ifdef HAS_PROTOCOL_HTTP2
	ADD_VECTOR_END(result, PT_INBOUND_HTTP2);
	ADD_VECTOR_END(result, PT_OUTBOUND_HTTP2);
	ADD_VECTOR_END(result, PT_HTTP_ADAPTOR);
    ADD_VECTOR_END(result, PT_HTTP_MSS_LIVEINGEST);
	ADD_VECTOR_END(result, PT_HTTP_MEDIA_RECV);
#endif /* HAS_PROTOCOL_HTTP2 */
#ifdef HAS_PROTOCOL_LIVEFLV
	ADD_VECTOR_END(result, PT_INBOUND_LIVE_FLV);
	ADD_VECTOR_END(result, PT_OUTBOUND_LIVE_FLV);
#endif /* HAS_PROTOCOL_LIVEFLV */
#ifdef HAS_PROTOCOL_VAR
	ADD_VECTOR_END(result, PT_BIN_VAR);
	ADD_VECTOR_END(result, PT_XML_VAR);
	ADD_VECTOR_END(result, PT_JSON_VAR);
#endif /* HAS_PROTOCOL_VAR */
#ifdef HAS_PROTOCOL_RTP
	ADD_VECTOR_END(result, PT_RTSP);
	ADD_VECTOR_END(result, PT_RTCP);
	ADD_VECTOR_END(result, PT_INBOUND_RTP);
	ADD_VECTOR_END(result, PT_RTP_NAT_TRAVERSAL);
	ADD_VECTOR_END(result, PT_SDP);
#endif /* HAS_PROTOCOL_RTP */
#ifdef HAS_PROTOCOL_CLI
	ADD_VECTOR_END(result, PT_INBOUND_JSONCLI);
	ADD_VECTOR_END(result, PT_HTTP_4_CLI);
#ifdef HAS_PROTOCOL_ASCIICLI
	ADD_VECTOR_END(result, PT_INBOUND_ASCIICLI);
#endif /* HAS_PROTOCOL_ASCIICLI */
#endif /* HAS_PROTOCOL_CLI */
//#ifdef HAS_PROTOCOL_HTTP4FMP4
	ADD_VECTOR_END(result, PT_INBOUND_HTTP_4_FMP4);
//#endif
#ifdef HAS_PROTOCOL_RPC
	ADD_VECTOR_END(result, PT_INBOUND_RPC);
	ADD_VECTOR_END(result, PT_OUTBOUND_RPC);
#endif /* HAS_PROTOCOL_RPC */
#ifdef HAS_PROTOCOL_DRM
	ADD_VECTOR_END(result, PT_DRM);
#endif /* HAS_PROTOCOL_DRM */
#ifdef HAS_UDS
	ADD_VECTOR_END(result, PT_UDS);
#endif /* HAS_UDS */
#ifdef HAS_PROTOCOL_RAWMEDIA
	ADD_VECTOR_END(result, PT_RAW_MEDIA);
#endif /* HAS_PROTOCOL_RAWMEDIA */
#ifdef HAS_PROTOCOL_WS
	ADD_VECTOR_END(result, PT_INBOUND_WS);
	ADD_VECTOR_END(result, PT_OUTBOUND_WS);
	ADD_VECTOR_END(result, PT_WS_METADATA);
#ifdef HAS_PROTOCOL_WS_FMP4
	ADD_VECTOR_END(result, PT_INBOUND_WS_FMP4);
#endif /* HAS_PROTOCOL_WS_FMP4 */
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_WEBRTC
	ADD_VECTOR_END(result, PT_WRTC_SIG);
	ADD_VECTOR_END(result, PT_WRTC_CNX);
	ADD_VECTOR_END(result, PT_ICE_TCP);
#endif /* HAS_PROTOCOL_WEBRTC */
#ifdef HAS_PROTOCOL_API
	ADD_VECTOR_END(result, PT_API_INTEGRATION);
#endif /* HAS_PROTOCOL_API */
	return result;
}

vector<string> DefaultProtocolFactory::HandledProtocolChains() {
	vector<string> result;
#ifdef HAS_PROTOCOL_RTMP
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_RTMP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_RTMP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_RTMPE);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_RTMPS);
#ifdef HAS_PROTOCOL_HTTP
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_RTMPS);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_RTMPT);
#endif /* HAS_PROTOCOL_HTTP */
#endif /* HAS_PROTOCOL_RTMP */
#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_TCP_TS);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_UDP_TS);
#endif /* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */
#ifdef HAS_PROTOCOL_HTTP
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTP);
#endif /* HAS_PROTOCOL_HTTP */
#ifdef HAS_PROTOCOL_LIVEFLV
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_LIVE_FLV);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_LIVE_FLV);
#endif /* HAS_PROTOCOL_LIVEFLV */
#ifdef HAS_PROTOCOL_VAR
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_XML_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_BIN_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_JSON_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_XML_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_BIN_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_JSON_VARIANT);
#ifdef HAS_PROTOCOL_HTTP
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP_XML_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP_BIN_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP_JSON_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTPS_XML_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTPS_BIN_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTPS_JSON_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTP_XML_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTP_BIN_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTP_JSON_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTPS_XML_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTPS_BIN_VARIANT);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTPS_JSON_VARIANT);
#endif /* HAS_PROTOCOL_HTTP */
#endif /* HAS_PROTOCOL_VAR */
#ifdef HAS_PROTOCOL_RTP
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_RTSP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_RTSP_RTCP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_UDP_RTCP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_RTSP_RTP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_UDP_RTP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_RTP_NAT_TRAVERSAL);
#endif /* HAS_PROTOCOL_RTP */
#ifdef HAS_PROTOCOL_CLI
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_CLI_JSON);
#ifdef HAS_PROTOCOL_ASCIICLI
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_CLI_ASCII);
#endif /* HAS_PROTOCOL_ASCIICLI */
#ifdef HAS_PROTOCOL_HTTP
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP_CLI_JSON);
#endif /* HAS_PROTOCOL_HTTP */
//#ifdef HAS_PROTOCOL_HTTP4FMP4
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP_FMP4);
//#endif
#ifdef HAS_PROTOCOL_WS
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_WS_CLI_JSON);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_WSS_CLI_JSON);
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_HTTP2
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTP2);
    ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTP_MSS_INGEST);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP_MEDIA_RCV);
#endif /* HAS_PROTOCOL_HTTP2 */
#endif /* HAS_PROTOCOL_CLI */
#ifdef HAS_PROTOCOL_RPC
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_RPC);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_SSL_RPC);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_RPC);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_SSL_RPC);
#ifdef HAS_PROTOCOL_WS
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_WS_RPC);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_WSS_RPC);
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_HTTP2
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTP_RPC);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_HTTPS_RPC);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTP_RPC);
	ADD_VECTOR_END(result, CONF_PROTOCOL_OUTBOUND_HTTPS_RPC);
#endif /* HAS_PROTOCOL_HTTP2 */
#endif /* HAS_PROTOCOL_RPC */
#ifdef HAS_PROTOCOL_DRM
#ifdef HAS_PROTOCOL_HTTP
	ADD_VECTOR_END(result, CONF_PROTOCOL_DRM_HTTP);
	ADD_VECTOR_END(result, CONF_PROTOCOL_DRM_HTTPS);
#endif /* HAS_PROTOCOL_HTTP */
#endif /* HAS_PROTOCOL_DRM */
#ifdef HAS_UDS
	ADD_VECTOR_END(result, CONF_PROTOCOL_UDS_RAW);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_UDS_CLI_JSON);
#endif /* HAS_UDS */
	ADD_VECTOR_END(result, CONF_PROTOCOL_TCP_RAW);
	ADD_VECTOR_END(result, CONF_PROTOCOL_UDP_RAW);
#ifdef HAS_PROTOCOL_WS
#ifdef HAS_PROTOCOL_WS_FMP4
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_WS_FMP4);
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_WSS_FMP4);
#endif /* HAS_PROTOCOL_WS_FMP4 */
	ADD_VECTOR_END(result, CONF_PROTOCOL_INBOUND_WS_JSON_META);
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_WEBRTC
	ADD_VECTOR_END(result, CONF_PROTOCOL_WRTC_SIG);
	ADD_VECTOR_END(result, CONF_PROTOCOL_WRTC_SIG_SSL);
	ADD_VECTOR_END(result, CONF_PROTOCOL_WRTC_CNX);
	ADD_VECTOR_END(result, CONF_PROTOCOL_WRTC_ICE_TCP);
#endif /* HAS_PROTOCOL_WEBRTC */
#ifdef HAS_PROTOCOL_API
	ADD_VECTOR_END(result, CONF_PROTOCOL_API_RAW);
#endif	/* HAS_PROTOCOL_API */
	return result;
}

vector<uint64_t> DefaultProtocolFactory::ResolveProtocolChain(string name) {
	vector<uint64_t> result;
	if (false) {

	}
#ifdef HAS_PROTOCOL_RTMP
	else if (name == CONF_PROTOCOL_INBOUND_RTMP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_RTMP);
	} else if ((name == CONF_PROTOCOL_OUTBOUND_RTMP)
			|| (name == CONF_PROTOCOL_OUTBOUND_RTMPE)) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_RTMP);
	} else if (name == CONF_PROTOCOL_OUTBOUND_RTMPS) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_RTMP);
	} else if (name == CONF_PROTOCOL_INBOUND_RTMPS) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_RTMPS_DISC);
	}
#ifdef HAS_PROTOCOL_HTTP
	else if (name == CONF_PROTOCOL_INBOUND_RTMPT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP_FOR_RTMP);
	}
#endif /* HAS_PROTOCOL_HTTP */
#endif /* HAS_PROTOCOL_RTMP */
#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
	else if (name == CONF_PROTOCOL_INBOUND_TCP_TS) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_TS);
	} else if (name == CONF_PROTOCOL_INBOUND_UDP_TS) {
		ADD_VECTOR_END(result, PT_UDP);
		ADD_VECTOR_END(result, PT_INBOUND_TS);
	}
#endif /* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */
#ifdef HAS_PROTOCOL_RTP
	else if (name == CONF_PROTOCOL_INBOUND_RTSP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_RTSP);
	} else if (name == CONF_PROTOCOL_RTSP_RTCP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_RTSP);
		ADD_VECTOR_END(result, PT_RTCP);
	} else if (name == CONF_PROTOCOL_UDP_RTCP) {
		ADD_VECTOR_END(result, PT_UDP);
		ADD_VECTOR_END(result, PT_RTCP);
	} else if (name == CONF_PROTOCOL_INBOUND_RTSP_RTP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_RTSP);
		ADD_VECTOR_END(result, PT_INBOUND_RTP);
	} else if (name == CONF_PROTOCOL_INBOUND_UDP_RTP) {
		ADD_VECTOR_END(result, PT_UDP);
		ADD_VECTOR_END(result, PT_INBOUND_RTP);
	} else if (name == CONF_PROTOCOL_RTP_NAT_TRAVERSAL) {
		ADD_VECTOR_END(result, PT_UDP);
		ADD_VECTOR_END(result, PT_RTP_NAT_TRAVERSAL);
	}
#endif /* HAS_PROTOCOL_RTP */
#ifdef HAS_PROTOCOL_HTTP
	else if (name == CONF_PROTOCOL_OUTBOUND_HTTP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
	}
#endif /* HAS_PROTOCOL_HTTP */
#ifdef HAS_PROTOCOL_LIVEFLV
	else if (name == CONF_PROTOCOL_INBOUND_LIVE_FLV) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_LIVE_FLV);
	}
#endif /* HAS_PROTOCOL_LIVEFLV */
#ifdef HAS_PROTOCOL_VAR
	else if (name == CONF_PROTOCOL_INBOUND_XML_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_XML_VAR);
	} else if (name == CONF_PROTOCOL_INBOUND_BIN_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_BIN_VAR);
	} else if (name == CONF_PROTOCOL_INBOUND_JSON_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_JSON_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_XML_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_XML_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_BIN_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_BIN_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_JSON_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_JSON_VAR);
	}
#ifdef HAS_PROTOCOL_HTTP
	else if (name == CONF_PROTOCOL_INBOUND_HTTP_XML_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP);
		ADD_VECTOR_END(result, PT_XML_VAR);
	} else if (name == CONF_PROTOCOL_INBOUND_HTTP_BIN_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP);
		ADD_VECTOR_END(result, PT_BIN_VAR);
	} else if (name == CONF_PROTOCOL_INBOUND_HTTP_JSON_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP);
		ADD_VECTOR_END(result, PT_JSON_VAR);
	} else if (name == CONF_PROTOCOL_INBOUND_HTTPS_XML_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP);
		ADD_VECTOR_END(result, PT_XML_VAR);
	} else if (name == CONF_PROTOCOL_INBOUND_HTTPS_BIN_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP);
		ADD_VECTOR_END(result, PT_BIN_VAR);
	} else if (name == CONF_PROTOCOL_INBOUND_HTTPS_JSON_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP);
		ADD_VECTOR_END(result, PT_JSON_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTP_XML_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_XML_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTP_BIN_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_BIN_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTP_JSON_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_JSON_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTPS_XML_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_XML_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTPS_BIN_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_BIN_VAR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTPS_JSON_VARIANT) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_JSON_VAR);
	}
#endif /* HAS_PROTOCOL_HTTP */
#endif /* HAS_PROTOCOL_VAR */
#ifdef HAS_PROTOCOL_CLI
	else if (name == CONF_PROTOCOL_INBOUND_CLI_JSON) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_JSONCLI);
	}
#ifdef HAS_PROTOCOL_HTTP
	else if (name == CONF_PROTOCOL_INBOUND_HTTP_CLI_JSON) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP);
		ADD_VECTOR_END(result, PT_HTTP_4_CLI);
		ADD_VECTOR_END(result, PT_INBOUND_JSONCLI);
	}
#endif /* HAS_PROTOCOL_HTTP */
//#ifdef HAS_PROTOCOL_HTTP4FMP4
	else if (name == CONF_PROTOCOL_INBOUND_HTTP_FMP4) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP_4_FMP4);
	}
//#endif
#ifdef HAS_PROTOCOL_ASCIICLI
	else if (name == CONF_PROTOCOL_INBOUND_CLI_ASCII) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_ASCIICLI);
	}
#endif /* HAS_PROTOCOL_ASCIICLI */
#ifdef HAS_PROTOCOL_WS
	else if (name == CONF_PROTOCOL_INBOUND_WS_CLI_JSON) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_WS);
		ADD_VECTOR_END(result, PT_INBOUND_JSONCLI);
	} else if (name == CONF_PROTOCOL_INBOUND_WSS_CLI_JSON) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_WS);
		ADD_VECTOR_END(result, PT_INBOUND_JSONCLI);
	}
#endif /* HAS_PROTOCOL_WS */
#endif /* HAS_PROTOCOL_CLI */
#ifdef HAS_PROTOCOL_HTTP2
	else if (name == CONF_PROTOCOL_INBOUND_HTTP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_HTTP_ADAPTOR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTP2) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_HTTP_ADAPTOR);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTP_MSS_INGEST) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_HTTP_MSS_LIVEINGEST);
	} else if (name == CONF_PROTOCOL_INBOUND_HTTP_MEDIA_RCV) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_HTTP_MEDIA_RECV);
	}
#endif /* HAS_PROTOCOL_HTTP2 */
#ifdef HAS_PROTOCOL_RPC
	else if (name == CONF_PROTOCOL_INBOUND_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_RPC);
	} else if (name == CONF_PROTOCOL_INBOUND_SSL_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_RPC);
	} else if (name == CONF_PROTOCOL_OUTBOUND_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_RPC);
	} else if (name == CONF_PROTOCOL_OUTBOUND_SSL_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_RPC);
	}
#ifdef HAS_PROTOCOL_HTTP2
	else if (name == CONF_PROTOCOL_INBOUND_HTTP_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_INBOUND_RPC);
	} else if (name == CONF_PROTOCOL_INBOUND_HTTPS_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_INBOUND_RPC);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTP_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_OUTBOUND_RPC);
	} else if (name == CONF_PROTOCOL_OUTBOUND_HTTPS_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP2);
		ADD_VECTOR_END(result, PT_OUTBOUND_RPC);
	}
#endif /* HAS_PROTOCOL_HTTP2 */
#ifdef HAS_PROTOCOL_WS
	else if (name == CONF_PROTOCOL_OUTBOUND_WS_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_WS);
		ADD_VECTOR_END(result, PT_OUTBOUND_RPC);
	} else if (name == CONF_PROTOCOL_OUTBOUND_WSS_RPC) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_WS);
		ADD_VECTOR_END(result, PT_OUTBOUND_RPC);
	}
#endif /* HAS_PROTOCOL_WS */
#endif /* HAS_PROTOCOL_RPC */
#ifdef HAS_PROTOCOL_DRM
#ifdef HAS_PROTOCOL_HTTP
	else if (name == CONF_PROTOCOL_DRM_HTTP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_DRM);
	} else if (name == CONF_PROTOCOL_DRM_HTTPS) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_OUTBOUND_HTTP);
		ADD_VECTOR_END(result, PT_DRM);
	}
#endif /* HAS_PROTOCOL_HTTP */
#endif /* HAS_PROTOCOL_DRM */
#ifdef HAS_UDS
	else if (name == CONF_PROTOCOL_UDS_RAW) {
		ADD_VECTOR_END(result, PT_UDS);
		ADD_VECTOR_END(result, PT_RAW_MEDIA);
	} else if (name == CONF_PROTOCOL_INBOUND_UDS_CLI_JSON) {
		ADD_VECTOR_END(result, PT_UDS);
		ADD_VECTOR_END(result, PT_INBOUND_JSONCLI);
	}
#endif /* HAS_UDS */
#ifdef HAS_PROTOCOL_API
	else if (name == CONF_PROTOCOL_API_RAW) {
		ADD_VECTOR_END(result, PT_API_INTEGRATION);
		ADD_VECTOR_END(result, PT_RAW_MEDIA);
	}
#endif /* HAS_PROTOCOL_API */
#ifdef HAS_PROTOCOL_WS
#ifdef HAS_PROTOCOL_WS_FMP4
	else if (name == CONF_PROTOCOL_INBOUND_WS_FMP4) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_WS);
		ADD_VECTOR_END(result, PT_INBOUND_WS_FMP4);
	} else if (name == CONF_PROTOCOL_INBOUND_WSS_FMP4) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_SSL);
		ADD_VECTOR_END(result, PT_INBOUND_WS);
		ADD_VECTOR_END(result, PT_INBOUND_WS_FMP4);
	}
#endif /* HAS_PROTOCOL_WS_FMP4 */
	else if (name == CONF_PROTOCOL_INBOUND_WS_JSON_META) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_INBOUND_WS);
		ADD_VECTOR_END(result, PT_WS_METADATA);
	}
#endif /* HAS_PROTOCOL_WS */
	else if (name == CONF_PROTOCOL_TCP_RAW) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_RAW_MEDIA);
	}
	else if (name == CONF_PROTOCOL_UDP_RAW) {
		ADD_VECTOR_END(result, PT_UDP);
		ADD_VECTOR_END(result, PT_RAW_MEDIA);
#ifdef HAS_PROTOCOL_WEBRTC
	} else if (name == CONF_PROTOCOL_WRTC_SIG) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_WRTC_SIG);
	} else if (name == CONF_PROTOCOL_WRTC_SIG_SSL) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_OUTBOUND_SSL);
		ADD_VECTOR_END(result, PT_WRTC_SIG);
	} else if (name == CONF_PROTOCOL_WRTC_CNX) {
		//TODO: Add here ICE, STUN and DTLS, 
		ADD_VECTOR_END(result, PT_WRTC_SIG);
	} else if (name == CONF_PROTOCOL_WRTC_ICE_TCP) {
		ADD_VECTOR_END(result, PT_TCP);
		ADD_VECTOR_END(result, PT_ICE_TCP);
#endif /* HAS_PROTOCOL_WEBRTC */
	} else {
		FATAL("Invalid protocol chain: %s.", STR(name));
	}
	return result;
}

BaseProtocol *DefaultProtocolFactory::SpawnProtocol(uint64_t type, Variant &parameters) {
	BaseProtocol *pResult = NULL;
	switch (type) {
		case PT_TCP:
			pResult = new TCPProtocol();
			break;
		case PT_UDP:
			pResult = new UDPProtocol();
			break;
		case PT_INBOUND_SSL:
			pResult = new InboundSSLProtocol();
			break;
		case PT_OUTBOUND_SSL:
			pResult = new OutboundSSLProtocol();
			break;
#ifdef HAS_PROTOCOL_WEBRTC
		case PT_INBOUND_DTLS:
			pResult = new InboundDTLSProtocol();
			break;
		case PT_OUTBOUND_DTLS:
			pResult = new OutboundDTLSProtocol();
			break;
#endif // HAS_PROTOCOL_WEBRTC

#ifdef HAS_PROTOCOL_RTMP
		case PT_INBOUND_RTMP:
			pResult = new InboundRTMPProtocol();
			break;
		case PT_INBOUND_RTMPS_DISC:
			pResult = new InboundRTMPSDiscriminatorProtocol();
			break;
		case PT_OUTBOUND_RTMP:
			pResult = new OutboundRTMPProtocol();
			break;
		case PT_INBOUND_HTTP_FOR_RTMP:
			pResult = new InboundHTTP4RTMP();
			break;
#endif /* HAS_PROTOCOL_RTMP */
#if defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS
		case PT_INBOUND_TS:
			pResult = new InboundTSProtocol();
			break;
#endif /* defined HAS_PROTOCOL_TS && defined HAS_MEDIA_TS */
#ifdef HAS_PROTOCOL_HTTP
		case PT_INBOUND_HTTP:
			pResult = new InboundHTTPProtocol();
			break;
		case PT_OUTBOUND_HTTP:
			pResult = new OutboundHTTPProtocol();
			break;
#endif /* HAS_PROTOCOL_HTTP */
#ifdef HAS_PROTOCOL_HTTP2
		case PT_INBOUND_HTTP2:
			pResult = new HTTPProtocol(PT_INBOUND_HTTP2);
			break;
		case PT_OUTBOUND_HTTP2:
			pResult = new HTTPProtocol(PT_OUTBOUND_HTTP2);
			break;
		case PT_HTTP_ADAPTOR:
			pResult = new HTTPAdaptorProtocol();
			break;
        case PT_HTTP_MSS_LIVEINGEST:
			pResult = new HTTPMssLiveIngest();
			break;
		case PT_HTTP_MEDIA_RECV:
			pResult = new HTTPMediaReceiver();
			break;
#endif /* HAS_PROTOCOL_HTTP2 */
#ifdef HAS_PROTOCOL_LIVEFLV
		case PT_INBOUND_LIVE_FLV:
			pResult = new InboundLiveFLVProtocol();
			break;
#endif /* HAS_PROTOCOL_LIVEFLV */
#ifdef HAS_PROTOCOL_VAR
		case PT_XML_VAR:
			pResult = new XmlVariantProtocol();
			break;
		case PT_BIN_VAR:
			pResult = new BinVariantProtocol();
			break;
		case PT_JSON_VAR:
			pResult = new JsonVariantProtocol();
			break;
#endif /* HAS_PROTOCOL_VAR */
#ifdef HAS_PROTOCOL_RTP
		case PT_RTSP:
			pResult = new RTSPProtocol();
			break;
		case PT_RTCP:
			pResult = new RTCPProtocol();
			break;
		case PT_INBOUND_RTP:
			pResult = new InboundRTPProtocol();
			break;
		case PT_RTP_NAT_TRAVERSAL:
			pResult = new NATTraversalProtocol();
			break;
#endif /* HAS_PROTOCOL_RTP */
#ifdef HAS_PROTOCOL_CLI
		case PT_INBOUND_JSONCLI:
			pResult = new InboundJSONCLIProtocol();
			break;
		case PT_HTTP_4_CLI:
			pResult = new HTTP4CLIProtocol();
			break;
#ifdef HAS_PROTOCOL_ASCIICLI
		case PT_INBOUND_ASCIICLI:
			pResult = new InboundASCIICLIProtocol();
			break;
#endif /* HAS_PROTOCOL_ASCIICLI */
#endif /* HAS_PROTOCOL_CLI */
//#ifdef HAS_PROTOCOL_HTTP4FMP4
		case PT_INBOUND_HTTP_4_FMP4:
			pResult = new HTTP4FMP4Protocol();
			break;
//#endif
#ifdef HAS_PROTOCOL_RPC
		case PT_INBOUND_RPC:
			pResult = new InboundRPCProtocol();
			break;
		case PT_OUTBOUND_RPC:
			pResult = new OutboundRPCProtocol();
			break;
#endif /* HAS_PROTOCOL_RPC */
#ifdef HAS_PROTOCOL_DRM
		case PT_DRM:
			pResult = new VerimatrixDRMProtocol();
			break;
#endif /* HAS_PROTOCOL_DRM */
#ifdef HAS_UDS
		case PT_UDS:
			pResult = new UDSProtocol();
			break;
#endif /* HAS_UDS */
#ifdef HAS_PROTOCOL_RAWMEDIA
		case PT_RAW_MEDIA:
			pResult = new RawMediaProtocol();
			break;
#endif	/* HAS_PROTOCOL_RAWMEDIA */
#ifdef HAS_PROTOCOL_WS
		case PT_INBOUND_WS:
			pResult = new InboundWSProtocol();
			break;
		case PT_OUTBOUND_WS:
			pResult = new OutboundWSProtocol();
			break;
		case PT_WS_METADATA:
			pResult = new WsMetadataProtocol();
			break;
#ifdef HAS_PROTOCOL_WS_FMP4
		case PT_INBOUND_WS_FMP4:
			pResult = new InboundWS4FMP4();
			break;
#endif /* HAS_PROTOCOL_WS_FMP4 */
#endif /* HAS_PROTOCOL_WS */
#ifdef HAS_PROTOCOL_WEBRTC
		case PT_SCTP:
			pResult = new SCTPProtocol();
			break;
		case PT_WRTC_SIG:
			pResult = new WrtcSigProtocol();
			break;
		case PT_WRTC_CNX:
			pResult = new WrtcConnection();
			break;
		case PT_ICE_TCP:
			pResult = new IceTcpProtocol();
			break;
#endif /* HAS_PROTOCOL_WEBRTC */
#ifdef HAS_PROTOCOL_API
		case PT_API_INTEGRATION:
			pResult = new ApiProtocol();
			break;
#endif /* HAS_PROTOCOL_API */
		default:
			FATAL("Spawning protocol %s not yet implemented",
					STR(tagToString(type)));
			break;
	}
	if (pResult != NULL) {
		if (!pResult->Initialize(parameters)) {
			FATAL("Unable to initialize protocol %s",
					STR(tagToString(type)));
			delete pResult;
			pResult = NULL;
		}
	}
	return pResult;
}
