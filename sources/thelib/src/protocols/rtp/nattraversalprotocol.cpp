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


#ifdef HAS_PROTOCOL_RTP
#include "protocols/rtp/nattraversalprotocol.h"
#include "protocols/rtp/connectivity/outboundconnectivity.h"

NATTraversalProtocol::NATTraversalProtocol()
: BaseProtocol(PT_RTP_NAT_TRAVERSAL) {
	_pOutboundAddress = NULL;
}

NATTraversalProtocol::~NATTraversalProtocol() {
}

bool NATTraversalProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	return true;
}

bool NATTraversalProtocol::AllowFarProtocol(uint64_t type) {
	return type == PT_UDP;
}

bool NATTraversalProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

bool NATTraversalProtocol::SignalInputData(int32_t recvAmount) {
	NYIR;
}

bool NATTraversalProtocol::SignalInputData(IOBuffer &buffer) {
	NYIR;
}

bool NATTraversalProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	//FINEST("_inputBuffer:\n%s", STR(buffer));
	buffer.IgnoreAll();
	if (_pOutboundAddress == NULL)
		return true;
	
	// Check if the outbound address is not the same as the peer address
	if ( _pOutboundAddress->getIPv4() != pPeerAddress->getIPv4() ) {
//	if (_pOutboundAddress->sin_addr.s_addr != pPeerAddress->sin_addr.s_addr) {
		//WARN("Attempt to divert traffic. DoS attack!?");
		return true;
	}
	//string ipAddress = inet_ntoa(_pOutboundAddress->sin_addr);
	//	if (_pOutboundAddress->sin_port == pPeerAddress->sin_port) {
	//		FINEST("The client has public endpoint: %s:%"PRIu16,
	//				STR(ipAddress),
	//				ENTOHS(_pOutboundAddress->sin_port));
	//	} else {
	//		FINEST("The client is behind firewall: %s:%"PRIu16" -> %s:%"PRIu16,
	//				STR(ipAddress),
	//				ENTOHS(_pOutboundAddress->sin_port),
	//				STR(ipAddress),
	//				ENTOHS(pPeerAddress->sin_port));
	//		_pOutboundAddress->sin_port = pPeerAddress->sin_port;
	//	}
	
	// Now, check if the ports are equal, if not, the make them equal!
	if (_pOutboundAddress->getSockPort() == pPeerAddress->getSockPort() ) {
		_pOutboundAddress->setSockPort(pPeerAddress->getSockPort());
	}
//	if (_pOutboundAddress->sin6_port != pPeerAddress->sin6_port)
//		_pOutboundAddress->sin6_port = pPeerAddress->sin6_port;
	_pOutboundAddress = NULL;
	return true;
}

void NATTraversalProtocol::SetOutboundAddress(SocketAddress *pOutboundAddress) {
	_pOutboundAddress = pOutboundAddress;
}

#endif /* HAS_PROTOCOL_RTP */
