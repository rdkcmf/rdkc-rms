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
#ifndef _ATOMTRUN_H
#define	_ATOMTRUN_H

#include "mediaformats/readers/mp4/versionedatom.h"

typedef struct _TRUNSample {
	uint32_t duration;
	uint32_t size;
	uint32_t flags;
	uint32_t compositionTimeOffset;
	int64_t absoluteOffset;

	_TRUNSample() {
		duration = size = flags = compositionTimeOffset = 0;
		absoluteOffset = 0;
	}

	operator string() {
		return format("duration: %u; size: %u; flags: %u; CTO: %u",
				duration, size, flags, compositionTimeOffset);
	}
} TRUNSample;

class AtomTRUN
: public VersionedAtom {
private:
	uint32_t _sampleCount;
	int32_t _dataOffset;
	uint32_t _firstSampleFlags;
	vector<TRUNSample *> _samples;
public:
	AtomTRUN(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomTRUN();
	bool HasDataOffset();
	bool HasFirstSampleFlags();
	bool HasSampleDuration();
	bool HasSampleSize();
	bool HasSampleFlags();
	bool HasSampleCompositionTimeOffsets();
	uint32_t GetDataOffset();
	vector<TRUNSample *> &GetSamples();
protected:
	virtual bool ReadData();
};


#endif	/* _ATOMTRUN_H */
#endif	/* HAS_MEDIA_MP4 */
