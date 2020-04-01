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


#ifdef HAS_PROTOCOL_RTMP
#include "protocols/rtmp/inboundrtmpsdiscriminatorprotocol.h"
#include "protocols/http/inboundhttpprotocol.h"
#include "protocols/rtmp/inboundhttp4rtmp.h"
#include "protocols/rtmp/inboundrtmpprotocol.h"

InboundRTMPSDiscriminatorProtocol::InboundRTMPSDiscriminatorProtocol()
: BaseProtocol(PT_INBOUND_RTMPS_DISC) {

}

InboundRTMPSDiscriminatorProtocol::~InboundRTMPSDiscriminatorProtocol() {
}

bool InboundRTMPSDiscriminatorProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool InboundRTMPSDiscriminatorProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_INBOUND_SSL;
}

bool InboundRTMPSDiscriminatorProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

bool InboundRTMPSDiscriminatorProtocol::SignalInputData(int32_t recvAmount) {
	NYIR;
}

bool InboundRTMPSDiscriminatorProtocol::SignalInputData(IOBuffer &buffer) {
	//1. Do we have enough data?
	if (GETAVAILABLEBYTESCOUNT(buffer) < 6)
		return true;

	const uint8_t *pBuffer = GETIBPOINTER(buffer);

	//3. Create the proper RTMP stack
	if (memmem(pBuffer, 6, "POST /", 6) == pBuffer) {
#ifdef HAS_PROTOCOL_HTTP
		return BindHTTP(buffer);
#else
		FATAL("No HTTP protocol support");
		return false;
#endif /* HAS_PROTOCOL_HTTP */
	} else {
		return BindSSL(buffer);
	}
}

#ifdef HAS_PROTOCOL_HTTP

bool InboundRTMPSDiscriminatorProtocol::BindHTTP(IOBuffer &buffer) {
	//2. Create the HTTP4RTMP protocol
	BaseProtocol *pHTTP4RTMP = new InboundHTTP4RTMP();
	if (!pHTTP4RTMP->Initialize(GetCustomParameters())) {
		FATAL("Unable to create HTTP4RTMP protocol");
		pHTTP4RTMP->EnqueueForDelete();
		return false;
	}

	//3. Destroy the link
	BaseProtocol *pSSL = _pFarProtocol;
	pSSL->ResetNearProtocol();
	ResetFarProtocol();

	//4. Create the new links
	pSSL->SetNearProtocol(pHTTP4RTMP);
	pHTTP4RTMP->SetFarProtocol(pSSL);

	//5. Set the application
	pHTTP4RTMP->SetApplication(GetApplication());

	//6. Enqueue for delete this protocol
	EnqueueForDelete();

	//7. Process the data
	if (!pHTTP4RTMP->SignalInputData(buffer)) {
		FATAL("Unable to process data");
		pHTTP4RTMP->EnqueueForDelete();
	}

	return true;
}
#endif /* HAS_PROTOCOL_HTTP */

bool InboundRTMPSDiscriminatorProtocol::BindSSL(IOBuffer &buffer) {
	//1. Create the RTMP protocol
	BaseProtocol *pRTMP = new InboundRTMPProtocol();
	if (!pRTMP->Initialize(GetCustomParameters())) {
		FATAL("Unable to create RTMP protocol");
		pRTMP->EnqueueForDelete();
		return false;
	}

	//2. Destroy the link
	BaseProtocol *pFar = _pFarProtocol;
	pFar->ResetNearProtocol();
	ResetFarProtocol();

	//3. Create the new links
	pFar->SetNearProtocol(pRTMP);
	pRTMP->SetFarProtocol(pFar);

	//4. Set the application
	pRTMP->SetApplication(GetApplication());

	//5. Enqueue for delete this protocol
	EnqueueForDelete();

	//6. Process the data
	if (!pRTMP->SignalInputData(buffer)) {
		FATAL("Unable to process data");
		pRTMP->EnqueueForDelete();
	}

	return true;
}

#endif /* HAS_PROTOCOL_RTMP */
