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
#ifndef _ATOMTKHD_H
#define _ATOMTKHD_H

#include "mediaformats/readers/mp4/versionedatom.h"

class AtomTKHD
: public VersionedAtom {
private:
	uint64_t _creationTime;
	uint64_t _modificationTime;
	uint32_t _trackId;
	uint8_t _reserved1[4];
	uint64_t _duration;
	uint8_t _reserved2[8];
	uint16_t _layer;
	uint16_t _alternateGroup;
	uint16_t _volume;
	uint8_t _reserved3[2];
	uint8_t _matrixStructure[36];
	uint32_t _trackWidth;
	uint32_t _trackHeight;
public:
	AtomTKHD(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomTKHD();

	uint32_t GetTrackId();
	uint32_t GetWidth();
	uint32_t GetHeight();
protected:
	virtual bool ReadData();
};

#endif	/* _ATOMTKHD_H */


#endif /* HAS_MEDIA_MP4 */
