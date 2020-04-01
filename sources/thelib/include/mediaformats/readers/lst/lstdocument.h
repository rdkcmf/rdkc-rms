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


#ifdef HAS_MEDIA_LST
#ifndef _LSTDOCUMENT_H
#define	_LSTDOCUMENT_H

#include "mediaformats/readers/basemediadocument.h"

#define MIN_LST_FILE_LIMIT (4096)
#define MAX_LST_FILE_LIMIT (4096 * 100)

class LSTDocument
: public BaseMediaDocument {
public:
	LSTDocument(Metadata &metadata);
	virtual ~LSTDocument();
protected:
	virtual bool ParseDocument();
	virtual bool BuildFrames();
	virtual Variant GetPublicMeta();
};


#endif	/* _LSTDOCUMENT_H */
#endif	/* HAS_MEDIA_LST */
