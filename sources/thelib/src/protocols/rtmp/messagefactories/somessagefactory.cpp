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
#include "protocols/rtmp/messagefactories/somessagefactory.h"
#include "protocols/rtmp/header.h"

Variant SOMessageFactory::GetSharedObject(uint32_t channelId, uint32_t streamId,
		double timeStamp, bool isAbsolute, string name, uint32_t version,
		bool persistent) {
	Variant result;

	VH(result, HT_FULL, channelId, timeStamp, 0,
			RM_HEADER_MESSAGETYPE_SHAREDOBJECT, streamId, isAbsolute);

	result[RM_SHAREDOBJECT][RM_SHAREDOBJECT_NAME] = name;
	result[RM_SHAREDOBJECT][RM_SHAREDOBJECT_VERSION] = version;
	result[RM_SHAREDOBJECT][RM_SHAREDOBJECT_PERSISTENCE] = persistent;

	return result;
}

void SOMessageFactory::AddSOPrimitiveConnect(Variant &message) {
	Variant primitive;
	primitive[RM_SHAREDOBJECTPRIMITIVE_TYPE] = SOT_CS_CONNECT;
	M_SO_PRIMITIVES(message).PushToArray(primitive);
}

void SOMessageFactory::AddSOPrimitiveSend(Variant &message, Variant &params) {
	Variant primitive;
	primitive[RM_SHAREDOBJECTPRIMITIVE_TYPE] = SOT_BW_SEND_MESSAGE;

	FOR_MAP(params, string, Variant, i) {
		primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD].PushToArray(MAP_VAL(i));
	}
	M_SO_PRIMITIVES(message).PushToArray(primitive);
}

void SOMessageFactory::AddSOPrimitiveSetProperty(Variant &message, string &propName,
		Variant &propValue) {
	Variant primitive;
	if ((propValue == V_NULL) || (propValue == V_UNDEFINED)) {
		primitive[RM_SHAREDOBJECTPRIMITIVE_TYPE] = SOT_CS_DELETE_FIELD_REQUEST;
		primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD].PushToArray(propName);
	} else {
		primitive[RM_SHAREDOBJECTPRIMITIVE_TYPE] = SOT_CS_UPDATE_FIELD_REQUEST;
		primitive[RM_SHAREDOBJECTPRIMITIVE_PAYLOAD][propName] = propValue;
	}
	M_SO_PRIMITIVES(message).PushToArray(primitive);
}
#endif /* HAS_PROTOCOL_RTMP */

