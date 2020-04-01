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


#ifdef HAS_MEDIA_TS
#include "mediaformats/readers/ts/streamdescriptors.h"
#include "mediaformats/readers/ts/tsboundscheck.h"

bool ReadStreamDescriptor(StreamDescriptor &descriptor,
		uint8_t *pBuffer, uint32_t &cursor, uint32_t maxCursor) {
	TS_CHECK_BOUNDS(2);
	descriptor.type = pBuffer[cursor++];
	descriptor.length = pBuffer[cursor++];
	TS_CHECK_BOUNDS(descriptor.length);

	//iso13818-1.pdf Table 2-39, page 81/174
	switch (descriptor.type) {
		case 14://Maximum_bitrate_descriptor
		{
			TS_CHECK_BOUNDS(3);
			descriptor.payload.maximum_bitrate_descriptor.maximum_bitrate =
					(((pBuffer[cursor] << 16) | (pBuffer[cursor + 1] << 8) | (pBuffer[cursor + 2]))&0x3fffff) * 50 * 8;
			break;
		}
#ifdef HAS_MULTIPROGRAM_TS
		case DESCRIPTOR_TYPE_ISO_639_LANGUAGE:
		{
			TS_CHECK_BOUNDS(4);
			for (uint8_t i = 0; i < 3; i++)
				descriptor.payload.iso_639_descriptor.language_code[i] = pBuffer[cursor + i];
			descriptor.payload.iso_639_descriptor.audio_type = pBuffer[cursor + 3];
			break;
		}
#endif	/* HAS_MULTIPROGRAM_TS */
		default:
			break;
	}

	cursor += descriptor.length;
	return true;
}
#endif	/* HAS_MEDIA_TS */

