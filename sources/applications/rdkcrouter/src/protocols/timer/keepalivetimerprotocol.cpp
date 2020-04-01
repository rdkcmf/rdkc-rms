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


#include "protocols/timer/keepalivetimerprotocol.h"
#include "netio/netio.h"
#include "protocols/protocolmanager.h"
#include "streaming/streamsmanager.h"
#include "streaming/basestream.h"
#include "application/baseclientapplication.h"
#include "eventlogger/eventlogger.h"
using namespace app_rdkcrouter;

KeepAliveTimerProtocol::KeepAliveTimerProtocol()
: BaseTimerProtocol() {
	_pStreamsManager = NULL;
}

KeepAliveTimerProtocol::~KeepAliveTimerProtocol() {
}

void KeepAliveTimerProtocol::SetApplication(BaseClientApplication *pApplication) {
	BaseTimerProtocol::SetApplication(pApplication);
	if (pApplication != NULL) {
		_pStreamsManager = pApplication->GetStreamsManager();
	}
}

bool KeepAliveTimerProtocol::TimePeriodElapsed() {
	if (_pStreamsManager == NULL) {
		WARN("No streams manager available yet");
		return true;
	}
	vector<uint32_t> toBeRemoved;
	vector<uint32_t> toBeDestroyed;
	time_t now = getutctime();

	for (map<uint32_t, pair<StreamsPacketStat, time_t> >::iterator i = _streams.begin();
			i != _streams.end(); i++) {
		BaseStream *pStream = _pStreamsManager->FindByUniqueId(MAP_KEY(i));
		if (pStream == NULL) {
			ADD_VECTOR_END(toBeRemoved, MAP_KEY(i));
			continue;
		}

		pStream->GetStats(_dummy, 0);
		uint64_t newVal = 0;
        int64_t audioPack = -1;
        int64_t videoPack = -1;
		if (_dummy.HasKeyChain(_V_NUMERIC, true, 2, "audio", "packetsCount")) {
			newVal += (uint64_t) _dummy["audio"]["packetsCount"];
            audioPack = (uint64_t) _dummy["audio"]["packetsCount"];
        }
		if (_dummy.HasKeyChain(_V_NUMERIC, true, 2, "video", "packetsCount")) {
			newVal += (uint64_t) _dummy["video"]["packetsCount"];
            videoPack = (uint64_t) _dummy["video"]["packetsCount"];
        }
		uint32_t diff = ((uint32_t) (now - MAP_VAL(i).second))*1000;
		//		FINEST("SI: %"PRIu32"; old: %"PRIu64"; new: %"PRIu64"; diff: %"PRIu32"; max: %"PRIu32,
		//				MAP_KEY(i),
		//				_streams[MAP_KEY(i)].first,
		//				newVal,
		//				diff,
		//				GetTimerPeriodInMilliseconds());
		if ((MAP_VAL(i).first.packetCount == newVal) && (MAP_VAL(i).second != 0) && (pStream && !pStream->IsPaused())) {
			if (diff >= GetTimerPeriodInMilliseconds()) {
				ADD_VECTOR_END(toBeDestroyed, MAP_KEY(i));
				continue;
			}
		}
        if ((audioPack > -1) && (MAP_VAL(i).first.audioPacketCount == (uint64_t)audioPack) 
            && (MAP_VAL(i).second != 0)) {
			if (diff >= GetTimerPeriodInMilliseconds()) {
                // Send the `audioFeedStopped` event here
                GetEventLogger()->LogAudioFeedStopped(pStream);
                continue;
			}
		}
        if ((videoPack > -1) && (MAP_VAL(i).first.videoPacketCount == (uint64_t)videoPack) 
            && (MAP_VAL(i).second != 0)) {
			if (diff >= GetTimerPeriodInMilliseconds()) {
                // Send the `videoFeedStopped` event here
                GetEventLogger()->LogVideoFeedStopped(pStream);
                continue;
			}
		}

        StreamsPacketStat packetVal = {newVal, (uint64_t)audioPack, (uint64_t)videoPack};

		_streams[MAP_KEY(i)] = make_pair(packetVal, now);
//        GetApplication()->GetRecordSession()[pStream->GetName()].ResetFlags();
	}

	for (uint32_t i = 0; i < toBeDestroyed.size(); i++) {
		BaseStream *pStream = _pStreamsManager->FindByUniqueId(toBeDestroyed[i]);
		if (pStream == NULL) {
			continue;
		}
		WARN("No traffic on stream %s. Destroy it", STR(*pStream));
		pStream->EnqueueForDelete();
	}

	for (uint32_t i = 0; i < toBeRemoved.size(); i++) {
		UnRegisterExpireStream(toBeRemoved[i]);
	}

	return true;
}

void KeepAliveTimerProtocol::RegisterExpireStream(uint32_t uniqueId) {
     StreamsPacketStat packetVal = {0, 0, 0};
	_streams[uniqueId] = pair<StreamsPacketStat, time_t > (packetVal, 0);
}

void KeepAliveTimerProtocol::UnRegisterExpireStream(uint32_t uniqueId) {
	_streams.erase(uniqueId);
}
