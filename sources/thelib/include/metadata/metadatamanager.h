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

#ifndef __METADATAMANAGER_H
#define __METADATAMANAGER_H

#include "common.h"
#include <string>
class BaseClientApplication;
class StreamsManager;
class BaseOutStream;
/**
 * MetadataManager - stores and serves Metadata Objects
 * Currently the only object stored is a string -
 * with the underlying presumption that it's a JSON object.
 */
class DLLEXP MetadataManager {
public:
	MetadataManager(BaseClientApplication * pApp);
	virtual ~MetadataManager();

	virtual void SetMetadata(string mdStr, uint32_t streamId);
	virtual void SetMetadata(string mdStr, string streamName);

	string GetMetadata(uint32_t streamId);
	string GetMetadata(string streamName);

	bool AddPushStream(string streamName, BaseOutStream * pOutStream);
	vector<BaseOutStream *> RemovePushStreams(string streamName);
	void RemovePushStream(string streamName, BaseOutStream * pOutStream);

	void ClearStream(uint32_t streamId);

	BaseClientApplication * GetApplication() { return _pApp;};

	StreamsManager * GetStreamsManager();

	bool GetIdFromName(string streamName, uint32_t & id);
protected:
	bool GetNameFromId(uint32_t id, string & name);

	void PushMetadata(string mdStr, string streamName);

	void PushOutMetadataStream(string & mdStr, string & streamName);

	void RemJsonKey(string & mdStr, char * key);

	BaseClientApplication * _pApp;
	StreamsManager * _pStreamsManager;

	map<string, string> metadataMap;	// streamID --> metadata string
	map<string, vector<BaseOutStream *> >pushStreams; // set of subcribing streams
private:
	string _mostRecentStreamName;
//	uint32_t _metadataId;	// counter for each new metadata into the system - yeah, it will wrap
	size_t _mapWarnSize;	// moving size to warn of runaway growth
};

#endif // __METADATAMANAGER_H

