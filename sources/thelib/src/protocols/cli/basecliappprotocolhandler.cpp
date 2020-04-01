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


#ifdef HAS_PROTOCOL_CLI
#include "protocols/cli/basecliappprotocolhandler.h"
#include "protocols/cli/inboundbasecliprotocol.h"
#include "eventlogger/eventlogger.h"

BaseCLIAppProtocolHandler::BaseCLIAppProtocolHandler(Variant &configuration)
: BaseAppProtocolHandler(configuration) {

}

BaseCLIAppProtocolHandler::~BaseCLIAppProtocolHandler() {
}

void BaseCLIAppProtocolHandler::RegisterProtocol(BaseProtocol *pProtocol) {

}

void BaseCLIAppProtocolHandler::UnRegisterProtocol(BaseProtocol *pProtocol) {

}

bool BaseCLIAppProtocolHandler::SendFail(BaseProtocol *pTo, string description, string feedbackId) {
	Variant dummy;
	return Send(pTo, "FAIL", description, dummy, feedbackId);
}

bool BaseCLIAppProtocolHandler::SendSuccess(BaseProtocol *pTo, string description, Variant &data, string feedbackId) {
	return Send(pTo, "SUCCESS", description, data, feedbackId);
}

bool BaseCLIAppProtocolHandler::Send(BaseProtocol *pTo, string status, string description, Variant &data, string feedbackId) {
	if (pTo == NULL)
		return true;
	//1. Prepare the final message
	Variant message;
	if (feedbackId != "") message["responseId"] = feedbackId;
	message["status"] = status;
	message["description"] = description;
	message["data"] = data;

	//2. Log results
	pTo->GetEventLogger()->LogCLIResponse(message);

	//3. Send it
	bool result;
	switch (pTo->GetType()) {
		case PT_INBOUND_JSONCLI:
		{
			result = ((InboundBaseCLIProtocol *)pTo)->SendMessage(message);
			if ((pTo->GetFarProtocol())->GetType() == PT_HTTP_4_CLI) {
				// If this is an http based CLI, enqueue it for delete
				pTo->GracefullyEnqueueForDelete();
			}
			return result;
		}
#ifdef HAS_PROTOCOL_ASCIICLI
		case PT_INBOUND_ASCIICLI:
			return ((InboundBaseCLIProtocol *) pTo)->SendMessage(message);
#endif /* HAS_PROTOCOL_ASCIICLI */
		default:
			WARN("Protocol %s not supported yet", STR(tagToString(pTo->GetType())));
			return false;
	}
}
#endif /* HAS_PROTOCOL_CLI */
