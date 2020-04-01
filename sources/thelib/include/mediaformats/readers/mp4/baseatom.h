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


#ifdef HAS_MEDIA_MP4
#ifndef _BASEATOM_H
#define	_BASEATOM_H

#include "common.h"

string U32TOS(uint32_t type);

class MP4Document;

class BaseAtom {
protected:
	uint64_t _start;
	uint64_t _size;

	uint32_t _type;

	MP4Document *_pDoc;
	BaseAtom *_pParent;
public:
	BaseAtom(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~BaseAtom();

	uint64_t GetStart();
	uint64_t GetSize();
	string GetTypeString();
	uint32_t GetTypeNumeric();

	MP4Document * GetDoc();

	virtual bool Read() = 0;

	virtual operator string();
	virtual bool IsIgnored();

	virtual BaseAtom * GetPath(vector<uint32_t> path);

	virtual string Hierarchy(uint32_t indent) = 0;

	BaseAtom *GetParentAtom();
	void SetParentAtom(BaseAtom *pParent);

protected:
	bool SkipRead(bool issueWarn = true);
	uint64_t CurrentPosition();
	bool CheckBounds(uint64_t size);
	bool ReadArray(uint8_t *pBuffer, uint64_t length);
	bool ReadUInt8(uint8_t &val);
	bool ReadUInt16(uint16_t &val, bool networkOrder = true);
	bool ReadInt16(int16_t &val, bool networkOrder = true);
	bool ReadUInt24(uint32_t &val, bool networkOrder = true);
	bool ReadUInt32(uint32_t &val, bool networkOrder = true);
	bool ReadInt32(int32_t &val, bool networkOrder = true);
	bool ReadUInt64(uint64_t &val, bool networkOrder = true);
	bool ReadInt64(int64_t &val, bool networkOrder = true);
	bool SkipBytes(uint64_t count);
	bool ReadString(string &val, uint64_t size);
	bool ReadNullTerminatedString(string &val);
};

#endif	/* _BASEATOM_H */
#endif /* HAS_MEDIA_MP4 */
