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


#include "metadata/metadatamanager.h"
#include "application/baseclientapplication.h"
#include "streaming/streamsmanager.h"
#include "streaming/basestream.h"
#include "streaming/baseoutstream.h"
#include "streaming/streamstypes.h"

static string defaultStreamName = "~0~0~0~";

MetadataManager::MetadataManager(BaseClientApplication * pApp) {
	_pApp = pApp;
	_pStreamsManager = 0;	// wait to get StreamsManager
	_mapWarnSize = 100;
}

MetadataManager::~MetadataManager() {
	metadataMap.clear();
}

/// SetMetadata(id) exists for the RTMP notify carrier mechanism
/// - we find the stream name and then call the SetMetadata(streamName) method
//
void MetadataManager::SetMetadata(string mdStr, uint32_t streamId) {
	string streamName;	// default stream name
	if (!GetNameFromId(streamId, streamName)) {
		WARN("SetMetadata can't find stream ID: %d", streamId);
	}
	SetMetadata(mdStr, streamName);
}

/// SetMetadata - rewrite 4/22/15
/// We first check to see if we've already wrapped it.
/// When we wrap it, we start the mess with:  "{"RMS"
//
void MetadataManager::SetMetadata(string mdStr, string streamName) {
	static string prefix = "{\"RMS";	// we tag (wrap) metadata with this
	bool wrapped = 0 == mdStr.compare(0, 5, prefix);	//
	bool all = streamName.size() == 0;	// use all input streams
	//WARN("Enter SetMetdata, wrapped=%s. all=%s", wrapped ? "true" : "false", all ? "true" : "false");
	//
	// make a list of streams to find timestamp and to send metadata to
	map<uint32_t, BaseStream *> inStreams;
	map<uint32_t, BaseStream *>::iterator it;	// we will need this iterator
	if (all) {		// all input streams types
		//WARN("finding Streams by type");
		inStreams = GetStreamsManager()->FindByType(ST_IN, true);
	} else {		// find IN streams that match the name
		//WARN("finding Streams by type by name");
		inStreams = GetStreamsManager()->FindByTypeByName(ST_IN, streamName, true, false);
	}
	//
	if (!wrapped) {	// then we need to get the timestamp and wrap it
		// find the timestamp
		//
		time_t timestamp = 0;
		for (it = inStreams.begin(); !timestamp && it != inStreams.end(); it++) {
			timestamp = (time_t) it->second->GetPtsInt64();
		}
		if (!timestamp) {
			GETMILLISECONDS(timestamp);	// use system time as last resort
		}
		//
		// create and insert our header, then append another "}"
		//
		char buf[64];
		sprintf(buf, "{\"RMS\":{\"type\":\"json\",\"timestamp\":%ld},\"data\":", timestamp);
		mdStr.insert(0, buf);
		mdStr.append("}");
		// done
	}  // if (!wrapped)
	//
	// default our streamName
	//
	if (!streamName.size()) {
		streamName = defaultStreamName;
	}
	// stash it in our local db
	metadataMap[streamName] = mdStr;
	_mostRecentStreamName = streamName;	// stash for default get
	//
	if (metadataMap.size() > _mapWarnSize) {
		FATAL("Metadata Map is getting Bigger! Number entries: %d", (int)_mapWarnSize);
		_mapWarnSize *= 10;
	}
	//
	// send a reference to the application
	//
	_pApp->SetMetadata(mdStr, streamName);
	//
	// sendMetadata to stream(s)
	for (it = inStreams.begin(); it != inStreams.end(); it++) {
		it->second->SendMetadata(mdStr);
	}
	//
	// pushMetadata (local table of pushers)
	if (streamName.size()) {
		PushMetadata(mdStr, streamName);
	}
	// done
}

string MetadataManager::GetMetadata(uint32_t streamId) {
	// find the name, use the streamName polymorph
	string streamName;
	if (streamId) {
		GetNameFromId(streamId, streamName);
	}
	return GetMetadata(streamName);
}

string MetadataManager::GetMetadata(string streamName) {
	// default to most recently added
	if (!streamName.size()) {
		streamName = _mostRecentStreamName;
	}
	if (metadataMap.find(streamName) == metadataMap.end()) {
		return string("");
	}
	return(metadataMap[streamName]);
}
//
// PushStream routines
//
bool MetadataManager::AddPushStream(string streamName, BaseOutStream * pOutStream) {
	//ADD_VECTOR_END(pushStreams.[streamName], pOutStream);
	// default our streamName
	//
	if (!streamName.size()) {
		streamName = defaultStreamName;
	}
	pushStreams[streamName].push_back(pOutStream);
	//INFO("$b2$: AddPushStream list for stream: '%s' now has %d entries",
	//		STR(streamName), (uint32_t)pushStreams[streamName].size());
	return true;
}

vector<BaseOutStream *> MetadataManager::RemovePushStreams(string streamName) {
	vector<BaseOutStream *> result;
	// default our streamName
	//
	if (!streamName.size()) {
		streamName = defaultStreamName;
	}
	if (MAP_HAS1(pushStreams, streamName)) {
		result = pushStreams[streamName];
		MAP_ERASE1(pushStreams, streamName);
		//INFO("$b2$:RemovePushStreams called - Removed stream: %s", STR(streamName));
	}
	return result;
}

void MetadataManager::RemovePushStream(string streamName, BaseOutStream * pOutStream) {
	// default our streamName
	//
	if (!streamName.size()) {
		streamName = defaultStreamName;
	}
	if (MAP_HAS1(pushStreams, streamName)) {
		FOR_VECTOR(pushStreams[streamName], i) {
			BaseOutStream * ps = pushStreams[streamName][i];
			if (ps == pOutStream) {
				//INFO("$b2$:RemovePushStreams called - Removed stream: %s", STR(streamName));
				pushStreams[streamName].erase(pushStreams[streamName].begin()+i);
			}
		}
	}
}

void MetadataManager::PushMetadata(string mdStr, string streamName) {
	// then send to any Added pushStreams
	if (streamName != defaultStreamName) {
		PushOutMetadataStream(mdStr, streamName);
	}
	PushOutMetadataStream(mdStr, defaultStreamName);
}

void MetadataManager::PushOutMetadataStream(string & mdStr, string & streamName) {
	// then send to any Added pushStreams
	if (MAP_HAS1(pushStreams, streamName)) {
		FOR_VECTOR(pushStreams[streamName], i) {
			BaseOutStream * ps = pushStreams[streamName][i];
			if (ps) {
				ps->SendMetadata(mdStr);
			} else {
				FATAL("MetadataManager pushStream catalog is corrupted!!");
			}
		}
	}
}

//
// stream lookup routines
//
StreamsManager * MetadataManager::GetStreamsManager() {
	//ASSERT(_pApp != NULL);
	if (!_pStreamsManager) {
		_pStreamsManager = _pApp->GetStreamsManager();
	}
	//ASSERT(_pStreamsManager != NULL);
	return _pStreamsManager;
}

bool MetadataManager::GetIdFromName(string streamName, uint32_t & id) {
	bool ok = false;
	map<uint32_t, BaseStream *> res;
	res = GetStreamsManager()->FindByName(streamName);
	if (res.size()) {
		map<uint32_t, BaseStream *>::iterator it = res.begin();
		id = it->first;
		ok = true;
	}
	return ok;
}

bool MetadataManager::GetNameFromId(uint32_t id, string & name) {
	BaseStream * pbs = GetStreamsManager()->FindByUniqueId(id);
	if (pbs != NULL) {
		name = pbs->GetName();
	}
	return pbs != NULL;
}


