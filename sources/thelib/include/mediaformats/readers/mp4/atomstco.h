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
#ifndef _ATOMSTCO_H
#define _ATOMSTCO_H

#include "mediaformats/readers/mp4/versionedatom.h"

class AtomSTCO
: public VersionedAtom {
private:
	vector<uint64_t> _entries;
public:
	AtomSTCO(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomSTCO();

	vector<uint64_t> GetEntries();
protected:
	virtual bool ReadData();
};

#endif	/* _ATOMSTCO_H */


#endif /* HAS_MEDIA_MP4 */
