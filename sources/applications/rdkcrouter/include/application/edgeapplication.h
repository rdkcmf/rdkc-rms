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


#ifndef _EDGEAPPLICATION_H
#define	_EDGEAPPLICATION_H

#include "application/baseclientapplication.h"

class RpcSink;
class BaseRPCAppProtocolHandler;

namespace app_rdkcrouter {
	class RTMPAppProtocolHandler;
	class EdgeVariantAppProtocolHandler;
	class JobsTimerAppProtocolHandler;
	class HTTPAppProtocolHandler;

	class DLLEXP EdgeApplication
	: public BaseClientApplication {
	private:
		vector<RpcSink *> _pRPCSinks;
		HTTPAppProtocolHandler *_pHTTPHandler;
		BaseRPCAppProtocolHandler *_pRPCHandler;
		RTMPAppProtocolHandler *_pRTMPHandler;
		EdgeVariantAppProtocolHandler *_pVariantHandler;
		JobsTimerAppProtocolHandler *_pTimerHandler;
		uint32_t _retryTimerProtocolId;
		uint32_t _pushId;
		Variant _dummy;
	public:
		EdgeApplication(Variant &configuration);
		virtual ~EdgeApplication();
		virtual bool Initialize();
		virtual bool ActivateAcceptor(IOHandler *pIOHandler);
		virtual void SignalStreamRegistered(BaseStream *pStream);
		virtual void SignalStreamUnRegistered(BaseStream *pStream);
		virtual void RegisterProtocol(BaseProtocol *pProtocol);
		virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
		virtual void SignalUnLinkingStreams(BaseInStream *pStream, BaseOutStream *pOutStream);
		virtual void SignalApplicationMessage(Variant &message);
		void SignalOriginInpuStreamAvailable(string streamName);
		virtual void SignalEventSinkCreated(BaseEventSink *pSink);
		void InsertPlaylistItem(string &playlistName, string &localStreamName,
				double insertPoint, double sourceOffset, double duration);
		void GatherStats(uint64_t flags, Variant &reportInfo);
	private:
		void EnqueueTimedOperation(Variant &request);
		uint8_t GetClusterStreamDirection(uint64_t streamType,
				BaseProtocol *pProtocol, Variant &customParameters);
		bool ProcessIngestPointsList();
	};
}

#endif	/* _EDGEAPPLICATION_H */

