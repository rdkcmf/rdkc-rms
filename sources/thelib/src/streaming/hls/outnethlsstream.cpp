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
#include "streaming/streamstypes.h"
#include "streaming/nalutypes.h"
#include "streaming/baseinstream.h"
#include "protocols/baseprotocol.h"
#include "streaming/hls/outnethlsstream.h"
#include "application/baseclientapplication.h"
#include "protocols/passthrough/passthroughprotocol.h"
#include "protocols/http/httpadaptorprotocol.h"
#include "utils/udpsenderprotocol.h"

OutNetHLSStream::OutNetHLSStream(BaseProtocol *pProtocol, string name,
		Variant &settings)
: BaseOutHLSStream(pProtocol, ST_OUT_NET_TS, name, settings, -1) {
	_lastPatPmtSpsPpsTimestamp = -1;
	_pCurrentSegment = new NetTSSegment(_segmentVideoBuffer,
			_segmentAudioBuffer, _segmentPatPmtAndCountersBuffer);
	_pSender = NULL;
	_enabled = false;
	if ((pProtocol != NULL)&&(pProtocol->GetType() == PT_HTTP_ADAPTOR))
		_pCurrentSegment->SetHTTPProtocol((HTTPAdaptorProtocol *) pProtocol);
}

OutNetHLSStream* OutNetHLSStream::GetInstance(
		BaseClientApplication *pApplication, string name, Variant &settings) {
	//1. Create a dummy protocol

	PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

	//3. Initialize the protocol
	if (!pDummyProtocol->Initialize(settings)) {
		FATAL("Unable to initialize passthrough protocol");
		pDummyProtocol->EnqueueForDelete();
		return NULL;
	}

	//4. Set the application
	pDummyProtocol->SetApplication(pApplication);

	//5. Create the HLS stream
	OutNetHLSStream *pOutNetHLS = new OutNetHLSStream(pDummyProtocol, name, settings);
	if (!pOutNetHLS->SetStreamsManager(pApplication->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pOutNetHLS;
		pOutNetHLS = NULL;
		return NULL;
	}
	pDummyProtocol->SetDummyStream(pOutNetHLS);
	if (!pOutNetHLS->InitUdpSender(settings)) {
		pDummyProtocol->EnqueueForDelete();
		return NULL;
	}

	//6. Done
	return pOutNetHLS;
}

OutNetHLSStream::~OutNetHLSStream() {
	if (_pCurrentSegment != NULL) {
		_pCurrentSegment->ResetUDPSender();
		_pCurrentSegment->ResetHTTPProtocol();
	}
	if (_pSender != NULL) {
		_pSender->ResetReadyForSendInterface();
	}
	if (_pCurrentSegment != NULL) {
		delete _pCurrentSegment;
		_pCurrentSegment = NULL;
	}
	if (_pSender != NULL) {
		_pSender->EnqueueForDelete();
		_pSender = NULL;
	}
}

void OutNetHLSStream::Enable(bool value) {
	_enabled = value;
}

bool OutNetHLSStream::InitUdpSender(Variant &config) {
	_pSender = UDPSenderProtocol::GetInstance(
			"", //bindIp
			0, //bindPort
			config["customParameters"]["localStreamConfig"]["targetUri"]["ip"], //targetIp
			config["customParameters"]["localStreamConfig"]["targetUri"]["port"], //targetPort
			config["customParameters"]["localStreamConfig"]["ttl"], //ttl
			config["customParameters"]["localStreamConfig"]["tos"], //tos
			_pProtocol //pReadyForSend
			);
	if (_pSender == NULL) {
		FATAL("Unable to initialize the UDP sender");
		return false;
	}
	_pCurrentSegment->SetUDPSender(_pSender);
	_nearIp = _farIp = (string) config["customParameters"]["localStreamConfig"]["targetUri"]["ip"];
	_nearPort = _farPort = (uint16_t) config["customParameters"]["localStreamConfig"]["targetUri"]["port"];
	return true;
}

uint16_t OutNetHLSStream::GetUdpBindPort() {
	if (_pCurrentSegment == NULL)
		return 0;
	return _pCurrentSegment->GetUdpBindPort();
}

void OutNetHLSStream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
	if (_pCurrentSegment != NULL) {
		if (!((BaseTSSegment *) _pCurrentSegment)->Init(pCapabilities)) {
			EnqueueForDelete();
		}
	}
}

void OutNetHLSStream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	GenericSignalVideoStreamCapabilitiesChanged(pCapabilities, pOld, pNew);

	// If this is SPS/PPS update, continue
	if (IsVariableSpsPps(pOld, pNew)) {
		return;
	}

	if (_pCurrentSegment != NULL) {
		if (!((BaseTSSegment *) _pCurrentSegment)->Init(pCapabilities)) {
			EnqueueForDelete();
		}
	}
}

bool OutNetHLSStream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutHLSStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}

	Variant dummy;
	if (!_pCurrentSegment->Init(dummy,
			pGenericProcessDataSetup->_pStreamCapabilities)) {
		FATAL("Unable to initialize TS writer");
		return false;
	}

	return true;
}

bool OutNetHLSStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
	Variant& videoParamsSizes) {
	if (!_enabled)
		return true;
	if (!WritePatPmt(dts)) {
		FATAL("Unable to write PAT/PMT/SPS/PPS");
		return false;
	}

	return _pCurrentSegment->PushVideoData(buffer, (int64_t) (pts * 90.0),
		(int64_t)(dts * 90.0), videoParamsSizes);
}

bool OutNetHLSStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	if (!_enabled)
		return true;
	if (!WritePatPmt(dts)) {
		FATAL("Unable to write PAT/PMT/SPS/PPS");
		return false;
	}

	return _pCurrentSegment->PushAudioData(buffer, (int64_t) (pts * 90.0),
			(int64_t) (dts * 90.0));
}

bool OutNetHLSStream::WritePatPmt(double ts) {
	if ((_lastPatPmtSpsPpsTimestamp >= 0)
			&& ((ts - _lastPatPmtSpsPpsTimestamp) < PAT_PMT_INTERVAL)) {
		return true;
	}
	if (!_pCurrentSegment->WritePATPMT()) {
		FATAL("Unable to write PAT/PMT");
		return false;
	}

	_lastPatPmtSpsPpsTimestamp = ts;

	return true;
}

#endif /* HAS_PROTOCOL_HLS */

