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


#ifdef HAS_STREAM_DEBUG
#include "streaming/streamdebug.h"

uint64_t StreamDebugFile::_streamDebugIdGenerator = 0;

StreamDebugFile::StreamDebugFile(string path, size_t structSize, uint32_t chunkSize) {
	_pFile = new File();
	if (!_pFile->Initialize(path, FILE_OPEN_MODE_TRUNCATE)) {
		ASSERT("Unable to open stream debug file %s", STR(path));
	}
	_structSize = structSize;
	_dataCount = (chunkSize / _structSize) + 1;
	_pData = new uint8_t[_dataCount * _structSize];
	memset(_pData, 0, _dataCount * _structSize);
	_dataCountCursor = 0;
	FINEST("StructDebug size: %"PRIu32" on file `%s`", _structSize, STR(path));
	//	FINEST("StreamDebugRTP size: %"PRIu32, (uint32_t)sizeof (StreamDebugRTP));
	//	FINEST("StreamDebugHLS size: %"PRIu32, (uint32_t)sizeof (StreamDebugHLS));
	//	FINEST("StreamDebugRTCP size: %"PRIu32, (uint32_t)sizeof (StreamDebugRTCP));
}

StreamDebugFile::~StreamDebugFile() {
	Flush();
	if (_pData != NULL) {
		delete[] _pData;
		_pData = NULL;
	}
	if (_pFile != NULL) {
		delete _pFile;
		_pFile = NULL;
	}
}

void StreamDebugFile::Flush() {
	if (_pFile == NULL)
		return;
	FINEST("Flush for %s", STR(_pFile->GetPath()));
	_pFile->WriteBuffer((uint8_t *) _pData, _structSize * _dataCountCursor);
	_pFile->Flush();
}

void StreamDebugFile::PushRTP(uint8_t *pBuffer, uint32_t length) {
	//FINEST("%"PRIu32"/%"PRIu32, _dataCountCursor, _dataCount);
	((StreamDebugRTP *) (_pData + _dataCountCursor * _structSize))->serial = _streamDebugIdGenerator++;
	((StreamDebugRTP *) (_pData + _dataCountCursor * _structSize))->headerLength = length >= 8 ? 8 : length;
	memcpy(((StreamDebugRTP *) (_pData + _dataCountCursor * _structSize))->rtpHeader, pBuffer,
			((StreamDebugRTP *) (_pData + _dataCountCursor * _structSize))->headerLength);
	((StreamDebugRTP *) (_pData + _dataCountCursor * _structSize))->firstPayloadByte = length >= 13 ? pBuffer[12] : 0;
	_dataCountCursor++;
	if (_dataCount == _dataCountCursor) {
		Flush();
		_dataCountCursor = 0;
	}
}

void StreamDebugFile::PushRTCP(uint8_t *pBuffer, uint32_t length) {
	//FINEST("%"PRIu32"/%"PRIu32, _dataCountCursor, _dataCount);
	((StreamDebugRTCP *) (_pData + _dataCountCursor * _structSize))->serial = _streamDebugIdGenerator++;
	((StreamDebugRTCP *) (_pData + _dataCountCursor * _structSize))->length = length >= 255 ? 255 : length;
	memcpy(((StreamDebugRTCP *) (_pData + _dataCountCursor * _structSize))->payload, pBuffer,
			((StreamDebugRTCP *) (_pData + _dataCountCursor * _structSize))->length);
	_dataCountCursor++;
	if (_dataCount == _dataCountCursor) {
		Flush();
		_dataCountCursor = 0;
	}
}

void StreamDebugFile::PushHLS(bool isAudio, double pts, double dts, uint32_t timestamp) {
	((StreamDebugHLS *) (_pData + _dataCountCursor * _structSize))->serial = _streamDebugIdGenerator++;
	((StreamDebugHLS *) (_pData + _dataCountCursor * _structSize))->isAudio = isAudio;
	((StreamDebugHLS *) (_pData + _dataCountCursor * _structSize))->pts = pts;
	((StreamDebugHLS *) (_pData + _dataCountCursor * _structSize))->dts = dts;
	((StreamDebugHLS *) (_pData + _dataCountCursor * _structSize))->timestamp = timestamp;
	_dataCountCursor++;
	if (_dataCount == _dataCountCursor) {
		Flush();
		_dataCountCursor = 0;
	}
}

#endif /* HAS_STREAM_DEBUG */
