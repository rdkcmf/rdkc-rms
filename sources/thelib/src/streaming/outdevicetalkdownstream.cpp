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



#include "streaming/outdevicetalkdownstream.h"
#include "streaming/streamstypes.h"
#include "protocols/protocolmanager.h"
#include "protocols/baseprotocol.h"
#include "protocols/rtp/connectivity/outboundconnectivity.h"
#include "eventlogger/eventlogger.h"
#include "streaming/codectypes.h"

#ifdef PLAYBACK_SUPPORT
#define DUMP_G711 0

#if DUMP_G711
static FILE *g711Fd;
#endif

#endif

OutDeviceTalkDownStream::OutDeviceTalkDownStream(BaseProtocol *pProtocol,
		string name)
: BaseOutDeviceStream(pProtocol, ST_OUT_NET_RAW, name) {
	_pConnectivity = NULL;
	_hasAudio = false;
	_hasVideo = false;
	_enabled = false;
#ifdef PLAYBACK_SUPPORT
#if DUMP_G711
	INFO("Opening g711 file on talkdown stream");
        g711Fd = fopen("/opt/g711.ulw", "a");
#endif
	pxa = NULL;
#endif
}

OutDeviceTalkDownStream::~OutDeviceTalkDownStream() {
	INFO("OutDeviceTalkDownStream destructor");

// close the open fd and playback channel as the talk down stream destruction can happen while playing back the audio
#ifdef PLAYBACK_SUPPORT

#if DUMP_G711
	INFO("Closing g711 file on talkdown stream");
        fclose(g711Fd);
#endif

    // close the playback channel
    if(NULL != pxa) {
        INFO("******* Closing xaudio pxa %x", pxa);
        xaudio_close(pxa);
        pxa = NULL;
    }

    _audioBuffer.IgnoreAll();

#endif

	GetEventLogger()->LogStreamClosed(this);
}

void OutDeviceTalkDownStream::Enable(bool value) {
#ifdef PLAYBACK_SUPPORT

    // initialize the playback
    if(NULL == pxa) {
	INFO("Initialize the xaudio playback");
	memset(&config, 0, sizeof(rdkc_xaConfig));
	config.codec = rdkx_xaAudoCodec_G711;
	config.format = rdkc_xaG711Format_ULAW;
	config.sample_rate = TALKDOWN_SAMPLE_RATE;
	config.channel_count = TALKDOWN_CHANNEL_COUNT;
	pxa = xaudio_open(&config, rdkc_xaOpenMode_Playback);
    }

#endif

	_enabled = value;
}

OutboundConnectivity *OutDeviceTalkDownStream::GetConnectivity() {
	return _pConnectivity;
}

void OutDeviceTalkDownStream::SetConnectivity(OutboundConnectivity *pConnectivity) {
	_pConnectivity = pConnectivity;
}

void OutDeviceTalkDownStream::HasAudioVideo(bool hasAudio, bool hasVideo) {
	_hasAudio = hasAudio;
	_hasVideo = hasVideo;
}

bool OutDeviceTalkDownStream::SignalPlay(double &dts, double &length) {
	return true;
}

bool OutDeviceTalkDownStream::SignalPause() {
	return true;
}

bool OutDeviceTalkDownStream::SignalResume() {
	return true;
}

bool OutDeviceTalkDownStream::SignalSeek(double &dts) {
	return true;
}

bool OutDeviceTalkDownStream::SignalStop() {
	return true;
}

void OutDeviceTalkDownStream::SignalDetachedFromInStream() {
	//_pConnectivity->SignalDetachedFromInStream();
}

void OutDeviceTalkDownStream::SignalStreamCompleted() {
	EnqueueForDelete();
}

void OutDeviceTalkDownStream::SignalAttachedToInStream() {
}

void OutDeviceTalkDownStream::SignalAudioStreamCapabilitiesChanged(
                StreamCapabilities *pCapabilities, AudioCodecInfo *pOld,
                AudioCodecInfo *pNew) {
        GenericSignalAudioStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
        if ((pNew == NULL) || (!IsCodecSupported(pNew->_type))) {
                _pAudioInfo = NULL;
                _audioSampleRate = 1;
        }
        _pAudioInfo = pNew;
        _audioSampleRate = (_pAudioInfo != NULL) ? _pAudioInfo->_samplingRate : 0;
}

void OutDeviceTalkDownStream::SignalVideoStreamCapabilitiesChanged(
                StreamCapabilities *pCapabilities, VideoCodecInfo *pOld,
                VideoCodecInfo *pNew) {
        GenericSignalVideoStreamCapabilitiesChanged(pCapabilities, pOld, pNew);
        if ((pNew == NULL) || (!IsCodecSupported(pNew->_type))) {
                _pVideoInfo = NULL;
                _videoSampleRate = 1;
        }
        _pVideoInfo = pNew;
        _firstVideoFrame = true;
        _videoSampleRate = (_pVideoInfo != NULL) ? _pVideoInfo->_samplingRate : 0;
}

bool OutDeviceTalkDownStream::PushVideoData(IOBuffer &buffer, double pts, double dts,
                bool isKeyFrame) {
        return false;
}

bool OutDeviceTalkDownStream::PushAudioData(IOBuffer &buffer, double pts, double dts) {
	// TODO
	return false;
}

bool OutDeviceTalkDownStream::IsCodecSupported(uint64_t codec) {
        return (codec == CODEC_VIDEO_H264)
                        || (codec == CODEC_AUDIO_AAC)
#ifdef HAS_G711
                        || ((codec & CODEC_AUDIO_G711) == CODEC_AUDIO_G711)
#endif  /* HAS_G711 */
                        ;
}

bool OutDeviceTalkDownStream::FeedData(uint8_t *pData, uint32_t dataLength,
		uint32_t processedLength, uint32_t totalLength,
		double pts, double dts, bool isAudio) {
	if (!_enabled)
		return true;
	// TODO
	//INFO("Feeding the data to camera speaker");
#ifdef PLAYBACK_SUPPORT

    _audioBuffer.ReadFromBuffer(pData, dataLength);

    if(_audioBuffer._published >= 1024) {
	if(NULL != pxa) {
		xaudio_playback(GETIBPOINTER(_audioBuffer), 1024, pxa);
#if DUMP_G711
		fwrite(GETIBPOINTER(_audioBuffer), 1024, 1, g711Fd);
		//INFO("Dumping G711 data to file");
#endif
		_audioBuffer._consumed = 1024;
		_audioBuffer.MoveData();
	}
    }

#endif
	return true;
}
