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


#include "protocols/rtmp/streaming/rtmpplaylist.h"
#include "protocols/rtmp/streaming/rtmpplaylistitem.h"
#include "protocols/rtmp/messagefactories/genericmessagefactory.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/protocolmanager.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"
#include "application/clientapplicationmanager.h"
#include "protocols/rtmp/rtmpmessagefactory.h"
#include "streaming/streamstypes.h"
#include "streaming/basestream.h"
#include "protocols/rtmp/streaming/outnetrtmpstream.h"
#include "streaming/baseinstream.h"

//#define __HAS_DEBUG_MESSAGES
#ifdef __HAS_DEBUG_MESSAGES
#define SHOW_STATE_TRANSITIONS() WARN("*** %4d: %s -> %s", lineNumber, RTMPPlaylistStateToString(_state), RTMPPlaylistStateToString(newState));
#define SHOW_ENTRY_POINT_ITEM_START() WARN("RTMPPlaylist::SignalItemStart: %s. It was %s", STR(RTMPPlaylistStateToString(_state)), STR(RTMPPlaylistStateToString(originatingState)))
#define SHOW_ENTRY_POINT_ITEM_COMPLETED() WARN("RTMPPlaylist::SignalItemCompleted: %s. It was %s; absoluteTimelineHead: %.2f -> %.2f", STR(RTMPPlaylistStateToString(_state)), STR(RTMPPlaylistStateToString(originatingState)),_absoluteTimelineHead,absoluteTimelineHead)
#define SHOW_ENTRY_POINT_ITEM_NOT_FOUND() WARN("RTMPPlaylist::SignalItemNotFound: %s. It was %s", STR(RTMPPlaylistStateToString(_state)), STR(RTMPPlaylistStateToString(originatingState)))
#define SHOW_ENTRY_POINT_PLAY() WARN("RTMPPlaylist::Play: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_ENTRY_POINT_SWITCH() WARN("RTMPPlaylist::Switch: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_ENTRY_POINT_INSERT_ITEM() WARN("RTMPPlaylist::InsertItem: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_ENTRY_POINT_PAUSE() WARN("RTMPPlaylist::Pause: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_ENTRY_POINT_RESUME() WARN("RTMPPlaylist::Resume: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_ENTRY_POINT_SEEK() WARN("RTMPPlaylist::Seek: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_ENTRY_POINT_STOP() WARN("RTMPPlaylist::Stop: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_ENTRY_POINT_CLOSE() WARN("RTMPPlaylist::Close: %s", STR(RTMPPlaylistStateToString(_state)));
#define SHOW_SENT_MESSAGE() FINEST("    %d    \n%s", lineNumber, STR(message.ToString()));
#define SHOW_PLAYLIST(lineNumber) FINEST("    %d    \n%s",lineNumber, STR(*this));
#else /* __HAS_DEBUG_MESSAGES */
#define SHOW_STATE_TRANSITIONS()
#define SHOW_ENTRY_POINT_ITEM_START()
#define SHOW_ENTRY_POINT_ITEM_COMPLETED()
#define SHOW_ENTRY_POINT_ITEM_NOT_FOUND()
#define SHOW_ENTRY_POINT_PLAY()
#define SHOW_ENTRY_POINT_SWITCH();
#define SHOW_ENTRY_POINT_INSERT_ITEM();
#define SHOW_ENTRY_POINT_PAUSE()
#define SHOW_ENTRY_POINT_RESUME()
#define SHOW_ENTRY_POINT_SEEK()
#define SHOW_ENTRY_POINT_STOP()
#define SHOW_ENTRY_POINT_CLOSE()
#define SHOW_SENT_MESSAGE()
#define SHOW_PLAYLIST(lineNumber)
#endif /* __HAS_DEBUG_MESSAGES */

map<string, map<uint32_t, RTMPPlaylist *> > RTMPPlaylist::_playlists;
map<uint32_t, RTMPPlaylist *> RTMPPlaylist::_emptyPlaylists;

const char *RTMPPlaylistStateToString(RTMPPlaylistState state) {
	switch (state) {
		case NOT_STARTED:
			return "NOT_STARTED";
		case PLAYING:
			return "PLAYING";
		case PAUSED:
			return "PAUSED";
		case FINISHED:
			return "FINISHED";
		default:
			return "unknown";
	}
}

RTMPPlaylist::RTMPPlaylist(uint32_t rtmpProtocolId, uint32_t rtmpStreamId)
: _rtmpProtocolId(rtmpProtocolId), _rtmpStreamId(rtmpStreamId) {
	_currentItemIndex = 0;
	BaseRTMPProtocol *pProtocol = GetProtocol();
	if (pProtocol != NULL)
		_clientId = pProtocol->GetClientId();
	_state = NOT_STARTED;
	_playResetSent = false;
	_switch = false;
	_playlistName = "";
	_isLstFile = false;
	_aggregateDuration = -1;
}

RTMPPlaylist::~RTMPPlaylist() {
	MAP_ERASE2(_playlists, _playlistName, _rtmpProtocolId);
	Cleanup(__LINE__);
}

RTMPPlaylist::operator string() {
	string result = "Name: `" + _playlistName + "`\n";
	for (size_t i = 0; i < _items.size(); i++) {
		result += *_items[i];
		if (i == _currentItemIndex)
			result += "(current)";
		if (i < (_items.size() - 1))
			result += "\n";
	}
	return result;
}

const map<uint32_t, RTMPPlaylist *> &RTMPPlaylist::GetPlaylists(const string &playlistName) {
	map<string, map<uint32_t, RTMPPlaylist *> >::iterator i = _playlists.find(playlistName);
	if (i == _playlists.end())
		return _emptyPlaylists;
	return i->second;
}

uint32_t RTMPPlaylist::GetRTMPStreamId() {
	return _rtmpStreamId;
}

BaseRTMPProtocol *RTMPPlaylist::GetProtocol() {
	BaseProtocol *pProtocol = NULL;
	if (((pProtocol = ProtocolManager::GetProtocol(_rtmpProtocolId)) == NULL)
			|| ((pProtocol->GetType() != PT_INBOUND_RTMP)&&(pProtocol->GetType() != PT_OUTBOUND_RTMP))
			|| (pProtocol->IsEnqueueForDelete()))
		return NULL;
	return (BaseRTMPProtocol *) pProtocol;
}

OutNetRTMPStream *RTMPPlaylist::GetOutStream() {
	BaseRTMPProtocol *pProtocol = NULL;
	BaseStream *pStream = NULL;
	if (((pProtocol = GetProtocol()) == NULL)
			|| ((pStream = pProtocol->GetRTMPStream(_rtmpStreamId)) == NULL)
			|| (pStream->GetType() != ST_OUT_NET_RTMP))
		return NULL;
	return (OutNetRTMPStream *) pStream;
}

BaseInStream *RTMPPlaylist::GetInStream() {
	OutNetRTMPStream *pOutStream = GetOutStream();
	if (pOutStream == NULL)
		return NULL;
	return pOutStream->GetInStream();
}

bool RTMPPlaylist::IsListFile() {
	return _isLstFile;
}

string const &RTMPPlaylist::GetOriginalRequestedStreamName() {
	return _originalRequestedStreamName;
}

bool RTMPPlaylist::Play(const string &tempRequestedStreamName, double requestedStart,
		double requestedLength, bool reset) {
	SHOW_ENTRY_POINT_PLAY();
	//1. Do we have a logical stream?
	if (!HasStream()) {
		FATAL("No valid stream id provided");
		return false;
	}

	//find the last ;* 
	string requestedStreamName = _originalRequestedStreamName = tempRequestedStreamName;
	string delim = ";*";
	size_t i = tempRequestedStreamName.find_last_of(delim);
	if (i != string::npos) {
		requestedStreamName = tempRequestedStreamName.substr(i + delim.size() - 1);
	}

	Metadata metadata;
	//apply stream alias if enabled for playlist file
	if (!StreamExists(requestedStreamName, requestedStart, requestedLength, metadata, false)) {
		if (!SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStreamNotFound(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, requestedStreamName), false, __LINE__)) {
			FATAL("Unable to send RTMP messages");
			return false;
		}
		if (GetCurrentItem() == NULL) {
			return SendMessage(RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlayComplete(_rtmpStreamId, _absoluteTimelineHead, true, 0, 0), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, requestedStreamName, ""), false, __LINE__);
		} else {
			return true;
		}
	}

	if (reset)
		Cleanup(__LINE__);

	if (metadata.type() == MEDIA_TYPE_LST) {
		_isLstFile = true;
		if (_playlistName == "") {
			_playlistName = metadata.computedCompleteFileName();
			_playlists[_playlistName][_rtmpProtocolId] = this;
		}

		Variant elements = metadata.publicMetadata()["playlistElements"];
		if (elements == V_NULL) {
			FATAL("Playlist not found");
			return false;
		}
		FOR_MAP(elements, string, Variant, i) {
			metadata.Reset();
			AttachCallbackData(metadata);
			if (!StreamExists(MAP_VAL(i)["name"], (double) MAP_VAL(i)["start"], (double) MAP_VAL(i)["length"], metadata, true))
				continue;
			double start = (double) MAP_VAL(i)["start"];
			if (metadata.type() == MEDIA_TYPE_VOD)
				start = -1000;
			if (!AppendItem(MAP_VAL(i)["name"], start, (double) MAP_VAL(i)["length"], metadata, __LINE__)) {
				FATAL("Unable to add item to playlist");
				return false;
			}
		}

		if (GetCurrentItem() == NULL) {
			return SendMessage(RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlayComplete(_rtmpStreamId, _absoluteTimelineHead, true, 0, 0), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, requestedStreamName, ""), false, __LINE__);
		}
	} else {
		if (metadata.type() == MEDIA_TYPE_VOD)
			requestedStart = -1000;
		if (!AppendItem(requestedStreamName, requestedStart, requestedLength, metadata, __LINE__)) {
			FATAL("Unable to add item to playlist");
			return false;
		}
	}

	//4. Based on the current state, perform certain actions
	switch (_state) {
		case NOT_STARTED:
		{
			if (!Transition(PLAYING, __LINE__)) {
				FATAL("Unable to transition");
				return false;
			}

			RTMPPlaylistItem *pItem = GetCurrentItem();
			if (pItem == NULL) {
				FATAL("Unable to get current item");
				return false;
			}

			return pItem->Play(_absoluteTimelineHead, false, NOT_STARTED, false);
		}
		case PLAYING:
		{
			if (!Transition(PLAYING, __LINE__)) {
				FATAL("Unable to transition");
				return false;
			}

			if (reset) {
				RTMPPlaylistItem *pItem = GetCurrentItem();
				if (pItem == NULL) {
					FATAL("Unable to get current item");
					return false;
				}

				return pItem->Play(_absoluteTimelineHead, false, PLAYING, false);
			}
			return true;
		}
		case PAUSED:
		{
			if (!Transition(PAUSED, __LINE__)) {
				FATAL("Unable to transition");
				return false;
			}

			if (reset) {
				RTMPPlaylistItem *pItem = GetCurrentItem();
				if (pItem == NULL) {
					FATAL("Unable to get current item");
					return false;
				}
				return pItem->Play(_absoluteTimelineHead, false, PAUSED, true);
			}
			return true;
		}
		case FINISHED:
		{
			if (!Transition(PLAYING, __LINE__)) {
				FATAL("Unable to transition");
				return false;
			}

			RTMPPlaylistItem *pItem = GetCurrentItem();
			if (pItem == NULL) {
				FATAL("Unable to get current item");
				return false;
			}

			return pItem->Play(_absoluteTimelineHead, false, FINISHED, false);
		}
		default:
		{
			FATAL("Invalid RTMP playlist state detected");
			return false;
		}
	}
}

bool RTMPPlaylist::Switch(const string &requestedOldStreamName,
		const string &requestedStreamName, double requestedOffset,
		double requestedStart, double requestedLen) {
	SHOW_ENTRY_POINT_SWITCH();

	//1. Get the metadata
	Metadata metadata;
	if (!StreamExists(requestedStreamName, requestedStart, requestedLen, metadata, false)) {
		if (!SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStreamNotFound(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, requestedStreamName), false, __LINE__)) {
			FATAL("Unable to send RTMP messages");
			return false;
		}
		return true;
	}
	if (metadata.mediaFullPath() != "")
		requestedStart = requestedOffset;

	//2. Get the protocol
	BaseRTMPProtocol *pProtocol = GetProtocol();
	if (pProtocol == NULL) {
		FATAL("Unable to get the protocol");
		return false;
	}

	//3. Get the stream
	BaseStream *pBaseStream = pProtocol->GetRTMPStream(_rtmpStreamId);
	if ((pBaseStream == NULL) || (pBaseStream->GetType() != ST_OUT_NET_RTMP)) {
		FATAL("Unable to get the output stream");
		return false;
	}
	//OutNetRTMPStream *pOutStream = (OutNetRTMPStream *) pBaseStream;

	//4. Get the last sent timestamp
	_absoluteTimelineHead = requestedOffset * 1000; //pOutStream->GetLastSentTimestamp();
	//	ASSERT("_absoluteTimelineHead: %.2f; requestedOffset: %.2f",
	//			_absoluteTimelineHead, requestedOffset);

	//5. Terminate the current stream
	pProtocol->CloseStream(_rtmpStreamId, true);

	//6. Scan the entire playlist and replace all occurrences of oldStreamName
	//with the new streamName. If oldStreamName is "", than only do that for the
	//first item
	vector<RTMPPlaylistItem *> oldItems = _items;
	_items.clear();
	uint32_t itemIndex = _currentItemIndex;
	for (uint32_t i = 0; i < oldItems.size(); i++) {
		if (((requestedOldStreamName == "")&&(i == 0))
				|| (requestedOldStreamName == oldItems[i]->_requestedStreamName)) {
			AppendItem(requestedStreamName, requestedStart, requestedLen, metadata,
					__LINE__);
			itemIndex = i;
		} else {
			AppendItem(oldItems[i]->_requestedStreamName,
					oldItems[i]->_requestedStart, oldItems[i]->_requestedLength,
					oldItems[i]->GetAllMetadata(), __LINE__);
		}
		delete oldItems[i];
	}

	//7. Fix the _currentItemIndex
	if (itemIndex >= _items.size())
		itemIndex = (uint32_t) _items.size() - 1;
	_currentItemIndex = itemIndex;

	//8. If we were paused or playing, put the current item back in that mode
	switch (_state) {
		case PLAYING:
		case PAUSED:
		{
			//9. Get the current item
			RTMPPlaylistItem *pItem = GetCurrentItem();
			if (pItem == NULL) {
				FATAL("Unable to get current item");
				return false;
			}

			//10. Make sure e send the proper messages, not the normal playback
			_switch = true;

			//11. Send the messages and do the play
			return SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayTransition(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName, true), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlayTransitionComplete(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetNotifyRtmpSampleAccess(_rtmpStreamId, _absoluteTimelineHead, true, false, false), false, __LINE__)
					&& SendMetadata(pItem->GetPublicMetadata(), _absoluteTimelineHead, true, __LINE__)
					&& pItem->Play(_absoluteTimelineHead, false, _state, _state == PAUSED);
		}
		case NOT_STARTED:
		case FINISHED:
		default:
		{
			FATAL("Invalid RTMP playlist state detected");
			return false;
		}
	}
}

bool RTMPPlaylist::InsertItem(const string &streamName, double insertPoint,
	double offset, double duration) {
	SHOW_ENTRY_POINT_INSERT_ITEM();

	//1. Get the protocol
	BaseRTMPProtocol *pProtocol = GetProtocol();
	if (pProtocol == NULL) {
		FATAL("Unable to get the protocol");
		return false;
	}

	//2. Validate the input
	if ((offset < 0) && (offset != -1000) && (offset != -2000))
		offset = -2000;
	if ((duration < 0) && (duration != -1000))
		duration = -1000;

	//3. get the instant time head of the playlist
	double timeHead = -1;
	OutNetRTMPStream *pOutStream = GetOutStream();
	if (pOutStream != NULL)
		timeHead = pOutStream->GetLastSentTimestamp();

	//4. Adjust/determine the insert point
	insertPoint = insertPoint >= 0 ? insertPoint : (timeHead < 0 ? 0 : timeHead);
	if ((pOutStream != NULL) && (insertPoint < pOutStream->GetLastSentTimestamp())) {
		FATAL("Timeline past the insert point");
		return false;
	}

	//5. Get the metadata
	Metadata metadata;
	AttachCallbackData(metadata);
	if (!StreamExists(streamName, offset, duration, metadata, false)) {
		FATAL("Stream %s not found", STR(streamName));
	}

	//6. Get the new item real duration
	double playlistDuration = -1;
	if (duration >= 0) {
		playlistDuration = duration;
	} else {
		playlistDuration = metadata.publicMetadata().duration();
		if (playlistDuration > 0)
			playlistDuration *= 1000.0;
	}

	/*
	* |   - begin/end of a playlist item
	* --> - known duration
	* ... - unknown duration (infinite item)
	* #   - the insertion point
	*
	* 1. insertPoint is at a border of 2 existing items
	* #|------>|------>|
	* |#------>|------>|
	* |------>#|------>|
	* |------>|#------>|
	* |------>|------>#|
	* |------>|------>|#
	*
	* 2. insertPoint in the middle of an item which has duration
	* |---#--->|------>|
	*
	*
	* 3. insertPoint in the middle of an item without duration (infinite item)
	* |------>|..#.|
	* |..#.|
	*
	* 4. insertPoint long past the end of the last item.
	* |------>|     #
	*/

	SHOW_PLAYLIST(__LINE__);
	//7. Find the item we want to split
	RTMPPlaylistItem *pItem = NULL;
	for (uint32_t i = 0; i < _items.size(); i++) {
		if (!_items[i]->ContainsTimestamp(insertPoint))
			continue;
		pItem = _items[i];
		break;
	}

	//8. If we didn't find any item to split, that means this is beyond last item
	//In this case, just insert the item and we are done
	if (pItem == NULL) {
		_items.push_back(new RTMPPlaylistItem(this, streamName, offset, duration,
			playlistDuration, metadata));
		FixPlaylistBounds();
		SHOW_PLAYLIST(__LINE__);
		return true;
	}

	//9. Close the stream if this is the current item
	bool isCurrentItem = pItem->_index == _currentItemIndex;
	if (isCurrentItem) {
		pProtocol->CloseStream(_rtmpStreamId, true);
		if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId,
			0, false), false, __LINE__)) {
			FATAL("Unable to send the EOF signal");
			return false;
		}
	}

	if (insertPoint == pItem->_playlistStart) {
		//10. We landed on an item boundary. We will only insert the new item
		_items.insert(_items.begin() + pItem->_index, new RTMPPlaylistItem(this,
			streamName, offset, duration, playlistDuration, metadata));
		FixPlaylistBounds();
		SHOW_PLAYLIST(__LINE__);
	} else {
		//11. We are in the middle of an item. First, see how much did we played
		//from it
		double alreadyPlayed = timeHead >= 0 ? timeHead - pItem->_playlistStart : 0;

		if (alreadyPlayed == 0) {
			//12. if we didn't yet played anything (waiting for a live stream, or
			//just delivered a frame in PAUSE mode, etc), than we simply add the item
			//and we are done
			_items.insert(_items.begin() + pItem->_index, new RTMPPlaylistItem(this,
				streamName, offset, duration, playlistDuration, metadata));
			FixPlaylistBounds();
			SHOW_PLAYLIST(__LINE__);
		} else {
			//14. The item we want to split has known duration. because of
			//this, we can actually split it
			double leftoversOffset = pItem->_requestedStart;
			if (leftoversOffset >= 0)
				leftoversOffset += alreadyPlayed;

			double leftoversDuration = pItem->_playlistDuration;
			if (leftoversDuration >= 0)
				leftoversDuration -= alreadyPlayed;

			RTMPPlaylistItem *pLeftovers = new RTMPPlaylistItem(this,
				pItem->_requestedStreamName, leftoversOffset,
				leftoversDuration, leftoversDuration, pItem->GetAllMetadata());
			RTMPPlaylistItem *pNewItem = new RTMPPlaylistItem(this,
				streamName, offset, duration, playlistDuration, metadata);

			pItem->_playlistDuration = alreadyPlayed;
			pItem->_requestedLength = alreadyPlayed;
			_items.insert(_items.begin() + pItem->_index + 1, pNewItem);
			_items.insert(_items.begin() + pItem->_index + 2, pLeftovers);
			FixPlaylistBounds();
			_currentItemIndex++;
			SHOW_PLAYLIST(__LINE__);
		}
	}

	//15. All done if we didn't interrupt anything
	if (!isCurrentItem) {
		_currentItemIndex--;
		return InsertItem(streamName, insertPoint, offset, duration);
	}

	//16. Resume the previous state
	pItem = GetCurrentItem();
	if (pItem == NULL) {
		FATAL("Unable to get the current item");
		return false;
	}

	return pItem->Play(pItem->_playlistStart, false, _state, _state == PAUSED);
}

bool RTMPPlaylist::Pause(double dts) {
	SHOW_ENTRY_POINT_PAUSE();
	//1. Do we have a logical stream?
	if (!HasStream()) {
		FATAL("No valid stream id provided");
		return false;
	}

	//2. Based on the current state, perform certain actions
	switch (_state) {
		case NOT_STARTED:
		{
			//nothing much to do here. Just transition to paused state
			//and wait for the resume call
			return Transition(PAUSED, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPauseNotify(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, ""), false, __LINE__);
		}
		case PLAYING:
		{
			RTMPPlaylistItem *pItem = GetCurrentItem();
			if (pItem == NULL) {
				FATAL("Unable to get current item");
				return false;
			}
			return Transition(PAUSED, __LINE__)
					&& pItem->Pause()
					&& SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPauseNotify(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName), false, __LINE__);
		}
			//case NOT_STARTED_PAUSED:
		case PAUSED:
		{
			RTMPPlaylistItem *pItem = GetCurrentItem();
			const string streamName = pItem == NULL ? "" : pItem->_requestedStreamName;
			return SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPauseNotify(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, streamName), false, __LINE__);
		}
		case FINISHED:
		{
			RTMPPlaylistItem *pItem = GetCurrentItem();
			const string streamName = pItem == NULL ? "" : pItem->_requestedStreamName;
			return Transition(PAUSED, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPauseNotify(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, streamName), false, __LINE__);
		}
		default:
		{
			FATAL("Invalid RTMP playlist state detected");
			return false;
		}
	}
}

bool RTMPPlaylist::Resume(double dts) {
	SHOW_ENTRY_POINT_RESUME();
	//1. Do we have a logical stream?
	if (!HasStream()) {
		FATAL("No valid stream id provided");
		return false;
	}

	//2. Based on the current state, perform certain actions
	switch (_state) {
		case NOT_STARTED:
		{
			return SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, "", ""), false, __LINE__);
		}
		case PLAYING:
		{
			BaseInStream *pInStream = GetInStream();
			if (pInStream == NULL) {
				FATAL("Unable to get out stream");
				return false;
			}

			if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)) {
				FATAL("Unable to send RTMP messages");
				return false;
			}

			if (TAG_KIND_OF(pInStream->GetType(), ST_IN_FILE)) {
				if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamIsRecorded(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)) {
					FATAL("Unable to send RTMP messages");
					return false;
				}
			}
			RTMPPlaylistItem *pItem = GetCurrentItem();
			if (pItem == NULL) {
				FATAL("Unable to get current item");
				return false;
			}
			return SendMessage(RTMPMessageFactory::GetUsrCtrlStreamBegin(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamUnpauseNotify(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStart(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetNotifyRtmpSampleAccess(_rtmpStreamId, _absoluteTimelineHead, true, false, false), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetNotifyOnStatusNetStreamDataStart(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
					&& SendMetadata(pItem->GetPublicMetadata(), _absoluteTimelineHead, true, __LINE__);
		}
		case PAUSED:
		{
			RTMPPlaylistItem *pItem = GetCurrentItem();

			if (pItem != NULL) {
				return Transition(PLAYING, __LINE__)
						&& pItem->Resume();
			} else {
				return Transition(FINISHED, __LINE__)
						&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, "", ""), false, __LINE__);
			}
		}
		case FINISHED:
		{
			BaseInStream *pInStream = GetInStream();
			if (pInStream == NULL) {
				FATAL("Unable to get out stream");
				return false;
			}

			if (TAG_KIND_OF(pInStream->GetType(), ST_IN_FILE)) {
				if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamIsRecorded(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)) {
					FATAL("Unable to send RTMP messages");
					return false;
				}
			}

			RTMPPlaylistItem *pItem = GetCurrentItem();
			const string streamName = pItem == NULL ? "" : pItem->_requestedStreamName;

			return SendMessage(RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlayComplete(_rtmpStreamId, _absoluteTimelineHead, true, 0, 0), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
					&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, streamName, ""), false, __LINE__);
		}
		default:
		{
			FATAL("Invalid RTMP playlist state detected");
			return false;
		}
	}
}

bool RTMPPlaylist::Seek(double absoluteTimestamp) {
	SHOW_ENTRY_POINT_SEEK();
	//1. Do we have a logical stream?
	if (!HasStream()) {
		FATAL("No valid stream id provided");
		return false;
	}

	if (_state == NOT_STARTED)
		return SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamSeekFailed(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, absoluteTimestamp), false, __LINE__);

	if ((_state != PLAYING)&&(_state != PAUSED)&&(_state != FINISHED)) {
		FATAL("Invalid RTMP playlist state detected: %d", _state);
		return false;
	}

	BaseRTMPProtocol *pProtocol = GetProtocol();
	if (pProtocol == NULL) {
		FATAL("Unable to get the protocol");
		return false;
	}

	RTMPPlaylistItem *pItem = GetItem(absoluteTimestamp);
	if (pItem == NULL) {
		FATAL("Unable to get current item");
		return false;
	}
	SHOW_PLAYLIST(__LINE__);

	_absoluteTimelineHead = absoluteTimestamp;

	if (!pProtocol->CloseStream(_rtmpStreamId, true)) {
		FATAL("Unable to close existing stream");
		return false;
	}

	if (_state == PLAYING) {
		if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)) {
			FATAL("Unable to send RTMP messages");
			return false;
		}
	}

	return pItem->Play(_absoluteTimelineHead, true, _state, _state == PAUSED);
}

bool RTMPPlaylist::Stop() {
	SHOW_ENTRY_POINT_STOP();
	if ((_state == NOT_STARTED) || (_state == FINISHED) || (_items.size() == 0)) {
		Cleanup(__LINE__);
		return true;
	}
	RTMPPlaylistItem *pItem = GetCurrentItem();
	const string streamName = pItem == NULL ? "" : pItem->_requestedStreamName;
	Cleanup(__LINE__);
	return SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
			&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, streamName, ""), false, __LINE__);
}

bool RTMPPlaylist::Close(int ___unknown___parameters___) {
	SHOW_ENTRY_POINT_CLOSE();
	Cleanup(__LINE__);
	return true;
}

bool RTMPPlaylist::SignalItemNotFound(RTMPPlaylistItem *pItem,
		RTMPPlaylistState originatingState) {
	SHOW_ENTRY_POINT_ITEM_NOT_FOUND();
	NYIR;
}

bool RTMPPlaylist::SignalItemStart(RTMPPlaylistItem *pItem, bool isRecorded,
		RTMPPlaylistState originatingState, bool isSeek) {
	SHOW_ENTRY_POINT_ITEM_START();
	//0. Skip this stem if this is a switch
	if (_switch) {
		_switch = false;
		return true;
	}
	
	GetProtocol()->SignalPlaylistItemStart(pItem->_index, (uint32_t) _items.size(), _playlistName);
	//1. Do we have a logical stream?
	if (!HasStream()) {
		FATAL("No valid stream id provided");
		return false;
	}

	if (_isLstFile) {
		if (_aggregateDuration > 0) {
			if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamIsRecorded(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)) {
				FATAL("Unable to send RTMP messages");
				return false;
			}
		}
	} else if (isRecorded) {
		if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamIsRecorded(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)) {
			FATAL("Unable to send RTMP messages");
			return false;
		}
	}

	if (!_playResetSent) {
		_playResetSent = true;
		if ((!SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayReset(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName), false, __LINE__))
				|| (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamBegin(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__))) {
			FATAL("Unable to send RTMP messages");
			return false;
		}
	} else {
		if (originatingState == PAUSED) {
			if ((!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamBegin(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__))
					|| (!SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamUnpauseNotify(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName), false, __LINE__))) {
				FATAL("Unable to send RTMP messages");
				return false;
			}
			return true;
		} else if ((originatingState == PLAYING) || (originatingState == FINISHED)) {
			if (isSeek) {
				if ((!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamBegin(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__))
						|| (!SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamSeekNotify(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName, _absoluteTimelineHead), false, __LINE__))) {
					FATAL("Unable to send RTMP messages");
					return false;
				}
			} else {
				if (!SendMessage(RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlaySwitch(_rtmpStreamId, _absoluteTimelineHead, true, 0, 0), false, __LINE__)) {
					FATAL("Unable to send RTMP messages");
					return false;
				}
			}
		} else {
			FATAL("Invalid RTMP playlist state detected: %d", originatingState);
			return false;
		}
	}

	return SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStart(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName), false, __LINE__)
			&& SendMessage(RTMPMessageFactory::GetNotifyRtmpSampleAccess(_rtmpStreamId, _absoluteTimelineHead, true, false, false), false, __LINE__)
			&& SendMessage(RTMPMessageFactory::GetNotifyOnStatusNetStreamDataStart(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
			&& SendMetadata(pItem->GetPublicMetadata(), _absoluteTimelineHead, true, __LINE__);
}

bool RTMPPlaylist::SignalItemCompleted(RTMPPlaylistItem *pItem,
		double absoluteTimelineHead, bool singleGop,
		RTMPPlaylistState originatingState) {
	SHOW_ENTRY_POINT_ITEM_COMPLETED();
	//1. Do we have a logical stream?
	if (!HasStream()) {
		FATAL("No valid stream id provided");
		return false;
	}

	//2. Save the absoluteTimelineHead
	_absoluteTimelineHead = absoluteTimelineHead;

	//4. if we are in PAUSED state, stay put
	if (_state == PAUSED) {
		//3. Send EOF
		if (!SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)) {
			FATAL("Unable to send RTMP messages");
			return false;
		}
		return true;
	}

	//5. Alright, we should be playing now
	if ((_state != PLAYING)&&(_state != FINISHED)) {
		FATAL("Invalid RTMP playlist state detected");
		return false;
	}

	//6. Increment current item index
	_currentItemIndex++;

	BaseRTMPProtocol *pProtocol = NULL;
	if (((pProtocol = GetProtocol()) == NULL)
			|| (!pProtocol->CloseStream(_rtmpStreamId, true))) {
		FATAL("Unable to close the stream");
		return false;
	}

	if (GetCurrentItem() != NULL) {
		//7. We have an item to play
		return SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
				&& GetCurrentItem()->Play(_absoluteTimelineHead, false, PLAYING, false);
	} else {
		//8. No more items
		return Transition(FINISHED, __LINE__)
				&& SendMessage(RTMPMessageFactory::GetNotifyOnPlayStatusNetStreamPlayComplete(_rtmpStreamId, _absoluteTimelineHead, true, 0, 0), false, __LINE__)
				&& SendMessage(RTMPMessageFactory::GetUsrCtrlStreamEof(_rtmpStreamId, _absoluteTimelineHead, true), false, __LINE__)
				&& SendMessage(RTMPMessageFactory::GetInvokeOnStatusNetStreamPlayStop(_rtmpStreamId, _absoluteTimelineHead, true, _clientId, pItem->_requestedStreamName, ""), false, __LINE__);
	}
}

bool RTMPPlaylist::ResendMetadata(RTMPPlaylistItem *pItem) {
	return SendMetadata(pItem->GetPublicMetadata(), 0, false, __LINE__);
}

void RTMPPlaylist::FixPlaylistBounds() {
	double duration = 0;
	for (uint32_t i = 0; i < _items.size(); i++) {
		_items[i]->_index = i;
		_items[i]->_playlistStart = duration;
		if ((_items[i]->_playlistDuration >= 0)&&(duration >= 0))
			duration += _items[i]->_playlistDuration;
		else
			duration = -1;
	}
	if (_isLstFile)
		_aggregateDuration = duration;
}

bool RTMPPlaylist::AppendItem(const string &requestedStreamName,
		double requestedStart, double requestedLength, Metadata &metadata,
		int lineNumber) {

	double playlistDuration = -1;
	if (requestedLength >= 0) {
		playlistDuration = requestedLength;
	} else {
		playlistDuration = metadata.publicMetadata().duration();
		if (playlistDuration > 0)
			playlistDuration *= 1000.0;
	}

	ADD_VECTOR_END(_items, new RTMPPlaylistItem(this, requestedStreamName,
			requestedStart, requestedLength, playlistDuration, metadata));
	FixPlaylistBounds();

	SHOW_PLAYLIST(lineNumber);
	return true;
}

bool RTMPPlaylist::StreamExists(const string &requestedStreamName,
		double requestedStart, double requestedLength, Metadata &metadata,
		bool bypassStreamAlias) {
	BaseRTMPProtocol *pProtocol = NULL;
	BaseClientApplication *pApp = NULL;
	StreamMetadataResolver *pSMR = NULL;
	if (((pProtocol = GetProtocol()) == NULL)
			|| ((pApp = pProtocol->GetApplication()) == NULL)
			|| ((pSMR = pApp->GetStreamMetadataResolver()) == NULL)) {
		FATAL("Unable to get the streams manager");
		return false;
	}

	//apply stream alias to playlist file or stream not belonging to a playlist file
	string streamName;
	if (bypassStreamAlias) {
		streamName = requestedStreamName;
	} else {
		streamName = pApp->GetStreamNameByAlias(requestedStreamName, false);
		if (streamName == "") {
			FATAL("Stream name alias value not found: %s", STR(requestedStreamName));
			return false;
		}
	}

	if (requestedStart == -2000) {
		if (!pSMR->ResolveMetadata(streamName, metadata, false))
			metadata.publicMetadata().duration(-1);
		return true;
	} else if (requestedStart == -1000) {
		metadata.publicMetadata().duration(-1);
		return true;
	} else {
		return pSMR->ResolveMetadata(streamName, metadata, true);
	}
}

RTMPPlaylistItem *RTMPPlaylist::GetItem(double absoluteTimestamp) {
	if (_items.size() == 0)
		return NULL;
	for (size_t i = 0; i < _items.size(); i++) {
		if (_items[i]->ContainsTimestamp(absoluteTimestamp)) {
			_currentItemIndex = (uint32_t) i;
			return GetCurrentItem();
		}
	}
	_currentItemIndex = (uint32_t) (_items.size() - 1);
	return GetCurrentItem();
}

RTMPPlaylistItem *RTMPPlaylist::GetCurrentItem() {
	if (_currentItemIndex >= _items.size())
		return NULL;
	return _items[_currentItemIndex];
}

bool RTMPPlaylist::HasStream() {
	BaseRTMPProtocol *pProtocol = GetProtocol();
	if (pProtocol == NULL)
		return false;
	return pProtocol->HasRTMPStream(_rtmpStreamId);
}

void RTMPPlaylist::Cleanup(int lineNumber) {
	for (size_t i = 0; i < _items.size(); i++) {
		delete _items[i];
	}
	_items.clear();
	_currentItemIndex = 0;
	_absoluteTimelineHead = 0;
	BaseRTMPProtocol *pProtocol = GetProtocol();
	if (pProtocol != NULL)
		pProtocol->CloseStream(_rtmpStreamId, true);
	_playResetSent = false;
	_switch = false;
	_isLstFile = false;
	_aggregateDuration = -1;
	Transition(NOT_STARTED, lineNumber);
}

bool RTMPPlaylist::Transition(RTMPPlaylistState newState, int lineNumber) {
	SHOW_STATE_TRANSITIONS();
	_state = newState;
	return true;
}

bool RTMPPlaylist::SendMessage(Variant &message, bool trackresponse, int lineNumber) {
	BaseRTMPProtocol *pProtocol = NULL;
	BaseClientApplication *pApp = NULL;
	BaseRTMPAppProtocolHandler *pHandler = NULL;
	if (((pProtocol = GetProtocol()) == NULL)
			|| ((pApp = pProtocol->GetApplication()) == NULL)
			|| ((pHandler = (BaseRTMPAppProtocolHandler *) pApp->GetProtocolHandler(pProtocol)) == NULL)
			|| (!pHandler->SendRTMPMessage(pProtocol, message, trackresponse))
			) {
		Cleanup(lineNumber);
		if (pProtocol != NULL)
			pProtocol->EnqueueForDelete();
		return false;
	}

	SHOW_SENT_MESSAGE();
	return true;
}

bool RTMPPlaylist::SendMetadata(Variant &metadata, double timestamp, bool isAbsolute, int lineNumber) {
	if (metadata == V_NULL)
		return true;
	if (!_isLstFile)
		return SendMessage(RTMPMessageFactory::GetNotifyOnMetaData(_rtmpStreamId,
			timestamp, isAbsolute, metadata), false, lineNumber);
	if (_aggregateDuration > 0)
		metadata["duration"] = _aggregateDuration / 1000.0;
	else
		metadata.RemoveKey("duration", false);

	return SendMessage(RTMPMessageFactory::GetNotifyOnMetaData(_rtmpStreamId,
			timestamp, isAbsolute, metadata), false, lineNumber);
}

void RTMPPlaylist::AttachCallbackData(Metadata &metadata) {
	BaseClientApplication *pApp = NULL;
	if (GetProtocol() != NULL && ((pApp = GetProtocol()->GetApplication()) != NULL)) {
		metadata["application"] = pApp->GetName();
		metadata["callback"]["protocolID"] = _rtmpProtocolId;
		metadata["callback"]["rtmpStreamID"] = _rtmpStreamId;
	}
}
