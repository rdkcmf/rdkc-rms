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
#include "streaming/hls/outfilehlsstream.h"
#include "streaming/hls/filetssegment.h"
#include "streaming/hls/hlsplaylist.h"
#include "application/baseclientapplication.h"
#include "protocols/passthrough/passthroughprotocol.h"
#ifdef HAS_PROTOCOL_DRM
#include "protocols/drm/basedrmappprotocolhandler.h"
#endif /* HAS_PROTOCOL_DRM */

OutFileHLSStream::OutFileHLSStream(BaseProtocol *pProtocol, string name,
		Variant &settings)
: BaseOutHLSStream(pProtocol, ST_OUT_FILE_HLS, name, settings, 0) {
	_pPlaylist = NULL;
}

OutFileHLSStream* OutFileHLSStream::GetInstance(
		BaseClientApplication *pApplication, string name, Variant &settings) {
	//1. Create a dummy protocol
	PassThroughProtocol *pDummyProtocol = new PassThroughProtocol();

	//2. create the parameters
	Variant parameters;
	parameters["customParameters"]["hlsConfig"] = settings;

	//3. Initialize the protocol
	if (!pDummyProtocol->Initialize(parameters)) {
		FATAL("Unable to initialize passthrough protocol");
		pDummyProtocol->EnqueueForDelete();
		return NULL;
	}

	//4. Set the application
	pDummyProtocol->SetApplication(pApplication);

	//5. Create the HLS stream
	OutFileHLSStream *pOutHLS = new OutFileHLSStream(pDummyProtocol, name, settings);
	if (!pOutHLS->SetStreamsManager(pApplication->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete pOutHLS;
		pOutHLS = NULL;
		return NULL;
	}
	pDummyProtocol->SetDummyStream(pOutHLS);

	//6. Done
	return pOutHLS;
}

OutFileHLSStream::~OutFileHLSStream() {
	ClosePlaylist();
	bool deleteFiles =
			(_pProtocol != NULL)
			&&(_pProtocol->GetCustomParameters().HasKeyChain(V_BOOL, true, 1, "IsTaggedForDeletion"))
			&&((bool)_pProtocol->GetCustomParameters()["IsTaggedForDeletion"]);
	if (deleteFiles) {
		// Delete the files
		deleteFolder((string) _settings["targetFolder"], true);
	}
}

void OutFileHLSStream::SignalDetachedFromInStream() {
	BaseOutHLSStream::SignalDetachedFromInStream();
    if (NULL != _pPlaylist)
	    _pPlaylist->SetStreamRemoved(true);
	ClosePlaylist();
}

void OutFileHLSStream::SignalAudioStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
		AudioCodecInfo *pNew) {
	GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
	if (_pPlaylist != NULL)
		_pPlaylist->ForceCut();
}

void OutFileHLSStream::SignalVideoStreamCapabilitiesChanged(
		StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
		VideoCodecInfo *pNew) {
	GenericSignalVideoStreamCapabilitiesChanged(pCapabilities, pOld, pNew);

	// If this is SPS/PPS update, continue
	if (IsVariableSpsPps(pOld, pNew)) {
		return;
	}
	
	if (_pPlaylist != NULL)
		_pPlaylist->ForceCut();
}

bool OutFileHLSStream::FinishInitialization(
		GenericProcessDataSetup *pGenericProcessDataSetup) {
	if (!BaseOutHLSStream::FinishInitialization(pGenericProcessDataSetup)) {
		FATAL("Unable to finish output stream initialization");
		return false;
	}
	if (_pPlaylist == NULL) {
		_pPlaylist = new HLSPlaylist();
		// Initialize the event logger
		_pPlaylist->SetEventLogger(GetEventLogger());

#ifdef HAS_PROTOCOL_DRM
		BaseProtocol *pProtocol = GetProtocol();
		if (pProtocol == NULL) {
			FATAL("Unable to get protocol");
			return false;
		}
		BaseClientApplication *pApp = pProtocol->GetApplication();
		if (pApp == NULL) {
			FATAL("Unable to get application");
			return false;
		}
		//Skip DRM if no DRM was configured
		BaseAppProtocolHandler *pHandler = pApp->GetProtocolHandler(PT_DRM);
		if (pHandler != NULL) {
			DRMKeys *pDRMQueue = ((BaseDRMAppProtocolHandler *) pHandler)->GetDRMQueue();
			if (pDRMQueue == NULL) {
				FATAL("Unable to get DRM queue");
				return false;
			}
			if (!_pPlaylist->SetDRMQueue(pDRMQueue)) {
				FATAL("DRM queue was set previously");
				return false;
			}
		}
#endif /* HAS_PROTOCOL_DRM */

		if (!_pPlaylist->Init(_settings,
				pGenericProcessDataSetup->_pStreamCapabilities, (string) (*this),
				GetUniqueId())) {
			FATAL("Unable to initialize play list");
			return false;
		}
	}

	return true;
}

bool OutFileHLSStream::PushVideoData(IOBuffer &buffer, double pts, double dts, Variant& videoParamsSizes) {
	return _pPlaylist->PushVideoData(buffer, pts, dts, videoParamsSizes);
}

bool OutFileHLSStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	return _pPlaylist->PushAudioData(buffer, pts, dts);
}

bool OutFileHLSStream::PushMetaData(string const& vmfMetadata, int64_t pts) {
	return _pPlaylist->PushMetaData(vmfMetadata, pts);
}

bool OutFileHLSStream::ClosePlaylist() {
	if (_pPlaylist != NULL) {
		delete _pPlaylist;
		_pPlaylist = NULL;
	}
	return true;
}

#endif /* HAS_PROTOCOL_HLS */
