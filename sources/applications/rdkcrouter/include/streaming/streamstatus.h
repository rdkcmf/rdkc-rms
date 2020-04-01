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


#ifndef _STREAMSTATUS_H
#define _STREAMSTATUS_H
#include "common.h"

namespace app_rdkcrouter {

	enum StreamStatus {
		SS_STREAMING = 0,
		SS_CONNECTING,
		SS_WAITFORSTREAM,
		SS_CONNECTED,
		SS_DISCONNECTED,
		SS_ERROR_CONNECTION_FAILED
	};

	string StreamStatusString(StreamStatus status);
}
#endif /* _STREAMSTATUS_H */
