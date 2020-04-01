/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
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
**/
#include "v4l2logger.h"
#include "v4l2controller.h"
#include <stdlib.h>
#include <sys/time.h>
#ifdef WIN32
#include <WinSock.h>
#endif

#define TS_MS(x) (double) (x.tv_sec * 1000) + ((double) x.tv_usec / 1000)

double diff(timeval t1, timeval t2) {
	double d1 = (double) (t1.tv_sec * 1000) + ((double) t1.tv_usec / 1000);
	double d2 = (double) (t2.tv_sec * 1000) + ((double) t2.tv_usec / 1000);
	printf("diff = %f\n", d1 - d2);
	return d1 - d2;
}

int main() {
	V4L2Controller controller;
	if (!controller.Initialize()) {
		V4L2Logger::Log(ALL, "Unable to initialize device");
		return -1;
	}

	controller.StartDeviceStreamStatus(false);
	controller.StartDeviceStreamStatus(true);
//	struct timeval current;

//	gettimeofday(&current, NULL);
//	double currentMs = TS_MS(current); 
//	double targetMs = currentMs;
	while(1) {
//		if (diff(target, current)  <= 33.333) {
//		if (targetMs - currentMs  <= 0) {
			V4L2Logger::Log(SCREEN, "Getting buffer");
			IOBuffer *buf = controller.GetVideoBuffer();
			if (buf != NULL) {
				V4L2Logger::Log(LOGFILE, "Video buffer:\n%s", STR(buf->ToString()));
			}
			if (controller._hasAudio) {
				IOBuffer *buf = controller.GetAudioBuffer();
				if (buf != NULL) {
					V4L2Logger::Log(LOGFILE, "Audio buffer:\n%s", STR(buf->ToString()));
				}
			}
//			targetMs = currentMs + 33.333;
//			printf("New Target = %f\n", targetMs);
//		}
//		gettimeofday(&current, NULL);
//		currentMs = TS_MS(current);
//		printf("Current = %f\n", currentMs);
	}
	return 0;
}
