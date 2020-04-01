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



#include "common.h"
#include "application/baseappprotocolhandler.h"
#include "application/baseclientapplication.h"
#include "eventlogger/eventlogger.h"

BaseAppProtocolHandler::BaseAppProtocolHandler(Variant &configuration) {
	_configuration = configuration;
}

BaseAppProtocolHandler::~BaseAppProtocolHandler() {
}

bool BaseAppProtocolHandler::ParseAuthenticationNode(Variant &node, Variant &result) {
	return false;
}

void BaseAppProtocolHandler::SetApplication(BaseClientApplication *pApplication) {
	_pApplication = pApplication;
}

BaseClientApplication *BaseAppProtocolHandler::GetApplication() {
	return _pApplication;
}

BaseAppProtocolHandler * BaseAppProtocolHandler::GetProtocolHandler(uint64_t protocolType) {
	if (_pApplication == NULL)
		return NULL;
	return _pApplication->GetProtocolHandler(protocolType);
}

bool BaseAppProtocolHandler::PullExternalStream(URI uri, Variant streamConfig) {
	WARN("Pulling in streams for scheme %s in application %s not yet implemented. Stream configuration was:\n%s",
			STR(uri.scheme()),
			STR(GetApplication()->GetName()),
			STR(streamConfig.ToString()));
	return false;
}

bool BaseAppProtocolHandler::PushLocalStream(Variant streamConfig) {
	WARN("Pushing out streams for this protocol handler in application %s not yet implemented.",
			STR(GetApplication()->GetName()));
	return false;
}

EventLogger * BaseAppProtocolHandler::GetEventLogger() {
	//1. Get the event logger from the application
	if (_pApplication != NULL)
		return _pApplication->GetEventLogger();

	//2. Give up. Return the default event logger
	return EventLogger::GetDefaultLogger();
}

StreamMetadataResolver *BaseAppProtocolHandler::GetStreamMetadataResolver() {
	if (_pApplication != NULL)
		return _pApplication->GetStreamMetadataResolver();
	return NULL;
}
