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



#ifndef _CLIENTAPPLICATIONMANAGER_H
#define	_CLIENTAPPLICATIONMANAGER_H


#include "common.h"
#include "application/baseclientapplication.h"

class BaseClientApplication;

/*!
	@brief
 */
class DLLEXP ClientApplicationManager {
private:
	static map<uint32_t, BaseClientApplication *> _applicationsById;
	static map<string, BaseClientApplication *> _applicationsByName;
	static BaseClientApplication *_pDefaultApplication;
public:
	/*!
		@brief Deletes applications registered to the base client application
	 */
	static void Shutdown();

	/*!
		@brief Registers the application using its id, name, and/or alias
		@param pClientApplication
	 */
	static bool RegisterApplication(BaseClientApplication *pClientApplication);

	/*!
		@brief Erases the application using its id, name, and/or alias
		@param pClientApplication
	 */
	static void UnRegisterApplication(BaseClientApplication *pClientApplication);
	/*!
		@brief Gets the default application based on what was indicated in the configuration file
	 */
	static BaseClientApplication *GetDefaultApplication();

	/*!
		@brief Returns the application name in string form
		@param appName
	 */
	static BaseClientApplication *FindAppByName(string appName);

	/*!
		@brief Returns the application's id
		@param id
	 */
	static BaseClientApplication *FindAppById(uint32_t id);

	/*!
		@brief Returns all applications by id.
	 */
	static map<uint32_t, BaseClientApplication *> GetAllApplications();

	/*!
		@brief Sends a message to an application.
	 */
	static bool SendMessageToApplication(string appName, Variant &message);

	/*!
		@brief Sends a message to all application.
	 */
	static bool BroadcastMessageToAllApplications(Variant &message);

	/*!
		@brief Sends signals to all applications that RMS is ready.
	 */
	static void SignalServerBootstrapped();
};


#endif	/* _CLIENTAPPLICATIONMANAGER_H */
