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


#ifndef _ATOM_TFHD_H
#define	_ATOM_TFHD_H

#include "mediaformats/writers/fmp4/atom.h"

class Atom_tfhd : public Atom {
	// Sample Flags structure used by TRUN, TREX and TFHD atoms/boxes

	typedef struct _SampleFlags {
		//reserved
		uint8_t dependsOn;
		uint8_t dependedOn;
		uint8_t hasRedundancy;
		uint8_t paddingValue;
		bool nonKeySyncSample;
		//uint16_t degradationPriority; //reserved
	} SampleFlags;

public:
	/*
	 * TFHD atom/box (see 2.12.2.1 of Adobe Flash Video File Format
	 * Specification v10.1)
	 */
	Atom_tfhd();
	virtual ~Atom_tfhd();

	// Setter and getter for the track ID field
	void SetTrackID(uint32_t trackID);
	uint32_t GetTrackID();

	// Setters for the rest of the TFHD fields
	void SetBaseDataOffset(uint64_t baseDataOffset);
	void SetSampleDescriptionIndex(uint32_t sampleDescriptionIndex);
	void SetDefaultSampleSize(uint32_t defaultSampleSize);
    void SetDefaultSampleDuration(uint32_t defaultSampleDuration);
	void SetDefaultSampleFlags(uint32_t sampleFlagBits);
	// Overload for the above function but this time using a struct
	void SetDefaultSampleFlags(SampleFlags &sampleFlags);

private:
	bool WriteFields();
	uint64_t GetFieldsSize();

	uint32_t _trackID;
	uint64_t _baseDataOffset;
	uint32_t _sampleDescriptionIndex;
	uint32_t _defaultSampleDuration;
	uint32_t _defaultSampleSize;
	uint32_t _defaultSampleFlagBits;
	//SampleFlags _defaultSampleFlags; //not used, reported by compiler
};
#endif	/* _ATOM_TFHD_H */
