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


#include "protocols/passthrough/streaming/innetpassthroughstream.h"
#include "protocols/passthrough/streaming/outnetpassthroughstream.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"
#include "eventlogger/eventlogger.h"

InNetPassThroughStream::InNetPassThroughStream(BaseProtocol *pProtocol, string name)
: BaseInNetStream(pProtocol, ST_IN_NET_PASSTHROUGH, name) {
}

InNetPassThroughStream::~InNetPassThroughStream() {
	GetEventLogger()->LogStreamClosed(this);
	//	FOR_MAP(_outProtocols, uint32_t, BaseProtocol *, i) {
	//		MAP_VAL(i)->EnqueueForDelete();
	//	}
	//	_outProtocols.clear();
}

bool InNetPassThroughStream::IsCompatibleWithType(uint64_t type) {
	return type == ST_OUT_NET_PASSTHROUGH;
}

StreamCapabilities * InNetPassThroughStream::GetCapabilities() {
	return NULL;
}

void InNetPassThroughStream::ReadyForSend() {

}

void InNetPassThroughStream::SignalOutStreamAttached(BaseOutStream *pOutStream) {

}

void InNetPassThroughStream::SignalOutStreamDetached(BaseOutStream *pOutStream) {

}

bool InNetPassThroughStream::SignalPlay(double &dts, double &length) {
	NYIR;
}

bool InNetPassThroughStream::SignalPause() {
	NYIR;
}

bool InNetPassThroughStream::SignalResume() {
	NYIR;
}

bool InNetPassThroughStream::SignalSeek(double &dts) {
	NYIR;
}

bool InNetPassThroughStream::SignalStop() {
	NYIR;
}

bool InNetPassThroughStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	_stats.video.packetsCount++;
	_stats.video.bytesCount += dataLength;
	if (!isAudio) SavePts(pts); // stash timestamp for MetadataManager
	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		if (!pCurrent->value->FeedData(pData, dataLength, processedLength, totalLength,
				pts, dts, isAudio)) {
			if (GetProtocol() == pCurrent->value->GetProtocol()) {
				pCurrent->value->EnqueueForDelete();
				return false;
			} else {
				pCurrent->value->EnqueueForDelete();
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

	return true;
}

bool InNetPassThroughStream::RegisterOutboundUDP(BaseClientApplication *pApplication,
		Variant streamConfig) {
	Variant temp;
	string configUri = streamConfig["customParameters"]["localStreamConfig"]["targetUri"]["originalUri"];

	StreamNode *pCurrent = _outStreams.MoveHead();
	while (pCurrent != NULL) {
		if (pCurrent->value->IsEnqueueForDelete()) {
			pCurrent = _outStreams.MoveNext();
			continue;
		}
		pCurrent->value->GetStats(temp);
		if (temp.HasKeyChain(V_STRING, false, 1, "pushUri")) {
			string uri = (string) temp.GetValue("pushUri", false);
			if (uri == configUri) {
				return true;
			}
		}
		pCurrent = _outStreams.MoveNext();
	}

	//1. Create  dummy protocol
	PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();
	pDummyProtocol->Initialize(streamConfig);
	pDummyProtocol->SetApplication(pApplication);

	//1. Create the outbound UDP stream
	OutNetPassThroughStream *pStream = new OutNetPassThroughStream(
			pDummyProtocol, GetName());
	if (!pStream->InitUdpSink(streamConfig)) {
		FATAL("Unable to init the UDP sink");
		delete pStream;
		pStream = NULL;
		return false;
	}
	if (!pStream->SetStreamsManager(GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pStream;
		pStream = NULL;
		return false;
	}
	pDummyProtocol->SetOutStream(pStream);
	pStream->Enable(true);

	//2. Save it in the list
	//_outProtocols[pDummyProtocol->GetId()] = pDummyProtocol;

	//3. Attach it
	if (!pStream->Link(this)) {
		FATAL("Unable to link the streams");
		pDummyProtocol->EnqueueForDelete();
		return false;
	}
	//	FINEST("streamConfig:\n%s", STR(streamConfig.ToString()));
	//	NYIR;

	//4. Done
	return true;
}
