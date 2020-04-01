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


#ifdef HAS_PROTOCOL_DRM

#include "protocols/drm/basedrmprotocol.h"
#include "protocols/drm/basedrmappprotocolhandler.h"
#include "protocols/http/outboundhttpprotocol.h"
#include "application/baseclientapplication.h"
#include "protocols/protocolmanager.h"

BaseDRMProtocol::BaseDRMProtocol(uint64_t type)
: BaseProtocol(type) {
	_pProtocolHandler = NULL;
}

BaseDRMProtocol::~BaseDRMProtocol() {
}

bool BaseDRMProtocol::Initialize(Variant &parameters) {
	return true;
}

void BaseDRMProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseProtocol::SetApplication(pApplication);
	if (pApplication != NULL) {
		_pProtocolHandler = (BaseDRMAppProtocolHandler *)
				pApplication->GetProtocolHandler(this);
	} else {
		_pProtocolHandler = NULL;
	}
}

IOBuffer *BaseDRMProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) == 0)
		return NULL;
	return &_outputBuffer;
}

bool BaseDRMProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_TCP
			|| type == PT_OUTBOUND_HTTP;
}

bool BaseDRMProtocol::AllowNearProtocol(uint64_t type) {
	ASSERT("This is an endpoint protocol");
	return false;
}

bool BaseDRMProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

bool BaseDRMProtocol::SignalInputData(IOBuffer &buffer) {
	if (_pProtocolHandler == NULL) {
		FATAL("This protocol is not registered to any application yet");
		return false;
	}

	if (_pFarProtocol->GetType() == PT_OUTBOUND_HTTP) {
#ifdef HAS_PROTOCOL_HTTP
		//1. This is a HTTP based transfer. We only start doing stuff
		//after a complete request is made.
		BaseHTTPProtocol *pHTTPProtocol = (BaseHTTPProtocol *) _pFarProtocol;
		if (!pHTTPProtocol->TransferCompleted())
			return true;

		_lastReceived.Reset();

		if (pHTTPProtocol->GetContentLength() > 0) {
			_headers = pHTTPProtocol->GetHeaders();
			if (!Deserialize(GETIBPOINTER(buffer), pHTTPProtocol->GetContentLength(),
					_lastReceived)) {
				string stringContent = string((char *) GETIBPOINTER(buffer), pHTTPProtocol->GetContentLength());
				FATAL("Unable to deserialize variant content:\n%s", STR(stringContent));
				return false;
			}
			_lastReceived.Compact();

			if (_headers.HasKeyChain(V_STRING, false, 2, "headers", "Location"))
				_lastReceived["location"] = (string) _headers["headers"]["Location"];
			else
				_lastReceived["location"] = "";
		}

		buffer.Ignore(pHTTPProtocol->GetContentLength());

		return _pProtocolHandler->ProcessMessage(this, _lastSent, _lastReceived);
#else
		FATAL("HTTP protocol not supported");
		return false;
#endif /* HAS_PROTOCOL_HTTP */
	} else if (_pFarProtocol->GetType() == PT_TCP) {
		while (GETAVAILABLEBYTESCOUNT(buffer) > 4) {
			uint32_t size = ENTOHLP(GETIBPOINTER(buffer));
			if (size > 4 * 1024 * 1024) {
				FATAL("Size too big: %u", size);
				return false;
			}
			if (GETAVAILABLEBYTESCOUNT(buffer) < size + 4) {
				FINEST("Need more data");
				return true;
			}

			_lastReceived.Reset();

			if (size > 0) {
				if (!Deserialize(GETIBPOINTER(buffer) + 4, size, _lastReceived)) {
					FATAL("Unable to deserialize variant");
					return false;
				}
				_lastReceived.Compact();
			}
			buffer.Ignore(size + 4);

			if (!_pProtocolHandler->ProcessMessage(this, _lastSent, _lastReceived)) {
				FATAL("Unable to process message");
				return false;
			}
		}
		return true;
	} else {
		FATAL("Invalid protocol stack");
		return false;
	}
}

bool BaseDRMProtocol::Send(Variant &variant) {
	//1. Do we have a protocol?
	if (_pFarProtocol == NULL) {
		FATAL("This protocol is not linked");
		return false;
	}

	//2. Save the variant
	_lastSent = variant;

	//3. Depending on the far protocol, we do different stuff
	string rawContent = "";
	switch (_pFarProtocol->GetType()) {
		case PT_TCP:
		{
			//5. Serialize it
			if (!Serialize(rawContent, variant)) {
				FATAL("Unable to serialize variant");
				return false;
			}

			_outputBuffer.ReadFromRepeat(0, 4);
			uint32_t rawContentSize = (uint32_t) rawContent.size();
			EHTONLP(
					GETIBPOINTER(_outputBuffer) + GETAVAILABLEBYTESCOUNT(_outputBuffer) - 4, //head minus 4 bytes
					rawContentSize
					);
			_outputBuffer.ReadFromString(rawContent);

			//6. enqueue for outbound
			if (!EnqueueForOutbound()) {
				FATAL("Unable to enqueue for outbound");
				return false;
			}
			return true;
		}
		case PT_OUTBOUND_HTTP:
		{
#ifdef HAS_PROTOCOL_HTTP
			//7. This is a HTTP request. So, first things first: get the http protocol
			OutboundHTTPProtocol *pHTTP = (OutboundHTTPProtocol *) _pFarProtocol;

			//8. We wish to disconnect after the transfer is complete
			pHTTP->SetDisconnectAfterTransfer(true);

			//9. Use GET instead of POST
			pHTTP->Method(HTTP_METHOD_GET);

			//10. Our document and the host
			pHTTP->Document(variant["document"]);
			pHTTP->Host(variant["host"]);

			//11. Serialize it
			if (!Serialize(rawContent, variant["payload"])) {
				FATAL("Unable to serialize variant");
				return false;
			}

			_outputBuffer.ReadFromString(rawContent);

			//12. enqueue for outbound
			return EnqueueForOutbound();
#else
			FATAL("HTTP protocol not supported");
			return false;
#endif /* HAS_PROTOCOL_HTTP */
		}
		default:
		{
			ASSERT("We should not be here");
			return false;
		}
	}
}

#endif	/* HAS_PROTOCOL_DRM */
