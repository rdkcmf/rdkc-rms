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


#include "session.h"
#include "protocols/baseprotocol.h"
#include "protocols/protocolmanager.h"
#include "rawtcpprotocol.h"
#include "dissector/rtmptrafficdissector.h"
using namespace app_livertmpdissector;

uint32_t Session::_idGenerator = 1;
map<uint32_t, Session *> Session::_allSessions;

Session::Session(Variant &parameters, BaseClientApplication *pApp) {
	_id = _idGenerator++;
	_allSessions[_id] = this;
	_inboundProtocolId = 0;
	_outboundProtocolId = 0;
	_pDissector = new RTMPTrafficDissector();
	assert(_pDissector->Init(parameters, pApp));
}

Session * Session::GetSession(uint32_t id) {
	map<uint32_t, Session *>::iterator i = _allSessions.find(id);
	if (i == _allSessions.end())
		return NULL;
	return MAP_VAL(i);
}

Session::~Session() {
	_allSessions.erase(_id);
	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(_inboundProtocolId);
	if (pProtocol != NULL)
		pProtocol->EnqueueForDelete();

	pProtocol = ProtocolManager::GetProtocol(_outboundProtocolId);
	if (pProtocol != NULL)
		pProtocol->EnqueueForDelete();
	if (_pDissector != NULL) {
		delete _pDissector;
		_pDissector = NULL;
	}
}

uint32_t Session::GetId() {
	return _id;
}

uint32_t Session::InboundProtocolId() {
	return _inboundProtocolId;
}

void Session::InboundProtocolId(BaseProtocol *pProtocol) {
	assert(_inboundProtocolId == 0);
	assert(pProtocol != NULL);
	_inboundProtocolId = pProtocol->GetId();
}

uint32_t Session::OutboundProtocolId() {
	return _outboundProtocolId;
}

void Session::OutboundProtocolId(BaseProtocol *pProtocol) {
	assert(_outboundProtocolId == 0);
	assert(pProtocol != NULL);
	_outboundProtocolId = pProtocol->GetId();
	RawTcpProtocol *pTo = (RawTcpProtocol *) pProtocol;
	if (!(pTo->FeedData(_tempIOBuffer) && _tempIOBuffer.IgnoreAll()))
		pTo->EnqueueForDelete();
}

bool Session::FeedData(IOBuffer &buffer, BaseProtocol *pFrom) {
	_pDissector->FeedData(buffer, pFrom->GetId() == _inboundProtocolId);
	if (_outboundProtocolId == 0) {
		return _tempIOBuffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer))
				&& buffer.IgnoreAll();
	}
	RawTcpProtocol *pTo = (RawTcpProtocol *) (pFrom->GetId() == _inboundProtocolId ?
			ProtocolManager::GetProtocol(_outboundProtocolId)
			: ProtocolManager::GetProtocol(_inboundProtocolId));
	if (pTo == NULL)
		return false;

	return pTo->FeedData(buffer) && buffer.IgnoreAll();
}
