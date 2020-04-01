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
#ifndef _ATOMABST_H
#define _ATOMABST_H

#include "mediaformats/readers/mp4/versionedatom.h"

class AtomABST
: public VersionedAtom {
private:
	uint32_t _bootstrapInfoVersion;
	uint8_t _profile;
	bool _live;
	bool _update;
	uint32_t _timeScale;
	uint64_t _currentMediaTime;
	uint64_t _smpteTimeCodeOffset;
	string _movieIdentifier;
	uint8_t _serverEntryCount;
	vector<string> _serverEntryTable;
	uint8_t _qualityEntryCount;
	vector<string> _qualityEntryTable;
	string _drmData;
	string _metaData;
	uint8_t _segmentRunTableCount;
	vector<BaseAtom*> _segmentRunTableEntries;
	uint8_t _fragmentRunTableCount;
	vector<BaseAtom*> _fragmentRunTableEntries;
public:
	AtomABST(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start);
	virtual ~AtomABST();
	virtual string Hierarchy(uint32_t indent);
protected:
	virtual bool ReadData();
};

#endif	/* _ATOMABST_H */
#endif /* HAS_MEDIA_MP4 */
