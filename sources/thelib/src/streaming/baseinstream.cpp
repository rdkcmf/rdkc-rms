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



#include "streaming/baseinstream.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"
#include "streaming/streamsmanager.h"
#include "eventlogger/eventlogger.h"
#include "streaming/codectypes.h"
#include "protocols/baseprotocol.h"
#include "application/clientapplicationmanager.h"

BaseInStream::BaseInStream(BaseProtocol *pProtocol, uint64_t type, string name)
: BaseStream(pProtocol, type, name) {
	if (!TAG_KIND_OF(type, ST_IN)) {
		ASSERT("Incorrect stream type. Wanted a stream type in class %s and got %s",
				STR(tagToString(ST_IN)), STR(tagToString(type)));
	}
	_canCallOutStreamDetached = true;
	Variant streamConfig;
	if (pProtocol != NULL) 
		pProtocol->GetApplication()->GetOperationType(pProtocol, streamConfig);
	_isVod = (pProtocol != NULL)
			&& (streamConfig.HasKeyChain(V_STRING, false, 1, "_isVod"))
			&& ((string)streamConfig["_isVod"] == "1");
	_pts = 0;
	_subscriberCount = 0;
	_initKeyFrameCache();
}

BaseInStream::~BaseInStream() {
	_canCallOutStreamDetached = false;
	while (_outStreams.Size() > 0)
		UnLink(_outStreams.MoveHead()->value, true);
}

void BaseInStream::_initKeyFrameCache() {
	_cachedPTS = _cachedDTS = 0;
	_cachedProcLen = _cachedTotLen = 0;
}

bool BaseInStream::_hasCachedKeyFrame() {
	return (GETAVAILABLEBYTESCOUNT(_cachedKeyFrame) > 0);
}

bool BaseInStream::_consumeCachedKeyFrame(IOBuffer& kfBuffer) {
	// Just copy the contents of the keyframe iobuffer to the
	// outboundstream's kfbuffer
	if (_hasCachedKeyFrame()) {
		kfBuffer.ReadFromBuffer(
			GETIBPOINTER(_cachedKeyFrame), // should be same effect as getPointer()
			GETAVAILABLEBYTESCOUNT(_cachedKeyFrame)); //published); 
		return true;
	}
	return false;
}

double   BaseInStream::_getCachedPTS() {
	return _cachedPTS;
}

double   BaseInStream::_getCachedDTS() {
	return _cachedDTS;
}

uint32_t BaseInStream::_getCachedProcLen() {
	return _cachedProcLen;
}

uint32_t BaseInStream::_getCachedTotLen() {
	return _cachedTotLen;
}

vector<BaseOutStream *> BaseInStream::GetOutStreams() {
	vector<BaseOutStream *> result;
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		ADD_VECTOR_END(result, pCurrent->value);
		pCurrent = _outStreams.MoveNext();
	}
	return result;
}

void BaseInStream::GetStats(Variant &info, uint32_t namespaceId) {
	BaseStream::GetStats(info, namespaceId);
//    INFO("Function Entry.");
	info["outStreamsUniqueIds"] = Variant();
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		info["outStreamsUniqueIds"].PushToArray(
				((((uint64_t) namespaceId) << 32) | pCurrent->value->GetUniqueId()));
		pCurrent = _outStreams.MoveNext();
	}

	StreamCapabilities *pCapabilities = GetCapabilities();
	if (pCapabilities != NULL)
		info["bandwidth"] = (uint32_t) (pCapabilities->GetTransferRate() / 1024.0);
	else
		info["bandwidth"] = (uint32_t) 0;

	Variant customParameters = GetProtocol()->GetCustomParameters();
	if (customParameters.HasKeyChain(V_STRING, false, 1, "ingestPoint"))
		info["ingestPoint"] = customParameters["ingestPoint"];
}

bool BaseInStream::Link(BaseOutStream *pOutStream, bool reverseLink) {
	if ((!pOutStream->IsCompatibleWithType(GetType()))
			|| (!IsCompatibleWithType(pOutStream->GetType()))) {
		FATAL("stream type %s not compatible with stream type %s",
				STR(tagToString(GetType())),
				STR(tagToString(pOutStream->GetType())));
		return false;
	}
	if (_outStreams.HasKey(pOutStream->GetUniqueId())) {
		WARN("BaseInStream::Link: This stream is already linked");
		return true;
	}

//	WARN("[Debug] Linking %s with %s", STR(tagToString(GetType())), STR(tagToString(pOutStream->GetType())));
	_outStreams.AddTail(pOutStream->GetUniqueId(), pOutStream);

	if (reverseLink) {
		if (!pOutStream->Link(this, false)) {
			FATAL("BaseInStream::Link: Unable to reverse link");
			//TODO: here we must remove the link from _pOutStreams and _linkedStreams
			NYIA;
		}
	}
	SignalOutStreamAttached(pOutStream);
	StreamCapabilities *pCapabilities = GetCapabilities();
	if ((pCapabilities != NULL)&&((pCapabilities->GetAudioCodecType() != CODEC_AUDIO_UNKNOWN) || (pCapabilities->GetVideoCodecType() != CODEC_VIDEO_UNKNOWN)))
		pOutStream->GetEventLogger()->LogStreamCodecsUpdated(pOutStream);
	_pStreamsManager->SignalLinkedStreams(this, pOutStream);
	return true;
}

void BaseInStream::UnLinkAll() {
	while (_outStreams.Size() != 0)
		UnLink(_outStreams.MoveHead()->value, true);
}

bool BaseInStream::UnLink(BaseOutStream *pOutStream, bool reverseUnLink) {
	if (!_outStreams.HasKey(pOutStream->GetUniqueId()))
		return true;

	_pStreamsManager->SignalUnLinkingStreams(this, pOutStream);

	_outStreams.Remove(pOutStream->GetUniqueId());

	if (reverseUnLink) {
		if (!pOutStream->UnLink(false)) {
			FATAL("BaseInStream::UnLink: Unable to reverse unLink");
			//TODO: what are we going to do here???
			NYIA;
		}
	}
	if (_canCallOutStreamDetached) {
		SignalOutStreamDetached(pOutStream);
	}

	if (IsUnlinkedVod()) {
		EnqueueForDelete();
		RemoveVodConfig();
	}

	return true;
}

bool BaseInStream::IsUnlinkedVod() {
	return (_isVod) && (_outStreams.Size() == 0) && _subscriberCount == 0;
}

void BaseInStream::SignalLazyPullSubscriptionAction(string action) {
	if (action == "subscribe") {
		_subscriberCount++;
	}
	else if (action == "unsubscribe") {
		if (_subscriberCount > 0)
			_subscriberCount--;
		if (IsUnlinkedVod()) {
			EnqueueForDelete();
			RemoveVodConfig();
		}
	}
}

bool BaseInStream::Play(double dts, double length) {
	if (!SignalPlay(dts, length)) {
		FATAL("Unable to signal play");
		return false;
	}
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->SignalPlay(dts, length)) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

	return true;
}

bool BaseInStream::Pause() {
	if (!SignalPause()) {
		FATAL("Unable to signal pause");
		return false;
	}
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->SignalPause()) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}
	return true;
}

bool BaseInStream::Resume() {
	if (!SignalResume()) {
		FATAL("Unable to signal resume");
		return false;
	}
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->SignalResume()) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

	return true;
}

bool BaseInStream::Seek(double dts) {
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->SignalSeek(dts)) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

    if (!SignalSeek(dts)) {
        FATAL("Unable to signal seek");
        return false;
    }

    return true;
}

bool BaseInStream::Stop() {
	if (!SignalStop()) {
		FATAL("Unable to signal stop");
		return false;
	}

	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->SignalStop()) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

	return true;
}

void BaseInStream::AudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	GetEventLogger()->LogStreamCodecsUpdated(this);

	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (IsEnqueueForDelete())
			return;
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		pCurrent->value->SignalAudioStreamCapabilitiesChanged(pCapabilities, pOld,
				pNew);
		if (pCurrent == _outStreams.Current())
			pCurrent->value->GetEventLogger()->LogStreamCodecsUpdated(pCurrent->value);
		pCurrent = _outStreams.MoveNext();
	}
	Variant &config = _pProtocol->GetCustomParameters();
	if (config.HasKeyChain(V_BOOL, false, 2, "_callback", "pendingCodecInfo")) {
		Variant message;
		message["header"] = "codecUpdated";
		message["payload"] = config["_callback"];
		message["payload"]["streamID"] = GetUniqueId();
		ClientApplicationManager::BroadcastMessageToAllApplications(message);
	} else if (config.HasKeyChain(V_MAP, false, 2, "customParameters", "externalStreamConfig")) {
		Variant &externConfig = config["customParameters"]["externalStreamConfig"];
		Variant message;
		message["header"] = "codecUpdated";
		message["payload"] = externConfig["_callback"];
		message["payload"]["streamID"] = GetUniqueId();
		ClientApplicationManager::BroadcastMessageToAllApplications(message);
	}
}

void BaseInStream::VideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	GetEventLogger()->LogStreamCodecsUpdated(this);
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (IsEnqueueForDelete())
			return;
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		pCurrent->value->SignalVideoStreamCapabilitiesChanged(pCapabilities, pOld,
				pNew);
		if (pCurrent == _outStreams.Current())
			pCurrent->value->GetEventLogger()->LogStreamCodecsUpdated(pCurrent->value);
		pCurrent = _outStreams.MoveNext();
	}
	Variant &config = _pProtocol->GetCustomParameters();
	if (config.HasKeyChain(V_BOOL, false, 2, "_callback", "pendingCodecInfo")) {
		Variant message;
		message["header"] = "codecUpdated";
		message["payload"] = config["_callback"];
		message["payload"]["streamID"] = GetUniqueId();
		ClientApplicationManager::BroadcastMessageToAllApplications(message);
	} else if (config.HasKeyChain(V_MAP, false, 2, "customParameters", "externalStreamConfig")) {
		Variant &externConfig = config["customParameters"]["externalStreamConfig"];
		Variant message;
		message["header"] = "codecUpdated";
		message["payload"] = externConfig["_callback"];
		message["payload"]["streamID"] = GetUniqueId();
		ClientApplicationManager::BroadcastMessageToAllApplications(message);
	}
}

StreamCapabilities * BaseInStream::GetCapabilities() {
	return NULL;
}

Variant &BaseInStream::GetPublicMetadata() {
	if (_publicMetadata == V_NULL) {
		StreamCapabilities *pCapabilities = GetCapabilities();
		if (pCapabilities != NULL)
			pCapabilities->GetRTMPMetadata(_publicMetadata);
	}
	_publicMetadata[HTTP_HEADERS_SERVER] = BRANDING_BANNER;
	return _publicMetadata;
}

uint32_t BaseInStream::GetInputVideoTimescale() {
	StreamCapabilities *pCapabilities = NULL;
	VideoCodecInfo *pCodecInfo = NULL;
	if (((pCapabilities = GetCapabilities()) == NULL)
			|| ((pCodecInfo = pCapabilities->GetVideoCodec()) == NULL))
		return 1;
	return pCodecInfo->_samplingRate;
}

uint32_t BaseInStream::GetInputAudioTimescale() {
	StreamCapabilities *pCapabilities = NULL;
	AudioCodecInfo *pCodecInfo = NULL;
	if (((pCapabilities = GetCapabilities()) == NULL)
			|| ((pCodecInfo = pCapabilities->GetAudioCodec()) == NULL))
		return 1;
	return pCodecInfo->_samplingRate;
}

bool BaseInStream::SendMetadata(string metadataStr) {
	// call SendMetadata() on each of our linked outstreams
	// rip the outstream looping logic from GetStreams()
	//
	bool ok = false;
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		BaseOutStream * os = pCurrent->value;
		if (os) {
			os->SendMetadata(metadataStr, GetPtsInt64());
			ok = true;	// sent at least one!
		}
		pCurrent = _outStreams.MoveNext();
	}

	return ok;
}

void BaseInStream::RemoveVodConfig() {
	Variant message;
	message["header"] = "removeConfig";
	message["payload"]["localStreamName"] = GetName();
	ClientApplicationManager::BroadcastMessageToAllApplications(message);
}
