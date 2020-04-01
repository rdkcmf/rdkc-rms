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


#ifndef _THREADING_H
#define _THREADING_H
#ifdef HAS_THREAD

#ifdef THREAD_POSIX
#include "threading/pthread/threadmanager.h"
#endif	/* THREAD_POSIX */

#define TASK_PARAM(v) v["params"]
#define TASK_RESULT(v) v["result"]
#define TASK_STATUS(v) TASK_RESULT(v)["status"]
#endif  /* HAS_THREAD */
#endif  /* _THREADING_H */
