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
#ifndef _BASESEGMENT_H
#define	_BASESEGMENT_H

#include "common.h"
#include "streaming/streamcapabilities.h"

enum PID_TYPE {
	audio = 1,
	video,
	metadata
};

class DLLEXP BaseSegment {
protected:
	uint16_t _audioPid;
	IOBuffer &_audioBuffer;
	StreamCapabilities *_pCapabilities;

	//Encryption stuff
protected:
	string _drmType;
	EVP_CIPHER_CTX* _ctxEncrypt;
	uint8_t* _pKey;
	uint8_t* _pIV;
private:
	char *_pCipherBuf;
	int _cipherSize;
	int _plainSize;

	Variant _startTime;
public:
	BaseSegment(IOBuffer &audioBuffer, string const& drmType);
	virtual ~BaseSegment();

	virtual bool Init(Variant &settings, StreamCapabilities *pCapabilities,
		unsigned char *pKey = NULL, unsigned char *pIV = NULL) = 0;
	bool PushAudioData(IOBuffer &buffer, int64_t pts, int64_t dts);
	virtual bool Init(StreamCapabilities *pCapabilities, unsigned char *pKey = NULL, unsigned char *pIV = NULL) = 0;
	bool FlushPacket();
	bool CreateKey(unsigned char * pKey, unsigned char * pIV);
	Variant GetStartTime();
protected:
	virtual uint16_t AddStream(PID_TYPE pidType) = 0;
	virtual bool WritePayload(uint8_t *pBuffer, uint32_t length, uint16_t pid, int64_t pts, int64_t dts) = 0;
	bool EncryptPacket(uint8_t *pBuffer, uint32_t size);
	virtual bool WritePacket(uint8_t *pBuffer, uint32_t size) = 0;
	char *EncryptBuffer(char *pPlainBuf, uint32_t *pSize); //size: in=plainBuf size, out=cipherBuf size
	char *FinalEncryptBuffer(uint32_t *pSize);
	bool EncryptSample(uint8_t* srcBuffer, uint16_t srcSize, uint8_t*& destBuffer, uint32_t& destSize, bool isAudio); //SAMPLE-AES encrypt function
private:
	void ApplyEPB(char*& cipherBuf, int cipherSize, uint8_t* const& pattern, uint32_t& newCipherSize);
};
#endif	/* _BASESEGMENT_H */
#endif  /* HAS_PROTOCOL_HLS */
