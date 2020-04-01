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


#include "protocols/baseprotocol.h"
#include "netio/netio.h"
#include "protocols/protocolmanager.h"
#include "application/baseclientapplication.h"
#include "protocols/tcpprotocol.h"
#include "eventlogger/eventlogger.h"
#include "application/clientapplicationmanager.h"
#include "utils/misc/socketaddress.h"

//#define LOG_CONSTRUCTOR_DESTRUCTOR

uint32_t BaseProtocol::_idGenerator = 0;

BaseProtocol::BaseProtocol(uint64_t type) {
	_id = ++_idGenerator;
	_type = type;
	_pFarProtocol = NULL;
	_pNearProtocol = NULL;
	_deleteFar = true;
	_deleteNear = true;
	_enqueueForDelete = false;
	_gracefullyEnqueueForDelete = false;
	_pApplication = NULL;
#ifdef LOG_CONSTRUCTOR_DESTRUCTOR
	FINEST("Protocol with id %"PRIu32" of type %s created; F: %p,N: %p, DF: %d, DN: %d",
			_id, STR(tagToString(_type)),
			_pFarProtocol, _pNearProtocol, _deleteFar, _deleteNear);
#endif
	ProtocolManager::RegisterProtocol(this);
	GETCLOCKS(_creationTimestamp, double);
	_creationTimestamp /= (double) CLOCKS_PER_SECOND;
	_creationTimestamp *= 1000.00;
	_lastKnownApplicationId = 0;
}

BaseProtocol::~BaseProtocol() {
#ifdef LOG_CONSTRUCTOR_DESTRUCTOR
	FINEST("Protocol with id %"PRIu32"(%p) of type %s going to be deleted; F: %p,N: %p, DF: %d, DN: %d",
			_id,
			this,
			STR(tagToString(_type)),
			_pFarProtocol,
			_pNearProtocol,
			_deleteFar,
			_deleteNear);
#endif
	BaseProtocol *pFar = _pFarProtocol;
	BaseProtocol *pNear = _pNearProtocol;

	_pFarProtocol = NULL;
	_pNearProtocol = NULL;
	if (pFar != NULL) {
		pFar->_pNearProtocol = NULL;
		if (_deleteFar) {
			pFar->EnqueueForDelete();
		}
	}
	if (pNear != NULL) {
		pNear->_pFarProtocol = NULL;
		if (_deleteNear) {
			pNear->EnqueueForDelete();
		}
	}
#ifdef LOG_CONSTRUCTOR_DESTRUCTOR
	FINEST("Protocol with id %"PRIu32"(%p) of type %s deleted; F: %p,N: %p, DF: %d, DN: %d",
			_id,
			this,
			STR(tagToString(_type)),
			_pFarProtocol,
			_pNearProtocol,
			_deleteFar,
			_deleteNear);
#endif
	ProtocolManager::UnRegisterProtocol(this);
}

uint64_t BaseProtocol::GetType() {
	return _type;
}

uint32_t BaseProtocol::GetId() {
	return _id;
}

double BaseProtocol::GetSpawnTimestamp() {
	return _creationTimestamp;
}

BaseProtocol *BaseProtocol::GetFarProtocol() {
	return _pFarProtocol;
}

void BaseProtocol::SetFarProtocol(BaseProtocol *pProtocol) {
	if (!AllowFarProtocol(pProtocol->_type)) {
		ASSERT("Protocol %s can't accept a far protocol of type: %s",
				STR(tagToString(_type)),
				STR(tagToString(pProtocol->_type)));
	}
	if (!pProtocol->AllowNearProtocol(_type)) {
		ASSERT("Protocol %s can't accept a near protocol of type: %s",
				STR(tagToString(pProtocol->_type)),
				STR(tagToString(_type)));
	}
	if (_pFarProtocol == NULL) {
		_pFarProtocol = pProtocol;
		pProtocol->SetNearProtocol(this);
#ifdef LOG_CONSTRUCTOR_DESTRUCTOR
		FINEST("Protocol with id %"PRIu32" of type %s setted up; F: %p,N: %p, DF: %d, DN: %d",
				_id, STR(tagToString(_type)),
				_pFarProtocol, _pNearProtocol, _deleteFar, _deleteNear);
#endif
	} else {
		if (_pFarProtocol != pProtocol) {
			ASSERT("Far protocol already present");
		}
	}
}

void BaseProtocol::ResetFarProtocol() {
	if (_pFarProtocol != NULL) {
		_pFarProtocol->_pNearProtocol = NULL;
	}
	_pFarProtocol = NULL;
}

BaseProtocol *BaseProtocol::GetNearProtocol() {
	return _pNearProtocol;
}

void BaseProtocol::SetNearProtocol(BaseProtocol *pProtocol) {
	if (!AllowNearProtocol(pProtocol->_type)) {
		ASSERT("Protocol %s can't accept a near protocol of type: %s",
				STR(tagToString(_type)),
				STR(tagToString(pProtocol->_type)));
	}
	if (!pProtocol->AllowFarProtocol(_type)) {
		ASSERT("Protocol %s can't accept a far protocol of type: %s",
				STR(tagToString(pProtocol->_type)),
				STR(tagToString(_type)));
	}
	if (_pNearProtocol == NULL) {
		_pNearProtocol = pProtocol;
		pProtocol->SetFarProtocol(this);
#ifdef LOG_CONSTRUCTOR_DESTRUCTOR
		FINEST("Protocol with id %"PRIu32" of type %s setted up; F: %p,N: %p, DF: %d, DN: %d",
				_id, STR(tagToString(_type)),
				_pFarProtocol, _pNearProtocol, _deleteFar, _deleteNear);
#endif
	} else {
		if (_pNearProtocol != pProtocol) {
			ASSERT("Near protocol already present");
		}
	}
}

void BaseProtocol::ResetNearProtocol() {
	if (_pNearProtocol != NULL)
		_pNearProtocol->_pFarProtocol = NULL;
	_pNearProtocol = NULL;
}

void BaseProtocol::DeleteNearProtocol(bool deleteNear) {
	_deleteNear = deleteNear;
}

void BaseProtocol::DeleteFarProtocol(bool deleteFar) {
	_deleteFar = deleteFar;
}

BaseProtocol *BaseProtocol::GetFarEndpoint() {
	if (_pFarProtocol == NULL) {
		return this;
	} else {
		return _pFarProtocol->GetFarEndpoint();
	}
}

BaseProtocol *BaseProtocol::GetNearEndpoint() {
	if (_pNearProtocol == NULL)
		return this;
	else
		return _pNearProtocol->GetNearEndpoint();
}

void BaseProtocol::SetWitnessFile(string path) {

}

void BaseProtocol::EnqueueForDelete() {
	if (_enqueueForDelete)
		return;
	_enqueueForDelete = true;
	ProtocolManager::EnqueueForDelete(this);
}

void BaseProtocol::GracefullyEnqueueForDelete(bool fromFarSide) {
	_gracefullyEnqueueForDelete = true;

	if (fromFarSide)
		return GetFarEndpoint()->GracefullyEnqueueForDelete(false);

	if (GetOutputBuffer() != NULL) {
		return;
	}

	if (_pNearProtocol != NULL) {
		_pNearProtocol->GracefullyEnqueueForDelete(false);
	} else {
		EnqueueForDelete();
	}
}

bool BaseProtocol::IsEnqueueForDelete() {
	return _enqueueForDelete || _gracefullyEnqueueForDelete;

}

BaseClientApplication * BaseProtocol::GetApplication() {
	return _pApplication;
}

BaseClientApplication * BaseProtocol::GetLastKnownApplication() {
	if (_pApplication != NULL)
		return _pApplication;
	return ClientApplicationManager::FindAppById(_lastKnownApplicationId);
}

void BaseProtocol::SetOutboundConnectParameters(Variant &customParameters) {
	_customParameters = customParameters;
}

void BaseProtocol::GetStackStats(Variant &info, uint32_t namespaceId) {
	IOHandler *pIOHandler = GetIOHandler();
	if (pIOHandler != NULL) {
		pIOHandler->GetStats(info["carrier"], namespaceId);
	} else {
		info["carrier"] = Variant();
	}
	BaseProtocol *pTemp = GetFarEndpoint();
	while (pTemp != NULL) {
		Variant item;
		pTemp->GetStats(item, namespaceId);
		info["stack"].PushToArray(item);
		pTemp = pTemp->GetNearProtocol();
	}
}

Variant &BaseProtocol::GetCustomParameters() {
	return _customParameters;
}

BaseProtocol::operator string() {
	string result = "";
	IOHandler *pHandler = NULL;
	if ((pHandler = GetIOHandler()) != NULL) {
		switch (pHandler->GetType()) {
			case IOHT_ACCEPTOR:
				result = format("A(%d) <-> ", pHandler->GetInboundFd());
				break;
			case IOHT_TCP_CARRIER:
				result = format("(Far: %s:%"PRIu16"; Near: %s:%"PRIu16") CTCP(%d) <-> ",
						STR(((TCPCarrier *) pHandler)->GetFarEndpointAddressIp()),
						((TCPCarrier *) pHandler)->GetFarEndpointPort(),
						STR(((TCPCarrier *) pHandler)->GetNearEndpointAddressIp()),
						((TCPCarrier *) pHandler)->GetNearEndpointPort(),
						pHandler->GetInboundFd());
				break;
			case IOHT_UDP_CARRIER:
				result = format("(Bound on: %s:%"PRIu16") CUDP(%d) <-> ",
						STR(((UDPCarrier *) pHandler)->GetNearEndpointAddress()),
						((UDPCarrier *) pHandler)->GetNearEndpointPort(),
						pHandler->GetInboundFd());
				break;
			case IOHT_TCP_CONNECTOR:
				result = format("CO(%d) <-> ", pHandler->GetInboundFd());
				break;
			case IOHT_TIMER:
				result = format("T(%d) <-> ", pHandler->GetInboundFd());
				break;
			case IOHT_STDIO:
				result = format("STDIO <-> ");
				break;
#ifdef HAS_UDS
			case IOHT_UDS_ACCEPTOR:
				result = ((UDSAcceptor *)pHandler)->ToString() + " <-> ";
				break;
			case IOHT_UDS_CARRIER:
				result = ((UDSCarrier *)pHandler)->ToString() + " <-> ";
				break;
#endif /* HAS_UDS */
#ifdef HAS_PROTOCOL_API
			case IOHT_API_FD:
				result = "API <-> ";
				break;
#endif /* HAS_PROTOCOL_API */
			default:
				result = format("#unknown %hhu#(%d,%d) <-> ",
						pHandler->GetType(),
						pHandler->GetInboundFd(),
						pHandler->GetOutboundFd());
				break;
		}
	}
	BaseProtocol *pTemp = GetFarEndpoint();
	while (pTemp != NULL) {
		result += pTemp->ToString(_id);
		pTemp = pTemp->_pNearProtocol;
		if (pTemp != NULL)
			result += " <-> ";
	}
	return result;
}

bool BaseProtocol::Initialize(Variant &parameters) {
	WARN("You should override bool BaseProtocol::Initialize(Variant &parameters) on protocol %s",
			STR(tagToString(_type)));
	_customParameters = parameters;
	return true;
}

IOHandler *BaseProtocol::GetIOHandler() {
	if (_pFarProtocol != NULL)
		return _pFarProtocol->GetIOHandler();
	return NULL;
}

void BaseProtocol::SetIOHandler(IOHandler *pCarrier) {
	if (_pFarProtocol != NULL)
		_pFarProtocol->SetIOHandler(pCarrier);
}

IOBuffer * BaseProtocol::GetInputBuffer() {
	if (_pFarProtocol != NULL)
		return _pFarProtocol->GetInputBuffer();
	return NULL;
}

IOBuffer * BaseProtocol::GetOutputBuffer() {
	if (_pNearProtocol != NULL)
		return _pNearProtocol->GetOutputBuffer();
	return NULL;
}

SocketAddress* BaseProtocol::GetDestInfo() {
	if (_pNearProtocol != NULL)
		return _pNearProtocol->GetDestInfo();
	return NULL;
}

uint64_t BaseProtocol::GetDecodedBytesCount() {
	if (_pFarProtocol != NULL)
		return _pFarProtocol->GetDecodedBytesCount();
	return 0;
}

bool BaseProtocol::EnqueueForOutbound() {
	if (_pFarProtocol != NULL)
		return _pFarProtocol->EnqueueForOutbound();
	return true;
}

bool BaseProtocol::EnqueueForTimeEvent(uint32_t seconds) {
	if (_pFarProtocol != NULL)
		return _pFarProtocol->EnqueueForTimeEvent(seconds);
	return true;
}

bool BaseProtocol::TimePeriodElapsed() {
	if (_pNearProtocol != NULL)
		return _pNearProtocol->TimePeriodElapsed();
	return true;
}

void BaseProtocol::ReadyForSend() {
	if (_gracefullyEnqueueForDelete) {
		EnqueueForDelete();
		return;
	}
	if (_pNearProtocol != NULL)
		_pNearProtocol->ReadyForSend();
}

void BaseProtocol::SetApplication(BaseClientApplication *pApplication) {
	//1. Get the old and the new application name and id
	string oldAppName = "(none)";
	uint32_t oldAppId = 0;
	string newAppName = "(none)";
	uint32_t newAppId = 0;
	if (_pApplication != NULL) {
		oldAppName = _pApplication->GetName();
		oldAppId = _pApplication->GetId();
	}
	if (pApplication != NULL) {
		newAppName = pApplication->GetName();
		newAppId = pApplication->GetId();
	}

	//2. Are we landing on the same application?
	if (oldAppId == newAppId) {
		return;
	}

	//3. If the application is the same, return. Otherwise, unregister
	if (_pApplication != NULL) {
		_pApplication->UnRegisterProtocol(this);
		_pApplication = NULL;
	}

	//4. Setup the new application
	_pApplication = pApplication;

	//5. Register to it
	if (_pApplication != NULL) {
		_lastKnownApplicationId = _pApplication->GetId();
		_pApplication->RegisterProtocol(this);
	}

	//6. Trigger log to production
}

bool BaseProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	WARN("This should be overridden. Protocol type is %s", STR(tagToString(_type)));
	return SignalInputData(buffer);
}

bool BaseProtocol::SignalInputData(IOBuffer &buffer, uint32_t rawBufferLength, SocketAddress *pPeerAddress) {
	WARN("This should be overridden. Protocol type is %s", STR(tagToString(_type)));
	NYIR;
}

bool BaseProtocol::SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress) {
	WARN("This should be overridden: %s", STR(tagToString(_type)));
	return SignalInputData(recvAmount);
}

void BaseProtocol::GetStats(Variant &info, uint32_t namespaceId) {
	info["id"] = (((uint64_t) namespaceId) << 32) | GetId();
	info["type"] = tagToString(_type);
	info["creationTimestamp"] = _creationTimestamp;
	double queryTimestamp = 0;
	GETCLOCKS(queryTimestamp, double);
	queryTimestamp /= (double) CLOCKS_PER_SECOND;
	queryTimestamp *= 1000.00;
	info["queryTimestamp"] = queryTimestamp;
	info["isEnqueueForDelete"] = (bool)IsEnqueueForDelete();
	if (_pApplication != NULL)
		info["applicationId"] = (((uint64_t) namespaceId) << 32) | _pApplication->GetId();
	else
		info["applicationId"] = (((uint64_t) namespaceId) << 32);
	if (_customParameters.HasKeyChain(V_STRING, false, 1, "userAgent"))
		info["userAgent"] = _customParameters["userAgent"];
	if (_customParameters.HasKeyChain(V_STRING, false, 1, "serverAgent"))
		info["serverAgent"] = _customParameters["serverAgent"];
}

bool BaseProtocol::SendOutOfBandData(const IOBuffer &buffer, void *pUserData) {
	FATAL("Protocol %s does not support this operation", STR(*this));
	return false;
}

void BaseProtocol::SignalOutOfBandDataEnd(void *pUserData) {
	EnqueueForDelete();
}

void BaseProtocol::SignalEvent(string eventName, Variant &args) {
	// Do nothing. Let the derived classes implement this
}
#ifdef HAS_PROTOCOL_HTTP2

HTTPInterface* BaseProtocol::GetHTTPInterface() {
	WARN("BaseProtocol::GetHTTPInterface must be overridden on protocols accepting HTTP as transport. Current protocol is %s", STR(tagToString(_type)));
	return NULL;
}
#endif /* HAS_PROTOCOL_HTTP2 */

string BaseProtocol::ToString(uint32_t currentId) {
	string result = "";
	if (_id == currentId)
		result = format("[%s(%u)]", STR(tagToString(_type)), _id);
	else
		result = format("%s(%u)", STR(tagToString(_type)), _id);
	return result;
}

EventLogger * BaseProtocol::GetEventLogger() {
	//1. If bound to an app, get it now
	if (_pApplication != NULL)
		return _pApplication->GetEventLogger();

	//2. Try to get the logger from the past app it was bound on
	BaseClientApplication *pLastKnownApp = NULL;
	if ((pLastKnownApp = GetLastKnownApplication()) != NULL)
		return pLastKnownApp->GetEventLogger();

	//3. Try to walk up the stack and get the logger
	EventLogger *pEventLogger = NULL;
	if (_pNearProtocol != NULL) {
		pEventLogger = _pNearProtocol->GetEventLogger();
		if (pEventLogger != NULL)
			return pEventLogger;
	}

	//4. ok, we give up, no app found anywhere. Just return the default logger
	return EventLogger::GetDefaultLogger();
}
#ifdef HAS_PROTOCOL_WS

WSInterface* BaseProtocol::GetWSInterface() {
	WARN("BaseProtocol::GetWSInterface must be overridden on protocols accepting WebSocket as transport. Current protocol is %s", STR(tagToString(_type)));
	return NULL;
}
#endif /* HAS_PROTOCOL_WS */
