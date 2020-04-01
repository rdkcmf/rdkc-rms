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
 * outvmfprotocol - transport the Video Metafile psuedo-stream
  * ----------------------------------------------------
 **/
#ifndef __OUTVMFPROTOCOL_H__
#define __OUTVMFPROTOCOL_H__

#include "utils/buffering/iobuffer.h"
#include "protocols/baseprotocol.h"
#include "protocols/tcpprotocol.h"

class MetadataManager;
class BaseOutStream;

class DLLEXP OutVmfProtocol
: public BaseProtocol {
public:
	OutVmfProtocol(uint64_t type = PT_OUTBOUND_VMF);
	~OutVmfProtocol() {}
	// main work done in this callback!
	static bool SignalProtocolCreated(BaseProtocol *pProtocol, Variant customParameters);

	virtual bool Initialize(Variant &parameters); // overriden, but I had to call it!??

	virtual bool SendMetadata(string jsonMetaData);
	//
	// over-ride to cause auto remove of Stream from MetadataManager
	virtual void EnqueueForDelete();
	/*!
		@brief Gets the output buffers
	 */
	virtual IOBuffer * GetOutputBuffer() {return &_outBuffer;}
	//
	// pure virtuals to satisfy
	virtual bool AllowFarProtocol(uint64_t type) {return true;}
	virtual bool AllowNearProtocol(uint64_t type) {return true;}
	virtual bool SignalInputData(int32_t recvAmount) {return true;}
	virtual bool SignalInputData(IOBuffer &buffer) {return true;}
protected:
	MetadataManager * _pMM;
	BaseOutStream * _pOutStream;
	IOBuffer _outBuffer;	// output data carrier fetches this
	string _streamName;
	string _outPrefix;	// command to remote VMF module
	string _outPostfix; // command to cause VMF to close & write

	TCPProtocol * _pTcpProtocol;	// for debugging
};

#endif // __OUTVMFPROTOCOL_H__
