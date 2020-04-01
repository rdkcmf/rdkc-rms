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



#ifndef _BASEINSTREAM_H
#define	_BASEINSTREAM_H

#include "streaming/basestream.h"

class BaseOutStream;

/*!
	@class BaseInStream
 */
class DLLEXP BaseInStream
: public BaseStream {
private:
	uint32_t _subscriberCount;
protected:
	bool _canCallOutStreamDetached;
	StreamsList _outStreams;
	bool _isVod;
	Variant _publicMetadata;
	IOBuffer _cachedKeyFrame;
	double   _cachedPTS;
	double   _cachedDTS;
	uint32_t _cachedProcLen;
	uint32_t _cachedTotLen;
public:
	BaseInStream(BaseProtocol *pProtocol, uint64_t type, string name);
	virtual ~BaseInStream();

	bool     _hasCachedKeyFrame();
	bool     _consumeCachedKeyFrame(IOBuffer& kfBuffer);
	void     _initKeyFrameCache();
	double   _getCachedPTS();
	double   _getCachedDTS();
	uint32_t _getCachedProcLen();
	uint32_t _getCachedTotLen();

	vector<BaseOutStream *> GetOutStreams();

	/*!
		@brief Returns information about the stream
		@param info
	 */
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);

	/*!
		@brief Links an out-stream to this stream
		@param pOutStream - the out-stream to be linked to this in stream.
		@param reverseLink - if true, pOutStream::Link will be called internally this is used to break the infinite calls.
	 */
	virtual bool Link(BaseOutStream *pOutStream, bool reverseLink = true);

	/*!
		@brief Unlinks an out-stream tot his stream
		@param pOutStream - the out-stream to be unlinked from this in stream.
		@param reverseUnLink - if true, pOutStream::UnLink will be called internally this is used to break the infinite calls
	 */
	virtual void UnLinkAll();
	virtual bool UnLink(BaseOutStream *pOutStream, bool reverseUnLink = true);

	/*!
		@brief This will start the feeding process
		@param dts - the timestamp where we want to seek before start the feeding process
	 */
	virtual bool Play(double dts, double length);

	/*!
		@brief This will pause the feeding process
	 */
	virtual bool Pause();

	/*!
		@brief This will resume the feeding process
	 */
	virtual bool Resume();

	/*!
		@brief This will seek to the specified point in time.
		@param dts
	 */
	virtual bool Seek(double dts);

	/*!
		@brief This will stop the feeding process
	 */
	virtual bool Stop();

	/*!
	 * @brief trigger SignalStreamCapabilitiesChanged on all subscribed streams
	 */
	virtual void AudioStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
			AudioCodecInfo *pNew);
	virtual void VideoStreamCapabilitiesChanged(
			StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
			VideoCodecInfo *pNew);

	/*!
		@brief Called after the link is complete
		@param pOutStream - the newly added stream
	 */
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream) = 0;

	/*!
		@brief Called after the link is broken
		@param pOutStream - the removed stream
	 */
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream) = 0;

	StreamCapabilities * GetCapabilities();

	/*!
	 * @brief should return complete metadata about the stream
	 */
	virtual Variant &GetPublicMetadata();

	/*!
	 *
	 */
	virtual uint32_t GetInputVideoTimescale();
	virtual uint32_t GetInputAudioTimescale();

	virtual bool SendMetadata(string metadataStr);  // we implement this!

	bool IsUnlinkedVod();
	void SignalLazyPullSubscriptionAction(string action);
private:
	void RemoveVodConfig();
};

#endif	/* _BASEINSTREAM_H */

