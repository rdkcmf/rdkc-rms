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


#include "protocols/variant/statsvariantappprotocolhandler.h"
#include "protocols/rtmp/basertmpprotocol.h"
#include "protocols/protocolmanager.h"
#include "protocols/protocolfactorymanager.h"
#include "application/originapplication.h"

using namespace app_rdkcrouter;

StatsVariantAppProtocolHandler::StatsReportTimer::StatsReportTimer(StatsVariantAppProtocolHandler *pStatsHandler) {
	_pStatsHandler = pStatsHandler;
}

StatsVariantAppProtocolHandler::StatsReportTimer::~StatsReportTimer() {
	
}

bool StatsVariantAppProtocolHandler::StatsReportTimer::TimePeriodElapsed() {
	_pStatsHandler->BeginStatusReport();
	return true;
}

StatsVariantAppProtocolHandler::StatsVariantAppProtocolHandler(Variant &config)
		: BaseVariantAppProtocolHandler(config) {
	_pendingFlags = 0;
	if (!config.HasKeyChain(_V_NUMERIC, false, 1, "interval")) {
		WARN("No interval for stats specified");
		delete this;
	}
	if (!config.HasKeyChain(V_STRING, false, 1, "url")) {
		WARN("No url for stats specified");
		delete this;
	} else {
		_reportUrl = (string) config["url"];
	}

	if (!config.HasKeyChain(V_STRING, false, 1, "enabledStats")) {
		_queryFlags = ALL_FLAGS;
	} else {
		_queryFlags = 0;
		Variant &enabledStats = config.GetValue("enabledStats", false);
		if (enabledStats.HasKeyChain(V_STRING, false, 1, REPORT_RTMP_OUTBOUND)) {
			_queryFlags |= REPORT_FLAG_RTMP_OUTBOUND;
		}
	}

	ArmTimer((uint32_t) config["interval"]);
}

StatsVariantAppProtocolHandler::~StatsVariantAppProtocolHandler() {
	DisarmTimer();
}

bool StatsVariantAppProtocolHandler::Send(string ip, uint16_t port, Variant &variant) {
	//1. Build the parameters
	Variant parameters;
	parameters["ip"] = ip;
	parameters["port"] = (uint16_t) port;
	parameters["applicationName"] = GetApplication()->GetName();
	parameters["payload"] = variant;

	//2. Start the HTTP request
	if (!TCPConnector<StatsVariantAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetTransport(VariantSerializer_JSON, true, parameters["isSsl"]),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

bool StatsVariantAppProtocolHandler::Send(string url, Variant &variant) {
	//1. Build the parameters
	Variant &parameters = GetScaffold(url);
	if (parameters != V_MAP) {
		FATAL("Unable to get parameters scaffold");
		return false;
	}
	parameters["payload"] = variant;

	//2. Start the HTTP request
	if (!TCPConnector<StatsVariantAppProtocolHandler>::Connect(parameters["ip"],
			parameters["port"],
			GetTransport(VariantSerializer_JSON, true, parameters["isSsl"]),
			parameters)) {
		FATAL("Unable to open connection");
		return false;
	}

	return true;
}

void StatsVariantAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol){

}

void StatsVariantAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {
}

bool StatsVariantAppProtocolHandler::BeginStatusReport() {
	OriginApplication *pApp = (OriginApplication *) GetApplication();
	_reportStatus.Reset();
	_reportStatus["queryFlags"] = _queryFlags;
	pApp->GatherStats(_reportStatus);
	if (!_reportStatus.HasKeyChain(V_MAP, false, 1, "edgeIds"))
		EndStatusReport();
	return true;
}
bool StatsVariantAppProtocolHandler::EndStatusReport() {
	Variant summary;

	if ((_queryFlags & REPORT_FLAG_RTMP_OUTBOUND) > 0) {
		SummarizeRtmpReport(summary);
	}
	
	if (!Send(_reportUrl, summary)) {
		FATAL("Cannot send status report.");
		return false;
	}
	return true;
}

bool StatsVariantAppProtocolHandler::SignalEdgeStatusReport(Variant &message) {
	// TODO:
	uint64_t pid = (uint64_t) message["pid"];
	Variant stats = message["stats"];
	string pid_key = format("%"PRIu64, pid);
	_reportStatus["stats"][pid_key] = stats;
	if (_reportStatus.HasKeyChain(V_MAP, false, 1, "edgeIds") &&
			_reportStatus.HasKeyChain(_V_NUMERIC, false, 2, "edgeIds", STR(pid_key))) {
		_reportStatus["edgeIds"].RemoveKey(pid_key);
	}
	if (_reportStatus["edgeIds"].MapSize() == 0) {
		return EndStatusReport();
	}
	return true;
}

void StatsVariantAppProtocolHandler::ArmTimer(uint32_t duration) {
	DisarmTimer();
	StatsReportTimer *pTimer = new StatsReportTimer(this);
	pTimer->EnqueueForTimeEvent(duration);
	_timerId = pTimer->GetId();
}

void StatsVariantAppProtocolHandler::DisarmTimer() {
	StatsReportTimer *pTimer = (StatsReportTimer *) ProtocolManager::GetProtocol(_timerId);
	if (pTimer == 0)
		return;
	pTimer->EnqueueForDelete();
	_timerId = 0;
}

void StatsVariantAppProtocolHandler::SummarizeRtmpReport(Variant &summary) {
	Variant stats = _reportStatus["stats"];
	double totalBufferSize = 0;
	double totalMaxBuffer = 0;
//	double rtmpOutboundBufferSize = 0;
//	double totalMaxBuffer = 0;

	FOR_MAP(stats, string, Variant, i) {
		if ((_queryFlags & REPORT_FLAG_RTMP_OUTBOUND) > 0) {
			double average = (double) MAP_VAL(i)[REPORT_RTMP_OUTBOUND]["average"];
			double count = (double) MAP_VAL(i)[REPORT_RTMP_OUTBOUND]["count"]; 
			uint32_t maxBuffer = (uint32_t) MAP_VAL(i)[REPORT_RTMP_OUTBOUND]["maxBuffer"];
			/*
			string log = "[DEBUG] Gathered data\n";
			log += format("average: %.3f\n", average);
			log += format("count: %.3f\n", count);
			log += format("maxBuffer: %"PRIu32, maxBuffer);
			WARN("%s", STR(log));
			*/
			double totalEdgeMaxBuffer = (count * ((double)maxBuffer));
			totalMaxBuffer += totalEdgeMaxBuffer;
			totalBufferSize += average * totalEdgeMaxBuffer;
//			rtmpOutboundBufferSize += average * count;
//			totalMaxBuffer += (count * ((double)maxBuffer));
		}
	}
	double averageBuffer = totalMaxBuffer == 0 ? 0 : (totalBufferSize / totalMaxBuffer) * 100.0;

	summary[REPORT_RTMP_OUTBOUND] = averageBuffer;
//	FATAL("[DEBUG] averageBuffer = %.6f", averageBuffer);
//	FATAL("[DEBUG] totalBufferSize = %.6f", totalBufferSize);
//	FATAL("[DEBUG] totalMaxBuffer = %.6f", totalMaxBuffer);
}

