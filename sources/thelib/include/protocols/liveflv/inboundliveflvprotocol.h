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


#ifdef HAS_PROTOCOL_LIVEFLV
#ifndef _INBOUNDLIVEFLVPROTOCOL_H
#define	_INBOUNDLIVEFLVPROTOCOL_H

#include "protocols/baseprotocol.h"

class InNetLiveFLVStream;

class DLLEXP InboundLiveFLVProtocol
: public BaseProtocol {
private:
	InNetLiveFLVStream *_pStream;
	bool _headerParsed;
	bool _waitForMetadata;

	struct Dts {
		uint64_t last;
		uint64_t current;
		uint64_t rollOverCount;
		double value;

		operator string() {
			return format("last: %"PRIx64"; current: %"PRIx64"; rollOverCount: %"PRIx64"; value: %.2f",
					last, current, rollOverCount, value);
		}
	};

	struct {
		Dts _audio;
		Dts _video;
		Dts _meta;
	} _timestamps;
public:
	InboundLiveFLVProtocol();
	virtual ~InboundLiveFLVProtocol();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
private:
	bool InitializeStream(string streamName);
	string ComputeStreamName(string suggestion);
};


#endif	/* _INBOUNDLIVEFLVPROTOCOL_H */
#endif /* HAS_PROTOCOL_LIVEFLV */

