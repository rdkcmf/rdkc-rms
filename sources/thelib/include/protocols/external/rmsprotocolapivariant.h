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

#ifndef _RMSPROTOCOLAPIVARIANT_H
#define	_RMSPROTOCOLAPIVARIANT_H

//forward declarations
struct variant_t;

typedef variant_t *(*apiVariantCreate_f)();
typedef variant_t *(*apiVariantCopy_f)(variant_t *pSource);
typedef variant_t *(*apiVariantCreateRtmpRequest_f)(const char *pFunctionName);
typedef bool (*apiVariantAddRtmpRequestParameter_f)(variant_t *pRequest, variant_t **ppParameter, bool release);
typedef void (*apiVariantRelease_f)(variant_t *pVariant);
typedef void (*apiVariantReset_f)(variant_t *pVariant, int depth, ...);
typedef char* (*apiVariantGetXml_f)(variant_t *pVariant, char *pDest, uint32_t destLength, int depth, ...);
typedef bool(*apiVariantGetBool_f)(variant_t *pVariant, int depth, ...);
typedef int8_t(*apiVariantGetI8_f)(variant_t *pVariant, int depth, ...);
typedef int16_t(*apiVariantGetI16_f)(variant_t *pVariant, int depth, ...);
typedef int32_t(*apiVariantGetI32_f)(variant_t *pVariant, int depth, ...);
typedef int64_t(*apiVariantGetI64_f)(variant_t *pVariant, int depth, ...);
typedef uint8_t(*apiVariantGetUI8_f)(variant_t *pVariant, int depth, ...);
typedef uint16_t(*apiVariantGetUI16_f)(variant_t *pVariant, int depth, ...);
typedef uint32_t(*apiVariantGetUI32_f)(variant_t *pVariant, int depth, ...);
typedef uint64_t(*apiVariantGetUI64_f)(variant_t *pVariant, int depth, ...);
typedef double (*apiVariantGetDouble_f)(variant_t *pVariant, int depth, ...);
typedef char* (*apiVariantGetString_f)(variant_t *pVariant, char *pDest, uint32_t destLength, int depth, ...);
typedef void (*apiVariantSetBool_f)(variant_t *pVariant, bool value, int depth, ...);
typedef void (*apiVariantSetI8_f)(variant_t *pVariant, int8_t value, int depth, ...);
typedef void (*apiVariantSetI16_f)(variant_t *pVariant, int16_t value, int depth, ...);
typedef void (*apiVariantSetI32_f)(variant_t *pVariant, int32_t value, int depth, ...);
typedef void (*apiVariantSetI64_f)(variant_t *pVariant, int64_t value, int depth, ...);
typedef void (*apiVariantSetUI8_f)(variant_t *pVariant, uint8_t value, int depth, ...);
typedef void (*apiVariantSetUI16_f)(variant_t *pVariant, uint16_t value, int depth, ...);
typedef void (*apiVariantSetUI32_f)(variant_t *pVariant, uint32_t value, int depth, ...);
typedef void (*apiVariantSetUI64_f)(variant_t *pVariant, uint64_t value, int depth, ...);
typedef void (*apiVariantSetDouble_f)(variant_t *pVariant, double value, int depth, ...);
typedef void (*apiVariantSetString_f)(variant_t *pVariant, const char *pValue, int depth, ...);
typedef void (*apiVariantPushBool_f)(variant_t *pVariant, bool value, int depth, ...);
typedef void (*apiVariantPushI8_f)(variant_t *pVariant, int8_t value, int depth, ...);
typedef void (*apiVariantPushI16_f)(variant_t *pVariant, int16_t value, int depth, ...);
typedef void (*apiVariantPushI32_f)(variant_t *pVariant, int32_t value, int depth, ...);
typedef void (*apiVariantPushI64_f)(variant_t *pVariant, int64_t value, int depth, ...);
typedef void (*apiVariantPushUI8_f)(variant_t *pVariant, uint8_t value, int depth, ...);
typedef void (*apiVariantPushUI16_f)(variant_t *pVariant, uint16_t value, int depth, ...);
typedef void (*apiVariantPushUI32_f)(variant_t *pVariant, uint32_t value, int depth, ...);
typedef void (*apiVariantPushUI64_f)(variant_t *pVariant, uint64_t value, int depth, ...);
typedef void (*apiVariantPushDouble_f)(variant_t *pVariant, double value, int depth, ...);
typedef void (*apiVariantPushString_f)(variant_t *pVariant, const char *pValue, int depth, ...);

typedef struct apiVariant_t {
	apiVariantCreate_f create;
	apiVariantCopy_f copy;
	apiVariantCreateRtmpRequest_f createRtmpRequest;
	apiVariantAddRtmpRequestParameter_f addRtmpRequestParameter;
	apiVariantRelease_f release;
	apiVariantReset_f reset;
	apiVariantGetXml_f getXml;
	apiVariantGetBool_f getBool;
	apiVariantGetI8_f getI8;
	apiVariantGetI16_f getI16;
	apiVariantGetI32_f getI32;
	apiVariantGetI64_f getI64;
	apiVariantGetUI8_f getUI8;
	apiVariantGetUI16_f getUI16;
	apiVariantGetUI32_f getUI32;
	apiVariantGetUI64_f getUI64;
	apiVariantGetDouble_f getDouble;
	apiVariantGetString_f getString;
	apiVariantSetBool_f setBool;
	apiVariantSetI8_f setI8;
	apiVariantSetI16_f setI16;
	apiVariantSetI32_f setI32;
	apiVariantSetI64_f setI64;
	apiVariantSetUI8_f setUI8;
	apiVariantSetUI16_f setUI16;
	apiVariantSetUI32_f setUI32;
	apiVariantSetUI64_f setUI64;
	apiVariantSetDouble_f setDouble;
	apiVariantSetString_f setString;
	apiVariantPushBool_f pushBool;
	apiVariantPushI8_f pushI8;
	apiVariantPushI16_f pushI16;
	apiVariantPushI32_f pushI32;
	apiVariantPushI64_f pushI64;
	apiVariantPushUI8_f pushUI8;
	apiVariantPushUI16_f pushUI16;
	apiVariantPushUI32_f pushUI32;
	apiVariantPushUI64_f pushUI64;
	apiVariantPushDouble_f pushDouble;
	apiVariantPushString_f pushString;
} apiVariant_t;


#endif	/* _RMSPROTOCOLAPIVARIANT_H */
