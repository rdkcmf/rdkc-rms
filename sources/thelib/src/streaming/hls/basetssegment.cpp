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


#ifdef HAS_PROTOCOL_HLS
#include "streaming/hls/basetssegment.h"
#include "streaming/nalutypes.h"
#include "streaming/codectypes.h"
#include "protocols/drm/drmdefines.h"

/**
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * Licensed under the BSD-3 license.
**/
static uint32_t crc_table[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static uint32_t crc32(uint8_t *data, uint8_t len) {
	uint32_t crc = 0xffffffff;
	for (uint8_t i = 0; i < len; i++)
		crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];

	return crc;
}

BaseTSSegment::BaseTSSegment(IOBuffer &videoBuffer, IOBuffer &audioBuffer,
	IOBuffer &patPmtAndCounters, string const& drmType)
	: BaseSegment(audioBuffer, drmType), _videoBuffer(videoBuffer), 
	_patPmtAndCounters(patPmtAndCounters) {
		_videoBuffer.IgnoreAll();
		if (GETAVAILABLEBYTESCOUNT(_patPmtAndCounters) !=
			(8192 //counters
			+ 188 //pat
			+ 188 //pmt
			+ 188 //current packet
			)) {
				_patPmtAndCounters.IgnoreAll();
				_patPmtAndCounters.ReadFromRepeat(0, 8192 + 188 + 188 + 188); //counters+pat+pmt+packet
		}

		_pCounters = GETIBPOINTER(_patPmtAndCounters);

		_pPatPacket = _pCounters + 8192;
		_patPacketLength = 0;

		_pPmtPacket = _pPatPacket + 188;
		_pmtPacketLength = 0;

		_pPacket = _pPmtPacket + 188;

		_pmtPid = 4096;
		_pcrPid = 0;

		_videoPid = 0;

		_pcrCount = 0;

		_frameCount = 0;

		_metadataPid = 0;
}

BaseTSSegment::~BaseTSSegment() {
}

bool BaseTSSegment::WritePATPMT() {
#ifdef DEBUG_DUMP_TS_HEADER
	FATAL("WritePATPMT");
#endif /* DEBUG_DUMP_TS_HEADER */
	//2. Write PAT and PMT
	if (!WritePAT()) {
		FATAL("Unable to write PAT");
		return false;
	}
	if (!WritePMT()) {
		FATAL("Unable to write PMT");
		return false;
	}

	return true;
}

bool BaseTSSegment::PushVideoData(IOBuffer &buffer, int64_t pts, int64_t dts, Variant& videoParamsSizes) {
	if (_videoPid == 0)
		return true;
	if (GETAVAILABLEBYTESCOUNT(buffer) < 5) {
		WARN("Invalid packet");
		return true;
	}
	_frameCount++;
	uint8_t *pBuffer = GETIBPOINTER(buffer);
	uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);
	if (!WritePayload(
		pBuffer, //pBuffer,
		length, //length
		_videoPid, //pid
		pts, //presentation timestamp
		dts, //decoding timestamp
		videoParamsSizes //video param sizes
		)) {
			FATAL("Unable to write NAL");
			return false;
	}
	return true;
}

bool BaseTSSegment::PushMetaData(string const& vmfMetadata, int64_t pts) {
	size_t metadataLength = vmfMetadata.length();
	if (!WritePayload((uint8_t*)vmfMetadata.c_str(), (uint32_t)metadataLength, pts)) {
		FATAL("Unable to write metadata");
		return false;
	}
	return true;
}

bool BaseTSSegment::Init(StreamCapabilities *pCapabilities, unsigned char *pKey, unsigned char *pIV) {
	_pCapabilities = pCapabilities;

	uint64_t audioCodecType = _pCapabilities->GetAudioCodecType();

	bool hasAudio = ((audioCodecType == CODEC_AUDIO_AAC) 
#ifdef HAS_G711
					|| ((audioCodecType & CODEC_AUDIO_G711) == CODEC_AUDIO_G711)
#endif	/* HAS_G711 */
					);

	bool hasVideo = (_pCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264);
	if (!(hasAudio || hasVideo)) {
#ifdef HAS_G711
		FATAL("In stream is not transporting H.264/AAC|G711 content");
#else
		FATAL("In stream is not transporting H.264/AAC content");
#endif	/* HAS_G711 */
		return false;
	}

	//Add audio/video streams
	if (hasAudio) {
		AddStream(audio);
	}
	if (hasVideo) {
		_pcrPid = AddStream(video);
	} else {
		_pcrPid = _audioPid;
	}
	//Add the metadata. No checking needed if metadata exists, this will simply
	//add a fix PSI info for metadata, but segments will only contain the actual metadata
	//when it arrived from the wire. Same behaviour is implemented by Apple's mediafilesegmenter tool.
	AddStream(metadata);

	if (_drmType != "" && _drmType != DRM_TYPE_SAMPLE_AES) {
		if ((pKey == NULL)
			|| (pIV == NULL)
			|| (EVP_EncryptInit_ex(_ctxEncrypt, EVP_aes_128_cbc(), NULL, pKey, pIV) != 1)) {
				FATAL("Unable to setup encryption");
				return false;
		}
	}

	_pKey = pKey;
	_pIV = pIV;

	return WritePATPMT();
}

bool BaseTSSegment::FlushPacket() {
	if (_drmType == "" || _drmType == DRM_TYPE_SAMPLE_AES)
		return true;
	//Flush out any remnants of the last encryption
	uint32_t newSize = 0;
	char *pCipherBuf = FinalEncryptBuffer(&newSize);
	if (pCipherBuf == NULL) {
		FATAL("Unable to encrypt final packet");
		return false;
	}
	return WritePacket((uint8_t *) pCipherBuf, newSize);
}

uint16_t BaseTSSegment::AddStream(PID_TYPE pidType) {
	uint16_t result = 0;
	switch (pidType) {
	case audio:
		_audioPid = result = 257;
		break;
	case video:
		_videoPid = result = 256;
		break;
	case metadata:
		_metadataPid = result = 258;
		break;
	}
	return result;
}

bool BaseTSSegment::WritePAT() {
	//1. Is the PAT already constructed?
	if (_pPatPacket[0] != 0) {
		//2. put the counter
		_pPatPacket[3] = (_pPatPacket[3]&0xf0) | (_pCounters[0]&0x0f);
		//3. increment the counter
		_pCounters[0]++;
	} else {
		//4. Build the PAT packet
		if (!BuildPATPacket()) {
			FATAL("Unable to build PAT packet");
			return false;
		}
	}

	//5. Done
#ifdef DEBUG_DUMP_TS_HEADER
	DumpTS4BytesHeader(_pPatPacket);
#endif /* DEBUG_DUMP_TS_HEADER */
	return EncryptPacket(_pPatPacket, 188);
}

bool BaseTSSegment::WritePMT() {
	if (_pPmtPacket[0] != 0) {
		//2. put the counter
		_pPmtPacket[3] = (_pPmtPacket[3]&0xf0) | (_pCounters[_pmtPid]&0x0f);
		//3. increment the counter
		_pCounters[_pmtPid]++;
	} else {
		//4. Build the PAT packet
		if (!BuildPMTPacket()) {
			FATAL("Unable to build PAT packet");
			return false;
		}
	}

	//5. Done
#ifdef DEBUG_DUMP_TS_HEADER
	DumpTS4BytesHeader(_pPmtPacket);
#endif /* DEBUG_DUMP_TS_HEADER */
	return EncryptPacket(_pPmtPacket, 188);
}

bool BaseTSSegment::BuildPATPacket() {
	//1. Build packet header
	if (!BuildPacketHeader(
		_pPatPacket, //pBuffer
		true, //payloadStart
		0, //PAT
		false, //hasAdaptationField
		true //hasPayload
		)) {
			FATAL("Unable to build packet header");
			return false;
	}
	_patPacketLength = 4;

	//2. 0 pointer
	_pPatPacket[_patPacketLength++] = 0;

	//Table 2-25 ??? Program association section
	//iso13818-1.pdf page 61/174

	//3. table_id
	_pPatPacket[_patPacketLength++] = 0;

	//4. section_syntax_indicator,'0',reserved,section_length to 0
	//section_length will be computed at the end
	uint8_t sectionLengthIndex = _patPacketLength;
	_patPacketLength += 2;

	//5. transport_stream_id will be always 1
	EHTONSP(_pPatPacket + _patPacketLength, 1);
	_patPacketLength += 2;

	//6. reserved, version_number,current_next_indicator.
	//We will always have one and only one version of this TS.
	//So, hard-code it to 0. Also, is immediately aplicable (current_next_indicator = 1)
	_pPatPacket[_patPacketLength++] = 0xc1;

	//7. section_number,last_section_number. We will always have one section.
	_pPatPacket[_patPacketLength++] = 0;
	_pPatPacket[_patPacketLength++] = 0;

	//8. program_number. We will always have one program we will give this
	//the ID number 1
	EHTONSP(_pPatPacket + _patPacketLength, 1);
	_patPacketLength += 2;

	//9. reserved,program_map_PID
	EHTONSP(_pPatPacket + _patPacketLength, (_pmtPid & 0x1fff) | 0xe000);
	_patPacketLength += 2;

	//10. Put in the section length
	EHTONSP(_pPatPacket + sectionLengthIndex, (((_patPacketLength - 4)&0x03ff) | 0xb000));

	//11. CRC32
	uint32_t crc = crc32(_pPatPacket + 5, _patPacketLength - 5);
	EHTONLP(_pPatPacket + _patPacketLength, crc);
	_patPacketLength += 4;

	//12. Stuffing
	memset(_pPatPacket + _patPacketLength, 0xff, 188 - _patPacketLength);

	//13. Done
	return true;
}

bool BaseTSSegment::BuildPMTPacket() {
	//1. Build packet header
	if (!BuildPacketHeader(
		_pPmtPacket, //pBuffer
		true, //payloadStart
		_pmtPid, //PMT
		false, //hasAdaptationField
		true //hasPayload
		)) {
			FATAL("Unable to build packet header");
			return false;
	}
	_pmtPacketLength = 4;

	//2. 0 pointer
	_pPmtPacket[_pmtPacketLength++] = 0;

	//Table 2-28 ??? Transport Stream program map section
	//iso13818-1.pdf page 64/174

	//3. table_id
	_pPmtPacket[_pmtPacketLength++] = 2;

	//4. section_syntax_indicator,'0',reserved,section_length to 0
	//section_length will be computed at the end
	uint8_t sectionLengthIndex = _pmtPacketLength;
	_pmtPacketLength += 2;

	//5. program_number. We will always have one program we will give this
	//the ID number 1
	EHTONSP(_pPmtPacket + _pmtPacketLength, 1);
	_pmtPacketLength += 2;

	//6. reserved, version_number,current_next_indicator.
	//We will always have one and only one version of this TS.
	//So, hard-code it to 0. Also, is immediately applicable (current_next_indicator = 1)
	_pPmtPacket[_pmtPacketLength++] = 0xc1;

	//7. section_number,last_section_number. We will always have one section.
	_pPmtPacket[_pmtPacketLength++] = 0;
	_pPmtPacket[_pmtPacketLength++] = 0;

	//8. PCR_PID
	//_pcrPid = 0;
	//FINEST("_pcrPid: %hu", _pcrPid);
	EHTONSP(_pPmtPacket + _pmtPacketLength, ((_pcrPid & 0x1fff) | 0xe000));
	_pmtPacketLength += 2;

	//9. reserved, program_info_length. We don't offer any program info
	//So, hard-code it to 0
	EHTONSP(_pPmtPacket + _pmtPacketLength, 0xf000);
	_pmtPacketLength += 2;

	//implementation of Timed Metadata is based on Apple developer specification found here
	//https://developer.apple.com/library/ios/documentation/AudioVideo/Conceptual/HTTP_Live_Streaming_Metadata_Spec/2/2.html#//apple_ref/doc/uid/TP40010435-CH2-DontLinkElementID_1
	if (_metadataPid != 0) {
		uint8_t program_info_len_offset = _pmtPacketLength - 2;
		//see section 2.3.2 Descriptor Loop of the PMT for the Program
		uint8_t metadata_pointer_descriptor[] = {
			0x25, 0x0f,	0xff, 0xff,		//1 byte descriptor tag, 1 byte length, 2 bytes metadata_application_format
			0x49, 0x44, 0x33, 0x20,		//metadata_format_identifier, value is 'I','D','3',' '
			0xff,						//metadata_format
			0x49, 0x44, 0x33, 0x20,		//metadata_format_identifier
			0x00, 0x1f,					//1 byte metadata_service_id, metadata_locator_record_flag, MPEG_carriage_flags, 1 byte reserved
			0x00, 0x01					//program_number of whose es descriptor loop contains the metadata_descriptor
		};
		memcpy(_pPmtPacket + _pmtPacketLength, metadata_pointer_descriptor, sizeof(metadata_pointer_descriptor));
		_pmtPacketLength += sizeof(metadata_pointer_descriptor);
		//update the program_info_length
		EHTONSP(_pPmtPacket + program_info_len_offset, (sizeof(metadata_pointer_descriptor) & 0xfff) | 0xf000);
	}

    //any values (constants, memory location, etc...) pertaining to encryption in this function are all derived from this document
    //https://developer.apple.com/library/ios/documentation/AudioVideo/Conceptual/HLS_Sample_Encryption/TransportStreamSignaling/TransportStreamSignaling.html#//apple_ref/doc/uid/TP40012862-CH3-SW1
	
	if (_videoPid != 0) {
		//10. stream_type for video pid
		if (_drmType == DRM_TYPE_SAMPLE_AES)
			_pPmtPacket[_pmtPacketLength++] = 0xdb; //AES-128 encrypted h264
		else
			_pPmtPacket[_pmtPacketLength++] = 0x1b; //h264

		//11. reserved, elementary_PID for video pid
		EHTONSP(_pPmtPacket + _pmtPacketLength, ((_videoPid & 0x1fff) | 0xe000));
		_pmtPacketLength += 2;

        if (_drmType == DRM_TYPE_SAMPLE_AES) {
            uint8_t private_data_indicator[] = {
                0xf0, 0x06,             //reserved, ES_info_length
                0x0f, 0x04,             //8 bit descriptor tag and 8 bit data length
                0x7a, 0x61, 0x76, 0x63  //'z','a','v','c' - check `3.0 Transport Stream Signaling` section
            };
            memcpy(_pPmtPacket + _pmtPacketLength, private_data_indicator, sizeof(private_data_indicator));
            EHTONLP(_pPmtPacket + _pmtPacketLength + 4, 0x7A617663); //change the byte order of the private_data_indicator to network order(little endian)
            _pmtPacketLength += sizeof(private_data_indicator);
        } else {
            //12. reserved, ES_info_length for video pid. no info
            EHTONSP(_pPmtPacket + _pmtPacketLength, 0xf000);
            _pmtPacketLength += 2;
        }
	}

	if (_audioPid != 0) {
		//13. Add audio pid
		uint8_t temp[] = {0xF0, 0x06, 0x0A, 0x04, 0x75, 0x6E, 0x64, 0x00};
		if (_drmType == DRM_TYPE_SAMPLE_AES) {
			uint8_t private_data_indicator_len = 6;
			uint8_t audio_setup_information_len = 16;

			//14. Do we have enough space?
			if (((uint8_t)(188 - _pmtPacketLength)) < (3 + sizeof(temp) + private_data_indicator_len
				+ audio_setup_information_len)) {
				//0x0b is the length of the audio pid+audio info
				//which now is hardcoded to "eng"
				FATAL("Not enough space. Too mane audio pids");
				return false;
			}
		}
		else {
			//14. Do we have enough space?
			if (((uint8_t)(188 - _pmtPacketLength)) < (3 + sizeof(temp))) {
				//0x0b is the length of the audio pid+audio info
				//which now is hardcoded to "eng"
				FATAL("Not enough space. Too mane audio pids");
				return false;
			}
		}

		//15. stream_type for audio pid
#ifdef HAS_G711
		uint64_t audioCodecType = 0;
		audioCodecType = _pCapabilities->GetAudioCodecType();
		if (audioCodecType == CODEC_AUDIO_AAC) {
#endif	/* HAS_G711 */
            if (_drmType == DRM_TYPE_SAMPLE_AES)
                _pPmtPacket[_pmtPacketLength++] = 0xcf; //AES-128 encrypted ADTS containing AAC
            else
                _pPmtPacket[_pmtPacketLength++] = 0x0f; //ADTS containing AAC
#ifdef HAS_G711
		} else if ((audioCodecType & CODEC_AUDIO_G711) == CODEC_AUDIO_G711) {
			//We need a G711 identifier here which is not provided in specs.
			_pPmtPacket[_pmtPacketLength++] = 0x06; //ISO_IEC_13818_1_PES
		}
#endif	/* HAS_G711 */

		//16. reserved, elementary_PID for audio pid
		EHTONSP(_pPmtPacket + _pmtPacketLength, ((_audioPid & 0x1fff) | 0xe000));
		_pmtPacketLength += 2;

		//17. hard-code the reserved, ES_info_length to "und"
		memcpy(_pPmtPacket + _pmtPacketLength, temp, sizeof (temp));
		_pmtPacketLength += sizeof (temp);

		if (_drmType == DRM_TYPE_SAMPLE_AES) {
			uint8_t private_data_indicator[] = {
				0x0f, 0x04,             //8 bit descriptor tag and 8 bit data length
				0x61, 0x61, 0x63, 0x64  //'a','a','c','d' - check `3.0 Transport Stream Signaling` section
			};
			//check `2.3.4 Audio Setup Information`
			uint8_t audio_setup_information[] = {
				0x05, 0x0e,             //tag descriptor and length. Tag descriptor of 5 means it is a Registration Descriptor
				0x61, 0x70, 0x61, 0x64, //format_identifier = 'a','p','a','d' - check `2.4.3.5 Transport Stream Setup`
				0x7a, 0x61, 0x61, 0x63, //audio_type = 'z','a','a','c' - AAC-LC - check `2.4.3.2 AAC Setup`
				0x00, 0x00,             //priming
				0x01,                   //version
				0x02,                   //setup_data_length
				0x11, 0x90              //setup_data = values just copied from Apple's mediafilesegmenter
			};

			uint8_t es_info_len_offset = _pmtPacketLength - sizeof(temp);
			memcpy(_pPmtPacket + _pmtPacketLength, private_data_indicator, sizeof(private_data_indicator));
			_pmtPacketLength += sizeof(private_data_indicator);
			//copy the audio setup information to the Registration Descriptor
			//check 2.6.8 Registration descriptor of ISO13818-1.pdf (TS specs.)
			memcpy(_pPmtPacket + _pmtPacketLength, audio_setup_information, sizeof(audio_setup_information));
			_pmtPacketLength += sizeof(audio_setup_information);

			uint8_t es_info_length = sizeof(audio_setup_information)
				+ sizeof(private_data_indicator) + sizeof(temp) - 2;
			//update ES_info_length;
			EHTONSP(_pPmtPacket + es_info_len_offset, ((es_info_length & 0xfff) | 0xf000));
		}
	}

	if (_metadataPid != 0) {
		//see section 2.3.3 Descriptor Loop of the PMT for the Elementary Stream
		uint8_t metadata_descriptor[] = {
			0x26, 0x0d, 0xff, 0xff,		//1 byte descriptor_tag, 1 byte descriptor_length, 2 bytes metadata_application_format
			0x49, 0x44, 0x33, 0x20,		//metadata_application_format_identifier, value is 'I','D','3',' '
			0xff,						//metadata_format
			0x49, 0x44, 0x33, 0x20,		//metadata_format_identifier
			0x00, 0x0f					//1 byte metadata_service_id, byte decoder_config_flags, byte DSM-CC_flag, 1 byte reserved
		};

		if (((uint8_t)(188 - _pmtPacketLength)) < (3 + sizeof(metadata_descriptor))) {
			//0x0b is the length of the audio pid+audio info
			//which now is hardcoded to "eng"
			FATAL("Not enough space for metadata pid");
			return false;
		}

		_pPmtPacket[_pmtPacketLength++] = 0x15; //stream_type
		//reserved, elementary_PID for metadata pid
		EHTONSP(_pPmtPacket + _pmtPacketLength, ((_metadataPid & 0x1fff) | 0xe000)); //3 bit reserved, 13 bit pid
		_pmtPacketLength += 2;

		EHTONSP(_pPmtPacket + _pmtPacketLength, 0xf000); //4 bit reserved, 12 bit ES_info_length
		uint8_t ES_reserved_offset = _pmtPacketLength;
		_pmtPacketLength += 2;

		//copy the metadata_descriptor to PMT packet
		memcpy(_pPmtPacket + _pmtPacketLength, metadata_descriptor, sizeof(metadata_descriptor));
		_pmtPacketLength += sizeof(metadata_descriptor);

		//update ES_info_length;
		EHTONSP(_pPmtPacket + ES_reserved_offset, ((sizeof(metadata_descriptor) & 0xfff) | 0xf000));
	}

	//18. Put in the section length
	EHTONSP(_pPmtPacket + sectionLengthIndex, (((_pmtPacketLength - 4)&0x03ff) | 0xb000));

	//19. CRC32
	uint32_t crc = crc32(_pPmtPacket + 5, _pmtPacketLength - 5);
	EHTONLP(_pPmtPacket + _pmtPacketLength, crc);
	_pmtPacketLength += 4;

	//20. Stuffing
	memset(_pPmtPacket + _pmtPacketLength, 0xff, 188 - _pmtPacketLength);

	//21. Done
	return true;
}

bool BaseTSSegment::BuildPacketHeader(uint8_t *pBuffer, bool payloadStart,
	uint16_t pid, bool hasAdaptationField, bool hasPayload) {
		//sync_byte
		pBuffer[0] = 0x47;

		//PID
		EHTONSP(pBuffer + 1, (pid & 0x1fff));

		//transport_error_indicator
		pBuffer[1] &= 0x7f;

		//payload_unit_start_indicator
		if (payloadStart)
			pBuffer[1] |= 0x40;
		else
			pBuffer[1] &= 0xbf;

		//transport_priority
		pBuffer[1] &= 0xdf;

		//continuity_counter
		pBuffer[3] = (_pCounters[pid & 0x1fff])&0x0f;
		if (hasPayload)
			_pCounters[pid & 0x1fff]++;

		//transport_scrambling_control
		pBuffer[3] &= 0x3f;

		//adaptation_field_control
		if (hasAdaptationField)
			pBuffer[3] |= 0x20;
		if (hasPayload)
			pBuffer[3] |= 0x10;

		return true;
}

#define PCR_INTERVAL 1

bool BaseTSSegment::WritePayload(uint8_t* pBuffer, uint32_t length, int64_t pts) {
	return WritePayload(pBuffer, length, _metadataPid, pts, pts);
}

bool BaseTSSegment::WritePayload(uint8_t *pBuffer, uint32_t length, uint16_t pid, int64_t pts, int64_t dts) {
	Variant dummy;
	return WritePayload(pBuffer, length, pid, pts, dts, dummy);
}

bool BaseTSSegment::WritePayload(uint8_t *pBuffer, uint32_t length, 
	uint16_t pid, int64_t pts, int64_t dts, Variant& videoParamsSizes) {
		uint32_t consumed = 0;
		uint32_t required = 0;
		uint32_t available = 0;
		bool hasAf = false;
		uint32_t dataStart = 0;
		uint32_t chunkStart = 0;
		uint32_t chunkSize = 0;
		uint8_t afLength = 0;
		uint8_t pesLength = 14;
		uint8_t id3Length = 62;
		uint32_t pesStart = 0;

		//encryption implementation for SAMPLE-AES as documented in Apple's developer library
		//https://developer.apple.com/library/ios/documentation/AudioVideo/Conceptual/HLS_Sample_Encryption/Encryption/Encryption.html#//apple_ref/doc/uid/TP40012862-CH2-SW7  
		if (_drmType == DRM_TYPE_SAMPLE_AES) {
			uint8_t* nalUnit = NULL;
			bool encryptNal = true;
			uint32_t nalUnitSize = 0;
			uint32_t packetOffset = 0;
			if (pid == _videoPid) {
				if ((uint32_t)videoParamsSizes["VCLSize"] <= 48) //since the size of encryptable block is fix (16 bytes), we need to have at least 1 encryptable block + at least 1 trailing byte
					encryptNal = false;
				uint8_t nalType = NALU_TYPE_UNDEFINED;
				uint32_t cursor = 4; // 4 bytes nalu start prefix code
				if (encryptNal)
					nalType = NALU_TYPE(pBuffer[cursor]);

				if (nalType == NALU_TYPE_PD) {
					//if first nalu is an AUD then read the next nalu in this access unit
					uint32_t nalSize = (uint32_t)videoParamsSizes["AUDSize"];
					do {
						cursor += nalSize;
						cursor += 4; // 4 bytes nalu start prefix code
						nalType = NALU_TYPE(pBuffer[cursor]);
						if (nalType == NALU_TYPE_SPS)
							nalSize = (uint32_t)videoParamsSizes["SPSSize"];
						else if (nalType == NALU_TYPE_PPS)
							nalSize = (uint32_t)videoParamsSizes["PPSSize"];
					} while (nalType == NALU_TYPE_SPS || nalType == NALU_TYPE_PPS);
				}
				if (length > cursor && (nalType == NALU_TYPE_SLICE || nalType == NALU_TYPE_IDR)) { //as per specs/documentation, only nal type of 1 & 5 should be encrypted
					nalUnitSize = length - cursor;
					packetOffset = cursor;
					nalUnit = new uint8_t[nalUnitSize];
					memcpy(nalUnit, pBuffer + packetOffset, nalUnitSize);
				}
				else {
					encryptNal = false;
				}
			}
			else if (pid == _audioPid) {
				if (length < 39)
					encryptNal = false;
				if (encryptNal) {
					nalUnitSize = length;
					nalUnit = new uint8_t[nalUnitSize];
					memcpy(nalUnit, pBuffer + consumed, nalUnitSize);
				}
			}

			if (encryptNal) {
				uint32_t bytesRemaining = nalUnitSize;
				uint32_t blockCounter = 0;
				uint32_t nalUnitCurrentOffset = 0;
				uint8_t* encryptedBlock = NULL;
				uint32_t encryptedSize = 0;
				uint8_t nalLeadingBytes = 0;
				if (pid == _videoPid) {
					nalLeadingBytes = 32; // 1 byte NAL type + 31 leading bytes accdg. to specs.
					bytesRemaining -= nalLeadingBytes;

					if ((_pKey == NULL)
						|| (_pIV == NULL)
						|| (EVP_EncryptInit_ex(_ctxEncrypt, EVP_aes_128_cbc(), NULL, _pKey, _pIV) != 1)) {
						FATAL("Unable to setup encryption");
						return false;
					}

					uint8_t* naluBuffer = new uint8_t[nalUnitSize];
					memcpy(naluBuffer, nalUnit, nalLeadingBytes);
					uint32_t sizeDiff = 0;
					uint32_t naluBufferTotalSize = nalUnitSize;

					while (bytesRemaining > AES_BLOCK_SIZE) {
						++blockCounter;
						nalUnitCurrentOffset = nalLeadingBytes + (blockCounter - 1) * AES_BLOCK_SIZE;
						//only encrypt 1 block (16 bytes) out of 10 blocks a.k.a. 10% block skipping
						if ((blockCounter % 10) == 1) {
							if (!EncryptSample(nalUnit + nalUnitCurrentOffset, AES_BLOCK_SIZE, encryptedBlock, encryptedSize, false)) {
								FATAL("Encryption of sample failed");
								return false;
							}
							if (encryptedSize == AES_BLOCK_SIZE)
								memcpy(naluBuffer + nalUnitCurrentOffset, encryptedBlock, AES_BLOCK_SIZE);
							else if (encryptedSize > AES_BLOCK_SIZE) {
								//EPB was applied, so the encrypted block size grew.
								sizeDiff = AES_BLOCK_SIZE - encryptedSize;
								if ((nalUnitCurrentOffset + encryptedSize) > (nalUnitSize - 1)){
									uint8_t* temp = new uint8_t[nalUnitCurrentOffset];
									memcpy(temp, naluBuffer, nalUnitCurrentOffset);
									delete[] naluBuffer;
									naluBuffer = new uint8_t[nalUnitSize + sizeDiff];
									memcpy(naluBuffer, temp, nalUnitCurrentOffset);
									delete[] temp;
								}
								memcpy(naluBuffer + nalUnitCurrentOffset, encryptedBlock, encryptedSize);
								naluBufferTotalSize += sizeDiff;
							}
							delete[] encryptedBlock;
						} else {
							memcpy(naluBuffer + nalUnitCurrentOffset + sizeDiff, nalUnit + nalUnitCurrentOffset, AES_BLOCK_SIZE);
							sizeDiff = 0;
						}
						bytesRemaining -= AES_BLOCK_SIZE;
					}
					if (bytesRemaining > 0)
						memcpy(naluBuffer + (naluBufferTotalSize - bytesRemaining), nalUnit + (nalUnitSize - bytesRemaining), bytesRemaining);

					delete[] nalUnit;
					nalUnit = naluBuffer;
					nalUnitSize = naluBufferTotalSize;
					naluBuffer = NULL;
				}
				else if (pid == _audioPid) {
					nalLeadingBytes = 23; //7 bytes ADTS header + 16 leading bytes accdg. to specs.
					bytesRemaining -= nalLeadingBytes;

					if ((_pKey == NULL)
						|| (_pIV == NULL)
						|| (EVP_EncryptInit_ex(_ctxEncrypt, EVP_aes_128_cbc(), NULL, _pKey, _pIV) != 1)) {
						FATAL("Unable to setup encryption");
						return false;
					}

					while (bytesRemaining >= AES_BLOCK_SIZE) {
						++blockCounter;
						nalUnitCurrentOffset = nalLeadingBytes + (blockCounter - 1) * AES_BLOCK_SIZE;
						if (!EncryptSample(nalUnit + nalUnitCurrentOffset, AES_BLOCK_SIZE, encryptedBlock, encryptedSize, true)) {
							FATAL("Encryption of sample failed");
							return false;
						}

						memcpy(nalUnit + nalUnitCurrentOffset, encryptedBlock, AES_BLOCK_SIZE);
						delete[] encryptedBlock;
						bytesRemaining -= AES_BLOCK_SIZE;
					}
				}

				memcpy(pBuffer + packetOffset, nalUnit, nalUnitSize);
				delete[] nalUnit;
			}
		}

		while (consumed != length) {
			afLength = 0;

			//1. reset everything
			memset(_pPacket, 0xff, 188);

			//1.1. Compute the PES length
			if (pts != dts)
				pesLength += 5;

			//2. compute required
			available = 184;
			required = length - consumed;
			if (consumed == 0 || (consumed > 0xffff && pid == _metadataPid)) //65535 bytes is the maximum metadata in one PES header
				required += pesLength;

			if (pid == _metadataPid && (consumed == 0 || consumed > 0xffff))
				required += id3Length;

			//3. compute has AF
			if (pid == _pcrPid) {
				hasAf = (consumed == 0) || (required < available);
				if (hasAf) {
					if (consumed == 0) {
						if ((_pcrCount % PCR_INTERVAL) == 0) {
							afLength = 8;
						} else {
							afLength = 2;
						}
						_pcrCount++;
					} else {
						afLength = 2;
					}
				}
			} else if (required < available && pid != _metadataPid) {
				hasAf = true;
				afLength = 2;
			} else {
				hasAf = false;
				afLength = 0;
			}
			available -= afLength;

			//4. write the adaptation field
			if (hasAf) {
				if (afLength == 2) {
					_pPacket[4] = 0x00;
					_pPacket[5] = 0x00;
				} else {
					uint64_t value = (pts + 63000) * 300;
					int64_t pcr_low = value % 300, pcr_high = value / 300;
					_pPacket[4] = 0x07;
					_pPacket[5] = 0x10;
					_pPacket[6] = (uint8_t) (pcr_high >> 25);
					_pPacket[7] = (uint8_t) (pcr_high >> 17);
					_pPacket[8] = (uint8_t) (pcr_high >> 9);
					_pPacket[9] = (uint8_t) (pcr_high >> 1);
					_pPacket[10] = (uint8_t) (pcr_high << 7 | pcr_low >> 8 | 0x7e);
					_pPacket[11] = (uint8_t) (pcr_low);
					//_pcrPid = 0;
				}
			}

			//7. compute data start
			if (required < available)
				dataStart = 188 - required;
			else
				dataStart = 188 - available;

			if (pid == _metadataPid)
				pesStart = 188 - available;
			else
				pesStart = dataStart;

			//8. compute chunkSize
			chunkSize = 188 - dataStart;
			if (consumed == 0 || (consumed > 0xffff && pid == _metadataPid))
				chunkSize -= pesLength;

			if (pid == _metadataPid && (consumed == 0 || consumed > 0xffff))
				chunkSize -= id3Length;

			//9. Compute the nalStart
			chunkStart = dataStart;
			if (consumed == 0 || (consumed > 0xffff && pid == _metadataPid))
				chunkStart += pesLength;

			if (pid == _metadataPid && (consumed == 0 || consumed > 0xffff))
				chunkStart += id3Length;

			//10. put pes header if necessary
			if (consumed == 0 || (consumed > 0xffff && pid == _metadataPid)) {
				//packet_start_code_prefix, stream_id. Stream id hard-coded to E0
				if (pid == _videoPid) {
					EHTONLP(_pPacket + pesStart, 0x000001E0);
				} else if (pid == _audioPid) {
#ifdef HAS_G711
					uint64_t audioCodecType = 0;
					audioCodecType = _pCapabilities->GetAudioCodecType();
					if (audioCodecType == CODEC_AUDIO_AAC) {
#endif	/* HAS_G711 */
					EHTONLP(_pPacket + pesStart, 0x000001C0);
#ifdef HAS_G711
					} else if ((audioCodecType & CODEC_AUDIO_G711) == CODEC_AUDIO_G711) {
						EHTONLP(_pPacket + pesStart, 0x000001BD); //set the stream as private_stream
					}
#endif	/* HAS_G711 */
				//Timed Metadata implementation details based on Apple developer documentation
				//https://developer.apple.com/library/ios/documentation/AudioVideo/Conceptual/HTTP_Live_Streaming_Metadata_Spec/2/2.html
				} else if (pid == _metadataPid) {
					EHTONLP(_pPacket + pesStart, 0x000001BD); //24 bit - packet_start_code_prefix, 8 bit - metadata stream_id
				}

				if (pid != _metadataPid) {
					//PES_packet_length
					if (length + 8 > 0xffff) {
						EHTONSP(_pPacket + pesStart + 4, 0);
					} else {
						if (pts != dts) {
							//FINEST("PTS - DTS");
							EHTONSP(_pPacket + pesStart + 4, (uint16_t)length + 8 + 5);
						} else {
							//FINEST("PTS - ONLY");
							EHTONSP(_pPacket + pesStart + 4, (uint16_t)length + 8);
						}
					}
				} else {
					//pesPacketLength is the size of space from this location (pesStart + 4) upto the end of the TS frame
					uint16_t pesPacketLength = 0;
					if (length > 0xffff)
						pesPacketLength = chunkStart + 0xffff - 10;
					else
						pesPacketLength = chunkStart + length - 10;

					EHTONSP(_pPacket + pesStart + 4, pesPacketLength);
				}
				
				//(2 bit)PTS_DTS_flags ESCR_flag,(1 bit)ES_rate_flag,(1 bit)DSM_trick_mode_flag,(1 bit)additional_copy_info_flag,(1 bit)PES_CRC_flag,(1 bit)PES_extension_flag
				//from all that, we only set PTS_DTS_flags to "has only pts"
				if (pid != _metadataPid) {
					//'10',(2 bit)PES_scrambling_control,(1 bit)PES_priority,(1 bit)data_alignment_indicator,(1 bit)copyright,(1 bit)original_or_copy
					_pPacket[pesStart + 6] = 0x80;
					if (pts != dts) {
						_pPacket[pesStart + 7] = 0xC0;
						//PES_header_data_length. Always 10
						_pPacket[pesStart + 8] = 0x0A;
					} else {
						_pPacket[pesStart + 7] = 0x80;
						//PES_header_data_length. Always 5
						_pPacket[pesStart + 8] = 0x05;
					}
				} else {
					//'10',(2 bit)PES_scrambling_control,(1 bit)PES_priority,(1 bit)data_alignment_indicator,(1 bit)copyright,(1 bit)original_or_copy
					if (consumed == 0)
						_pPacket[pesStart + 6] = 0x84; //data_alignment_indicator is set to 1 if it is the first ID3 packet (2.4 PES Stream Format)
					else
						_pPacket[pesStart + 6] = 0x80;

					if (consumed > 0xffff) //if metadata is > 64k, zero out the PTS_DTS flags
						_pPacket[pesStart + 7] = 0x00;
					else
						_pPacket[pesStart + 7] = 0x80;
					// Get the size of the space between this position (pesStart + 8) upto the start offset of ID3 header
					uint8_t pesHeaderLength = 0;
					if ((uint8_t)chunkStart > id3Length - 13)
						pesHeaderLength = chunkStart - id3Length - 13;
					_pPacket[pesStart + 8] = pesHeaderLength;
				}

				//PCR
				uint64_t value = pts + 63000 * 2;
				uint16_t val = 0;
				uint8_t ppts[5];
				if (pts == dts) { // PTS only
					// 1: 4 bit id value(0010) | TS [32..30] | marker_bit
					ppts[0] = ((2 << 4) | ((value >> 29) & 0x0E) | 0x01) & 0xff;
					val = ((value >> 14) & 0xfffe) | 0x01;
					// 2, 3: TS[29..15] | marker_bit
					ppts[1] = (val >> 8) & 0xff;
					ppts[2] = val & 0xff;
					val = ((value << 1) & 0xfffe) | 0x01;
					// 4, 5: TS[14..0] | marker_bit
					ppts[3] = (val >> 8) & 0xff;
					ppts[4] = val & 0xff;
					memcpy(_pPacket + pesStart + 9, ppts, 5);
				} else { // PTS DTS
					//pts
					// 1: 4 bit id value(0011) | TS [32..30] | marker_bit
					ppts[0] = ((3 << 4) | ((value >> 29) & 0x0E) | 0x01) & 0xff;
					val = ((value >> 14) & 0xfffe) | 0x01;
					// 2, 3: TS[29..15] | marker_bit
					ppts[1] = (val >> 8) & 0xff;
					ppts[2] = val & 0xff;
					val = ((value << 1) & 0xfffe) | 0x01;
					// 4, 5: TS[14..0] | marker_bit
					ppts[3] = (val >> 8) & 0xff;
					ppts[4] = val & 0xff;
					memcpy(_pPacket + pesStart + 9, ppts, 5);
					//dts
					value = dts + 63000 * 2;
					// 1: 4 bit id value(0001) | TS [32..30] | marker_bit
					ppts[0] = ((1 << 4) | ((value >> 29) & 0x0E) | 0x01) & 0xff;
					val = ((value >> 14) & 0xfffe) | 0x01;
					// 2, 3: TS[29..15] | marker_bit
					ppts[1] = (val >> 8) & 0xff;
					ppts[2] = val & 0xff;
					val = ((value << 1) & 0xfffe) | 0x01;
					// 4, 5: TS[14..0] | marker_bit
					ppts[3] = (val >> 8) & 0xff;
					ppts[4] = val & 0xff;
					memcpy(_pPacket + pesStart + 9 + 5, ppts, 5);
				}
			}

			if (pid == _videoPid || pid == _audioPid) {
				//11. put the data
				memcpy(_pPacket + chunkStart, pBuffer + consumed, chunkSize);

				//13. fix adaptation field length
				if (hasAf) {
					_pPacket[4] = (uint8_t)(dataStart - 5);
				}
			}
			else if (pid == _metadataPid) {
				uint32_t offset = chunkStart;
				if (consumed == 0 || consumed > 0xffff) {
					//build ID3v4 tag here, encapsulating the vmf metadata
					//ID3v4 specs. derived from this link (http://id3.org/id3v2.4.0-structure)
					uint8_t ID3Header[] = {
						0x49, 0x44, 0x33,		//'I','D','3' mandatory header
						0x04, 0x00,				//version - major = 4, minor = 0
						0x00,					//flags
						0x00, 0x00, 0x00, 0x00,	//frame size, this should be replaced by the actual metadata length
					};
					uint8_t frameHeader[] = {
						0x47, 0x45, 0x4f, 0x42,	//frame id - 'G','E','O','B' it means generic object, any data that does not conform to other formats
						0x00, 0x00, 0x00, 0x00, //size - this should be replaced by the actual metadata length
						0x00, 0x00				//flags - normally just an empty bytes
						//0x03					//encoding id - UTF-8
					};
					uint8_t metadataheader[] = {
						0x03,					//encoding id - UTF-8
						0x61, 0x70, 0x70, 0x6c, //next 24 bytes is mime type
						0x69, 0x63, 0x61, 0x74, //'a','p','p','l','i','c','a','t','i','o','n','/'
						0x69, 0x6f, 0x6e, 0x2f, //'o','c','t','e','t','-','s','t','r','e','a','m'
						0x6f, 0x63, 0x74, 0x65,
						0x74, 0x2d, 0x73, 0x74,
						0x72, 0x65, 0x61, 0x6d,
						0x00,					//spacer
						0x65, 0x76, 0x6f, 0x73, //metadata name/label
						0x74, 0x72, 0x65, 0x61, //'e','v','o','s','t','r','e','a','m','.'
						0x6d, 0x2e, 0x6a, 0x73, //'j','s','o','n'
						0x6f, 0x6e,
						0x00, 0x00				//spacer
					};

					uint32_t realId3Size = length + sizeof(frameHeader) + sizeof(metadataheader);
					uint32_t synchSafeSize = synchsafe(realId3Size);
					EHTONLP(ID3Header + 6, synchSafeSize); //set the ID3Header size to chunkSize + frame size + metadata header size
					uint32_t realFrameSize = length + sizeof(metadataheader);
					synchSafeSize = synchsafe(realFrameSize);
					EHTONLP(frameHeader + 4, synchSafeSize); //set the frameHeader size to chunkSize + metadata header size
				
					offset = chunkStart - id3Length;

					memcpy(_pPacket + offset, ID3Header, sizeof(ID3Header)); //pack the ID3 header
					offset += sizeof(ID3Header);

					memcpy(_pPacket + offset, frameHeader, sizeof(frameHeader)); //pack the frame header
					offset += sizeof(frameHeader);

					memcpy(_pPacket + offset, metadataheader, sizeof(metadataheader)); //pack the metadata header
					offset += sizeof(metadataheader);
				} else {
					offset = pesStart;
				}

				memcpy(_pPacket + offset, pBuffer + consumed, chunkSize); //pack the metadata
			}

			//12. Put the header
			if (!BuildPacketHeader(
				_pPacket, //pBuffer
				consumed == 0, //payloadStart
				pid, //pid
				hasAf, //hasAdaptationField
				true //hasPayload
				)) {
					FATAL("Unable to build packet header");
					return false;
			}
			//		FINEST("\n%s", STR(IOBuffer::DumpBuffer(_packet, 188)));
			//		sleep(1);

			//13. Write the packet
#ifdef DEBUG_DUMP_TS_HEADER
			DumpTS4BytesHeader(_pPacket);
#endif /* DEBUG_DUMP_TS_HEADER */
			if (!EncryptPacket(_pPacket, 188)) {
				FATAL("Unable to write packet");
				return false;
			}

			//14. advance the consumed
			consumed += chunkSize;
		}
		return true;
}

uint32_t BaseTSSegment::synchsafe(uint32_t realNumber) {
	uint32_t mask = 0x7F;
	uint32_t synchSafe = 0;
	while (mask ^ 0x7FFFFFFF) {
		synchSafe = realNumber & ~mask;
		synchSafe <<= 1;
		synchSafe |= realNumber & mask;
		mask = ((mask + 1) << 8) - 1;
		realNumber = synchSafe;
	}
	return synchSafe;
}

#ifdef DEBUG_DUMP_TS_HEADER

void BaseTSSegment::DumpTS4BytesHeader(uint8_t *pBuffer) {
	uint8_t sb = pBuffer[0];
	uint8_t tei = pBuffer[1] >> 7;
	uint8_t pusi = (pBuffer[1] >> 6)&0x01;
	uint8_t tp = (pBuffer[1] >> 5)&0x01;
	uint16_t outPid = (ENTOHSP(pBuffer + 1))&0x1fff;
	uint8_t sc = (pBuffer[3] >> 6)&0x03;
	uint8_t af = (pBuffer[3] >> 4)&0x03;
	uint8_t cc = (pBuffer[3])&0x0f;
	uint32_t all = ENTOHLP(pBuffer);
	string dbg = format("sb: %02"PRIx8"; tei: %"PRIu8"; pusi: %"PRIu8"; tp: %"PRIu8"; OPID: %5"PRIu16"; sc: %"PRIu8"; af: %"PRIu8"; cc: %2"PRIu8"; all: %"PRIx32,
		sb, tei, pusi, tp, outPid, sc, af, cc, all);
	if (pusi)
		WARN("%s", STR(dbg));
	else
		FINEST("%s", STR(dbg));
}
#endif /* DEBUG_DUMP_TS_HEADER */

uint64_t BaseTSSegment::GetFrameCount() {
	return _frameCount;
}
#endif  /* HAS_PROTOCOL_HLS */
