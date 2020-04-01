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


#include "streaming/streamstypes.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "application/baseclientapplication.h"
#include "application/clientapplicationmanager.h"
#include "streaming/codectypes.h"
#include "streaming/baseinstream.h"
#include "recording/mp4/outfilemp4.h"
#include "application/baseappprotocolhandler.h"
#include "eventlogger/eventlogger.h"

FlushTimerProtocol::FlushTimerProtocol(OutFileMP4 *recorder) {
    _recorder = recorder;
    _enabled = false;
}

bool FlushTimerProtocol::TimePeriodElapsed() {
    if (_enabled) _recorder->SignalFlush();
    return true;
}

void FlushTimerProtocol::Enable(bool state) {
    _enabled = state;
}

uint8_t OutFileMP4::_ftyp[32] = {
    0x00, 0x00, 0x00, 0x20, //size: 0x20
    0x66, 0x74, 0x79, 0x70, //type: ftyp
    0x69, 0x73, 0x6F, 0x6D, //major_brand: isom
    0x00, 0x00, 0x02, 0x00, //minor_version
    0x69, 0x73, 0x6F, 0x6D, //compatible_brands[0]: isom
    0x69, 0x73, 0x6F, 0x32, //compatible_brands[1]: iso2
    0x61, 0x76, 0x63, 0x31, //compatible_brands[2]: avc1
    0x6D, 0x70, 0x34, 0x31, //compatible_brands[3]: mp41
};

OutFileMP4::OutFileMP4(BaseProtocol *pProtocol, string name, Variant &settings)
: OutFileMP4Base(pProtocol, name, settings),
_pInfoFile(NULL),
_hasInitialkeyFrame(false),
_waitForIDR(false),
_ptsDtsDelta(0),
_normalOperation(true),
_chunkCount(0),
_flushNow(false),
_recoveryMode(false),
_pFlushTimer(NULL),
_frameCount(0),
_pAudio(NULL),
_pVideo(NULL) {
#ifdef DEBUG_MP4
    _pHeaderFile = NULL;
#endif
    _pApp = pProtocol->GetApplication();
}

OutFileMP4* OutFileMP4::GetInstance(BaseClientApplication *pApplication,
        string name, Variant &settings) {
    //1. Create a dummy protocol
    PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

    //2. create the parameters
    Variant parameters;
    parameters["customParameters"]["recordConfig"] = settings;

    //3. Initialize the protocol
    if (!pDummyProtocol->Initialize(parameters)) {
        FATAL("Unable to initialize passthrough protocol");
        pDummyProtocol->EnqueueForDelete();
        return NULL;
    }

    //4. Set the application
    pDummyProtocol->SetApplication(pApplication);

    //5. Create the MP4 stream
    OutFileMP4 *pOutFileMP4 = new OutFileMP4(pDummyProtocol, name, settings);
    if (!pOutFileMP4->SetStreamsManager(pApplication->GetStreamsManager())) {
        FATAL("Unable to set the streams manager");
        delete pOutFileMP4;
        pOutFileMP4 = NULL;
        return NULL;
    }
    pDummyProtocol->SetDummyStream(pOutFileMP4);

    //6. Done
    return pOutFileMP4;
}

OutFileMP4::~OutFileMP4() {
    CloseMP4();
    if (_pFlushTimer != NULL) {
        delete _pFlushTimer;
    }
}

void OutFileMP4::InitializeTimer() {
    if (_pFlushTimer == NULL) {
        _pFlushTimer = new FlushTimerProtocol(this);
        _pFlushTimer->EnqueueForTimeEvent(FLUSH_INTERVAL);
    }
}

bool OutFileMP4::InitializeFromFile(string filePath, bool recoveryMode) {
    _recoveryMode = recoveryMode;
    _filePath = filePath;
    _normalOperation = false;

    _pAudio = new Track();
    _pVideo = new Track();

    // Initialize the files
    _pMdatFile = new File();
    if (!_pMdatFile->Initialize(filePath + ".mdat", FILE_OPEN_MODE_WRITE)) {
        string error = "Unable to open file " + filePath + ".mdat";
        FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }

    // Seek to end of MDAT
    if (!_pMdatFile->SeekEnd()) {
        string error = "Unable to seek into MDAT file!";
        FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
        GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }

    _pInfoFile = new File();
    if (!_pInfoFile->Initialize(filePath + ".info", FILE_OPEN_MODE_READ)) {
        string error = "Unable to open file " + filePath + ".info";
        FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
        GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }

    // Initialize STSS
    // _entryCount
    if (!_pInfoFile->PeekUI32(&_pVideo->_stss._entryCount, true)) return false;
    //DEBUG("_pVideo->_stss._entryCount: %"PRIu32, _pVideo->_stss._entryCount);
    // _content + _entryCount
    _pVideo->_stss._content.IgnoreAll();
    if (!_pVideo->_stss._content.ReadFromFs(*_pInfoFile,
            (_pVideo->_stss._entryCount + 1) * sizeof (uint32_t))) return false;

    // Initialize the headers
    // _mdatDataSizeOffset
    // DEBUG("Reading at 0x%016"PRIx64, _pInfoFile->Cursor());
    if (!_pInfoFile->ReadUI64(&_mdatDataSizeOffset, true)) return false;
    // DEBUG("_mdatDataSizeOffset: %"PRIu64, _mdatDataSizeOffset);
    // _mdatDataSize
    if (!_pInfoFile->ReadUI64(&_mdatDataSize, true)) return false;
    // _mvhdTimescale
    if (!_pInfoFile->ReadUI32(&_mvhdTimescale, true)) return false;
    // _winQtCompat
    uint8_t tmpVal = 0;
    if (!_pInfoFile->ReadUI8(&tmpVal)) return false;
    if (tmpVal) {
        _winQtCompat = true;
    } else {
        _winQtCompat = false;
    }
    // _creationTime
    if (!_pInfoFile->ReadUI64(&_creationTime, true)) return false;
    // _nextTrackId
    if (!_pInfoFile->ReadUI32(&_nextTrackId, true)) return false;
    // hasAudio/hasVideo
    uint8_t avFlag = 0;
    if (!_pInfoFile->ReadUI8(&avFlag)) return false;

    //DEBUG("_mdatDataSizeOffset: %"PRIu64, _mdatDataSizeOffset);
    //DEBUG("_mvhdTimescale: %"PRIu32, _mvhdTimescale);
    //	if (_winQtCompat) {
    //		DEBUG("_winQtCompat: true");
    //	} else {
    //		DEBUG("_winQtCompat: false");
    //	}
    //DEBUG("_creationTime: %"PRIu64, _creationTime);
    //DEBUG("_nextTrackId: %"PRIu32, _nextTrackId);

    // Initialize the individual tracks
    // Audio
    if (avFlag & 0x01) {
        if (!_pAudio->InitFromFile(_pInfoFile, filePath, true)) {
            FATAL("Unable to initialize Audio track!");
            return false;
        }
    }

    // Video
    if (avFlag & 0x02) {
        if (!_pVideo->InitFromFile(_pInfoFile, filePath, false)) {
            FATAL("Unable to initialize Video track!");
            return false;
        }
    }

    return true;
}

bool OutFileMP4::WriteInfoFile() {
    //DEBUG("_mdatDataSizeOffset: %"PRIu64, _mdatDataSizeOffset);
    //DEBUG("_mvhdTimescale: %"PRIu32, _mvhdTimescale);
    //	if (_winQtCompat) {
    //		DEBUG("_winQtCompat: true");
    //	} else {
    //		DEBUG("_winQtCompat: false");
    //	}
    //DEBUG("_creationTime: %"PRIu64, _creationTime);
    //DEBUG("_nextTrackId: %"PRIu32, _nextTrackId);

    if (_pInfoFile == NULL) {
        FATAL("Empty Info file");
        return false;
    }

    // Write the remaining STSS entries
    if (_pVideo != NULL && _pVideo->_id > 0) {
        //DEBUG("_pVideo->_stss._entryCount: %"PRIu32, _pVideo->_stss._entryCount);
        if (!_pVideo->_stss.Finalize()) {
            FATAL("Could not write STSS entries!");
            return false;
        }
    }
    //	_pInfoFile->SeekTo(_infoWriteBegin);
    // DEBUG("Begin Writing at 0x%016"PRIx64, _pInfoFile->Cursor());
    // Headers: 34 bytes
    // _mdatDataSizeOffset 8
    //DEBUG("Writing mdatDataSizeOffset: %"PRIu64, _mdatDataSizeOffset);
    if (!_pInfoFile->WriteUI64(_mdatDataSizeOffset, true)) return false;
    // _mdatDataSize 8
    if (!_pInfoFile->WriteUI64(_mdatDataSize, true)) return false;
    // _mvhdTimescale 4
    if (!_pInfoFile->WriteUI32(_mvhdTimescale, true)) return false;
    // _winQtCompat 1
    if (_winQtCompat) {
        if (!_pInfoFile->WriteUI8(0x0F)) return false;
    } else {
        if (!_pInfoFile->WriteUI8(0x00)) return false;
    }
    // _creationTime 8
    if (!_pInfoFile->WriteUI64(_creationTime, true)) return false;
    // _nextTrackId 4
    if (!_pInfoFile->WriteUI32(_nextTrackId, true)) return false;
    // hasAudio/hasVideo 1
    uint8_t avFlag = 0;
    if (_pAudio != NULL && _pAudio->_id > 0) avFlag |= 0x01;
    if (_pVideo != NULL && _pVideo->_id > 0) avFlag |= 0x02;
    if (!_pInfoFile->WriteUI8(avFlag)) return false;

    // For now, since we only have two tracks (one for audio and one for video)
    // Audio
    if (_pAudio != NULL && _pAudio->_id > 0) {
        // Track ID
        // DEBUG("_pAudio->_id: %"PRIu32" @ 0x%016"PRIx64, _pAudio->_id, _pInfoFile->Cursor());
        if (!_pInfoFile->WriteUI8((uint8_t) _pAudio->_id)) return false;
        // _timescale
        //DEBUG("_pAudio->_timescale: %"PRIu32, _pAudio->_timescale);
        if (!_pInfoFile->WriteUI32(_pAudio->_timescale, true)) return false;
        // _esDescriptor size
        uint32_t eSize = GETAVAILABLEBYTESCOUNT(_pAudio->_esDescriptor);
        //DEBUG("Audio eSize: %"PRIu32, eSize);
        if (!_pInfoFile->WriteUI32(eSize, true)) return false;
        // _esDescriptor
        if (!_pInfoFile->WriteBuffer(GETIBPOINTER(_pAudio->_esDescriptor), eSize)) return false;

        // STTS
        // _entryCount
        //DEBUG("_pAudio->_stts._entryCount: %"PRIu32, _pAudio->_stts._entryCount);
        if (!_pInfoFile->WriteUI32(_pAudio->_stts._entryCount, true)) return false;
        // _currentDeltaCount
        //DEBUG("_pAudio->_stts._currentDeltaCount: %"PRIu32, _pAudio->_stts._currentDeltaCount);
        if (!_pInfoFile->WriteUI32(_pAudio->_stts._currentDeltaCount, true)) return false;
        // _currentDelta
        //DEBUG("_pAudio->_stts._currentDelta: %"PRIu32, _pAudio->_stts._currentDelta);
        if (!_pInfoFile->WriteUI32(_pAudio->_stts._currentDelta, true)) return false;
        // _lastDts
        //DEBUG("_pAudio->_stts._lastDts: %"PRIu64, _pAudio->_stts._lastDts);
        if (!_pInfoFile->WriteUI64(_pAudio->_stts._lastDts, true)) return false;

        // STSZ
        // _sameSize and _sampleSize/_sampleCount
        if (_pAudio->_stsz._sameSize) {
            if (!_pInfoFile->WriteUI8(0x0F)) return false;
            //DEBUG("_pAudio->_stsz._sampleSize: %"PRIu32, _pAudio->_stsz._sampleSize);
            if (!_pInfoFile->WriteUI32(_pAudio->_stsz._sampleSize, true)) return false;
        } else {
            if (!_pInfoFile->WriteUI8(0x00)) return false;
            //DEBUG("_pAudio->_stsz._sampleCount: %"PRIu32, _pAudio->_stsz._sampleCount);
            if (!_pInfoFile->WriteUI32(_pAudio->_stsz._sampleCount, true)) return false;
        }

        // CO64
        // _64-bit flag
        if (!_pInfoFile->WriteUI32(_pAudio->_co64._is64 ? 1 : 0)) return false;
        // _sampleCount
        //DEBUG("_pAudio->_co64._sampleCount: %"PRIu32, _pAudio->_co64._sampleCount);
        if (!_pInfoFile->WriteUI32(_pAudio->_co64._sampleCount, true)) return false;
    }

    // Video
    if (_pVideo != NULL && _pVideo->_id > 0) {
        // Track ID
        // DEBUG("_pVideo->_id: %"PRIu32" @ 0x%016"PRIx64, _pVideo->_id, _pInfoFile->Cursor());
        if (!_pInfoFile->WriteUI8((uint8_t) _pVideo->_id)) return false;
        // _width
        //DEBUG("_pVideo->_width: %"PRIu32, _pVideo->_width);
        if (!_pInfoFile->WriteUI32(_pVideo->_width, true)) return false;
        // _height
        //DEBUG("_pVideo->_height: %"PRIu32, _pVideo->_height);
        if (!_pInfoFile->WriteUI32(_pVideo->_height, true)) return false;
        // _timescale
        //DEBUG("_pVideo->_timescale: %"PRIu32, _pVideo->_timescale);
        if (!_pInfoFile->WriteUI32(_pVideo->_timescale, true)) return false;
        // _esDescriptor size
        uint32_t eSize = GETAVAILABLEBYTESCOUNT(_pVideo->_esDescriptor);
        //DEBUG("Video eSize: %"PRIu32, eSize);
        if (!_pInfoFile->WriteUI32(eSize, true)) return false;
        // _esDescriptor
        if (!_pInfoFile->WriteBuffer(GETIBPOINTER(_pVideo->_esDescriptor), eSize)) return false;

        // STTS
        // _entryCount
        if (!_pInfoFile->WriteUI32(_pVideo->_stts._entryCount, true)) return false;
        //DEBUG("_pVideo->_stts._entryCount: %"PRIu32, _pVideo->_stts._entryCount);
        // _currentDeltaCount
        if (!_pInfoFile->WriteUI32(_pVideo->_stts._currentDeltaCount, true)) return false;
        //DEBUG("_pVideo->_stts._currentDeltaCount: %"PRIu32, _pVideo->_stts._currentDeltaCount);
        // _currentDelta
        if (!_pInfoFile->WriteUI32(_pVideo->_stts._currentDelta, true)) return false;
        //DEBUG("_pVideo->_stts._currentDelta: %"PRIu32, _pVideo->_stts._currentDelta);
        // _lastDts
        //DEBUG("_pVideo->_stts._lastDts: %"PRIu64, _pVideo->_stts._lastDts);
        if (!_pInfoFile->WriteUI64(_pVideo->_stts._lastDts, true)) return false;

        // STSZ
        // _sameSize and _sampleSize/_sampleCount
        if (_pVideo->_stsz._sameSize) {
            if (!_pInfoFile->WriteUI8(0x0F)) return false;
            if (!_pInfoFile->WriteUI32(_pVideo->_stsz._sampleSize, true)) return false;
            //DEBUG("_pVideo->_stts._sampleSize: %"PRIu32, _pVideo->_stsz._sampleSize);
        } else {
            if (!_pInfoFile->WriteUI8(0x00)) return false;
            if (!_pInfoFile->WriteUI32(_pVideo->_stsz._sampleCount, true)) return false;
            //DEBUG("_pVideo->_stts._sampleCount: %"PRIu32, _pVideo->_stsz._sampleCount);
        }

        // CTTS
        // _entryCount
        if (!_pInfoFile->WriteUI32(_pVideo->_ctts._entryCount, true)) return false;
        //DEBUG("_pVideo->_ctts._entryCount: %"PRIu32, _pVideo->_ctts._entryCount);
        // _currentCtsCount
        if (!_pInfoFile->WriteUI32(_pVideo->_ctts._currentCtsCount, true)) return false;
        //DEBUG("_pVideo->_ctts._currentCtsCount: %"PRIu32, _pVideo->_ctts._currentCtsCount);
        // _currentCts
        if (!_pInfoFile->WriteUI32(_pVideo->_ctts._currentCts, true)) return false;
        //DEBUG("_pVideo->_ctts._currentCts: %"PRIu32, _pVideo->_ctts._currentCts);

        // CO64
        // _64-bit flag
        if (!_pInfoFile->WriteUI32(_pVideo->_co64._is64 ? 1 : 0)) return false;
        // _sampleCount
        if (!_pInfoFile->WriteUI32(_pVideo->_co64._sampleCount, true)) return false;
        //DEBUG("_pVideo->_co64._sampleCount: %"PRIu32, _pVideo->_co64._sampleCount);
    }

    return true;
}

bool OutFileMP4::Assemble() {
	File mp4File;
	if (!mp4File.Initialize(_filePath, FILE_OPEN_MODE_TRUNCATE)) {
		string error = "Unable to open file " + _filePath;
		FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
		return false;
	}

	//Write ftyp and moov so that we can determine the size preceding the mdat atom
	if (!WriteFTYP(&mp4File)) {
		FATAL("Unable to write FTYP");
		return false;
	}

	uint64_t moovLocation = mp4File.Cursor(); //Save the moov location since we need to overwrite it later

	if (!WriteMOOV(&mp4File)) {
		FATAL("Unable to write moov");
		return false;
	}

	uint64_t headerSize = mp4File.Size(); //Stores the size of ftyp+moov, it will be added in sample offsets

	//Transform the sample offsets to mdat in co64 atom. This is needed because when the moov
	//atom is repositioned over the mdat, the offsets of each sample is also invalidated.
	//The code below will adjust the sample offsets from moov to point to the valid offset in mdat
	if (NULL != _pVideo 
		&& _pVideo->_co64._sampleCount > 0
		&& _pVideo->_sampleOffsets.size() > 0) {
		uint8_t* co64Buffer = _pVideo->_co64.Content();
		for (uint32_t i = 0; i < _pVideo->_co64._sampleCount; ++i) {
			uint64_t offsetIndex = 4 + (i * sizeof(uint64_t));
			uint64_t newoffset = ENTOHLL(_pVideo->_sampleOffsets[i]) + headerSize;
			memset(co64Buffer + offsetIndex, 0x00, sizeof(uint64_t));
			EHTONLLP((co64Buffer + offsetIndex), newoffset);
		}
	}

	if (NULL != _pAudio 
		&& _pAudio->_co64._sampleCount > 0
		&& _pAudio->_sampleOffsets.size() > 0) {
		uint8_t* co64Buffer = _pAudio->_co64.Content();
		for (uint32_t i = 0; i < _pAudio->_co64._sampleCount; ++i) {
			uint64_t offsetIndex = 4 + (i * sizeof(uint64_t));
			uint64_t newoffset = ENTOHLL(_pAudio->_sampleOffsets[i]) + headerSize;
			memset(co64Buffer + offsetIndex, 0x00, sizeof(uint64_t));
			EHTONLLP((co64Buffer + offsetIndex), newoffset);
		}
	}

	//Repoints the cursor to the start of the moov atom
	if (!mp4File.SeekTo(moovLocation)) {
		FATAL("Unable to seek into mp4 file");
		return false;
	}
	//Rewrite the moov atom with the updated sample offsets
	if (!WriteMOOV(&mp4File)) {
		FATAL("Unable to write moov");
		return false;
	}
	
	if (!_pMdatFile->SeekBegin()) {
		FATAL("Unable to seek into mdat file");
		return false;
	}

	uint64_t mdatBufferCount = _pMdatFile->Size();
	uint8_t* mdatBuffer = new uint8_t[mdatBufferCount];
	//Copy the content of the .mdat file to memory
	if (!_pMdatFile->ReadBuffer(mdatBuffer, mdatBufferCount)) {
		FATAL("Unable to read mdat");
		return false;
	}
	_pMdatFile->Close();

	//Append the mdat atom to the mp4 file
	if (!mp4File.WriteBuffer(mdatBuffer, mdatBufferCount)) {
		FATAL("Unable to write mdat to mp4");
		return false;
	}

	delete[] mdatBuffer;
	mp4File.Close();

    return true;
}

//void OutFileMP4::SetWriter(BaseClientApplication *pApp, string binPath) {
//	_pOriginApp = pApp;
//	_mp4BinPath = binPath;
//}

bool OutFileMP4::OpenMP4() {
    // Close any existing files
    if (!CloseMP4()) {
        string error = "Could not close MP4 file!";
        FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
        GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }

    _filePath = (string) _settings["computedPathToFile"];

    if (_chunkLength > 0) {
        if (_settings.HasKeyChain(_V_NUMERIC, false, 1, "preset")) {
            uint64_t preset = (uint64_t) _settings["preset"];
			bool dateFolderStructure = (bool) _settings["dateFolderStructure"];
            if (preset == 1 || dateFolderStructure) { //see also "preset" in OriginApplication::RecordStream
				// Remove obsolete localstreamname & date from extended file path
				string recordFolder = _filePath;
				size_t slashPos = _filePath.length();
				int count = 0;
				do {
					slashPos = _filePath.find_last_of(PATH_SEPARATOR, slashPos - 1);
				} while ((++count < 3) && (slashPos != string::npos));
				if (slashPos != string::npos) {
					recordFolder = _filePath.substr(0, slashPos);
				}
				// Append latest localstreamname and date to truncated file path (original pathToFile)
				time_t now = time(NULL);
				Timestamp tsNow = *localtime(&now);
				char datePart[9];
				char timePart[7];
				strftime(datePart, 9, "%Y%m%d", &tsNow);
				strftime(timePart, 7, "%H%M%S", &tsNow);
				recordFolder += format("%c%s%c%s%c", PATH_SEPARATOR, STR(GetName()), PATH_SEPARATOR,
						datePart, PATH_SEPARATOR);
				if (!fileExists(recordFolder)) {
					if (!createFolder(recordFolder, true)) {
						FATAL("Unable to create folder %s", STR(recordFolder));
						return false;
					}
				}
				// Append filename in streamname-date-time format
				_filePath = recordFolder + format("%s-%s-%s.mp4", STR(GetName()), datePart, timePart);
            } else {
                string count = format("_part%04"PRIu32".mp4", _chunkCount);
                replace(_filePath, ".mp4", count);
                _chunkCount++;
            }
        }
    }

    _pMdatFile = new File();
    if (!_pMdatFile->Initialize(_filePath + ".mdat", FILE_OPEN_MODE_TRUNCATE)) {
        string error = "Unable to open file " + _filePath + ".mdat";
        FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
        GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }

    _pInfoFile = new File();
    if (!_pInfoFile->Initialize(_filePath + ".info", FILE_OPEN_MODE_TRUNCATE)) {
        string error = "Unable to open file " + _filePath + ".info";
        FATAL("%s", STR(error));
		_chunkInfo["errorMsg"] = error;
        GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }
//    if (_pFlushTimer != NULL) _pFlushTimer->Enable();
    _chunkInfo["file"] = _filePath;
	_startTime = (string) Variant::Now();
	_chunkInfo["startTime"] = _startTime;
	// include user defined variables (starting with "_") into the event
	FOR_MAP(_settings, string, Variant, i) {
		if (MAP_KEY(i).substr(0, 1) == "_") {
			_chunkInfo[MAP_KEY(i)] = MAP_VAL(i);
		}
	}
    GetEventLogger()->LogRecordChunkCreated(_chunkInfo);

#ifdef DEBUG_MP4
    _pHeaderFile = new File();
    if (!_pHeaderFile->Initialize(_filePath + ".header", FILE_OPEN_MODE_TRUNCATE)) {
        FATAL("Unable to open file %s", STR(_filePath));
        return false;
    }
#endif
    /*if (!WriteFTYP(_pMdatFile)) {
        FATAL("Unable to write FTYP");
        return false;
    }*/

    // Reset _mdatDataSize
    _mdatDataSizeOffset = 8;
    _mdatDataSize = 0;

    //00 00 00 01 m d a t XX XX XX XX XX XX XX XX
    if ((!_pMdatFile->WriteUI64(0x000000016D646174LL, true))
            || (!_pMdatFile->WriteUI64(0, true))) {
        FATAL("Unable to write the mdat header");
        return false;
    }

    if (_permanentData._audioId > 0) {
        _pAudio = new Track();
        _pAudio->_id = _permanentData._audioId;
        _pAudio->_width = 0;
        _pAudio->_height = 0;
        _pAudio->_timescale = _permanentData._audioTimescale;
        _pAudio->_esDescriptor.ReadFromInputBuffer(_permanentData._audioEsDescriptor,
                GETAVAILABLEBYTESCOUNT(_permanentData._audioEsDescriptor));
        _pAudio->_stts._timescale = _pAudio->_timescale;
        _pAudio->_ctts._timescale = _pAudio->_timescale;
		_pAudio->_co64._is64 = true; //Set the flag to 64 bit
        // Initialize the audio track
        if (!_pAudio->Initialize(_filePath, true, true)) {
            FATAL("Could not initialize audio track!");
            return false;
        }
    }

    if (_permanentData._videoId > 0) {
        _pVideo = new Track();
        _pVideo->_id = _permanentData._videoId;
        _pVideo->_width = _permanentData._videoWidth;
        _pVideo->_height = _permanentData._videoHeight;
        _pVideo->_timescale = _permanentData._videoTimescale;
        _pVideo->_esDescriptor.ReadFromInputBuffer(_permanentData._videoEsDescriptor,
                GETAVAILABLEBYTESCOUNT(_permanentData._videoEsDescriptor));
        _pVideo->_stts._timescale = _pVideo->_timescale;
        _pVideo->_ctts._timescale = _pVideo->_timescale;
		_pVideo->_co64._is64 = true; //Set the flag to 64 bit
        // Initialize the video track
        if (!_pVideo->Initialize(_filePath, false, true)) {
            FATAL("Could not initialize video track!");
            return false;
        }
        if (!_pVideo->_stss.SetInfoFile(_pInfoFile)) {
            FATAL("Could not initialize video track!");
            return false;
        }
    }

    return true;
}

bool OutFileMP4::CloseMP4() {
    // Nothing to close
    if (_pFlushTimer != NULL) _pFlushTimer->Enable(false);

    if ((_pAudio == NULL) && (_pVideo == NULL)) return true;

    if (_pMdatFile != NULL) {
        // Nothing to do here if NOT normal operation
        if (_pVideo != NULL) _pVideo->_stts.Finish();
        if (_pAudio != NULL) _pAudio->_stts.Finish();
        if (_normalOperation) {
            if (!Finalize()) {
                if (_pMdatFile->IsOpen())
                    WARN("Recording failed! Partial file: %s", STR(_pMdatFile->GetPath()));
                else
                    WARN("Recording failed!");
            }
        }

        _pMdatFile->Close();
        delete _pMdatFile;
        _pMdatFile = NULL;
    }

    if (_pAudio != NULL) {
        delete _pAudio;
        _pAudio = NULL;
    }

    if (_pVideo != NULL) {
        delete _pVideo;
        _pVideo = NULL;
    }

    if (_pInfoFile != NULL) {
        _pInfoFile->Close();
        delete _pInfoFile;
        _pInfoFile = NULL;
    }
#ifdef DEBUG_MP4
    if (_pHeaderFile != NULL) {
        delete _pHeaderFile;
        _pHeaderFile = NULL;
    }
#endif

    if (_normalOperation) {
        // Launch a process that would assemble the MP4
        Variant settings;
        string arguments = "-path=" + _filePath;

        settings["header"] = "launchProcess";
        settings["payload"]["fullBinaryPath"] = _mp4BinPath;
        settings["payload"]["arguments"] = arguments;
        settings["payload"]["keepAlive"] = false;

        ClientApplicationManager::SendMessageToApplication("rdkcms", settings);
    } else {
        // Clean-up
        deleteFile(_filePath + ".info");
		deleteFile(_filePath + ".mdat");
        for (uint8_t i = 1; i < _nextTrackId; i++) {
            string track = format("%s.track%"PRIu8, STR(_filePath), i);
            deleteFile(track);
        }
    }

    _pApp->GetRecordSession()[GetName()].ResetFlag(RecordingSession::FLAG_MP4);

    _chunkInfo["file"] = _filePath;
    _chunkInfo["startTime"] = _startTime;
    _chunkInfo["stopTime"] = Variant::Now();
    _chunkInfo["frameCount"] = _frameCount;
    GetEventLogger()->LogRecordChunkClosed(_chunkInfo);

    return true;
}

bool OutFileMP4::FinishInitialization(
        GenericProcessDataSetup *pGenericProcessDataSetup) {
    if (!BaseOutStream::FinishInitialization(pGenericProcessDataSetup)) {
        FATAL("Unable to finish output stream initialization");
        return false;
    }

    // Check first the presence of the mp4writer
    if ((!_settings.HasKeyChain(V_STRING, false, 1, "mp4BinPath"))
            || ((_mp4BinPath = normalizePath((string) _settings["mp4BinPath"], "")) == "")) {
        FATAL("Could not normalize mp4 binary path! Make sure that config.lua is set up properly and has a valid value for mp4BinPath");
        return false;
    }

	// include user defined variables (starting with "_") into the event
	if (_settings.MapSize() > 0) {
		FOR_MAP(_settings, string, Variant, i) {
			if (MAP_KEY(i).substr(0, 1) == "_") {
				_chunkInfo[MAP_KEY(i)] = MAP_VAL(i);
			}
		}
	}

    _winQtCompat = (bool) _settings["winQtCompat"];
    _waitForIDR = (bool) _settings["waitForIDR"];

    //video setup
    pGenericProcessDataSetup->video.avc.Init(
            NALU_MARKER_TYPE_SIZE, //naluMarkerType,
            false, //insertPDNALU,
            false, //insertRTMPPayloadHeader,
            true, //insertSPSPPSBeforeIDR,
            true //aggregateNALU
            );

    //audio setup
    pGenericProcessDataSetup->audio.aac._insertADTSHeader = false;
    pGenericProcessDataSetup->audio.aac._insertRTMPPayloadHeader = false;

    //misc setup
    pGenericProcessDataSetup->_timeBase = 0;
    pGenericProcessDataSetup->_maxFrameSize = 16 * 1024 * 1024;

    // Chunk length in milliseconds
    _chunkLength = ((uint64_t) _settings["chunkLength"])*1000;

    if (pGenericProcessDataSetup->_hasAudio) {
        _permanentData._audioId = _nextTrackId;
        _nextTrackId++;
        switch (pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodecType()) {
            case CODEC_AUDIO_AAC:
            {
                AudioCodecInfoAAC *pTemp = pGenericProcessDataSetup->_pStreamCapabilities->GetAudioCodec<AudioCodecInfoAAC>();
                if (pTemp == NULL) {
                    FATAL("Invalid audio codec");
                    return false;
                }
                uint8_t raw[] = {
                    0x03, //ES_Descriptor tag=ES_DescrTag, ISO_IEC_14496-1;2010(E)-Character_PDF_document page 47/158
                    0x80, 0x80, 0x80, 0x22, //length
                    0xFF, 0xFF, //<ES_ID>
                    0x00, //streamDependenceFlag,URL_Flag,OCRstreamFlag,streamPriority
                    0x04, //DecoderConfigDescriptor tag=DecoderConfigDescrTag, ISO_IEC_14496-1;2010(E)-Character_PDF_document page 48/158
                    0x80, 0x80, 0x80, 0x14, //length
                    0x40, //objectTypeIndication, table 5 ISO_IEC_14496-1;2010(E)-Character_PDF_document page 49/158
                    0x15, //streamType=3, upStream=1, reserved=1
                    0x00, 0x00, 0x00, //bufferSizeDB
                    0x00, 0x00, 0x00, 0x00, //maxBitrate
                    0x00, 0x00, 0x00, 0x00, //avgBitrate
                    0x05, //AudioSpecificConfig tag=DecSpecificInfoTag
                    0x80, 0x80, 0x80, 0x02, //length
                    0xFF, 0xFF, //<codecBytes>, ISO-IEC-14496-3_2005_MPEG4_Audio page 49/1172
                    0x06, //SLConfigDescriptor tag=SLConfigDescrTag
                    0x80, 0x80, 0x80, 0x01, //length
                    0x02
                };
                EHTONSP(raw + 5, (uint16_t) _permanentData._audioId);
                memcpy(raw + 31, pTemp->_pCodecBytes, 2);
                _permanentData._audioEsDescriptor.ReadFromBuffer(raw, sizeof (raw));
                _permanentData._audioTimescale = GetInStream()->GetInputAudioTimescale();
                break;
            }
            default:
            {
                FATAL("Audio codec not supported");
                return false;
            }
        }
    }

    if (pGenericProcessDataSetup->_hasVideo) {
        _permanentData._videoId = _nextTrackId;
        _nextTrackId++;
        switch (pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodecType()) {
            case CODEC_VIDEO_H264:
            {
                VideoCodecInfoH264 *pTemp = pGenericProcessDataSetup->_pStreamCapabilities->GetVideoCodec<VideoCodecInfoH264>();
                if ((pTemp == NULL)
                        || (pTemp->_width == 0)
                        || (pTemp->_height == 0)) {
                    FATAL("Invalid video codec");
                    return false;
                }
                _permanentData._videoWidth = pTemp->_width;
                _permanentData._videoHeight = pTemp->_height;
                IOBuffer &raw = pTemp->GetRTMPRepresentation();
                _permanentData._videoEsDescriptor.ReadFromBuffer(GETIBPOINTER(raw) + 5,
                        GETAVAILABLEBYTESCOUNT(raw) - 5);
                _permanentData._videoTimescale = GetInStream()->GetInputVideoTimescale();
                break;
            }
            default:
            {
                FATAL("Video codec not supported");
                return false;
            }
        }
    } else {
        _onlyAudio = true;
    }

    if (_waitForIDR) {
        _hasInitialkeyFrame = !pGenericProcessDataSetup->_hasVideo;
    } else {
        _hasInitialkeyFrame = true;
    }

    if (_winQtCompat) {
        if (_creationTime >= 0x100000000LL)
            _creationTime = 0;
    }

    OpenMP4();

    /*
     * ftyp
     *	-major_brand			isom
     *	-minor_version			0x200
     *	-compatible_brands[]	isom,iso2,avc1
     * moov
     * 	mvhd
     *		-version			1
     *		-flags				0
     *		-creation_time		time(NULL)
     *		-modification_time	creation_time
     *		-timescale			1000
     *		-duration			max value from all tkhd:duration in mvhd:timescale
     *		-rate				0x00010000
     *		-volume				0x0100
     *		-reserved			0
     *		-reserved			0
     *		-matrix				0x00010000,0,0,0,0x00010000,0,0,0,0x40000000
     *		-pre_defined		0
     *		-next_track_ID		max(trak:track_ID)+1
     * 	iods - this is optional, should be skipped
     * 	trak
     * 		tkhd
     *			-version			1
     *			-flags				0x0f
     *			-creation_time		mvhd:creation_time
     *			-modification_time	mvhd:creation_time
     *			-track_ID			incremental number starting from 1
     *			-reserved			0
     *			-duration			duration in mvhd:timescale
     *			-reserved			0
     *			-layer				0 (this is the Z order)
     *			-alternate_group	a unique number for each track. This is how we can group multiple tracks on the same logical track (audio in multiple languages for example)
     *			-volume				if track is audio 0x0100 else 0
     *			-reserved			0
     *			-matrix				0x00010000,0,0,0,0x00010000,0,0,0,0x40000000
     *			-width				0 for non-video otherwise see libavformat/movenc.c
     *			-height				0 for non-video otherwise see libavformat/movenc.c
     * 		edts - this is optional, should be skipped
     * 		mdia
     * 			mdhd
     *				-version			1
     *				-flags				0
     *				-creation_time		mvhd:creation_time
     *				-modification_time	mvhd:creation_time
     *				-timescale			1000 for video and audio rate for audio.
     *				-duration			duration in mdhd:timescale
     *				-language			default it to ENG or to the "unknown" language
     *				-pre_defined		0
     * 			hdlr(vide)
     *				-version			0
     *				-flags				0
     *				-pre_defined		0
     *				-handler_type		vide
     *				-reserved			0
     *				-name				null terminated string to identify the track. NULL in our case
     * 			minf
     * 				vmhd
     *					-version			0
     *					-flags				1
     *					-graphicsmode		0
     *					-opcolor			0
     * 				dinf
     * 					dref
     *						-version			0
     *						-flags				0
     *						-entry_count		1
     * 						url
     *							-version			0
     *							-flags				1
     *							-location			NULL(0)
     * 				stbl
     * 					stsd
     *						-version			0
     *						-flags				0
     *						-entry_count		1
     * 						avc1
     *							-reserved				0
     *							-data_reference_index	1
     *							-pre_defined			0
     *							-reserved				0
     *							-pre_defined			0
     *							-width					width of video
     *							-height					height of video
     *							-horizresolution		0x00480000 (72DPI)
     *							-vertresolution			0x00480000 (72DPI)
     *							-reserved				0
     *							-frame_count			1 (how many frames per sample)
     *							-compressorname			0x16, 'E','v','o',	'S','t','r','e','a','m',' ','M','e','d','i','a',' ','S','e','r','v','e','r',0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
     *							-depth					0x18
     *							-pre_defined			0xffff
     * 							avcC
     *								-here we just deposit the RTMP representation for SPS/PPS from stream capabilities
     * 							btrt - this is optional, should be skipped
     * 					stts (time-to-sample, and DTS must be used, not PTS)
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						*(sample_count		number of samples
     *						sample_delta)		DTS delta expressed in mdia:timescale
     * 					ctts (compositionTime-to-sample)
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						*(sample_count		number of samples
     *						sample_offset)		CTS expressed in mdia:timescale
     * 					stss (sync sample table (random access points))
     *						-version			0
     *						-flags				0
     *						-entry_count		how many entries we have
     *						-sample_number		the ordinal number of the sample which is also a key frame
     * 					stsc (sample-to-chunk, partial data-offset information). We are going to have only one chunk per file
     *						-version					0
     *						-flags						0
     *						-entry_count				1
     *						-first_chunk				1
     *						-samples_per_chunk			total frames count in this track
     *						-sample_description_index	1
     * 					stsz (sample sizes (framing))
     *						-version					0
     *						-flags						0
     *						-sample_size				0 (we ware going to always specify the sample sizes)
     *						-sample_count				total frames count in this track
     *						*entry_size					size of each frame
     * 					co64 (64-bit chunk offset, we are going to always use 64 bits, not stco)
     *						-version					0
     *						-flags						0
     *						-entry_count				1 (we are going to have only one chunk)
     *						-chunk_offset				where the first frame begins, regardless of the audio/video type
     * 	trak
     * 		tkhd (solved)
     * 		mdia (solved)
     * 			mdhd (solved)
     * 			hdlr(soun)
     *				-version			0
     *				-flags				0
     *				-pre_defined		0
     *				-handler_type		soun
     *				-reserved			0
     *				-name				null terminated string to identify the track. NULL in our case
     * 			minf
     * 				smhd (sound media header, overall information (sound track only))
     *					-version			0
     *					-flags				0
     *					-balance			0
     *					-reserved			0
     * 				dinf
     * 					dref (solved)
     * 						url (solved)
     * 				stbl
     * 					stsd (solved)
     * 						mp4a
     *							-reserved				0
     *							-data_reference_index	1
     *							-reserved				0
     *							-channelcount			2
     *							-samplesize				16
     *							-pre_defined			0
     *							-reserved				0
     *							-samplerate				(audio_sample_rate)<<16
     * 							esds
     *								-version			0
     *								-flags				0
     *								-ES_Descriptor
     * 					stts (solved)
     * 					stsc (solved)
     * 					stsz (solved)
     * 					co64 (solved)
     * mdat (solved)
     * free (solved)
     */

    InitializeTimer();
	if (_pFlushTimer != NULL) _pFlushTimer->Enable();
    return true;
}

bool OutFileMP4::PushVideoData(IOBuffer &buffer, double pts, double dts,
        bool isKeyFrame) {

    if (!_hasInitialkeyFrame) {
        if (!isKeyFrame) {
            return true;
        }
        _hasInitialkeyFrame = true;
        _ptsDtsDelta = dts;
        //		DEBUG("_ptsDtsDelta: %f", _ptsDtsDelta);
    }
    _frameCount++;

    if (PushData(
            _pVideo,
            GETIBPOINTER(buffer),
            GETAVAILABLEBYTESCOUNT(buffer),
            pts,
            dts,
            isKeyFrame)) {
        return true;
    } else {
		_chunkInfo["errorMsg"] = "Could not write video sample to " + _filePath;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }
}

bool OutFileMP4::PushAudioData(IOBuffer &buffer, double pts, double dts) {
    if (!_hasInitialkeyFrame)
        return true;

    if (PushData(
            _pAudio,
            GETIBPOINTER(buffer),
            GETAVAILABLEBYTESCOUNT(buffer),
            pts,
            dts,
            false)) {
        return true;
    } else {
		_chunkInfo["errorMsg"] = "Could not write audio sample to " + _filePath;
		GetEventLogger()->LogRecordChunkError(_chunkInfo);
        return false;
    }
}

bool OutFileMP4::PushData(Track *track, const uint8_t *pData, uint32_t dataLength,
        double pts, double dts, bool isKeyFrame) {
    // Adjust PTS/DTS
    pts = pts < _ptsDtsDelta ? 0 : pts - _ptsDtsDelta;
    dts = dts < _ptsDtsDelta ? 0 : dts - _ptsDtsDelta;

    // Check if we need to create a new chunk
    if ((_chunkLength > 0)
            && (dts > _chunkLength)
            && ((!_waitForIDR) || (_waitForIDR && isKeyFrame) || _onlyAudio)) {

        bool isAudio = track->_isAudio;

        // Open a new MP4 file
        OpenMP4();

        if (isAudio) {
            track = _pAudio;
        } else {
            track = _pVideo;
        }

        // Increment PTS/DTS delta, then adjust PTS/DTS
        _ptsDtsDelta += dts;
        pts = 0;
        dts = 0;
    }

    //STTS
    track->_stts.PushDTS(dts);

    //STSZ
    track->_stsz.PushSize(dataLength);

    if (!track->_isAudio) {
        //STSS
        if (isKeyFrame)
            track->_stss.PushKeyFrameIndex(track->_stsz._sampleCount);

        //CTTS
        track->_ctts.PushCTS(pts - dts);
    }

    //CO64
    track->_co64.PushOffset(_pMdatFile->Cursor());

    // Check if needed to write to file already
    if (!track->WriteToFile()) {
        FATAL("Unable to write to track file!");
        return false;
    }

    if (!StoreDataToFile(pData, dataLength)) {
        FATAL("Unable to store data!");
        return false;
    }
    if (_flushNow && !FlushToFile()) {
        FATAL("Unable to write to info file!");
        return false;
    }
    _flushNow = false;
    return true;
}

bool OutFileMP4::StoreDataToFile(const uint8_t *pData, uint32_t dataLength) {
    if (!_pMdatFile->WriteBuffer(pData, dataLength)) {
        FATAL("Unable to write data to file");
        return false;
    }
    _mdatDataSize += dataLength;
    return true;
}

uint64_t OutFileMP4::Duration(uint32_t timescale) {
    uint64_t audio = _pAudio->Duration(timescale);
    //DEBUG("audio: %"PRIu64, audio);
    uint64_t video = _pVideo->Duration(timescale);
    //DEBUG("video: %"PRIu64, video);
    uint64_t result = audio < video ? video : audio;
    //DEBUG("result: %"PRIu64, result);
    if (_winQtCompat) {
        if (result >= 0x100000000LL)
            result = 0xffffffff;
    }
    //DEBUG("result: %"PRIu64, result);
    return result;
}

bool OutFileMP4::Finalize() {

    if (!FinalizeMdat())
        return false;
    // Build MP4 using old code and DEBUG_MP4 enabled
    //	if (!_pMdatFile->SeekEnd()) {
    //		FATAL("Unable to seek into file");
    //		return false;
    //	}
    //	if (!WriteMOOV(_pMdatFile)) {
    //		FATAL("Unable to write moov");
    //		return false;
    //	}
    //
    //	string from = _pMdatFile->GetPath();
    //	string to = (string) _settings["computedPathToFile"];
    //	_pMdatFile->Close();
    //
    //	return moveFile(from, to);

    if (!WriteInfoFile()) {
        FATAL("Unable to write info!");
        return false;
    }
#ifdef DEBUG_MP4
    if (!WriteMOOV(_pHeaderFile)) {
        FATAL("Unable to write moov");
        return false;
    }

    _pHeaderFile->Close();
#endif
    return true;
}

bool OutFileMP4::WriteFTYP(File *pFile) {
    /*
     * ftyp
     *	-major_brand			isom
     *	-minor_version			0x200
     *	-compatible_brands[]	isom,iso2,avc1,mp41
     */
    return pFile->WriteBuffer(_ftyp, sizeof (_ftyp));
}

bool OutFileMP4::WriteMOOV(File *pFile) {
    uint64_t sizeOffset = pFile->Cursor();
    if ((!pFile->WriteUI64(0x000000006D6F6F76LL, true)) //XX XX XX XX 'm' 'o' 'o' 'v'
            || (!WriteMVHD(pFile)) //mvhd
            || (!WriteTRAK(pFile, _pVideo)) //trak(vide)
            || (!WriteTRAK(pFile, _pAudio)) //trak(soun)
            || (!UpdateSize(pFile, sizeOffset))
            ) {
        FATAL("Unable to write moov");
        return false;
    }

    return true;
}

void OutFileMP4::SignalFlush() {
    _flushNow = true;
}

bool OutFileMP4::FlushToFile() {

    // Prepare Mdat
    if (!FinalizeMdat()) {
        FATAL("Cannot flush MDAT file: %s", STR(_pMdatFile->GetPath()));
        return false;
    }
    _pMdatFile->SeekEnd();
    // flush tracks
    if (_pVideo != NULL) _pVideo->Finalize();
    if (_pAudio != NULL) _pAudio->Finalize();

    // Flush info file
    uint64_t infoCursor = _pInfoFile->Cursor();
    if (!WriteInfoFile()) {
        FATAL("Cannot flush MDAT file: %s", STR(_pMdatFile->GetPath()));
        return false;
    }

    // Return all files and tracks to previous positions
    _pInfoFile->SeekTo(infoCursor);
    if (_pVideo != NULL) _pVideo->Unfinalize();
    if (_pAudio != NULL) _pAudio->Unfinalize();
    _pMdatFile->SeekEnd();

    return true;
}

bool OutFileMP4::FinalizeMdat() {
    if (_pMdatFile == NULL) {
        FATAL("No file opened");
        return false;
    }
    //	uint64_t mDatEnd = _pMdatFile->Cursor();
    if (!_pMdatFile->SeekTo(_mdatDataSizeOffset)) {
        FATAL("Unable to seek into file");
        return false;
    }

    if (!_pMdatFile->WriteUI64(_mdatDataSize + 16, true)) {
        FATAL("Unable to write bytes into file");
        return false;
    }
    //	WARN("MDAT File size: %"PRIu64, _pMdatFile->Size());
    //	WARN("Cursor diff: %"PRIu64, mDatEnd - _mdatDataSizeOffset);
    //	WARN("mdat Data Size: %"PRIu64, _mdatDataSize + 16);
    return true;
}
