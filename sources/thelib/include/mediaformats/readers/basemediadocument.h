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


#ifndef _BASEMEDIADOCUMENT_H
#define	_BASEMEDIADOCUMENT_H


#include "common.h"
#include "mediaformats/readers/mediaframe.h"
#include "mediaformats/readers/mediafile.h"
#include "streaming/streamcapabilities.h"
#include "mediaformats/readers/streammetadataresolver.h"

class BaseMediaDocument {
protected:
	MediaFile _mediaFile;
	vector<MediaFrame> _frames;
	uint32_t _audioSamplesCount;
	uint32_t _videoSamplesCount;
	Metadata &_metadata;
	string _mediaFilePath;
	string _seekFilePath;
	string _metaFilePath;
	bool _keyframeSeek;
	uint32_t _seekGranularity;
	StreamCapabilities _streamCapabilities;
public:
	BaseMediaDocument(Metadata &metadata);
	virtual ~BaseMediaDocument();

	/*!
		@brief This functions do things like opening the media file, building frames, saving the meta, etc.
	 */
	bool Process();

	/*!
		@brief Returns the meta data
	 */
	Variant GetMetadata();

	/*!
		@brief Returns the media file
	 */
	MediaFile &GetMediaFile();
protected:
	static bool CompareFrames(const MediaFrame &frame1, const MediaFrame &frame2);
	virtual bool ParseDocument() = 0;
	virtual bool BuildFrames() = 0;
	virtual Variant GetPublicMeta() = 0;
private:
	bool SaveSeekFile();
	bool SaveMetaFile();
};


#endif	/* _BASEMEDIADOCUMENT_H */

