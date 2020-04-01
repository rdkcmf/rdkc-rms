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

//TODO: Refactor this to merge with recording/mp4/outfilemp4base to practice code reuse!

#ifndef _MP4STREAMWRITER_H
#define	_MP4STREAMWRITER_H

#include "common.h"
#include "utils/buffering/iobufferext.h"
#include <math.h>

typedef enum {
	M_PBT_BEGIN = 0xf9,
	M_PBT_HEADER_MOOV = 0xfa,
	M_PBT_VIDEO_BUFFER = 0xfc,
	M_PBT_AUDIO_BUFFER = 0xfd,
	M_PBT_HEADER_MDAT = 0xfe,
	M_PBT_END = 0xff
} MP4ProgressiveBufferType;

class DLLEXP Mp4StreamWriter {
public:
    Mp4StreamWriter();
    virtual ~Mp4StreamWriter();

protected:
    struct Track;

    struct STTS {
        double _timescale;
        uint64_t _sampleCount;
        uint64_t _recordedCount;
        uint32_t _entryCount;
        IOBufferExt _content;
        uint64_t _lastDts;
        uint32_t _currentDelta;
        uint32_t _currentDeltaCount;

        STTS() {
            _timescale = 0;
            _entryCount = 0;
            _lastDts = (uint64_t) 0xffffffffffffffffLL;
            _currentDelta = 0;
            _currentDeltaCount = 0;
            _sampleCount = 0;
            _recordedCount = 0;
        }

        void PushDTS(double dts) {
            uint64_t value;
            _sampleCount++;
            if (_timescale != 1000.0)
                value = (uint64_t) floor(dts / 1000.0 * _timescale + 0.5);
            else
                value = (uint64_t) floor(dts + 0.5);

            // indicate that stts is NOT present by default
            _pTrack->_hasSttsEntry = false;

            if (_lastDts == 0xffffffffffffffffLL) {
                _lastDts = value;

                return;
            }
            uint32_t delta = (uint32_t) (value - _lastDts);
            if (delta != _currentDelta) {
                if (_currentDeltaCount != 0) {
                    _entryCount++;

					_content.ReadFromU32(_currentDeltaCount, true);
					_content.ReadFromU32(_currentDelta, true);

                    _pTrack->_hasSttsEntry = true;
                    _recordedCount += _currentDeltaCount;
                }
                _currentDelta = delta;
                _currentDeltaCount = 1;
            } else {
                _currentDeltaCount++;
            }
            _lastDts = value;
        }

        void Finish() {
            _currentDeltaCount = (uint32_t) (_sampleCount - _recordedCount);
			
			// Put the last entry
			_content.ReadFromU32(_currentDeltaCount, true);
			_content.ReadFromU32(_currentDelta, true);
		
			_entryCount++;
        }

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
			if (_entryCount) {
				return GETAVAILABLEBYTESCOUNT(_content);
			}

			return 0;
        }

        void SetTrack(Track *track) {
            _pTrack = track;
        }

    private:
        Track *_pTrack;
    };

    struct STSZ {
        uint32_t _sampleSize;
        uint32_t _sampleCount;
        bool _sameSize;
        IOBufferExt _content;

        STSZ() {
            _sampleSize = 0;
            _sampleCount = 0;
            _sameSize = true;
        }

        virtual ~STSZ() {
        }

        void PushSize(uint32_t size) {
            _sameSize &= (_sampleSize == 0) || (_sampleSize == size);
            _sampleSize = size;
            _sampleCount++;

			_content.ReadFromU32(size, true);
        }

        uint8_t * Content() {
            // For some reason, it still gets here
            if (_sameSize && (GETAVAILABLEBYTESCOUNT(_content) < 8)) {
                _content.IgnoreAll();
                _content.ReadFromU64(0, true);
            }
            
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
            if (_sameSize)
                return 8;
			
			if (_sampleCount) {
				return GETAVAILABLEBYTESCOUNT(_content);
			}

			return 0;
        }

        void SetTrack(Track *track) {
            _pTrack = track;
        }

    private:
        Track *_pTrack;
    };

    struct CO64 {
        IOBufferExt _content;
        uint32_t _sampleCount;
        bool _is64;
		uint32_t _offset;

        CO64() {
            _sampleCount = 0;
            _is64 = false;
        }

        virtual ~CO64() {
        }

        void PushOffset(uint64_t offset) {
			// Consider the FTYP box/atom
			offset += _offset;
			
            if ((offset > 0x00000000ffffffffLL) && !_is64) {
                _is64 = true;
            }
			
            _sampleCount++;
            
			if (_is64) {
				_content.ReadFromU64(offset, true);
            } else {
				_content.ReadFromU32((uint32_t) offset, true);
            }
        }

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
			if (_sampleCount) {
				return GETAVAILABLEBYTESCOUNT(_content);
			}
			
			return 0;
        }

        void SetTrack(Track *track) {
            _pTrack = track;
        }
		
		void SetOffset(uint32_t offset) {
			_offset = offset;
		}

    private:
        Track *_pTrack;
    };

    struct STSS {
        IOBufferExt _content;
        uint32_t _entryCount;

        STSS() {
            _entryCount = 0;
        }

        virtual ~STSS() {
        }

        void PushKeyFrameIndex(uint32_t sampleNumber) {
			_content.ReadFromU32(sampleNumber, true);

            _entryCount++;
        }

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
			if (_entryCount) {
				return GETAVAILABLEBYTESCOUNT(_content);
			}
			
			return 0;
        }

        void SetTrack(Track *pTrack) {
            _pTrack = pTrack;
        }

    private:
        Track *_pTrack;
    };

    struct CTTS {
        double _timescale;
        uint32_t _entryCount;
        IOBufferExt _content;
        uint32_t _currentCts;
        uint32_t _currentCtsCount;

        CTTS() {
            _timescale = 0;
            _entryCount = 0;
            _currentCts = 0;
            _currentCtsCount = 0;
        }

        void PushCTS(double cts) {
            uint32_t value;
            if (_timescale != 1000.0)
                value = (uint32_t) floor(cts / 1000.0 * _timescale + 0.5);
            else
                value = (uint32_t) floor(cts + 0.5);

            if (_currentCts != value) {
                if (_currentCtsCount != 0) {
                    _entryCount++;

					_content.ReadFromU32(_currentCtsCount);
					_content.ReadFromU32(_currentCts);
                }
                _currentCtsCount = 1;
                _currentCts = value;
            } else {
                _currentCtsCount++;
            }
        }

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
			if (_entryCount) {
				return GETAVAILABLEBYTESCOUNT(_content);
			}

			return 0;
        }

        void SetTrack(Track *track) {
            _pTrack = track;
        }

    private:
        Track *_pTrack;
    };

    struct TrackInfo {
        uint32_t _videoId;
        uint32_t _audioId;
        uint32_t _videoWidth;
        uint32_t _videoHeight;
        uint32_t _audioTimescale;
        uint32_t _videoTimescale;
        IOBuffer _audioEsDescriptor;
        IOBuffer _videoEsDescriptor;

        TrackInfo() {
            _videoId = 0;
            _audioId = 0;
            _videoWidth = 0;
            _videoHeight = 0;
            _audioTimescale = 0;
            _videoTimescale = 0;
        }
    };

    struct Track {
        uint32_t _id;
        bool _isAudio;
        uint32_t _width;
        uint32_t _height;
        uint32_t _timescale;
        IOBuffer _esDescriptor;
        uint16_t _prevIndex;
        uint16_t _bufferSize;
        uint8_t _dataSize;
        bool _normalOperation;
        bool _hasSttsEntry;
        uint32_t _tmpSttsCurrentDeltaCount;

        STTS _stts;
        STSZ _stsz;
        CO64 _co64;
        STSS _stss;
        CTTS _ctts;

        Track()
        : _id(0),
        _isAudio(false),
        _width(0),
        _height(0),
        _timescale(0),
        _prevIndex(0),
        _normalOperation(true),
        _hasSttsEntry(false),
        _tmpSttsCurrentDeltaCount(0) {
        }

        virtual ~Track() {
        }

        virtual uint64_t Duration(uint32_t timescale) {
            if (_stts._timescale == timescale)
                return _stts._lastDts;
            if (_stts._timescale == 0)
                return 0;
            return (uint64_t) floor((double) _stts._lastDts / _stts._timescale * (double) timescale + 0.5);
        }

        virtual void Finalize() {
            _tmpSttsCurrentDeltaCount = _stts._currentDeltaCount;
            _stts.Finish();
        }

        virtual void Unfinalize() {
            _stts._currentDeltaCount = _tmpSttsCurrentDeltaCount;
        }

        virtual bool Initialize(bool isAudio) {
            _isAudio = isAudio;
            if (isAudio) {
                // Size of samples (flag (1), stts (8), stsz (4), co64 (8))
                _dataSize = 21;
            } else {
                // Size of samples (flag, stts, stsz, co64, ctts)
                _dataSize = 29;
            }

            _stts.SetTrack(this);
            _stsz.SetTrack(this);
            _co64.SetTrack(this);
            _stss.SetTrack(this); // Init not needed
            _ctts.SetTrack(this);

            return true;
        }
    };

    uint64_t _mdatDataSize;
    uint64_t _mdatDataSizeOffset;
    uint64_t _creationTime;
    uint32_t _nextTrackId;
    uint64_t _infoWriteBegin;
    bool _winQtCompat;
    uint64_t _chunkLength;
    bool _onlyAudio;
    TrackInfo _permanentData;
    Variant _chunkInfo;
    string _startTime;

protected:
    uint32_t _mvhdTimescale;

protected:
    virtual uint64_t Duration(uint32_t timescale);
    virtual bool IsCodecSupported(uint64_t codec);
    virtual bool UpdateSize(IOBufferExt &dst, uint32_t sizeOffset);
    virtual bool WriteMVHD(IOBufferExt &dst);
    virtual bool WriteTRAK(IOBufferExt &dst, Track *track);
    virtual bool WriteTKHD(IOBufferExt &dst, Track *track);
    virtual bool WriteMDIA(IOBufferExt &dst, Track *track);
    virtual bool WriteMDHD(IOBufferExt &dst, Track *track);
    virtual bool WriteHDLR(IOBufferExt &dst, Track *track);
    virtual bool WriteMINF(IOBufferExt &dst, Track *track);
    virtual bool WriteSMHD(IOBufferExt &dst, Track *track);
    virtual bool WriteVMHD(IOBufferExt &dst, Track *track);
    virtual bool WriteDINF(IOBufferExt &dst, Track *track);
    virtual bool WriteSTBL(IOBufferExt &dst, Track *track);
    virtual bool WriteDREF(IOBufferExt &dst, Track *track);
    virtual bool WriteURL(IOBufferExt &dst, Track *track);
    virtual bool WriteSTSD(IOBufferExt &dst, Track *track);
    virtual bool WriteSTTS(IOBufferExt &dst, Track *track);
    virtual bool WriteCTTS(IOBufferExt &dst, Track *track);
    virtual bool WriteSTSS(IOBufferExt &dst, Track *track);
    virtual bool WriteSTSC(IOBufferExt &dst, Track *track);
    virtual bool WriteSTSZ(IOBufferExt &dst, Track *track);
    virtual bool WriteCO64(IOBufferExt &dst, Track *track);
    virtual bool WriteMP4A(IOBufferExt &dst, Track *track);
    virtual bool WriteAVC1(IOBufferExt &dst, Track *track);
    virtual bool WriteESDS(IOBufferExt &dst, Track *track);
    virtual bool WriteAVCC(IOBufferExt &dst, Track *track);
};

#endif	/* _MP4STREAMWRITER_H */
