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
#ifndef _INFILERTMPSTREAM_H
#define	_INFILERTMPSTREAM_H

#include "streaming/baseinfilestream.h"

class BaseRTMPProtocol;
class StreamsManager;

class InFileRTMPStream
: public BaseInFileStream {

	class BaseBuilder {
	public:
		BaseBuilder();
		virtual ~BaseBuilder();
		virtual bool BuildFrame(MediaFile *pFile, MediaFrame &mediaFrame,
				IOBuffer &buffer) = 0;
	};

	class AVCBuilder : public BaseBuilder {
	private:
		uint8_t _videoCodecHeaderInit[5];
		uint8_t _videoCodecHeaderKeyFrame[2];
		uint8_t _videoCodecHeader[2];
	public:
		AVCBuilder();
		virtual ~AVCBuilder();
		virtual bool BuildFrame(MediaFile *pFile, MediaFrame &mediaFrame,
				IOBuffer &buffer);
	};

	class AACBuilder : public BaseBuilder {
	private:
		uint8_t _audioCodecHeaderInit[2];
		uint8_t _audioCodecHeader[2];
	public:
		AACBuilder();
		virtual ~AACBuilder();
		virtual bool BuildFrame(MediaFile *pFile, MediaFrame &mediaFrame,
				IOBuffer &buffer);
	};

	class MP3Builder : public BaseBuilder {
	private:

	public:
		MP3Builder();
		virtual ~MP3Builder();
		virtual bool BuildFrame(MediaFile *pFile, MediaFrame &mediaFrame,
				IOBuffer &buffer);
	};

	class PassThroughBuilder : public BaseBuilder {
	public:
		PassThroughBuilder();
		virtual ~PassThroughBuilder();
		virtual bool BuildFrame(MediaFile *pFile, MediaFrame &mediaFrame,
				IOBuffer &buffer);
	};
	BaseBuilder *_pAudioBuilder;
	BaseBuilder *_pVideoBuilder;
	IOBuffer _videoBuffer;
	IOBuffer _audioBuffer;
public:
	InFileRTMPStream(BaseProtocol *pProtocol, string name);
	virtual ~InFileRTMPStream();

	static InFileRTMPStream *GetInstance(BaseRTMPProtocol *pRTMPProtocol,
			StreamsManager *pStreamsManager, Metadata &metadata);

	/*!
	 * @brief Called by the framework to initialize the stream
	 *
	 * @return true on success, false otherwise
	 *
	 * @param metadata - The metadata object used to initialize the stream
	 */
	bool Initialize(Metadata &metadata);

	virtual bool IsCompatibleWithType(uint64_t type);
	virtual bool SignalSeek(double &dts);
	virtual bool SignalStop();
	virtual void SignalOutStreamAttached(BaseOutStream *pOutStream);
	virtual void SignalOutStreamDetached(BaseOutStream *pOutStream);

	uint32_t GetChunkSize();

	virtual bool FeedData(uint8_t *pData, uint32_t dataLength,
			uint32_t processedLength, uint32_t totalLength,
			double pts, double dts, bool isAudio);
	bool StreamCompleted();
protected:
	/*!
	 * @brief called by the framework for each individual frame that needs to
	 * be delivered
	 *
	 * @param frame - the metadata of the frame
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool FeedFrame(MediaFrame &frame, bool dtsAdjusted);
};


#endif	/* _INFILERTMPSTREAM_H */
#endif	/* HAS_PROTOCOL_RTMP */
