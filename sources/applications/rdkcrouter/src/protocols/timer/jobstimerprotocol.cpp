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


#include "protocols/timer/jobstimerprotocol.h"
#include "application/baseclientapplication.h"
#include "application/originapplication.h"
#include "application/rdkcrouter.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"
#include "mediaformats/readers/streammetadataresolver.h"
using namespace app_rdkcrouter;

JobsTimerProtocol::JobsTimerProtocol(bool isMetaFileGenerator) {
    _isMetaFileGenerator = isMetaFileGenerator;
    _serverStartTime = 0;
    _lastServerUpTime = 0;
    _totalReversalTime = 0;
}

JobsTimerProtocol::~JobsTimerProtocol() {

}

bool JobsTimerProtocol::TimePeriodElapsed() {
    if (_isMetaFileGenerator) {
        return DoCreateMetaFiles();
    } else {
        return DoRequests();
    }
}

void JobsTimerProtocol::EnqueueOperation(Variant &request,
        OperationType operationType) {
    switch (operationType) {
        case OPERATION_TYPE_PULL:
        {
            _pullRequests[(uint32_t) request["configId"]] = request;
            break;
        }
        case OPERATION_TYPE_PUSH:
        {
            _pushRequests[(uint32_t) request["configId"]] = request;
            break;
        }
        case OPERATION_TYPE_HLS:
        {
            _hlsRequests[(uint32_t) request["configId"]] = request;
            break;
        }
        case OPERATION_TYPE_HDS:
        {
            _hdsRequests[(uint32_t) request["configId"]] = request;
            break;
        }
        case OPERATION_TYPE_MSS:
        {
            _mssRequests[(uint32_t) request["configId"]] = request;
            break;
        }
        case OPERATION_TYPE_DASH:
        {
            _dashRequests[(uint32_t) request["configId"]] = request;
            break;
        }
        case OPERATION_TYPE_RECORD:
        {
            _recordRequests[(uint32_t) request["configId"]] = request;
            break;
        }
        case OPERATION_TYPE_LAUNCHPROCESS:
        {
            _launchProcessRequests[(uint32_t) request["configId"]] = request;
            break;
        }
		case OPERATION_TYPE_RESTARTSERVICE:
		{
			// TODO: for now use the PORT as the unique identifier...
			//       once we go ipv6 + ipv4, we'll have to revisit
			//       this!
			_restartServiceRequests[(uint32_t) request["port"]] = request;
			break;
		}
        default:
        {
            WARN("Invalid connection type: %d", operationType);
            return;
        }
    }
}

bool JobsTimerProtocol::DoRequests() {
    // We are no longer licensing this software, so clock reversals are not problematic
    //if (!CheckTime())
    //    IOHandlerManager::SignalShutdown();

    //1. Do the pull requests
    map<uint32_t, Variant> temp = _pullRequests;
    _pullRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        GetApplication()->PullExternalStream(MAP_VAL(i));
    }

    //2. Do the push requests
    temp = _pushRequests;
    _pushRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        GetApplication()->PushLocalStream(MAP_VAL(i));
    }

#ifdef HAS_PROTOCOL_HLS
    //3. Do the hls requests
    temp = _hlsRequests;
    _hlsRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        ((OriginApplication *) GetApplication())->CreateHLSStreamPart(MAP_VAL(i));
    }
#endif /* HAS_PROTOCOL_HLS */

#ifdef HAS_PROTOCOL_HDS
    //4. Do the hds requests
    temp = _hdsRequests;
    _hdsRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        ((OriginApplication *) GetApplication())->CreateHDSStreamPart(MAP_VAL(i));
    }
#endif /* HAS_PROTOCOL_HDS */

#ifdef HAS_PROTOCOL_MSS
    //5. Do the mss requests
    temp = _mssRequests;
    _mssRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        ((OriginApplication *) GetApplication())->CreateMSSStreamPart(MAP_VAL(i));
    }
#endif /* HAS_PROTOCOL_MSS */

#ifdef HAS_PROTOCOL_DASH
    //5. Do the dash requests
    temp = _dashRequests;
    _dashRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        ((OriginApplication *) GetApplication())->CreateDASHStreamPart(MAP_VAL(i));
    }
#endif /* HAS_PROTOCOL_DASH */

    //6. Do the record requests
    temp = _recordRequests;
    _recordRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        ((OriginApplication *) GetApplication())->RecordStream(MAP_VAL(i));
    }

    //7. Do the launch process requests
    temp = _launchProcessRequests;
    _launchProcessRequests.clear();

    FOR_MAP(temp, uint32_t, Variant, i) {
        ((OriginApplication *) GetApplication())->LaunchProcess(MAP_VAL(i));
    }

	//8. Do the restart service requests
	temp = _restartServiceRequests;
	_restartServiceRequests.clear();

	FOR_MAP(temp, uint32_t, Variant, i) {
		((OriginApplication *)GetApplication())->RestartService(MAP_VAL(i));
	}

    //8. Done
    return true;
}

bool JobsTimerProtocol::DoCreateMetaFiles() {
    //1. Get the application
    BaseClientApplication *pApplication = GetApplication();
    if (pApplication == NULL) {
        FATAL("Unable to get the application");
        return false;
    }

    StreamMetadataResolver *pSMR = pApplication->GetStreamMetadataResolver();
    if (pSMR == NULL) {
        FATAL("No stream metadata resolver found");
        return false;
    }

    pSMR->GenerateMetaFiles();

    return true;
}

#define OBFUSCATION_MASK 0x108e034b8ff87565LL

bool JobsTimerProtocol::CheckTime() {
    if (_serverStartTime == 0) {
        _serverStartTime = (uint64_t) getutctime();
        return true;
    }
    uint64_t now = (uint64_t) getutctime();
    if (now < _serverStartTime) {
        WARN("Internal issues detected: %"PRIx64"; %"PRIx64,
                (uint64_t) (_serverStartTime^OBFUSCATION_MASK),
                (uint64_t) (now^OBFUSCATION_MASK));
        return false;
    }

    uint64_t serverUpTime = now - _serverStartTime;
    if (_lastServerUpTime == 0) {
        _lastServerUpTime = serverUpTime;
        return true;
    }

    if (serverUpTime < _lastServerUpTime) {
        _totalReversalTime += (_lastServerUpTime - serverUpTime);
        if (_totalReversalTime >= (3600 * 24)) {
            WARN("Internal issues detected: %"PRIx64"; %"PRIx64"; %"PRIx64"; %"PRIx64,
                    (uint64_t) (_serverStartTime^OBFUSCATION_MASK),
                    (uint64_t) (serverUpTime^OBFUSCATION_MASK),
                    (uint64_t) (_lastServerUpTime^OBFUSCATION_MASK),
                    (uint64_t) (_totalReversalTime^OBFUSCATION_MASK));
            return false;
        }
    }

    _lastServerUpTime = serverUpTime;
    return true;
}

