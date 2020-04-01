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

#ifdef ANDROID
#include "platform/platform.h"
#if !defined(__LP64__)
#include <time64.h>

time_t timegm(struct tm * const t) {
	// time_t is signed on Android.
	static const time_t kTimeMax = ~(1 << (sizeof (time_t) * CHAR_BIT - 1));
	static const time_t kTimeMin = (1 << (sizeof (time_t) * CHAR_BIT - 1));
	time64_t result = timegm64(t);
	if (result < kTimeMin || result > kTimeMax)
		return -1;
	return result;
}

#endif
#endif /* ANDROID */
