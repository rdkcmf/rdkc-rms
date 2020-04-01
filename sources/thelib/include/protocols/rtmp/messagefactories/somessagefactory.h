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



#ifdef HAS_PROTOCOL_RTMP
#ifndef _SOMESSAGEFACTORY_H
#define	_SOMESSAGEFACTORY_H

#include "protocols/rtmp/messagefactories/genericmessagefactory.h"

class DLLEXP SOMessageFactory {
public:
	static Variant GetSharedObject(uint32_t channelId, uint32_t streamId,
			double timeStamp, bool isAbsolute, string name, uint32_t version,
			bool persistent);
	static void AddSOPrimitiveConnect(Variant &message);
	static void AddSOPrimitiveSend(Variant &message, Variant &params);
	static void AddSOPrimitiveSetProperty(Variant &message, string &propName,
			Variant &propValue);
};


#endif	/* _SOMESSAGEFACTORY_H */

#endif /* HAS_PROTOCOL_RTMP */

