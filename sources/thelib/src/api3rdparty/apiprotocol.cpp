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

#ifdef HAS_PROTOCOL_API
#include "api3rdparty/apiprotocol.h"
#include "api3rdparty/g711.h"
#include "utils/buffering/iobufferext.h"
#include "threading/threading.h"
#include <sys/stat.h>

#ifdef RMS_PLATFORM_RPI
#include "api3rdparty/gst_rmsframe.h"
#endif

#define DUMP_G711 0

#define DUMP_PCM 0

#if DUMP_G711
FILE *g711Fd;
#endif

#if DUMP_PCM
FILE *pcmFd;
#endif

static int is_gst_enabled = RDKC_FAILURE;

RdkCPluginFactory* ApiProtocol::plugin_factory;
RdkCVideoCapturer* ApiProtocol::recorder;

ApiProtocol::ApiProtocol() : BaseProtocol(PT_API_INTEGRATION) {
	_streamId = 0;
	_streamFd = 0;
	_pIoHandler = NULL;
#ifndef SDK_DISABLED
	_soxr = NULL;
        is_soxr_initialized = false;
#endif /* SDK_DISABLED */
#ifdef HAS_THREAD
	_feeding = false;
#endif /* HAS_THREAD */
}

ApiProtocol::~ApiProtocol() {
	Terminate();
}

bool ApiProtocol::Initialize(Variant &config) {

        int ret = true;
#if !defined ( RMS_PLATFORM_RPI )
        conf = new camera_resource_config_t;
        plugin_factory = CreatePluginFactoryInstance();
        if( NULL == plugin_factory ) {
	    FATAL("%s(%d): initialization of temp factory failed",  __FILE__, __LINE__);
            ret = false;
        }
        else {
            recorder = ( RdkCVideoCapturer* )plugin_factory->CreateVideoCapturer();
            _videoConfig = new video_stream_config_t;
            _audioConfig = new audio_stream_config_t;
            if( NULL == recorder ) {
	        FATAL("%s(%d): initialization of video plugins failed",  __FILE__, __LINE__);
                ret = false;
            }
        }
	INFO("%s(%d): Recoder is created successfully",  __FILE__, __LINE__);
#endif
	return ret;
}

IOHandler *ApiProtocol::GetIOHandler() {
	return _pIoHandler;
}

void ApiProtocol::SetIOHandler(IOHandler *pIOHandler) {
	if (pIOHandler != NULL) {
		if (pIOHandler->GetType() != IOHT_API_FD) {
			ASSERT("This protocol accepts only API handlers!");
		}
	}
	_pIoHandler = pIOHandler;
}

#ifndef SDK_DISABLED
/* Initialize Sound Exchange Resampler */
void ApiProtocol::InitializeSoXR() {

	double const irate = 16000.0;
	double const orate = 8000.0;

	/* for io spec */
	io_spec = soxr_io_spec(SOXR_INT16_I, SOXR_INT16_I);

	/* for quality spec */
	/*
        SOXR_QQ                 0 'Quick' cubic interpolation.
        SOXR_LQ                 1 'Low' 16-bit with larger rolloff.
        SOXR_MQ                 2 'Medium' 16-bit with medium rolloff.
        SOXR_HQ                 SOXR_20_BITQ 'High quality'.
        SOXR_VHQ                SOXR_28_BITQ 'Very high quality'.
        SOXR_16_BITQ            3
        SOXR_20_BITQ            4
        SOXR_24_BITQ            5
        SOXR_28_BITQ            6
        SOXR_32_BITQ            7
        SOXR_LSR0Q              8 'Best sinc'.
        SOXR_LSR1Q              9 'Medium sinc'.
        SOXR_LSR2Q              10 'Fast sinc'.
        */
        quality =  SOXR_VHQ;
        qt_spec = soxr_quality_spec(quality, 0);

	/* for runtime spec */
        rt_spec = soxr_runtime_spec(1);
	/* Create a stream resampler: */
        _soxr = soxr_create(
                irate, orate, 1,             /* Input rate, output rate, # of channels. */
                &error,                         /* To report any error during creation. */
                &io_spec,
                &qt_spec,
                &rt_spec );

	if(_soxr) {
		INFO("Initialized Sound Exchange Resampler successfully _SoXR: %x", _soxr);
		is_soxr_initialized = true;
	}
	else {
		INFO("Initialization of Sound Exchange Resampler failed _SoXR: %x", _soxr);
		is_soxr_initialized = false;
	}


}
#endif/* SDK_DISABLED */

bool ApiProtocol::InitializeApi(Variant &config) {
	_streamId = (uint32_t) config["apiStreamProfileId"];
	_streamName = (string) config["localStreamName"];
	_outputBuffer.IgnoreAll();
	bool force_audio_config =  false;

	FINEST("API initialized with: %s", STR(config.ToString()));

#ifdef RMS_PLATFORM_RPI
       //is_gst_enabled = IsGSTEnabledInRFC( (char*)RFC_RMS, (char*)GST_FLAG );
       is_gst_enabled = RDKC_SUCCESS;
#endif/* RMS_PLATFORM_RPI */

       if( is_gst_enabled != RDKC_SUCCESS )
        {

	// Initialize the inbound stream, set the defaults
#ifndef SDK_DISABLED
	memset(_videoConfig, 0, sizeof(video_stream_config_t));
	INFO("Invoking getVideoStreamConfing");
	if( recorder->GetVideoStreamConfig((_streamId & VIDEO_MASK), _videoConfig) < 0 ) {
		WARN("getVideoStreamConfig returned <0");
	}
	INFO("InitializeApi : Existing video configurations");
	INFO("Video stream ID=%" PRIu32, (_streamId & VIDEO_MASK));
	INFO("stream_type=%" PRIu32, _videoConfig->stream_type);
	INFO("width=%" PRIu32, _videoConfig->width);
	INFO("height=%" PRIu32, _videoConfig->height);
	INFO("frame_rate=%" PRIu32, _videoConfig->frame_rate);
	INFO("gov_length=%" PRIu32, _videoConfig->gov_length);
	INFO("profile=%" PRIu32, _videoConfig->profile);
	INFO("quality_type=%" PRIu32, _videoConfig->quality_type);
	INFO("quality_level=%" PRIu32, _videoConfig->quality_level);
	INFO("bit_rate=%" PRIu32, _videoConfig->bit_rate);

	// No need to initialize, we rely on the web admin page
	/*
	if (rdkc_stream_set_config(_streamId, &_videoConfig) < 0) {
		FATAL("rdkc_stream_get_config error!");
		return false;
	}
	*/

	if( !UpdateStreamConfig(config) ) {
		INFO("Did not update stream config");
	}

	memset(_audioConfig, 0, sizeof(audio_stream_config_t));
	INFO("Invoking StreamConfig for audio");
#ifdef HAS_G711
	/* get audio configuration of LPCM */
	if( recorder->GetAudioStreamConfig((((_streamId & AUDIO_MASK) & AUDIO_PCM) >> NIBBLE_LENGTH), _audioConfig) < 0) {
#elif HAS_AAC
	if( recorder->GetAudioStreamConfig(((_streamId & AUDIO_MASK) >> NIBBLE_LENGTH), _audioConfig) < 0) {
#endif
		WARN("StreamConfig for audio returned <0");
	}
	INFO("audio Stream ID=%" PRIu32, (((_streamId & AUDIO_MASK) & AUDIO_PCM) >> NIBBLE_LENGTH));
	INFO("audio enable=%" PRIu32, _audioConfig->enable);
	INFO("audio type=%" PRIu32, _audioConfig->type);
	INFO("audio samplerate=%" PRIu32, _audioConfig->sample_rate);
	if((_audioConfig->enable != 1) || (_audioConfig->sample_rate != SAMPLE_RATE_16K)) {
		force_audio_config =  true;
		INFO("Need to enable the audio configuration forcefully");
	}

#ifdef HAS_G711
	/* Enable audio only if type PCM as rdkc streaming support is not avaialble for G711 */
        if( (AUDIO_G711 == (_streamId & AUDIO_MASK) ) &&
            (AUDIO_CODEC_TYPE_LPCM == _audioConfig->type) ) {
		if(force_audio_config) {
			_audioConfig->enable = 1;
			_audioConfig->sample_rate = SAMPLE_RATE_16K;
		}

                if (force_audio_config && recorder->SetAudioStreamConfig((((_streamId & AUDIO_MASK) & AUDIO_PCM) >> NIBBLE_LENGTH), _audioConfig) < 0) {
                        FATAL("Error while enabling LPCM Audio using setAudioStreamConfig() for stream id 0x%x, switching to video only.", _streamId);
                        INFO("Invoking stream init with stream 0x%x.", (_streamId & VIDEO_MASK));
                        conf->KeyValue = (_streamId & VIDEO_MASK) + MAX_ENCODE_STREAM_NUM;
                        av_flag = 1;
                        recorder->StreamInit( conf, av_flag); // Video only
                        _streamFd = recorder->getStreamHandler(av_flag);
                }
                else {
                        INFO("setAudioStreamConfig for stream id 0x%x: Enable LPCM audio with 16k sample rate.", _streamId);
                        INFO("Invoking stream init with stream 0x%x.", _streamId & VIDEO_MASK);
                        conf->KeyValue = (_streamId & VIDEO_MASK) + MAX_ENCODE_STREAM_NUM;
                        av_flag = 3;
                        recorder->StreamInit( conf, av_flag); // Video only
                        _streamFd = recorder->getStreamHandler(av_flag);

			// Iniatilize SoxR
			INFO("Initialize SoXR");
			InitializeSoXR();
                }
        }
        else {
                FATAL("LPCM Audio not enabled _streamId 0x%x.", _streamId);
                INFO("Invoking stream init with stream with stream 0x%x.", (_streamId & VIDEO_MASK));
                conf->KeyValue = (_streamId & VIDEO_MASK) + MAX_ENCODE_STREAM_NUM;
                av_flag = 1;
                recorder->StreamInit( conf, av_flag); // Video only
                _streamFd = recorder->getStreamHandler(av_flag);
        }
#elif HAS_AAC
	/* Enable audio only if type AAC */
	if( (AUDIO_AAC == (_streamId & AUDIO_MASK) ) &&
	    (AUDIO_CODEC_TYPE_AAC == _audioConfig->type) ) {
		_audioConfig->enable = 1;
		_audioConfig->sample_rate = SAMPLE_RATE_16K;
		if ( recorder->SetAudioStreamConfig(((_streamId & AUDIO_MASK) >> NIBBLE_LENGTH), _audioConfig) < 0) {
			FATAL("Error while enabling AAC Audio using setAudioStreamConfig() for stream id 0x%x, switching to video only!!!", _streamId);
			INFO("Invoking stream init with stream 0x%x!!!", (_streamId & VIDEO_MASK));
                        conf->KeyValue = (_streamId & VIDEO_MASK) + MAX_ENCODE_STREAM_NUM;
                        av_flag = 1;
                        recorder->StreamInit( conf, av_flag); // Video only
                        _streamFd = recorder->getStreamHandler(av_flag);
		}
		else {
			INFO("setAudioStreamConfig  for stream id 0x%x: Enable AAC audio with 16k sample rate!!!\n", _streamId);
			INFO("Invoking stream init with stream 0x%x!!!", _streamId);
                        conf->KeyValue = _streamId + MAX_ENCODE_STREAM_NUM;
                        av_flag = 3;
                        recorder->StreamInit( conf, av_flag); // Video only
                        _streamFd = recorder->getStreamHandler(av_flag);
		}
	}
	else {
		FATAL("AAC Audio not enabled _streamId 0x%x!!!", _streamId);
		INFO("Invoking stream init with stream with stream 0x%x!!!", (_streamId & VIDEO_MASK));
                conf->KeyValue = (_streamId & VIDEO_MASK) + MAX_ENCODE_STREAM_NUM;
                av_flag = 1;
                recorder->StreamInit( conf, av_flag); // Video only
                _streamFd = recorder->getStreamHandler(av_flag);
	}
#endif

	// No need to initialize, we rely on the web admin page
	/*
	if (rdkc_stream_set_audio_config(_streamId, &_audioConfig) < 0) {
		FATAL("rdkc_stream_set_audio_config error!");
		return false;
	}

	sleep(4); //TODO: if we have to sleep, we'll need to move this initialization to a thread
	*/

	//INFO("Invoking rdkc_stream_init");
	//_streamFd = rdkc_stream_init(_streamId, 3); // Video AND Audio
	//_streamFd = rdkc_stream_init(_streamId, 3);
	if (_streamFd < 0) {
		FATAL("stream init error: %" PRIi32"!", _streamFd);
		EnqueueForDelete(); // do a clean-up in case of failure
		return false;
	}
	INFO("stream init SUCCESS: %d" PRIi32, _streamFd);

#if DUMP_G711
        g711Fd = fopen("/opt/g711.ulw", "a");
#endif

#if DUMP_PCM
        pcmFd = fopen("/opt/audio_8k.pcm", "a");
#endif

#endif /* SDK_DISABLED */
       }
       else //GST is enabled in RFC
        {
#ifdef RMS_PLATFORM_RPI
                _streamFd = gst_InitFrame ( GST_RMS_APPNAME_VIDEO, GST_RMS_APPNAME_AUDIO );
                if(_streamFd < 0)
                {
                        INFO("GST Frame Init FAILED!\n");
                        return false;
                }
                INFO("gst_InitFrame SUCCESS: AppNameVideo=%s, AppNameAudio=%s\n", GST_RMS_APPNAME_VIDEO, GST_RMS_APPNAME_AUDIO);
#endif
        }

	// Set stream config
	if (!SetStreamConfig()) {
		FATAL("Stream config could not be set!");
		EnqueueForDelete(); // do a clean-up in case of failure
		return false;
	}

#ifdef HAS_G711
	// Set audio config if LPCM
        if( (AUDIO_G711 == (_streamId & AUDIO_MASK) ) &&
            (AUDIO_CODEC_TYPE_LPCM == _audioConfig->type) ) {
                if (!SetAudioConfig()) {
                        FATAL("G711 Audio config could not be set!");
                }
        }
#elif HAS_AAC
	// Set audio config if AAC
	if( (AUDIO_AAC == (_streamId & AUDIO_MASK) ) &&
            (AUDIO_CODEC_TYPE_AAC == _audioConfig->type) ) {
		if (!SetAudioConfig()) {
			FATAL("AAC Audio config could not be set!");
		}
	}
#endif

	return true;
}

bool ApiProtocol::UpdateStreamConfig(Variant &config) {

#ifndef SDK_DISABLED
       if( is_gst_enabled != RDKC_SUCCESS )
        {
	Variant videoConfig  = config["videoconfig"];
        INFO("UpdateStreamConfig : API initialized with video config : %s", STR(videoConfig.ToString()));

	video_stream_config_t vidConfig;
	memset(&vidConfig, 0, sizeof (vidConfig));

    if (videoConfig.HasKeyChain(V_UINT32, true, 1, "stream_type")) {
                INFO("UpdateStreamConfig : stream_type : Found in keychain");
                vidConfig.stream_type = videoConfig["stream_type"];
	}else {
                INFO("UpdateStreamConfig : stream_type : NOT FOUND in keychain");
                vidConfig.stream_type = _videoConfig->stream_type;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "width")) {
                INFO("UpdateStreamConfig : width : Found in keychain");
                vidConfig.width = videoConfig["width"];
	} else {
                INFO("UpdateStreamConfig : width : NOT FOUND in keychain");
                vidConfig.width = _videoConfig->width;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "height")) {
                INFO("UpdateStreamConfig : height : Found in keychain");
                vidConfig.height = videoConfig["height"];
	} else {
                INFO("UpdateStreamConfig : height : NOT FOUND in keychain");
                vidConfig.height = _videoConfig->height;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "frame_rate")) {
                INFO("UpdateStreamConfig : frame_rate : Found in keychain");
                vidConfig.frame_rate = videoConfig["frame_rate"];
	} else {
                INFO("UpdateStreamConfig : frame_rate : NOT FOUND in keychain");
                vidConfig.frame_rate = _videoConfig->frame_rate;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "gov_length")) {
                INFO("UpdateStreamConfig : gov_length : Found in keychain");
                vidConfig.gov_length = videoConfig["gov_length"];
	} else {
                INFO("UpdateStreamConfig : gov_length : NOT FOUND in keychain");
                vidConfig.gov_length = _videoConfig->gov_length;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "profile")) {
                INFO("UpdateStreamConfig : profile : Found in keychain");
                vidConfig.profile = videoConfig["profile"];
	} else {
                INFO("UpdateStreamConfig : profile : NOT FOUND in keychain");
                vidConfig.profile = _videoConfig->profile;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "quality_type")) {
                INFO("UpdateStreamConfig : quality_type : Found in keychain");
                vidConfig.quality_type = videoConfig["quality_type"];
	} else {
                INFO("UpdateStreamConfig : quality_type : NOT FOUND in keychain");
                vidConfig.quality_type = _videoConfig->quality_type;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "quality_level")) {
                INFO("UpdateStreamConfig : quality_level : Found in keychain");
                vidConfig.quality_level = videoConfig["quality_level"];
	} else {
                INFO("UpdateStreamConfig : quality_level : NOT FOUND in keychain");
                vidConfig.quality_level = _videoConfig->quality_level;
	}

	if (videoConfig.HasKeyChain(V_UINT32, true, 1, "bit_rate")) {
                INFO("UpdateStreamConfig : bit_rate : Found in keychain");
                vidConfig.bit_rate = videoConfig["bit_rate"];
	} else {
                INFO("UpdateStreamConfig : bit_rate : NOT FOUND in keychain");
                vidConfig.bit_rate = _videoConfig->bit_rate;
	}

	//check if stream set is required	
	if ( ( _videoConfig->stream_type == vidConfig.stream_type ) &&
		( _videoConfig->width == vidConfig.width ) &&
		( _videoConfig->height == vidConfig.height ) &&
		( _videoConfig->frame_rate == vidConfig.frame_rate ) &&
		( _videoConfig->gov_length == vidConfig.gov_length ) &&
		( _videoConfig->profile == vidConfig.profile ) &&
		( _videoConfig->quality_type == vidConfig.quality_type ) &&
		( _videoConfig->quality_level == vidConfig.quality_level ) &&
		( _videoConfig->bit_rate == vidConfig.bit_rate ) ) {
		INFO("UpdateStreamConfig : config match found - Avoid set stream config call");
	} else {
		if (recorder->SetVideoStreamConfig((_streamId & VIDEO_MASK), &vidConfig) < 0) {
			WARN("UpdateStreamConfig : start : Failed to set video configurations");
			WARN("stream_type=%" PRIu32, vidConfig.stream_type);
			WARN("width=%" PRIu32, vidConfig.width);
			WARN("height=%" PRIu32, vidConfig.height);
			WARN("frame_rate=%" PRIu32, vidConfig.frame_rate);
			WARN("gov_length=%" PRIu32, vidConfig.gov_length);
			WARN("profile=%" PRIu32, vidConfig.profile);
			WARN("quality_type=%" PRIu32, vidConfig.quality_type);
			WARN("quality_level=%" PRIu32, vidConfig.quality_level);
			WARN("bit_rate=%" PRIu32, vidConfig.bit_rate);
			WARN("UpdateStreamConfig : end : Failed to set video configurations");
			return false;
		}
	} 

	// Read again after the set command above
	memset(&vidConfig, 0, sizeof (vidConfig));
	if( recorder->GetVideoStreamConfig((_streamId & VIDEO_MASK), &vidConfig) < 0 ) {
		WARN("UpdateStreamConfig : getVideoStreamConfig error !");
	}

	/* Below log line is part of US RDKC-3856 to have the Telemetry Marker for RMS Video Configuration
		{ RMS Video Configurations- StreamType Width Height Bitrate Framerate GOVlen Profile QualityType QualityLevel: <StreamType>, <Width>, <Height>, 
							<Birate>, <Framerate>, <GOVlen>, <profileID>, <QualityType>, <QualityLevel> }
	*/
	INFO("RMS Video Configurations- StreamType Width Height Bitrate Framerate GOVlen Profile QualityType QualityLevel: %"PRIu16", %"PRIu32", %"PRIu32", %"PRIu32", %"PRIu32", %"PRIu32", %"PRIu32", %"PRIu32", %"PRIu32"", vidConfig.stream_type, vidConfig.width, vidConfig.height, vidConfig.bit_rate, vidConfig.frame_rate, vidConfig.gov_length, vidConfig.profile, vidConfig.quality_type, vidConfig.quality_level);

	/*INFO("UpdateStreamConfig : Streaming started with below video configurations");
	INFO("stream_type=%" PRIu32, vidConfig.stream_type);
	INFO("width=%" PRIu32, vidConfig.width);
	INFO("height=%" PRIu32, vidConfig.height);
	INFO("frame_rate=%" PRIu32, vidConfig.frame_rate);
	INFO("gov_length=%" PRIu32, vidConfig.gov_length);
	INFO("profile=%" PRIu32, vidConfig.profile);
	INFO("quality_type=%" PRIu32, vidConfig.quality_type);
	INFO("quality_level=%" PRIu32, vidConfig.quality_level);
	INFO("bit_rate=%" PRIu32, vidConfig.bit_rate);*/
       }
       else
        {
#ifdef RMS_PLATFORM_RPI
                // GST Set stream config here
#endif
        }

#endif
	return true;
}

bool ApiProtocol::Terminate() {
	if (_pIoHandler != NULL) {
		IOHandler *pIoHandler = _pIoHandler;
		_pIoHandler = NULL;
		pIoHandler->SetProtocol(NULL);
		delete pIoHandler;
	}

       if( is_gst_enabled != RDKC_SUCCESS )
        {
#ifndef SDK_DISABLED
	if (_streamFd > 0) {
		INFO("Invoking rdkc_stream_close");
		if (recorder->StreamClose(av_flag) < 0) {
			WARN("rdkc_stream_close failed!");
		}
		else {
			INFO("rdkc_stream_close OK");
		}
	}

        if(_videoConfig) {
                delete _videoConfig;
                _videoConfig = NULL;
        }

        if(_audioConfig) {
                delete _audioConfig;
                _audioConfig = NULL;
        }

	// Clean up SoXR
	if(is_soxr_initialized && _soxr) {
		INFO("Deleting SoxR _SoXR: %x is_soxr_initialized: %d", _soxr, is_soxr_initialized);
		soxr_delete(_soxr);
		_soxr = NULL;
		is_soxr_initialized = false;
	}

#if DUMP_G711
	fclose(g711Fd);
#endif

#if DUMP_PCM
	fclose(pcmFd);
#endif

#endif /* SDK_DISABLED */
       }
       else
       {
#ifdef RMS_PLATFORM_RPI
                INFO("Invoking gst_TerminateFrame AppNameVideo=%s, AppNameAudio=%s\n", GST_RMS_APPNAME_VIDEO, GST_RMS_APPNAME_AUDIO);
                gst_TerminateFrame( GST_RMS_APPNAME_VIDEO, GST_RMS_APPNAME_AUDIO );
#endif
       }
	return true;
}

bool ApiProtocol::FeedData() {
        static bool frameDebug(false);
        static uint32_t framecount(0);
        if( is_gst_enabled != RDKC_SUCCESS )
        {

#ifndef SDK_DISABLED

	static bool once(true);
	static bool frameDebug(false);
#if DUMP_PCM
	size_t const osize = sizeof(short);
#endif
	size_t odone = 0;

	if( once ) {
		string fileCheck( "/opt/.enable_rms_debug_logs" );
		struct stat buffer;
  		frameDebug = (stat(fileCheck.c_str(), &buffer) == 0);
		once = false;
	}

	// Read the data from the API
	frame_info_t frame_info;
	static uint32_t invokecount(0);
	if( frameDebug && (invokecount++%30 == 0)) INFO("Invoking get stream");
	int retVal = recorder->GetStream(&frame_info,av_flag);
	if (retVal == 0) {
		static uint32_t framecount(0);
		if( frameDebug && (framecount++%30 == 0)) INFO("FRAME: type=%d, size=%d, pic_type=%d, frame_num=%d, width=%d, height=%d, timestamp=%d, arm_pts=%llu\n", frame_info.stream_type, frame_info.frame_size, frame_info.pic_type, frame_info.frame_num, frame_info.width, frame_info.height, frame_info.frame_timestamp, frame_info.arm_pts);

		// Set the header as payload type (0x00)
		uint8_t header = 0x00;
		if (frame_info.stream_type == 1) {
			// h264 frame, header is as-is
		} else if ((frame_info.stream_type == 3) || (frame_info.stream_type == 10)) {
			header = 0x01; // audio frame
#ifdef HAS_G711
			if(frame_info.stream_type == 3)
                        {

                                int16_t *pcmPtr = (int16_t *) frame_info.frame_ptr;

				// Downsample the PCM audio to 8k
				if(is_soxr_initialized && _soxr) {
					error = soxr_process(_soxr, (void *)pcmPtr, frame_info.frame_size/2, NULL, (void *)pcmPtr, frame_info.frame_size/4, &odone);
				}

#if DUMP_PCM
				fwrite((uint8_t *)pcmPtr, odone*osize, 1, pcmFd);
#endif

#if DUMP_G711
				uint8_t *g711Data = (uint8_t *) malloc(odone);
				memset(g711Data, 0, odone);
#endif
				if(odone <= 0) {
					INFO("Resampled audio is not available");
					return true;
				}

				// Form the header and actual payload
				uint32_t length = odone + 9; // length is (frame size)/2 + header and timestamp
				_outputBuffer.ReadFromU32(length, true); // set the payload length
				_outputBuffer.ReadFromByte(header); // payload type header
				_outputBuffer.ReadFromU64(((uint64_t)frame_info.frame_timestamp * 1000), true);

				// g711 encoding
                                for(uint32_t i = 0; i < odone; ++i)
                                {
                                        //g711Ptr[i] = linear_to_ulaw(g711Ptr[i]);
					_outputBuffer.ReadFromByte(linear_to_ulaw(pcmPtr[i]));
#if DUMP_G711
					g711Data[i] = linear_to_ulaw(pcmPtr[i]);
#endif
                                }
#if DUMP_G711
				fwrite((uint8_t *)g711Data, odone, 1, g711Fd);
#endif
#endif
			}
		} else {
			WARN("Not supported stream type: %" PRIu8, header);
			return true;
		}

		if ((frame_info.stream_type == 1) || (frame_info.stream_type == 10)) {
		// Form the header and actual payload
			uint32_t length = frame_info.frame_size + 9; // length is frame size + header and timestamp
			_outputBuffer.ReadFromU32(length, true); // set the payload length
			_outputBuffer.ReadFromByte(header); // payload type header
			_outputBuffer.ReadFromU64(((uint64_t)frame_info.frame_timestamp * 1000), true);
			_outputBuffer.ReadFromBuffer((uint8_t *) frame_info.frame_ptr, frame_info.frame_size);
		}

		// Pass the data to the upper protocol
		return _pNearProtocol->SignalInputData(_outputBuffer);
	} else if (retVal == 1) {
		static uint32_t notreadycount(0);
		if( frameDebug && (notreadycount++%10 == 0)) INFO("No frame ready. ReturnVal: %d", retVal);
	} else {
		static uint32_t failcount(0);
		if( frameDebug && (failcount++%10 == 0)) INFO("rdkc_stream_read_frame failed! ReturnVal: %d", retVal);
		EnqueueForDelete(); // do a clean-up in case of read frame failure
		return false;
	}
#endif /* SDK_DISABLED */
       }
       else //GST is enabled in RFC
        {
#ifdef RMS_PLATFORM_RPI
                GST_FrameInfo frame_info;
                int retVal = gst_ReadFrameData ( &frame_info );
                if (retVal == 0) {

                        if( frameDebug && (framecount++%30 == 0)) INFO("FRAME: streamid=%d, type=%d, size=%d, width=%d, height=%d, timestamp=0x%llx, frame_ptr=0x%x\n",frame_info.stream_id, frame_info.stream_type, frame_info.frame_size, frame_info.width, frame_info.height, frame_info.frame_timestamp, frame_info.frame_ptr);

                        // Set the header as payload type (0x00)
                        uint8_t header = 0x00;
                        if (frame_info.stream_type == 1) {
                                // h264 frame, header is as-is
                        } else if ((frame_info.stream_type == 3) || (frame_info.stream_type == 10)) {
                                header = 0x01; // audio frame
                        } else {
                                WARN("Not supported stream type: %" PRIu8, header);
                                return true;
                        }


                        // Form the header and actual payload
                        uint32_t length = frame_info.frame_size + 9; // length is frame size + header and timestamp
                        _outputBuffer.ReadFromU32(length, true); // set the payload length
                        _outputBuffer.ReadFromByte(header); // payload type header
                        _outputBuffer.ReadFromU64(((uint64_t)frame_info.frame_timestamp * 1000), true);
                        _outputBuffer.ReadFromBuffer((uint8_t *) frame_info.frame_ptr, frame_info.frame_size);
                        deleteBuffer();
                        return _pNearProtocol->SignalInputData(_outputBuffer);
                } else if (retVal == 1) {
                        static uint32_t notreadycount(0);
                        if( frameDebug && (notreadycount++%10 == 0)) INFO("No frame ready. ReturnVal: %d", retVal);
                } else {
                        static uint32_t failcount(0);
                        if( frameDebug && (failcount++%10 == 0)) INFO("rdkc_stream_read_frame failed! ReturnVal: %d", retVal);
                        EnqueueForDelete(); // do a clean-up in case of read frame failure
                        return false;
                }
#endif
        }

	return true;
}

bool ApiProtocol::AllowFarProtocol(uint64_t type) {
	return false; // no allowed underlying protocol
}

bool ApiProtocol::AllowNearProtocol(uint64_t type) {
	return (type == PT_RAW_MEDIA); // only raw media on top, for now
}

bool ApiProtocol::SignalInputData(int32_t recvAmount) {
#ifdef HAS_THREAD
	return FeedAsync(_threadStatus);
#else
	return FeedData();
#endif 	/* HAS_THREAD */
}

bool ApiProtocol::SignalInputData(IOBuffer &buffer) {
	NYIR;
}

bool ApiProtocol::SetStreamConfig() {
	IOBufferExt config;

	uint32_t length = _streamName.size() + 1; // length is stream name + header
	config.ReadFromU32(length, true); // set the payload length
	config.ReadFromByte(0x40); // set the header as stream config (0x40)
	config.ReadFromString(_streamName); // set the target stream name

	if (_pNearProtocol) {
		return _pNearProtocol->SignalInputData(config);
	}

	return false;
}

bool ApiProtocol::SetAudioConfig() {
	// Get the actual type when it was initialized
	//TODO: For now, this is assumed as G711U
	IOBufferExt config;
#ifdef HAS_AAC
	uint32_t length = 3; // length is config (2 bytes) + header
	config.ReadFromU32(length, true); // set the payload length
	// set the header as payload config (0x80), audio (0x01), and aac type (0x00)
	config.ReadFromByte((0x80 | 0x01 | 0x00));
	/* 5 bits: object type 			: AAC Main
	   4 bits: frequency index		: 16000 Hz
	   4 bits: channel configuration	: 1:1 channel:front-center */
	config.ReadFromByte(0x0C);
	config.ReadFromByte(0x08);
#elif HAS_G711
	uint32_t length = 2; // length is config (1 byte) + header
	config.ReadFromU32(length, true); // set the payload length
	// set the header as payload config (0x80), audio (0x01), and g711 type (0x02)
	config.ReadFromByte((0x80 | 0x01 | 0x02));
	config.ReadFromByte(0x00); // set as u-law (0x00) and not a-law (0x01)
#endif
	if (_pNearProtocol) {
		return _pNearProtocol->SignalInputData(config);
	}

	return false;
}

int32_t ApiProtocol::GetStreamFd() {
	return _streamFd;
}

#ifdef HAS_THREAD
void FeedTask(Variant &params, void *requestingObject) {
    ApiProtocol *pProtocol = (ApiProtocol *) requestingObject;
    TASK_STATUS(params) = pProtocol->DoFeedAsyncTask(params);
}

void FeedCompletionTask(Variant &params, void *requestingObject) {
    ApiProtocol *pProtocol = (ApiProtocol *) requestingObject;
    pProtocol->CompleteFeedAsyncTask(params);
}

bool ApiProtocol::FeedAsync(Variant &params) {
	if (_feeding) {
		return true; // return immediately if previous thread is not finished yet
	}

    return ThreadManager::QueueAsyncTask(&FeedTask, &FeedCompletionTask, params, (void *)this);
}

bool ApiProtocol::DoFeedAsyncTask(Variant &params) {
	_feeding = true;
	return FeedData();
}

void ApiProtocol::CompleteFeedAsyncTask(Variant &params) {
	_feeding = false;
}
#endif  /* HAS_THREAD */
#endif /* HAS_PROTOCOL_API */

