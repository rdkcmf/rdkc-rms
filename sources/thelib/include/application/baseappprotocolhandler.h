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



#ifndef _BASEAPPPROTOCOLHANDLER_H
#define	_BASEAPPPROTOCOLHANDLER_H

#include "common.h"

class BaseClientApplication;
class StreamMetadataResolver;
class BaseProtocol;
class BaseInStream;
class EventLogger;

/*!
	@class BaseAppProtocolHandler
	@brief
 */
class DLLEXP BaseAppProtocolHandler {
private:
	BaseClientApplication *_pApplication;
protected:
	Variant _configuration;
public:
	BaseAppProtocolHandler(Variant &configuration);
	virtual ~BaseAppProtocolHandler();

	virtual bool ParseAuthenticationNode(Variant &node, Variant &result);

	/*!
		@brief Sets the application.
		@param pApplication
	 */
	virtual void SetApplication(BaseClientApplication *pApplication);
	/*!
		@brief Sets the pointer to the application.
	 */
	BaseClientApplication *GetApplication();

	/*!
		@brief Gets the porotocol handler of the application.
		@param protocolType - Type of portocol
	 */
	BaseAppProtocolHandler * GetProtocolHandler(uint64_t protocolType);

	/*!
		@brief
	 */
	virtual bool PullExternalStream(URI uri, Variant streamConfig);

	/*!
		@brief
	 */
	virtual bool PushLocalStream(Variant streamConfig);

	/*!
		@brief
	 */
	virtual void RegisterProtocol(BaseProtocol *pProtocol) = 0;

	/*!
		@brief
	 */
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol) = 0;

	/*!
	 * @brief returns the event logger associated with the application
	 */
	EventLogger * GetEventLogger();

	/*!
	 * @brief returns the stream metadata resolver associated with this application
	 */
	StreamMetadataResolver *GetStreamMetadataResolver();
};

#endif	/* _BASEAPPPROTOCOLHANDLER_H */


