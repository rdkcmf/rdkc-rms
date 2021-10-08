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

/***** HEADER FILE *****/

#ifdef ENABLE_RMS_STREAM_WITH_PIPEWIRE

#include "api3rdparty/pip_rmsframe.h"

#include "spa/param/video/format-utils.h"
#include "spa/debug/types.h"
#include "spa/param/video/type-info.h"
#include "pipewire/pipewire.h"

#ifdef RMS_PLATFORM_RPI
#include <netinet/in.h>
#include "utils/logging/logging.h"
#endif

/***** MACROS *****/
#define RDKC_FAILURE            -1
#define RDKC_SUCCESS            0
#define MAX_NAL_SIZE            65536

#define PORT                8090
#define MAXLINE             30
#define MAX_QUEUESIZE       30
#define QUEUE_FRAME_SIZE    100

/***** Global Variable Declaration *****/
pthread_t       pip_getFrame;
pthread_t       pip_thread_id;
pthread_mutex_t pip_queueLock;

guint64 pip_frame_timestamp  = 0;
guint8* pip_frame_buffer     = NULL;
guint32 pip_frame_size=0;

volatile static int pushIndex=0;

static bool FrameDebug(false);


int pip_buffer_width = 1280;
int pip_buffer_height  = 720;

int pip_streamId       = 4;
int pip_stream_type    = 1;
int pip_sockfd;

char acfilename[50] = { 0 };

/***** Structure Declaration *****/

struct sockaddr_in pip_servaddr;

struct data {
        struct pw_main_loop *loop;
        struct pw_stream *stream;
        struct spa_video_info format;
};

typedef struct
{
    guint8* buffer;
    guint64  timestamp;
    unsigned long long size;
}BUFFERQUEUE;  //same struct definition

BUFFERQUEUE pip_bufferqueue[MAX_QUEUESIZE];

/***** PROTOTYPE *****/
void *PIP_UdpServer( void *vargp );
int PIP_getSocket();
int PIP_CreateServerSocket(  );
int PIP_CreateClientSocket(  );
int PIP_pushBuffer( unsigned long long frameSize, guint8 * frame, guint64 timestamp );
int PIP_popBuffer( PIP_FrameInfo * frame_info );
void *PIP_start_Video_Streaming( void *vargp );
static void PIP_on_process(void *userdata);
static void PIP_on_param_changed(void *userdata, uint32_t id, const struct spa_pod *param);
void PIP_current_time(  char *sys_time);

/***** Function Definition *****/

/* {{{ PIP_UdpServer() */
void *PIP_UdpServer( void *vargp )
{
    int ret = PIP_CreateServerSocket(  );

    if ( RDKC_FAILURE == ret )
    {
        INFO( "%s(%d): Failed to create server socket\n", __FILE__,
              __LINE__ );
    }

    return NULL;
}
/* }}} */

/* {{{ PIP_getSocket() */
int PIP_getSocket()
{
    if ( ( pip_sockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        INFO( "%s(%d): socket creation failed\n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }
    return pip_sockfd;
}
/* }}} */

/* {{{ PIP_CreateServerSocket() */
int PIP_CreateServerSocket(  )
{
    char buffer[MAXLINE];
    char *ack = "acknowledge";
    struct sockaddr_in servaddr1, cliaddr1;

    memset( &servaddr1, 0, sizeof( servaddr1 ) );
    memset( &cliaddr1, 0, sizeof( cliaddr1 ) );

    // Filling server information
    servaddr1.sin_family = AF_INET; // IPv4
    servaddr1.sin_addr.s_addr = INADDR_ANY;
    servaddr1.sin_port = htons( PORT );

    // Bind the socket with the server address
    if ( bind
         ( pip_sockfd, ( const struct sockaddr * ) &servaddr1,
           sizeof( servaddr1 ) ) < 0 )
    {
        INFO( "%s(%d): Bind  failed\n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    socklen_t len, n;

    while ( 0 <=
            recvfrom( pip_sockfd, ( char * ) buffer, MAXLINE, MSG_WAITALL,
                      ( struct sockaddr * ) &cliaddr1, &len ) )
    {
        buffer[n] = '\0';
        sendto( pip_sockfd, ( const char * ) ack, strlen( ack ), MSG_CONFIRM,
                ( const struct sockaddr * ) &cliaddr1, len );
    }

    return RDKC_SUCCESS;
}
/* }}} */

/* {{{ PIP_CreateClientSocket() */
int PIP_CreateClientSocket(  )
{
    int sockfd;

    // Creating socket file descriptor
    if ( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        INFO( "%s(%d): Socket creation failed\n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    memset( &pip_servaddr, 0, sizeof( pip_servaddr ) );

    // Filling server information
    pip_servaddr.sin_family = AF_INET;  //IPV4
    pip_servaddr.sin_port = htons( PORT );
    pip_servaddr.sin_addr.s_addr = INADDR_ANY;

    int n, len;

    sendto( sockfd, ( const char * ) "Trigger", strlen( "Trigger" ),
            MSG_CONFIRM, ( const struct sockaddr * ) &pip_servaddr,
            sizeof( pip_servaddr ) );
    close( sockfd );

    return RDKC_SUCCESS;
}
/* }}} */

/* {{{ PIP_AllocateBufferForQueue() */
int PIP_AllocateBufferForQueue()
{
    int i=0;

    for(i=0; i<MAX_QUEUESIZE; i++)
    {
        pip_bufferqueue[i].buffer = ( guint8 * ) ( malloc( QUEUE_FRAME_SIZE ) );

	if( NULL == pip_bufferqueue[i].buffer )
	{
            FATAL( "%s(%d): Error : memory allocation failed \n", __FILE__, __LINE__ );
            return RDKC_FAILURE;
	}
    }

    return RDKC_SUCCESS;
}
/* }}} */

/* {{{ PIP_pushBuffer() */
int PIP_pushBuffer( unsigned long long frameSize, guint8 * frame, guint64 timestamp )
{
    if ( pthread_mutex_lock( &pip_queueLock ) != 0 )
    {
        return RDKC_FAILURE;
    }

    if ( pushIndex >= MAX_QUEUESIZE )
    {
        if ( FrameDebug )
            DEBUG( "%s(%d): Maximum queue size reached \n", __FILE__,
                   __LINE__ );
        pushIndex = 0;
    }

    pip_bufferqueue[pushIndex].buffer = ( guint8 * ) ( realloc( pip_bufferqueue[pushIndex].buffer , frameSize ) );
    memcpy( pip_bufferqueue[pushIndex].buffer, frame, frameSize );

    pip_bufferqueue[pushIndex].timestamp = timestamp;
    pip_bufferqueue[pushIndex].size = frameSize;
    pushIndex++;
    pthread_mutex_unlock( &pip_queueLock );

    return RDKC_SUCCESS;
}
/* }}} */

/* {{{ PIP_popBuffer() */
int PIP_popBuffer( PIP_FrameInfo * frame_info )
{
    static int popIndex = 0;

    if ( pthread_mutex_lock( &pip_queueLock ) != 0 )
    {
        FATAL( "%s(%d): Error : mutex lock failed \n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    if ( pushIndex > 0 )
    {

        frame_info->frame_size = pip_bufferqueue[popIndex].size;
        frame_info->frame_timestamp = pip_bufferqueue[popIndex].timestamp;

        frame_info->frame_ptr = pip_bufferqueue[popIndex].buffer;
    }
    else
    {
        if ( FrameDebug )
            DEBUG( "%s(%d): Queue is empty \n", __FILE__, __LINE__ );
    }

    pthread_mutex_unlock( &pip_queueLock );

    return RDKC_SUCCESS;
}
/* }}} */

/* {{{ PIP_deleteBuffer() */
int PIP_deleteBuffer(  )
{
    if ( pthread_mutex_lock( &pip_queueLock ) != 0 )
    {
        INFO( "%s(%d): Mutex lock failed \n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    int i;

    if ( 1 == pushIndex )
    {
        pushIndex--;
    }
    else if ( pushIndex > 1 )
    {
        for ( i = 0; i < ( pushIndex - 1 ); i++ )
        {
            pip_bufferqueue[i].buffer =
                ( guint8* ) ( realloc( pip_bufferqueue[i].buffer,
                                 pip_bufferqueue[i + 1].size ) );

            memcpy( pip_bufferqueue[i].buffer, pip_bufferqueue[i + 1].buffer,
                    pip_bufferqueue[i + 1].size );
            pip_bufferqueue[i].timestamp = pip_bufferqueue[i + 1].timestamp;
            pip_bufferqueue[i].size = pip_bufferqueue[i + 1].size;
        }

        pushIndex--;
    }
    else
    {
        if ( FrameDebug )
            DEBUG( "%s(%d): Nothing to delete \n", __FILE__, __LINE__ );
    }
    pthread_mutex_unlock( &pip_queueLock );

    return RDKC_SUCCESS;
}
/* }}} */

/* {{{ PIP_ReadFrameData() */
int PIP_ReadFrameData( PIP_FrameInfo *frame_info )
{
        int ret=0;

        if(FrameDebug) DEBUG("%s(%d): Entering pip_ReadFrameData() fn \n",__FILE__, __LINE__);

        if( pushIndex > 0 )
        {
                ret = PIP_popBuffer(frame_info );

                if ( RDKC_SUCCESS != ret)
                {
                   FATAL( "%s(%d): Failed to read data from queue \n",__FILE__, __LINE__);
                }

                frame_info->stream_id = pip_streamId;
                frame_info->stream_type = pip_stream_type;  // Video or Audio frame
                frame_info->width = pip_buffer_width;
                frame_info->height = pip_buffer_height;

                if(FrameDebug) DEBUG("%s(%d): RMS frame data Read \n",__FILE__, __LINE__);
        }
        else
        {
                return 1; // No Frame data
        }

        return 0;
}
/* }}} */

/* {{{ PIP_InitFrame() */
int PIP_InitFrame()
{
        pip_sockfd = PIP_getSocket();

        pthread_create( &pip_thread_id, NULL, PIP_UdpServer, NULL );

        if(FrameDebug) INFO("%s(%d): Entering Pipewire based rmsframe enable...\n", __FILE__, __LINE__);

        pip_frame_buffer = ( guint8 * ) ( malloc( MAX_NAL_SIZE ) );

        if(pip_frame_buffer == NULL)
        {
                FATAL("%s(%d): Malloc for frameBuffer failed...\n", __FILE__, __LINE__);
                close(pip_sockfd);
                return -1;
        }

        if(FrameDebug) DEBUG("%s(%d): AFTER MALLOC pip_frame_buffer=%d ...\n", __FILE__, __LINE__, pip_frame_buffer);

        pw_init(NULL, NULL);

        if(pthread_mutex_init(&pip_queueLock, NULL) != 0)
        {
                FATAL("%s(%d): Mutex init for queue failed...\n", __FILE__, __LINE__);
                free (pip_frame_buffer);
                close(pip_sockfd);
                return -1;
        }

	int ret = 0;

	ret = PIP_AllocateBufferForQueue();

	if(ret != 0)
        {
		return -1;
	}

        ret = pthread_create( &pip_getFrame, NULL, PIP_start_Video_Streaming, NULL );

        if(ret != 0)
        {
                FATAL("%s(%d): Failed to create start_Streaming thread,error_code: %d\n",
                        __FILE__, __LINE__,ret);
                close(pip_sockfd);
                pthread_mutex_destroy(&pip_queueLock);
                free (pip_frame_buffer);
                return -1;
        }

        if(FrameDebug) DEBUG("%s(%d): Exiting Pipewire based rmsframe enabled...\n", __FILE__, __LINE__);

        return pip_sockfd;
}
/* }}} */

static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .param_changed = PIP_on_param_changed,
        .process = PIP_on_process,
};

/* {{{ PIP_start_Video_Streaming() */
void *PIP_start_Video_Streaming( void *vargp )
{
        struct data data = { 0, };
        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        char sys_time[25] = { 0 };

        size_t stksize;
        pthread_attr_t atr;

        pthread_attr_getstacksize(&atr, &stksize);

        PIP_current_time( sys_time );

        sprintf( acfilename, "/tmp/pipewire%s.h264", &sys_time );

        if(FrameDebug) DEBUG("%s(%d): Entering rmsframe start streaming thread...\n",
                __FILE__, __LINE__);

        data.loop = pw_main_loop_new(NULL);

        data.stream = pw_stream_new_simple(
                                        pw_main_loop_get_loop(data.loop),
                                        "video-capture",
                                        pw_properties_new(
                                                        PW_KEY_MEDIA_TYPE, "Video",
                                                        PW_KEY_MEDIA_CATEGORY, "Capture",
                                                        PW_KEY_MEDIA_ROLE, "Camera",
                                                        NULL),
                                        &stream_events,
                                        &data);


	params[0] = spa_pod_builder_add_object(&b,
                        SPA_TYPE_OBJECT_Format,   SPA_PARAM_EnumFormat,
                        SPA_FORMAT_mediaType,     SPA_POD_Id(SPA_MEDIA_TYPE_video),
                        SPA_FORMAT_mediaSubtype,  SPA_POD_Id(SPA_MEDIA_SUBTYPE_h264),
                        SPA_FORMAT_VIDEO_format,  SPA_POD_Id(SPA_VIDEO_FORMAT_ENCODED),
                        SPA_FORMAT_VIDEO_size,    SPA_POD_Rectangle(&SPA_RECTANGLE(pip_buffer_width, pip_buffer_height)),
                        SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(
                                                              &SPA_FRACTION(25, 1),
                                                              &SPA_FRACTION(0, 1),
                                                              &SPA_FRACTION(1000, 1)));

        pw_stream_connect(data.stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS,
                          params, 1);

        pw_main_loop_run(data.loop);

        pw_stream_destroy(data.stream);
        pw_main_loop_destroy(data.loop);

        return 0;
}
/* }}} */

/* {{{ PIP_on_process() */
static void PIP_on_process(void *userdata)
{
        struct data *data = userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
	struct timeb timer_msec;
        int    ret=0;


        if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
            pw_log_warn("out of buffers: %m");
            return;
        }

        buf = b->buffer;

        if (buf->datas[0].data == NULL)
            return;

        if (!ftime(&timer_msec))
        {
            pip_frame_timestamp = ((long long int) timer_msec.time) * 1000ll +
                                    (long long int) timer_msec.millitm;
        }
        else {
            pip_frame_timestamp = -1;
        }

#if 0
	FILE* pFile;

        if( '\0' != acfilename[0] )
        {
	    pFile = fopen(acfilename, "a+");

	    fwrite(buf->datas[0].data,buf->datas[0].chunk->size,1,pFile);

	    fclose(pFile);
        }
#endif

        pip_frame_size = buf->datas[0].chunk->size;
        pip_frame_buffer = (guint8 *)realloc(pip_frame_buffer, sizeof(guint8)* pip_frame_size);

        memcpy(pip_frame_buffer, buf->datas[0].data, pip_frame_size );
	
	ret = PIP_pushBuffer( pip_frame_size, pip_frame_buffer , pip_frame_timestamp );

        if ( RDKC_SUCCESS != ret)
        {
            FATAL( "%s(%d): Pushing to the queue failed \n",__FILE__, __LINE__);
        }

         ret = PIP_CreateClientSocket(  );

         if ( RDKC_FAILURE == ret )
         {
             INFO( "%s(%d): Failed to create client socket \n",__FILE__, __LINE__);
          }

          pw_stream_queue_buffer(data->stream, b);

}
/* }}} */

/* {{{ PIP_on_param_changed() */
static void PIP_on_param_changed(void *userdata, uint32_t id, const struct spa_pod *param)
{
        struct data *data = userdata;
        struct pw_stream *stream = data->stream;
        uint8_t params_buffer[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(params_buffer, sizeof(params_buffer));
        const struct spa_pod *params[5];

        if (param == NULL || id != SPA_PARAM_Format)
                return;

        if (spa_format_parse(param,
                        &data->format.media_type,
                        &data->format.media_subtype) < 0)
                return;

        if (data->format.media_type != SPA_MEDIA_TYPE_video )
                return;

        if( data->format.media_subtype == SPA_MEDIA_SUBTYPE_raw )
        {
            if( spa_format_video_raw_parse(param, &data->format.info.raw) >= 0 )
            {
                printf("got video format:\n");
                printf("  format: %d (%s)\n", data->format.info.raw.format,
                        spa_debug_type_find_name(spa_type_video_format,
                                data->format.info.raw.format));
                printf("  size: %dx%d\n", data->format.info.raw.size.width,
                        data->format.info.raw.size.height);
                printf("  framerate: %d/%d\n", data->format.info.raw.framerate.num,
                        data->format.info.raw.framerate.denom);
            }
        }

	       if( data->format.media_subtype == SPA_MEDIA_SUBTYPE_h264 )
        {
            if( spa_format_video_h264_parse(param, &data->format.info.h264) >= 0 )
            {
                printf("got video format: H264\n");
/*                printf("  format: %d (%s)\n", data->format.info.h264.stream_format,
                        spa_debug_type_find_name(spa_type_video_format,
                                data->format.info.h264.stream_format));*/
                printf("  size: %dx%d\n", data->format.info.h264.size.width,
                        data->format.info.h264.size.height);
                printf("  framerate: %d/%d\n", data->format.info.h264.framerate.num,
                        data->format.info.h264.framerate.denom);
            }
        }
               /* a SPA_TYPE_OBJECT_ParamBuffers object defines the acceptable size,
         * number, stride etc of the buffers */

        params[0] = spa_pod_builder_add_object(&b,
                SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
                SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1<<SPA_DATA_MemPtr)));

                /* we are done */
        pw_stream_update_params(stream, params, 1);

}
/* }}} */

/* {{{ PIP_current_time() */
void PIP_current_time(  char *sys_time)
{
    time_t time_now;
    struct tm *timeinfo;

    if ( NULL != sys_time )
    {
        time( &time_now );

        timeinfo = localtime( &time_now );

        strftime( sys_time, 21, "%F:%T", timeinfo );  //Setting format of time
    }
}
/* }}} */

/* {{{ PIP_TerminateFrame() */
int PIP_TerminateFrame ( )
{
	int i=0;

        if(FrameDebug) DEBUG("%s(%d): Entering Gstreamer based rmsframe Disable...\n", __FILE__, __LINE__);

        if(pip_frame_buffer)
        {
                free(pip_frame_buffer);
                pip_frame_buffer = NULL;
        }

	for( i=0; i<MAX_NAL_SIZE; i++)
	{
	    free( pip_bufferqueue[i].buffer );
	    pip_bufferqueue[i].buffer = 0;
	}

        pthread_mutex_destroy(&pip_queueLock);

        return 0;
}
/* }}} */

#endif /* ENABLE_RMS_STREAM_WITH_PIPEWIRE */
