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




#include "streaming/nalutypes.h"

string NALUToString(uint8_t naluType) {
	switch (NALU_TYPE(naluType)) {
		case NALU_TYPE_SLICE: return "SLICE";
		case NALU_TYPE_DPA: return "DPA";
		case NALU_TYPE_DPB: return "DPB";
		case NALU_TYPE_DPC: return "DPC";
		case NALU_TYPE_IDR: return "IDR";
		case NALU_TYPE_SEI: return "SEI";
		case NALU_TYPE_SPS: return "SPS";
		case NALU_TYPE_PPS: return "PPS";
		case NALU_TYPE_PD: return "PD";
		case NALU_TYPE_EOSEQ: return "EOSEQ";
		case NALU_TYPE_EOSTREAM: return "EOSTREAM";
		case NALU_TYPE_FILL: return "FILL";
		case NALU_TYPE_RESERVED13: return "RESERVED13";
		case NALU_TYPE_RESERVED14: return "RESERVED14";
		case NALU_TYPE_RESERVED15: return "RESERVED15";
		case NALU_TYPE_RESERVED16: return "RESERVED16";
		case NALU_TYPE_RESERVED17: return "RESERVED17";
		case NALU_TYPE_RESERVED18: return "RESERVED18";
		case NALU_TYPE_RESERVED19: return "RESERVED19";
		case NALU_TYPE_RESERVED20: return "RESERVED20";
		case NALU_TYPE_RESERVED21: return "RESERVED21";
		case NALU_TYPE_RESERVED22: return "RESERVED22";
		case NALU_TYPE_RESERVED23: return "RESERVED23";
		case NALU_TYPE_STAPA: return "STAPA";
		case NALU_TYPE_STAPB: return "STAPB";
		case NALU_TYPE_MTAP16: return "MTAP16";
		case NALU_TYPE_MTAP24: return "MTAP24";
		case NALU_TYPE_FUA: return "FUA";
		case NALU_TYPE_FUB: return "FUB";
		case NALU_TYPE_RESERVED30: return "RESERVED30";
		case NALU_TYPE_RESERVED31: return "RESERVED31";
		case NALU_TYPE_UNDEFINED:
		default:
			return "UNDEFINED";
	}
}

