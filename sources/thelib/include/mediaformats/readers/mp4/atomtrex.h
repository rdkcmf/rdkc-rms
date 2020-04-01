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
#ifndef _ATOMTREX_H
#define	_ATOMTREX_H

#include "mediaformats/readers/mp4/versionedatom.h"

class AtomTREX
: public VersionedAtom {
private:
	uint32_t _trackID;
	uint32_t _defaultSampleDescriptionIndex;
	uint32_t _defaultSampleDuration;
	uint32_t _defaultSampleSize;
	uint32_t _defaultSampleFlags;
public:
	AtomTREX(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomTREX();
	uint32_t GetTrackID();
	uint32_t GetDefaultSampleDescriptionIndex();
	uint32_t GetDefaultSampleDuration();
	uint32_t GetDefaultSampleSize();
	uint32_t GetDefaultSampleFlags();
protected:
	virtual bool ReadData();
};


#endif	/* _ATOMTREX_H */
#endif /* HAS_MEDIA_MP4 */
