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
#ifdef HAS_PROTOCOL_WEBRTC
#include "protocols/wrtc/icetcpprotocol.h"
#include "protocols/wrtc/wrtcsigprotocol.h"
#include "protocols/wrtc/stunmsg.h"
#include "protocols/wrtc/candidate.h"
#include "protocols/wrtc/wrtcconnection.h"
#include "netio/netio.h"
#include "protocols/protocolmanager.h"

IceTcpProtocol::IceTcpProtocol() 
: BaseIceProtocol(PT_ICE_TCP) {
	_senderProtocol = NULL;
	_isTcp = true;
}

IceTcpProtocol::~IceTcpProtocol() {
//#ifdef WEBRTC_DEBUG
	FINE("IceTcpProtocol deleted.");
//#endif
/*
 * Seems like the protocol chain stack already cleans this up
	if (_senderProtocol) {
		_senderProtocol->EnqueueForDelete();
	}
	_senderProtocol = NULL;
*/
}

void IceTcpProtocol::Start(time_t ms) {
//#ifdef WEBRTC_DEBUG
	DEBUG("[ICE-TCP] Start called for host: %s", STR(_bindIp));
//#endif

	// We need to set this to started first, otherwise we'll block the progression of 
	// candidate selection if something goes wrong here
	_started = true;
	
	if (_turnServerIpStr.empty()) {
		// We don't have any TURN server
		FATAL("[ICE-TCP] No available TURN server.");
		_state |= ST_DEAD;
		return;
	}

	// Attempt to connect to TURN server
	vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
			CONF_PROTOCOL_TCP_PASSTHROUGH);
	if (chain.size() == 0) {
		FATAL("[ICE-TCP] Unable to resolve protocol chain");
		_state |= ST_DEAD;
		return;
	}

	Variant &params = GetCustomParameters();
	params["iceTcpProtocolId"] = GetId();
	
	// Get the ip and port fields
	string ip = _turnServerAddress.getIPv4();
	uint16_t port = _turnServerAddress.getSockPort();
	INFO("[ICE-TCP] Connecting to %s:%" PRIu16"...", STR(ip), port);

	//UPDATE: At this point, we already should have an IP address
	//string resolvedIP = getHostByName(ip);
	if (!TCPConnector<IceTcpProtocol>::Connect(ip, port, chain, params)) {
		FATAL("[ICE-TCP] Unable to connect to %s:%" PRIu16, STR(ip), port);
		_state |= ST_DEAD;
		return;
	}
}

bool IceTcpProtocol::SignalProtocolCreated(BaseProtocol *pProtocol,
		Variant &parameters) {
//#ifdef WEBRTC_DEBUG
	DEBUG("[ICE-TCP] SignalProtocolCreated : ");
//#endif
	IceTcpProtocol *pIceTcpProtocol =
			(IceTcpProtocol *) ProtocolManager::GetProtocol(
			(uint32_t) parameters["iceTcpProtocolId"]);

	if (pIceTcpProtocol == NULL) {
		// Corresponding ice protocol not found
		FATAL("[ICE-TCP] Protocol not found.");
		return false;
	}

	return pIceTcpProtocol->SignalPassThroughProtocolCreated((PassThroughProtocol *) pProtocol, parameters);
}

bool IceTcpProtocol::SignalPassThroughProtocolCreated(PassThroughProtocol *pProtocol,
		Variant &parameters) {
//#ifdef WEBRTC_DEBUG
	DEBUG("[ICE-TCP] SignalPassThroughProtocolCreated");
//#endif

	if (pProtocol == NULL) {
		FATAL("[ICE-TCP] Connection failed to TURN server! Either it is down or blocked.");
		// There will be cases that RMS would actually fail to connect to the
		// TURN server (either it is down or the port is blocked.
		// We'll not enqueue for delete, but attempt to do a retry and/or see
		// if there are other TURN servers that can be used.
		// UPDATE: we can do a safe enqueue for deletion because of the fixes
		Terminate(false);
		return false;
	}
	
	_senderProtocol = (PassThroughProtocol *) pProtocol;
	_senderProtocol->SetNearProtocol(this);

	TCPCarrier * carrier = (TCPCarrier *)_senderProtocol->GetIOHandler();

	if (carrier == NULL) {
		FATAL("[ICE-TCP] Carrier is NULL!");
		return false;
	}

	_bindIp = carrier->GetNearEndpointAddressIp();
	_bindPort = carrier->GetNearEndpointPort();
	_bindAddress.setIPAddress(_bindIp, _bindPort);
	_hostIpStr = _bindAddress.getIPwithPort();
//#ifdef WEBRTC_DEBUG
	DEBUG("[ICE-TCP] host: %s", STR(_hostIpStr));
//#endif
	//TODO: is this needed?
	// Disable for now, will get back for full implementation of ICE-TCP

	_hostCan = Candidate::CreateHost(_hostIpStr);
	_hostCan->SetTransport(false); //TODO: for testing purposes only
	_pSig->SendCandidate(_hostCan);
	_hostCan->SetStatePierceSent();
	
	bool result;
	FastTick(true, result);	// kick send to Stun server

	return true;
}

bool IceTcpProtocol::SignalInputData(IOBuffer &buffer) {
	//TODO: for now, this is directly linked to the TURN server

	// first see if it is a Stun Message
	if ( StunMsg::IsStun(buffer) ) {
		//UPDATE: loop through the messages in case that it is a combination
		// of responses
		do {
			// create a StunMsg object!
			StunMsg * pMsg = StunMsg::ParseBuffer(buffer, _turnServerIpStr);
			int type = pMsg->GetType();
			int err = pMsg->GetErrorCodeAttr();
			HandleServerMsg(pMsg, type, err, buffer);

			// Skip the complete message
			if (!buffer.Ignore(pMsg->GetMessageLen())) {
				//UPDATE: Just encountered a critical bug where message length
				// can be corrupted/invalid.
				// Remove everything if this is encountered
				WARN("Corrupted STUN packet.");
				buffer.IgnoreAll();
			}
			delete pMsg;
		} while (GETAVAILABLEBYTESCOUNT(buffer) > StunMsg::STUN_HEADER_LEN);
		
		return true;
	} else {	// Not STUN
		// Bubble up any non-STUN messages to the upper protocol
		if (_pNearProtocol != NULL) {
//#ifdef WEBRTC_DEBUG
			DEBUG("Near protocol: %s", STR(tagToString(_pNearProtocol->GetType())));
//#endif
			_pNearProtocol->SignalInputData(buffer);//TODO
		}
	}
	
	buffer.IgnoreAll();
	
	return true;
}

bool IceTcpProtocol::SendToV4(uint32_t ip, uint16_t port, uint8_t * pData, int len) {
	if (ip != _turnServerIp) {
		// Only bind/communicate with the TURN server for now
//#ifdef WEBRTC_DEBUG
		FINE("[ICE-TCP] Not binding to any endpoint other than TURN server.");
//#endif
		return true;
	}

	if (_senderProtocol == NULL) {
		WARN("[ICE-TCP] Sender is not initialized.");
		return true;
	}

	return _senderProtocol->SendTCPData(pData, len);
}

bool IceTcpProtocol::SendTo(string ip, uint16_t port, uint8_t * pData, int len) {
	if (ip != _turnServerAddress.getIPv4()) {
		// Only bind/communicate with the TURN server for now
//#ifdef WEBRTC_DEBUG
		FINE("[ICE-TCP] Not binding to any endpoint other than TURN server. Tried to bind to %s", STR(ip));
//#endif
		return true;
	}

	if (_senderProtocol == NULL) {
		WARN("[ICE-TCP] Sender is not initialized.");
		return true;
	}

	return _senderProtocol->SendTCPData(pData, len);
}
#endif // HAS_PROTOCOL_WEBRTC
