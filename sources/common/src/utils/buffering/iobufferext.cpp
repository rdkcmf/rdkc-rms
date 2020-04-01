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


#include "utils/buffering/iobufferext.h"

IOBufferExt::IOBufferExt() 
: IOBuffer() {
}

IOBufferExt::~IOBufferExt() {
}

bool IOBufferExt::ReadFromU16(uint16_t value, bool networkOrder) {
	if (networkOrder)
		value = ENTOHS(value); //----MARKED-SHORT----

	return ReadFromBuffer((uint8_t *) &value, 2);
}

bool IOBufferExt::ReadFromU32(uint32_t value, bool networkOrder) {
	if (networkOrder)
		value = ENTOHL(value); //----MARKED-LONG---

	return ReadFromBuffer((uint8_t *) &value, 4);
}

bool IOBufferExt::ReadFromU64(uint64_t value, bool networkOrder) {
	if (networkOrder)
		value = ENTOHLL(value);

	return ReadFromBuffer((uint8_t *) &value, 8);
}

bool IOBufferExt::ReadFromRepeat(uint8_t byte, uint32_t size) {
	//TODO: Should we modify the base IoBuffer instead?
	IOBuffer::ReadFromRepeat(byte, size);
	
	return true;
}

