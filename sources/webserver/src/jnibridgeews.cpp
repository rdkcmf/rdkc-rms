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
#include "jni.h"
#include "jnibridgeews.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

const char *appName = "rms-webserver";
char *args[3];
string GetString(uint16_t *import, uint32_t length) {
	char *buf = new char[length + 1];
	for (uint32_t i = 0; i < length; i++) {
		buf[i] = (char) import[i];
	}
	buf[length] = '\0';
	string ret(buf);
	delete buf;
	return ret;
}

JNIEXPORT jboolean JNICALL Java_com_rdkc_rdkcms_EWS_startWebServer(JNIEnv *env, jclass clazz,
		jstring configFile) {
	//AndroidPlatform::SetAndroidEnvParameter(env, clazz);
	args[0] = (char *)appName;
	const jchar *str = env->GetStringCritical(configFile, 0);
	args[1] = strdup(STR(GetString((jchar *)str, env->GetStringLength(configFile))));
	env->ReleaseStringCritical(configFile, str);
	return startEWS(2, (const char **) args) == 0 ? JNI_TRUE : JNI_FALSE;
}

#endif	/* ANDROID */
