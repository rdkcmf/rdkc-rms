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


#ifndef _OUTFILEMP4BASE_H
#define	_OUTFILEMP4BASE_H

#include "recording/baseoutrecording.h"
#include "protocols/timer/basetimerprotocol.h"
#include <math.h>

class DLLEXP OutFileMP4Base
: public BaseOutRecording {
public:
    OutFileMP4Base(BaseProtocol *pProtocol, string name, Variant &settings);
    virtual ~OutFileMP4Base();

protected:
    struct Track;

    struct STTS {
        double _timescale;
        uint64_t _sampleCount;
        uint64_t _recordedCount;
        uint32_t _entryCount;
        IOBuffer _content;
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
            _pTrack->_pBuffer[_pTrack->_index] = 0x00;
            _pTrack->_hasSttsEntry = false;

            if (_lastDts == 0xffffffffffffffffLL) {
                _lastDts = value;
                // Increment buffer index whether we have stts or not
                _pTrack->_index++;
                return;
            }
            uint32_t delta = (uint32_t) (value - _lastDts);
            if (delta != _currentDelta) {
                if (_currentDeltaCount != 0) {
                    //					FINEST("delta: %"PRIu32"; _currentDeltaCount: %"PRIu32"; _currentDelta: %"PRIu32,
                    //							delta,
                    //							_currentDeltaCount,
                    //							_currentDelta);
#ifdef DEBUG_MP4
                    _content.ReadFromRepeat(0, 8);
                    uint8_t *pBuffer = GETIBPOINTER(_content) + GETAVAILABLEBYTESCOUNT(_content) - 8;
                    EHTONLP(pBuffer, _currentDeltaCount);
                    EHTONLP(pBuffer + 4, ((uint32_t) _currentDelta));
#endif	
                    _entryCount++;

                    // indicate that stts is present
                    _pTrack->_pBuffer[_pTrack->_index] |= 0x0F;
                    EHTONLP((_pTrack->_pBuffer + _pTrack->_index + 1), EHTONL(_currentDelta));
                    EHTONLP((_pTrack->_pBuffer + _pTrack->_index + 5), EHTONL(_currentDeltaCount));
                    _pTrack->_index += 8;
                    _pTrack->_hasSttsEntry = true;
                    _recordedCount += _currentDeltaCount;
                }
                _currentDelta = delta;
                _currentDeltaCount = 1;
            } else {
                _currentDeltaCount++;
            }
            _lastDts = value;
            // Increment buffer index
            _pTrack->_index++;
        }

        void Finish() {
            /*			if (!_pTrack->_normalOperation) return;

                                    _entryCount++;
                                    _content.ReadFromRepeat(0, 8);
                                    uint8_t *pBuffer = GETIBPOINTER(_content) + GETAVAILABLEBYTESCOUNT(_content) - 8;
                                    EHTONLP(pBuffer, _currentDeltaCount);
                                    EHTONLP(pBuffer + 4, ((uint32_t) _currentDelta));
                                    //			FINEST("_currentDeltaCount: %"PRIu32"; _currentDelta: %"PRIu32,
                                    //					_currentDeltaCount,
                                    //					_currentDelta);
             */
            _currentDeltaCount = (uint32_t) (_sampleCount - _recordedCount);
        }

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
            return GETAVAILABLEBYTESCOUNT(_content);
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
        IOBuffer _content;
        uint32_t _tempSize;
        uint32_t *_pTemp;
        uint32_t _index;

        STSZ() {
            _sampleSize = 0;
            _sampleCount = 0;
            _sameSize = true;
            _tempSize = 1024;
            _pTemp = NULL;
#ifdef DEBUG_MP4
            _pTemp = new uint32_t[_tempSize];
#endif
            _index = 0;
            _content.ReadFromRepeat(0, 8);
        }

        virtual ~STSZ() {
            if (_pTemp != NULL) {
                delete[] _pTemp;
                _pTemp = NULL;
            }
        }

        void PushSize(uint32_t size) {
            _sameSize &= (_sampleSize == 0) || (_sampleSize == size);

#ifdef DEBUG_MP4
            _index = _sampleCount % _tempSize;
            //			FINEST("size %"PRIu32"; _sameSize: %d; _index: %"PRIu32"; _sampleCount: %"PRIu32,
            //					size,
            //					_sameSize,
            //					_index,
            //					_sampleCount);
            if ((_index == 0)&&(_sampleCount != 0))
                _content.ReadFromBuffer((uint8_t *) _pTemp, _tempSize * sizeof (uint32_t));
            _pTemp[_index] = EHTONL(size);
#endif
            _sampleSize = size;
            _sampleCount++;

            EHTONLP((_pTrack->_pBuffer + _pTrack->_index), EHTONL(size));
            _pTrack->_index += 4;
        }
#ifdef DEBUG_MP4

        void Finish() {
            if (!_pTrack->_normalOperation) return;

            uint8_t *pBuffer = GETIBPOINTER(_content);
            if (_sameSize) {
                EHTONLP(pBuffer, _sampleSize);
                return;
            }
            EHTONLP(pBuffer + 4, _sampleCount);
            _index = _sampleCount % _tempSize;
            _content.ReadFromBuffer((uint8_t *) _pTemp, _index * sizeof (uint32_t));
        }
#endif

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
            if (_sameSize)
                return 8;
            return GETAVAILABLEBYTESCOUNT(_content);
        }

        void SetTrack(Track *track) {
            _pTrack = track;
        }

    private:
        Track *_pTrack;
    };

    struct CO64 {
        IOBuffer _content;
        uint64_t _index;
        uint32_t _sampleCount;
        uint64_t _tempSize;
        uint64_t *_pTemp;
        bool _is64;

        CO64() {
            _index = 0;
            _sampleCount = 0;
            _tempSize = 1024;
            _pTemp = NULL;
            _is64 = false;
#ifdef DEBUG_MP4
            _pTemp = new uint64_t[_tempSize];
#endif
            _content.ReadFromRepeat(0, 4);

        }

        virtual ~CO64() {
            if (_pTemp != NULL) {
                delete[] _pTemp;
                _pTemp = NULL;
            }
        }

        void PushOffset(uint64_t offset) {
#ifdef DEBUG_MP4
            _index = _sampleCount % _tempSize;
            if ((_index == 0)&&(_sampleCount != 0)) {
                _content.ReadFromBuffer((uint8_t *) _pTemp, _tempSize * sizeof (uint64_t));
            }
            _pTemp[_index] = EHTONLL(offset);
#endif
            if ((offset > 0x00000000ffffffffLL) && !_is64) {
                _is64 = true;
            }
            _sampleCount++;
            if (_is64) {
                EHTONLLP((_pTrack->_pBuffer + _pTrack->_index), EHTONLL(offset));
                _pTrack->_index += 8;
            } else {
                EHTONLP((_pTrack->_pBuffer + _pTrack->_index), EHTONL((uint32_t) offset));
                _pTrack->_index += 4;
            }
        }
#ifdef DEBUG_MP4

        void Finish() {
            if (!_pTrack->_normalOperation) return;

            uint8_t *pBuffer = GETIBPOINTER(_content);
            EHTONLP(pBuffer, _sampleCount);
            _index = _sampleCount % _tempSize;
            _content.ReadFromBuffer((uint8_t *) _pTemp, _index * sizeof (uint64_t));
        }
#endif

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
            return GETAVAILABLEBYTESCOUNT(_content);
        }

        void SetTrack(Track *track) {
            _pTrack = track;
        }

    private:
        Track *_pTrack;
    };

    struct STSS {
        IOBuffer _content;
        uint32_t _index;
        uint32_t _entryCount;
        uint32_t _tempSize;
        uint32_t *_pTemp;
        File *_pFile;
        uint64_t _lastWrite;
        uint64_t _lastDump;

        STSS() {
            _index = 0;
            _entryCount = 0;
            _tempSize = 1024;
            _pTemp = new uint32_t[_tempSize];
            _content.ReadFromRepeat(0, 4);
            _lastWrite = 0;
            _lastDump = 0;
        }

        virtual ~STSS() {
            if (_pTemp != NULL) {
                delete[] _pTemp;
                _pTemp = NULL;
            }
        }

        void PushKeyFrameIndex(uint32_t sampleNumber) {
            _index = _entryCount % _tempSize;
            if ((_index == 0)&&(_entryCount != 0)) {
#ifdef DEBUG_MP4
                _content.ReadFromBuffer((uint8_t *) _pTemp, _tempSize * sizeof (uint32_t));
#endif
                // Write to first part of INFO file
                if (_lastDump != 0 && _lastDump != _pFile->Cursor())
                    _pFile->SeekTo(_lastDump);
//                WARN("STSS Writing to info file at 0x%016"PRIu64, _pFile->Cursor());
                if (!_pFile->WriteBuffer((uint8_t *) _pTemp, (uint64_t) (_tempSize * sizeof (uint32_t)))) {
                    FATAL("Unable to write key frame!");
                    return;
                }
                _lastDump = _pFile->Cursor();
            }
            _pTemp[_index] = EHTONL(sampleNumber);
            _entryCount++;
        }
#ifdef DEBUG_MP4

        void Finish() {
            if (!_pTrack->_normalOperation) return;

            uint8_t *pBuffer = GETIBPOINTER(_content);
            EHTONLP(pBuffer, _entryCount);
            _index = _entryCount % _tempSize;
            _content.ReadFromBuffer((uint8_t *) _pTemp, _index * sizeof (uint32_t));
        }
#endif

        bool Finalize() {
            // Write the remaining entries
            if (_lastDump != 0 && _lastDump != _pFile->Cursor())
                _pFile->SeekTo(_lastDump);
            _index = _entryCount % _tempSize;
            // WARN("Writing Final STSS entries on 0x%016"PRIu64, _pFile->Cursor());
            if (!_pFile->WriteBuffer((uint8_t *) _pTemp, (uint64_t) (_index * sizeof (uint32_t)))) {
                FATAL("Unable to write key frame!");
                return false;
            }
            _lastWrite = _pFile->Cursor();

            // Seek at beginning
            if (!_pFile->SeekBegin()) {
                FATAL("Unable to seek!");
                return false;
            }

            // Write the entry count
            if (!_pFile->WriteUI32(_entryCount, true)) {
                FATAL("Unable to write key frame count!");
                return false;
            }

            // Seek to end
            if (!_pFile->SeekTo(_lastWrite)) {
                FATAL("Unable to seek!");
                return false;
            }

            return true;
        }

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
            return GETAVAILABLEBYTESCOUNT(_content);
        }

        void SetTrack(Track *pTrack) {
            _pTrack = pTrack;
        }

        bool SetInfoFile(File *pFile) {
            _pFile = pFile;

            // STSS initialization write 4 bytes
            uint32_t tmp = 0;
            if (!_pFile->WriteUI32(tmp, true)) {
                FATAL("Unable to initialize STSS!");
                return false;
            }

            return true;
        }

    private:
        Track *_pTrack;
    };

    struct CTTS {
        double _timescale;
        uint32_t _entryCount;
        IOBuffer _content;
        uint32_t _currentCts;
        uint32_t _currentCtsCount;

        CTTS() {
            _timescale = 0;
            _entryCount = 0;
            _currentCts = 0;
            _currentCtsCount = 0;
            _content.ReadFromRepeat(0, 4);
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
#ifdef DEBUG_MP4
                    _content.ReadFromRepeat(0, 8);
                    uint8_t *pBuffer = GETIBPOINTER(_content) + GETAVAILABLEBYTESCOUNT(_content) - 8;
                    EHTONLP(pBuffer, _currentCtsCount);
                    EHTONLP(pBuffer + 4, ((uint32_t) _currentCts));
#endif
                    if (_pTrack->_hasSttsEntry) {
                        // indicate that ctts is present (stts flag 13 bytes prior - has stts)
                        _pTrack->_pBuffer[_pTrack->_index - 13] |= 0xF0;
                    } else {
                        // indicate that ctts is present (stts flag 5 bytes prior - NO stts)
                        _pTrack->_pBuffer[_pTrack->_index - 5] |= 0xF0;
                    }
                    EHTONLP((_pTrack->_pBuffer + _pTrack->_index), EHTONL(_currentCts));
                    EHTONLP((_pTrack->_pBuffer + _pTrack->_index + 4), EHTONL(_currentCtsCount));
                    _pTrack->_index += 8;
                }
                _currentCtsCount = 1;
                _currentCts = value;
            } else {
                _currentCtsCount++;
            }
        }
#ifdef DEBUG_MP4

        void Finish() {
            if (!_pTrack->_normalOperation) return;

            _entryCount++;
            if (_entryCount == 1) {
                _content.IgnoreAll();
                return;
            }
            _content.ReadFromRepeat(0, 8);
            uint8_t *pBuffer = GETIBPOINTER(_content);
            EHTONLP(pBuffer, _entryCount);
            EHTONLP(pBuffer + GETAVAILABLEBYTESCOUNT(_content) - 8, _currentCtsCount);
            EHTONLP(pBuffer + GETAVAILABLEBYTESCOUNT(_content) - 4, ((uint32_t) _currentCts));
            //			FINEST("_currentDeltaCount: %"PRIu32"; _currentDelta: %"PRIu32,
            //					_currentDeltaCount,
            //					_currentDelta);
        }
#endif

        uint8_t * Content() {
            return GETIBPOINTER(_content);
        }

        uint32_t ContentSize() {
            return GETAVAILABLEBYTESCOUNT(_content);
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
        uint16_t _index;
        uint16_t _prevIndex;
        uint16_t _bufferSize;
        uint8_t *_pBuffer;
        uint8_t _dataSize;
        uint64_t _lastCursorPos;
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
        _index(0),
        _prevIndex(0),
        _bufferSize(4096),
        _pBuffer(new uint8_t[_bufferSize]),
        _lastCursorPos(0),
        _normalOperation(true),
        _hasSttsEntry(false),
        _tmpSttsCurrentDeltaCount(0) {
        }

        virtual ~Track() {
            if (_pBuffer != NULL) {
                delete[] _pBuffer;
                _pBuffer = NULL;
            }
        }

        virtual uint64_t Duration(uint32_t timescale) {
            if (_stts._timescale == timescale)
                return _stts._lastDts;
            if (_stts._timescale == 0)
                return 0;
            return (uint64_t) floor((double) _stts._lastDts / _stts._timescale * (double) timescale + 0.5);
        }

        virtual void Finalize() {
            _prevIndex = _index;
            _tmpSttsCurrentDeltaCount = _stts._currentDeltaCount;
            _stts.Finish();
        }

        virtual void Unfinalize() {
            _index = _prevIndex;
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

    File *_pMdatFile;
    uint64_t _mdatDataSize;
    uint64_t _mdatDataSizeOffset;
    uint64_t _creationTime;
    uint32_t _nextTrackId;
    uint64_t _infoWriteBegin;
    bool _winQtCompat;
    uint64_t _chunkLength;
    string _filePath;
    bool _onlyAudio;
    TrackInfo _permanentData;
    Variant _chunkInfo;
    string _startTime;

protected:
    uint32_t _mvhdTimescale;

protected:
    virtual uint64_t Duration(uint32_t timescale);
    virtual bool IsCodecSupported(uint64_t codec);
    virtual bool UpdateSize(File *pFile, uint64_t sizeOffset);
    virtual bool WriteFTYP(File *pFile) = 0;
    virtual bool WriteMVHD(File *pFile);
    virtual bool WriteTRAK(File *pFile, Track *track);
    virtual bool WriteTKHD(File *pFile, Track *track);
    virtual bool WriteMDIA(File *pFile, Track *track);
    virtual bool WriteMDHD(File *pFile, Track *track);
    virtual bool WriteHDLR(File *pFile, Track *track);
    virtual bool WriteMINF(File *pFile, Track *track);
    virtual bool WriteSMHD(File *pFile, Track *track);
    virtual bool WriteVMHD(File *pFile, Track *track);
    virtual bool WriteDINF(File *pFile, Track *track);
    virtual bool WriteSTBL(File *pFile, Track *track);
    virtual bool WriteDREF(File *pFile, Track *track);
    virtual bool WriteURL(File *pFile, Track *track);
    virtual bool WriteSTSD(File *pFile, Track *track);
    virtual bool WriteSTTS(File *pFile, Track *track);
    virtual bool WriteCTTS(File *pFile, Track *track);
    virtual bool WriteSTSS(File *pFile, Track *track);
    virtual bool WriteSTSC(File *pFile, Track *track);
    virtual bool WriteSTSZ(File *pFile, Track *track);
    virtual bool WriteCO64(File *pFile, Track *track);
    virtual bool WriteMP4A(File *pFile, Track *track);
    virtual bool WriteAVC1(File *pFile, Track *track);
    virtual bool WriteESDS(File *pFile, Track *track);
    virtual bool WriteAVCC(File *pFile, Track *track);
};

#endif	/* _OUTFILEMP4BASE_H */
