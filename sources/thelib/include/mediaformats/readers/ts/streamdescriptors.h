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
#ifndef _STREAMDESCRIPTORS_H
#define	_STREAMDESCRIPTORS_H

#include "common.h"

//frame rates
//ISO13818-2 table 6-4  (61/255)
#define FRAME_RATE_CODE_FORBIDEN 0
//23, 976
#define FRAME_RATE_CODE_24_PARTIAL 1
#define FRAME_RATE_CODE_24 2
#define FRAME_RATE_CODE_25 3
//29, 97
#define FRAME_RATE_CODE_30_PARTIAL 4
#define FRAME_RATE_CODE_30 5
#define FRAME_RATE_CODE_50 6
//59, 94
#define FRAME_RATE_CODE_60_PARTIAL 7
#define FRAME_RATE_CODE_60 8

//data stream alignaments
//ISO13818-1 Table 2-47 : Video stream alignment values (85/174)
#define DATA_STREAM_ALIGNMENT_SLICE 1
#define DATA_STREAM_ALIGNMENT_VIDEO_ACCESS_UNIT 2
#define DATA_STREAM_ALIGNMENT_GOP_SEQ 3
#define DATA_STREAM_ALIGNMENT_SEQ 4

//udio types values
//ISO13818-1 Table 2-53 : Audio type values (88/174)
#define AUDIO_TYPE_UNDEFINED 0x00
#define AUDIO_TYPE_CLEAN_EFFECTS 0x01
#define AUDIO_TYPE_HEARING_IMPAIRED 0x02
#define AUDIO_TYPE_VISUALY_IMPAIRED 0x03

//descriptors types
//ISO13818-1 Table 2-39 : Program and program element descriptors (81/174)
#define DESCRIPTOR_TYPE_USER_PRIVATE 255
#define DESCRIPTOR_TYPE_VIDEO 2
#define DESCRIPTOR_TYPE_REGISTRATION 5
#define DESCRIPTOR_TYPE_DATA_STREAM_ALIGNMENT 6
#define DESCRIPTOR_TYPE_ISO_639_LANGUAGE 10
#define DESCRIPTOR_TYPE_SMOOTHING_BUFFER 16
#define DESCRIPTOR_TYPE_IOD_DESCRIPTOR 29
#define DESCRIPTOR_TYPE_FMC_DESCRIPTOR 31

struct StreamDescriptor {
	uint8_t type;
	uint8_t length;

	union {

		struct {
			uint32_t maximum_bitrate;
		} maximum_bitrate_descriptor;
#ifdef HAS_MULTIPROGRAM_TS
		struct {
			uint8_t language_code[3];
			uint8_t audio_type;
		} iso_639_descriptor;
#endif	/* HAS_MULTIPROGRAM_TS */
	} payload;

	operator string() {
		return format("type: %hhu; length: %hhu", type, length);
	};
};

bool ReadStreamDescriptor(StreamDescriptor &descriptor,
		uint8_t *pBuffer, uint32_t &cursor, uint32_t maxCursor);


#endif	/* _STREAMDESCRIPTORS_H */
#endif	/* HAS_MEDIA_TS */

