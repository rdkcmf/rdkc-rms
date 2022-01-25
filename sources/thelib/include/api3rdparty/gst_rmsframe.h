/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef RMS_PLATFORM_RPI

#ifndef __GST_RMSFRAME_H__
#define __GST_RMSFRAME_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include<gst/gst.h>
#include<stdio.h>
#include<gst/app/gstappsink.h>
#include<iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include<sys/time.h>
#include <assert.h>
#include <sys/syscall.h>
#include "rdk_debug.h"
#include "RFCConfigAPI.h"
#include "RFCCommon.h"

#define RFC_RMS 		"rfcVariable.ini"
#define GST_FLAG		"RFC_ENABLE_RDKC_GST"
#define TIMESTAMP_SIZE 		8
#define RDKC_FAILURE		-1
#define RDKC_SUCCESS 		0
#define MAX_NAL_SIZE 		65536
#define GST_RMS_APPNAME_VIDEO 	"rms_video_1"
#define GST_RMS_APPNAME_AUDIO 	"rms_audio_1"

typedef struct GST_FrameInfo
{
	guint16 stream_id;		// buffer id (0~3)
	guint16 stream_type;		// 0 = Video H264 frame, 1 = Audio frame
	guint8  *frame_ptr;		// same as guint8 *y_addr
	guint32 width;			// Buffer Width
	guint32 height;			// Buffer Height
	guint32 frame_size;		// FrameSize = Width * Height * 1.5 (Y + UV)
	guint64 frame_timestamp;	// Time stamp 8 bytes from GST
}GST_FrameInfo;


/*End of variable declaration*/

int gst_InitFrame( char *in_appName_Video, char *in_appName_Audio );
int gst_TerminateFrame ( char *in_appName_Video, char *in_appName_Audio );
int gst_ReadFrameData( GST_FrameInfo *frame_info );
gboolean fetch_response( GstElement *elt, gpointer user_data );
void send_request( char *request,char *response );
void *start_Video_Streaming( void *vargp );
void *start_Audio_Streaming( void *vargp );
void stop_Video_Streaming( char *in_appName_Video );
void stop_Audio_Streaming( char *in_appName_Audio );
int IsGSTEnabledInRFC( char *file, char *name );
int deleteBuffer();
#endif

#endif /* RMS_PLATFORM_RPI */
