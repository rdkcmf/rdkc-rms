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
#ifndef _ID3PARSER_H
#define	_ID3PARSER_H

#include "common.h"
#include "mediaformats/readers/mediafile.h"

class ID3Parser {
protected:
	bool _unsynchronisation;
	bool _compression;
	bool _extendedHeader;
	bool _experimentalIndicator;
	bool _footerPresent;
	Variant _metadata;
	uint32_t _majorVersion;
	uint32_t _minorVersion;
public:
	ID3Parser(uint32_t majorVersion, uint32_t minorVersion);
	virtual ~ID3Parser();

	Variant GetMetadata();
	bool Parse(MediaFile &file);
private:
	bool ParseTags(IOBuffer &buffer);
	bool ReadStringWithSize(IOBuffer &buffer, Variant &value, uint32_t size, bool hasEncoding);
	bool ReadStringNullTerminated(IOBuffer &buffer, Variant &value, bool unicode);
	bool ParseTextTag(IOBuffer &buffer, Variant &tag);
	bool ParseUSLT(IOBuffer &buffer, Variant &tag);
	bool ParseAPIC(IOBuffer &buffer, Variant &tag);
	bool ParseCOMM(IOBuffer &buffer, Variant &tag);
	bool ParseUrlTag(IOBuffer &buffer, Variant &tag);
	bool ParseWXXX(IOBuffer &buffer, Variant &tag);
	bool ParseTXXX(IOBuffer &buffer, Variant &tag);
};


#endif	/* _ID3PARSER_H */
#endif /* HAS_MEDIA_MP3 */
