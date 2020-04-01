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



#ifdef HAS_PROTOCOL_RTP
#ifndef _RTPHEADER_H
#define _RTPHEADER_H

#include "common.h"

#define GET_RTP_V(x)   ((uint8_t )((((x)._flags)>>30)))
#define GET_RTP_P(x)   ((bool    )((((x)._flags)>>29)&0x01))
#define GET_RTP_X(x)   ((bool    )((((x)._flags)>>28)&0x01))
#define GET_RTP_CC(x)  ((uint8_t )((((x)._flags)>>24)&0x0f))
#define GET_RTP_M(x)   ((bool    )((((x)._flags)>>23)&0x01))
#define GET_RTP_PT(x)  ((uint8_t )((((x)._flags)>>16)&0x7f))
#define GET_RTP_SEQ(x) ((uint16_t)(((x)._flags)&0xffff))

typedef struct DLLEXP _RTPHeader {
	uint32_t _flags;
	uint32_t _timestamp;
	uint32_t _ssrc;

	operator string() {
		return format("f: %hhx; V: %hhu; P: %hhu; X: %hhu; CC: %hhu; M: %hhu; PT: %hhu; SEQ: %hu; TS: %u; SSRC: %x",
				_flags,
				GET_RTP_V(*this),
				GET_RTP_P(*this),
				GET_RTP_X(*this),
				GET_RTP_CC(*this),
				GET_RTP_M(*this),
				GET_RTP_PT(*this),
				GET_RTP_SEQ(*this),
				_timestamp,
				_ssrc);
	};
} RTPHeader;

#endif  /* _RTPHEADER_H */
#endif /* HAS_PROTOCOL_RTP */

