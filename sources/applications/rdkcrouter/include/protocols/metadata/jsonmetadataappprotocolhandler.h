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
/**
* HISTORY:
  * - ripped from nmeaprotocolhandler.h
* 
**/

//#ifdef HAS_PROTOCOL_JSONMETADATA

#ifndef _JSONMETADATAAPPPROTOCOLHANDLER_H
#define	_JSONMETADATAAPPPROTOCOLHANDLER_H

#include "application/baseappprotocolhandler.h"


/**
 * JsonMetadataAppProtocolHandler does that.
 * Basically set up the stack for a TCP transport.
 * $ToDo: Not sure how we add UDP?
 * @param configuration standard variant of config.lua
 */
//class OriginApplication; // dad, used to stash MetadataObject
#include "application/originapplication.h"


#define SCHEME_JSONMETA "jsonmeta"

/*!
 * JsonMetadataAppProtocolHandler class stands up the metadata input stack
 * $ToDo: define an equivilent TCP push mechanism
 */
class JsonMetadataAppProtocolHandler
: public BaseAppProtocolHandler {
public:
	JsonMetadataAppProtocolHandler(Variant &configuration);
	virtual ~JsonMetadataAppProtocolHandler();

	virtual void RegisterProtocol(BaseProtocol *pProtocol);
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

	virtual bool PullExternalStream(URI uri, Variant streamConfig);

	virtual void SetApplication(BaseClientApplication *pApplication);
protected:
	app_rdkcrouter::OriginApplication * _pApp;
};

#endif	/* _JSONMETADATAAPPPROTOCOLHANDLER_H */

//#endif // HAS_PROTOCOL_NMEA

