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
#ifndef _MAX_H
#define _MAX_H

#ifdef UINT64_MAX
#undef UINT64_MAX
#endif

#ifdef INT64_MAX
#undef INT64_MAX
#endif

#ifdef UINT32_MAX
#undef UINT32_MAX
#endif

#ifdef INT32_MAX
#undef INT32_MAX
#endif

#ifdef UINT16_MAX
#undef UINT16_MAX
#endif

#ifdef INT16_MAX
#undef INT16_MAX
#endif

#ifdef UINT8_MAX
#undef UINT8_MAX
#endif

#ifdef INT8_MAX
#undef INT8_MAX
#endif



#ifndef UINT64_MAX
#define UINT64_MAX ((uint64_t)(0xffffffffffffffffULL))
#endif

#ifndef INT64_MAX
#define INT64_MAX  ((int64_t)(0x7fffffffffffffffLL))
#endif

#ifndef INT64_MIN
#define INT64_MIN  ((int64_t)(0x8000000000000000LL))
#endif

#ifndef UINT32_MAX
#define UINT32_MAX ((uint32_t)(0xffffffffUL))
#endif

#ifndef INT32_MAX
#define INT32_MAX  ((int32_t)(0x7fffffffL))
#endif

#ifndef INT32_MIN
#define INT32_MIN  ((int32_t)(0x80000000L))
#endif

#ifndef UINT16_MAX
#define UINT16_MAX ((uint16_t)(0xffff))
#endif

#ifndef INT16_MAX
#define INT16_MAX  ((int16_t)(0x7fff))
#endif

#ifndef INT16_MIN
#define INT16_MIN  ((int16_t)(0x8000))
#endif

#ifndef UINT8_MAX
#define UINT8_MAX  ((uint8_t)(0xff))
#endif

#ifndef INT8_MAX
#define INT8_MAX   ((int8_t)(0x7f))
#endif

#ifndef INT8_MIN
#define INT8_MIN   ((int8_t)(0x80))
#endif

#endif /* _MAX_H */
#endif /* ANDROID */

