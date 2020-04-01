/*
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
 */


#ifndef _ATOM_SIDX_H
#define	_ATOM_SIDX_H

#include "mediaformats/writers/fmp4/atom.h"

struct SidxReference {
	uint32_t referenceTypeSize;
	uint32_t subsegmentDuration;
	uint32_t sapFlags;
};

class Atom_sidx : public Atom {
public:
	Atom_sidx();
	virtual ~Atom_sidx();

    bool UpdateTimeScale();
    bool UpdateEarliestPresentationTime();
	bool UpdateFirstOffset();
	bool UpdateReference();
	bool Update();

	inline void SetTimeScale(uint32_t value) {
		_timeScale = value;
	}

    inline void SetEarliestPresentationTime(uint64_t value) {
		_earliestPresentationTime = value;
    }

	inline void SetFirstOffset(uint64_t value) {
		_firstOffset = value;
	}

	inline void SetReferenceTypeSize(size_t index, uint32_t value) {
		if (_reference.size() > 0) {
			_reference[index].referenceTypeSize = value;
		}
	}

	inline void SetSubSegmentDuration(size_t index, uint32_t value) {
		if (_reference.size() > 0) {
			_reference[index].subsegmentDuration = value;
		}
	}

	inline void SetSAPFlags(size_t index, uint32_t value) {
		if (_reference.size() > 0) {
			_reference[index].sapFlags = value;
		}
	}

	void AddReference(uint32_t referenceTypeSize, uint32_t subsegmentDuration, uint32_t sapFlags);
private:
	bool WriteFields();
	uint64_t GetFieldsSize();

private:
	uint64_t _referenceId;
	uint32_t _timeScale;
	uint64_t _earliestPresentationTime;
	uint64_t _firstOffset;
	uint16_t _reserved;
	vector<SidxReference> _reference;
};

#endif	/* _ATOM_SIDX_H */
