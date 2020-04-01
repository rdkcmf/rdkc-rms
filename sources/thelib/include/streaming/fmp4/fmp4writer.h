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

#ifndef _FMP4WRITER_H
#define	_FMP4WRITER_H

#include "common.h"
#include "utils/buffering/iobufferext.h"

class StreamCapabilities;

typedef enum {
	WS_UNKNOWN = 0,
	WS_MOOV,
	WS_MOOF
} FMP4WriteState;

typedef enum {
	PBT_BEGIN = 0xf9,
	PBT_HEADER_MOOV = 0xfa,
	PBT_HEADER_MOOF = 0xfb,
	PBT_VIDEO_BUFFER = 0xfc,
	PBT_AUDIO_BUFFER = 0xfd,
	PBT_HEADER_MDAT = 0xfe,
	PBT_END = 0xff
} FMP4ProgressiveBufferType;

const char * FMP4ProgressiveBufferTypeToString(FMP4ProgressiveBufferType type);

class FMP4Writer {
private:
	uint64_t _now;
	IOBufferExt _moov;
	IOBufferExt _moof;
	uint32_t _fragmentSequenceNumber;
	uint32_t _offsetFragmentSequenceNumber;
	uint32_t _offsetDuration;
	uint64_t _mdat;
	uint64_t _moofOffset; 
	bool _initialSegmentOpened;
	double _moovStartPts;
	bool _winQtCompat;
	uint32_t _mvhdTimescale;
	double _startAudioTimeTicks;
	double _startVideoTimeTicks;
	int _vidRcv, _audRcv;
	deque<double> _videoDtsDeltas; //max of 200 dts samples for average, first in, first out
	deque<double> _audioDtsDeltas;
	double _aveVideoDtsDelta;
	double _aveAudioDtsDelta;

	Variant _fmp4MetaData;

	struct TrackContext {
		uint32_t _id;
		bool _isAudio;
		uint32_t _trunEntrySize;
		uint32_t _width;
		uint32_t _height;
		uint32_t _samplingRate;
		uint32_t _defaultSampleDuration;
		uint32_t _samplesCount;
		uint32_t _maxSamplesCount;
		uint32_t _trafSize;
		uint32_t _trunSize;
		double _fragmentStartTs;
		uint32_t _lastDuration;
		uint64_t _totalDuration;
		uint32_t _lastDurationDelta;
		uint32_t _sampleSize;

		uint32_t _timescale;
		bool _ignoreLastSample;

		struct {
			bool _enabled;
			int64_t _syncPoint;
			uint32_t _samplesCount;
			double _maxDeviation;
		} _sync;

		struct {
			uint32_t _tfdtBaseMediaDecodeTime;
			uint32_t _trun;
			uint32_t _tfhd;
			uint32_t _lastSampleDuration;
		} _offset;
		IOBufferExt _codecBytes;
		IOBufferExt _trafHeader;
		IOBufferExt _trunBuffer;
		IOBufferExt _mdat;
		TrackContext();
		void Reset();
		void Init();
	} _audio, _video;

	struct {
		bool _enabled;
		size_t _mdatAudioSize;
		size_t _mdatVideoSize;
	} _progressive;

	double _lastKnownVideoPts;
	double _lastKnownVideoDts;
	double _lastKnownAudioPts;
	double _lastKnownAudioDts;

	struct BufferedAudio {
		IOBufferExt buffer;
		double pts;
		double dts;
	};
	vector<BufferedAudio *> _bufferedAudio;
public:
	FMP4Writer();
	virtual ~FMP4Writer();

	virtual bool Initialize(bool enableAudio, bool enableVideo,
			StreamCapabilities *pCapabilities, bool progressive);

	virtual bool WriteVideoData(IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame);
	virtual bool WriteAudioData(IOBuffer &buffer, double pts, double dts);
	bool Flush();

protected:
	virtual bool BeginWriteMOOV() = 0;
	virtual bool EndWriteMOOV(uint32_t durationOffset, bool durationIs64Bits) = 0;
	virtual bool BeginWriteFragment() = 0;
	virtual bool EndWriteFragment(double moovStart, double moovDuration,
			double moofStart) = 0;
	virtual bool WriteProgressiveBuffer(FMP4ProgressiveBufferType type,
			const uint8_t *pBuffer, size_t size) = 0;
	virtual bool WriteBuffer(FMP4WriteState state, const uint8_t *pBuffer, size_t size) = 0;
	virtual bool SignalProtocols(const IOBuffer &buffer, bool isMetadata, void *metadata) = 0;

	// Pause/Resume group of functions.
	bool _isPlayerPaused; // outbound player is paused for specific outboundstream
	bool _stopOutbound; // if true all outbound data will be disregarded
	double _dtsOffset;
	bool _inboundVod;


	// FMP4 Writer Reset function group.
	void _initMembers();
	void _resetFMP4Writer();
	bool EnqueueResetStream(bool enableAudio, bool enableVideo, StreamCapabilities *pCapabilities, bool progressive);
	void ResetDtsValues();
	struct ResetContext { 
		bool isEnqueued;		// set to TRUE if a reset has been requested.
		bool enableAudio;
		bool enableVideo; 
		bool progressive;
		StreamCapabilities *pCap;

		ResetContext();
	} _resetter;

private:
	bool BuildAtomFTYP(IOBufferExt &dst);
	bool BuildAtomMOOV(IOBufferExt &dst);
	bool BuildAtomMVHD(IOBufferExt &dst);
	bool BuildAtomMVEX(IOBufferExt &dst);
	bool BuildAtomTREX(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomTRAK(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomTKHD(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomEDTS(IOBufferExt &dst);
	bool BuildAtomELST(IOBufferExt &dst);
	bool BuildAtomMDIA(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomMDHD(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomHDLR(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomMINF(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomSMHD(IOBufferExt &dst);
	bool BuildAtomVMHD(IOBufferExt &dst);
	bool BuildAtomDINF(IOBufferExt &dst);
	bool BuildAtomDREF(IOBufferExt &dst);
	bool BuildAtomURL(IOBufferExt &dst);
	bool BuildAtomSTBL(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomSTSD(IOBufferExt &dst, TrackContext &track);
	bool BuildAtomSTTS(IOBufferExt &dst);
	bool BuildAtomSTSC(IOBufferExt &dst);
	bool BuildAtomSTSZ(IOBufferExt &dst);
	bool BuildAtomSTCO(IOBufferExt &dst);
	bool BuildAtomMP4A(IOBufferExt &dst);
	bool BuildAtomAVC1(IOBufferExt &dst);
	bool BuildAtomESDS(IOBufferExt &dst);
	bool BuildAtomAVCC(IOBufferExt &dst);
	bool BuildAtomMOOF(IOBufferExt &dst);
	bool BuildAtomMFHD(IOBufferExt &dst);
	bool BuildAtomTRAF(TrackContext &track);
	bool BuildAtomTFHD(TrackContext &track);
	bool BuildAtomTFDT(TrackContext &track);
	bool BuildAtomTRUN(TrackContext &track);
	void SynchronizeTimestamps(TrackContext &track, double dts);
	bool WriteData(TrackContext &track, IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame);
	bool OpenSegment();
	uint32_t GetTracksCount();
	bool OpenFragment();
	bool CloseFragment();
	bool ComputeSize(TrackContext &track);
	bool CloseFragmentTrack(TrackContext &track);
	bool WriteTrackBuffers(TrackContext &track);
	bool NewFragment();

	void RemoveVideoDtsSpikes(double dts, double lastTs, TrackContext &track);
	void RemoveAudioDtsSpikes(double dts, double lastTs, TrackContext &track);
};


#endif	/* _FMP4WRITER_H */
