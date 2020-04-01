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
#ifndef _ATOMSTSC_H
#define _ATOMSTSC_H

#include "mediaformats/readers/mp4/versionedatom.h"

typedef struct _STSCEntry {
	uint32_t firstChunk;
	uint32_t samplesPerChunk;
	uint32_t sampleDescriptionIndex;
} STSCEntry;

class AtomSTSC
: public VersionedAtom {
private:
	vector<STSCEntry> _stscEntries;
	vector<uint32_t> _normalizedEntries;
public:
	AtomSTSC(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomSTSC();

	vector<uint32_t> GetEntries(uint32_t totalChunksCount);
protected:
	virtual bool ReadData();
};

#endif	/* _ATOMSTSC_H */


#endif /* HAS_MEDIA_MP4 */
