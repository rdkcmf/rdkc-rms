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

#include "mediaformats/writers/fmp4/atom_abst.h"
#include "mediaformats/writers/fmp4/atom_asrt.h"
#include "mediaformats/writers/fmp4/atom_free.h"

Atom_abst::Atom_abst() : Atom(0x61627374, true) {
	_bootstrapInfoVersion = 1;
	_profileRangeAccess = false; //2 bit
	_live = true; //1 bit
	_update = false; //1 bit
	//_reserved = 0;//4 bit
	_timeScale = 1000;
	_currentMediaTime = 0;
	_smpteTimeCodeOffset = 0;
	_movieIdentifier = "";
	_drmData = "";
	_metadata = "";
	_pFree = NULL;
}

Atom_abst::~Atom_abst() {

	// Clear out the contents of the vectors

	FOR_VECTOR(_fragmentRunTableEntries, i) {
		delete _fragmentRunTableEntries[i];
	}

	FOR_VECTOR(_segmentRunTableEntries, i) {
		delete _segmentRunTableEntries[i];
	}

	_serverEntryTable.clear();
	_qualityEntryTable.clear();
	_segmentRunTableEntries.clear();
	_fragmentRunTableEntries.clear();

	if (NULL != _pFree) {
		delete _pFree;
		_pFree = NULL;
	}
}

void Atom_abst::SetInfoVersion(uint32_t infoVersion) {
	_bootstrapInfoVersion = infoVersion;
}

void Atom_abst::SetProfileFlag(bool isRange) {
	_profileRangeAccess = isRange;
}

void Atom_abst::SetLiveFlag(bool isLive) {
	_live = isLive;
}

void Atom_abst::SetUpdateFlag(bool isUpdate) {
	_update = isUpdate;
}

void Atom_abst::SetTimeScale(uint32_t timeScale) {
	_timeScale = timeScale;
}

void Atom_abst::SetMediaTimeStamp(uint64_t timeStamp) {
	_currentMediaTime = timeStamp;
}

void Atom_abst::AddSegmentRunTableEntry() {

	Atom_asrt * asrt = new Atom_asrt;

	// Add a default entry
	asrt->AddSegmentRunEntry(1, 0);

	ADD_VECTOR_END(_segmentRunTableEntries, asrt);
}

void Atom_abst::AddFragmentRunTableEntry() {

	Atom_afrt * afrt = new Atom_afrt;

	// Add a default entry
	afrt->AddFragmentRunEntry(0, 0, 1, 0);

	ADD_VECTOR_END(_fragmentRunTableEntries, afrt);
}

void Atom_abst::AddSegmentRunEntry(uint32_t asrtIndex, uint32_t firstSegment,
		uint32_t fragmentsPerSegment) {

	_segmentRunTableEntries[asrtIndex]->AddSegmentRunEntry(firstSegment,
			fragmentsPerSegment);
}

void Atom_abst::AddFragmentRunEntry(uint32_t afrtIndex, uint32_t firstFragment,
		uint64_t firstFragmentTimestamp, uint32_t fragmentDuration,
		uint8_t discontinuityIndicator) {

	_fragmentRunTableEntries[afrtIndex]->AddFragmentRunEntry(firstFragment,
			firstFragmentTimestamp, fragmentDuration, discontinuityIndicator);
}

bool Atom_abst::ModifyFragmentRunEntry(uint32_t afrtIndex, uint32_t entryIndex,
		uint32_t firstFragment, uint64_t firstFragmentTimestamp,
		uint32_t fragmentDuration, uint8_t discontinuityIndicator) {

	if (afrtIndex >= _fragmentRunTableEntries.size()) {
		FATAL("Index out of range!");
		return false;
	} else if (_fragmentRunTableEntries[afrtIndex]->GetEntryCount() == entryIndex) {
		// Add a new entry
		AddFragmentRunEntry(afrtIndex, firstFragment, firstFragmentTimestamp,
				fragmentDuration, discontinuityIndicator);
	} else {
		// Modify the existing one
		_fragmentRunTableEntries[afrtIndex]->ModifyFragmentRunEntry(entryIndex,
			firstFragment, firstFragmentTimestamp, fragmentDuration, discontinuityIndicator);
	}

	return true;
}

bool Atom_abst::ModifySegmentRunEntry(uint32_t asrtIndex, uint32_t entryIndex,
		uint32_t firstSegment, uint32_t fragmentsPerSegment) {

	if (asrtIndex >= _segmentRunTableEntries.size()) {
		FATAL("Index out of range!");
		return false;
	}

	_segmentRunTableEntries[asrtIndex]->ModifySegmentRunEntry(entryIndex,
			firstSegment, fragmentsPerSegment);

	return true;
}

bool Atom_abst::UpdateFragments(uint32_t fragmentNumber, double starTS, double endTS) {

	// Update the segment run entry to contain the actual number of fragments
	if (_isRolling) {
		uint32_t fragments = fragmentNumber;

		// Check if the fragment number is greater than the playlist length
		// OR that the fragment queue size has the playlist length
		if ((fragmentNumber > _playlistLength) 
				|| (_fragmentsRETable.size() >= _playlistLength)) {

			fragments = _playlistLength;

			/*
			 * Since this is a rolling type, modify the fragment run entry to point
			 * to _playlistLength - 1 as the first fragment created.
			 * 
			 * Adjust when the 32bit boundary was reached
			 */
			int64_t firstFragment = fragmentNumber - (_playlistLength - 1);
			if (firstFragment < 0) firstFragment += 65536;
			else if (firstFragment == 0) firstFragment = 1;

			// Modify fragment entry to have the correct 'first' fragment
			Atom_afrt::FragmentRunEntry firstFragmentEntry;
			//Added to remove compilation errors in windows
			memset((void *) &firstFragmentEntry, 0, sizeof (Atom_afrt::FragmentRunEntry));
			firstFragmentEntry._discontinuityIndicator = 1; // dummy marker (error!)

			while (!_fragmentsRETable.empty()) {
				// Check to see if this previously recorded fragment is what we want
				if ((_fragmentsRETable.front())._firstFragment == firstFragment) {
					firstFragmentEntry = _fragmentsRETable.front();

					break;
				}

				// Remove from queue
				_fragmentsRETable.pop();
			}

			if (1 == firstFragmentEntry._discontinuityIndicator) {
				FATAL("HDS rolling queue has been corrupted!");
				return false;
			}

			// A match is found, modify accordingly
			ModifyFragmentRunEntry(0, 0, firstFragmentEntry._firstFragment,
					firstFragmentEntry._firstFragmentTimestamp, firstFragmentEntry._fragmentDuration, 0);
		}

		// Store the info of this fragment to a queue
		Atom_afrt::FragmentRunEntry newEntry;
		newEntry._firstFragment = fragmentNumber;
		newEntry._firstFragmentTimestamp = (uint64_t) starTS;
		newEntry._fragmentDuration = (uint32_t) (endTS - starTS);
		newEntry._discontinuityIndicator = 0;
		_fragmentsRETable.push(newEntry);

		// Update the segment run entry
		ModifySegmentRunEntry(0, 0, 1, fragments);
	} else {
		// Update the segment run entry
		ModifySegmentRunEntry(0, 0, 1, fragmentNumber);
	}

	return true;
}

bool Atom_abst::WriteFields() {
	/*
	 * Special case for ASRT, AFRT and ABST: data are in buffer since they are
	 * common to all fragments and they contain small amount of information.
	 */

	// Bootstrap information
	_buffer.ReadFromRepeat(0, 4);
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4,
			_bootstrapInfoVersion);

	// Bitflags
	uint8_t _bitFlags = 0;
	if (_profileRangeAccess) _bitFlags |= 0x40;
	if (_live) _bitFlags |= 0x20;
	if (_update) _bitFlags |= 0x10;
	_buffer.ReadFromByte(_bitFlags);

	_buffer.ReadFromRepeat(0, 20);

	// timeScale
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 20, _timeScale);

	// currentMediaTime
	EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 16,
			_currentMediaTime);

	// smpteTimeCodeOffset
	EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
			_smpteTimeCodeOffset);

	// movieIdentifier
	WriteString(_movieIdentifier);

	// serverEntryCount
	_buffer.ReadFromByte((uint8_t) _serverEntryTable.size());

	FOR_VECTOR(_serverEntryTable, i) {
		// For each server entry, transfer to buffer
		WriteString(_serverEntryTable[i]._serverBaseUrl);
	}

	// qualityEntryCount;
	_buffer.ReadFromByte((uint8_t) _qualityEntryTable.size());

	FOR_VECTOR(_qualityEntryTable, j) {
		// For each quality entry, transfer to buffer
		WriteString(_qualityEntryTable[j]._qualitySegmentUriModifier);
	}

	// _drmData = "";
	WriteString(_drmData);

	// _metadata = ""
	WriteString(_metadata);

	// ASRT
	_buffer.ReadFromByte((uint8_t) _segmentRunTableEntries.size());

	FOR_VECTOR(_segmentRunTableEntries, i) {
		// Transfer the content of this atom to the buffer
		_segmentRunTableEntries[i]->ReadContent(_buffer);
	}

	// AFRT
	_buffer.ReadFromByte((uint8_t) _fragmentRunTableEntries.size());

	FOR_VECTOR(_fragmentRunTableEntries, j) {
		// Transfer the content of this atom to the buffer
		_fragmentRunTableEntries[j]->ReadContent(_buffer);
	}

	/*
	 * Check if we need to add some free space
	 */
	if (_live) {
		/*
		 * For 1st fragment:
		 * FREE -> 2 * fragment run entry (4 (firstFragment) + 8 (firstFragmentTimestamp) +
		 * 4 (fragmentDuration)) + 1 (discontinuity indicator) = 33 bytes
		 * 
		 * Remaining fragments:
		 * FREE -> 1 * fragment run entry (4 (firstFragment) + 8 (firstFragmentTimestamp) +
		 * 4 (fragmentDuration)) + 1 (discontinuity indicator) = 17 bytes
		 */
		if (NULL == _pFree) {
			_pFree = new Atom_free;
		}
		
		//TODO: modify if we decided to use multiple AFRTs
		if (_fragmentRunTableEntries[0]->GetEntryCount() > 1) {
			// If we have more than 1 afrt entries, allot just enough space
			_pFree->SetFreeSpace(17); // Set enough space for 17 bytes
		} else {
			// Single afrt, which means this is the first fragment
			_pFree->SetFreeSpace(33); // Set enough space for 33 bytes, see computation above
		}

		_pFree->ReadContent(_buffer);
	}

	return true;
}

uint64_t Atom_abst::GetFieldsSize() {

	/*
	 * Compute the size of the this atom.
	 */
	uint64_t size = 0;

	// Bootstrap information
	size += 4;

	// Bitflags
	size += 1;

	// timeScale
	size += 4;

	// currentMediaTime
	size += 8;

	// smpteTimeCodeOffset
	size += 8;

	// movieIdentifier
	size += GetStringSize(_movieIdentifier);

	// serverEntryCount
	size += 1;

	FOR_VECTOR(_serverEntryTable, i) {
		// For each server entry, compute the size
		size += GetStringSize(_serverEntryTable[i]._serverBaseUrl);
	}

	// qualityEntryCount;
	size += 1;

	FOR_VECTOR(_qualityEntryTable, j) {
		// For each quality entry, compute the size
		size += GetStringSize(_qualityEntryTable[j]._qualitySegmentUriModifier);
	}

	// drmData
	size += GetStringSize(_drmData);

	// _metadata = ""
	size += GetStringSize(_metadata);

	// ASRT entries size
	size += 1;

	FOR_VECTOR(_segmentRunTableEntries, i) {
		// Transfer the content of this atom to the buffer
		size += _segmentRunTableEntries[i]->GetSize();
	}

	// AFRT entries size
	size += 1;

	FOR_VECTOR(_fragmentRunTableEntries, j) {
		// Transfer the content of this atom to the buffer
		size += _fragmentRunTableEntries[j]->GetSize();
	}

	/*
	 * Check if we need to add some free space
	 */
	if (_live) {
		/*
		 * For 1st fragment:
		 * FREE -> 2 * fragment run entry (4 (firstFragment) + 8 (firstFragmentTimestamp) +
		 * 4 (fragmentDuration)) + 1 (discontinuity indicator) = 33 bytes
		 * 
		 * Remaining fragments:
		 * FREE -> 1 * fragment run entry (4 (firstFragment) + 8 (firstFragmentTimestamp) +
		 * 4 (fragmentDuration)) + 1 (discontinuity indicator) = 17 bytes
		 */
		//TODO: modify if we decided to use multiple AFRTs
		if (_fragmentRunTableEntries[0]->GetEntryCount() > 1) {
			size += 17;
		} else {
			size += 33;
		}
	}

	return size;
}

bool Atom_abst::WriteBootStrap(string filePath) {

	File bsFile;

	// Create an external bootstrapinfo file
	if (!bsFile.Initialize(filePath, FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to initialize bootstrap: %s!", STR(filePath));
		return false;
	};

	// Write the contents of the ABST file to the external bootstrap file
	SetFile(&bsFile);
	SetPosition(0);
	WriteBufferToFile();

	// Close the bootstrap file
	bsFile.Close();

	return true;
}

void Atom_abst::Initialize(bool writeDirect, File* pFile, uint64_t position) {

	// Initialize the common members
	Atom::Initialize(writeDirect, pFile, position);

	SetLiveFlag(true);
	SetProfileFlag(false); // named access
	SetUpdateFlag(false);
	SetInfoVersion(0x01);

	// ASRT
	AddSegmentRunTableEntry();

	// AFRT
	AddFragmentRunTableEntry();
}

void Atom_abst::SetRolling(bool isRolling, uint32_t playlistLength) {

	_isRolling = isRolling;
	_playlistLength = playlistLength;
}

void Atom_abst::SetPath(string path) {

	_fragmentPath = path;
}
