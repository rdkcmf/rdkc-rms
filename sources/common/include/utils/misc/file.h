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



#ifndef _FILE_H
#define	_FILE_H

#include "platform/platform.h"

enum FILE_OPEN_MODE {
	FILE_OPEN_MODE_READ,
	FILE_OPEN_MODE_WRITE,
	FILE_OPEN_MODE_TRUNCATE,
	FILE_OPEN_MODE_APPEND
};

class DLLEXP File {
private:
	FILE *_pFile;
	uint64_t _size;
	string _path;
	bool _truncate;
	bool _append;
	bool _suppressLogErrorsOnInit;
public:
	File();
	virtual ~File();

	void SuppressLogErrorsOnInit();

	//Init
	bool Initialize(string path);
	bool Initialize(string path, FILE_OPEN_MODE mode);
	void Close();

	//info
	uint64_t Size();
	uint64_t Cursor();
	bool IsEOF();
	string GetPath();
	bool IsOpen();

	//seeking
	bool SeekBegin();
	bool SeekEnd();
	bool SeekAhead(int64_t count);
	bool SeekBehind(int64_t count);
	bool SeekTo(uint64_t position);

	//read data
	bool ReadI8(int8_t *pValue);
	bool ReadI16(int16_t *pValue, bool networkOrder = true);
	bool ReadI24(int32_t *pValue, bool networkOrder = true);
	bool ReadI32(int32_t *pValue, bool networkOrder = true);
	bool ReadSI32(int32_t *pValue);
	bool ReadI64(int64_t *pValue, bool networkOrder = true);
	bool ReadUI8(uint8_t *pValue);
	bool ReadUI16(uint16_t *pValue, bool networkOrder = true);
	bool ReadUI24(uint32_t *pValue, bool networkOrder = true);
	bool ReadUI32(uint32_t *pValue, bool networkOrder = true);
	bool ReadSUI32(uint32_t *pValue);
	bool ReadUI64(uint64_t *pValue, bool networkOrder = true);
	bool ReadBuffer(uint8_t *pBuffer, uint64_t count);
	bool ReadLine(uint8_t *pBuffer, uint64_t &maxSize);
	bool ReadAll(string &str);

	//peek data
	bool PeekI8(int8_t *pValue);
	bool PeekI16(int16_t *pValue, bool networkOrder = true);
	bool PeekI24(int32_t *pValue, bool networkOrder = true);
	bool PeekI32(int32_t *pValue, bool networkOrder = true);
	bool PeekSI32(int32_t *pValue);
	bool PeekI64(int64_t *pValue, bool networkOrder = true);
	bool PeekUI8(uint8_t *pValue);
	bool PeekUI16(uint16_t *pValue, bool networkOrder = true);
	bool PeekUI24(uint32_t *pValue, bool networkOrder = true);
	bool PeekUI32(uint32_t *pValue, bool networkOrder = true);
	bool PeekSUI32(uint32_t *pValue);
	bool PeekUI64(uint64_t *pValue, bool networkOrder = true);
	bool PeekBuffer(uint8_t *pBuffer, uint64_t count);

	//write data
	bool WriteI8(int8_t value);
	bool WriteI16(int16_t value, bool networkOrder = true);
	bool WriteI24(int32_t value, bool networkOrder = true);
	bool WriteI32(int32_t value, bool networkOrder = true);
	bool WriteSI32(int32_t value);
	bool WriteI64(int64_t value, bool networkOrder = true);
	bool WriteUI8(uint8_t value);
	bool WriteUI16(uint16_t value, bool networkOrder = true);
	bool WriteUI24(uint32_t value, bool networkOrder = true);
	bool WriteUI32(uint32_t value, bool networkOrder = true);
	bool WriteSUI32(uint32_t value);
	bool WriteUI64(uint64_t value, bool networkOrder = true);
	bool WriteString(string &value);
	bool WriteBuffer(const uint8_t *pBuffer, uint64_t count);
	bool Flush();
};


#endif	/* _FILE_H */


