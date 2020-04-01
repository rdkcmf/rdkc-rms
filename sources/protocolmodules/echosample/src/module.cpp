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


//ffmpeg -re -i /Volumes/Storage/media/tbs.mp4 -vcodec copy -acodec pcm_mulaw -ar 8000 -ac 1 -f flv tcp://localhost:5555
//ffmpeg -re -i /Volumes/Storage/media/tbs.mp4 -vcodec copy -acodec libfaac -f flv tcp://localhost:5555

#define __STDC_FORMAT_MACROS
#include "module.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#ifdef WIN32
#include <Winsock2.h> //this is for htons,ntohs,etc
#ifndef snprintf
#define snprintf _snprintf
#endif /* snprintf */
#else
#include <netinet/in.h>
#endif /* WIN32 */

//minimal logging facilities
#define __LOG(fmt) do { fprintf(stdout,fmt);fprintf(stdout,"\n");}while(0)

//implementation for new/delete as macros
//needed because this example MUST be compilable with both C and C++ compilers
#ifdef __cplusplus
#define NEW(type,length) new type[length]
#define DELETE(type,p) delete[] (type *)(p)
#else /* __cplusplus */
#define NEW(type,length) (type *)malloc((length)*sizeof(type))
#define DELETE(type,p) free((type *)(p))
#endif /* __cplusplus */

#define PROTOCOL_TYPE_ECHO 0x4F4543484F000000LL //OECHO
#define MAX_STREAM_NAME 256

// - our pUserData structures

typedef struct connection_user_data_t {
	//this is always incremented and used to generate unique names for the streams
	uint32_t inStreamIndex;
	bool flvHeaderParsed;
} connection_user_data_t;

typedef struct in_stream_user_data_t {
	bool videoCodecIsSetup;
	bool audioCodecIsSetup;
} in_stream_user_data_t;

//local functions
//this one creates a stream on a connection. Internally, the black-magic is
//computing an unique stream name.
void createInStream(connection_t *pConnection);

void Init() {
	__LOG("Hello from Init function");
}

void GetProtocolFactory(protocol_factory_t *pFactory) {
	//1. Setup the events
	pFactory->events.lib.initCompleted = onLibInitCompleted;
	pFactory->events.connection.connectSucceeded = onConnectSucceeded;
	pFactory->events.connection.connectFailed = onConnectFailed;
	pFactory->events.connection.connectionAccepted = onConnectionAccepted;
	pFactory->events.connection.dataAvailable = onDataAvailable;
	pFactory->events.connection.outputBufferCompletelySent = onOutputBufferCompletelySent;
	pFactory->events.connection.connectionClosed = onConnectionClosed;
	pFactory->events.stream.inStreamCreated = onInStreamCreated;
	pFactory->events.stream.inStreamCreateFailed = onInStreamCreateFailed;
	pFactory->events.stream.inStreamClosed = onInStreamClosed;
	pFactory->events.stream.inStreamAttached = onInStreamAttached;
	pFactory->events.stream.inStreamDetached = onInStreamDetached;
	pFactory->events.stream.outStreamCreated = onOutStreamCreated;
	pFactory->events.stream.outStreamCreateFailed = onOutStreamCreateFailed;
	pFactory->events.stream.outStreamClosed = onOutStreamClosed;
	pFactory->events.stream.outStreamData = onOutStreamData;
	pFactory->events.stream.outStreamAttached = onOutStreamAttached;
	pFactory->events.stream.outStreamDetached = onOutStreamDetached;
	pFactory->events.job.timerCreated = onJobTimerCreated;
	pFactory->events.job.timerCreateFailed = onJobTimerCreateFailed;
	pFactory->events.job.timerClosed = onJobTimerClosed;
	pFactory->events.job.timerPulse = onJobTimerPulse;
	pFactory->events.rtmp.clientConnected = onRtmpClientConnected;
	pFactory->events.rtmp.clientDisconnected = onRtmpClientDisconnected;
	pFactory->events.rtmp.clientOutBufferFull = onRtmpClientOutBufferFull;
	pFactory->events.rtmp.requestAvailable = onRtmpRequestAvailable;
	pFactory->events.rtmp.responseAvailable = onRtmpResponseAvailable;

	//2. We are going to support only one protocol
	pFactory->supportedProtocolSize = 1;
	pFactory->pSupportedProtocols = NEW(uint64_t, pFactory->supportedProtocolSize);
	pFactory->pSupportedProtocols[0] = PROTOCOL_TYPE_ECHO;

	//2. We are going to support 1 chain of protocols
	pFactory->supportedPorotocolChainSize = 1;
	pFactory->pSupportedPorotocolChains = NEW(protocol_chain_t, pFactory->supportedPorotocolChainSize);
	pFactory->pSupportedPorotocolChains[0].pName = "echo";
	pFactory->pSupportedPorotocolChains[0].pSchema = "echo";
	pFactory->pSupportedPorotocolChains[0].chainSize = 2;
	pFactory->pSupportedPorotocolChains[0].pChain = NEW(uint64_t, pFactory->pSupportedPorotocolChains[0].chainSize);
	pFactory->pSupportedPorotocolChains[0].pChain[0] = PROTOCOL_TYPE_TCP;
	pFactory->pSupportedPorotocolChains[0].pChain[1] = PROTOCOL_TYPE_ECHO;
}

void ReleaseProtocolFactory(protocol_factory_t *pFactory) {
	DELETE(uint64_t, pFactory->pSupportedProtocols);
	pFactory->pSupportedProtocols = NULL;
	pFactory->supportedProtocolSize = 0;
	DELETE(uint64_t, pFactory->pSupportedPorotocolChains[0].pChain);
	DELETE(protocol_chain_t, pFactory->pSupportedPorotocolChains);
	pFactory->pSupportedPorotocolChains = NULL;
	pFactory->supportedPorotocolChainSize = 0;
	__LOG("Hello from ReleaseProtocolFactory function");
}

void Release() {
	__LOG("Hello from Release function");
}

void onLibInitCompleted(protocol_factory_t *pFactory) {
	__LOG("Hello from onLibInitCompleted function");
	pFactory->api.job.createTimer(pFactory, 2, (void *) 0x12345678);
}

void onConnectSucceeded(connection_t *pConnection) {
	__LOG("Hello from onConnectSucceeded function");
	if (pConnection->pPullInStreamUri != NULL)
		fprintf(stdout, "pullstream `%s` into stream name %s\n",
			pConnection->pPullInStreamUri,
			pConnection->pLocalStreamName);
	if (pConnection->pPushOutStreamUri != NULL) {
		fprintf(stdout, "pushstream %s to `%s`\n",
				pConnection->pLocalStreamName,
				pConnection->pPushOutStreamUri);
		pConnection->pFactory->api.stream.createOutStream(pConnection,
				pConnection->pLocalStreamName,
				NULL);
	}
}

void onConnectFailed(protocol_factory_t *pFactory, const char *pUri, const char *pIp, uint16_t port, void *pUserData) {
	__LOG("Hello from onConnectFailed function");
	DELETE(connection_user_data_t, pUserData);
}

void onConnectionAccepted(connection_t *pConnection) {
	__LOG("Hello from onConnectionAccepted function");
	//1. Initialize the pUserData structure
	pConnection->context.pUserData = NEW(connection_user_data_t, 1);
	memset(pConnection->context.pUserData, 0, sizeof ( connection_user_data_t));

	//2. Create a stream
	createInStream(pConnection);
}

uint32_t onDataAvailable(connection_t *pConnection, void *pData, uint32_t length) {
	//for simplicity, we assume that the stream has both tracks: audio and video
	//also, we also assume that audio is AAC and video is H264

	//1. first, make sure we have a stream
	if ((pConnection->inStreamsCount == 0) || (pConnection->ppInStreams == NULL)) {
		//bail out. This is bogus. We should have a stream
		//this code should NOT be executed in normal circumstances
		//but is here just for safety
		pConnection->pFactory->api.connection.closeConnection(pConnection, false);
		return length;
	}

	//2. Since we only have a stream per connection, we just get the
	//first stream from ppInStreams. If multiple streams are here,
	//we either need to cycle over the streams in ppInStreams to pick up
	//the proper stream name. Needless to say, optimizations can be made
	//for this search. We can do that by using various info from pUserData
	//from both the streams and connection. Right now, we keep it simple
	in_stream_t *pInStream = pConnection->ppInStreams[0];
	in_stream_user_data_t *pInStreamData = (in_stream_user_data_t *) pInStream->context.pUserData;

	//3. get the pUserData from the connection
	connection_user_data_t *pConnectionData = (connection_user_data_t *) pConnection->context.pUserData;

	//4. Start eating the FLV data now. First, check if we skipped the FLV header
	uint32_t consumed = 0;
	if (!pConnectionData->flvHeaderParsed) {
		if (length < 13) {
			//not enough data. Wait for more
			return consumed;
		}
		consumed += 13;
		pConnectionData->flvHeaderParsed = true;
	}

	uint8_t *pTemp = (uint8_t *) pData;

	//5. Cycle over FLV tags now
	while (true) {
		//6. do we have enough data to read the tag header?
		if (consumed + 11 > length) {
			return consumed;
		}

		//7. read the 24bits network order tag size
		uint32_t tagSize = ntohl(*((uint32_t *) (pTemp + consumed + 1)));
		tagSize = tagSize >> 8;

		//8. Do we have enough data to read an entire tag?
		if (consumed + 11 + tagSize + 4 > length) {
			return consumed;
		}

		//9. read the 32bits adobe-style order timestamp
		uint32_t timestamp = ntohl(*((uint32_t *) (pTemp + consumed + 4)));
		timestamp = (timestamp & 0xff) | (timestamp >> 8);

		//10. if this tag is not audio nor video, just skip it
		if ((pTemp[consumed] != 8) && (pTemp[consumed] != 9)) {
			consumed += 11 + tagSize + 4;
			continue;
		}
		if (tagSize > 0) {
			if (pTemp[consumed] == 8) {
				//11. Audio data coming in. First, check and see if we have
				//both codecs set up
				if (!pInStreamData->audioCodecIsSetup) {
					switch ((pTemp[consumed + 11] >> 4)) {
						case 10://aac
						{
							if (pTemp[consumed + 12] == 0) {
								pConnection->pFactory->api.stream.setupInStreamAudioCodecAAC(pInStream, pTemp + consumed + 13);
								pInStreamData->audioCodecIsSetup = true;
								__LOG("Audio codec ready");
							}
							break;
						}
						case 7: //G711 ALAW
						case 8: //G711 MULAW
						{
							pConnection->pFactory->api.stream.setupInStreamAudioCodecG711(pInStream);
							pInStreamData->audioCodecIsSetup = true;
							__LOG("Audio codec ready");
							break;
						}
						default:
						{
							__LOG("Audio codec not supported");
							return length;
						}
					}
				} else if (pInStreamData->videoCodecIsSetup) {
					//13. We have audio codecs, we have video codecs, now we can send the data
					if (!pConnection->pFactory->api.stream.feedInStream(pInStream, timestamp, timestamp,
							pTemp + consumed + 11, tagSize, true)) {
						__LOG("Feed failed");
						return length;
					}
				} else {
					//14. No video codec yet
					__LOG("No video codec yet");
				}
			} else {
				//11. Video data coming in. First, check and see if we have
				//both codecs set up
				if (!pInStreamData->videoCodecIsSetup) {
					//12. Wait for that special packet which has the video codec setup marker
					if (pTemp[consumed + 12] == 0) {
						uint32_t spsLength = ntohs(*((uint16_t *) (pTemp + consumed + 11 + 11)));
						uint8_t *pSPS = pTemp + consumed + 11 + 11 + 2;
						uint32_t ppsLength = ntohs(*((uint16_t *) (pSPS + spsLength + 1)));
						uint8_t *pPPS = pSPS + spsLength + 1 + 2;
						pConnection->pFactory->api.stream.setupInStreamVideoCodecH264(pInStream,
								pSPS, spsLength,
								pPPS, ppsLength);
						pInStreamData->videoCodecIsSetup = true;
						__LOG("Video codec ready");
					}
				} else if (pInStreamData->audioCodecIsSetup) {
					//13. We have audio codecs, we have video codecs, now we can send the data
					if (!pConnection->pFactory->api.stream.feedInStream(pInStream, timestamp, timestamp,
							pTemp + consumed + 11, tagSize, false)) {
						__LOG("Feed failed");
						return length;
					}
				} else {
					//14. No audio codec yet
					__LOG("No audio codec yet");
				}
			}
		}
		consumed += 11 + tagSize + 4;
	}
}

void onOutputBufferCompletelySent(connection_t *pConnection) {
	__LOG("Hello from onOutputBufferCompletelySent function");
}

void onConnectionClosed(connection_t *pConnection) {
	__LOG("Hello from onConnectionClosed function");
	DELETE(connection_user_data_t, pConnection->context.pUserData);
	pConnection->context.pUserData = NULL;
}

void onInStreamCreated(in_stream_t *pInStream) {
	__LOG("Hello from onInStreamCreated function");
	//3. create the userdata for the new stream
	pInStream->context.pUserData = NEW(in_stream_user_data_t, 1);
	memset(pInStream->context.pUserData, 0, sizeof ( in_stream_user_data_t));
}

void onInStreamCreateFailed(connection_t *pConnection, const char *pUniqueName, void *pUserData) {
	__LOG("Hello from onInStreamCreateFailed function");
}

void onInStreamClosed(in_stream_t *pInStream) {
	__LOG("Hello from onInStreamClosed function");
	DELETE(in_stream_user_data_t, pInStream->context.pUserData);
	pInStream->context.pUserData = NULL;
}

void onInStreamAttached(struct in_stream_t *pInStream, uint32_t outStreamId, uint32_t outStreamProtocolId) {
	/*
	 * public CallTestFunction(s:String, n:Number, o:Object):Object
	 * {
	 *		var result:Object={name_s:s, name_n:n, name_o:o};
	 *		return result;
	 * }
	 */
	__LOG("Hello from onInStreamAttached function");
	//1. Prepare the message

	//variant_t *pMessage = pInStream->pFactory->api.variant.createRtmpRequest("CallTestFunction");
	//variant_t *pMessage = pInStream->pFactory->api.variant.createRtmpRequest("CallTestProcedure");

	//2. Prepare the parameters
	//variant_t *pParam1 = pInStream->pFactory->api.variant.create();
	//pInStream->pFactory->api.variant.setString(pParam1, "Hello world!", 0);

	//variant_t *pParam2 = pInStream->pFactory->api.variant.create();
	//pInStream->pFactory->api.variant.setDouble(pParam2, 123.456, 0);

	//variant_t *pParam3 = pInStream->pFactory->api.variant.create();
	//pInStream->pFactory->api.variant.setString(pParam3, "another text", 3, "complex", "object", "string");
	//pInStream->pFactory->api.variant.setDouble(pParam3, 123.456, 3, "complex", "object", "number");
	//pInStream->pFactory->api.variant.setI8(pParam3, -3, 2, "complex", "another sibling");
	//pInStream->pFactory->api.variant.pushString(pParam3, "some string", 2, "complex", "an array");
	//pInStream->pFactory->api.variant.pushDouble(pParam3, 123.456, 2, "complex", "an array");
	//pInStream->pFactory->api.variant.pushBool(pParam3, true, 2, "complex", "an array");
	//pInStream->pFactory->api.variant.pushUI32(pParam3, 0xdeadbeaf, 2, "complex", "an array");
	/*
	 *	complex
	 *		object
	 *			string: "another text"
	 *			number: 123.456
	 *		another sibling: -3
	 *		an array:
	 *			0 -> "some string"
	 *			1 -> 123.456
	 *			2 -> true
	 *			3 -> 0xdeadbeaf
	 */


	//3. Add the parameters to the request
	//pInStream->pFactory->api.variant.addRtmpRequestParameter(pMessage, &pParam1, true);
	//pInStream->pFactory->api.variant.addRtmpRequestParameter(pMessage, &pParam2, true);
	//this one we manually release just to exemplify the usage of the release parameter
	//pInStream->pFactory->api.variant.addRtmpRequestParameter(pMessage, &pParam3, false);

	//4. Do the call
	/*pInStream->pFactory->api.rtmp.sendMessage(
			pInStream->pFactory,
			outStreamProtocolId,
			pMessage,
			true
			);*/

	//5. Cleanup. It doesn't matter the order in which we deallocate ANY variants
	//The only rule is to deallocate all variants created through
	//create,copy or createRtmpRequest variant functions
	//pInStream->pFactory->api.variant.release(pMessage);
	//pInStream->pFactory->api.variant.release(pParam3);
}

void onInStreamDetached(struct in_stream_t *pInStream, uint32_t outStreamId, uint32_t outStreamProtocolId) {
	__LOG("Hello from onInStreamDetached function");
}

void onOutStreamCreated(out_stream_t *pOutStream) {
	__LOG("Hello from onOutStreamCreated function");
}

void onOutStreamCreateFailed(connection_t *pConnection, const char *pInStreamName, void *pUserData) {
	__LOG("Hello from onOutStreamCreateFailed function");
}

void onOutStreamClosed(out_stream_t *pOutStream) {
	__LOG("Hello from onOutStreamClosed function");
}

void onOutStreamData(struct out_stream_t *pOutStream, uint64_t pts, uint64_t dts, void *pData, uint32_t length, bool isAudio) {
	fprintf(stdout, "%c %"PRIu64"\n", isAudio ? 'A' : 'V', pts);
	//__LOG("Hello from onOutStreamData function");
}

void onOutStreamAttached(struct out_stream_t *pOutStream, uint32_t inStreamId, uint32_t inStreamProtocolId) {
	__LOG("Hello from onOutStreamAttached function");
}

void onOutStreamDetached(struct out_stream_t *pOutStream, uint32_t inStreamId, uint32_t inStreamProtocolId) {
	__LOG("Hello from onOutStreamDetached function");
}

void onJobTimerCreated(jobtimer_t *pJobTimer) {
	__LOG("Hello from onJobTimerCreated function");
}

void onJobTimerCreateFailed(protocol_factory_t *pFactory, void *pUserData) {
	__LOG("Hello from onJobTimerCreateFailed function");
}

void onJobTimerClosed(jobtimer_t *pJobTimer) {
	__LOG("Hello from onJobTimerClosed function");
}

void onJobTimerPulse(jobtimer_t *pJobTimer) {
	//__LOG("Hello from onJobTimerPulse function");
}

void onRtmpClientConnected(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pConnectRequest) {
	__LOG("Hello from onRtmpClientConnected function");
	char *pTemp = NEW(char, 16384);
	printf("CONNECT REQUEST (%u) \n%s\n", rtmpConnectionId, pFactory->api.variant.getXml(pConnectRequest, pTemp, 16384, 0));
	DELETE(char, pTemp);
}

void onRtmpClientDisconnected(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId) {
	__LOG("Hello from onRtmpClientDisconnected function");
}

void onRtmpRequestAvailable(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pRequest) {
	__LOG("Hello from onRtmpRequestAvailable function");
	char *pTemp = NEW(char, 16384);
	printf("REQUEST\n%s\n", pFactory->api.variant.getXml(pRequest, pTemp, 16384, 0));
	DELETE(char, pTemp);
}

void onRtmpClientOutBufferFull(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, uint32_t maximumAmount, uint32_t currentAmount) {
        __LOG("Hello from onRtmpClientOutBufferFull");
}

void onRtmpResponseAvailable(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pRequest, variant_t *pResponse) {
	__LOG("Hello from onRtmpResponseAvailable function");
	char *pTemp = NEW(char, 16384);
	printf("REQUEST\n%s\n", pFactory->api.variant.getXml(pRequest, pTemp, 16384, 0));
	printf("RESPONSE\n%s\n", pFactory->api.variant.getXml(pResponse, pTemp, 16384, 0));
	DELETE(char, pTemp);
}

void createInStream(connection_t *pConnection) {
	//1. Get our hands on the pUserData which is a the locally administered structure
	//connection_user_data_t *pConnectionData = (connection_user_data_t *) pConnection->context.pUserData;

	//2. compute the stream name
	char *pName = NEW(char, MAX_STREAM_NAME);
	memset(pName, 0, MAX_STREAM_NAME);
	snprintf(pName, MAX_STREAM_NAME - 1, "test555"); //pConnection->uniqueId, pConnectionData->inStreamIndex++);

	//3. create the stream
	pConnection->pFactory->api.stream.createInStream(pConnection, pName, NULL);

	//4. cleanup the stream name.
	DELETE(char, pName);
}
