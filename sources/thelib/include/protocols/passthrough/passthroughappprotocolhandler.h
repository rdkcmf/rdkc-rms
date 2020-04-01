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


#ifndef _PASSTHROUGHAPPPROTOCOLHANDLER_H
#define	_PASSTHROUGHAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"

#define SCHEME_MPEGTSUDP "mpegtsudp"
#define SCHEME_MPEGTSTCP "mpegtstcp"
#define SCHEME_DEEP_MPEGTSUDP "dmpegtsudp"
#define SCHEME_DEEP_MPEGTSTCP "dmpegtstcp"

class DLLEXP PassThroughAppProtocolHandler
: public BaseAppProtocolHandler {
public:
	PassThroughAppProtocolHandler(Variant &configuration);
	virtual ~PassThroughAppProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
	virtual bool PullExternalStream(URI uri, Variant streamConfig);
	virtual bool PushLocalStream(Variant streamConfig);
private:
	bool PullMpegtsUdp(URI &uri, Variant &streamConfig);
	bool PullMpegtsTcp(URI &uri, Variant &streamConfig);
	bool PushMpegtsUdp(BaseInStream *pInStream, Variant streamConfig);
	bool PushMpegtsTcp(BaseInStream *pInStream, Variant streamConfig);
};
#endif	/* _PASSTHROUGHAPPPROTOCOLHANDLER_H */
