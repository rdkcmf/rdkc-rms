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


#ifndef _MODULE_H
#define	_MODULE_H

#include "rmsprotocol.h"

__EXTERN_C DLLEXP void Init();
__EXTERN_C DLLEXP void GetProtocolFactory(protocol_factory_t *pFactory);
__EXTERN_C DLLEXP void ReleaseProtocolFactory(protocol_factory_t *pFactory);
__EXTERN_C DLLEXP void Release();

void onLibInitCompleted(protocol_factory_t *pFactory);
void onConnectSucceeded(connection_t *pConnection);
void onConnectFailed(protocol_factory_t *pFactory, const char *pUri, const char *pIp, uint16_t port, void *pUserData);
void onConnectionAccepted(connection_t *pConnection);
uint32_t onDataAvailable(connection_t *pConnection, void *pData, uint32_t length);
void onOutputBufferCompletelySent(connection_t *pConnection);
void onConnectionClosed(connection_t *pConnection);
void onInStreamCreated(in_stream_t *pInStream);
void onInStreamCreateFailed(connection_t *pConnection, const char *pUniqueName, void *pUserData);
void onInStreamClosed(in_stream_t *pInStream);
void onInStreamAttached(struct in_stream_t *pInStream, uint32_t outStreamId, uint32_t outStreamProtocolId);
void onInStreamDetached(struct in_stream_t *pInStream, uint32_t outStreamId, uint32_t outStreamProtocolId);
void onOutStreamCreated(out_stream_t *pOutStream);
void onOutStreamCreateFailed(connection_t *pConnection, const char *pInStreamName, void *pUserData);
void onOutStreamClosed(out_stream_t *pOutStream);
void onOutStreamData(struct out_stream_t *pOutStream, uint64_t pts, uint64_t dts, void *pData, uint32_t length, bool isAudio);
void onOutStreamAttached(struct out_stream_t *pOutStream, uint32_t inStreamId, uint32_t inStreamProtocolId);
void onOutStreamDetached(struct out_stream_t *pOutStream, uint32_t inStreamId, uint32_t inStreamProtocolId);
void onJobTimerCreated(jobtimer_t *pJobTimer);
void onJobTimerCreateFailed(protocol_factory_t *pFactory, void *pUserData);
void onJobTimerClosed(jobtimer_t *pJobTimer);
void onJobTimerPulse(jobtimer_t *pJobTimer);
void onRtmpClientConnected(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pConnectRequest);
void onRtmpClientDisconnected(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId);
void onRtmpClientOutBufferFull(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, uint32_t maximumAmount, uint32_t currentAmount);
void onRtmpRequestAvailable(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pRequest);
void onRtmpResponseAvailable(struct protocol_factory_t *pFactory, uint32_t rtmpConnectionId, variant_t *pRequest, variant_t *pResponse);

#endif	/* _MODULE_H */
