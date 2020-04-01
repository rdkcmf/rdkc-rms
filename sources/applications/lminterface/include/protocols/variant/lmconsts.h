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


#ifndef _LMCONSTS_H
#define _LMCONSTS_H

#define LM_IP_COUNT 2
#define LM_STARTUP_MSG 1
#define LM_STARTUP_ACK 2
#define LM_HEARTBEAT_MSG 3
#define LM_HEARTBEAT_ACK 4
#define LM_SHUTDOWN_MSG 5
#define LM_SHUTDOWN_ACK 6

namespace app_lminterface {
	const uint32_t CONNECTION_TIMEOUT = 3;
	const uint32_t SHUTDOWN_TIMEOUT = 60 * 10;
	const uint32_t HEARTBEAT_INTERVAL = 60 * 10;
}
//#define TEST_LM

#endif /* _LMCONSTS_H */

