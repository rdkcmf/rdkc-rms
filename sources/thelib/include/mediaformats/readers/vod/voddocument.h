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
#ifndef _VODDOCUMENT_H
#define	_VODDOCUMENT_H

#include "mediaformats/readers/basemediadocument.h"

class VODDocument
: public BaseMediaDocument {
private:
	Variant _message;
public:
	VODDocument(Metadata &metadata);
	virtual ~VODDocument();
protected:
	virtual bool ParseDocument();
	virtual bool BuildFrames();
	virtual Variant GetPublicMeta();
};

#endif	/* _VODDOCUMENT_H */
#endif /* HAS_MEDIA_VOD */

