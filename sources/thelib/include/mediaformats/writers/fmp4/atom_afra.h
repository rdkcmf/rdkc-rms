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

#ifndef _ATOM_AFRA_H
#define	_ATOM_AFRA_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_afra : public Atom {

	/*
	 * AFRAENTRY structure
	 * See page 38 of Adobe Flash Video File Format Specification v10.1
	 */
	typedef struct _AfraEntry {
		uint64_t _time;
		uint64_t _offset; // if longOffset == 0, offset = 32 bit else 64
	} AfraEntry;

	/*
	 * GLOBALAFRAENTRY structure
	 * See page 38 of Adobe Flash Video File Format Specification v10.1
	 */
	typedef struct _GlobalAfraEntry {
		uint64_t _time;
		uint32_t _segment; // if longIds == 0, segment = 16 bit else 32
		uint32_t _fragment; // if longIds == 0, fragment = 16 bit else 32
		uint64_t _afraOffset; // if longOffset == 0, afraoffset = 32 bit else 64
		uint64_t _offsetFromAfra; // if longOffset == 0, offsetFromAfra = 32 bit else 64
	} GlobalAfraEntry;

public:
	/*
	 * AFRA atom/box (see 2.11.1 of Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_afra();
	virtual ~Atom_afra();

	/**
	 * Sets the bit flags of this atom
	 *
	 * @param longID
	 * @param longOffset
	 * @param globalEntries
	 */
	void SetBitFlags(bool longID, bool longOffset, bool globalEntries);

	/**
	 * Sets the timescale field of this atom/box
	 *
	 * @param timeScale Value to set
	 */
	void SetTimeScale(uint32_t timeScale);

	/**
	 * Adds an AFRA entry to this atom/box
	 *
	 * @param time Timestamp of this sample (normally the firstTimeStamp value)
	 * @param offset Offset from the very start of this box to the first sample
	 * of the MDAT box
	 */
	void AddAfraEntry(uint64_t time, uint64_t offset);

	bool ModifyAfraRunEntry(uint32_t index, uint64_t time, uint64_t offset);

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	bool _longIds;
	bool _longOffset;
	bool _globalEntries;

	uint32_t _timeScale;

	uint32_t _afraEntrySize;
	uint32_t _globalAfraEntrySize;

	vector<AfraEntry > _localAccessEntries;
	vector<GlobalAfraEntry > _globalAccessEntries;
};
#endif	/* _ATOM_AFRA_H */
