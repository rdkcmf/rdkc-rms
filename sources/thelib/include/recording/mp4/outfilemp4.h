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


#ifndef _OUTFILEMP4_H
#define	_OUTFILEMP4_H

#include "recording/mp4/outfilemp4base.h"

#define FLUSH_INTERVAL 5

class OutFileMP4;

class DLLEXP FlushTimerProtocol
: public BaseTimerProtocol {
private:
    OutFileMP4 *_recorder;
    bool _enabled;

public:
    FlushTimerProtocol(OutFileMP4 *recorder);
    bool TimePeriodElapsed();
    void Enable(bool state = true);
};

class DLLEXP OutFileMP4
: public OutFileMP4Base {
private:

    struct Track : public OutFileMP4Base::Track {
        File *_pFile;

        vector<uint64_t> _sampleOffsets;

        Track()
        : _pFile(NULL) {
        }

        virtual ~Track() {
            if (_pFile != NULL) {
                if (_normalOperation) {
                    // Write down the remaining samples
                    _pFile->WriteBuffer(_pBuffer, _index);
                    // Add some padding
                    //_pFile->WriteBuffer(_pBuffer, _dataSize);
                }
                _pFile->Close();
                delete _pFile;
                _pFile = NULL;
            }

            if (_pBuffer != NULL) {
                delete[] _pBuffer;
                _pBuffer = NULL;
            }
            //cleanup the _sampleoffsets here
            _sampleOffsets.clear();
        }

        uint64_t Duration(uint32_t timescale) {
            if (_stts._timescale == timescale)
                return _stts._lastDts;
            if (_stts._timescale == 0)
                return 0;
            return (uint64_t) floor((double) _stts._lastDts / _stts._timescale * (double) timescale + 0.5);
        }

        void Finalize() {
            _prevIndex = _index;
            _tmpSttsCurrentDeltaCount = _stts._currentDeltaCount;
            if (_pFile != NULL) {
                _lastCursorPos = _pFile->Cursor();
                _pFile->WriteBuffer(_pBuffer, _index);
            }
            _stts.Finish();
        }

        void Unfinalize() {
            _index = _prevIndex;
            _stts._currentDeltaCount = _tmpSttsCurrentDeltaCount;
            if (_pFile != NULL) _pFile->SeekTo(_lastCursorPos);
        }

        bool PopulateHeader(File *pInfoFile) {
            uint8_t tmpVal = 0;
            uint8_t *pBuffer = NULL;

            // Track ID
            //uint64_t cursor = pInfoFile->Cursor();
            if (!pInfoFile->ReadUI8(&tmpVal)) return false;
            // DEBUG("id: %"PRIu32" @ 0x%016"PRIx64, tmpVal, cursor);

            _id = tmpVal;
            if (!_isAudio) {
                // _width
                if (!pInfoFile->ReadUI32(&_width, true)) return false;
                //DEBUG("_width: %"PRIu32, _width);
                // _height
                if (!pInfoFile->ReadUI32(&_height, true)) return false;
                //DEBUG("_height: %"PRIu32, _height);
            }
            // _timescale
            if (!pInfoFile->ReadUI32(&_timescale, true)) return false;
            //DEBUG("_timescale: %"PRIu32, _timescale);
            // _esDescriptor size
            uint32_t eSize = 0;
            if (!pInfoFile->ReadUI32(&eSize, true)) return false;
            //DEBUG("eSize: %"PRIu32, eSize);
            // _esDescriptor
            _esDescriptor.ReadFromRepeat(0x0, eSize);
            if (!pInfoFile->ReadBuffer(GETIBPOINTER(_esDescriptor), eSize)) return false;

            // STTS
            // _entryCount
            if (!pInfoFile->ReadUI32(&_stts._entryCount, true)) return false;
            //DEBUG("_stts._entryCount: %"PRIu32, _stts._entryCount);
            // _currentDeltaCount
            if (!pInfoFile->ReadUI32(&_stts._currentDeltaCount, true)) return false;
            //DEBUG("_stts._currentDeltaCount: %"PRIu32, _stts._currentDeltaCount);
            // _currentDelta
            if (!pInfoFile->ReadUI32(&_stts._currentDelta, true)) return false;
            //DEBUG("_stts._currentDelta: %"PRIu32, _stts._currentDelta);
            // _lastDts
            if (!pInfoFile->ReadUI64(&_stts._lastDts, true)) return false;
            //DEBUG("_stts._lastDts: %"PRIu64, _stts._lastDts);
            // _timescale
            _stts._timescale = _timescale;

            // STSZ
            // _sameSize, _sampleSize/_sampleCount
            if (!pInfoFile->ReadUI8(&tmpVal)) return false;
            pBuffer = GETIBPOINTER(_stsz._content);
            if (tmpVal > 0) {
                _stsz._sameSize = true;
                if (!pInfoFile->ReadUI32(&_stsz._sampleSize, true)) return false;
                EHTONLP(pBuffer, _stsz._sampleSize);
                //DEBUG("_stsz._sampleSize: %"PRIu32, _stsz._sampleSize);
            } else {
                _stsz._sameSize = false;
                if (!pInfoFile->ReadUI32(&_stsz._sampleCount, true)) return false;
                EHTONLP(pBuffer + 4, _stsz._sampleCount);
                //DEBUG("_stsz._sampleCount: %"PRIu32, _stsz._sampleCount);
            }

            if (!_isAudio) {
                // CTTS
                // _entryCount
                if (!pInfoFile->ReadUI32(&_ctts._entryCount, true)) return false;
                //DEBUG("_ctts._entryCount: %"PRIu32, _ctts._entryCount);
                // _currentCtsCount
                if (!pInfoFile->ReadUI32(&_ctts._currentCtsCount, true)) return false;
                //DEBUG("_ctts._currentCtsCount: %"PRIu32, _ctts._currentCtsCount);
                // _currentCts
                if (!pInfoFile->ReadUI32(&_ctts._currentCts, true)) return false;
                //DEBUG("_ctts._currentCts: %"PRIu32, _ctts._currentCts);
                // timescale
                _ctts._timescale = _timescale;
                // Add the entryCount
                if (_ctts._entryCount > 0) {
                    pBuffer = GETIBPOINTER(_ctts._content);
                    EHTONLP(pBuffer, _ctts._entryCount);
                } else {
                    _ctts._content.IgnoreAll();
                }
            }

            // CO64
            // insert flag here
            uint32_t is64 = 0;
            if (!pInfoFile->ReadUI32(&is64)) return false;
            _co64._is64 = is64 > 0;
            // _sampleCount
            if (!pInfoFile->ReadUI32(&_co64._sampleCount, true)) return false;
            //DEBUG("_co64._sampleCount: %"PRIu32, _co64._sampleCount);
            pBuffer = GETIBPOINTER(_co64._content);
            EHTONLP(pBuffer, _co64._sampleCount);

            return true;
        }

        bool Initialize(string filePath, bool isAudio, bool normalOperation) {
            _isAudio = isAudio;
            if (isAudio) {
                // Size of samples (flag (1), stts (8), stsz (4), co64 (8))
                _dataSize = 21;
            } else {
                // Size of samples (flag, stts, stsz, co64, ctts)
                _dataSize = 29;
            }

            // Read or create the track file?
            FILE_OPEN_MODE mode = FILE_OPEN_MODE_READ;
            _normalOperation = normalOperation;
            if (_normalOperation) {
                mode = FILE_OPEN_MODE_TRUNCATE;
            }

            // Initialize the track file
            _pFile = new File();

            filePath = format("%s.track%"PRIu32, STR(filePath), _id);
            if (!_pFile->Initialize(filePath, mode)) {
                FATAL("Unable to open file %s", STR(filePath));
                return false;
            }

            _stts.SetTrack(this);
            _stsz.SetTrack(this);
            _co64.SetTrack(this);
            _stss.SetTrack(this); // Init not needed
            _ctts.SetTrack(this);

            return true;
        }

        bool WriteToFile() {
            // Check if needed to write
            // stts: 8
            // stsz: 4
            // co64: 8
            // ctts: 8 -> video only
            // _dataSize = (_isAudio) ? 21 : 29;

            if ((_index + _dataSize) >= _bufferSize) {
                if (!_pFile->WriteBuffer(_pBuffer, _index)) return false;

                _index = 0;
            }

            return true;
        }

        bool PopulateContents() {
            // Check if contents are present
            uint64_t fileSize = _pFile->Size();
            //DEBUG("fileSize %"PRIu64, fileSize);
            if (fileSize == 0) {
                WARN("File size is 0");
                return true;
            }

            uint8_t *pBuffer = NULL;
            uint64_t *pSttsBuffer = NULL;
            uint32_t *pStszBuffer = NULL;
            uint64_t *pCttsBuffer = NULL;
            uint64_t *pCo64Buffer = NULL;
            uint32_t *pStcoBuffer = NULL;

            // Contents are arranged as:
            // flag - 1 byte indicates if STTS/CTTS are present
            // STTS - 8 bytes if present
            // STSZ - 4 bytes
            // CTTS - 8 bytes if present
            // CO64 - 8 bytes

            // Flag to indicate if we have STTS and/or CTTS
            uint8_t csFlag = 0;

            // Initialize the buffer and index
            uint32_t sttsIndex, stszIndex, cttsIndex, co64Index;
            sttsIndex = stszIndex = cttsIndex = co64Index = 0;

            pSttsBuffer = new uint64_t[_stts._entryCount];

            bool hasStsz = false;
            uint32_t stszCount = 0;
            if (!_stsz._sameSize) {
                hasStsz = true;
                stszCount = _stsz._sampleCount;
                pStszBuffer = new uint32_t[stszCount];
            }

            bool hasCtts = false;
            uint32_t cttsCount = 0;
            if (!_isAudio && (_ctts._entryCount > 0)) {
                hasCtts = true;
                cttsCount = _ctts._entryCount;
            }
            pCttsBuffer = new uint64_t[cttsCount];

            if (_co64._is64) {
                pCo64Buffer = new uint64_t[_co64._sampleCount];
            } else {
                pStcoBuffer = new uint32_t[_co64._sampleCount];
            }

            // Read the file contents
            do {
                if (!_pFile->ReadUI8(&csFlag)) {
                    //DEBUG("Cursor: %"PRIu64, _pFile->Cursor());
                    return false;
                }

                // Check if we have STTS entry and get it
                if (csFlag & 0x0F) {
                    // _currentDeltaCount and _currentDelta
                    if (!_pFile->ReadUI64(&pSttsBuffer[sttsIndex++], true)) {
                        //DEBUG("Cursor: %"PRIu64, _pFile->Cursor());
                        return false;
                    }
                }

                // Get STSZ entry
                if (hasStsz) {
                    // _sampleSize
                    if (!_pFile->ReadUI32(&pStszBuffer[stszIndex++], true)) {
                        //DEBUG("Cursor: %"PRIu64, _pFile->Cursor());
                        return false;
                    }
                } else {
                    // Sample sizes are written on track files even though they are of the same size
                    // so just ignore these
                    if (!_pFile->SeekAhead(4)) {
                        return false;
                    }
                }

                // Check if we have CTTS entry and get it
                if ((hasCtts) && (csFlag & 0xF0)) {
                    // _currentCtsCount and _currentCts
                    if (!_pFile->ReadUI64(&pCttsBuffer[cttsIndex++], true)) {
                        //DEBUG("Cursor: %"PRIu64, _pFile->Cursor());
                        return false;
                    }
                }

                // Get CO64 entry
                if (pCo64Buffer != NULL) {
                    if (!_pFile->ReadUI64(&pCo64Buffer[co64Index++], true)) {
                        //DEBUG("Cursor: %"PRIu64, _pFile->Cursor());
                        return false;
                    }
                    _sampleOffsets.push_back(*(pCo64Buffer + (co64Index - 1)));

                } else {
                    if (!_pFile->ReadUI32(&pStcoBuffer[co64Index++], true)) {
                        //DEBUG("Cursor: %"PRIu64, _pFile->Cursor());
                        return false;
                    }
                }
            } while (_pFile->Cursor() < fileSize);

            //DEBUG("sttsIndex: %"PRIu32, sttsIndex);
            //DEBUG("stszIndex: %"PRIu32, stszIndex);
            //DEBUG("cttsIndex: %"PRIu32, cttsIndex);
            //DEBUG("co64Index: %"PRIu32, co64Index);

            // Populate individual _content buffer
            // STTS
            _stts._content.ReadFromBuffer((uint8_t *) pSttsBuffer,
                    _stts._entryCount * sizeof (uint64_t));
            // Add the last entry
            _stts._content.ReadFromRepeat(0, 8);
            pBuffer = GETIBPOINTER(_stts._content)
                    + GETAVAILABLEBYTESCOUNT(_stts._content) - 8;
            EHTONLP(pBuffer, _stts._currentDeltaCount);
            EHTONLP(pBuffer + 4, _stts._currentDelta);
            _stts._entryCount++;
            //DEBUG("STTS ok.");

            // STSZ
            if (hasStsz) {
                _stsz._content.ReadFromBuffer((uint8_t *) pStszBuffer,
                        _stsz._sampleCount * sizeof (uint32_t));
            }
            //DEBUG("STSZ ok.");

            // CTTS
            if (hasCtts) {
                _ctts._content.ReadFromBuffer((uint8_t *) pCttsBuffer,
                        _ctts._entryCount * sizeof (uint64_t));
                // Add the last entry
                _ctts._content.ReadFromRepeat(0, 8);
                pBuffer = GETIBPOINTER(_ctts._content)
                        + GETAVAILABLEBYTESCOUNT(_ctts._content) - 8;
                EHTONLP(pBuffer, _ctts._currentCtsCount);
                EHTONLP(pBuffer + 4, _ctts._currentCts);
                pBuffer = GETIBPOINTER(_ctts._content);
                EHTONLP(pBuffer, ++_ctts._entryCount);

                //DEBUG("CTTS ok.");
                //DEBUG("CTTS _entryCount: %"PRIu32, _ctts._entryCount);
            }

            // CO64
            // _sampleCount
            uint8_t *buffer = _co64._is64 ? (uint8_t*) pCo64Buffer : (uint8_t*) pStcoBuffer;
            _co64._content.ReadFromBuffer(buffer,
                    _co64._sampleCount * (_co64._is64 ? sizeof (uint64_t) : sizeof (uint32_t)));
            //DEBUG("CO64 ok.");

            if (pSttsBuffer != NULL) delete[] pSttsBuffer;
            if (pStszBuffer != NULL) delete[] pStszBuffer;
            if (pCttsBuffer != NULL) delete[] pCttsBuffer;
            if (pCo64Buffer != NULL) delete[] pCo64Buffer;

            return true;
        }

        bool InitFromFile(File *pInfoFile, string filePath, bool isAudio) {
            _isAudio = isAudio;

            // Read the headers
            if (!PopulateHeader(pInfoFile)) return false;
            //DEBUG("Header ok.");
            // Initialize the track
            if (!Initialize(filePath, isAudio, false)) return false;
            //DEBUG("Initialization ok.");
            // Read the contents
            if (!PopulateContents()) return false;
            //DEBUG("Contents ok.");

            return true;
        }
    };

    static uint8_t _ftyp[32];
    File *_pInfoFile;
#ifdef DEBUG_MP4
    File *_pHeaderFile; //TODO: for testing purposes
#endif
    bool _hasInitialkeyFrame;
    bool _waitForIDR;
    double _ptsDtsDelta;
    bool _normalOperation;
    uint32_t _chunkCount;
    bool _flushNow;
    bool _recoveryMode;
    FlushTimerProtocol *_pFlushTimer;
    uint64_t _frameCount;
    Track *_pAudio;
    Track *_pVideo;
    string _mp4BinPath;
    BaseClientApplication *_pApp;

public:
    OutFileMP4(BaseProtocol *pProtocol, string name, Variant &settings);
    static OutFileMP4* GetInstance(BaseClientApplication *pApplication,
            string name, Variant &settings);
    virtual ~OutFileMP4();
    /**
     * Reads the .info and .trackXX files to instantiate the necessary boxes/atoms.
     * 
     * @param filePath Points to the MP4 file to be generated
     * @return true on success, false otherwise
     */
    bool InitializeFromFile(string filePath, bool recoveryMode = false);

    /**
     * Write the necessary information of the MP4 file as well as the STSS samples.
     * 
     * @return true on success, false otherwise
     */
    bool WriteInfoFile();

    /**
     * Assembles/creates the actual MP4 based on the current state of this class.
     * 
     * @return true on success, false otherwise
     */
    bool Assemble();

    void SignalFlush();
protected:
    virtual bool FinishInitialization(
            GenericProcessDataSetup *pGenericProcessDataSetup);
    virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame);
    virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
private:
    bool PushData(Track *track, const uint8_t *pData, uint32_t dataLength,
            double pts, double dts, bool isKeyFrame);
    bool StoreDataToFile(const uint8_t *pData, uint32_t dataLength);
    uint64_t Duration(uint32_t timescale);
    bool Finalize();
    bool OpenMP4();
    bool CloseMP4();
    bool FlushToFile();
    void InitializeTimer();
    bool FinalizeMdat();
    bool WriteFTYP(File *pFile);
    bool WriteMOOV(File *pFile);
};

#endif	/* _OUTFILEMP4_H */
