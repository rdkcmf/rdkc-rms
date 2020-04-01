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

#include "streaming/fmp4/outnetfmp4stream.h"
#include "streaming/streamstypes.h"
#include "application/baseclientapplication.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/codectypes.h"
#include "protocols/protocolmanager.h"
#include "eventlogger/eventlogger.h"
#include "protocols/fmp4/http4fmp4protocol.h"
#include "streaming/baseinstream.h"
#include "protocols/rtp/rtspprotocol.h"
#include "streaming/nalutypes.h"

OutNetFMP4Stream::OutNetFMP4Stream(BaseProtocol *pProtocol, string name, bool progressive, bool useHttp)
: BaseOutNetStream(pProtocol, ST_OUT_NET_FMP4, name) {
	_emptyRunsCount = 0;
	_progressive = progressive;
	_delayedRemoval = false;
	_useHttp = useHttp;
	_isPlayerPaused = false;
	_pauseDts = -1;
	_resumeDts = -1;
	_dtsNeedsUpdate = false;
	_pauseDts = -1;
	_resumeDts = -1;
	_wrtcChannelReady = false;
	_firstFrameSent = false;
	_isOutStreamStackEstablished = false;

//#ifdef WEBRTC_DEBUG
	FINE("OutNetFMP4Stream(%s)", STR(tagToString(pProtocol->GetType())));
//#endif
}

OutNetFMP4Stream::~OutNetFMP4Stream() {
//#ifdef WEBRTC_DEBUG
	FINE("OutNetFMP4Stream deleted.");
//#endif
	/* This is a good thought, but it causes a seg fault if the instream goes down first
	  In reality, there should only be one out for any _inboundVod==true streams so resuming
	  on destruction is not really necessary.  keeping the code here in case in some future
	  situation this becomes necessary. Maybe check _inboundVod with the initial if(_pOrigInStream) 
	  would solve the crash?

	  -- I will readd this part, now with safe checking if stream is still existing.
	  It is now being checked thru unique ids, not using instream pointers -  */
	BaseStream* pOrigInStream = GetOrigInstream();
	BaseProtocol* pProtocol = NULL;
	if (pOrigInStream) {
		pProtocol = pOrigInStream->GetProtocol();
		//check if pInstream's protocol is RTSP, stream is a vodstream, and is paused,
		//send resume message so that instream still continues its inbound process
		if (pProtocol && pProtocol->GetType() == PT_RTSP
			&& _inboundVod	&& pOrigInStream->IsPaused()) {
			pOrigInStream->SignalResume();
		}
	}
	
	FOR_MAP(_signaledProtocols, uint32_t, void *, i) {
		BaseProtocol *pProtocol = ProtocolManager::GetProtocol(MAP_KEY(i));
		if (pProtocol != NULL)
			pProtocol->SignalOutOfBandDataEnd(MAP_VAL(i));
	}
	GetEventLogger()->LogStreamClosed(this);
}

OutNetFMP4Stream *OutNetFMP4Stream::GetInstance(BaseClientApplication *pApplication,
		string name, bool progressive, bool useHttp) {
	//1. Create a dummy protocol
	PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

	//2. create the parameters
	Variant parameters;

	//3. Initialize the protocol
	if (!pDummyProtocol->Initialize(parameters)) {
		FATAL("Unable to initialize passthrough protocol");
		pDummyProtocol->EnqueueForDelete();
		return NULL;
	}

	//4. Set the application
	pDummyProtocol->SetApplication(pApplication);

	//5. Create outbound fragmented MP4 stream
	OutNetFMP4Stream *pOutNetFMP4 = new OutNetFMP4Stream(pDummyProtocol, name,
			progressive, useHttp);
	if (!pOutNetFMP4->SetStreamsManager(pApplication->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pOutNetFMP4;
		pOutNetFMP4 = NULL;
		return NULL;
	}
	pDummyProtocol->SetDummyStream(pOutNetFMP4);

	//6. Done
	return pOutNetFMP4;
}

void OutNetFMP4Stream::RegisterOutputProtocol(uint32_t protocolId, void *pUserData) {
	BaseProtocol *pProtocol = ProtocolManager::GetProtocol(protocolId);
	if (pProtocol == NULL)
		return;
	INFO("%s protocol linked to stream %s", STR(*pProtocol), STR(*this));
	if (GETAVAILABLEBYTESCOUNT(_completedMoov) != 0) {
		if (!pProtocol->SendOutOfBandData(_completedMoov, pUserData)) {
			pProtocol->SignalOutOfBandDataEnd(pUserData);
			return;
		}
	}
	_signaledProtocols[protocolId] = pUserData;
	_emptyRunsCount = 0;
}

void OutNetFMP4Stream::UnRegisterOutputProtocol(uint32_t protocolId) {
	if (!MAP_HAS1(_signaledProtocols, protocolId))
		return;
	if (_delayedRemoval) {
		_pendigRemovals.push_back(protocolId);
	} else {
		_signaledProtocols.erase(protocolId);
		if (_signaledProtocols.size() == 0)
			EnqueueForDelete();
	}
}

bool OutNetFMP4Stream::IsProgressive() const {
	return _progressive;
}

bool OutNetFMP4Stream::Flush() {
	if (!BaseOutStream::Flush())
		return false;
	return FMP4Writer::Flush();
}

bool OutNetFMP4Stream::IsCompatibleWithType(uint64_t type) {
	return TAG_KIND_OF(type, ST_IN_NET_RTMP)
			|| TAG_KIND_OF(type, ST_IN_NET_RTP)
			|| TAG_KIND_OF(type, ST_IN_NET_TS)
			|| TAG_KIND_OF(type, ST_IN_NET_LIVEFLV)
			|| TAG_KIND_OF(type, ST_IN_DEVICE)
			|| TAG_KIND_OF(type, ST_IN_FILE)
            || TAG_KIND_OF(type, ST_IN_NET_EXT);
}

bool OutNetFMP4Stream::SignalPlay(double &dts, double &length) {
	return true;
}

bool OutNetFMP4Stream::SignalPause() {
	UpdatePauseStatus(true);
	return true;
}

bool OutNetFMP4Stream::SignalResume() {
	UpdatePauseStatus(false);
	return true;
}

bool OutNetFMP4Stream::SignalSeek(double &dts) {
	return true;
}

bool OutNetFMP4Stream::SignalStop() {
	return true;
}

void OutNetFMP4Stream::SignalAttachedToInStream() {
}

void OutNetFMP4Stream::SignalDetachedFromInStream() {
//#ifdef WEBRTC_DEBUG
	FINE("SignalDetachedFromInStream");
//#endif
	if (_pProtocol != NULL) {
		_pProtocol->EnqueueForDelete();
	}
}

void OutNetFMP4Stream::SignalStreamCompleted() {
//#ifdef WEBRTC_DEBUG
	FINE("SignalStreamCompleted");
//#endif
	if (_pProtocol != NULL) {
		_pProtocol->EnqueueForDelete();
	}
}

bool OutNetFMP4Stream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {

	//FATAL("DTS: %lf", dts);
	//_stopOutbound == true means a fragment has been completely closed while outbound player is paused
	if (_stopOutbound) {
		//if player paused and fragment just closed
		/*if (_dtsNeedsUpdate) {
			_pauseDts = dts;
			_dtsNeedsUpdate = false;
		}*/

		uint64_t &droppedBytesCount = isAudio ? _stats.audio.droppedBytesCount : _stats.video.droppedBytesCount;
		uint64_t &droppedPacketsCount = isAudio ? _stats.audio.droppedPacketsCount : _stats.video.droppedPacketsCount;
		droppedBytesCount += dataLength;
		droppedPacketsCount++;
	} else {
		Variant &streamConfig = GetProtocol()->GetCustomParameters();
		WrtcConnection *pWrtc = GetWrtcConnection();
		if (streamConfig.HasKeyChain(V_MAP, false, 1, "_metadata")) {
			if (pWrtc != NULL && pWrtc->GetCommandChannelId() != 0) {
				SendMetadata(streamConfig.GetValue("_metadata", false), (int64_t)pts);
				SendMetadata(streamConfig.GetValue("_metadata2", false), (int64_t)pts);
				streamConfig.RemoveKey("_metadata");
				streamConfig.RemoveKey("_metadata2");
			}
		}
		BaseInStream* pOrigInStream = (BaseInStream*)GetOrigInstream();
		if (pOrigInStream) {
			BaseProtocol* pProtocol = pOrigInStream->GetProtocol();
			if (pProtocol && pProtocol->GetType() == PT_RTSP) {
				Variant &playNotifyReq = ((RTSPProtocol*)pProtocol)->GetPlayNotifyReq();
				if (pWrtc != NULL && pWrtc->GetCommandChannelId() != 0 && playNotifyReq != V_NULL) {
					SendMetadata(playNotifyReq, (int64_t)pts);
					// removes the content once sent, should refresh once a new play notify req is received
					playNotifyReq.Reset();
				}
			}
		}

		return GenericProcessData(pData, dataLength, processedLength, totalLength,
			pts, dts, isAudio);
	}

	return true;
}

void OutNetFMP4Stream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	if ((pOld == NULL)&&(pNew != NULL))
		return;
	WARN("Codecs changed and the out net fragmented MP4 does not support it. Closing stream");
	if (pOld != NULL)
		FINEST("pOld: %s", STR(*pOld));
	if (pNew != NULL)
		FINEST("pNew: %s", STR(*pNew));
	else
		FINEST("pNew: NULL");
	EnqueueForDelete();
}

/*
 * This function is when the video characteristics of the inbound stream has changed.  
 * This usually occurs when the source stream is a PLAYLIST, and the next entry in the
 * list is a stream that has a different Video characteristic.
 */
void OutNetFMP4Stream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	// OLD and NEW are NULL if the stream is being played for the first time.  So in this case,
	// just ignore!
	if ((pOld == NULL)&&(pNew != NULL))
		return;

	// In the past, video stream cap changes only supported (actually, ignored) SPS/PPS changes.
	// Any other change was not supported and has resulted in the shutdown of outbound stream.
	// Now we have to support mid-stream changes as part of the PLAYlist support.
	EnqueueResetStream(true, true, pCapabilities, _progressive);
	//ResetStream();
	//Initialize(true, true, pCapabilities, _progressive);

/*
	// If this is SPS/PPS update, continue
	if (IsVariableSpsPps(pOld, pNew)) {
		return;
	}
	
	WARN("Codecs changed and the out net fragmented MP4 does not support it. Closing stream");
	if (pOld != NULL)
		FINEST("pOld: %s", STR(*pOld));
	if (pNew != NULL)
		FINEST("pNew: %s", STR(*pNew));
	else
		FINEST("pNew: NULL");
	EnqueueForDelete();
*/
}

bool OutNetFMP4Stream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}

	//PROBE_POINT("webrtc", "sess1", "outnet_fmp4_init_finish", false);

	//video setup
	pGenericProcessDataSetup->video.avc.Init(
			NALU_MARKER_TYPE_SIZE, //naluMarkerType,
			false, //insertPDNALU,
			false, //insertRTMPPayloadHeader,
			true, //insertSPSPPSBeforeIDR,
			true //aggregateNALU
			);

	//audio setup
	pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
	pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = false;

	//misc setup
	pGenericProcessDataSetup->_timeBase = 0;
	pGenericProcessDataSetup->_maxFrameSize = 8 * 1024 * 1024;

	return Initialize(true, true, GetCapabilities(), _progressive);
}

bool OutNetFMP4Stream::SignalPing() {
	string timestamp = "sync";

	time_t now = time(NULL);
	tm* tmNow = gmtime(&now);
	now = mktime(tmNow);
	return SendMetadata(timestamp, (uint64_t)now);
}

bool OutNetFMP4Stream::PushVideoData(IOBuffer &buffer, double pts, double dts,
		bool isKeyFrame) {
	//if ( isKeyFrame ) {
	//	string timestamp = "sync";

	//	time_t now =  time(NULL);
	//	tm* tmNow = gmtime(&now);
	//	now = mktime(tmNow);
	//	SendMetadata(timestamp, (uint64_t) now);
	//}
	return WriteVideoData(buffer, pts, dts, isKeyFrame);
}

bool OutNetFMP4Stream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	return WriteAudioData(buffer, pts, dts);
}

bool OutNetFMP4Stream::IsCodecSupported(uint64_t codec) {
	return (codec == CODEC_VIDEO_H264) || (codec == CODEC_AUDIO_AAC);
}

// BeginWriteMOOV : Erase the contents of the inProgressMOOV buffer!
// 
bool OutNetFMP4Stream::BeginWriteMOOV() {
	return _inProgressMoov.IgnoreAll();
}

// EndWriteMOOV : Copy inProressMOOV buffer to completedMOOV buffer.  Then
//                send it out to the outbound connections.
bool OutNetFMP4Stream::EndWriteMOOV(uint32_t durationOffset, bool durationIs64Bits) {
	return _completedMoov.IgnoreAll()
			&& _completedMoov.ReadFromInputBufferWithIgnore(_inProgressMoov)
			&& SignalProtocols(_completedMoov)
			;
}

bool OutNetFMP4Stream::BeginWriteFragment() {
	return _inProgressFragment.IgnoreAll();
}

bool OutNetFMP4Stream::EndWriteFragment(double moovStart, double moovDuration,
		double moofStart) {
	return _completedFragment.IgnoreAll()
			&& _completedFragment.ReadFromInputBufferWithIgnore(_inProgressFragment)
			&& SignalProtocols(_completedFragment)
			;
}

bool OutNetFMP4Stream::WriteBuffer(FMP4WriteState state, const uint8_t *pBuffer, size_t size) {
	return state == WS_MOOV ?
			_inProgressMoov.ReadFromBuffer(pBuffer, (uint32_t) size)
			: _inProgressFragment.ReadFromBuffer(pBuffer, (uint32_t) size);
}

bool OutNetFMP4Stream::WriteProgressiveBuffer(FMP4ProgressiveBufferType type,
		const uint8_t *pBuffer, size_t size) {
	if (size > 0x00ffffff) {
		FATAL("Frame too big. Maximum allowed size is 0x00ffffff bytes");
		return false;
	}
	if (type == PBT_HEADER_MOOV) {
		if ((!_completedMoov.IgnoreAll())
				|| (_useHttp ? false : !_completedMoov.ReadFromU32((uint32_t) size | (type << 24), true))
				|| (!_completedMoov.ReadFromBuffer(pBuffer, (uint32_t) size))) {
			FATAL("Unable to save completed MOOV atom");
			return false;
		}
	}

	if (_useHttp) {
		// For HTTP format
		
		_progressiveBuffer.IgnoreAll();
		_progressiveBuffer.ReadFromBuffer(pBuffer, (uint32_t) size);
		//	_progressiveBuffer.ReadFromString("\r\n");
		return SignalProtocols(_progressiveBuffer);
	} else {
		// For WS/Webrtc format
		return _progressiveBuffer.IgnoreAll()
				&& _progressiveBuffer.ReadFromU32((uint32_t) size | (type << 24), true)
				&& _progressiveBuffer.ReadFromBuffer(pBuffer, (uint32_t) size)
				&& SignalProtocols(_progressiveBuffer);
	}
}

bool OutNetFMP4Stream::SignalProtocols(const IOBuffer &buffer, bool isMetadata, void *metadata) {
	map<uint32_t, void *>::iterator i = _signaledProtocols.begin();
	_delayedRemoval = true;

	while (i != _signaledProtocols.end()) {
		BaseProtocol *pProtocol = ProtocolManager::GetProtocol(i->first);
		if (pProtocol == NULL) {
			_signaledProtocols.erase(i++);
			continue;
		}

		if (pProtocol->GetType() == PT_INBOUND_HTTP_4_FMP4) {
			if (!((HTTP4FMP4Protocol*)pProtocol)->SendBlock(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer))) {
				FATAL("Unable to send data");
				return false;
			}
		} else {
			void *pUserData = i->second;
			// Variant altData;

			// Only update the user data if protocol is webrtc and a metadata
			if (((pProtocol->GetType() == PT_INBOUND_WS_FMP4)||(pProtocol->GetType() == PT_WRTC_CNX)) && isMetadata) {
				pUserData = metadata;
			}
		

			if (!pProtocol->SendOutOfBandData(buffer, pUserData)) {
				pProtocol->SignalOutOfBandDataEnd(pUserData);
				_signaledProtocols.erase(i++);
				continue;
			}

			if(!_firstFrameSent && _isOutStreamStackEstablished){
				// client only needs a debug log for this now...
				//GetEventLogger()->LogOutStreamFirstFrameSent(this);
				INFO( "First frame sent to player (FMP4)" );
				//Output the time it took to get to this point
				std::stringstream sstrm;
				sstrm << pProtocol->GetId();
				string probSess("wrtcplayback");
				probSess += sstrm.str();
				uint64_t ts = PROBE_TRACK_TIME_MS(STR(probSess) );
				INFO("First frame sent event:%"PRId64"", ts);
				// We need to go get the other data we stored earlier to report here
				probSess += "wrtcConn";
				uint64_t wconTs = PROBE_GET_STORED_TIME(probSess,true);
				INFO("Session statistics:%"PRId32",%"PRId64",%"PRId64"", pProtocol->GetId(), wconTs, ts );
				_firstFrameSent = true;
			}

			//PROBE_POINT("webrtc", "sess1", "outnet_fmp4_signal_proto_end", true);
			PROBE_FLUSH("webrtc", "sess1", true);
		}
		i++;
	}
	_delayedRemoval = false;
	while (_pendigRemovals.size() != 0) {
		UnRegisterOutputProtocol(_pendigRemovals[0]);
		_pendigRemovals.erase(_pendigRemovals.begin());
	}

	if (_signaledProtocols.size() == 0)
		_emptyRunsCount++;
	else
		_emptyRunsCount = 0;
	if (_emptyRunsCount >= 10)
		EnqueueForDelete();

	return true;
}

bool OutNetFMP4Stream::SendMetadata(string const& vmfMetadata, int64_t pts) {
#if 0
	Variant meta;

	meta["type"] = "metadata";
	meta["payload"]["timestamp"] = pts;
	meta["payload"]["data"] = vmfMetadata;

	// _progressiveBuffer here is just a dummy placeholder
	return SignalProtocols(_progressiveBuffer, true, (void *) &meta);
#else
	Variant payload;
	payload["timestamp"] = pts;
	payload["data"] = vmfMetadata;
	return SignalProtocols(_progressiveBuffer, true, (void *)&payload);
#endif

}

bool OutNetFMP4Stream::PushMetaData(string const& vmfMetadata, int64_t pts) {
	Variant payload;
	payload["timestamp"] = pts;
	payload["data"] = vmfMetadata;
	return SignalProtocols(_progressiveBuffer, true, (void *)&payload);
}

bool OutNetFMP4Stream::SendMetadata(Variant sdpMetadata, int64_t pts) {
	string data = "";
	sdpMetadata.SerializeToJSON(data, true);

	// _progressiveBuffer here is just a dummy placeholder
//	return SendMetadata(data, pts);
	return PushMetaData(data, pts);
}

void OutNetFMP4Stream::UpdatePauseStatus(bool isNowPaused) {
	_isPlayerPaused = isNowPaused;
	/*if (!_inboundVod)
		_dtsNeedsUpdate = true;*/
	BaseStream* pOrigInStream = GetOrigInstream();
	if (isNowPaused == false) {
		_stopOutbound = false;
		if (pOrigInStream && _inboundVod && pOrigInStream->IsPaused()) {
			pOrigInStream->SignalResume();
		}
	} else {
		//this flag will be turned to false again so that outbound stream will look for keyframe first when stream resumes
		_keyframeCacheConsumed = false;
		if (pOrigInStream && _inboundVod) {
			pOrigInStream->SignalPause();
		}
	}
}

void OutNetFMP4Stream::SetWrtcConnection(WrtcConnection* pWrtc) {
	if (pWrtc != NULL) _wrtcProtocolId = pWrtc->GetId();
}

void OutNetFMP4Stream::SetOutStreamStackEstablished() {
	_isOutStreamStackEstablished = true;
}

WrtcConnection *OutNetFMP4Stream::GetWrtcConnection() {
	if (_wrtcProtocolId == 0) return NULL;
	BaseProtocol *pProto = ProtocolManager::GetProtocol(_wrtcProtocolId);
	if (pProto == NULL) {
		_wrtcProtocolId = 0;
	}
	return (WrtcConnection *)pProto;
}

void OutNetFMP4Stream::SetInboundVod(bool inboundVod) {
	_inboundVod = inboundVod;
}

bool OutNetFMP4Stream::GenericProcessData(
	uint8_t * pData, uint32_t dataLength,
	uint32_t processedLength, uint32_t totalLength,
	double pts, double dts,
	bool isAudio)
{
	bool result = false;
	if (!_stopOutbound || _inboundVod) {
		//if player just resumed from pause
		/*if (!_inboundVod) {
			if (_dtsNeedsUpdate && !_isPlayerPaused) {
				_resumeDts = dts;
				_dtsNeedsUpdate = false;
			}
			if (_resumeDts != -1 && _pauseDts != -1) {
				_dtsOffset = _resumeDts - _pauseDts;
				_pauseDts = -1;
				_resumeDts = -1;
			}
		}*/
		BaseInStream* pOrigInStream = (BaseInStream*) GetOrigInstream();
		if (pOrigInStream != _pOrigInStream) {
			ResetDtsValues(); //if there is a change in the playlist, reset the averages
			INFO("Resetting Dts Ave Values.");
		}
		_pOrigInStream = pOrigInStream;
		// We've got a cached keyframe?
		// pData is not a keyframe?
		// Cache hasn't been used yet?
		//
		// IF yes to all, then let's send out the cached keyframe first.
		if (pOrigInStream && pOrigInStream->_hasCachedKeyFrame() &&
			(NALU_TYPE(pData[0]) != NALU_TYPE_IDR) &&
			!_keyframeCacheConsumed &&
			!_isPlayerPaused &&
			!isAudio) {
			//!_pInStream->_cacheConsumed() ) {
			IOBuffer cachedKeyFrame;
			if (pOrigInStream->_consumeCachedKeyFrame(cachedKeyFrame)) {
				INFO("Keyframe Cache HIT! SIZE=%d PTS=%lf",
					GETAVAILABLEBYTESCOUNT(cachedKeyFrame),
					pts);
				result = BaseOutStream::GenericProcessData(
					GETIBPOINTER(cachedKeyFrame),
					GETAVAILABLEBYTESCOUNT(cachedKeyFrame),
					pOrigInStream->_getCachedProcLen(),
					pOrigInStream->_getCachedTotLen(),
					pts, dts, isAudio);
				if (!result) {
					return false;
				}
				_keyframeCacheConsumed = true;
			}
		} else if (NALU_TYPE(pData[0]) == NALU_TYPE_IDR && !_keyframeCacheConsumed && !_isPlayerPaused) {
			INFO("Outbound frame was a keyframe, sending cached keyframe is not needed anymore.");
			_keyframeCacheConsumed = true;
			result = BaseOutStream::GenericProcessData(
				pData, dataLength,
				processedLength, totalLength,
				pts, dts,
				isAudio);
		} else {
			result = BaseOutStream::GenericProcessData(
				pData, dataLength,
				processedLength, totalLength,
				pts, dts,
				isAudio);
		}
		uint64_t &bytesCount = isAudio ? _stats.audio.bytesCount : _stats.video.bytesCount;
		uint64_t &packetsCount = isAudio ? _stats.audio.packetsCount : _stats.video.packetsCount;
		packetsCount++;
		bytesCount += dataLength;
	}
	return result;
}
 
