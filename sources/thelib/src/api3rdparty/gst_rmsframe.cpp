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

#include "api3rdparty/gst_rmsframe.h"
#include <netinet/in.h>
#include "utils/logging/logging.h"

#define GST_ARRAYMAXSIZE    100
#define PORT                8090
#define SERVER              "127.0.0.1"
#define MAXLINE             30
#define ABSOLUTETIME_LENGTH 8
#define MAPLENGTH           4
#define MAX_QUEUESIZE       30

pthread_attr_t  attr;
pthread_t       getFrame;
pthread_mutex_t queueLock;
pthread_t       thread_id;

volatile static int gstTerminateFrame = 0;
volatile static int pushIndex=0;

guint8* frame_timestamp  = NULL;
guint8* frame_buffer     = NULL;

static int  StreamStartednStopped = 0;
static bool FrameDebug(false);
static int _gst_SocketFd;

int data_size      = 0;
int buffer_width   = 1280;
int buffer_height  = 720;
int streamId_Video = 4;
int streamId_Audio = 0;
int streamId       = 0;
int stream_type    = 0;
int sockfd;
int clientsockfd;

char appName_Video[32] = {'\0'};
char appName_Audio[32] = {'\0'};

struct sockaddr_in servaddr;
uint8_t *videobuffer = ( uint8_t * ) malloc( sizeof( uint8_t ) * 8 );
static signed  long long  videobuf_length  = 0;

typedef struct
{
    guint8* buffer;
    guint8* pts;
    unsigned long long size;
}BUFFERQUEUE;  //same struct definition

BUFFERQUEUE bufferqueue[MAX_QUEUESIZE]; //but create an array of line here.

int set_bufferParams( int in_streamId_Video );
int CreateClientSocket(  );
int CreateServerSocket(  );
int getSocket();
int pushBuffer( unsigned long long frameSize, guint8 *frame , guint8 *pts );
int popBuffer(GST_FrameInfo *frame_info );

/** @description: Thread function  for creating UDP server.
 *  @param[in] elt: none
 *  @return: None
 */
void *UdpServer( void *vargp )
{
    int ret = CreateServerSocket(  );
    
    if ( RDKC_FAILURE == ret )
    {
        INFO( "%s(%d): Failed to create server socket\n", __FILE__,
              __LINE__ );
    }

    return NULL;
}
/** @description: Callback function from GST on new frame sample.
 *  @param[in] elt: GST element
 *  @return: None
 */
void  *on_new_sample( GstElement * elt )
{
       static unsigned long long mapsize=0;
       static int absolutetime_flag = 1;
       static int mapsize_flag = 0;
       static int mapdata_flag = 0;
       int ret=0;

       GstSample *sample;
       GstBuffer *buffer;
       GstMapInfo map;

       sample = gst_app_sink_pull_sample( GST_APP_SINK( elt ) );
       buffer = gst_sample_get_buffer( sample );
       gst_buffer_map( buffer, &map, GST_MAP_READ );
       videobuf_length = map.size + videobuf_length;
       videobuffer =( uint8_t * ) realloc(videobuffer,sizeof( uint8_t ) * (videobuf_length ));

       if( NULL == videobuffer )
        {
		INFO( "%s(%d): realloc failed \n",__FILE__, __LINE__);
        }

       memcpy(&videobuffer[videobuf_length - map.size] , map.data,map.size);

       if(FrameDebug) DEBUG("%s(%d): Entering New Sample Callback \n",__FILE__, __LINE__);

       if( (videobuf_length >= ABSOLUTETIME_LENGTH ) && ( 1 == absolutetime_flag ) && (0 == gstTerminateFrame ) )
        {
                if(FrameDebug) DEBUG("%s(%d): Timestamp received \n",__FILE__, __LINE__);
                memset(frame_timestamp, 0, TIMESTAMP_SIZE);
                memcpy(frame_timestamp,videobuffer , TIMESTAMP_SIZE);
                memmove(&videobuffer[0],&videobuffer[ABSOLUTETIME_LENGTH],videobuf_length - ABSOLUTETIME_LENGTH);
                videobuf_length = videobuf_length - ABSOLUTETIME_LENGTH ;
                absolutetime_flag = 0;
                mapsize_flag = 1;
        }

        if( (videobuf_length >= MAPLENGTH ) && ( 1 == mapsize_flag ) && (0 == gstTerminateFrame ) )
        {
                memcpy(&mapsize,videobuffer, MAPLENGTH);
                memmove(&videobuffer[0],&videobuffer[MAPLENGTH],videobuf_length - MAPLENGTH);
                videobuf_length =videobuf_length - MAPLENGTH ;
                mapsize_flag = 0;
                mapdata_flag = 1;
        }

       if( (videobuf_length >= mapsize ) && ( 1 == mapdata_flag ) && (0 == gstTerminateFrame )  )
        {
                frame_buffer = (guint8 *)realloc(frame_buffer, sizeof(guint8)* mapsize);
                memcpy(frame_buffer, &videobuffer[0], mapsize);
                memmove(&videobuffer[0],&videobuffer[mapsize],videobuf_length - mapsize );
		ret = pushBuffer( mapsize, frame_buffer , frame_timestamp );
		
                if ( RDKC_SUCCESS != ret)
		{
		   FATAL( "%s(%d): Pushing to the queue failed \n",__FILE__, __LINE__);
		}

                data_size= mapsize;
                stream_type = 1; /* For H264 Video */
                streamId = streamId_Video;
                videobuf_length =videobuf_length - mapsize;
                absolutetime_flag =1;
                mapdata_flag =0;
		ret = CreateClientSocket(  );

                if ( RDKC_FAILURE == ret )
                 {
                   INFO( "%s(%d): Failed to create client socket \n",__FILE__, __LINE__);
                 }

                if(FrameDebug) DEBUG("%s(%d): Video frame received \n",__FILE__, __LINE__);
         }

        gst_buffer_unmap( buffer, &map );
        gst_sample_unref( sample );
        return NULL;
}



/** @description: API called from Application to read new Video or Audio frame data.
 *  @param[out] frame_info: GST GST_FrameInfo structure pointer to fill frame data.
 *  @return: SUCCESS or FAILURE
 */
int gst_ReadFrameData( GST_FrameInfo *frame_info )
{
	int ret=0;

	if(FrameDebug) DEBUG("%s(%d): Entering gst_ReadFrameData() fn \n",__FILE__, __LINE__);

	if( StreamStartednStopped == 1)
	{
		FATAL("%s(%d): Error : GST Stream started and stopped \n", __FILE__, __LINE__);
		return -1;
	}
	if( pushIndex > 0 )
	{
		ret = popBuffer(frame_info );
                if ( RDKC_SUCCESS != ret)
                {
                   FATAL( "%s(%d): Failed to read data from queue \n",__FILE__, __LINE__);
                }
		frame_info->stream_id = streamId;
		frame_info->stream_type = stream_type;  // Video or Audio frame
		frame_info->width = buffer_width;
		frame_info->height = buffer_height;
		if(FrameDebug) DEBUG("%s(%d): RMS frame data Read \n",__FILE__, __LINE__);
	}
	else
	{
		return 1; // No Frame data
	}

	return 0;
}

/** @description: Callback function from GST on EOS or stream error message
 *  @param[in] bus: GSTBus
 *  @param[in] message: GstMessage (EOS or ERROR)
 *  @param[in] user_data: User data related to stream
 *  @return: None
 */
static gboolean on_message( GstBus * bus, GstMessage * message, gpointer user_data )
{
	GError *err = NULL;
	gchar *dbg_info = NULL;

	switch ( GST_MESSAGE_TYPE( message ) )
	{
		case GST_MESSAGE_EOS:
			if(FrameDebug) DEBUG("%s(%d): EOS of startstream request reached\n",__FILE__, __LINE__);
			g_main_loop_quit( ( GMainLoop * ) user_data );
			break;

		case GST_MESSAGE_ERROR:
			if(FrameDebug) DEBUG("%s(%d): Stream request ERROR has occured,error_code: %d\n",
				__FILE__, __LINE__,GST_MESSAGE_ERROR);
			gst_message_parse_error (message, &err, &dbg_info);
			g_error_free (err);
			g_free (dbg_info);
			g_main_loop_quit( ( GMainLoop * ) user_data );
			break;

		default:
			break;
	}

	return TRUE;
}

/** @description: Callback function from GST on EOS or stream error message
 *  @param[in] bus: GSTBus
 *  @param[in] message: GstMessage (EOS or ERROR)
 *  @param[in] user_data: User data related to stream
 *  @return: None
 */
static gboolean on_message_for_stoprequest( GstBus * bus, GstMessage * message, gpointer user_data )
{
	switch ( GST_MESSAGE_TYPE( message ) )
	{
		case GST_MESSAGE_EOS:
			if(FrameDebug) DEBUG("%s(%d): Stop request EOS reached\n",__FILE__, __LINE__);
			g_main_loop_quit( ( GMainLoop * ) user_data );
			break;

		case GST_MESSAGE_ERROR:
			if(FrameDebug) DEBUG("%s(%d): Stop request ERROR has occured,error_code: %d\n",
				__FILE__, __LINE__,GST_MESSAGE_ERROR);
			g_main_loop_quit( ( GMainLoop * ) user_data );
			break;

		default:
			break;
	}
	return TRUE;
}

/** @description: API from application to initialize gst based get frame functionality
 *  @param[in] in_appName_Video: App name to initialize Video stream
 *  @param[in] in_appName_Audio: App name to initialize Audio stream
 *  @return: SUCCESS or FAILURE
 */
int gst_InitFrame ( char *in_appName_Video, char *in_appName_Audio )
{
	gstTerminateFrame=0;
	sockfd = getSocket();
	pthread_create( &thread_id, NULL, UdpServer, NULL );

	if(FrameDebug) INFO("%s(%d): Entering Gstreamer based rmsframe enable...\n", __FILE__, __LINE__);

	strcpy(appName_Video, in_appName_Video);
	strcpy(appName_Audio, in_appName_Audio);

        frame_buffer = ( guint8 * ) ( malloc( MAX_NAL_SIZE ) );

        if(frame_buffer == NULL)
        {
                FATAL("%s(%d): Malloc for frameBuffer failed...\n", __FILE__, __LINE__);
	        close(sockfd);
                return -1;
        }

        frame_timestamp = ( guint8 * ) ( malloc( TIMESTAMP_SIZE ) );

        if(frame_timestamp == NULL)
        {
                FATAL("%s(%d): Malloc for frame_timestamp failed...\n", __FILE__, __LINE__);
                free (frame_buffer);
		close(sockfd);
                return -1;
        }

	if(FrameDebug) DEBUG("%s(%d): AFTER MALLOC frame_buffer=%d ...\n", __FILE__, __LINE__, frame_buffer);

	gst_init( NULL, NULL ); /*Initializing GStreamer*/

	if(pthread_mutex_init(&queueLock, NULL) != 0)
	{
		FATAL("%s(%d): Mutex init for queue failed...\n", __FILE__, __LINE__);
		free (frame_buffer);
		free (frame_timestamp);
		close(sockfd);
		return -1;
	}

	int ret = 0;
	ret = pthread_attr_init(&attr);

	if(ret != 0 )
	{
		WARN("%s(%d): Failed to initialize attributes,error_code: %d\n", __FILE__, __LINE__,ret);
	}

	size_t stacksize = PTHREAD_STACK_MIN;
	ret  = pthread_attr_setstacksize(&attr, stacksize);

        if(0 != ret  )
        {
            FATAL("%s(%d): Failed to set stack size,error_code: %d\n",__FILE__, __LINE__,ret);
        }

	ret = pthread_create( &getFrame, &attr, start_Video_Streaming, NULL );

	if(ret != 0)
	{
		FATAL("%s(%d): Failed to create start_Streaming thread,error_code: %d\n",
			__FILE__, __LINE__,ret);
		close(sockfd);
		pthread_mutex_destroy(&queueLock);
		free (frame_buffer);
		free (frame_timestamp);
		return -1;
	}
#if 0
	ret = pthread_create( &getFrame, &attr, start_Audio_Streaming, NULL );
	if(ret != 0)
	{
		FATAL("%s(%d): Failed to create start_Streaming thread,error_code: %d\n",
			__FILE__, __LINE__,ret);
		close(sockfd);
		pthread_mutex_destroy(&frameMutex);
		free (frame_buffer);
		free (frame_timestamp);
		return -1;
	}
#endif
	if(FrameDebug) DEBUG("%s(%d): Exiting Gstreamer based gstrmsframe enabled...\n", __FILE__, __LINE__);

	return sockfd;
}

/** @description: Function to set the buffer params
 *  @param[in] in_streamId_Video: Video stream ID
 *  @return: SUCCESS or FAILURE
 */
int set_bufferParams ( int in_streamId_Video )
{
	if(FrameDebug) DEBUG("%s(%d): Entering init_gstmem in_streamId_Video=%d...\n", __FILE__, __LINE__, in_streamId_Video);

	switch( in_streamId_Video )
	{
		case 4:
			buffer_width = 1280;
			buffer_height = 720;
			streamId_Video = 4;
			break;
		case 5:
			buffer_width = 640;
			buffer_height = 480;
			streamId_Video = 5;
			break;
		case 6:
			buffer_width = 320;
			buffer_height = 240;
			streamId_Video = 6;
			break;
		case 7:
			buffer_width = 640;
			buffer_height = 480;
			streamId_Video = 7;
			break;
		default:
			FATAL("%s(%d): Invalid in_streamId_Video=%d...\n", __FILE__, __LINE__, in_streamId_Video);
			return -1;
	}

	return 0;
}

/** @description: Thread function to start video streaming
 *  @param[in] vargp: thread arguments
 *  @return: None
 */
void *start_Video_Streaming( void *vargp )
{
	if(FrameDebug) DEBUG("%s(%d): Entering gstrmsframe start streaming thread...\n",
		__FILE__, __LINE__);

l1: 	GstElement *souphttpsrc = NULL, *pipeline = NULL, *appsink = NULL;
	GMainLoop *loop = NULL;
	pushIndex=0;
	loop = g_main_loop_new( NULL, FALSE );
	appsink = gst_element_factory_make( "appsink", NULL );
	pipeline = gst_element_factory_make( "pipeline", NULL );
	souphttpsrc = gst_element_factory_make( "souphttpsrc", NULL );

	if ( !pipeline || !appsink || !souphttpsrc )
	{
		FATAL("%s(%d): Failed to make elements %d %d %d \n",
			__FILE__, __LINE__, pipeline, appsink, souphttpsrc);
		sleep(1);
		goto l1;
	}

	GstBus *bus = NULL;
	g_object_set( G_OBJECT( appsink ), "emit-signals", TRUE, "sync", FALSE,
			NULL );

	bus = gst_pipeline_get_bus( GST_PIPELINE( pipeline ) );
	gst_bus_add_watch( bus, ( GstBusFunc ) on_message, ( gpointer ) loop );

	gst_bin_add_many( GST_BIN( pipeline ), souphttpsrc, appsink, NULL );

	if( !gst_element_link_many( souphttpsrc, appsink, NULL ) )
		FATAL("%s(%d): Failed to link elements \n",__FILE__, __LINE__);

	StreamStartednStopped = 0;
	g_signal_connect( appsink, "new-sample", G_CALLBACK( on_new_sample ),	NULL ); /* This API is called when frame is captured */

#ifdef RMS_PLATFORM_RPI
        /* Stream params with flags */
        int do_timestamp = 1;        /* timestamp required for Video */
        int framerate = 30;         /* Video framerate */
        char startrequest[GST_ARRAYMAXSIZE] = { 0 };

        sprintf( startrequest,
                        "http://127.0.0.1:8080/startstream&do_timestamp=%d&framerate=%d",
                                        do_timestamp,
                                        framerate);
#else
	/* Stream params with flags */
	int format = 0;             /* for H264 Video */
	int dotimestamp = 1;        /* timestamp required for Video */
	int framerate = 15;         /* Video framerate */
	int bitrate = 768;          /* Video bitrate, default value */
	char startrequest[GST_ARRAYMAXSIZE] = { 0 };

	sprintf( startrequest,
                        "http://127.0.0.1:8085/startstream&appname=%s&format=%d&dotimestamp=%d&framerate=%d&",
                        appName_Video, format, dotimestamp, framerate );
#endif

	if(FrameDebug) INFO("%s(%d): gstrmsframe Start request URL is: %s\n",__FILE__, __LINE__,startrequest);
	g_object_set( G_OBJECT( souphttpsrc ), "location", startrequest, NULL );
	g_object_set( G_OBJECT( souphttpsrc ), "is-live", TRUE, NULL );
	g_object_set( G_OBJECT( souphttpsrc ), "blocksize", MAX_NAL_SIZE, NULL );
	gst_element_set_state( pipeline, GST_STATE_PLAYING );
	if(FrameDebug) DEBUG("%s(%d): gst_element_set_state\n",__FILE__, __LINE__);

        /* ToDo : If stream Id that is used is different than assumed (4) after startstream request,
		  then get stream Id which is used by cam_stream server for RMS */
        /*char getrequest[GST_ARRAYMAXSIZE] = { 0 };
	char getresponse[GST_ARRAYMAXSIZE];
        getresponse[0] = '\0';

        sprintf( getrequest,
                        "http://127.0.0.1:8085/getstream&appname=%s&keyvalue=**&", appName_Video);
	send_request( getrequest, getresponse );
        */

	set_bufferParams( 4 /* video stream Id */ /* This should be response value from getrequest above */ );

	g_main_loop_run( loop );

	if(FrameDebug) INFO("%s(%d): Exited from gstrmsframe loop\n",__FILE__, __LINE__);
	gst_element_set_state( appsink, GST_STATE_READY );
	gst_element_set_state( souphttpsrc, GST_STATE_READY );
	gst_element_set_state( appsink, GST_STATE_NULL );
	gst_element_set_state( souphttpsrc, GST_STATE_NULL );

	gst_element_set_state( pipeline, GST_STATE_NULL );

	if ( gst_element_get_state( pipeline, NULL, NULL, GST_CLOCK_TIME_NONE ) ==
					GST_STATE_CHANGE_SUCCESS )
	{
		if(FrameDebug) DEBUG("%s(%d): The state of pipeline changed to GST_STATE_NULL successfully\n",
			__FILE__, __LINE__);
	}
	else
	{
		WARN("%s(%d): Changing the state of pipeline to GST_STATE_NULL failed\n",
			__FILE__, __LINE__);
	}

	gst_element_unlink_many( souphttpsrc, appsink, NULL );
	gst_object_ref( souphttpsrc );
	gst_object_ref( appsink );
	gst_bin_remove_many( GST_BIN( pipeline ), appsink, souphttpsrc, NULL );
	gst_object_unref( bus );
	gst_object_unref( pipeline );
	gst_object_unref( souphttpsrc );
	gst_object_unref( appsink );

	if(FrameDebug) INFO("%s(%d): Pipeline deleted\n",__FILE__, __LINE__);
	if(FrameDebug) INFO("************* Exiting GST stream start  \n");

	sleep(1);
	goto l1;

	return 0;
}

/** @description: Thread function to start audio streaming
 *  @param[in] vargp: thread arguments
 *  @return: None
 */
void *start_Audio_Streaming( void *vargp )
{
	if(FrameDebug) DEBUG("%s(%d): Entering gstrmsframe start audio streaming thread...\n",
		__FILE__, __LINE__);

	GstElement *souphttpsrc = NULL, *pipeline = NULL, *appsink = NULL;
	GMainLoop *loop = NULL;
	loop = g_main_loop_new( NULL, FALSE );
	appsink = gst_element_factory_make( "appsink", NULL );
	pipeline = gst_element_factory_make( "pipeline", NULL );
	souphttpsrc = gst_element_factory_make( "souphttpsrc", NULL );

	if ( !pipeline || !appsink || !souphttpsrc )
	{
		FATAL("%s(%d): Failed to make elements %d %d %d \n",
			__FILE__, __LINE__, pipeline, appsink, souphttpsrc);
	}

	GstBus *bus = NULL;

	g_object_set( G_OBJECT( appsink ), "emit-signals", TRUE, "sync", FALSE,
			NULL );
	bus = gst_pipeline_get_bus( GST_PIPELINE( pipeline ) );
	gst_bus_add_watch( bus, ( GstBusFunc ) on_message, ( gpointer ) loop );

	gst_bin_add_many( GST_BIN( pipeline ), souphttpsrc, appsink, NULL );

	if( !gst_element_link_many( souphttpsrc, appsink, NULL ) )
		FATAL("%s(%d): Failed to link elements \n",__FILE__, __LINE__);

	g_signal_connect( appsink, "new-sample", G_CALLBACK( on_new_sample ),	NULL ); /* This API is called when frame is captured */

	/* Stream params with flags */
	int format = 1;             /* for Audio */
	int dotimestamp = 1;        /* timestamp required for Audio */
	char startrequest[GST_ARRAYMAXSIZE] = { 0 };

	sprintf( startrequest,
                        "http://127.0.0.1:8085/startaudiostream&keyvalue=%d&format=%d&dotimestamp=%d&",
                        streamId_Audio, format, dotimestamp );

	if(FrameDebug) INFO("%s(%d): gstrmsframe Audio Start request URL is: %s\n",__FILE__, __LINE__,startrequest);

	g_object_set( G_OBJECT( souphttpsrc ), "location", startrequest, NULL );
	g_object_set( G_OBJECT( souphttpsrc ), "is-live", TRUE, NULL );
	g_object_set( G_OBJECT( souphttpsrc ), "blocksize", 4096 /*MAX_NAL_SIZE*/, NULL );
	gst_element_set_state( pipeline, GST_STATE_PLAYING );

	if(FrameDebug) DEBUG("%s(%d): gst_element_set_state\n",__FILE__, __LINE__);

	g_main_loop_run( loop );

	if(FrameDebug) INFO("%s(%d): Exited from gstrmsframe Audio loop\n",__FILE__, __LINE__);

	gst_element_set_state( appsink, GST_STATE_READY );
	gst_element_set_state( souphttpsrc, GST_STATE_READY );
	gst_element_set_state( appsink, GST_STATE_NULL );
	gst_element_set_state( souphttpsrc, GST_STATE_NULL );

	gst_element_set_state( pipeline, GST_STATE_NULL );

	if ( gst_element_get_state( pipeline, NULL, NULL, GST_CLOCK_TIME_NONE ) ==
					GST_STATE_CHANGE_SUCCESS )
	{
		if(FrameDebug) DEBUG("%s(%d): The state of pipeline changed to GST_STATE_NULL successfully\n",
			__FILE__, __LINE__);
	}
	else
	{
		WARN("%s(%d): Changing the state of pipeline to GST_STATE_NULL failed\n",
			__FILE__, __LINE__);
	}

	gst_element_unlink_many( souphttpsrc, appsink, NULL );
	gst_object_ref( souphttpsrc );
	gst_object_ref( appsink );
	gst_bin_remove_many( GST_BIN( pipeline ), appsink, souphttpsrc, NULL );
	gst_object_unref( bus );
	gst_object_unref( pipeline );
	gst_object_unref( souphttpsrc );
	gst_object_unref( appsink );

	if(FrameDebug) INFO("%s(%d): Pipeline deleted\n",__FILE__, __LINE__);
	if(FrameDebug) INFO("************* Exiting GST stream start Audio \n");

	StreamStartednStopped = 1;

	return 0;
}

/** @description: API from application to terminate gst based get frame functionality
 *  @param[in] in_appName_Video: App name to terminate Video stream
 *  @param[in] in_appName_Audio: App name to terminate Audio stream
 *  @return: SUCCESS or FAILURE
 */
int gst_TerminateFrame ( char *in_appName_Video, char *in_appName_Audio )
{
	gstTerminateFrame = 1;

	if(FrameDebug) DEBUG("%s(%d): Entering Gstreamer based rmsframe Disable...\n", __FILE__, __LINE__);

	stop_Video_Streaming( in_appName_Video );

	if(frame_buffer)
	{
		free(frame_buffer);
		frame_buffer = NULL;
	}
	if(frame_timestamp)
	{
		free(frame_timestamp);
		frame_timestamp = NULL;
	}

	StreamStartednStopped = 1;
	pthread_mutex_destroy(&queueLock);
	close(_gst_SocketFd);

	return 0;
}

/** @description: Function to terminate video streaming
 *  @param[in] in_appName_Video: App name to terminate Video stream
 *  @return: None
 */
void stop_Video_Streaming( char *in_appName_Video )
{
	if(FrameDebug) INFO("******************** INSIDE STOP STREAMING, in_appName_Video=%s", in_appName_Video);
	if(FrameDebug) DEBUG("%s(%d): Stopping Pipeline via EOS signal!!\n",__FILE__, __LINE__);

	char stoprequest[GST_ARRAYMAXSIZE];
	char stopresponse[GST_ARRAYMAXSIZE];

	stopresponse[0] = '\0';

#ifdef RMS_PLATFORM_RPI
	sprintf( stoprequest,"http://127.0.0.1:8080/stopstream");
#else
	sprintf( stoprequest,"http://127.0.0.1:8085/stopstream&appname=%s&", in_appName_Video );
#endif

	if(FrameDebug) DEBUG("%s(%d): Stop request URL:%s\n",__FILE__, __LINE__,stoprequest);

	send_request( stoprequest, stopresponse );

	if(FrameDebug) DEBUG("%s(%d): Streaming stopped!!!\n",__FILE__, __LINE__);
}

/** @description: Function to terminate audio streaming
 *  @param[in] in_appName_Audio: App name to terminate Audio stream
 *  @return: None
 */
void stop_Audio_Streaming( char *in_appName_Audio )
{
	if(FrameDebug) INFO("******************** INSIDE STOP STREAMING, in_appName_Audio=%s", in_appName_Audio);
	if(FrameDebug) DEBUG("%s(%d): Stopping Pipeline via EOS signal!!\n",__FILE__, __LINE__);

	char stoprequest[GST_ARRAYMAXSIZE];
	char stopresponse[GST_ARRAYMAXSIZE];

	stopresponse[0] = '\0';
	sprintf( stoprequest,"http://127.0.0.1:8085/stopaudiostream&keyvalue=%d&", streamId_Audio );

	if(FrameDebug) DEBUG("%s(%d): Stop request URL:%s\n",__FILE__, __LINE__,stoprequest);

	send_request( stoprequest, stopresponse );

	if(FrameDebug) DEBUG("%s(%d): Streaming stopped!!!\n",__FILE__, __LINE__);
}

/** @description: Internal function to stop video and audio streaming
 *  @param[in] request: Stop request
 *  @param[in] response: Stop response
 *  @return: None
 */
void send_request( char *request, char *response )
{
	GstElement *req_souphttpsrc, *req_pipeline, *req_appsink;
	GMainLoop *loop2;

	loop2 = g_main_loop_new( NULL, FALSE );

	GstBus *req_bus;

	req_pipeline = gst_element_factory_make( "pipeline", NULL );
	req_appsink = gst_element_factory_make( "appsink", NULL );
	req_souphttpsrc = gst_element_factory_make( "souphttpsrc", NULL );

	g_object_set( G_OBJECT( req_appsink ), "emit-signals", TRUE, "sync", FALSE,
				NULL );
	req_bus = gst_pipeline_get_bus( GST_PIPELINE(req_pipeline ) );

	gst_bus_add_watch( req_bus, ( GstBusFunc ) on_message_for_stoprequest, ( gpointer ) loop2 );

	if ( !req_souphttpsrc )
	{
		WARN("%s(%d): Failed to create httpsrc\n",__FILE__, __LINE__);
	}

	gst_bin_add_many( GST_BIN( req_pipeline ), req_souphttpsrc, req_appsink, NULL );

	if(FrameDebug) DEBUG("%s(%d): gst_bin_add_many\n",__FILE__, __LINE__);

	gst_element_link_many( req_souphttpsrc, req_appsink, NULL );

	if(FrameDebug) DEBUG("%s(%d): gst_bin_link_many\n",__FILE__, __LINE__);

	g_signal_connect( req_appsink, "new-sample", G_CALLBACK( fetch_response ),
			( gpointer ) response );

	g_object_set( G_OBJECT( req_souphttpsrc ), "location", request, NULL );

	if(FrameDebug) DEBUG("%s(%d): g_object_set\n",__FILE__, __LINE__);

	gst_element_set_state( req_appsink, GST_STATE_PLAYING );
	gst_element_set_state( req_souphttpsrc, GST_STATE_PLAYING );
	gst_element_set_state( req_pipeline, GST_STATE_PLAYING );

	g_main_loop_run( loop2 );

	if(FrameDebug) DEBUG("%s(%d): Exited from loop2\n",__FILE__, __LINE__);

	gst_element_set_state( req_appsink, GST_STATE_READY );
	gst_element_set_state( req_souphttpsrc, GST_STATE_READY );
	gst_element_set_state( req_appsink, GST_STATE_NULL );
	gst_element_set_state( req_souphttpsrc, GST_STATE_NULL );
	gst_element_set_state( req_pipeline, GST_STATE_NULL );

	if ( gst_element_get_state( req_pipeline, NULL, NULL, GST_CLOCK_TIME_NONE ) ==
			GST_STATE_CHANGE_SUCCESS )
	{
		if(FrameDebug) DEBUG("%s(%d): The state of pipeline changed to GST_STATE_NULL successfully\n",
			__FILE__, __LINE__);
	}
	else
	{
		WARN("%s(%d): Changing the state of pipeline to GST_STATE_NULL failed\n",
			__FILE__, __LINE__);
	}

	gst_element_unlink_many( req_souphttpsrc, req_appsink, NULL );
	gst_object_ref( req_souphttpsrc );
	gst_object_ref( req_appsink );
	gst_bin_remove_many( GST_BIN( req_pipeline ), req_appsink, req_souphttpsrc, NULL );
	gst_object_unref( req_bus );
	gst_object_unref( req_pipeline );
	gst_object_unref( req_souphttpsrc );
	gst_object_unref( req_appsink );
	if(FrameDebug) DEBUG("%s(%d): Pipeline deleted\n",__FILE__, __LINE__);
	if(FrameDebug) INFO("************* Exiting GST stream stop  \n");
}

/** @description: Internal function to fetch response from GST on stop video and audio streaming
 *  @param[in] elt: GST Element
 *  @param[in] user_data: User Data
 *  @return: TRUE or FALSE
 */
gboolean fetch_response( GstElement * elt, gpointer user_data )
{
	char *response;

	response = ( char * ) user_data;

	char *str;
	GstSample *sample;
	GstBuffer *buffer;
	GstMapInfo map;

	sample = gst_app_sink_pull_sample( GST_APP_SINK( elt ) );
	buffer = gst_sample_get_buffer( sample );
	gst_buffer_map( buffer, &map, GST_MAP_READ );
	str = ( char * ) malloc( map.size );
	strncpy( str, ( char * ) map.data, map.size );
	str = strtok( str, "!" );
	strcat( response, str );
	gst_buffer_unmap( buffer, &map );
	gst_sample_unref( sample );

	return FALSE;
}

/** @description: Reading the gst feature via RFC.
 *  @param[in] file: file name
 *  @param[in] name: Name of the gst feature
 *  @return: int
 */
int IsGSTEnabledInRFC( char *file, char *name )
{
	if ( ( NULL == file ) || ( NULL ==  name ) )
	{
		FATAL("%s(%d): Either file or name is NULL\n", __FILE__, __LINE__ );
		return RDKC_FAILURE;
	}

	if(RDKC_SUCCESS != RFCConfigInit())
	{
		FATAL("%s(%d): RFCConfigInit Failed\n", __FILE__, __LINE__ );
		return RDKC_FAILURE;
	}

	if(FrameDebug) DEBUG("%s(%d): Reading RFC values\n", __FILE__, __LINE__ );

	char value[MAX_SIZE] = { 0 };

	if ( RDKC_SUCCESS == IsRFCFileAvailable( file ) )
	{
		if(FrameDebug) INFO("%s(%d): RFC file is available...", __FILE__, __LINE__ );

		if ( RDKC_SUCCESS == GetValueFromRFCFile( file, name, value ) )
		{
			if(FrameDebug) DEBUG("%s(%d): Able to get value from file\n", __FILE__, __LINE__ );

			if ( strcmp( value, RDKC_TRUE ) == 0 )
			{
				if(FrameDebug) INFO("%s(%d): RFC is set inside the file\n", __FILE__, __LINE__ );
				return RDKC_SUCCESS;
			}
			else
			{
				FATAL("%s(%d): RFC is not set inside the file\n", __FILE__, __LINE__ );
				return RDKC_FAILURE;
			}
		}
		else
		{
			FATAL("%s(%d): Unable to get  value from the file\n", __FILE__,  __LINE__ );
			return RDKC_FAILURE;
		}
	}
	else
	{
		FATAL("%s(%d): RFC file is not available\n", __FILE__, __LINE__ );
		return RDKC_FAILURE;
	}

}
/** @description: for creating socket.
 *  @param[in] elt: void
 *  @return: sockfd
 */

int getSocket()
{
    if ( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        INFO( "%s(%d): socket creation failed\n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }
    return sockfd;
}

/** @description: Creating UDP server socket
 *  @param[in] : None
 *  @return: int
 */
int CreateServerSocket(  )
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
         ( sockfd, ( const struct sockaddr * ) &servaddr1,
           sizeof( servaddr1 ) ) < 0 )
    {
        INFO( "%s(%d): Bind  failed\n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    socklen_t len, n;

    while ( 0 <=
            recvfrom( sockfd, ( char * ) buffer, MAXLINE, MSG_WAITALL,
                      ( struct sockaddr * ) &cliaddr1, &len ) )
    {
        buffer[n] = '\0';
        sendto( sockfd, ( const char * ) ack, strlen( ack ), MSG_CONFIRM,
                ( const struct sockaddr * ) &cliaddr1, len );
    }

    return RDKC_SUCCESS;
}

/** @description: Writng to server socket
 *  @param[in] : None
 *  @return: int
 */
int CreateClientSocket(  )
{
    int sockfd;

    // Creating socket file descriptor
    if ( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
    {
        INFO( "%s(%d): Socket creation failed\n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    memset( &servaddr, 0, sizeof( servaddr ) );

    // Filling server information
    servaddr.sin_family = AF_INET;  //IPV4
    servaddr.sin_port = htons( PORT );
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int n, len;

    sendto( sockfd, ( const char * ) "Trigger", strlen( "Trigger" ),
            MSG_CONFIRM, ( const struct sockaddr * ) &servaddr,
            sizeof( servaddr ) );
    close( sockfd );

    return RDKC_SUCCESS;
}


/** @description: function for pushing the frame into queue.
 *  @param[in] elt: famebuffer,framesize,pts
 *  @return: RDKC_SUCCESS if pushed successfully
 */
int pushBuffer( unsigned long long frameSize, guint8 * frame, guint8 * pts )
{
    if ( pthread_mutex_lock( &queueLock ) != 0 )
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

    bufferqueue[pushIndex].buffer = ( guint8 * ) ( malloc( frameSize ) );
    bufferqueue[pushIndex].pts = ( guint8 * ) ( malloc( sizeof( pts ) ) );
    memcpy( bufferqueue[pushIndex].buffer, frame, frameSize );
    memcpy( bufferqueue[pushIndex].pts, pts, TIMESTAMP_SIZE );
    bufferqueue[pushIndex].size = frameSize;
    pushIndex++;
    pthread_mutex_unlock( &queueLock );

    return RDKC_SUCCESS;
}

/** @description: function for pop the frame from queue.
 *  @param[in] elt: framinfo
 *  @return: RDKC_SUCCESS
 */
int popBuffer( GST_FrameInfo * frame_info )
{
    static int popIndex = 0;

    if ( pthread_mutex_lock( &queueLock ) != 0 )
    {
        FATAL( "%s(%d): Error : mutex lock failed \n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    if ( pushIndex > 0 )
    {

        frame_info->frame_size = bufferqueue[popIndex].size;
        frame_info->frame_timestamp = 0;
        frame_info->frame_timestamp =
        frame_info->frame_timestamp | bufferqueue[popIndex].pts[7];

        for ( int i = 6; i >= 0; i-- )
        {
            frame_info->frame_timestamp =
                ( frame_info->frame_timestamp << 8 ) | bufferqueue[popIndex].
                pts[i];
        }

        frame_info->frame_timestamp = frame_info->frame_timestamp / 1000000;    /* convert time from nano sec to milli sec */
        frame_info->frame_ptr = bufferqueue[popIndex].buffer;
    }
    else
    {
        if ( FrameDebug )
            DEBUG( "%s(%d): Queue is empty \n", __FILE__, __LINE__ );
    }

    pthread_mutex_unlock( &queueLock );

    return RDKC_SUCCESS;
}

/** @description: function for deleting single node from queue.
 *  @param[in] elt: Void
 *  @return: RDKC_SUCCESS
 */
int deleteBuffer(  )
{
    if ( pthread_mutex_lock( &queueLock ) != 0 )
    {
        INFO( "%s(%d): Mutex lock failed \n", __FILE__, __LINE__ );
        return RDKC_FAILURE;
    }

    int i;

    if ( 1 == pushIndex )
    {
        pushIndex--;
        free( bufferqueue[0].buffer );
        free( bufferqueue[0].pts );
        bufferqueue[0].size = 0;
    }
    else if ( pushIndex > 1 )
    {
        for ( i = 0; i < ( pushIndex - 1 ); i++ )
        {
            bufferqueue[i].buffer =
                ( guint8* ) ( realloc( bufferqueue[i].buffer,
                                 bufferqueue[i + 1].size ) );

            memcpy( bufferqueue[i].buffer, bufferqueue[i + 1].buffer,
                    bufferqueue[i + 1].size );
            memcpy( bufferqueue[i].pts, bufferqueue[i + 1].pts,
                    TIMESTAMP_SIZE );
            bufferqueue[i].size = bufferqueue[i + 1].size;
        }

        free( bufferqueue[i].buffer );
        free( bufferqueue[i].pts );
        bufferqueue[i].size = 0;
        pushIndex--;
    }
    else
    {
        if ( FrameDebug )
            DEBUG( "%s(%d): Nothing to delete \n", __FILE__, __LINE__ );
    }
    pthread_mutex_unlock( &queueLock );

    return RDKC_SUCCESS;
}

#endif /* RMS_PLATFORM_RPI */

/**************END*****************/
