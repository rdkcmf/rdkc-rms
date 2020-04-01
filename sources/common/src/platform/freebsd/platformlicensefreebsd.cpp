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


#ifdef FREEBSD
#ifdef HAS_LICENSE
#include "common.h"
#include <sys/utsname.h>

int8_t getMACAddresses(vector<string> & addresses) {
	addresses.clear();
	struct ifaddrs *if_addrs = NULL;
	struct ifaddrs *if_addr = NULL;
	if (0 == getifaddrs(&if_addrs)) {
		for (if_addr = if_addrs; if_addr != NULL; if_addr = if_addr->ifa_next) {
			// GET MAC address
			if (if_addr->ifa_addr != NULL && if_addr->ifa_addr->sa_family == AF_LINK) {
				struct sockaddr_dl* sdl = (struct sockaddr_dl *) if_addr->ifa_addr;
				unsigned char mac[6];
				// check if we got the first mac address
				if (6 == sdl->sdl_alen) {
					memcpy(mac, LLADDR(sdl), sdl->sdl_alen);
					string buff;
					buff = format("%02X:%02X:%02X:%02X:%02X:%02X",
							(unsigned char) mac[0],
							(unsigned char) mac[1],
							(unsigned char) mac[2],
							(unsigned char) mac[3],
							(unsigned char) mac[4],
							(unsigned char) mac[5]);
					addresses.push_back(buff);
					break;
				}
			}
		}
		freeifaddrs(if_addrs);
		if_addrs = NULL;
	}
	return addresses.size();
}

string getGID() {
	gid_t gid = getgid();
	string retString = format("%"PRIu32"", gid);
	return retString;
}

string getKernelVersion() {
	struct utsname buf;

	memset(&buf, 0, sizeof (struct utsname));
	uname(&buf);

	return STR(buf.release);
}

int64_t getTotalSystemMemory() {
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
}

string getUID() {
	uid_t uid = getuid();
	string retString = format("%"PRIu32"", uid);
	return retString;
}



#endif /* HAS_LICENSE */
#endif /* FREEBSD */
