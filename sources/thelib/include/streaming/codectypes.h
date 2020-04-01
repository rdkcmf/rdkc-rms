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



#ifndef _CODECTYPES_H
#define	_CODECTYPES_H

#include "common.h"

#define CODEC_UNKNOWN				MAKE_TAG3('U','N','K')
#define CONTAINER_AUDIO_VIDEO_MP2T	MAKE_TAG6('A','V','M','P','2','T') //This is used when packets contained in RTP channels is encapsulated with TS

#define CODEC_VIDEO					MAKE_TAG1('V')
#define CODEC_VIDEO_UNKNOWN			MAKE_TAG4('V','U','N','K')
#define CODEC_VIDEO_PASS_THROUGH	MAKE_TAG3('V','P','T')
#define CODEC_VIDEO_JPEG			MAKE_TAG5('V','J','P','E','G')
#define CODEC_VIDEO_SORENSONH263	MAKE_TAG5('V','S','2','6','3')
#define CODEC_VIDEO_SCREENVIDEO		MAKE_TAG3('V','S','V')
#define CODEC_VIDEO_SCREENVIDEO2	MAKE_TAG4('V','S','V','2')
#define CODEC_VIDEO_VP6				MAKE_TAG4('V','V','P','6')
#define CODEC_VIDEO_VP6ALPHA		MAKE_TAG5('V','V','P','6','A')
#define CODEC_VIDEO_H264			MAKE_TAG5('V','H','2','6','4')

#define CODEC_AUDIO					MAKE_TAG1('A')
#define CODEC_AUDIO_UNKNOWN			MAKE_TAG4('A','U','N','K')
#define CODEC_AUDIO_PASS_THROUGH	MAKE_TAG3('A','P','T')
#define CODEC_AUDIO_PCMLE			MAKE_TAG6('A','P','C','M','L','E')
#define CODEC_AUDIO_PCMBE			MAKE_TAG6('A','P','C','M','B','E')
#define CODEC_AUDIO_NELLYMOSER		MAKE_TAG3('A','N','M')
#define CODEC_AUDIO_MP3				MAKE_TAG4('A','M','P','3')
#define CODEC_AUDIO_AAC				MAKE_TAG4('A','A','A','C')
#define CODEC_AUDIO_G711			MAKE_TAG5('A','G','7','1','1')
#define CODEC_AUDIO_G711A			MAKE_TAG6('A','G','7','1','1','A')
#define CODEC_AUDIO_G711U			MAKE_TAG6('A','G','7','1','1','U')
#define CODEC_AUDIO_SPEEX			MAKE_TAG6('A','S','P','E','E','X')

#define CODEC_SUBTITLE				MAKE_TAG1('S')
#define CODEC_SUBTITLE_UNKNOWN		MAKE_TAG4('S','U','N','K')
#define CODEC_SUBTITLE_SRT			MAKE_TAG4('S','S','R','T')
#define CODEC_SUBTITLE_SUB			MAKE_TAG4('S','S','U','B')


#endif	/* _CODECTYPES_H */
