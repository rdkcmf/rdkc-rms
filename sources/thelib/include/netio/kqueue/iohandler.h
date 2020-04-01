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


#ifdef NET_KQUEUE
#ifndef _IOHANDLER_H
#define	_IOHANDLER_H

#include "common.h"
#include "netio/kqueue/iohandlermanagertoken.h"
#include "netio/iohandlertype.h"
#ifdef HAS_UDS
#include <sys/un.h>
#endif

class BaseProtocol;

class IOHandler {
protected:
	static uint32_t _idGenerator;
	uint32_t _id;
	int32_t _inboundFd;
	int32_t _outboundFd;
	BaseProtocol *_pProtocol;
	IOHandlerType _type;
private:
	IOHandlerManagerToken *_pToken;
public:
	IOHandler(int32_t inboundFd, int32_t outboundFd, IOHandlerType type);
	virtual ~IOHandler();
	void SetIOHandlerManagerToken(IOHandlerManagerToken *pToken);
	IOHandlerManagerToken * GetIOHandlerManagerToken();
	uint32_t GetId();
	int32_t GetInboundFd();
	int32_t GetOutboundFd();
	IOHandlerType GetType();
	void SetProtocol(BaseProtocol *pPotocol);
	BaseProtocol *GetProtocol();
	virtual bool SignalOutputData() = 0;
	virtual bool OnEvent(struct kevent &event) = 0;
	static string IOHTToString(IOHandlerType type);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0) = 0;
};


#endif	/* _IOHANDLER_H */
#endif /* NET_KQUEUE */

