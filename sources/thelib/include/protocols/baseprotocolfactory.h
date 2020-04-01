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



#ifndef _BASEPROTOCOLFACTORY_H
#define	_BASEPROTOCOLFACTORY_H

#include "common.h"

class BaseProtocol;

/*!
	@class BaseProtocolFactory
	@brief The base class on which all protocol factories must derive from.
 */
class DLLEXP BaseProtocolFactory {
private:
	static uint32_t _idGenerator;
	uint32_t _id;
public:
	/*!
		@brief Constructor: Increments the _idGenerator parameter.
	 */
	BaseProtocolFactory();
	virtual ~BaseProtocolFactory();

	/*!
		@brief Returns the factory's ID.
	 */
	uint32_t GetId();

	/*!
		@brief Function that will contain all the protocols that will be handled by the factory.
	 */
	virtual vector<uint64_t> HandledProtocols() = 0;
	/*!
		@brief Function that will contain all the protocol chains that will be handled by the factory.
	 */
	virtual vector<string> HandledProtocolChains() = 0;
	/*!
		@brief This functions is where the protocol chains whose names are all defined, are resolved.
		@param name: The name given to the protocol chain.
	 */
	virtual vector<uint64_t> ResolveProtocolChain(string name) = 0;
	/*!
		@brief This function is where protocols are spawned.
		@param type: The protocol to be spawned.
		@param parameters: Each protocol can have parameters. This is optional.
	 */
	virtual BaseProtocol *SpawnProtocol(uint64_t type, Variant &parameters) = 0;
};

#endif	/* _BASEPROTOCOLFACTORY_H */


