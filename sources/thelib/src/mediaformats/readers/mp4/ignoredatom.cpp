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


#ifdef HAS_MEDIA_MP4
#include "mediaformats/readers/mp4/ignoredatom.h"
#include "mediaformats/readers/mp4/mp4document.h"

IgnoredAtom::IgnoredAtom(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {

}

IgnoredAtom::~IgnoredAtom() {
}

bool IgnoredAtom::IsIgnored() {
	return true;
}

bool IgnoredAtom::Read() {
	return SkipRead(
			(_type != A_SKIP)
			&& (_type != A_FREE)
			&& (_type != A_MDAT)
			&& (_type != A_IODS)
			&& (_type != A_WIDE)
			&& (_type != A_TREF)
			&& (_type != A_TMCD)
			&& (_type != A_TAPT)
			&& (_type != A_STPS)
			&& (_type != A_SDTP)
			&& (_type != A_RTP)
			&& (_type != A_PASP)
			&& (_type != A_LOAD)
			&& (_type != A_HNTI)
			&& (_type != A_HINV)
			&& (_type != A_HINF)
			&& (_type != A_GMHD)
			&& (_type != A_GSHH)
			&& (_type != A_GSPM)
			&& (_type != A_GSPU)
			&& (_type != A_GSSD)
			&& (_type != A_GSST)
			&& (_type != A_GSTD)
			&& (_type != A_ALLF)
			&& (_type != A_SELO)
			&& (_type != A_WLOC)
			&& (_type != A_ALIS)
			&& (_type != A_BTRT)
			&& (_type != A_BTRT)
			&& (_type != A_CHAN)
			&& (_type != A_COLR)
			&& (_type != A_CSLG)
			&& (_type != A_____)
			&& (_type != A_UUID)
			);
}

string IgnoredAtom::Hierarchy(uint32_t indent) {
	return string(4 * indent, ' ') + GetTypeString();
}

#endif /* HAS_MEDIA_MP4 */
