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



#ifndef _HTTPMSSLIVEINGEST_H
#define _HTTPMSSLIVEINGEST_H

#include "protocols/baseprotocol.h"
#include "protocols/http/httpinterface.h"
class OutNetHLSStream;

class HTTPMssLiveIngest
: public BaseProtocol, HTTPInterface {
private:
    uint32_t _available;
    IOBuffer _outputBuffer;
    IOBuffer _inputBuffer;
    static HTTPMssLiveIngest* _pMssLiveIngestProtocol;
    bool _moreChunkedData;
public:
    HTTPMssLiveIngest();
    ~HTTPMssLiveIngest();
    bool SendRequest();
    bool SendFragments(uint8_t *pBuffer, uint32_t size);
    void AddOutboundData(uint8_t *pBuffer, uint32_t size);
    inline void SignalEndOfPOST() { _moreChunkedData = false; }
    // BaseProtocol function implementations
    virtual bool Initialize(Variant &parameters);
    virtual bool AllowFarProtocol(uint64_t type);
    virtual bool AllowNearProtocol(uint64_t type);
    virtual bool SignalInputData(int32_t recvAmount);
    virtual bool SignalInputData(IOBuffer &buffer);
    virtual HTTPInterface* GetHTTPInterface();
    virtual IOBuffer * GetOutputBuffer();

    // HTTPInterface function implementations
    virtual bool SignalRequestBegin(HttpMethod method, const char *pUri,
            const uint32_t uriLength, const char *pHttpVersion,
            const uint32_t httpVersionLength);
    virtual bool SignalResponseBegin(const char *pHttpVersion,
            const uint32_t httpVersionLength, uint32_t code,
            const char *pDescription, const uint32_t descriptionLength);
    virtual bool SignalHeader(const char *pName, const uint32_t nameLength,
            const char *pValue, const uint32_t valueLength);
    virtual bool SignalHeadersEnd();
    virtual bool SignalBodyData(HttpBodyType bodyType, uint32_t length,
            uint32_t consumed, uint32_t available, uint32_t remaining);
    virtual bool SignalBodyEnd();
    virtual bool SignalRequestTransferComplete();
    virtual bool SignalResponseTransferComplete();
    virtual bool WriteFirstLine(IOBuffer &outputBuffer);
    virtual bool WriteTargetHost(IOBuffer &outputBuffer);
    virtual bool WriteCustomHeaders(IOBuffer &outputBuffer);
    virtual HttpBodyType GetOutputBodyType();
    virtual int64_t GetContentLenth();
    virtual bool NeedsNewOutboundRequest();
    virtual bool HasMoreChunkedData();
public:
    static bool Connect(string ip, uint16_t port, Variant customParameters);
    static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant customParameters);
    static HTTPMssLiveIngest* GetInstance();
private:
    string GetHTTPDescription(uint32_t returnCode);
};

#endif /* _HTTPMSSLIVEINGEST_H */
