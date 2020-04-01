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


#ifdef HAS_PROTOCOL_HLS
#ifndef _HLSPLAYLIST_H
#define	_HLSPLAYLIST_H

#include "common.h"
#include "streaming/streamcapabilities.h"
#ifdef HAS_PROTOCOL_DRM
#include "protocols/drm/drmkeys.h"
#endif /* HAS_PROTOCOL_DRM */

class FileSegment;
class EventLogger;
#ifdef HAS_STREAM_DEBUG
class StreamDebugFile;
#endif /* HAS_STREAM_DEBUG */

#define PAYLOAD_LENGTH_FACTOR 1250000 // bytes per second

class DLLEXP HLSPlaylist {
private:
	string _targetFolder;
	bool _overwriteDestination;
	bool _hlsResume;
	string _playlistPath;
	string _chunkBaseName;
	string _origChunkBaseName;
	uint32_t _playlistLength;
	uint32_t _chunkLength;
    uint32_t _targetDuration;
	bool _appending;
	bool _chunkOnIDR;
	string _masterPlaylistPath;
	string _localStreamName;
	string _playlistName;
	uint32_t _bandwidth;
	StreamCapabilities *_pCapabilities;
	FileSegment *_pCurrentSegment;
	double _segmentStartTs;
	double _firstSegmentStartTs;
    double _fileStartTs;
	double _segmentLastTs;
	uint64_t _segmentSequence;
	File _playlistFile;
	uint32_t _audioCount;
	double _audioStart;
	double _lastAudioComputedTs;
	double _playlistCreationTime;
	double _groupTimestamp;
	uint32_t _staleRetentionCount;
	bool _createMasterPlaylist;
	bool _masterPlaylistInitialized;
	bool _cleanupDestination;
	bool _discontinuity;
	double _minimumChunkSize;
	string _streamInfo;
	uint32_t _maxChunkLength;
	bool _useSystemTime;
	uint32_t _offsetTime;
	bool _cleanupOnClose;

	IOBuffer _segmentVideoBuffer;
	IOBuffer _segmentAudioBuffer;
	IOBuffer _segmentPatPmtAndCountersBuffer;

	map<uint64_t, Variant> _rollingItems;
	map<uint64_t, Variant> _staleItems;

	bool _waitForKeyFrame;

	uint8_t _version;
	bool _hasMilliseconds;
	bool _hasPlaylistType;
	uint64_t _playlistTypeCursor;
	//Encrypt segments
	string _drmType;
	int _AESKeyCount;
	unsigned char _currentKey[16]; //128 bits
	unsigned char _currentIV[16];

	uint64_t _maxChunkSize;
#ifdef HAS_PROTOCOL_DRM
	uint32_t _drmId;
	string _drmKey;
	DRMKeys *_pDRMQueue;
	string _keyURL;
#endif /* HAS_PROTOCOL_DRM */

	EventLogger *_pEventLogger;
	Variant _chunkInfo;
#ifdef HAS_STREAM_DEBUG
	StreamDebugFile *_pStreamDebugFile;
#endif /* HAS_STREAM_DEBUG */

	Variant _settings;
	int _currentKeyCount; //number of files using the current key
	string _keyFilename;

	bool _audioOnly;

	bool _instreamDetached;
    uint64_t _targetDurationCursor; //the file position of the value of the EXT-X-TARGETDURATION
    bool _useByteRange;
    uint32_t _fileLength;
	uint32_t _startOffset;
    bool _isLastFragment;
    uint64_t _playlistSequence;
public:
	HLSPlaylist();
	virtual ~HLSPlaylist();
	string GetSystemTime();
	bool Init(Variant &_settings, StreamCapabilities *pCapabilities,
		string streamInfo, uint32_t streamId);
	bool PushVideoData(IOBuffer &buffer, double pts, double dts, Variant& videoParamsSizes);
	bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	bool PushMetaData(string const& vmfMetadata, int64_t pts);
	static void RemoveFromMasterPlaylist(Variant &streamConfig);
	void ForceCut();
	void SetEventLogger(EventLogger *eventLogger);
	bool AppendToChildPlaylist(File &playlistfile);
	void UpdateSegmentSequence(string const &buf);
	bool RefreshRollingList(string const &buf);
	void SetStreamRemoved(bool instreamDetached);
	void RefreshStaleList(vector<string> &filesList);
#ifdef HAS_PROTOCOL_DRM
	bool SetDRMQueue(DRMKeys *pDRMQueue);
#endif /* HAS_PROTOCOL_DRM */
private:
	bool OpenSegment(double pts, double dts);
	bool CloseSegment(bool endOfPlaylist);
	bool CloseSegmentAppending(bool endOfPlaylist);
	bool CloseSegmentRolling(bool endOfPlaylist);
	bool CreateNewSegment(double pts, double dts);
	bool ClosePlaylist();
	bool InitMasterPlaylist();
	static map<string, string> ParsePlaylist(string path);
	static bool SaveMasterPlaylist(map<string, string> &playlist, string path);
	uint32_t GetBandwidth();
	uint32_t GetWidth();
	uint32_t GetHeight();
	string GetProgramDateTime(double segmentStartTs);
	bool PrepareNewKey();
	void SetVersion(uint8_t version);
	bool ForceNewChunk(double pts);
	void movePlaylist(string const& src, string const& dest);
};
#endif	/* _HLSPLAYLIST_H */
#endif /* HAS_PROTOCOL_HLS */
