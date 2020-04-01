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
#ifndef _ATOMMVHD_H
#define _ATOMMVHD_H

#include "mediaformats/readers/mp4/versionedatom.h"

class AtomMVHD
: public VersionedAtom {
private:
	uint64_t _creationTime;
	uint64_t _modificationTime;
	uint32_t _timeScale;
	uint64_t _duration;
	uint32_t _preferredRate;
	uint16_t _preferredVolume;
	uint8_t _reserved[10];
	uint32_t _matrixStructure[9];
	uint32_t _previewTime;
	uint32_t _previewDuration;
	uint32_t _posterTime;
	uint32_t _selectionTime;
	uint32_t _selectionDuration;
	uint32_t _currentTime;
	uint32_t _nextTrakId;
public:
	AtomMVHD(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomMVHD();

protected:
	virtual bool ReadData();
};

#endif	/* _ATOMMVHD_H */


#endif /* HAS_MEDIA_MP4 */
