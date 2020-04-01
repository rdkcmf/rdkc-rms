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
#ifndef _ATOMESDS_H
#define _ATOMESDS_H

#include "mediaformats/readers/mp4/versionedatom.h"

class AtomESDS
: public VersionedAtom {
private:
	uint16_t _MP4ESDescrTag_ID;
	uint8_t _MP4ESDescrTag_Priority;
	uint8_t _MP4DecConfigDescrTag_ObjectTypeID;
	uint8_t _MP4DecConfigDescrTag_StreamType;
	uint32_t _MP4DecConfigDescrTag_BufferSizeDB;
	uint32_t _MP4DecConfigDescrTag_MaxBitRate;
	uint32_t _MP4DecConfigDescrTag_AvgBitRate;
	uint64_t _extraDataStart;
	uint64_t _extraDataLength;
	bool _isMP3;
#ifdef DEBUG_ESDS_ATOM
	uint8_t _objectType;
	uint8_t _sampleRate;
	uint8_t _channels;
	uint8_t _extObjectType;
	uint8_t _sbr;
	uint8_t _extSampleRate;
#endif /* DEBUG_ESDS_ATOM */
public:
	AtomESDS(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomESDS();

	bool IsMP3();
	uint64_t GetExtraDataStart();
	uint64_t GetExtraDataLength();
protected:
	virtual bool ReadData();
private:
	bool ReadTagLength(uint32_t &length);
	bool ReadTagAndLength(uint8_t &tagType, uint32_t &length);
	bool ReadESDescrTag();
	bool ReadDecoderConfigDescriptorTag();
};

#endif	/* _ATOMESDS_H */


#endif /* HAS_MEDIA_MP4 */
