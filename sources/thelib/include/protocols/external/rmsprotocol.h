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

#ifndef _RMSPROTOCOL_H
#define	_RMSPROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#define __EXTERN_C extern "C"
#else /* __cplusplus */
#define __EXTERN_C
#ifndef bool
#define bool int
#endif /* bool */
#ifndef true
#define true 1
#endif /* true */
#ifndef false
#define false 0
#endif /* false */
#endif /* __cplusplus */

#ifndef DLLEXP
#ifdef WIN32
#define DLLEXP __declspec(dllexport)
#else
#define DLLEXP
#endif /* WIN32 */
#endif /* DLLEXP */

	//stock protocols
#define PROTOCOL_TYPE_TCP 0x5443500000000000LL //TCP

#include "rmsprotocolapivariant.h"
#include "rmsprotocolapifunctions.h"
#include "rmsprotocolevents.h"

	//forward declarations
	struct variant_t;
	struct context_t;
	struct protocol_factory_t;
	struct connection_t;
	struct in_stream_t;
	struct out_stream_t;
	struct jobtimer_t;

	//Library initialization function pointers
	/*
	 * This functions are called only one time over the entire existence of the
	 * module
	 * A module must implement and export only this 4 function prototypes.
	 * They can be named in any way as long as they respect the prototype
	 *
	 * 1. libInit_f
	 * This function is called first. This function can contain initialization
	 * routines specific to the module. For example, is memory pool is needed,
	 * this is the perfect place to do mem pool initializations. If no initialization
	 * is needed, than the function can be empty but it MUST be implemented anyway.
	 *
	 * 2. libGetProtocolFactory_f
	 * This function is called immediately after libInit_f. It has one parameter
	 * of type "struct protocol_factory_t *". The pointer is managed by the framework
	 * so no allocations/deallocations are needed. This function MUST
	 * populate the following fields inside that structure:
	 *		-- context.pUserData - pointer to arbitrary data/function containing
	 *		   entities specific to the module. Can be NULL. If allocated
	 *		   by the module, it MUST also be deallocated by the module as well
	 *		   inside libReleaseProtocolFactory_f function
	 *		-- pDescription - short text description of the module. Can be NULL
	 *		-- pSupportedProtocols/supportedProtocolSize - list of supported protocols
	 *		-- pSupportedPorotocolChains/supportedPorotocolChainSize - list of supported protocol chains
	 *		-- events - list of callbacks made by the framework inside the module
	 * After this function is executed successfully, a call to eventLibInitCompleted_f
	 * event will also be triggered.
	 *
	 * 3. libReleaseProtocolFactory_f
	 * This function is the opposite of libGetProtocolFactory_f. It should deallocate
	 * context.pUserData if previously allocated. Same goes for pSupportedProtocols/supportedProtocolSize
	 * and pSupportedPorotocolChains/supportedPorotocolChainSize which were allocated
	 * inside the libGetProtocolFactory_f. The moment this function is called
	 * all the connections ever made by the module are already terminated. Special care
	 * should be taken because this are modules and can be loaded/unloaded at run-time.
	 * Having leftovers lurking around results in memory leaks and affects the stability
	 * of the server.
	 *
	 * 4. libRelease_f
	 * This function is the opposite libInit_f. So, whatever general-purpose resources
	 * were allocated inside libInit_f, should be deallocated in here. This is the
	 * last call inside the modules. After this call completes, the modules
	 * is completely unloaded from run-time environment of the server
	 */
	typedef void (*libInit_f)();
	typedef void (*libGetProtocolFactory_f)(struct protocol_factory_t *pFactory);
	typedef void (*libReleaseProtocolFactory_f)(struct protocol_factory_t *pFactory);
	typedef void (*libRelease_f)();


	//Structures

	/*
	 * This structure holds the context information about the
	 * protocol_factory_t, connection_t or in_stream_t.
	 *		-- pUserData - is the user defined data in the context
	 *		   of a protocol_factory_t, connection_t or in_stream_t
	 *		-- pOpaque - framework defined data. This MUST be kept AS IS
	 *		   and never touched in any way. Failing to do so, will result
	 *		   in serious problems when running the server
	 */
	typedef struct context_t {
		void *pUserData;
		void *pOpaque;
	} context_t;

	/*
	 * This structure describes a protocol chain exported by the module
	 *		-- pName - the name of the protocol chain. It must be unique.
	 *		   Example: myOutboundChain,
	 *		-- pSchema - schema associated with the protocol chain. This
	 *		   can be later used inside CLI pull/push functionality.
	 *		   Example: foo. Pull example: foo://192.168.1.2:777/....
	 *		   Example: bar. Pull example: bar://192.168.1.2:777/....
	 *		-- pChain - protocol types that are forming this chain
	 *		-- chainSize - the number of protocol types inside the protocol chain
	 */
	typedef struct protocol_chain_t {
		const char *pName;
		const char *pSchema;
		uint64_t *pChain;
		uint8_t chainSize;
	} protocol_chain_t;

	/*
	 * This structure describes the module. There is going to be only one instance
	 * of this structure at any given time of the entire life cycle for the module
	 *		-- version - It is a read-only value and designates the module interface version
	 *		-- context - the context associated with the module
	 *		-- pDescription - short description for the module. It can be setup
	 *		   by the module and it can be NULL
	 *		-- pSupportedProtocols/supportedProtocolSize - supported protocols
	 *		   It MSUT be setup by the module and it can not be NULL/0
	 *		-- pSupportedPorotocolChains/supportedPorotocolChainSize - supported protocol chains
	 *		   It MSUT be setup by the module and it can not be NULL/0
	 *		-- api - the API provided be the server. It is ALWAY setup by the framework
	 *		-- events - the events interface. It MUST be setup by the module
	 */
	typedef struct protocol_factory_t {
		uint32_t version;
		struct context_t context;
		char *pDescription;
		uint64_t *pSupportedProtocols;
		uint32_t supportedProtocolSize;
		struct protocol_chain_t *pSupportedPorotocolChains;
		uint32_t supportedPorotocolChainSize;
		struct api_t api;
		struct events_t events;
	} protocol_factory_t;

	/*
	 * This structure describes a connection. It is allocated/deallocated
	 * by the framework. The module is forbidden to allocate/deallocate this
	 * kind of structures
	 *		-- context - the context associated with the connection
	 *		-- type - the type of
	 */
	typedef struct connection_t {
		struct context_t context;
		uint64_t type;
		uint32_t uniqueId;
		const char *pPullInStreamUri;
		const char *pPushOutStreamUri;
		const char *pLocalStreamName;
		struct protocol_factory_t *pFactory;
		struct in_stream_t **ppInStreams;
		uint32_t inStreamsCount;
		struct out_stream_t **ppOutStreams;
		uint32_t outStreamsCount;
	} connection_t;

	typedef struct in_stream_t {
		struct context_t context;
		uint32_t uniqueId;
		char *pUniqueName;
		struct connection_t *pConnection;
		struct protocol_factory_t *pFactory;
	} in_stream_t;

	typedef struct out_stream_t {
		struct context_t context;
		uint32_t uniqueId;
		char *pUniqueName;
		struct connection_t *pConnection;
		struct protocol_factory_t *pFactory;
	} out_stream_t;

	typedef struct jobtimer_t {
		struct context_t context;
		uint32_t uniqueId;
		struct protocol_factory_t *pFactory;
	} jobtimer_t;

	typedef struct variant_t {
		void *pOpaque;
	} variant_t;

#ifdef __cplusplus
}
#endif /*__cplusplus */
#endif	/* _RMSPROTOCOL_H */
