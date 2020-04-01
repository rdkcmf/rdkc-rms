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


#ifdef HAS_MEDIA_VOD

#include "mediaformats/readers/vod/voddocument.h"
#include "application/clientapplicationmanager.h"

#define MAX_VOD_FILE_SIZE 4096

VODDocument::VODDocument(Metadata &metadata)
: BaseMediaDocument(metadata) {
}

VODDocument::~VODDocument() {
}

bool VODDocument::ParseDocument() {
	//1. Go to the beginning document
	if (!_mediaFile.SeekBegin()) {
		FATAL("Unable to seek in file");
		return false;
	}

	//2. Take no more than 4K worth of data. That is more than enough to
	//store all the info about the stream
	if (_mediaFile.Size() > MAX_VOD_FILE_SIZE) {
		FATAL("Invalid *.vod file size detected");
		return false;
	}
	IOBuffer temp;
	temp.ReadFromRepeat(0, MAX_VOD_FILE_SIZE);
	if (!_mediaFile.ReadBuffer(GETIBPOINTER(temp), _mediaFile.Size())) {
		FATAL("Unable to read the file content");
		return false;
	}
	string content((char *) GETIBPOINTER(temp), (string::size_type)_mediaFile.Size());

	//3. Split the content into kvp and cerate the variant
	replace(content, "\r\n", "\n");
	map<string, string> kvp = mapping(content, "\n", "=", true);
	_message["header"] = "pullStream";
	_message["payload"]["command"] = "pullStream";

	FOR_MAP(kvp, string, string, i) {
		if ((MAP_KEY(i) == "") || (MAP_VAL(i) == ""))
			continue;
		_message["payload"]["parameters"][MAP_KEY(i)] = MAP_VAL(i);
	}

	//4. cleanup the parameters
	Variant &parameters = _message["payload"]["parameters"];
	parameters.IsArray(false);
	string localStreamName = "";
	if (_message.HasKeyChain(V_STRING, false, 1, "localStreamName")) {
		localStreamName = (string) parameters.GetValue("localStreamName", false);
		trim(localStreamName);
	}
	if (localStreamName == "")
		localStreamName = _metadata.completeFileName();
	parameters.RemoveKey("localStreamName", false);
	parameters["localStreamName"] = localStreamName;
	string isKeepAlive = "";
	if (parameters.HasKeyChain(V_STRING, false, 1, "keepAlive")) {
		isKeepAlive = (string) parameters.GetValue("keepAlive", false);
		if (isKeepAlive == "1") {
			parameters["_isVod"] = "0";
		}
	}
	if (isKeepAlive != "1") {
		parameters["_isVod"] = "1";
	}
	parameters.RemoveKey("keepAlive", false);
	parameters["keepAlive"] = "0";
	parameters["_isLazyPull"] = "1"; // indicate that this is a lazyPull operation
#ifdef HAS_RTSP_LAZYPULL
	if (_metadata.HasKey("callback")) {
		parameters["_callback"] = _metadata["callback"];
	}
#endif /* HAS_RTSP_LAZYPULL */
	//5. Broadcast the injected CLI command to whomever has ears to listen
	//This should be intercepted by the rdkcrouter application
	ClientApplicationManager::BroadcastMessageToAllApplications(_message);

	if ((!_message.HasKeyChain(V_BOOL, true, 3, "payload", "parameters", "ok"))
			|| (!((bool)_message["payload"]["parameters"]["ok"]))) {
		WARN("Unable to pull the requested stream from VOD file %s", STR(_metadata.mediaFullPath()));
	}

	//5. Done
	return true;
}

bool VODDocument::BuildFrames() {
	//1. Insert 3 dummy frames to keep the seek file happy
	MediaFrame mf;
	memset(&mf, 0, sizeof (mf));
	_frames.push_back(mf);
	_frames.push_back(mf);
	_frames.push_back(mf);

	return true;
}

Variant VODDocument::GetPublicMeta() {
	Variant result;
	if ((_message.HasKeyChain(V_STRING, false, 4, "payload", "parameters", "uri", "fullUri"))
			&&(_message.HasKeyChain(V_STRING, false, 4, "payload", "parameters", "uri", "scheme"))) {
		result["sourceUri"] = _message["payload"]["parameters"]["uri"]["fullUri"];
		result["scheme"] = _message["payload"]["parameters"]["uri"]["scheme"];
	}
	return result;
}

#endif /* HAS_MEDIA_VOD */
