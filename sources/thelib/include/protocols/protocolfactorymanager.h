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



#ifndef _PROTOCOLFACTORYMANAGER_H
#define	_PROTOCOLFACTORYMANAGER_H

#include "common.h"

class BaseProtocolFactory;
class BaseProtocol;

/*!
	@class ProtocolFactoryManager
	@brief Class that manages protocol factories.
 */
class DLLEXP ProtocolFactoryManager {
private:
	static map<uint32_t, BaseProtocolFactory *> _factoriesById;
	static map<uint64_t, BaseProtocolFactory *> _factoriesByProtocolId;
	static map<string, BaseProtocolFactory *> _factoriesByChainName;
public:
	//(Un)Register functionality
	/*!
		@brief Registers the atomic protocols and protocol chains of the factory.
		@param pFactory: Pointer to the BaseProtocolFactory that contains the protocols and protocol chains.
	 */
	static bool RegisterProtocolFactory(BaseProtocolFactory *pFactory);
	/*!
		@brief Erases the atomic protocols and protocol chains of the factory
		@param factoryId: ID of the protocol factory to be erased.
	 */
	static bool UnRegisterProtocolFactory(uint32_t factoryId);
	/*!
		@brief Erases the atomic protocols and protocol chains of the factory
		@param pFactory: Pointer to the BaseProtocolFactory that contains the protocols and protocol chains.
	 */
	static bool UnRegisterProtocolFactory(BaseProtocolFactory *pFactory);

	//protocol creation
	/*!
		@brief Resolves the protocol chain based on the given name.
		@param name: Name of the protocol chain to be resolved.
	 */
	static vector<uint64_t> ResolveProtocolChain(string name);
	/*!
		@brief Creates protocol chains.
		@param name: The chain's name in string. This will be resolved using the ResolveProtocolChain function.
		@param parameters: The parameters that come with the chain.
	 */
	static BaseProtocol *CreateProtocolChain(string name, Variant &parameters);
	/*!
		@brief The function that spawns the protocols in the protocol chain.
		@param chain: Vector that resulted from resolving the protocol chain.
		@param parameters: The parameters that come with the chain.
	 */
	static BaseProtocol *CreateProtocolChain(vector<uint64_t> &chain,
			Variant &parameters);
private:
	static string Dump();
};


#endif	/* _PROTOCOLFACTORYMANAGER_H */


