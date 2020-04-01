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



#ifdef HAS_MEDIA_TS
#ifndef _PIDTYPES_H
#define	_PIDTYPES_H

#include "common.h"

//iso13818-1 page 37/174
//Table 2-3 – PID table
//WARN: This are not the values from the table. This are types-over-types

enum PIDType {
	PID_TYPE_UNKNOWN,
	PID_TYPE_PAT,
	PID_TYPE_PMT,
	PID_TYPE_NIT,
	PID_TYPE_CAT,
	PID_TYPE_TSDT,
	PID_TYPE_RESERVED,
	PID_TYPE_AUDIOSTREAM,
	PID_TYPE_VIDEOSTREAM,
	PID_TYPE_NULL
};

#endif	/* _PIDTYPES_H */
#endif	/* HAS_MEDIA_TS */

