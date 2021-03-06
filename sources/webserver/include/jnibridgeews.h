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


#ifdef ANDROID
#ifndef _JNIBRIDGE_EWS_H
#include "jni.h"
#include "common.h"

extern int startEWS(int argc, const char **argv);

string GetString(uint16_t *import, uint32_t length);

extern "C" {
JNIEXPORT jboolean JNICALL Java_com_rdkc_rdkcms_EWS_startWebServer(JNIEnv *env, jclass clazz,
		jstring configFile);

}

#endif	/* _JNIBRIDGE_EWS_H */
#endif	/* ANDROID */
