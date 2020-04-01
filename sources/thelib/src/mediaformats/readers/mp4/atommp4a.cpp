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
#include "mediaformats/readers/mp4/atommp4a.h"
#include "mediaformats/readers/mp4/mp4document.h"

AtomMP4A::AtomMP4A(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BoxAtom(pDocument, type, size, start) {
	_pESDS = NULL;
	_pWAVE = NULL;
	_pCHAN = NULL;
}

AtomMP4A::~AtomMP4A() {
}

bool AtomMP4A::Read() {
	//aligned(8) abstract class SampleEntry (unsigned int(32) format) extends Box(format){
	//	const unsigned int(8)[6] reserved = 0;
	//	unsigned int(16) data_reference_index;
	//}
	//class AudioSampleEntry(codingname) extends SampleEntry (codingname){
	//	const unsigned int(16) version;
	//	const unsigned int(16) revision_level;
	//	const unsigned int(32) vendor;
	//	template unsigned int(16) channelcount = 2;
	//	template unsigned int(16) samplesize = 16;
	//	unsigned int(16) audio_cid = 0;
	//	const unsigned int(16) packet_size = 0 ;
	//	template unsigned int(32) samplerate = {timescale of media}<<16;
	//}

	if (GetSize() == 0x0c)
		return true;

	if (!SkipBytes(8)) {
		FATAL("Unable to skip 8 bytes");
		return false;
	}

	uint16_t version = 0;
	if (!ReadUInt16(version)) {
		FATAL("Unable to read version");
		return false;
	}

	if (!SkipBytes(18)) {
		FATAL("Unable to skip 18 bytes");
		return false;
	}

	switch (version) {
		case 0:
		{
			//standard isom
			break;
		}
		case 1:
		{
			//QT ver1
			//	const unsigned int(32) samples_per_frame;
			//	const unsigned int(32) bytes_per_packet;
			//	const unsigned int(32) bytes_per_frame;
			//	const unsigned int(32) bytes_per_sample;
			if (!SkipBytes(16)) {
				FATAL("Unable to skip 16 bytes");
				return false;
			}
			break;
		}
		case 2:
		{
			//QT ver2
			FATAL("QT version 2 not supported");
			return false;
		}
		default:
		{
			FATAL("MP4A version not supported");
			return false;
		}
	}

	return BoxAtom::Read();
}

bool AtomMP4A::AtomCreated(BaseAtom *pAtom) {
	switch (pAtom->GetTypeNumeric()) {
		case A_ESDS:
			_pESDS = (AtomESDS *) pAtom;
			return true;
		case A_WAVE:
			_pWAVE = (AtomWAVE *) pAtom;
			return true;
		case A_CHAN:
			_pCHAN = (AtomCHAN *) pAtom;
			return true;
		default:
		{
			FATAL("Invalid atom type: %s", STR(pAtom->GetTypeString()));
			return false;
		}
	}
}

#endif /* HAS_MEDIA_MP4 */
