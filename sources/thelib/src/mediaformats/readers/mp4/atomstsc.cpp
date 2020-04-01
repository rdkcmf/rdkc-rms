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
#include "mediaformats/readers/mp4/atomstsc.h"

AtomSTSC::AtomSTSC(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: VersionedAtom(pDocument, type, size, start) {
}

AtomSTSC::~AtomSTSC() {
}

vector<uint32_t> AtomSTSC::GetEntries(uint32_t totalChunksCount) {
	if ((_normalizedEntries.size() != 0) || (_stscEntries.size() == 0))
		return _normalizedEntries;

	//1. Expand the table
	vector<uint32_t> samplesPerChunk;
	for (uint32_t i = 0; i < _stscEntries.size() - 1; i++) {
		for (uint32_t j = 0; j < _stscEntries[i + 1].firstChunk - _stscEntries[i].firstChunk; j++) {
			ADD_VECTOR_END(samplesPerChunk, _stscEntries[i].samplesPerChunk);
		}
	}

	uint32_t samplesPerChunkCount = (uint32_t) samplesPerChunk.size();
	for (uint32_t i = 0; i < totalChunksCount - samplesPerChunkCount; i++) {
		ADD_VECTOR_END(samplesPerChunk,
				_stscEntries[_stscEntries.size() - 1].samplesPerChunk);
	}

	//2. build the final result based on the expanded table
	samplesPerChunkCount = (uint32_t) samplesPerChunk.size();
	for (uint32_t i = 0; i < samplesPerChunkCount; i++) {
		for (uint32_t j = 0; j < samplesPerChunk[i]; j++) {
			ADD_VECTOR_END(_normalizedEntries, i);
		}
	}

	return _normalizedEntries;
}

bool AtomSTSC::ReadData() {
	uint32_t count;
	if (!ReadUInt32(count)) {
		FATAL("Unable to read count");
		return false;
	}

	if (count == 0)
		return true;

	for (uint32_t i = 0; i < count; i++) {
		STSCEntry entry;

		if (!ReadUInt32(entry.firstChunk)) {
			FATAL("Unable to read first chunk");
			return false;
		}

		if (!ReadUInt32(entry.samplesPerChunk)) {
			FATAL("Unable to read first samples per chunk");
			return false;
		}

		if (!ReadUInt32(entry.sampleDescriptionIndex)) {
			FATAL("Unable to read first sample description index");
			return false;
		}

		ADD_VECTOR_END(_stscEntries, entry);
	}

	return true;
}


#endif /* HAS_MEDIA_MP4 */
