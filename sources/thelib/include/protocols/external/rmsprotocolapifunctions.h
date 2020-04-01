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

#ifndef _RMSPROTOCOLAPIFUNCTIONS_H
#define	_RMSPROTOCOLAPIFUNCTIONS_H


//forward declarations
struct variant_t;
struct protocol_factory_t;
struct connection_t;
struct in_stream_t;
struct out_stream_t;
struct jobtimer_t;

//API function pointers
/*
 * This set of function can be called be the module to accomplish various things
 *
 * 1. apiConnect_f
 * This function can be called to open a new connection to a distant server.
 * The new connection will be over TCP. In the future, it will be enhanced
 * to also support UDP connections.
 * PARAMETERS:
 *		-- pFactory - needs to be supplied and MUST be the pointer
 *		   previously got via libGetProtocolFactory_f call
 *		-- pIp - the string representation of the IP we want to connect to.
 *		   It is a string representation because it will be enhanced to support
 *		   host names in the future
 *		-- port - the destination port
 *		-- pProtocolChainName - the name of the protocol stack that needs
 *		   to be constructed for the new connection. This should be one
 *		   of the vales exported via protocol_factory_t::pSupportedPorotocolChains::pName
 *		-- pUserData - pointer to arbitrary data. This pointer will be stored inside the
 *		   context of the new connection and can be used to associate custom
 *		   data with the new connection. It can be NULL
 * GENERATED EVENTS:
 *		-- eventConnectSucceeded_f - connection successfully created
 *		-- eventConnectFailed_f - connection failed
 *
 * 2. apiSendData_f
 * This function can be used to send data. After this call is finished, it is
 * OK to discard pData because is already mirrored on the output buffer
 * PARAMETERS:
 *		-- pConnection - pointer to the connection we want to send data on
 *		-- pData - pointer to the data we want to send
 *		-- length - the length of the pData memory region expressed in bytes
 * GENERATED EVENTS:
 *		-- eventOutputBufferCompletelySent_f - when all outstanding output data
 *		   was successfully sent. It is NOT guaranteed that for each apiSendData_f
 *		   an eventOutputBufferCompletelySent_f is going to be triggered. However,
 *		   if this behavior is needed, than use apiSendData_f inside
 *		   eventConnectSucceeded_f or eventConnectionAccepted_f and after that
 *		   only inside eventOutputBufferCompletelySent_f. This basically creates
 *		   a reaction chain. Particularly useful when we want to stream a file
 *		   because files don't have events
 *		-- eventConnectionClosed_f - connection was terminated because of a
 *		   transfer error or the distant party explicitly closed the connection
 *		   due to malformed data or business rules
 *
 * 3. apiCloseConnection_f
 * This function is going to terminate a connection
 * PARAMETERS:
 *		-- pConnection - the connection that needs to be terminated
 *		-- gracefully - it can have 2 values. If it is 1, all the output data
 *		   is send and only after the connection is terminated. If 0, the connection
 *		   is terminated immediately. In both cases, no more input data is accepted
 * GENERATED EVENTS:
 *		-- eventOutputBufferCompletelySent_f - could be triggered once more time
 *		   to send the last leftover bytes enqueued for sending if gracefully is 1
 *		-- eventConnectionClosed_f - the connection was successfully terminated
 *
 * 4. apiCreateInStream_f
 * This function will create a logical A/V stream and will associate it with
 * an existing connection
 * PARAMETERS:
 *		-- pConnection - the connection on which the data of the stream will flow
 *		-- pUniqueName - a string containing the unique name for the stream
 *		-- pUserData - pointer to arbitrary data. This pointer will be stored inside the
 *		   context of the new stream and can be used to associate custom
 *		   data with the new stream. It can be NULL
 * GENERATED EVENTS:
 *		-- eventInStreamCreated_f - triggered if the stream is successfully created
 *		-- eventInStreamCreateFailed_f - triggered when the stream couldn't be created
 *		   The only situation when this could happen is when the pUniqueName is
 *		   already taken by some other stream
 *
 * 5. apiSetupInStreamAudioCodecAAC_f,apiSetupInStreamAudioCodecG711_f and apiSetupInStreamVideoCodecH264_f
 * This functions will setup the A/V codecs inside the stream.
 * It is illegal to call apiSetupInStreamAudioCodecAAC_f AND apiSetupInStreamAudioCodecG711_f
 * over the same stream. They are mutually exclusive
 * PARAMETERS:
 *		-- pInStream - target stream on which we want to setup the codecs
 *		-- adts - the ADTS header used to extract information about the AAC data
 *		-- pSPS/pSPS - pointers to memory regions containing the SPS/PPS NAL units
 *		-- spsLength/ppsLength - lengths for the pSPS/pPPS
 * GENERATED EVENTS:
 *		-- eventInStreamClosed_f - occurs if one of the apiSetupInStreamXXX fails. Otherwise
 *		   no events will be triggered
 *
 * 6. apiFeedInStream_f
 * This function can be used to feed the inbound stream with A/V data. Each call
 * to this function should be made for one and only one A/V frame. Partial A/V frames
 * are not yet supported (but they will be)
 * PARAMETERS:
 *		-- pInStream - the stream we want to feed
 *		-- timestamp - the timestamp of the frame
 *		-- pData - pointer to a region of memory containing the A/V data
 *		-- length - length of the pData memory region in bytes
 *		-- isAudio - If the data is audio data, isAudio MUST be 1 otherwise it MUST be 0
 *		   All other values are treated as errors
 * GENERATED EVENTS:
 *		-- eventInStreamClosed_f - occurs when the A/V data is invalid.
 *
 * 7. apiCloseInStream_f
 * This function can be used to tear down a stream. The owning connection will remain usable
 * PARAMETERS:
 *		-- pInStream - the stream that needs to be closed
 * GENERATED EVENTS:
 *		-- eventInStreamClosed_f - occurs when the stream was successfully closed
 */
typedef void (*apiConnect_f)(struct protocol_factory_t *pFactory, const char *pIp, uint16_t port, const char *pProtocolChainName, void *pUserData);
typedef void (*apiSendData_f)(struct connection_t *pConnection, const void *pData, uint32_t length);
typedef void (*apiCloseConnection_f)(struct connection_t *pConnection, int gracefully);
typedef void (*apiCreateInStream_f)(struct connection_t *pConnection, const char *pUniqueName, void *pUserData);
typedef bool (*apiSetupInStreamAudioCodecAAC_f)(struct in_stream_t *pInStream, const uint8_t pCodec[2]);
typedef bool (*apiSetupInStreamAudioCodecG711_f)(struct in_stream_t *pInStream);
typedef bool (*apiSetupInStreamVideoCodecH264_f)(struct in_stream_t *pInStream, const uint8_t *pSPS, uint32_t spsLength, const uint8_t *pPPS, uint32_t ppsLength);
typedef bool (*apiFeedInStream_f)(struct in_stream_t *pInStream, uint64_t pts, uint64_t dts, uint8_t *pData, uint32_t length, int isAudio);
typedef void (*apiCloseInStream_f)(struct in_stream_t *pInStream);
typedef void (*apiCreateOutStream_f)(struct connection_t *pConnection, const char *pInStreamName, void *pUserData);
typedef void (*apiCloseOutStream_f)(struct out_stream_t *pOutStream);
typedef void (*apiCreateJobTimer_f)(struct protocol_factory_t *pFactory, uint32_t period, void *pUserData);
typedef void (*apiCloseJobTimer_f)(struct jobtimer_t *pJobTimer);
typedef uint32_t(*apiRtmpGetProtocolIdByStreamId_f)(struct protocol_factory_t *pFactory, uint32_t uniqueStreamId);
typedef bool (*apiRtmpSendMessage_f)(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pMessage, bool expectResponse);
typedef bool (*apiRtmpBroadcastMessage_f)(struct in_stream_t *pInStream, variant_t *pMessage);


//Structures

typedef struct apiConnection_t {
	apiConnect_f connect;
	apiSendData_f sendData;
	apiCloseConnection_f closeConnection;
} apiConnection_t;

typedef struct apiStream_t {
	apiCreateInStream_f createInStream;
	apiSetupInStreamAudioCodecAAC_f setupInStreamAudioCodecAAC;
	apiSetupInStreamAudioCodecG711_f setupInStreamAudioCodecG711;
	apiSetupInStreamVideoCodecH264_f setupInStreamVideoCodecH264;
	apiFeedInStream_f feedInStream;
	apiCloseInStream_f closeInStream;
	apiCreateOutStream_f createOutStream;
	apiCloseOutStream_f closeOutStream;
} apiStream_t;

typedef struct apiJob_t {
	apiCreateJobTimer_f createTimer;
	apiCloseJobTimer_f closeTimer;
} apiJob_t;

typedef struct apiRtmp_t {
	apiRtmpGetProtocolIdByStreamId_f getProtocolIdByStreamId;
	apiRtmpSendMessage_f sendMessage;
	apiRtmpBroadcastMessage_f broadcastMessage;
} apiRtmp_t;

/*
 * This structure contains all the API functions
 * The values inside this structure MUST not be modified
 * in any way. They are provided and they can ONLY be used
 * Explanations for each of them can be found the the API section
 */
typedef struct api_t {
	apiConnection_t connection;
	apiStream_t stream;
	apiJob_t job;
	apiVariant_t variant;
	apiRtmp_t rtmp;
} api_t;


#endif	/* _RMSPROTOCOLAPIFUNCTIONS_H */
