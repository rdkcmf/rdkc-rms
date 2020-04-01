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


#ifndef _CODECINFO_H
#define	_CODECINFO_H

#include "common.h"

#ifdef HAS_G711
#define RTMP_AUDIO_FORMAT_PCMA 7
#define RTMP_AUDIO_FORMAT_PCMU 8

#define RTMP_AUDIO_RATE_5500 0
#define RTMP_AUDIO_RATE_11000 1
#define RTMP_AUDIO_RATE_22000 2
#define RTMP_AUDIO_RATE_44000 3

#define RTMP_AUDIO_SIZE_8 0
#define RTMP_AUDIO_SIZE_16 1

#define RTMP_AUDIO_TYPE_MONO 0
#define RTMP_AUDIO_TYPE_STEREO 1
#endif	/* HAS_G711 */

class BaseInStream;

struct CodecInfo {
	uint64_t _type;
	uint32_t _samplingRate;
	double _transferRate;
	CodecInfo();
	virtual ~CodecInfo();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	virtual void GetRTMPMetadata(Variant & destination);
	virtual operator string();
};

struct VideoCodecInfo : CodecInfo {
	uint32_t _width;
	uint32_t _height;
	uint32_t _frameRateNominator;
	uint32_t _frameRateDenominator;
	double _fps;
	VideoCodecInfo();
	virtual ~VideoCodecInfo();
	double GetFPS();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	virtual void GetRTMPMetadata(Variant & destination);
	virtual operator string();
};

struct VideoCodecInfoPassThrough : VideoCodecInfo {
	IOBuffer _rtmpRepresentation;
	VideoCodecInfoPassThrough();
	virtual ~VideoCodecInfoPassThrough();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	bool Init();
	virtual void GetRTMPMetadata(Variant & destination);
	IOBuffer & GetRTMPRepresentation();
	void SetRTMPRepresentation(const uint8_t *pBuffer, uint32_t length);
};

struct VideoCodecInfoH264 : VideoCodecInfo {
	uint8_t _level;
	uint8_t _profile;
	uint8_t *_pSPS;
	uint32_t _spsLength;
	uint8_t *_pPPS;
	uint32_t _ppsLength;
	IOBuffer _rtmpRepresentation;
	IOBuffer _sps;
	IOBuffer _pps;
	VideoCodecInfoH264();
	virtual ~VideoCodecInfoH264();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	virtual operator string();
	bool Init(uint8_t *pSPS, uint32_t spsLength, uint8_t *pPPS, uint32_t ppsLength,
			uint32_t samplingRate, int16_t a, int16_t b, bool obfuscate);
	virtual void GetRTMPMetadata(Variant & destination);
	IOBuffer & GetRTMPRepresentation();
	IOBuffer & GetSPSBuffer();
	IOBuffer & GetPPSBuffer();
	bool Compare(uint8_t *pSPS, uint32_t spsLength, uint8_t *pPPS,
			uint32_t ppsLength);
};

struct VideoCodecInfoSorensonH263 : VideoCodecInfo {
	uint8_t *_pHeaders;
	uint32_t _length;
	VideoCodecInfoSorensonH263();
	virtual ~VideoCodecInfoSorensonH263();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	virtual operator string();
	bool Init(uint8_t *pHeaders, uint32_t length);
	virtual void GetRTMPMetadata(Variant & destination);
};

struct VideoCodecInfoVP6 : VideoCodecInfo {
	uint8_t *_pHeaders;
	uint32_t _length;
	VideoCodecInfoVP6();
	virtual ~VideoCodecInfoVP6();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	virtual operator string();
	bool Init(uint8_t *pHeaders, uint32_t length);
	virtual void GetRTMPMetadata(Variant & destination);
};

struct AudioCodecInfo : CodecInfo {
	uint8_t _channelsCount;
	uint8_t _bitsPerSample;
	int32_t _samplesPerPacket;
	AudioCodecInfo();
	virtual ~AudioCodecInfo();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	virtual void GetRTMPMetadata(Variant & destination);
	virtual operator string();
};

#ifdef HAS_G711
struct AudioCodecInfoG711 : AudioCodecInfo {
	IOBuffer _rtmpRepresentation;
	AudioCodecInfoG711();
	virtual ~AudioCodecInfoG711();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	bool Init(bool aLaw);
	IOBuffer & GetRTMPRepresentation();
//	virtual void GetRTMPMetadata(Variant & destination);
//	virtual operator string();
};
#endif	/* HAS_G711 */

struct AudioCodecInfoPassThrough : AudioCodecInfo {
	IOBuffer _rtmpRepresentation;
	AudioCodecInfoPassThrough();
	virtual ~AudioCodecInfoPassThrough();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	bool Init();
	virtual void GetRTMPMetadata(Variant & destination);
	IOBuffer & GetRTMPRepresentation();
	void SetRTMPRepresentation(const uint8_t *pBuffer, uint32_t length);
};

struct AudioCodecInfoAAC : AudioCodecInfo {
	uint8_t _audioObjectType;
	uint8_t _sampleRateIndex;
	uint8_t *_pCodecBytes;
	uint8_t _codecBytesLength;
	IOBuffer _rtmpRepresentation;
	AudioCodecInfoAAC();
	virtual ~AudioCodecInfoAAC();
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(IOBuffer & buffer);
	virtual operator string();
	bool Init(uint8_t *pCodecBytes, uint8_t codecBytesLength, bool simple);
	IOBuffer & GetRTMPRepresentation();
	virtual void GetRTMPMetadata(Variant & destination);
	void GetADTSRepresentation(uint8_t *pDest, uint32_t length);
	static void UpdateADTSRepresentation(uint8_t *pDest, uint32_t length);
	bool Compare(uint8_t *pCodecBytes, uint8_t codecBytesLength, bool simple);
};

struct AudioCodecInfoNellymoser : AudioCodecInfo {
	AudioCodecInfoNellymoser();
	virtual ~AudioCodecInfoNellymoser();
	bool Init(uint32_t samplingRate, uint8_t channelsCount, uint8_t bitsPerSample);
	virtual void GetRTMPMetadata(Variant & destination);
};

struct AudioCodecInfoMP3 : AudioCodecInfo {
	AudioCodecInfoMP3();
	virtual ~AudioCodecInfoMP3();
	bool Init(uint32_t samplingRate, uint8_t channelsCount, uint8_t bitsPerSample);
	virtual void GetRTMPMetadata(Variant & destination);
};

class StreamCapabilities {
private:
	VideoCodecInfo *_pVideoTrack;
	AudioCodecInfo *_pAudioTrack;
	double _transferRate;
public:
	StreamCapabilities();
	virtual ~StreamCapabilities();
	void Clear();
	void ClearVideo();
	void ClearAudio();
	void FalseAudioCodecUpdate(BaseInStream *pInStream);
	void FalseVideoCodecUpdate(BaseInStream *pInStream);
	virtual bool Serialize(IOBuffer & buffer);
	virtual bool Deserialize(string & filePath, BaseInStream *pInStream);
	virtual bool Deserialize(const char *pFilePath, BaseInStream *pInStream);
	virtual bool Deserialize(IOBuffer & buffer, BaseInStream *pInStream);
	virtual operator string();

	/*!
	 * @brief Returns the detected transfer rate in bits/s. If the returned value is
	 * less than 0, that menas the transfer rate is not available
	 */
	double GetTransferRate();

	/*!
	 * @brief Sets the transfer rate in bits per second. This will override
	 * bitrate detection from the audio and video tracks
	 *
	 * @param value - the value expressed in bits/second
	 */
	void SetTransferRate(double value);

	/*!
	 * @brief Returns the video codec type or CODEC_VIDEO_UNKNOWN if video
	 * not available
	 */
	uint64_t GetVideoCodecType();

	/*!
	 * @brief Returns the audio codec type or CODEC_AUDIO_UNKNOWN if audio
	 * not available
	 */
	uint64_t GetAudioCodecType();

	/*!
	 * @brief Returns the video codec as a pointer to the specified class
	 */
	template<typename T> T * GetVideoCodec() {
		if (_pVideoTrack != NULL)
			return (T *) _pVideoTrack;
		return NULL;
	}

	/*!
	 * @brief Returns the generic video codec
	 */
	VideoCodecInfo *GetVideoCodec();

	/*!
	 * @brief Returns the audio codec as a pointer to the specified class
	 */
	template<typename T> T * GetAudioCodec() {
		if (_pAudioTrack != NULL)
			return (T *) _pAudioTrack;
		return NULL;
	}

	/*!
	 * @brief Returns the generic audio codec
	 */
	AudioCodecInfo *GetAudioCodec();

	/*!
	 * @brief Pupulates RTMP info metadata
	 *
	 * @param destination where the information will be stored
	 */
	void GetRTMPMetadata(Variant &destination);

	VideoCodecInfoPassThrough * AddTrackVideoPassThrough(
			BaseInStream *pInStream);
	VideoCodecInfoH264 * AddTrackVideoH264(uint8_t *pSPS, uint32_t spsLength,
			uint8_t *pPPS, uint32_t ppsLength, uint32_t samplingRate, int16_t a,
			int16_t b, bool obfuscate, BaseInStream *pInStream);
	VideoCodecInfoSorensonH263 * AddTrackVideoSorensonH263(uint8_t *pData,
			uint32_t length, BaseInStream *pInStream);
	VideoCodecInfoVP6 * AddTrackVideoVP6(uint8_t *pData, uint32_t length,
			BaseInStream *pInStream);

	AudioCodecInfoPassThrough * AddTrackAudioPassThrough(BaseInStream *pInStream);
	AudioCodecInfoAAC * AddTrackAudioAAC(uint8_t *pCodecBytes,
			uint8_t codecBytesLength, bool simple, BaseInStream *pInStream);
	AudioCodecInfoNellymoser * AddTrackAudioNellymoser(uint32_t samplingRate,
			uint8_t channelsCount, uint8_t bitsPerSample, BaseInStream *pInStream);
	AudioCodecInfoMP3 * AddTrackAudioMP3(uint32_t samplingRate,
			uint8_t channelsCount, uint8_t bitsPerSample, BaseInStream *pInStream);
#ifdef HAS_G711
	AudioCodecInfoG711 * AddTrackAudioG711(bool aLaw, BaseInStream *pInStream);
#endif	/* HAS_G711 */
};

#endif	/* _CODECINFO_H */
