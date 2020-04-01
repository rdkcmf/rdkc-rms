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


#ifndef _EDGESERVER_H
#define	_EDGESERVER_H

#include "common.h"

namespace app_rdkcrouter {

	class EdgeServer
	: public Variant {
	private:
		Variant _dummy;
	public:
		VARIANT_GETSET(uint32_t, Load, 0);
		VARIANT_GETSET(uint32_t, LoadIncrement, 0);
		VARIANT_GETSET(uint32_t, ProtocolId, 0);
		VARIANT_GETSET(uint64_t, Pid, 0);
		VARIANT_GETSET(string, Ip, "");
		VARIANT_GETSET(uint16_t, Port, 0);
		VARIANT_GETSET(uint32_t, StreamsCount, 0);
		VARIANT_GETSET(int64_t, ConnectionsCount, 0);
		VARIANT_GETSET(Variant, IOStats, _dummy);
#ifdef HAS_WEBSERVER
		VARIANT_GETSET(bool, IsWebServer, false);
#endif /*HAS_WEBSERVER*/
	};

}

#endif	/* _EDGESERVER_H */

