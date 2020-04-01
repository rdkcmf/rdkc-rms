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

#ifndef _ATOM_ABST_H
#define	_ATOM_ABST_H

#include "mediaformats/writers/fmp4/atom.h"
#include "mediaformats/writers/fmp4/atom_afrt.h"

class Atom_asrt;
class Atom_free;

class Atom_abst : public Atom {

	/*
	 * SERVERENTRY structure
	 * See page 41 of Adobe Flash Video File Format Specification v10.1
	 */
	typedef struct _ServerEntry {
		string _serverBaseUrl;
	} ServerEntry;

	/*
	 * QUALITYENTRY structure
	 * See page 41 of Adobe Flash Video File Format Specification v10.1
	 */
	typedef struct _QualityEntry {
		string _qualitySegmentUriModifier;
	} QualityEntry;

public:
	/*
	 * ABST atom/box (see 2.11.2 of Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_abst();
	virtual ~Atom_abst();

	// Setters for this atom/box
	void SetInfoVersion(uint32_t infoVersion);
	void SetProfileFlag(bool isRange);
	void SetLiveFlag(bool isLive);
	void SetUpdateFlag(bool isUpdate);
	void SetTimeScale(uint32_t timeScale);
	void SetMediaTimeStamp(uint64_t timeStamp);
	// TODO add here other setters if needed

	// Adds the atoms/boxes inside this ABST atom/box
	void AddFragmentRunTableEntry(); // for dummy data
	void AddSegmentRunTableEntry(); // for dummy data

	void AddSegmentRunEntry(uint32_t asrtIndex, uint32_t firstSegment,
			uint32_t fragmentsPerSegment);

	void AddFragmentRunEntry(uint32_t afrtIndex, uint32_t firstFragment,
			uint64_t firstFragmentTimestamp, uint32_t fragmentDuration,
			uint8_t discontinuityIndicator);

	bool ModifyFragmentRunEntry(uint32_t afrtIndex, uint32_t entryIndex,
			uint32_t firstFragment, uint64_t firstFragmentTimestamp,
			uint32_t fragmentDuration, uint8_t discontinuityIndicator);

	bool ModifySegmentRunEntry(uint32_t asrtIndex, uint32_t entryIndex,
			uint32_t firstSegment, uint32_t fragmentsPerSegment);

	bool UpdateFragments(uint32_t fragmentNumber, double starTS, double endTS);

	bool WriteBootStrap(string filePath);

	void Initialize(bool writeDirect = false, File* pFile = NULL,
			uint64_t position = 0);

	void SetRolling(bool isRolling, uint32_t playlistLength);

	void SetPath(string path);

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	/*
	 * See 2.11.2 of Adobe Flash Video File Format Specification v10.1 for the
	 * following ABST fields
	 */

	uint32_t _bootstrapInfoVersion;
	bool _profileRangeAccess;
	bool _live;
	bool _update;

	uint32_t _timeScale;
	uint64_t _currentMediaTime;
	uint64_t _smpteTimeCodeOffset;
	string _movieIdentifier;

	vector<ServerEntry > _serverEntryTable;
	vector<QualityEntry > _qualityEntryTable;

	string _drmData;
	string _metadata;

	// Container for the ASRT atoms/boxes
	vector<Atom_asrt* > _segmentRunTableEntries;

	// Container for the AFRT atoms/boxes
	vector<Atom_afrt* > _fragmentRunTableEntries;

	Atom_free* _pFree;

	string _fragmentPath;

	// Indicates if HDS is of rolling type (fragments generated are limited)
	bool _isRolling;
	uint32_t _playlistLength;

	// Stores the fragment run entry of EACH fragment, used for rolling types
	std::queue<Atom_afrt::FragmentRunEntry > _fragmentsRETable;
};
#endif	/* _ATOM_ABST_H */
