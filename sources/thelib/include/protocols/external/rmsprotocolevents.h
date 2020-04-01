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

#ifndef _RMSPROTOCOLEVENTS_H
#define	_RMSPROTOCOLEVENTS_H


//forward declarations
struct variant_t;
struct context_t;
struct protocol_factory_t;
struct connection_t;
struct in_stream_t;
struct out_stream_t;
struct jobtimer_t;

//Event function pointers
/*
 * This functions should ALL be implemented even if that means an empty function
 * The function pointers should be after that stored inside protocol_factory_t::events
 * This functions will be called by the framework in response to various events occurring
 * All of them will have a pointer to protocol_factory_t, connection_t or in_stream_t
 * This can be used to navigate up or down the hierarchy of the factory/protocols/streams
 *
 * 1. eventLibInitCompleted_f
 * This function is called after libReleaseProtocolFactory_f is called and
 * the framework is ready to commence normal operations. Here is the perfect
 * place to initiate outbound connections programmatically. Despite the fact that
 * new connections can be opened programmatically, it is not recommended to do so.
 * The standard way of doing that is through the CLI. But, again, if needed, it
 * can be done here.
 *
 * 2. eventConnectSucceeded_f
 * This function is called after a new outbound connection was successfully established
 * It is called in response to either apiConnect_f function or in response to
 * pullStream/pushStream CLI commands. After this call have been made,
 * apiSendData_f, apiCloseConnection_f and apiCreateInStream_f can be called
 * according to the module requirements
 *
 * 3. eventConnectFailed_f
 * This function is called after an outbound connection failed. It is called
 * in response to either apiConnect_f function or in response to pullStream/pushStream CLI commands.
 *
 * 4. eventConnectionAccepted_f
 * This function is called after a new inbound connection was accepted by
 * the framework. This is happening only if a service handling the protocols
 * exposed by this module was defined in the configuration file. Similar to
 * eventConnectSucceeded_f, functions like apiSendData_f, apiCloseConnection_f
 * and apiCreateInStream_f can be called according to the module requirements
 *
 * 5. eventDataAvailable_f
 * This function is called the moment we have outstanding data that is not yet
 * consumed. This function MUST return the number of consumed bytes.
 * This number MUST be less or equal than the length parameters. DO NOT
 * try to consume the bytes if is not necessary. For example, let's say
 * that we have an echo protocol. And we should send back the string we've
 * received. The string should be "\r\n" or "\n" terminated. If we get
 * partial data (like "Hello wo"), we should just return 0. This is translated
 * into "the message is not complete. Wait for more data" by the framework and
 * the pointer is preserved. This way the memory routines and amount are kept to a minimum.
 * If later we also get "rld\r\n" on the pipe, eventDataAvailable_f is executed
 * again and the pointer will have the complete data: "Hello world\r\n". Now
 * is ok to send the buffer back with apiSendData_f AND return 13. This results
 * in discarding the first 13 bytes from the data which we don't need anymore.
 * As a conclusion, DO NOT preserve any raw data inside the module. Always
 * let the framework do that for you. Just return the amount of bytes processed/consumed
 *
 * 6. eventOutputBufferCompletelySent_f
 * This function is called whenever all the data from output buffer
 * was completely transfered. This is purely an informational event. That means
 * it is not a bad thing to see this called over and over again. On the contrary:
 * if this is called often, that is a good indication that the connection
 * is capable of transferring the data without buffering it for too long
 *
 * 7. eventConnectionClosed_f
 * This function is called when a connection is terminated. This can occur because of
 * transfer errors or because apiCloseConnection_f was called
 *
 * 8. eventInStreamCreated_f
 * This function is called after a stream was successfully created via apiCreateInStream_f
 *
 * 9. eventInStreamCreateFailed_f
 * This function is called when the call to apiCreateInStream_f failed
 *
 * 10. eventInStreamClosed_f
 * This function is called when a stream is closed. This can occur either
 * because the underlaying connection was terminated or because of a call to
 * apiCloseInStream_f
 */
typedef void (*eventLibInitCompleted_f)(struct protocol_factory_t *pFactory);
typedef void (*eventConnectSucceeded_f)(struct connection_t *pConnection);
typedef void (*eventConnectFailed_f)(struct protocol_factory_t *pFactory, const char *pUri, const char *pIp, uint16_t port, void *pUserData);
typedef void (*eventConnectionAccepted_f)(struct connection_t *pConnection);
typedef uint32_t(*eventDataAvailable_f)(struct connection_t *pConnection, void *pData, uint32_t length);
typedef void (*eventOutputBufferCompletelySent_f)(struct connection_t *pConnection);
typedef void (*eventConnectionClosed_f)(struct connection_t *pConnection);
typedef void (*eventInStreamCreated_f)(struct in_stream_t *pInStream);
typedef void (*eventInStreamCreateFailed_f)(struct connection_t *pConnection, const char *pUniqueName, void *pUserData);
typedef void (*eventInStreamClosed_f)(struct in_stream_t *pInStream);
typedef void (*eventInStreamAttached_f)(struct in_stream_t *pInStream, uint32_t outStreamId, uint32_t outStreamProtocolId);
typedef void (*eventInStreamDetached_f)(struct in_stream_t *pInStream, uint32_t outStreamId, uint32_t outStreamProtocolId);
typedef void (*eventOutStreamCreated_f)(struct out_stream_t *pOutStream);
typedef void (*eventOutStreamCreateFailed_f)(struct connection_t *pConnection, const char *pInStreamName, void *pUserData);
typedef void (*eventOutStreamClosed_f)(struct out_stream_t *pOutStream);
typedef void (*eventOutStreamData_f)(struct out_stream_t *pOutStream, uint64_t pts, uint64_t dts, void *pData, uint32_t length, bool isAudio);
typedef void (*eventOutStreamAttached_f)(struct out_stream_t *pOutStream, uint32_t inStreamId, uint32_t inStreamProtocolId);
typedef void (*eventOutStreamDetached_f)(struct out_stream_t *pOutStream, uint32_t inStreamId, uint32_t inStreamProtocolId);
typedef void (*eventJobTimerCreated_f)(struct jobtimer_t *pJobTimer);
typedef void (*eventJobTimerCreateFailed_f)(struct protocol_factory_t *pFactory, void *pUserData);
typedef void (*eventJobTimerClosed_f)(struct jobtimer_t *pJobTimer);
typedef void (*eventJobTimerPulse_f)(struct jobtimer_t *pJobTimer);
typedef void (*eventRtmpClientConnected_f)(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pConnectRequest);
typedef void (*eventRtmpClientDisconnected_f)(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId);
typedef void (*eventRtmpClientOutBufferFull_f)(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, uint32_t maximumAmount, uint32_t currentAmount);
typedef void (*eventRtmpRequestAvailable_f)(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pRequest);
typedef void (*eventRtmpResponseAvailable_f)(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pRequest, variant_t *pResponse);

typedef struct eventsLib_t {
	eventLibInitCompleted_f initCompleted;
} eventsLib_t;

typedef struct eventsConnection_t {
	eventConnectSucceeded_f connectSucceeded;
	eventConnectFailed_f connectFailed;
	eventConnectionAccepted_f connectionAccepted;
	eventDataAvailable_f dataAvailable;
	eventOutputBufferCompletelySent_f outputBufferCompletelySent;
	eventConnectionClosed_f connectionClosed;
} eventsConnection_t;

typedef struct eventsStream_t {
	eventInStreamCreated_f inStreamCreated;
	eventInStreamCreateFailed_f inStreamCreateFailed;
	eventInStreamClosed_f inStreamClosed;
	eventInStreamAttached_f inStreamAttached;
	eventInStreamDetached_f inStreamDetached;
	eventOutStreamCreated_f outStreamCreated;
	eventOutStreamCreateFailed_f outStreamCreateFailed;
	eventOutStreamClosed_f outStreamClosed;
	eventOutStreamData_f outStreamData;
	eventOutStreamAttached_f outStreamAttached;
	eventOutStreamDetached_f outStreamDetached;
} eventsStream_t;

typedef struct eventsJob_t {
	eventJobTimerCreated_f timerCreated;
	eventJobTimerCreateFailed_f timerCreateFailed;
	eventJobTimerClosed_f timerClosed;
	eventJobTimerPulse_f timerPulse;
} eventsJob_t;

typedef struct eventsRtmp_t {
	eventRtmpClientConnected_f clientConnected;
	eventRtmpClientDisconnected_f clientDisconnected;
	eventRtmpClientOutBufferFull_f clientOutBufferFull;
	eventRtmpRequestAvailable_f requestAvailable;
	eventRtmpResponseAvailable_f responseAvailable;
} eventsRtmp_t;

/*
 * This structure MUST be initialized by the module
 * ALL functions are MANDATORY. If no special things needs to be done
 * in this events, it is ok to just provide an empty function.
 * Explanations for each of them can be found the the events section
 */
typedef struct events_t {
	eventsLib_t lib;
	eventsConnection_t connection;
	eventsStream_t stream;
	eventsJob_t job;
	eventsRtmp_t rtmp;
} events_t;

#endif	/* _RMSPROTOCOLEVENTS_H */
