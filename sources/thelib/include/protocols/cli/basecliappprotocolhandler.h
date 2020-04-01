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
#ifndef _BASECLIAPPPROTOCOLHANDLER_H
#define	_BASECLIAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"

class BaseProtocol;

class DLLEXP BaseCLIAppProtocolHandler
: public BaseAppProtocolHandler {
public:
	BaseCLIAppProtocolHandler(Variant &configuration);
	virtual ~BaseCLIAppProtocolHandler();

	void RegisterProtocol(BaseProtocol *pProtocol);
	void UnRegisterProtocol(BaseProtocol *pProtocol);

	virtual bool ProcessMessage(BaseProtocol *pFrom, Variant &message) = 0;
protected:
	static bool SendFail(BaseProtocol *pTo, string description, string feedbackId);
	static bool SendSuccess(BaseProtocol *pTo, string description, Variant &data, string feedbackId);
private:
	static bool Send(BaseProtocol *pTo, string status, string description, Variant &data, string feedbackId);
};

#endif	/* _BASECLIAPPPROTOCOLHANDLER_H */
#endif /* HAS_PROTOCOL_CLI */
