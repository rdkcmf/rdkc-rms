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


#ifdef HAS_PROTOCOL_HLS
#include "streaming/hls/filesegment.h"
#include "streaming/nalutypes.h"

FileSegment::FileSegment() {
}

FileSegment::~FileSegment() {
	if (_file.IsOpen()) {
		_file.Close();
	}
}

bool FileSegment::Initialize(Variant &settings, StreamCapabilities *pCapabilities,
	unsigned char *pKey, unsigned char *pIV) {
		//1. Open the file
		if (!settings.HasKeyChain(V_STRING, true, 1, "computedPathToFile")) {
			FATAL("computedPathToFile element not found in settings");
			return false;
		}
        bool isFileOpen = ((bool) settings["useByteRange"]) && _file.IsOpen();

		if (!isFileOpen && !_file.Initialize(settings["computedPathToFile"], FILE_OPEN_MODE_TRUNCATE)) {
			FATAL("Unable to open file %s", STR(settings["computedPathToFile"]));
			return false;
		}
		return true;
}

bool FileSegment::WritePacket(uint8_t *pBuffer, uint32_t size) {
	_file.WriteBuffer(pBuffer, size);
	_file.Flush();
	return true;
}

string FileSegment::GetPath() {
	return _file.GetPath();
}

#endif /* HAS_PROTOCOL_HLS */
