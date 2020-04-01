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


#include "protocols/rtmp/streaming/rtmpplaylistitem.h"
#include "protocols/rtmp/streaming/rtmpplaylist.h"
#include "protocols/rtmp/streaming/infilertmpstream.h"
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolmanager.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinnetstream.h"
#include "streaming/streamsmanager.h"
#include "streaming/baseindevicestream.h"

RTMPPlaylistItem::RTMPPlaylistItem(RTMPPlaylist *pPlaylist,
		const string &requestedStreamName, double requestedStart,
		double requestedLength, double playlistDuration,
		Metadata &metadata)
: _requestedStreamName(requestedStreamName) {
	_index = 0;
	_requestedStart = requestedStart;
	_requestedLength = requestedLength;
	_playlistStart = 0;
	_playlistDuration = playlistDuration;
	_metadata = metadata;
	_pPlaylist = pPlaylist;
	_type = _requestedStart == -2000 ? LIVE_OR_RECORDED
			: (_requestedStart == -1000 ? LIVE : RECORDED);
	_streamName = "";
	memset(&_parameters, 0, sizeof (_parameters));
	if (_metadata.type() == MEDIA_TYPE_VOD) {
		string streamName = _metadata.originalStreamName();
		SignalLazyPullAction("subscribe");
	}
}

RTMPPlaylistItem::~RTMPPlaylistItem() {
	if (_metadata.type() == MEDIA_TYPE_VOD) {
		SignalLazyPullAction("unsubscribe");
	}
}

void RTMPPlaylistItem::SignalLazyPullAction(string action) {
	string streamName = _metadata.originalStreamName();
	if (streamName != "") {
		Variant message;
		message["header"] = "vodRequest";
		message["payload"]["action"] = action;
		message["payload"]["streamName"] = streamName;
		ClientApplicationManager::BroadcastMessageToAllApplications(message);
	}
}

RTMPPlaylistItem::operator string() {
	return format("%s requested: (S: %.0f; D: %.0f) from [S: %.0f; D: %.0f]",
			STR(_requestedStreamName), _requestedStart, _requestedLength,
			_playlistStart, _playlistDuration);
}

bool RTMPPlaylistItem::ResendMetadata() {
	return _pPlaylist->ResendMetadata(this);
}

Variant &RTMPPlaylistItem::GetPublicMetadata() {
	if (_publicMetadata != V_NULL)
		return _publicMetadata;
	OutNetRTMPStream *pOutStream = _pPlaylist->GetOutStream();
	if (pOutStream == NULL)
		return _publicMetadata;
	StreamCapabilities *pCapabilities = pOutStream->GetCapabilities();
	if (pCapabilities == NULL)
		return _publicMetadata;
	pCapabilities->GetRTMPMetadata(_publicMetadata);
	return _publicMetadata;
}

Metadata &RTMPPlaylistItem::GetAllMetadata() {
	return _metadata;
}

double RTMPPlaylistItem::GetPlaylistStart() {
	return _playlistStart;
}

double RTMPPlaylistItem::GetPlaylistDuration() {
	return _playlistDuration;
}

bool RTMPPlaylistItem::ContainsTimestamp(double absoluteTimelineHead) {
	if ((absoluteTimelineHead < 0)
			|| (_playlistStart < 0)
			|| (_playlistDuration < 0))
		return true;

	if (_playlistDuration > 0) {
		return (_playlistStart <= absoluteTimelineHead)
			&& (absoluteTimelineHead < (_playlistStart + _playlistDuration));
	} else { //handle the case where the playlist duration is 0
		return (_playlistStart <= absoluteTimelineHead);
	}
}

bool RTMPPlaylistItem::Play(double absoluteTimelineHead, bool isSeek,
		RTMPPlaylistState startupState, bool singleGop) {
	//1. Reset the parameters and gather the ones from function call
	memset(&_parameters, 0, sizeof (_parameters));
	_parameters.singleGop = singleGop;
	_parameters.startupState = startupState;
	_parameters.absoluteTimelineHead = absoluteTimelineHead;
	_parameters.isSeek = isSeek;

	//2. get the protocol, application and streams manager
	_parameters.pProtocol = NULL;
	_parameters.pApp = NULL;
	_parameters.pSMR = NULL;
	if (((_parameters.pProtocol = _pPlaylist->GetProtocol()) == NULL)
			|| ((_parameters.pApp = _parameters.pProtocol->GetApplication()) == NULL)
			|| ((_parameters.pSMR = _parameters.pApp->GetStreamMetadataResolver()) == NULL)
			|| ((_parameters.pSM = _parameters.pApp->GetStreamsManager()) == NULL)) {
		FATAL("Unable to get the protocol");
		return false;
	}

	//3. un-alias the stream name
	if (_streamName == "") {
		BaseClientApplication *pApp = NULL;
		if ((pApp = _parameters.pProtocol->GetApplication()) == NULL) {
			FATAL("Unable to get the application");
			return false;
		}

		//disregard stream alias if stream belongs to a playlist file
		if (_pPlaylist->IsListFile()) {
			_streamName = _requestedStreamName;
		} else {
			_streamName = pApp->GetStreamNameByAlias(_requestedStreamName, true);
		}
		trim(_streamName);
		if (_streamName == "") {
			FATAL("Stream name alias value not found: %s", STR(_requestedStreamName));
			return false;
		}
	}

	//4. Compute playback start and length
	_parameters.playbackStart = _requestedStart;
	_parameters.playbackLength = _requestedLength;
	if ((_parameters.isSeek)&&(_playlistStart >= 0)&&(_playlistDuration >= 0)) {
		if (absoluteTimelineHead < _playlistStart) {
			FATAL("Invalid absoluteTimelineHead");
			return false;
		}
		double offset = absoluteTimelineHead - _playlistStart;
		if (_parameters.playbackStart < 0)
			_parameters.playbackStart = 0;
		if (_parameters.playbackLength < 0)
			_parameters.playbackLength = _playlistDuration;
		if (offset > _parameters.playbackLength) {
			FATAL("Invalid absoluteTimelineHead");
			return false;
		}
		_parameters.playbackStart += offset;
		_parameters.playbackLength -= offset;
	}
	//	FINEST("%.2f:%.2f -> %.2f:%.2f", _requestedStart, _requestedLength,
	//			playbackStart, playbackLength);

	//5. Close any output streams first
	if (!_parameters.pProtocol->CloseStream(_pPlaylist->GetRTMPStreamId(), true)) {
		FATAL("Unable to close existing output stream");
		return false;
	}

	//6. Based on the item type, we try the adobe fallback routine
	switch (_type) {
		case LIVE_OR_RECORDED:
		{
			//7. Try to play the live stream
			if (!TryPlayLive()) {
				return false;
			} else {
				_type = LIVE;
			}
			if (_parameters.linked)
				return true;

			//8. Try to play the recorded stream
			if (!TryPlayRecorded(true)) {
				return false;
			} else {
				_type = RECORDED;
			}
			if (_parameters.linked)
				return true;

			//9. Check if redirection is enabled
			if (_parameters.pApp->IsVodRedirectRtmpEnabled()) {
				string backupRmsAddress = "";
				backupRmsAddress = _parameters.pApp->GetBackupRmsAddress();
				_parameters.pProtocol->CloseStream(_pPlaylist->GetRTMPStreamId(), true);
				PullRedirectedSource(backupRmsAddress);						
				if (!TryPlayLive()) {
					return false;
				} else {
					_type = LIVE;
				}

				if (_parameters.linked)
					return true;
			}
			//10. Couldn't play neither live or recorded. Wait for the live
			if (WaitForLive(true) == NULL) {
				return false;
			} else {
				_type = LIVE;
			}
			return true;
		}
		case LIVE:
		{
			//11. Try to play the live stream
			if (!TryPlayLive())
				return false;
			if (_parameters.linked)
				return true;

			if (WaitForLive(true) == NULL)
				return false;
			return true;
		}
		case RECORDED:
		{
			//12. Try to play the recorded stream
			if (!TryPlayRecorded(false)) {
				FATAL("TryPlayRecorded failed");
				return false;
			}
			if (_parameters.linked)
				return true;

			//13. No recorded stream found. Signal stream not found
			if (!_pPlaylist->SignalItemNotFound(this, _parameters.startupState))
				return false;

			return true;
		}
		default:
		{
			//14. Invalid
			return false;
		}
	}
}

bool RTMPPlaylistItem::Pause() {
	OutNetRTMPStream *pOutStream = _pPlaylist->GetOutStream();
	if (pOutStream == NULL) {
		FATAL("Unable to get stream");
		return false;
	}

	return pOutStream->Pause();
}

bool RTMPPlaylistItem::Resume() {
	OutNetRTMPStream *pOutStream = _pPlaylist->GetOutStream();
	if (pOutStream == NULL) {
		FATAL("Unable to get stream");
		return false;
	}

	BaseStream *pInStream = pOutStream->GetInStream();
	if (pInStream == NULL) {
		FATAL("Unable to get stream");
		return false;
	}

	if (TAG_KIND_OF(pInStream->GetType(), ST_IN_FILE))
		((BaseInFileStream *) pInStream)->SingleGop(false);
	_parameters.startupState = PAUSED;

	if (TAG_KIND_OF(pInStream->GetType(), ST_IN_FILE)) {
		if ((!_pPlaylist->SignalItemStart(this, true, _parameters.startupState, false))
				|| (!pOutStream->Resume())) {
			FATAL("Unable to signal resume");
			return false;
		}
		pOutStream->ReadyForSend();
	} else if (TAG_KIND_OF(pInStream->GetType(), ST_IN_NET)) {
		if ((!_pPlaylist->SignalItemStart(this, false, _parameters.startupState, false))
				|| (!pOutStream->SignalResume())) {
			FATAL("Unable to signal resume");
			return false;
		}
	}

	return true;
}

bool RTMPPlaylistItem::SignalStreamCompleted(double absoluteTimelineHead) {
	
	if (_type == RECORDED || (_type == LIVE 
			&& (_requestedLength == -2000 || _requestedLength >= 0))) {
		return _pPlaylist->SignalItemCompleted(this, absoluteTimelineHead,
				_parameters.singleGop, _parameters.startupState);
	}
	return true;
}

bool RTMPPlaylistItem::TryPlayRecorded(bool fallBackOnLive) {
	//1. reset the linked flag
//	WARN("[Debug] Trying to play recorded RTMP");
	_parameters.linked = false;

	//2. get the metadata
	_metadata.Reset();
	if (!_parameters.pSMR->ResolveMetadata(_streamName, _metadata, !fallBackOnLive))
		return true;

	//3. Try to create the out net stream
	OutNetRTMPStream *pOutNetRTMPStream = _parameters.pProtocol->CreateONS(this,
			_pPlaylist->GetRTMPStreamId(), _streamName);
	if (pOutNetRTMPStream == NULL) {
		FATAL("Unable to create the output stream");
		return false;
	}

	//4. Set the timeline head
	pOutNetRTMPStream->SetAbsoluteTimelineHead(_parameters.absoluteTimelineHead);

	//5. Try to create the input file stream
	InFileRTMPStream *pInFileStream = _parameters.pProtocol->CreateIFS(
			_pPlaylist->GetRTMPStreamId(), _metadata);
	if (pInFileStream == NULL) {
		FATAL("Unable to create the input stream");
		return false;
	}

	//6. get and store the metadata
	_publicMetadata.Reset();
	_publicMetadata = _metadata.publicMetadata();
	_publicMetadata[HTTP_HEADERS_SERVER] = BRANDING_BANNER;
	StreamCapabilities *pCapabilities = pInFileStream->GetCapabilities();
	if (pCapabilities != NULL)
		pCapabilities->GetRTMPMetadata(_publicMetadata);

	//7. Link the streams
	if (!pInFileStream->Link(pOutNetRTMPStream)) {
		FATAL("Unable to link input and output streams");
		return false;
	}

	//8. Register it to the signaled streams
	if (!_parameters.pProtocol->SignalReadyForSendOnIFS(_pPlaylist->GetRTMPStreamId())) {
		FATAL("Unable to register input file stream for send events");
		return false;
	}

	//9. Trigger the RTMP events associated with an item streaming
	if (!_pPlaylist->SignalItemStart(this, true, _parameters.startupState,
			_parameters.isSeek)) {
		FATAL("Unable to signal item start");
		return false;
	}

	//10. Start the feeding process
	pInFileStream->SingleGop(_parameters.singleGop);
	if (!pInFileStream->Play(_parameters.playbackStart, _parameters.playbackLength)) {
		FATAL("Unable to start playback on the input stream");
		return false;
	}

	//11. set the linked flag
	_parameters.linked = true;

	//12. Trigger the feed loop
	pInFileStream->ReadyForSend();

	//13. Done
	return true;
}

bool RTMPPlaylistItem::TryPlayLive() {
	//1. reset the linked flag
	_parameters.linked = false;

	//2. Search for the stream
	BaseInStream *pInStream = NULL;
	map<uint32_t, BaseStream *> streams = _parameters.pSM->FindByTypeByName(
			ST_IN_NET, _streamName, true, false);

	FOR_MAP(streams, uint32_t, BaseStream *, i) {
		if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_NET_RTMP)) {
			pInStream = (BaseInStream *) MAP_VAL(i);
			break;
		}
	}
//	WARN("[Debug] pInNetStream is %sNULL", pInStream == NULL ? "" : "not ");
	if (pInStream == NULL) {
		// try device stream
		map<uint32_t, BaseStream *> streams = _parameters.pSM->FindByTypeByName(
				ST_IN_DEVICE, _streamName, true, false);

		FOR_MAP(streams, uint32_t, BaseStream *, i) {
			if (MAP_VAL(i)->IsCompatibleWithType(ST_OUT_NET_RTMP)) {
				pInStream = (BaseInStream *) MAP_VAL(i);
				break;
			}
		}
//		WARN("[Debug] pInDevStream is %sNULL", pInStream == NULL ? "" : "not ");
		if (pInStream == NULL)
			return true;
		else
			_publicMetadata = pInStream->GetPublicMetadata();
	} else {
		_publicMetadata = pInStream->GetPublicMetadata();
	}

//	WARN("[Debug] Trying to get outstream");
	//1. Try to create the out net stream
	OutNetRTMPStream *pOutNetRTMPStream = WaitForLive(false);
	if (pOutNetRTMPStream == NULL) {
		FATAL("Unable to create the output stream");
		return false;
	}
//	WARN("[Debug] Created OutNetRTMPStream");
	//7. Link the streams
	if (!pInStream->Link(pOutNetRTMPStream)) {
		FATAL("Unable to link input and output streams");
		return false;
	}

//	WARN("[Debug] Instream linked to outstream");
	//8. Mark the stream as linked
	_parameters.linked = true;

	//9. Done
	return true;
}

OutNetRTMPStream * RTMPPlaylistItem::WaitForLive(bool doPull) {
	//1. Try to create the out net stream
//	WARN("[Debug] Trying to play live RTMP");
	OutNetRTMPStream *pOutNetRTMPStream = _parameters.pProtocol->CreateONS(this,
			_pPlaylist->GetRTMPStreamId(), _streamName);
	if (pOutNetRTMPStream == NULL) {
		FATAL("Unable to create the output stream");
		return NULL;
	}

	//2. Set the timeline head
	pOutNetRTMPStream->SetAbsoluteTimelineHead(_parameters.absoluteTimelineHead);

	//3. Trigger the RTMP events associated with an item streaming
	if (!_pPlaylist->SignalItemStart(this, false, _parameters.startupState,
			_parameters.isSeek)) {
		FATAL("Unable to signal item start");
		return NULL;
	}

	//4. Set the playback length and singleGop
	pOutNetRTMPStream->SetPlaybackLength(_parameters.playbackLength,
			_parameters.singleGop);

	//5. If this is the origin or we don't need to pull yet, we're done
	if ((!doPull) || (_parameters.pApp->IsOrigin())) {
//		WARN("[Debug] Returning outstream");
		return pOutNetRTMPStream;
	}
	//6. Get the stream from origin
	Variant pullSettings;
	pullSettings["uri"] = "rtmp://localhost:1936/live/" + _requestedStreamName;
	pullSettings["keepAlive"] = (bool)false;
	pullSettings["localStreamName"] = _streamName;
	pullSettings["forceTcp"] = (bool)true;
	pullSettings["swfUrl"] = "";
	pullSettings["pageUrl"] = "";
	pullSettings["ttl"] = (uint32_t) 0x100;
	pullSettings["tos"] = (uint32_t) 0x100;
	pullSettings["width"] = (uint32_t) 0;
	pullSettings["height"] = (uint32_t) 0;
	pullSettings["rangeStart"] = (int64_t) - 2;
	pullSettings["rangeEnd"] = (int64_t) - 1;

	//6. Do the request
	if (!_parameters.pApp->PullExternalStream(pullSettings)) {
		FATAL("Unable to pull stream %s", STR(pullSettings["uri"]));
		return NULL;
	}

	//7. Done
	return pOutNetRTMPStream;
}

void RTMPPlaylistItem::PullRedirectedSource(string nextRmsIp) {
	Variant message;
	message["header"] = "pullStream";
	message["payload"]["command"] = "pullStream";

	//cleanup the parameters
	Variant &parameters = message["payload"]["parameters"];
	parameters.IsArray(false);

	vector<string> streamNameParts;
	split(_requestedStreamName,":",streamNameParts);
	string fileName;
	fileName = streamNameParts.size() == 1 ? streamNameParts[0] : streamNameParts[1];
	if (streamNameParts.size() != 1 && fileName.find("." + streamNameParts[0]) == string::npos) {
		fileName += ("." + streamNameParts[0]);
	}

	string localStreamName = "";
	split(fileName,".",streamNameParts);
	//localStreamName will be the file name of the stored file to be streamed (eg. bunny.mp4 -> localStreamName = bunny)
	localStreamName = streamNameParts[0];
	_streamName = localStreamName;

	string uri = "rtmp://" + nextRmsIp + "/vod/" + fileName;

	parameters["uri"] = uri;
	parameters["localStreamName"] = localStreamName;
	parameters["keepAlive"] = "0";
	parameters["_isVod"] = "1";

	//Broadcast the injected CLI command to whomever has ears to listen
	//This should be intercepted by the rdkcrouter application
	_parameters.pApp->SignalApplicationMessage(message);
}
