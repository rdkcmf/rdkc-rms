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
#include "streaming/hls/basesegment.h"
#include "streaming/nalutypes.h"
#include "streaming/codectypes.h"
#include "protocols/drm/drmdefines.h"
#define H264_START_CODE_SIZE	3

BaseSegment::BaseSegment(IOBuffer &audioBuffer, string const& drmType)
: _audioBuffer(audioBuffer) {
	_audioBuffer.IgnoreAll();
	_audioPid = 0;

	_drmType = drmType;
	_pCipherBuf = NULL;
	_cipherSize = 0;
	_plainSize = 0;
	if (_drmType != "" && _drmType != DRM_TYPE_SAMPLE_AES){
		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		memset(_ctxEncrypt, 0, sizeof (EVP_CIPHER_CTX));
		EVP_CIPHER_CTX_init(_ctxEncrypt);
		#else
		_ctxEncrypt = EVP_CIPHER_CTX_new();
		#endif
	}

	_startTime = Variant::Now();
	_pKey = NULL;
	_pIV = NULL;
}

BaseSegment::~BaseSegment() {
	if (_drmType != "") {
		if (_pCipherBuf != NULL) {
			delete[] _pCipherBuf;
			_pCipherBuf = NULL;
		}
		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		EVP_CIPHER_CTX_cleanup(_ctxEncrypt);
		#else
		EVP_CIPHER_CTX_free(_ctxEncrypt);
		#endif
	}
}

bool BaseSegment::PushAudioData(IOBuffer &buffer, int64_t pts, int64_t dts) {
	if (_audioPid == 0)
		return true;
	if (GETAVAILABLEBYTESCOUNT(buffer) < 3) {
		WARN("Invalid audio data");
		return true;
	}
	uint8_t *pBuffer = NULL;
	uint32_t length = 0;
	if ((ENTOHSP(GETIBPOINTER(buffer)) >> 4) != 0x0fff) {
		//		if ((_pCapabilities == NULL)
		//				|| (_pCapabilities->audioCodecId != CODEC_AUDIO_AAC)) {
		//			WARN("Invalid audio codec");
		//			return true;
		//		}
#ifdef HAS_G711
		uint64_t audioCodecType = 0;
		audioCodecType = _pCapabilities->GetAudioCodecType();

		if (audioCodecType == CODEC_AUDIO_AAC) {
#endif	/* HAS_G711 */
			BitArray adts;
			// build adts header
			adts.PutBits<uint16_t > (0xFFF, 12); // syncword
			adts.PutBits<uint8_t > (0, 1); // id
			adts.PutBits<uint8_t > (0, 2); // layer
			adts.PutBits<uint8_t > (1, 1); // protection_absent
			adts.PutBits<uint8_t > (1, 2); // profile ///**************
			adts.PutBits<uint8_t > (7, 4); // sampling_frequency_index /**********************
			adts.PutBits<uint8_t > (0, 1); // private
			adts.PutBits<uint8_t > (2, 3); // channel_configuration /**********************
			adts.PutBits<uint8_t > (0, 1); // original
			adts.PutBits<uint8_t > (0, 1); // home
			adts.PutBits<uint8_t > (0, 1); // copyright_id
			adts.PutBits<uint8_t > (0, 1); // copyright_id_start
			adts.PutBits<uint16_t > ((uint16_t) (GETAVAILABLEBYTESCOUNT(buffer) + 7), 13); // aac_frame_length = datalength + 7 byte header
			adts.PutBits<uint16_t > (0x7FF, 11); // adts_buffer_fullness
			adts.PutBits<uint8_t > (0, 2); // num_raw_data_blocks
			_audioBuffer.IgnoreAll();
			_audioBuffer.ReadFromBuffer(GETIBPOINTER(adts), GETAVAILABLEBYTESCOUNT(adts));
			_audioBuffer.ReadFromBuffer(GETIBPOINTER(buffer), GETAVAILABLEBYTESCOUNT(buffer));
			pBuffer = GETIBPOINTER(_audioBuffer);
			length = GETAVAILABLEBYTESCOUNT(_audioBuffer);
#ifdef HAS_G711
		} else if ((audioCodecType & CODEC_AUDIO_G711) == CODEC_AUDIO_G711) {
			pBuffer = GETIBPOINTER(buffer);
			length = GETAVAILABLEBYTESCOUNT(buffer);
		}
#endif	/* HAS_G711 */
	} else {
		pBuffer = GETIBPOINTER(buffer);
		length = GETAVAILABLEBYTESCOUNT(buffer);
	}

	if (!WritePayload(
			pBuffer, //pBuffer,
			length, //length
			_audioPid, //pid
			pts, //presentation timestamp
			dts //decoding timestamp
			)) {
		FATAL("Unable to write ADTS");
		return false;
	}

	return true;
}

bool BaseSegment::FlushPacket() {
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

bool BaseSegment::CreateKey(unsigned char * pKey, unsigned char * pIV) {
	int rounds = 5;

	//Gen key & IV for AES 128 CBC mode. A SHA1 digest is used to hash the supplied key material.
	//rounds is the number of times the we hash the material. More rounds are more secure but slower.
	unsigned char pTempKey[16];
	memset(pTempKey, 0, 16);

	if (RAND_bytes(&pTempKey[0], 16) == 0) {
		FATAL("Unable to generate random key");
		return false;
	}

	//Generate key and iv
	if (16 != EVP_BytesToKey(EVP_aes_128_cbc(), EVP_sha1(), NULL, pTempKey, 16, rounds, pKey, pIV)) {
		FATAL("Error on generating key and iv. Key size should be 128 bits");
		return false;
	}

	return true;
}

bool BaseSegment::EncryptPacket(uint8_t *pBuffer, uint32_t size) {
	if (_drmType == "" || _drmType == DRM_TYPE_SAMPLE_AES)
		return WritePacket(pBuffer, size);
	uint32_t newSize = size;
	char *pCipherBuf = EncryptBuffer((char *) pBuffer, &newSize);
	if (pCipherBuf == NULL) {
		FATAL("Unable to encrypt packet");
		return false;
	}
	return WritePacket((uint8_t *) pCipherBuf, newSize);
}

char *BaseSegment::EncryptBuffer(char *pPlainBuf, uint32_t *pSize) {
	//Allow enough space in output buffer for additional block
	if (_pCipherBuf == NULL) {
		_plainSize = (int) *pSize;
		_pCipherBuf = new char[_plainSize + (2 * AES_BLOCK_SIZE)];
		//INFO("Cipher buffer sized to %d", _plainSize + (2 * AES_BLOCK_SIZE));
	}
	//Resize output buffer if required
	if (*pSize > (uint32_t) _plainSize) {
		_plainSize = (int) *pSize;
		delete[] _pCipherBuf;
		_pCipherBuf = new char[_plainSize + (2 * AES_BLOCK_SIZE)];
		//INFO("Cipher buffer resized to %d", _plainSize + (2 * AES_BLOCK_SIZE));
	} else {
		_plainSize = (int) *pSize;
	}

	if (!EVP_EncryptUpdate(_ctxEncrypt, (unsigned char*) _pCipherBuf, &_cipherSize,
			(unsigned char*) pPlainBuf, _plainSize)) {
		FATAL("Error on executing EVP_EncryptUpdate");
		return NULL;
	}
	//INFO("Encryption count=%d bytes.", _cipherSize);
	*pSize = (uint32_t) _cipherSize;
	return _pCipherBuf;
}

char *BaseSegment::FinalEncryptBuffer(uint32_t *pSize) {
	int finalSize = 0;

	if (_pCipherBuf == NULL) {
		FATAL("Cypher buffer not initialized");
		return NULL;
	}
	//Point after last encrypted data where final data, including padding, will be written
	if (!EVP_EncryptFinal_ex(_ctxEncrypt, (unsigned char*) _pCipherBuf + _cipherSize, &finalSize)) {
		FATAL("Error on executing EVP_EncryptFinal_ex");
		return NULL;
	}
	//INFO("Final encryption offset=%d, count=%d bytes.", _cipherSize, finalSize);
	*pSize = (uint32_t) finalSize;
	return _pCipherBuf + _cipherSize;
}

Variant BaseSegment::GetStartTime() {
	return _startTime;
}

bool BaseSegment::EncryptSample(uint8_t* srcBuffer, uint16_t srcSize, uint8_t*& destBuffer, uint32_t& destSize, bool isAudio) {
	char *pCipherBuf = new char[srcSize + AES_BLOCK_SIZE];
	int cipherSize;
	if (!EVP_EncryptUpdate(_ctxEncrypt, (unsigned char*) pCipherBuf, &cipherSize,
	(unsigned char*) srcBuffer, srcSize)) {
		FATAL("Error on executing EVP_EncryptUpdate");
		return false;
	}

	uint32_t finalCipherSize = cipherSize;

	if (!isAudio) {
		//search pCipherBuf for 3 byte start codes like sequence (0x000000, 0x000001, 0x000002, 0x000003)
		//and apply emulation prevention byte for the said sequence (p.65 T-REC-H.264-201304-S!!PDF-E.pdf)
		uint8_t W1[H264_START_CODE_SIZE] = { 0x00, 0x00, 0x00 };
		uint8_t W2[H264_START_CODE_SIZE] = { 0x00, 0x00, 0x01 };
		uint8_t W3[H264_START_CODE_SIZE] = { 0x00, 0x00, 0x02 };
		uint8_t W4[H264_START_CODE_SIZE] = { 0x00, 0x00, 0x03 };
		//1. Search for 0x000000 sequence
		ApplyEPB(pCipherBuf, cipherSize, W1, finalCipherSize);
		//2. Search for 0x000001 sequence
		ApplyEPB(pCipherBuf, cipherSize, W2, finalCipherSize);
		//3. Search for 0x000002 sequence
		ApplyEPB(pCipherBuf, cipherSize, W3, finalCipherSize);
		//4. Search for 0x000003 sequence
		ApplyEPB(pCipherBuf, cipherSize, W4, finalCipherSize);
		if (srcSize > finalCipherSize) {
			FATAL("Encrypted block must not be less than the non-encrypted block, no compression here");
			return false;
		}
	}

	destBuffer = (uint8_t*)pCipherBuf;
	destSize = finalCipherSize;

	return true;
}

void BaseSegment::ApplyEPB(char*& cipherBuf, int cipherSize, uint8_t* const& pattern, uint32_t& newCipherSize) {
	//Knuth-Morris_Pratt_algorithm to search for sequence - https://en.wikipedia.org/wiki/Knuth%E2%80%93Morris%E2%80%93Pratt_algorithm
	uint8_t m = 0;
	uint8_t i = 0;
	struct HitsInfo {
		uint8_t hits;
		uint8_t mVal;
		HitsInfo() :
			hits(0), mVal(0) {}
	};
	HitsInfo codeHits[H264_START_CODE_SIZE + 1];

	while (true) {
		size_t remainingSize = cipherSize - m;
		if (remainingSize < H264_START_CODE_SIZE)
			break;
		if (cipherBuf[m + i] == pattern[i]) {
			++codeHits[pattern[i]].hits;
			codeHits[pattern[i]].mVal = m + i;
			if (i == H264_START_CODE_SIZE) {
				//insert EPB (0x03) unto cipherBuf[m + (i - 2)]
				memcpy(cipherBuf + m + 3, cipherBuf + m + 2, cipherSize - (m + 3));
				cipherBuf[m + 2] = 0x03;
				++newCipherSize;
				m += i;
				i = 0;
			}
			else {
				++i;
			}
		}
		else {
			if (remainingSize == H264_START_CODE_SIZE)
				break;
			if (i > 0)
				m += i;
			else
				++m;

			i = 0;
			for (uint8_t j = 0; j <= H264_START_CODE_SIZE; ++j) {
				if (codeHits[j].hits >= 2) {
					if (j == 0)
						m = codeHits[j].mVal;
					i = j + 1;
				}
				codeHits[j].hits = 0;
				codeHits[j].mVal = 0;
			}
		}
	}
}
#endif  /* HAS_PROTOCOL_HLS */
