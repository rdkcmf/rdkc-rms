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

#ifdef HAS_PROTOCOL_WS
#ifdef HAS_PROTOCOL_WS_FMP4
#ifndef _INBOUNDHTTPSTREAMINGPROTOCOL_H
#define	_INBOUNDHTTPSTREAMINGPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/ws/wsinterface.h"

class InboundWS4FMP4
: public BaseProtocol, WSInterface {
private:
	uint64_t _outputStreamId;
	bool _fullMp4;
	string _streamName;
	bool _isPlaylist;
public:
	InboundWS4FMP4();
	virtual ~InboundWS4FMP4();

	//BaseProtocol
	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool SendOutOfBandData(const IOBuffer &buffer, void *pUserData);

	//WSInterface
	virtual WSInterface* GetWSInterface();
	virtual bool SignalInboundConnection(Variant &request);
	virtual bool SignalTextData(const uint8_t *pBuffer, size_t length);
	virtual bool SignalBinaryData(const uint8_t *pBuffer, size_t length);
	virtual bool SignalPing(const uint8_t *pBuffer, size_t length);
	virtual void SignalConnectionClose(uint16_t code, const uint8_t *pReason, size_t length);
	virtual bool DemaskBuffer();
private:
	uint64_t FindStream(string requestedStreamName, bool& repeat);
	void RemovePlaylist();
};
#endif /* _INBOUNDHTTPSTREAMINGPROTOCOL_H */
#endif	/* HAS_PROTOCOL_WS_FMP4 */
#endif	/* HAS_PROTOCOL_WS */
