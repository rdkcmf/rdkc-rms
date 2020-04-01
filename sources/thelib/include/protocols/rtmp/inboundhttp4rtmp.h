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



#ifdef HAS_PROTOCOL_HTTP
#ifdef HAS_PROTOCOL_RTMP
#ifndef _INBOUNDHTTP4RTMP_H
#define	_INBOUNDHTTP4RTMP_H

#include "protocols/baseprotocol.h"
#include "protocols/timer/basetimerprotocol.h"

class DLLEXP InboundHTTP4RTMP
: public BaseProtocol {
private:
	static map<uint64_t, time_t> _generatedSids;
	static map<uint64_t, uint32_t> _protocolsBySid;
	static uint32_t _activityTimer;
	static uint16_t _crc1;
	static uint64_t _crc2;
	IOBuffer _inputBuffer;
	IOBuffer _outputBuffer;
	int8_t _httpState;
	int32_t _contentLength;
	bool _keepAlive;
	uint64_t _sid;

	class CleanupTimerProtocol
	: public BaseTimerProtocol {
	public:
		CleanupTimerProtocol();
		virtual ~CleanupTimerProtocol();

		virtual bool TimePeriodElapsed();
	};
public:
	InboundHTTP4RTMP();
	virtual ~InboundHTTP4RTMP();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual IOBuffer * GetOutputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool EnqueueForOutbound();
	virtual void ReadyForSend();
private:
	bool ComputeHTTPHeaders(uint32_t length);
	BaseProtocol *Bind(bool bind);
};


#endif	/* _INBOUNDHTTP4RTMP_H */

#endif /* HAS_PROTOCOL_RTMP */
#endif /* HAS_PROTOCOL_HTTP */


