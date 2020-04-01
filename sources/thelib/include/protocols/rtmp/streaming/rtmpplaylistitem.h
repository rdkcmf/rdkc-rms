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


#ifndef _RTMPPLAYLISTITEM_H
#define	_RTMPPLAYLISTITEM_H

#include "common.h"
#include "mediaformats/readers/streammetadataresolver.h"
#include "rtmpplayliststate.h"

class RTMPPlaylist;
class BaseClientApplication;
class BaseRTMPProtocol;
class StreamMetadataResolver;
class StreamsManager;
class OutNetRTMPStream;
class BaseStream;

enum RTMPPlaylistItemType {
	LIVE_OR_RECORDED,
	LIVE,
	RECORDED
};

class RTMPPlaylistItem {
public:
	const string _requestedStreamName;
	double _requestedStart;
	double _requestedLength;
	double _playlistStart;
	double _playlistDuration;
	uint32_t _index;
private:
	RTMPPlaylist *_pPlaylist;
	RTMPPlaylistItemType _type;
	Metadata _metadata;
	Variant _publicMetadata;
	string _streamName;

	struct {
		bool singleGop;
		RTMPPlaylistState startupState;
		double playbackStart;
		double playbackLength;
		double absoluteTimelineHead;
		bool isSeek;
		bool linked;
		BaseRTMPProtocol *pProtocol;
		BaseClientApplication *pApp;
		StreamMetadataResolver *pSMR;
		StreamsManager *pSM;
	} _parameters;
public:
	RTMPPlaylistItem(RTMPPlaylist *pPlaylist, const string &requestedStreamName,
			double requestedStart, double requestedLength,
			double playlistDuration, Metadata &metadata);
	virtual ~RTMPPlaylistItem();
	operator string();

	bool ResendMetadata();

	Variant &GetPublicMetadata();
	Metadata &GetAllMetadata();
	double GetPlaylistStart();
	double GetPlaylistDuration();
	void PullRedirectedSource(string nextRmsIp);
	bool ContainsTimestamp(double absoluteTimelineHead);

	bool Play(double absoluteTimelineHead, bool isSeek, RTMPPlaylistState startupState,
			bool singleGop);
	bool Pause();
	bool Resume();

	bool SignalStreamCompleted(double absoluteTimelineHead);
private:
	bool TryPlayRecorded(bool fallBackOnLive);
	bool TryPlayLive();
	OutNetRTMPStream * WaitForLive(bool doPull);
	void SignalLazyPullAction(string action);
};

#endif	/* _RTMPPLAYLISTITEM_H */
