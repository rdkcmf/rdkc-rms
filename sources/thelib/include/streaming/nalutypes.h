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



#ifndef _NALUTYPES_H
#define	_NALUTYPES_H

#include "common.h"

//iso14496-10.pdf Page 60/280
//Table 7-1 – NAL unit type codes
#define NALU_TYPE_UNDEFINED     0
#define NALU_TYPE_SLICE         1
#define NALU_TYPE_DPA           2
#define NALU_TYPE_DPB           3
#define NALU_TYPE_DPC           4
#define NALU_TYPE_IDR           5
#define NALU_TYPE_SEI           6
#define NALU_TYPE_SPS           7
#define NALU_TYPE_PPS           8
#define NALU_TYPE_PD            9 //Access unit delimiter (p.63 T-REC-H.264-201304-S!!PDF-E.pdf)
#define NALU_TYPE_EOSEQ         10
#define NALU_TYPE_EOSTREAM      11
#define NALU_TYPE_FILL          12
#define NALU_TYPE_RESERVED13    13
#define NALU_TYPE_RESERVED14    14
#define NALU_TYPE_RESERVED15    15
#define NALU_TYPE_RESERVED16    16
#define NALU_TYPE_RESERVED17    17
#define NALU_TYPE_RESERVED18    18
#define NALU_TYPE_RESERVED19    19
#define NALU_TYPE_RESERVED20    20
#define NALU_TYPE_RESERVED21    21
#define NALU_TYPE_RESERVED22    22
#define NALU_TYPE_RESERVED23    23

//RFC3984 Page 11/82
//Table 1.  Summary of NAL unit types and their payload structures
#define NALU_TYPE_STAPA         24
#define NALU_TYPE_STAPB         25
#define NALU_TYPE_MTAP16        26
#define NALU_TYPE_MTAP24        27
#define NALU_TYPE_FUA           28
#define NALU_TYPE_FUB           29
#define NALU_TYPE_RESERVED30    30
#define NALU_TYPE_RESERVED31    31

#define NALU_F(x) ((bool)((x)>>7))
#define NALU_NRI(x) ((uint8_t)(((x)>>5)&0x03))
#define NALU_TYPE(x) ((uint8_t)((x)&0x1f))

DLLEXP string NALUToString(uint8_t naluType);

#endif	/* _NALUTYPES_H */


