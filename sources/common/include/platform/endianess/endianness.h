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



#ifndef _ENDIANESS_H
#define	_ENDIANESS_H

//LITTLE ENDIAN SYSTEMS
#ifdef LITTLE_ENDIAN_BYTE_ALIGNED
#include "platform/endianess/little_endian_byte_aligned.h"
#endif

#ifdef LITTLE_ENDIAN_SHORT_ALIGNED
#include "platform/endianess/little_endian_short_aligned.h"
#endif

#ifdef BIG_ENDIAN_BYTE_ALIGNED
#include "platform/endianess/big_endian_byte_aligned.h"
#endif

#ifdef BIG_ENDIAN_SHORT_ALIGNED
#include "platform/endianess/big_endian_short_aligned.h"
#endif

#endif	/* _ENDIANESS_H */


