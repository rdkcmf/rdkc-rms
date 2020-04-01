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

#ifdef HAS_PROTOCOL_API
#ifndef _APIFDHANDLER_H
#define _APIFDHANDLER_H
#ifdef NET_EPOLL
#include "netio/epoll/iohandler.h"
#endif /* NET_EPOLL */
class DLLEXP ApiFdHandler : public IOHandler {
public:
	ApiFdHandler(int32_t fd);
	virtual ~ApiFdHandler();
	virtual bool SignalOutputData();
	virtual bool OnEvent(struct epoll_event &event);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);
private:

};

#endif  /* _APIFDHANDLER_H */
#endif	/* HAS_PROTOCOL_API */
