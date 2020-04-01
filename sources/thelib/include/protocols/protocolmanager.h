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



#ifndef _PROTOCOLMANAGER_H
#define	_PROTOCOLMANAGER_H

#include "common.h"

class BaseProtocol;

typedef bool (*protocolManagerFilter_f)(BaseProtocol *pProtocol);

/*!
	@class ProtocolManager
	@brief Class where that handles atomic protocols
 */
class DLLEXP ProtocolManager {
private:
	static map<uint32_t, BaseProtocol *> _activeProtocols;
	static map<uint32_t, BaseProtocol *> _deadProtocols;
public:
	/*!
		@brief Function that registers the protocol. If the protocol is already active or dead, this function will do nothing.
		@param pProtocol: The pointer to the protocol to be registered.
	 */
	static void RegisterProtocol(BaseProtocol *pProtocol);
	/*!
		@brief Function that ters erases protocol.
		@param pProtocol: The pointer to the protocol to be erased.
	 */
	static void UnRegisterProtocol(BaseProtocol *pProtocol);
	/*!
		@brief Function which deletes the protocol. This is called during shutdown to delete the protocol and NULL the application set for the protocol.
		@param pProtocol: The pointer to the protocol to be erased.
	 */
	static void EnqueueForDelete(BaseProtocol *pProtocol);
	/*!
		@brief This function makes sure that the memory allocated to all dead protocols are deleted.
	 */
	static uint32_t CleanupDeadProtocols();
	/*!
		@brief This function deletes all active protocols.
	 */
	static void Shutdown();
	/*!
		@brief Returns the BaseProtocol pointer to the protocol referred to by the provided id.
		@param id: ID of the protocol to be returned
		@param includeDeadProtocols: A boolean that can optionally be set to true of dead protocols are desired. It is by default set to false to include active protocols only.
	 */
	static BaseProtocol * GetProtocol(uint32_t id,
			bool includeDeadProtocols = false);
	/*!
		@brief Returns the vector that contains all active protocols
	 */
	static const map<uint32_t, BaseProtocol *> &GetActiveProtocols();

	/*!
		@brief Returns the vector that contains all active protocols but filtered using filter
	 */
	static void GetActiveProtocols(map<uint32_t, BaseProtocol *> &result,
			protocolManagerFilter_f filter);
};

bool protocolManagerNetworkedProtocolsFilter(BaseProtocol *pProtocol);
bool protocolManagerNearProtocolsFilter(BaseProtocol *pProtocol);

#endif	/* _PROTOCOLMANAGER_H */


