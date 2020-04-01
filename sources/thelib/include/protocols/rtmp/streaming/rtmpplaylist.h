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


#ifndef _RTMPPLAYLIST_H
#define	_RTMPPLAYLIST_H

#include "common.h"
#include "mediaformats/readers/streammetadataresolver.h"
#include "protocols/rtmp/streaming/rtmpplayliststate.h"

class RTMPPlaylistItem;
class BaseRTMPProtocol;
class OutNetRTMPStream;
class BaseInStream;

class RTMPPlaylist {
private:
	static map<string, map<uint32_t, RTMPPlaylist *> > _playlists;
	static map<uint32_t, RTMPPlaylist *> _emptyPlaylists;
	string _playlistName;
	const uint32_t _rtmpProtocolId;
	const uint32_t _rtmpStreamId;
	string _clientId;
	vector<RTMPPlaylistItem *> _items;
	uint32_t _currentItemIndex;
	double _absoluteTimelineHead;
	RTMPPlaylistState _state;
	bool _playResetSent;
	bool _switch;
	bool _isLstFile;
	double _aggregateDuration;
	string _originalRequestedStreamName; //used for selective playlistitem 
										 //inserts
public:
	RTMPPlaylist(uint32_t rtmpProtocolId, uint32_t rtmpStreamId);
	virtual ~RTMPPlaylist();
	operator string();

	static const map<uint32_t, RTMPPlaylist *> &GetPlaylists(const string &playlistName);

	uint32_t GetRTMPStreamId();
	BaseRTMPProtocol *GetProtocol();
	OutNetRTMPStream *GetOutStream();
	BaseInStream *GetInStream();
	bool IsListFile();

	bool Play(const string &tempRequestedStreamName, double requestedStart,
			double requestedLength, bool reset);
	bool Switch(const string &requestedOldStreamName,
			const string &requestedStreamName, double requestedOffset,
			double requestedStart, double requestedLen);
	bool InsertItem(const string &streamName, double insertPoint, double offset,
			double duration);
	bool Pause(double dts);
	bool Resume(double dts);
	bool Seek(double absoluteTimestamp);
	bool Stop();
	bool Close(int ___unknown___parameters___);

	bool SignalItemNotFound(RTMPPlaylistItem *pItem, RTMPPlaylistState originatingState);
	bool SignalItemStart(RTMPPlaylistItem *pItem, bool isRecorded,
			RTMPPlaylistState originatingState, bool isSeek);
	bool SignalItemCompleted(RTMPPlaylistItem *pItem,
			double absoluteTimelineHead, bool singleGop, RTMPPlaylistState originatingState);
	bool ResendMetadata(RTMPPlaylistItem *pItem);
	string const &GetOriginalRequestedStreamName();
private:
	void FixPlaylistBounds();
	bool AppendItem(const string &requestedStreamName, double requestedStart,
			double requestedLength, Metadata &metadata, int lineNumber);
	bool StreamExists(const string &requestedStreamName, double requestedStart,
			double requestedLength, Metadata &metadata, bool bypassStreamAlias);
	RTMPPlaylistItem *GetItem(double absoluteTimestamp);
	RTMPPlaylistItem *GetCurrentItem();
	bool HasStream();
	void Cleanup(int lineNumber);
	bool Transition(RTMPPlaylistState newState, int lineNumber);
	bool SendMessage(Variant &message, bool trackresponse, int lineNumber);
	bool SendMetadata(Variant &metadata, double timestamp, bool isAbsolute, int lineNumber);
	void AttachCallbackData(Metadata &metadata);
};


#endif	/* _RTMPPLAYLIST_H */
