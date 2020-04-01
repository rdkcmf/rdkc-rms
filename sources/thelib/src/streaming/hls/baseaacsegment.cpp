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
#include "streaming/hls/baseaacsegment.h"
#include "streaming/nalutypes.h"
#include "streaming/codectypes.h"
#include "protocols/drm/drmdefines.h"

BaseAACSegment::BaseAACSegment(IOBuffer &audioBuffer, string const& drmType)
	: BaseSegment(audioBuffer, drmType) {
}

BaseAACSegment::~BaseAACSegment() {	
}

bool BaseAACSegment::Init(StreamCapabilities *pCapabilities, unsigned char *pKey, unsigned char *pIV) {
	_pCapabilities = pCapabilities;

	bool hasAudio = (_pCapabilities->GetAudioCodecType() == CODEC_AUDIO_AAC);
	bool hasVideo = (_pCapabilities->GetVideoCodecType() == CODEC_VIDEO_H264);
	if (!(hasAudio || hasVideo)) {
		FATAL("In stream is not transporting H.264/AAC content");
		return false;
	}

	//Add audio/video streams
	if (hasAudio) {
		AddStream(audio);
	}

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

	return true;
}

uint16_t BaseAACSegment::AddStream(PID_TYPE pidType) {
	uint16_t result = 0;
	if (pidType == audio) {
		_audioPid = result = 257;
	} 
	return result;
}

bool BaseAACSegment::WritePayload(uint8_t *pBuffer,
	uint32_t length, uint16_t pid, int64_t pts, int64_t dts) {
		if (_drmType == DRM_TYPE_SAMPLE_AES) {
			bool encryptNal = true;
			if (length < 39)
				encryptNal = false;
			if (encryptNal) {
				uint8_t*  nalUnit = new uint8_t[length];
				memcpy(nalUnit, pBuffer, length);
				uint32_t bytesRemaining = 0;
				uint32_t blockCounter = 0;
				uint32_t nalUnitCurrentOffset = 0;
				uint8_t* encryptedBlock = NULL;
				uint32_t encryptedSize = 0;
				bytesRemaining = length - 23; //7 bytes ADTS header + 16 bytes accdg. to specs.

				if ((_pKey == NULL)
					|| (_pIV == NULL)
					|| (EVP_EncryptInit_ex(_ctxEncrypt, EVP_aes_128_cbc(), NULL, _pKey, _pIV) != 1)) {
					FATAL("Unable to setup encryption");
					return false;
				}

				while (bytesRemaining >= AES_BLOCK_SIZE) {
					++blockCounter;
					nalUnitCurrentOffset = 23 + (blockCounter - 1) * AES_BLOCK_SIZE;
					if (!EncryptSample(nalUnit + nalUnitCurrentOffset, AES_BLOCK_SIZE, encryptedBlock, encryptedSize, true)) {
						FATAL("Encryption of sample failed");
						return false;
					}
					memcpy(nalUnit + nalUnitCurrentOffset, encryptedBlock, AES_BLOCK_SIZE);
					bytesRemaining -= AES_BLOCK_SIZE;
				}

				memcpy(pBuffer, nalUnit, length);
				delete[] nalUnit;
			}
		}
		if (!EncryptPacket(pBuffer, length)) {
			FATAL("Unable to write packet");
			return false;
		}
		return true;
}

#endif  /* HAS_PROTOCOL_HLS */
