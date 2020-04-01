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
#ifndef _TSPACKETHEADER_H
#define	_TSPACKETHEADER_H

#include "common.h"

//iso13818-1.pdf page 36/174
//Table 2-2 – Transport packet of this Recommendation | International Standard
#define TS_TRANSPORT_PACKET_PID(x)					((uint16_t)(((x)>>8)&0x00001fff))
#define TS_TRANSPORT_PACKET_HAS_ADAPTATION_FIELD(x)	((bool)(((x)&(0x00000020))!=0))
#define TS_TRANSPORT_PACKET_HAS_PAYLOAD(x)			((bool)(((x)&(0x00000010))!=0))
#define TS_TRANSPORT_PACKET_IS_PAYLOAD_START(x)		((bool)(((x)&(0x00400000))!=0))

#endif	/* _TSPACKETHEADER_H */
#endif	/* HAS_MEDIA_TS */
