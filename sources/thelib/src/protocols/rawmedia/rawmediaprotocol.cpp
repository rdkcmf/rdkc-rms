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

#ifdef HAS_PROTOCOL_RAWMEDIA
#include "application/baseclientapplication.h"
#include "protocols/rawmedia/rawmediaprotocol.h"
#include "protocols/rawmedia/streaming/innetrawstream.h"
#include "streaming/streamstypes.h"
#include "streaming/codectypes.h"

#define MAX_BUFFER_THRESHOLD 1024 * 128
#define FLAG_PACKET_TYPE_CONFIG 0x80
#define FLAG_STREAM_CONFIG 0x40
#define FLAG_MEDIA_TYPE_VIDEO 0x0
#define FLAG_MEDIA_TYPE_AUDIO 0x1
//#define FLAG_PACKET_TYPE_DATA 0x02
#define FLAG_PACKET_TYPE_DATA 0x00

//#define FLAG_CONFIG (FLAG_PACKET_TYPE_CONFIG | FLAG_STREAM_CONFIG | FLAG_MEDIA_TYPE_VIDEO | FLAG_MEDIA_TYPE_AUDIO | FLAG_PACKET_TYPE_DATA)
#define FLAG_CONFIG (FLAG_PACKET_TYPE_CONFIG | FLAG_STREAM_CONFIG)
#define RESP_OK 0x00
#define RESP_STREAMNAME_TAKEN 0x01
#define FLAG_AUDIO_TYPE_CONFIG 0x06
#define FLAG_AUDIO_TYPE_AAC 0x00
#define FLAG_AUDIO_TYPE_G711 0x02

RawMediaProtocol::RawMediaProtocol()
		:BaseProtocol(PT_RAW_MEDIA){
	_pStream = NULL;
}
RawMediaProtocol::~RawMediaProtocol() {
	if(_pStream != NULL) {
		INFO("Deleting _pStream\n");
		delete _pStream;
		_pStream = NULL;
	}

}

bool RawMediaProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_TCP) || (type == PT_UDP) || (type == PT_UDS) || (type == PT_API_INTEGRATION);
}
bool RawMediaProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

IOBuffer * RawMediaProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outputBuffer) > 0)
		return &_outputBuffer;

	return NULL;
}

bool RawMediaProtocol::SignalInputData(int32_t recvAmount) {
	NYIR;
}
bool RawMediaProtocol::SignalInputData(IOBuffer &buffer) {
	uint8_t *pData = GETIBPOINTER(buffer);
	uint32_t dataLength = GETAVAILABLEBYTESCOUNT(buffer) - 4;
	
	// Get the expected length and see if we need to wait for the rest
	uint32_t expectedLength = ENTOHLP(pData);
	if (expectedLength > dataLength) {
		return true;
	}
	
	dataLength = expectedLength; // only read what is expected
	pData += 4; // skip the expected length

	uint8_t configType = (pData[0] & FLAG_CONFIG);
//#ifdef UDS_DEBUG
//	FATAL("pdata: %"PRIu8", configType: %"PRIu8, pData[0], configType);
//#endif
	//FINE("RawMediaProtocol::SignalInputData pData[0]: %x configType: %x FLAG_CONFIG %x\n", pData[0], configType, FLAG_CONFIG);
	/*if ( (configType == FLAG_PACKET_TYPE_DATA) ||
	     (configType == FLAG_MEDIA_TYPE_VIDEO) ||
	     (configType == FLAG_MEDIA_TYPE_AUDIO) ) {*/
	if ( (configType == FLAG_PACKET_TYPE_DATA) || (pData[0] == FLAG_MEDIA_TYPE_AUDIO)) {

		if (_pStream == NULL) { // no stream yet
			WARN("No existing stream yet");
			buffer.IgnoreAll();
			return true;
		}
		// audio or video ?
		bool isAudio = (pData[0] & FLAG_MEDIA_TYPE_AUDIO);
		//FINE("RawMediaProtocol::SignalInputData pData[0]: %d isAudio: %d\n", pData[0], isAudio);
	
		pData++;
		dataLength--;
//#ifdef UDS_DEBUG
//		FATAL("[Debug] Got %s data", isAudio ? "audio" : "video");
//#endif
		// get timestamps (value will be in microseconds)
		uint64_t rawTs = 0;
		for (uint32_t i = 0; i < 8; i++) {
			rawTs = rawTs << 8;
			rawTs |= pData[i];
		}

		double ts = (double) rawTs / 1000; // normalize to milliseconds
//#ifdef UDS_DEBUG
//		FATAL("[Debug] ts = %.3f", ts);
//#endif
		pData += 8;
		dataLength -= 8;
		
		_pStream->FeedData(pData, dataLength, ts, isAudio);
	} else if (configType == FLAG_PACKET_TYPE_CONFIG) {
		if (_pStream == NULL) { // no stream yet
			WARN("No existing stream yet");
			buffer.IgnoreAll();
			return true;
		}
		bool isAudio = (pData[0] & FLAG_MEDIA_TYPE_AUDIO);
		
#ifdef UDS_DEBUG
		FATAL("[Debug] Got %s config", isAudio ? "audio" : "video");
#endif

		if (isAudio) {
			// 0x00 - AAC
			// 0x02 - G711
			uint64_t codec = CODEC_AUDIO_AAC;
			if ((pData[0] & FLAG_AUDIO_TYPE_CONFIG) == FLAG_AUDIO_TYPE_G711) {
				codec = CODEC_AUDIO_G711;
			}
			
			pData++;
			dataLength--;
			INFO("RawMediaProtocol::SignalInputData codec--------> %d dataLength-----------> %d\n", codec, dataLength);
			_pStream->SetAudioConfig(codec, pData, dataLength);
		} else {
			WARN("Video config not handled.");
		}
	} else if (configType == FLAG_STREAM_CONFIG) { // stream config flag is on
		// check _pStream
		if (_pStream != NULL) { // already existing
			WARN("Protocol has an existing stream. Ignoring config");
			buffer.IgnoreAll();
			return true;
		}

		// gather streamname
		pData++;
		dataLength--;
		string streamname((char *) pData, dataLength);
#ifdef UDS_DEBUG
		FATAL("streamName: %s", STR(streamname));
#endif
		// check with streams manager
		BaseClientApplication *pApp = GetApplication();
		if (pApp == NULL){
			buffer.IgnoreAll();
			return false;
		}
		StreamsManager *pSM = pApp->GetStreamsManager();
		if (pSM == NULL) {
			buffer.IgnoreAll();
			return true;
		}
		uint8_t responseCode = RESP_OK;
		map<uint32_t, BaseStream *> streamResults = pSM->FindByName(streamname);
		if (streamResults.size() > 0) { // already existing
			FOR_MAP(streamResults, uint32_t, BaseStream *, i) {
				if (TAG_KIND_OF(MAP_VAL(i)->GetType(), ST_IN)) {
					WARN("Stream name already exist. Cannot create stream");
					responseCode = RESP_STREAMNAME_TAKEN;
					break;
				}
			}
		} else {
			// create stream
			_pStream = new InNetRawStream(this, streamname);
			_pStream->SetStreamsManager(pSM);
		}
		_outputBuffer.ReadFromByte(responseCode);
#ifdef UDS_DEBUG
		FATAL("Response code: %"PRIu8, responseCode);
#endif
		buffer.Ignore(expectedLength + 4); // ignore the read data and length header
		return EnqueueForOutbound();
	} else {
		INFO("Unknown packets");
//		return false;
	}
	
	buffer.Ignore(expectedLength + 4); // ignore the read data and length header
	return true;
}
#endif	/* HAS_PROTOCOL_RAWMEDIA */

