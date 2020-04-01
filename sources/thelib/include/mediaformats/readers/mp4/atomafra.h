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
#ifndef _ATOMAFRA_H
#define _ATOMAFRA_H

#include "mediaformats/readers/mp4/versionedatom.h"

struct AFRAENTRY {
	uint64_t time;
	uint64_t offset;
};

struct GLOBALAFRAENTRY {
	uint64_t time;
	uint32_t segment;
	uint32_t fragment;
	uint64_t afraOffset;
	uint64_t offsetFromAfra;
};

class AtomAFRA
: public VersionedAtom {
private:
	uint8_t _flags;
	uint32_t _timeScale;
	uint32_t _entryCount;
	uint32_t _globalEntryCount;
	vector<AFRAENTRY> _localAccessEntries;
	vector<GLOBALAFRAENTRY> _globalAccessEntries;
public:
	AtomAFRA(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomAFRA();
protected:
	virtual bool ReadData();
};

#endif	/* _ATOMAFRA_H */
#endif /* HAS_MEDIA_MP4 */
