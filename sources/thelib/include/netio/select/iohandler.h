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


#ifdef NET_SELECT
#ifndef _IOHANDLER_H
#define	_IOHANDLER_H

#include "common.h"
#include "netio/iohandlertype.h"

class BaseProtocol;

/*!
	@class IOHandler
 */
class DLLEXP IOHandler {
protected:
	static uint32_t _idGenerator;
	uint32_t _id;
	int32_t _inboundFd;
	int32_t _outboundFd;
	BaseProtocol *_pProtocol;
	IOHandlerType _type;
public:
	IOHandler(int32_t inboundFd, int32_t outboundFd, IOHandlerType type);
	virtual ~IOHandler();

	/*!
		@brief Returns the id of the IO handler.
	 */
	uint32_t GetId();

	/*!
		@brief Returns the id of the inbound file descriptor
	 */
	int32_t GetInboundFd();

	/*!
		@brief Returns the id of the outbound file descriptor
	 */
	int32_t GetOutboundFd();

	/*!
		@brief Returns the IOHandler type
	 */
	IOHandlerType GetType();

	/*!
		@brief Sets the protocol for the IO handler
		@param pProtocol
	 */
	void SetProtocol(BaseProtocol *pPotocol);

	/*!
		@brief Gets the protocol of the IO handler
	 */
	BaseProtocol *GetProtocol();
	virtual bool SignalOutputData() = 0;
	virtual bool OnEvent(select_event &event) = 0;

	/*!
		@brief Returns the string value of the IO handler type
		@param type: Type of IO handler. E.g. acceptor, tct carrier, udp carrier, etc.
	 */
	static string IOHTToString(IOHandlerType type);
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0) = 0;
};


#endif	/* _IOHANDLER_H */
#endif /* NET_SELECT */

