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



#include "streaming/basestream.h"
#include "streaming/streamsmanager.h"
#include "protocols/baseprotocol.h"
#include "eventlogger/eventlogger.h"
#include "netio/netio.h"
#include "streaming/codectypes.h"
#include "application/baseclientapplication.h"

uint32_t BaseStream::_uniqueIdGenerator = 1;

BaseStream::BaseStream(BaseProtocol *pProtocol, uint64_t type, string name) {
	memset(&_stats, 0, sizeof (_stats));
	_pStreamsManager = NULL;
	_type = type;
	_uniqueId = _uniqueIdGenerator++;
	_pProtocol = pProtocol;
	_name = name;
	GETCLOCKS(_creationTimestamp, double);
	_creationTimestamp /= (double) CLOCKS_PER_SECOND;
	_creationTimestamp *= 1000.00;
	_isStreamPaused = false;
	GetIpPortInfo();
	StoreConnectionType();
}

BaseStream::~BaseStream() {
	if (_pStreamsManager != NULL) {
		_pStreamsManager->UnRegisterStream(this);
		_pStreamsManager = NULL;
	}
}

bool BaseStream::SetStreamsManager(StreamsManager *pStreamsManager, bool registerStreamExpiry) {
	if (pStreamsManager == NULL) {
		FATAL("no streams manager provided for stream %s(%"PRIu32")",
				STR(tagToString(_type)), _uniqueId);
		return false;
	}
	if (_pStreamsManager != NULL) {
		FATAL("Stream %s(%"PRIu32") already registered to the streams manager",
				STR(tagToString(_type)), _uniqueId);
		return false;
	}
	_pStreamsManager = pStreamsManager;
	_pStreamsManager->RegisterStream(this, registerStreamExpiry);
	return true;
}

StreamsManager *BaseStream::GetStreamsManager() {
	return _pStreamsManager;
}

uint64_t BaseStream::GetType() {
	return _type;
}

uint32_t BaseStream::GetUniqueId() {
	return _uniqueId;
}

double BaseStream::GetSpawnTimestamp() {
	return _creationTimestamp;
}

string BaseStream::GetName() {
	return _name;
}

void BaseStream::SetName(string name) {
	if (_name != "") {
		ASSERT("name already set");
	}
	_name = name;
}

void BaseStream::GetStats(Variant &info, uint32_t namespaceId) {
	GetIpPortInfo();
	info["uniqueId"] = (((uint64_t) namespaceId) << 32) | _uniqueId;
	info["type"] = tagToString(_type);
	info["typeNumeric"] = _type;
	info["name"] = _name;
	info["creationTimestamp"] = _creationTimestamp;
	info["ip"] = _nearIp;
	info["port"] = _nearPort;
	info["nearIp"] = _nearIp;
	info["nearPort"] = _nearPort;
	info["farIp"] = _farIp;
	info["farPort"] = _farPort;
	if (_userAgent != "")
		info["userAgent"] = _userAgent;
	if (_serverAgent != "")
		info["serverAgent"] = _serverAgent;
	double queryTimestamp = 0;
	GETCLOCKS(queryTimestamp, double);
	queryTimestamp /= (double) CLOCKS_PER_SECOND;
	queryTimestamp *= 1000.00;
	info["queryTimestamp"] = queryTimestamp;
	info["upTime"] = queryTimestamp - _creationTimestamp;
	info["video"]["width"] = 0;
	info["video"]["height"] = 0;
	info["video"]["profile"] = 0;
	info["video"]["level"] = 0;
	StreamCapabilities *pCapabilities = GetCapabilities();
	if (pCapabilities != NULL) {
		info["video"]["codec"] = tagToString(pCapabilities->GetVideoCodecType());
		info["video"]["codecNumeric"] = (uint64_t) pCapabilities->GetVideoCodecType();
		info["audio"]["codec"] = tagToString(pCapabilities->GetAudioCodecType());
		info["audio"]["codecNumeric"] = (uint64_t) pCapabilities->GetAudioCodecType();
		info["bandwidth"] = pCapabilities->GetTransferRate();
		VideoCodecInfo *vcInfo = pCapabilities->GetVideoCodec();
		if (vcInfo) {
			info["video"]["width"] = vcInfo->_width;
			info["video"]["height"] = vcInfo->_height;
			VideoCodecInfoH264 *h264VCInfo = static_cast<VideoCodecInfoH264 *>(vcInfo);
			if (h264VCInfo) {
				info["video"]["profile"] = h264VCInfo->_profile;
				info["video"]["level"] = h264VCInfo->_level;
			} 
		} 
	} else {
		info["video"]["codec"] = tagToString(CODEC_VIDEO_UNKNOWN);
		info["video"]["codecNumeric"] = (uint64_t) CODEC_VIDEO_UNKNOWN;
		info["audio"]["codec"] = tagToString(CODEC_AUDIO_UNKNOWN);
		info["audio"]["codecNumeric"] = (uint64_t) CODEC_AUDIO_UNKNOWN;
		info["bandwidth"] = 0;
	}

	info["audio"]["packetsCount"] = _stats.audio.packetsCount;
	info["audio"]["droppedPacketsCount"] = _stats.audio.droppedPacketsCount;
	info["audio"]["bytesCount"] = _stats.audio.bytesCount;
	info["audio"]["droppedBytesCount"] = _stats.audio.droppedBytesCount;
	info["video"]["packetsCount"] = _stats.video.packetsCount;
	info["video"]["droppedPacketsCount"] = _stats.video.droppedPacketsCount;
	info["video"]["bytesCount"] = _stats.video.bytesCount;
	info["video"]["droppedBytesCount"] = _stats.video.droppedBytesCount;
	BaseClientApplication *pApp = NULL;
	if ((_pProtocol != NULL)
			&& ((pApp = _pProtocol->GetLastKnownApplication()) != NULL)) {
		info["appName"] = pApp->GetName();
		Variant &customParams = _pProtocol->GetCustomParameters();
		if (customParams.HasKeyChain(V_BOOL, false, 1, "_isInternal")) {
			info["_isInternal"] = customParams["_isInternal"];
		}
	}
	info["processId"] = (uint64_t) GetPid();
	info["processType"] = pApp != NULL ?
			(pApp->IsOrigin() ? "origin" : "edge")
			: "unknown";
	StoreConnectionType();
	if (_connectionType == V_MAP) {

		FOR_MAP(_connectionType, string, Variant, i) {
			info[MAP_KEY(i)] = MAP_VAL(i);
		}
	}
}

BaseProtocol * BaseStream::GetProtocol() {
	return _pProtocol;
}

bool BaseStream::IsEnqueueForDelete() {
	if (_pProtocol != NULL)
		return _pProtocol->IsEnqueueForDelete();
	return false;
}

void BaseStream::EnqueueForDelete() {
	if (_pProtocol != NULL) {
		_pProtocol->EnqueueForDelete();
	} else {
		delete this;
	}
}

EventLogger *BaseStream::GetEventLogger() {
	EventLogger *pResult = NULL;

	//1. try to get it from the protocol
	if (_pProtocol != NULL)
		pResult = _pProtocol->GetEventLogger();

	//2. No choice, return the default event logger
	if (pResult == NULL)
		pResult = EventLogger::GetDefaultLogger();

	//3. Done
	return pResult;
}

BaseStream::operator string() {
	return format("%s(%"PRIu32") with name `%s` from protocol %s(%"PRIu32")",
			STR(tagToString(_type)),
			_uniqueId,
			STR(_name),
			_pProtocol != NULL ? STR(tagToString(_pProtocol->GetType())) : "",
			_pProtocol != NULL ? _pProtocol->GetId() : 0);
}

void BaseStream::StoreConnectionType() {
	if (_connectionType != V_NULL)
		return;
	BaseClientApplication *pApp = NULL;
	if ((_pProtocol != NULL)
			&& ((pApp = _pProtocol->GetLastKnownApplication()) != NULL))
		pApp->StoreConnectionType(_connectionType, _pProtocol);
}

void BaseStream::UpdatePauseStatus(bool newPauseStatus) {
	_isStreamPaused = newPauseStatus;
}

bool BaseStream::IsPaused() {
	return _isStreamPaused;
}

void BaseStream::GetIpPortInfo() {
//	INFO("Function entry. Stream type: %s", STR(tagToString(GetType())));
//	INFO("_nearIp:%s\t_nearPort:%"PRIu16"\t_nearIp:%s\t_nearPort:%"PRIu16,
//			STR(_nearIp), _nearPort, STR(_nearIp), _nearPort);
	if ((_nearIp != "")
			&& (_nearPort != 0)
			&& (_farIp != "")
			&& (_farPort != 0)) {
//            INFO("Not Here");
		return;
	}
//            INFO("Here");
	IOHandler *pIOHandler = NULL;
//        FINE("_pProtocol is %sNULL\tIOHandler is %sNULL", 
//                        _pProtocol != NULL ? "not " : "", 
//                        _pProtocol->GetIOHandler() != NULL ? "not ":"");
	if ((_pProtocol != NULL) 
			&& ((pIOHandler = _pProtocol->GetIOHandler()) != NULL)
			&& (pIOHandler->GetType() == IOHT_TCP_CARRIER)) {
		_nearIp = ((TCPCarrier *) pIOHandler)->GetNearEndpointAddressIp();
		_nearPort = ((TCPCarrier *) pIOHandler)->GetNearEndpointPort();
		_farIp = ((TCPCarrier *) pIOHandler)->GetFarEndpointAddressIp();
		_farPort = ((TCPCarrier *) pIOHandler)->GetFarEndpointPort();
	} else if ((pIOHandler != NULL)
			&& (pIOHandler->GetType() == IOHT_UDP_CARRIER)) {
		_nearIp = ((UDPCarrier *) pIOHandler)->GetNearEndpointAddress();
		_nearPort = ((UDPCarrier *) pIOHandler)->GetNearEndpointPort();
		_farIp = "";
		_farPort = 0;
	} else {
		_nearIp = "";
		_nearPort = 0;
		_farIp = "";
		_farPort = 0;
	}
	if ((_pProtocol != NULL) && _pProtocol->GetCustomParameters().HasKeyChain(V_STRING, false, 1, "userAgent"))
		_userAgent = (string) _pProtocol->GetCustomParameters().GetValue("userAgent", false);

	if ((_pProtocol != NULL) && _pProtocol->GetCustomParameters().HasKeyChain(V_STRING, false, 1, "serverAgent"))
		_serverAgent = (string) _pProtocol->GetCustomParameters().GetValue("serverAgent", false);
}
