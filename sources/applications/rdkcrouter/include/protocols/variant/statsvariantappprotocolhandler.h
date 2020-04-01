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



#include "protocols/variant/basevariantappprotocolhandler.h"
#include "protocols/variant/statusreportconstant.h"
#include "protocols/timer/basetimerprotocol.h"


namespace app_rdkcrouter {

	class StatsVariantAppProtocolHandler
	: public BaseVariantAppProtocolHandler {

	private:
		class StatsReportTimer
		: public BaseTimerProtocol {
		private:
			StatsVariantAppProtocolHandler *_pStatsHandler;
		public:
			StatsReportTimer(StatsVariantAppProtocolHandler *pStatsHandler);
			virtual ~StatsReportTimer();
			virtual bool TimePeriodElapsed();
		};

		friend class StatsReportTimer;
		
		uint32_t _timerId;
		uint64_t _pendingFlags;
		uint64_t _queryFlags;
		Variant _reportStatus;
		string _reportUrl;
	public:
		StatsVariantAppProtocolHandler(Variant &config);
		virtual ~StatsVariantAppProtocolHandler();
		virtual void RegisterProtocol(BaseProtocol *pProtocol);
		virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
		virtual bool Send(string ip, uint16_t port, Variant &variant);
		virtual bool Send(string url, Variant &variant);
		bool BeginStatusReport();
		bool EndStatusReport();
		bool SignalEdgeStatusReport(Variant &message);
	private:
		void ArmTimer(uint32_t duration);
		void DisarmTimer();
		void SummarizeRtmpReport(Variant &summary);
	};

}

