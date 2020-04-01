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



#ifdef HAS_PROTOCOL_RTMP
#ifndef _MONITORRTMPPROTOCOL_H
#define	_MONITORRTMPPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/rtmp/channel.h"
#include "protocols/rtmp/rtmpprotocolserializer.h"

class DLLEXP RTMPOutputChecks
: public BaseProtocol {
protected:
	Channel *_channels;
	int32_t _selectedChannel;
	uint32_t _inboundChunkSize;
	RTMPProtocolSerializer _rtmpProtocolSerializer;
	IOBuffer _input;
	uint32_t _maxStreamCount;
	uint32_t _maxChannelsCount;
public:
	RTMPOutputChecks(uint32_t maxStreamIndex, uint32_t maxChannelIndex);
	virtual ~RTMPOutputChecks();

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);

	bool SetInboundChunkSize(uint32_t chunkSize);

	bool Feed(IOBuffer &buffer);
private:
	bool ProcessBytes(IOBuffer &buffer);

};


#endif	/* _MONITORRTMPPROTOCOL_H */

#endif /* HAS_PROTOCOL_RTMP */

