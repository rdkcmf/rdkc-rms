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
#include "jnibridge.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

const char *appName = "rdkcms";
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
JNIEXPORT jboolean JNICALL Java_com_rdkc_rdkcms_RMS_startRDKCMS(JNIEnv *env, jclass clazz,
		jstring configFile) {
	AndroidPlatform::SetAndroidEnvParameter(env, clazz);
	args[0] = (char *)appName;
	const jchar *str = env->GetStringCritical(configFile, 0);
	args[1] = strdup(STR(GetString((jchar *)str, env->GetStringLength(configFile))));
	env->ReleaseStringCritical(configFile, str);
	return startRDKCMS(2, (const char **) args) == 0 ? JNI_TRUE : JNI_FALSE;
}
JNIEXPORT jboolean JNICALL Java_com_rdkc_rdkcms_RMS_createNativeBridge(JNIEnv *env, jobject clazz,
		jint id) {
	return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL Java_com_rdkc_rdkcms_RMS_destroyNativeBridge(JNIEnv *env, jobject clazz,
		jint id) {
	return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL Java_com_rdkc_rdkcms_RMS_createNamedPipe(JNIEnv *env, jobject clazz,
		jstring filename) {
	const jchar *str = env->GetStringCritical(filename, 0);
	const char *name = strdup(STR(GetString((jchar *)str, env->GetStringLength(filename))));
//	DEBUG("[Debug] Creating named pipe: %s", name);
	__android_log_print(ANDROID_LOG_DEBUG, "rdkcms", "[Debug] Creating named pipe: %s", name);
	if (fileExists(name))
		deleteFile(name);
	int32_t pipeResult = mkfifo(name, 0777); 
	env->ReleaseStringCritical(filename, str);
	if (pipeResult == -1) {
//		FATAL("Unable to create pipe. Err: %"PRIu32, errno);
		__android_log_print(ANDROID_LOG_ERROR, "rdkcms", "Unable to create pipe (%s). Err: %"PRIu32, name, errno);
		return false;
	}
	return true;
}

JNIEXPORT jboolean JNICALL Java_com_rdkc_rdkcms_RMS_removeNamedPipe(JNIEnv *env, jobject clazz,
		jstring filename) {
	const jchar *str = env->GetStringCritical(filename, 0);
	const char *name = STR(GetString((jchar *)str, env->GetStringLength(filename)));
	__android_log_print(ANDROID_LOG_DEBUG, "rdkcms", "[Debug] Deleting named pipe: %s", name);
	bool retVal = (unlink(name) == 0);
	env->ReleaseStringCritical(filename, str);
	return retVal;
}
#endif	/* ANDROID */
