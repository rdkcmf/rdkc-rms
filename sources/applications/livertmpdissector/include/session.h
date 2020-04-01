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


#ifndef _SESSION_H
#define	_SESSION_H

#include "common.h"

class BaseProtocol;
class BaseClientApplication;
class RTMPTrafficDissector;

namespace app_livertmpdissector {

class Session {
private:
	static uint32_t _idGenerator;
	static map<uint32_t, Session *> _allSessions;
	uint32_t _id;
	uint32_t _inboundProtocolId;
	uint32_t _outboundProtocolId;
	IOBuffer _tempIOBuffer;
	RTMPTrafficDissector *_pDissector;
public:
	Session(Variant &parameters, BaseClientApplication *pApp);
	static Session * GetSession(uint32_t id);
	virtual ~Session();

	uint32_t GetId();

	uint32_t InboundProtocolId();
	void InboundProtocolId(BaseProtocol *pProtocol);

	uint32_t OutboundProtocolId();
	void OutboundProtocolId(BaseProtocol *pProtocol);

	bool FeedData(IOBuffer &buffer, BaseProtocol *pFrom);
};
};

#endif	/* _SESSION_H */
