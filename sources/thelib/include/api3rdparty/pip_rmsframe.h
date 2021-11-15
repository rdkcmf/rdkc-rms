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

#ifdef ENABLE_RMS_STREAM_WITH_PIPEWIRE

#ifndef __PIP_RMSFRAME_H__
#define __PIP_RMSFRAME_H__

/***** HEADER FILE *****/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>
#include<stdio.h>
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
#include <sys/timeb.h>
#include "glib.h"

typedef struct PIP_FrameInfo
{
        guint16 stream_id;              // buffer id (0~3)
        guint16 stream_type;            // 0 = Video H264 frame, 1 = Audio frame
        guint8  *frame_ptr;             // same as guint8 *y_addr
        guint32 width;                  // Buffer Width
        guint32 height;                 // Buffer Height
        guint32 frame_size;             // FrameSize = Width * Height * 1.5 (Y + UV)
        guint64 frame_timestamp;        // Time stamp 8 bytes from GST
}PIP_FrameInfo;

/***** PROTOTYPE *****/
int PIP_ReadFrameData( PIP_FrameInfo *frame_info );
int PIP_deleteBuffer(  );
int PIP_InitFrame();
int PIP_TerminateFrame ( );

#endif

#endif /* ENABLE_RMS_STREAM_WITH_PIPEWIRE */
