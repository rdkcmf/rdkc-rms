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


#ifdef HAS_MEDIA_MP3
#ifndef _MP3DOCUMENT_H
#define	_MP3DOCUMENT_H

#include "mediaformats/readers/basemediadocument.h"

class MP3Document
: public BaseMediaDocument {
private:
	//ver/layer/bitRateIndex
	static int32_t _bitRates[4][4][16];
	static int32_t _samplingRates[4][4];
	static string _versionNames[4];
	static string _layerNames[4];
	static map<uint8_t, map<uint8_t, map<uint8_t, map<uint8_t, map<uint8_t, uint64_t > > > > > _frameSizes;
	Variant _tags;
	bool _capabilitiesInitialized;
public:
	MP3Document(Metadata &metadata);
	virtual ~MP3Document();
protected:
	virtual bool ParseDocument();
	virtual bool BuildFrames();
	virtual Variant GetPublicMeta();
private:
	bool FindFrameData();
	bool ParseMetadata();
	void InitFrameSizes();
};


#endif /* _MP3DOCUMENT_H */
#endif /* HAS_MEDIA_MP3 */
