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



#ifndef _BASEOUTNETSTREAM_H
#define	_BASEOUTNETSTREAM_H

#include "streaming/baseoutstream.h"

class DLLEXP BaseOutNetStream
	: public BaseOutStream {
protected:
	bool _keyframeCacheConsumed;
	/*!
	* @brief This function can be used to split the audio and video data
	* into meaningful parts. Only works with H264 and AAC content
	*
	* @param pData - the pointer to the chunk of data
	*
	* @param dataLength - the size of pData in bytes
	*
	* @param processedLength - how many bytes of the entire frame were
	* processed so far
	*
	* @param totalLength - the size in bytes of the entire frame
	*
	* @param pts - presentation timestamp
	*
	* @param dts - decoding timestamp
	*
	* @param isAudio - if true, the packet contains audio data. Otherwise it
	* contains video data
	*
	* @return should return true for success or false for errors
	*
	* @discussion dataLength+processedLength==totalLength at all times
	*/
	virtual bool GenericProcessData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio);
private:
	bool _useSourcePts;
public:
	BaseOutNetStream(BaseProtocol *pProtocol, uint64_t type, string name);
	virtual ~BaseOutNetStream();
	bool UseSourcePts();
	virtual void SetKeyFrameConsumed(bool keyFrameStatus);
};

#endif	/* _BASEOUTNETSTREAM_H */

