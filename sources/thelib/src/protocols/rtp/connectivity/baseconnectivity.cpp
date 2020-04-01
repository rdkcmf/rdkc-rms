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


#ifdef HAS_PROTOCOL_RTP
#include "protocols/rtp/connectivity/baseconnectivity.h"

//#define LIVE555WAY

BaseConnectivity::BaseConnectivity() {

}

BaseConnectivity::~BaseConnectivity() {
}

uint32_t BaseConnectivity::ToRTPTS(struct timeval &tv, uint32_t rate) {
#ifdef LIVE555WAY
	u_int32_t timestampIncrement = (rate * tv.tv_sec);
	timestampIncrement += (u_int32_t) ((2.0 * rate * tv.tv_usec + 1000000.0) / 2000000);
	return timestampIncrement;
#else
	return (uint32_t) ((((double) tv.tv_sec * 1000000.00 + (double) tv.tv_usec) / 1000000.00)*(double) rate);
#endif
}

uint32_t BaseConnectivity::ToRTPTS(double milliseconds, uint32_t rate) {
#ifdef LIVE555WAY
	struct timeval tv;
	tv.tv_sec = (milliseconds / 1000.00);
	tv.tv_usec = (((uint32_t) milliseconds) % 1000)*1000;
	return ToRTPTS(tv, rate);
#else
	return (uint32_t) ((milliseconds / 1000.00)*(double) rate);
#endif
}

#endif /* HAS_PROTOCOL_RTP */
