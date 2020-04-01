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



#ifndef _BIG_ENDIAN_SHORT_ALIGNED_H
#define	_BIG_ENDIAN_SHORT_ALIGNED_H

// For A Big Endian ARM system this should be htonll(x) (x)
#define htonll(x)   (x)
#define ntohll(x)   (x)

//adobe shit. Sometimes, Adobe's reprezentation of 0x0a0b0c0d
//in "network-order" is like this: 0b 0c 0d 0a
#define htona(x)    (((x)<<8)|((x)>>24))
#define ntoha(x)    (((x)>>8)|((x)<<24))

// Pointer to value defines
#define ptovh(x) ((uint16_t)(*((uint8_t*)(x))))
#define ptovl(x) ((uint32_t)(*((uint8_t*)(x))))
#define ptovll(x) ((uint64_t)(*((uint8_t*)(x))))

// These will work with both BE and LE systems
#define ntohsp(x) ((ptovh(x) << 8) | (ptovh(x + 1)))
#define ntohlp(x) ((ptovl(x) << 24) | (ptovl(x + 1) << 16) | (ptovl(x + 2) << 8) | (ptovl(x + 3)))
#define ntohap(x) ((ptovl(x) << 16) | (ptovl(x + 1) << 8) | (ptovl(x + 2)) | (ptovl(x + 3) << 24))
#define ntohllp(x) \
		((ptovll(x) << 56) | (ptovll(x + 1) << 48) | (ptovll(x + 2) << 40) | (ptovll(x + 3) << 32) | \
		(ptovll(x + 4) << 24) | (ptovll(x + 5) << 16) | (ptovll(x + 6) << 8) | (ptovll(x + 7)))

#error put_htons in big_endian_short_aligned.h needs testing
/* These should work as-is for both BE and LE systems */

#define put_htons(x,v) {\
    (*(((uint8_t*)(x))) = (uint8_t)((v) >> 8));\
    (*(((uint8_t*)(x))+1) = (uint8_t)(v));\
    }

#define put_htonl(x,v) {\
    (*(((uint8_t*)(x))) = (uint8_t)((v) >> 24));\
    (*(((uint8_t*)(x))+1) = (uint8_t)((v) >> 16));\
    (*(((uint8_t*)(x))+2) = (uint8_t)((v) >> 8));\
    (*(((uint8_t*)(x))+3) = (uint8_t)(v));\
    }

/* TODO: Do we need put_value methods for aligned access when not swapping bytes */
/* TODO: Do we need put_htonll() or put_htona() */

#endif	/* _BIG_ENDIAN_SHORT_ALIGNED_H */


