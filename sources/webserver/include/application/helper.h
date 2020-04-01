/*
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
 */

#ifndef _HELPER_H
#define _HELPER_H

namespace webserver {

    class Helper {
    public:

        template<typename T> inline static T AtomicIncrement(T *t) {
            return AtomicStep(t, (T)1);
        }

        template<typename T> inline static T AtomicDecrement(T *t) {
            return AtomicStep(t, (T)-1);
        }

        template<typename T, typename U> inline static T AtomicStep(T *t, U value) {
#ifdef WIN32
            return (T) InterlockedExchangeAdd64((volatile LONG64*)t, (LONG64) value);
#else
#ifdef REPLAYXD
            return (T) __sync_add_and_fetch_4((T*) t, (T) value);
#else                        
            return (T) __sync_add_and_fetch((T*) t, (T) value);
#endif
#endif
        }

		static string ConvertToLocalSystemTime(time_t const& theTime);
#ifdef WIN32
		static string GetProgramDirectory();
#endif
		static string NormalizeFolder(string const &folder);
        static string GetTemporaryDirectory();
		static string GetIPAddress(struct sockaddr const *addr, uint16_t &port, 
            bool ipv6 = false);
		static struct sockaddr *GetNetworkAddress(struct sockaddr *addr, uint16_t port, string ipAddress, bool ipv6 = false);
		static void SplitFileName(string const& filePath, string &folder, string &name, string &extension);
		static void Split(string const &str, string const &separators, vector<string> &result);
		static void ListFiles(string const &path, vector<string> &files);
		static bool GetFileInfo(string const &filePath,
#ifdef WIN32
		struct _stat64 &buf
#else
		struct stat64 &buf
#endif
			);
    };
}

#endif
