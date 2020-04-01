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


#ifdef HAS_PROTOCOL_HDS
#ifndef _OUTFILEHDSSTREAM_H
#define	_OUTFILEHDSSTREAM_H

#include  "streaming/baseoutfilestream.h"

class BaseClientApplication;

class Atom_abst;
class HDSFragment;
class HDSManifest;

class DLLEXP OutFileHDSStream
: public BaseOutFileStream {
private:
	Variant _settings;
	//audio
	int64_t _audioRate;

	//video
	int64_t _videoRate;

	GenericProcessDataSetup *_pGenericProcessDataSetup;

	double _fragmentStartupTime;
	bool _chunkOnIDR;
	HDSFragment *_pFragment;
	HDSManifest *_pManifest;
	uint32_t _fragmentSequence;
	double _chunkLength;
	string _targetFolder;
	string _chunkBaseName;

	// ABST atom (which is also used for the bootstrap)
	Atom_abst* _pAbstBox;

	// Filename of the bootstrap to be used
	string _bootstrapFilename;
	string _bootstrapFilePath;

	// Used for the bootstrap atom/box entries behaviour
	bool _isLastFragment;
	bool _isFirstFragment;

	// Indicates the fragments can be be overwritten or not
	bool _overwriteDestination;

	// Indicates if HDS is of rolling type (fragments generated are limited)
	bool _isRolling;
	uint32_t _playlistLength;
	uint32_t _staleRetentionCount;
	std::queue<string> _retainedFragments;

	//double _lastAudioTS;
	bool _audioCodecsSent;
	bool _videoCodecsSent;

	Variant _chunkInfo;
private:
	OutFileHDSStream(BaseProtocol *pProtocol, string name, Variant &settings);

	bool OpenFragment(double pts, double dts, double additionalFramesPercent);
	bool CloseFragment(double lastTS);
	bool NewFragment(double pts, double dts, bool forced);
	bool InsertSequenceHeaders(double videoTS, double audioTS);

	// Writes an external bootstrap file using the contents of the ABST atom
	bool WriteBootstrapFile();
public:
	static OutFileHDSStream* GetInstance(BaseClientApplication *pApplication,
			string name, Variant &settings);
	virtual ~OutFileHDSStream();

	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual void SignalAttachedToInStream();
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts,
			bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
};

#endif	/* _OUTFILEHDSSTREAM_H */
#endif	/* HAS_PROTOCOL_HDS */
