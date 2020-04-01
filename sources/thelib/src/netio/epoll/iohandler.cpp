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


#ifdef NET_EPOLL
#include "netio/epoll/iohandler.h"
#include "netio/epoll/iohandlermanager.h"
#include "protocols/baseprotocol.h"

uint32_t IOHandler::_idGenerator = 0;

IOHandler::IOHandler(int32_t inboundFd, int32_t outboundFd, IOHandlerType type) {
	_pProtocol = NULL;
	_type = type;
	_id = ++_idGenerator;
	_inboundFd = inboundFd;
	_outboundFd = outboundFd;
	_pToken = NULL;
	IOHandlerManager::RegisterIOHandler(this);
//	_pEvtLog = NULL;
}

IOHandler::~IOHandler() {
	if (_pProtocol != NULL) {
		_pProtocol->SetIOHandler(NULL);
		_pProtocol->EnqueueForDelete();
		_pProtocol = NULL;
	}
	IOHandlerManager::UnRegisterIOHandler(this);
}

void IOHandler::SetIOHandlerManagerToken(IOHandlerManagerToken *pToken) {
	_pToken = pToken;
}

IOHandlerManagerToken * IOHandler::GetIOHandlerManagerToken() {
	return _pToken;
}

uint32_t IOHandler::GetId() {
	return _id;
}

int32_t IOHandler::GetInboundFd() {
	return _inboundFd;
}

int32_t IOHandler::GetOutboundFd() {
	return _outboundFd;
}

IOHandlerType IOHandler::GetType() {
	return _type;
}

void IOHandler::SetProtocol(BaseProtocol *pPotocol) {
	_pProtocol = pPotocol;
}

BaseProtocol *IOHandler::GetProtocol() {
	return _pProtocol;
}

string IOHandler::IOHTToString(IOHandlerType type) {
	switch (type) {
		case IOHT_ACCEPTOR:
			return "IOHT_ACCEPTOR";
		case IOHT_TCP_CARRIER:
			return "IOHT_TCP_CARRIER";
		case IOHT_UDP_CARRIER:
			return "IOHT_UDP_CARRIER";
		case IOHT_TCP_CONNECTOR:
			return "IOHT_TCP_CONNECTOR";
		case IOHT_TIMER:
			return "IOHT_TIMER";
		case IOHT_INBOUNDNAMEDPIPE_CARRIER:
			return "IOHT_INBOUNDNAMEDPIPE_CARRIER";
#ifdef HAS_UDS
		case IOHT_UDS_ACCEPTOR:
			return "IOHT_UDS_ACCEPTOR";
		case IOHT_UDS_CARRIER:
			return "IOHT_UDS_CARRIER";
#endif /* HAS_UDS */
#ifdef HAS_PROTOCOL_API
		case IOHT_API_FD:
			return "IOHT_API_FD";
#endif /* HAS_PROTOCOL_API */	
		default:
			return format("#unknown: %hhu#", type);
	}
}
//void IOHandler::SetEventLogger(EventLogger *pEvtLogger) {
//	_pEvtLog = pEvtLogger;
//}
#endif /* NET_EPOLL */


