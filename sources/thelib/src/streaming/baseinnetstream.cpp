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



#include "streaming/baseinnetstream.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"

BaseInNetStream::BaseInNetStream(BaseProtocol *pProtocol, uint64_t type, string name)
: BaseInStream(pProtocol, type, name) {
	if (!TAG_KIND_OF(type, ST_IN_NET)) {
		ASSERT("Incorrect stream type. Wanted a stream type in class %s and got %s",
				STR(tagToString(ST_IN_NET)), STR(tagToString(type)));
	}
}

BaseInNetStream::~BaseInNetStream() {
	_canCallOutStreamDetached = false;
	vector<BaseOutStream *> outStreams = GetOutStreams();
	FOR_VECTOR(outStreams, i) {
		outStreams[i]->SignalStreamCompleted();
	}
}

