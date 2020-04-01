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


#ifndef _RTMPDISSECTORPROTOCOL_H
#define	_RTMPDISSECTORPROTOCOL_H

#include "protocols/rtmp/basertmpprotocol.h"
#include "basesource.h"

class RTMPDissectorProtocol
: public BaseRTMPProtocol {
private:
	bool _isClient;
	IOBuffer _internalBuffer;
public:
	RTMPDissectorProtocol(bool isClient);
	virtual ~RTMPDissectorProtocol();

	virtual bool EnqueueForOutbound();

	virtual bool PerformHandshake(IOBuffer &buffer);

	bool Ingest(DataBlock &block);
	bool Ingest(IOBuffer &buffer);

	bool IsClient();
};

#endif	/* _RTMPDISSECTORPROTOCOL_H */
