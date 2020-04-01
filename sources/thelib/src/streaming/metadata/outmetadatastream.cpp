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
/**
 * OutMetadataStream - handle the Metadata psuedo-stream
 * 
 * ----------------------------------------------------
 * This file links with any instream.
 * It ignores all data and returns true to PushVideoData() and PushAudioData()
 * It outputs on BaseProtool::SendOutOfBandData() using it's own IOBuffer
 */

#include "streaming/metadata/outmetadatastream.h"

OutMetadataStream::OutMetadataStream(BaseProtocol *pProtocol, uint64_t type, string name)
: BaseOutStream(pProtocol, type, name) {
	// ST_OUT_META
	_attached = false;
	_pProtocol = pProtocol;
}

bool OutMetadataStream::SendMetadata(string metadataStr) {
	if (_pProtocol) {
		//IOBuffer * pBuf = _pProtocol->GetOutputBuffer();
		//if (!pBuf) {
		//	FATAL("OutMetadataStream can't get a buffer from Protocol");
		//}
		//pBuf->ReadFromString(metadataStr);
		//_pProtocol->EnqueueForOutbound();
		//
		_ioBuf.ReadFromString(metadataStr);
		_pProtocol->SendOutOfBandData(_ioBuf, NULL);
		_ioBuf.IgnoreAll();
	}else{
    	//FATAL("$b2$: OutMetadataStream::SendMetadata: no Protocol!");
	}
	return true;
}
//
// life cycle calls
//
void OutMetadataStream::SignalAttachedToInStream() {
	//INFO("$b2$ OutMetadataStream: SignalAttachedToInStream called");
	_attached = true;
}
void OutMetadataStream::SignalDetachedFromInStream() {
	//INFO("$b2$ OutMetadataStream: SignalAttachedFrom,InStream called");
	_attached = false;
}
void OutMetadataStream::SignalStreamCompleted() {
	//INFO("$b2$ OutMetadataStream: SignalStreamCompleted called");
	;
}
bool OutMetadataStream::Stop() {
	//INFO("$b2$ OutMetadataStream: Stop() Called!!");
	return true;
}

bool OutMetadataStream::SignalStop() {
	//INFO("$b2$ OutMetadataStream: SignalStop() Called!!");
	return true;
}
