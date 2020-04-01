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




#ifdef HAS_PROTOCOL_RTMP
#ifndef _SOMANAGER_H
#define	_SOMANAGER_H

#include "common.h"

class BaseRTMPProtocol;
class SO;

class DLLEXP SOManager {
private:
	map<string, SO *> _sos;
	map<uint32_t, vector<SO *> > _protocolSos;
public:
	SOManager();
	virtual ~SOManager();
public:
	void UnRegisterProtocol(BaseRTMPProtocol *pProtocol);
	bool Process(BaseRTMPProtocol *pFrom, Variant &request);
	SO * GetSO(string name, bool persistent);
	bool ContainsSO(string name);
private:
	bool ProcessFlexSharedObject(BaseRTMPProtocol *pFrom, Variant &request);
	bool ProcessSharedObject(BaseRTMPProtocol *pFrom, Variant &request);
	bool ProcessSharedObjectPrimitive(BaseRTMPProtocol *pFrom, SO *pSO,
			string name, Variant &request, uint32_t primitiveId);
};


#endif	/* _SOMANAGER_H */

#endif /* HAS_PROTOCOL_RTMP */

