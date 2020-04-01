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


#ifdef NET_IOCP
#include "netio/iocp/iohandler.h"
#include "netio/iocp/iohandlermanager.h"
#include "protocols/baseprotocol.h"

uint32_t IOHandler::_idGenerator = 0;

IOHandler::IOHandler(SOCKET inboundFd, SOCKET outboundFd, IOHandlerType type) {
	_pProtocol = NULL;
	_type = type;
	_id = ++_idGenerator;
	_inboundFd = inboundFd;
	_outboundFd = outboundFd;
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

uint32_t IOHandler::GetId() {
	return _id;
}

SOCKET IOHandler::GetInboundFd() {
	return _inboundFd;
}

SOCKET IOHandler::GetOutboundFd() {
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
		case IOHT_STDIO:
			return "IOHT_STDIO";
		default:
			return format("#unknown: %hhu#", type);
	}
}

//void IOHandler::SetEventLogger(EventLogger *pEvtLogger) {
//	_pEvtLog = pEvtLogger;
//}
#endif /* NET_IOCP */
