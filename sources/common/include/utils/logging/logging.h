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



#ifndef _LOGGING_H
#define _LOGGING_H

#include "defines.h"
#ifdef ANDROID
#include "utils/logging/androidloglocation.h"
#include "utils/logging/androidapploglocation.h"
#endif
#ifdef HAS_PROTOCOL_API
#include "utils/logging/apiloglocation.h"
#endif /* HAS_PROTOCOL_API */
#include "utils/logging/baseloglocation.h"
#include "utils/logging/consoleloglocation.h"
#include "utils/logging/fileloglocation.h"
#include "utils/logging/delayedfileloglocation.h"
#include "utils/logging/logcatloglocation.h"
#include "utils/logging/logger.h"



#ifdef VALIDATE_FROMAT_SPECIFIERS
#define __VALIDATE_FROMAT_SPECIFIERS(...) \
do { \
   char ___tempLocation[1024]; \
   snprintf(___tempLocation,1023,__VA_ARGS__); \
}while (0)
#else
#define __VALIDATE_FROMAT_SPECIFIERS(...)
#endif /* VALIDATE_FROMAT_SPECIFIERS */

#ifdef FILE_OVERRIDE
#define __FILE__OVERRIDE ""
#else
#define __FILE__OVERRIDE __FILE__
#endif

#ifdef LINE_OVERRIDE
#define __LINE__OVERRIDE 0
#else
#define __LINE__OVERRIDE __LINE__
#endif

#ifdef FUNC_OVERRIDE
#define __FUNC__OVERRIDE ""
#else
#define __FUNC__OVERRIDE __func__
#endif

#ifdef FILE_OVERRIDE
#define __FILE__OVERRIDE ""
#define E__FILE__ __FILE__OVERRIDE
#else /* FILE_OVERRIDE */
#define __FILE__OVERRIDE __FILE__
#ifdef SHORT_PATH_IN_LOGGER
#define E__FILE__ ((const char *)__FILE__OVERRIDE)+SHORT_PATH_IN_LOGGER
#else /* SHORT_PATH_IN_LOGGER */
#define E__FILE__ __FILE__OVERRIDE
#endif /* SHORT_PATH_IN_LOGGER */
#endif /* FILE_OVERRIDE */

#define LOG(level,...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(level, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);}while(0)
#define FATAL(...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(_FATAL_, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);}while(0)

#define WARN(...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(_WARNING_, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);}while(0)
#define INFO(...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(_INFO_, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);}while(0)
#define DEBUG(...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(_DEBUG_, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);}while(0)
#define FINE(...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(_FINE_, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);}while(0)
#define FINEST(...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(_FINEST_, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);}while(0)
#define ASSERT(...) do{__VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);Logger::Log(_FATAL_, E__FILE__, __LINE__OVERRIDE, __FUNC__OVERRIDE, __VA_ARGS__);o_assert(false);abort();}while(0)
#define NYI WARN("%s not yet implemented",__FUNC__OVERRIDE);
#define NYIF FINE("%s not yet implemented",__FUNC__OVERRIDE);
#define NYIR do{NYI;return false;}while(0)
#define NYIA do{NYI;o_assert(false);abort();}while(0)
#endif /* _LOGGING_H */
