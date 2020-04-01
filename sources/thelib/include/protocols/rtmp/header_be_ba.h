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
#ifdef BIG_ENDIAN_BYTE_ALIGNED
#ifndef _HEADER_BE_BA_H
#define	_HEADER_BE_BA_H

#include "common.h"

class IOBuffer;
struct _Channel;

#define HT_FULL 0
#define HT_SAME_STREAM 1
#define HT_SAME_LENGTH_AND_STREAM 2
#define HT_CONTINUATION 3

#define H_HT(x) ((x).ht)
#define H_CI(x) ((x).ci)
#define H_TS(x) ((x).hf.s.ts)
#define H_ML(x) ((x).hf.s.ml)
#define H_MT(x) ((x).hf.s.mt)
#define H_SI(x) ((x).hf.s.si)
#define H_IA(x) ((x).isAbsolute)

typedef struct DLLEXP _Header {
	uint32_t ci;
	uint8_t ht;

	union _hf {

		struct _s {
			uint32_t ts : 32;
			uint32_t ml : 24;
			uint32_t mt : 8;
			uint32_t si : 32;
		} s;
		uint8_t datac[12];
		uint32_t dataw[3];
	} hf;
	bool readCompleted;
	bool isAbsolute;
	bool skip4bytes;

	bool Read(uint32_t channelId, uint8_t type, IOBuffer &buffer,
			uint32_t availableBytes);

	Variant GetVariant();
	static bool GetFromVariant(struct _Header &header, Variant & variant);

	bool Write(struct _Channel &channel, IOBuffer & buffer);
	bool Write(IOBuffer & buffer);

	operator string();
} Header;

#endif	/* _HEADER_BE_BA_H */
#endif /* BIG_ENDIAN_BYTE_ALIGNED */
#endif /* HAS_PROTOCOL_RTMP */

