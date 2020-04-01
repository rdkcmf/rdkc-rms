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


#ifdef SOLARIS
#ifdef HAS_LICENSE

#include "platform/solaris/solarisplatform.h"
#include "common.h"

int8_t getMACAddresses(vector<string> & addresses) {
	//	addresses.clear();
	//	struct ifaddrs *if_addrs = NULL;
	//	struct ifaddrs *if_addr = NULL;
	//
	//	struct ifreq ifr;
	//	memset(&ifr, 0x0, sizeof (struct ifreq));
	//
	//	if (0 == getifaddrs(&if_addrs)) {
	//		for (if_addr = if_addrs; if_addr != NULL; if_addr = if_addr->ifa_next) {
	//			// GET MAC address
	//			if ((if_addr->ifa_addr != NULL) &&
	//					(if_addr->ifa_data != 0) &&
	//					(if_addr->ifa_addr->sa_family == AF_PACKET) &&
	//					(strcmp("lo", if_addr->ifa_name))) {
	//				// ^ Ignore loopback
	//				strcpy(ifr.ifr_name, if_addr->ifa_name);
	//
	//				int tsocket = socket(PF_INET, SOCK_STREAM, 0); //NOINHERIT
	//				if (tsocket >= 0 && ioctl(tsocket, SIOCGIFHWADDR, &ifr) >= 0) {
	//					close(tsocket);
	//
	//					string buff;
	//					buff = format("%02X:%02X:%02X:%02X:%02X:%02X",
	//							(unsigned char) ifr.ifr_hwaddr.sa_data[0],
	//							(unsigned char) ifr.ifr_hwaddr.sa_data[1],
	//							(unsigned char) ifr.ifr_hwaddr.sa_data[2],
	//							(unsigned char) ifr.ifr_hwaddr.sa_data[3],
	//							(unsigned char) ifr.ifr_hwaddr.sa_data[4],
	//							(unsigned char) ifr.ifr_hwaddr.sa_data[5]);
	//
	//					addresses.push_back(buff);
	//				}
	//			}
	//		}
	//		freeifaddrs(if_addrs);
	//		if_addrs = NULL;
	//	}
	NYI;
	return addresses.size();
}

string getUID() {
	uid_t uid = getuid();
	string retString = format("%"PRIu32"", uid);
	return retString;
}

string getGID() {
	gid_t gid = getgid();
	string retString = format("%"PRIu32"", gid);
	return retString;
}

int64_t getTotalSystemMemory() {
	getKernelVersion();
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
}

string getKernelVersion() {
	//	struct utsname buf;
	//
	//	memset(&buf, 0, sizeof (struct utsname));
	//	uname(&buf);
	//
	//	return STR(buf.release);
	NYI;
	return "NOT_YET_IMPLEMENTED";
}

#endif /* HAS_LICENSE */
#endif /* SOLARIS */
