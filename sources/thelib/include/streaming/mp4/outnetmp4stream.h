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

#ifndef _OUTNETMP4STREAM_H
#define	_OUTNETMP4STREAM_H

#define FTYP_SIZE 32

#include "streaming/baseoutnetstream.h"
#include "streaming/mp4/mp4streamwriter.h"

class DLLEXP OutNetMP4Stream
: public BaseOutNetStream, public Mp4StreamWriter {
private:

    struct Track : public Mp4StreamWriter::Track {
//        IOBufferExt _outputBuffer;

        Track() {
        }

        virtual ~Track() {
        }

        uint64_t Duration(uint32_t timescale) {
            if (_stts._timescale == timescale)
                return _stts._lastDts;
            if (_stts._timescale == 0)
                return 0;
            return (uint64_t) floor((double) _stts._lastDts / _stts._timescale * (double) timescale + 0.5);
        }

        void Finalize() {
            _tmpSttsCurrentDeltaCount = _stts._currentDeltaCount;
            _stts.Finish();
        }

        void Unfinalize() {
            _stts._currentDeltaCount = _tmpSttsCurrentDeltaCount;
        }

        bool Initialize(bool isAudio) {
            _isAudio = isAudio;
            if (isAudio) {
                // Size of samples (flag (1), stts (8), stsz (4), co64 (8))
                _dataSize = 21;
            } else {
                // Size of samples (flag, stts, stsz, co64, ctts)
                _dataSize = 29;
            }

//            _outputBuffer.IgnoreAll();

			// Initialize the track file
            _stts.SetTrack(this);
            _stsz.SetTrack(this);
            _co64.SetTrack(this);
			_co64.SetOffset(FTYP_SIZE);
            _stss.SetTrack(this); // Init not needed
            _ctts.SetTrack(this);

            return true;
        }
    };

    static uint8_t _ftyp[FTYP_SIZE];
    bool _hasInitialkeyFrame;
    bool _waitForIDR;
    double _ptsDtsDelta;
    uint32_t _chunkCount;

    uint64_t _frameCount;
    Track *_pAudio;
    Track *_pVideo;

	map<uint32_t, void *> _signaledProtocols;
	uint32_t _emptyRunsCount;
	vector<uint32_t> _pendigRemovals;
	bool _delayedRemoval;
	
	IOBufferExt _ftypBuffer;
	IOBufferExt _mdatBuffer;
	IOBufferExt _moovBuffer;
	IOBufferExt _progressiveBuffer;
	uint64_t _mdatHeader;
public:
    OutNetMP4Stream(BaseProtocol *pProtocol, string name);
    static OutNetMP4Stream* GetInstance(BaseClientApplication *pApplication, string name);
    virtual ~OutNetMP4Stream();
	
	void RegisterOutputProtocol(uint32_t protocolId, void *pUserData);
	void UnRegisterOutputProtocol(uint32_t protocolId);

protected:
	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool SignalPlay(double &dts, double &length);
	virtual bool SignalPause();
	virtual bool SignalResume();
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual void SignalAttachedToInStream();
	virtual void SignalDetachedFromInStream();
	virtual void SignalStreamCompleted();
	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	virtual void SignalAudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void SignalVideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);
    virtual bool FinishInitialization(
            GenericProcessDataSetup *pGenericProcessDataSetup);
    virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame);
    virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
	virtual bool WriteProgressiveBuffer(MP4ProgressiveBufferType type,
			const uint8_t *pBuffer, size_t size);
	bool SignalProtocols(const IOBuffer &buffer);
private:
    bool PushData(Track *track, const uint8_t *pData, uint32_t dataLength,
            double pts, double dts, bool isKeyFrame);
    bool StoreDataToFile(const uint8_t *pData, uint32_t dataLength);
    uint64_t Duration(uint32_t timescale);
    bool Finalize();
    bool OpenMP4();
    bool CloseMP4();
    bool FinalizeMdat();
    bool WriteFTYP(IOBufferExt &dst);
    bool WriteMOOV(IOBufferExt &dst);

	//TODO: test method
	bool AssembleFromBuffer();
};

#endif	/* _OUTNETMP4STREAM_H */
