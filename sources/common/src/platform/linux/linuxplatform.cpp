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


#ifdef LINUX

#include "platform/linux/linuxplatform.h"
#include "common.h"

string alowedCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static map<int, SignalFnc> _signalHandlers;

LinuxPlatform::LinuxPlatform() {

}

LinuxPlatform::~LinuxPlatform() {
}

string GetEnvVariable(const char *pEnvVarName) {
	char *pTemp = getenv(pEnvVarName);
	if (pTemp != NULL)
		return pTemp;
	return "";
}

void replace(string &target, string search, string replacement) {
	if (search == replacement)
		return;
	if (search == "")
		return;
	string::size_type i = string::npos;
	string::size_type lastPos = 0;
	while ((i = target.find(search, lastPos)) != string::npos) {
		target.replace(i, search.length(), replacement);
		lastPos = i + replacement.length();
	}
}

bool fileExists(string path) {
	struct stat fileInfo;
	if (stat(STR(path), &fileInfo) == 0) {
		return true;
	} else {
		return false;
	}
}

string lowerCase(string value) {
	return changeCase(value, true);
}

string upperCase(string value) {
	return changeCase(value, false);
}

string changeCase(string &value, bool lowerCase) {
	string result = "";
	for (string::size_type i = 0; i < value.length(); i++) {
		if (lowerCase)
			result += tolower(value[i]);
		else
			result += toupper(value[i]);
	}
	return result;
}

string tagToString(uint64_t tag) {
	string result;
	for (uint32_t i = 0; i < 8; i++) {
		uint8_t v = (tag >> ((7 - i)*8)&0xff);
		if (v == 0)
			break;
		result += (char) v;
	}
	return result;
}

bool setMaxFdCount(uint32_t &current, uint32_t &max) {
	//1. reset stuff
	current = 0;
	max = 0;
	struct rlimit limits;
	memset(&limits, 0, sizeof (limits));

	//2. get the current value
	if (getrlimit(RLIMIT_NOFILE, &limits) != 0) {
		int err = errno;
		FATAL("getrlimit failed: (%d) %s", err, strerror(err));
		return false;
	}
	current = (uint32_t) limits.rlim_cur;
	max = (uint32_t) limits.rlim_max;

	//3. Set the current value to max value
	limits.rlim_cur = limits.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &limits) != 0) {
		int err = errno;
		FATAL("setrlimit failed: (%d) %s", err, strerror(err));
		return false;
	}

	//4. Try to get it back
	memset(&limits, 0, sizeof (limits));
	if (getrlimit(RLIMIT_NOFILE, &limits) != 0) {
		int err = errno;
		FATAL("getrlimit failed: (%d) %s", err, strerror(err));
		return false;
	}
	current = (uint32_t) limits.rlim_cur;
	max = (uint32_t) limits.rlim_max;


	return true;
}

bool enableCoreDumps() {
	struct rlimit limits;
	memset(&limits, 0, sizeof (limits));

	memset(&limits, 0, sizeof (limits));
	if (getrlimit(RLIMIT_CORE, &limits) != 0) {
		int err = errno;
		FATAL("getrlimit failed: (%d) %s", err, strerror(err));
		return false;
	}

	limits.rlim_cur = limits.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &limits) != 0) {
		int err = errno;
		FATAL("setrlimit failed: (%d) %s", err, strerror(err));
		return false;
	}

	memset(&limits, 0, sizeof (limits));
	if (getrlimit(RLIMIT_CORE, &limits) != 0) {
		int err = errno;
		FATAL("getrlimit failed: (%d) %s", err, strerror(err));
		return false;
	}

	return limits.rlim_cur == RLIM_INFINITY;
}

bool setFdJoinMulticast(SOCKET sock, string bindIp, uint16_t bindPort, string ssmIp, uint16_t sockFamily) {
	if ( sockFamily == AF_INET ) {
		if (ssmIp == "") {
			struct ip_mreq group;
			group.imr_multiaddr.s_addr = inet_addr(STR(bindIp));
			group.imr_interface.s_addr = INADDR_ANY;
			if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					(char *) &group, sizeof (group)) < 0) {
				int err = errno;
				FATAL("Adding multicast failed. Error was: (%d) %s", err, strerror(err));
				return false;
			}
			return true;
		} else {
			struct group_source_req multicast;
			struct sockaddr_in *pGroup = (struct sockaddr_in*) &multicast.gsr_group;
			struct sockaddr_in *pSource = (struct sockaddr_in*) &multicast.gsr_source;

			memset(&multicast, 0, sizeof (multicast));

			//Setup the group we want to join
			pGroup->sin_family = AF_INET;
			pGroup->sin_addr.s_addr = inet_addr(STR(bindIp));
			pGroup->sin_port = EHTONS(bindPort);

			//setup the source we want to listen
			pSource->sin_family = AF_INET;
			pSource->sin_addr.s_addr = inet_addr(STR(ssmIp));
			if (pSource->sin_addr.s_addr == INADDR_NONE) {
				FATAL("Unable to SSM on address %s", STR(ssmIp));
				return false;
			}
			pSource->sin_port = 0;

			INFO("Try to SSM on ip %s", STR(ssmIp));

			if (setsockopt(sock, IPPROTO_IP, MCAST_JOIN_SOURCE_GROUP, &multicast,
					sizeof (multicast)) < 0) {
				int err = errno;
				FATAL("Adding multicast failed. Error was: (%d) %s", err,
						strerror(err));
				return false;
			}
		}
	} else {
		SocketAddress tempAddr;
		tempAddr.setIPAddress(bindIp, bindPort);		
		if (ssmIp == "") {
			struct ipv6_mreq group;

			memcpy(&group.ipv6mr_multiaddr, 
				   &(((sockaddr_in6*)tempAddr.getSockAddr())->sin6_addr), 
				   sizeof(struct in6_addr));	
			group.ipv6mr_interface = 0; // any				   		
			if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
					(char *) &group, sizeof (group)) < 0) {
				int err = errno;
				FATAL("Adding multicast failed. Error was: (%d) %s", err, strerror(err));
				return false;
			}
			return true;
		} else {
			struct group_source_req multicast;
			struct sockaddr_in *pGroup = (struct sockaddr_in*) &multicast.gsr_group;
			struct sockaddr_in *pSource = (struct sockaddr_in*) &multicast.gsr_source;
			SocketAddress ssmAddr;

			if (ssmAddr.setIPAddress(ssmIp,0)) {
				memset(&multicast, 0, sizeof (multicast));

				//Setup the group we want to join
				pGroup->sin_family = AF_INET6;
				memcpy(&pGroup->sin_addr,
					&(((sockaddr_in6*)tempAddr.getSockAddr())->sin6_addr),
					sizeof(struct in6_addr));
				pGroup->sin_port = tempAddr.getSockPort();

				//setup the source we want to listen
				pSource->sin_family = AF_INET6;
				memcpy(&pSource->sin_addr,
					&(((sockaddr_in6*)ssmAddr.getSockAddr())->sin6_addr),
					sizeof(struct in6_addr));
				pSource->sin_port = 0;

				INFO("Try to SSM on ip %s", STR(ssmIp));

				if (setsockopt(sock, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, &multicast,
						sizeof (multicast)) < 0) {
					int err = errno;
					FATAL("Adding multicast failed. Error was: (%d) %s", err,
							strerror(err));
					return false;
				}
			} else {
				FATAL("Unable to SSM on address %s", STR(ssmIp));
				return false;
			}
		}
	}
	return true;
}

bool setFdNonBlock(SOCKET fd) {
	int32_t arg;
	if ((arg = fcntl(fd, F_GETFL, NULL)) < 0) {
		int err = errno;
		FATAL("Unable to get fd flags: (%d) %s", err, strerror(err));
		return false;
	}
	arg |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, arg) < 0) {
		int err = errno;
		FATAL("Unable to set fd flags: (%d) %s", err, strerror(err));
		return false;
	}

	return true;
}

bool setFdNoSIGPIPE(SOCKET fd) {
	//This is not needed because we use MSG_NOSIGNAL when using
	//send/write functions
	return true;
}

bool setFdKeepAlive(SOCKET fd, bool isUdp) {
	if (isUdp)
		return true;
	int32_t one = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
			(const char*) & one, sizeof (one)) != 0) {
		FATAL("Unable to set SO_NOSIGPIPE");
		return false;
	}
	return true;
}

bool setFdNoNagle(SOCKET fd, bool isUdp) {
	if (isUdp)
		return true;
	int32_t one = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) & one, sizeof (one)) != 0) {
		return false;
	}
	return true;
}

bool setFdReuseAddress(SOCKET fd) {
	int32_t one = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) & one, sizeof (one)) != 0) {
		FATAL("Unable to reuse address");
		return false;
	}
#ifdef SO_REUSEPORT_OFF
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) & one, sizeof (one)) != 0) {
		FATAL("Unable to reuse port");
		return false;
	}
#endif /* SO_REUSEPORT */
	return true;
}

bool setFdTTL(SOCKET fd, uint8_t ttl) {
	int temp = ttl;
	if (setsockopt(fd, IPPROTO_IP, IP_TTL, &temp, sizeof (temp)) != 0) {
		int err = errno;
		WARN("Unable to set IP_TTL: %"PRIu8"; error was (%d) %s", ttl, err, strerror(err));
	}
	return true;
}

bool setFdMulticastTTL(SOCKET fd, uint8_t ttl, uint16_t sockFamily) {
	int nProto  = (sockFamily == AF_INET6) ? IPPROTO_IPV6 : IPPROTO_IP;
	int nOption = (sockFamily == AF_INET6) ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL;	
	int temp = ttl;
	if (setsockopt(fd, nProto, nOption, &temp, sizeof (temp)) != 0) {
		int err = errno;
		WARN("Unable to set IP_MULTICAST_TTL: %"PRIu8"; error was (%d) %s", ttl, err, strerror(err));
	}
	return true;
}

bool setFdTOS(SOCKET fd, uint8_t tos) {
	int temp = tos;
	if (setsockopt(fd, IPPROTO_IP, IP_TOS, &temp, sizeof (temp)) != 0) {
		int err = errno;
		WARN("Unable to set IP_TOS: %"PRIu8"; error was (%d) %s", tos, err, strerror(err));
	}
	return true;
}

int32_t __maxSndBufValUdp = 0;
int32_t __maxRcvBufValUdp = 0;
int32_t __maxSndBufValTcp = 0;
int32_t __maxRcvBufValTcp = 0;
SOCKET __maxSndBufSocket = -1;

bool DetermineMaxRcvSndBuff(int option, bool isUdp) {
	int32_t &maxVal = isUdp ?
			((option == SO_SNDBUF) ? __maxSndBufValUdp : __maxRcvBufValUdp)
			: ((option == SO_SNDBUF) ? __maxSndBufValTcp : __maxRcvBufValTcp);
	CLOSE_SOCKET(__maxSndBufSocket);
	__maxSndBufSocket = -1;
	__maxSndBufSocket = socket(AF_INET, isUdp ? SOCK_DGRAM : SOCK_STREAM, 0);
	if (__maxSndBufSocket < 0) {
		FATAL("Unable to create testing socket");
		return false;
	}

	int32_t known = 0;
	int32_t testing = 0x7fffffff;
	int32_t prevTesting = testing;
	//	FINEST("---- isUdp: %d; option: %s ----", isUdp, (option == SO_SNDBUF ? "SO_SNDBUF" : "SO_RCVBUF"));
	while (known != testing) {
		//		assert(known <= testing);
		//		assert(known <= prevTesting);
		//		assert(testing <= prevTesting);
		//		FINEST("%"PRId32" (%"PRId32") %"PRId32, known, testing, prevTesting);
		if (setsockopt(__maxSndBufSocket, SOL_SOCKET, option, (const char*) & testing,
				sizeof (testing)) == 0) {
			known = testing;
			testing = known + (prevTesting - known) / 2;
			//FINEST("---------");
		} else {
			prevTesting = testing;
			testing = known + (testing - known) / 2;
		}
	}
	CLOSE_SOCKET(__maxSndBufSocket);
	__maxSndBufSocket = -1;
	maxVal = known;
	//	FINEST("%s maxVal: %"PRId32, (option == SO_SNDBUF ? "SO_SNDBUF" : "SO_RCVBUF"), maxVal);
	return maxVal > 0;
}

bool setFdMaxSndRcvBuff(SOCKET fd, bool isUdp) {
	int32_t &maxSndBufVal = isUdp ? __maxSndBufValUdp : __maxSndBufValTcp;
	int32_t &maxRcvBufVal = isUdp ? __maxRcvBufValUdp : __maxRcvBufValTcp;
	if (maxSndBufVal == 0) {
		if (!DetermineMaxRcvSndBuff(SO_SNDBUF, isUdp)) {
			FATAL("Unable to determine maximum value for SO_SNDBUF");
			return false;
		}
	}

	if (maxRcvBufVal == 0) {
		if (!DetermineMaxRcvSndBuff(SO_RCVBUF, isUdp)) {
			FATAL("Unable to determine maximum value for SO_SNDBUF");
			return false;
		}
	}

	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*) & maxSndBufVal,
			sizeof (maxSndBufVal)) != 0) {
		FATAL("Unable to set SO_SNDBUF");
		return false;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*) & maxRcvBufVal,
			sizeof (maxSndBufVal)) != 0) {
		FATAL("Unable to set SO_RCVBUF");
		return false;
	}
	return true;
}

bool setFdIPv6Only(SOCKET fd, bool v6Only) {
	int32_t ipV6Only = 0;

	if (v6Only) {
		ipV6Only = 1;
	}

	if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *) & ipV6Only, sizeof (ipV6Only)) != 0) {
		return false;
	}

	return true;
}

bool setFdOptions(SOCKET fd, bool isUdp) {
	if (!isUdp) {
		if (!setFdNonBlock(fd)) {
			FATAL("Unable to set non block");
			return false;
		}
	}

	if (!setFdNoSIGPIPE(fd)) {
		FATAL("Unable to set no SIGPIPE");
		return false;
	}

	if (!setFdKeepAlive(fd, isUdp)) {
		FATAL("Unable to set keep alive");
		return false;
	}

	if (!setFdNoNagle(fd, isUdp)) {
		WARN("Unable to disable Nagle algorithm");
	}

	if (!setFdReuseAddress(fd)) {
		FATAL("Unable to enable reuse address");
		return false;
	}

	if (!setFdMaxSndRcvBuff(fd, isUdp)) {
		FATAL("Unable to set max SO_SNDBUF on UDP socket");
		return false;
	}

	return true;
}

bool deleteFile(string path) {
	if (remove(STR(path)) != 0) {
		FATAL("Unable to delete file `%s`", STR(path));
		return false;
	}
	return true;
}

bool deleteFolder(string path, bool force) {
	if (!force) {
		return deleteFile(path);
	} else {
		string command = format("rm -rf %s", STR(path));
		if (system(STR(command)) != 0) {
			FATAL("Unable to delete folder %s", STR(path));
			return false;
		}
		return true;
	}
}

bool createFolder(string path, bool recursive) {
	string command = format("mkdir %s %s",
			recursive ? "-p" : "",
			STR(path));
	if (system(STR(command)) != 0) {
		FATAL("Unable to create folder %s", STR(path));
		return false;
	}

	return true;
}

//get list of hosts addresses of stream socket
bool getAllHostByName(string name, vector<string> &result, bool ipv4_only) {
	// Setting this to false to ensure lookups are done by UDP
	// If sethostent(true) an existing TCP connection would be used and
	// kept open for DNS lookups.  This causes a major problem when the RMS
	// is started prior to the creation/initialization of the correct interface
	static bool once( true );
	if( once ) {
		sethostent( false );
		once=false;
	}
/** ORIG ***********************************************
	struct hostent *pHostEnt = gethostbyname(STR(name));
        endhostent();
	if (pHostEnt == NULL)
		return "";
	if (pHostEnt->h_length <= 0)
		return "";
	string result = format("%hhu.%hhu.%hhu.%hhu",
			(uint8_t) pHostEnt->h_addr_list[0][0],
			(uint8_t) pHostEnt->h_addr_list[0][1],
			(uint8_t) pHostEnt->h_addr_list[0][2],
			(uint8_t) pHostEnt->h_addr_list[0][3]);
	return result;
*******************************************************/

	//cache clearing call for comcast cameras
	res_init();

	string tAddr("");
	struct addrinfo hints, *servinfo, *addrptr;
	// struct sockaddr_in *h;
	int    rv;
	char   ipstr[INET6_ADDRSTRLEN];
	void *ipaddr = NULL;

	memset(&hints, 0, sizeof hints);
	// hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	if (ipv4_only)
		hints.ai_family = AF_INET;
	else
		hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6

	hints.ai_socktype = SOCK_STREAM; // include only stream sockets

	// if ( (rv = getaddrinfo( name.c_str() , NULL , NULL , &servinfo)) != 0)
	if ( (rv = getaddrinfo( name.c_str() , NULL , &hints , &servinfo)) != 0)	{
		FATAL("getaddrinfo error looking up: %s : %s", STR(name), gai_strerror(rv));
		return false;
	}

	// loop through all the results and connect to the first we can
	// for(addrptr = servinfo; addrptr != NULL; addrptr = addrptr->ai_next)
	// {
	// 	h = (struct sockaddr_in *) addrptr->ai_addr;
	// 	result = inet_ntoa( h->sin_addr );
	// 	//break if a valid ip address has been encountered already
	// 	if (result != "" && result != "0.0.0.0") {
	// 		break;
	// 	}
	// }

	// On a successful call to getaddrinfo(), servinfo should contain 1-n
	// entries.  Loop through these entries until we find the first one
	// that is not empty or "0.0.0.0"

	bool bSkip = false;

    // Hey... first push all ipv4 addresses to the list
    for(addrptr = servinfo; addrptr != NULL; addrptr = addrptr->ai_next) {

		switch (addrptr->ai_family) {

		/* IPv4 family */
		case AF_INET:
		{
			struct sockaddr_in *v4ip = (struct sockaddr_in *)addrptr->ai_addr;
			ipaddr = &(v4ip->sin_addr);
			bSkip = false;
			break;
		}

		/* IPv6 family */
		default:
		// skip IPv6 first to force IPv4 on the top
			bSkip = true;
			break;

		}

		if(false == bSkip) {
			/* IP addresses from binary to string */
	        inet_ntop(addrptr->ai_family, ipaddr, ipstr, sizeof (ipstr));
			tAddr = ipstr;
			if (tAddr != "" && tAddr != "0.0.0.0" && tAddr != "::") {
				ADD_VECTOR_END(result, ipstr);

			}
		}
    }

    // Hey... now its time to push all ipv6 addresses to the list
    for(addrptr = servinfo; addrptr != NULL; addrptr = addrptr->ai_next) {

	bSkip = false;
        switch (addrptr->ai_family) {

        /* IPv4 family */
        case AF_INET:
        // skip IPV4 as its already been added
        bSkip = true;
        break;

        /* IPv6 family */
        default:
        struct sockaddr_in6 *v6ip = (struct sockaddr_in6 *)addrptr->ai_addr;
        ipaddr = &(v6ip->sin6_addr);
        bSkip = false;
        break;
        }

        if(false == bSkip) {
			/* IP addresses from binary to string */
	        	inet_ntop(addrptr->ai_family, ipaddr, ipstr, sizeof (ipstr));
			tAddr = ipstr;
			if (tAddr != "" && tAddr != "0.0.0.0" && tAddr != "::") {
				ADD_VECTOR_END(result, ipstr);
			}
        }
    }

#if 0
    for(addrptr = servinfo; addrptr != NULL; addrptr = addrptr->ai_next) {
        void *addr;

        if (addrptr->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)addrptr->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addrptr->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        // convert the IP to a string and add to the vector
        inet_ntop(addrptr->ai_family, addr, ipstr, sizeof (ipstr));
		tAddr = ipstr;
		if (tAddr != "" && tAddr != "0.0.0.0" && tAddr != "::") {
			ADD_VECTOR_END(result, ip_str);
		}
    }
#endif

	freeaddrinfo(servinfo); // all done with this structure
	//FATAL("getHostByName( %s ) = %s", STR(name), STR(result));
	return true;
}

string getHostByName(string name) {
	// Setting this to false to ensure lookups are done by UDP
	// If sethostent(true) an existing TCP connection would be used and
	// kept open for DNS lookups.  This causes a major problem when the RMS
	// is started prior to the creation/initialization of the correct interface
	static bool once( true );
	if( once ) {
		sethostent( false );
		once=false;
	}
/** ORIG ***********************************************
	struct hostent *pHostEnt = gethostbyname(STR(name));
        endhostent();
	if (pHostEnt == NULL)
		return "";
	if (pHostEnt->h_length <= 0)
		return "";
	string result = format("%hhu.%hhu.%hhu.%hhu",
			(uint8_t) pHostEnt->h_addr_list[0][0],
			(uint8_t) pHostEnt->h_addr_list[0][1],
			(uint8_t) pHostEnt->h_addr_list[0][2],
			(uint8_t) pHostEnt->h_addr_list[0][3]);
	return result; 
*******************************************************/

	//cache clearing call for comcast cameras
	res_init();

	string result("");
	struct addrinfo hints, *servinfo, *addrptr;
	// struct sockaddr_in *h;
	int    rv;
	char   ipstr[INET6_ADDRSTRLEN];
	void *ipaddr = NULL;

	memset(&hints, 0, sizeof hints);
	// hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_socktype = 0; // include both stream and datagram sockets

	// if ( (rv = getaddrinfo( name.c_str() , NULL , NULL , &servinfo)) != 0)
	if ( (rv = getaddrinfo( name.c_str() , NULL , &hints , &servinfo)) != 0)	{
		FATAL("getaddrinfo error looking up: %s : %s", STR(name), gai_strerror(rv));
		return result;
	}

	// loop through all the results and connect to the first we can
	// for(addrptr = servinfo; addrptr != NULL; addrptr = addrptr->ai_next)
	// {
	// 	h = (struct sockaddr_in *) addrptr->ai_addr;
	// 	result = inet_ntoa( h->sin_addr );
	// 	//break if a valid ip address has been encountered already
	// 	if (result != "" && result != "0.0.0.0") {
	// 		break;
	// 	}
	// }

	// On a successful call to getaddrinfo(), servinfo should contain 1-n
	// entries.  Loop through these entries until we find the first one
	// that is not empty or "0.0.0.0"
    for(addrptr = servinfo; addrptr != NULL; addrptr = addrptr->ai_next) {

        switch (addrptr->ai_family) {

        /* IPv4 family */
        case AF_INET:
        {
            struct sockaddr_in *v4ip = (struct sockaddr_in *)addrptr->ai_addr;
            ipaddr = &(v4ip->sin_addr);
            break;
        }

        /* IPv6 family */
        case AF_INET6:
        {
            struct sockaddr_in6 *v6ip = (struct sockaddr_in6 *)addrptr->ai_addr;
            ipaddr = &(v6ip->sin6_addr);
            break;
        }

        }

        /* IP addresses from binary to string */
        inet_ntop(addrptr->ai_family, ipaddr, ipstr, sizeof (ipstr));
        result = ipstr;
        if (result != "" && result != "0.0.0.0" && result != "::") {
            break;
        }
    }

	freeaddrinfo(servinfo); // all done with this structure
	//FATAL("getHostByName( %s ) = %s", STR(name), STR(result));
	return result;
}

bool isNumeric(string value) {
	return value == format("%d", atoi(STR(value)));
}

void split(string str, string separator, vector<string> &result) {
	result.clear();
	string::size_type position = str.find(separator);
	string::size_type lastPosition = 0;
	uint32_t separatorLength = separator.length();

	while (position != str.npos) {
		ADD_VECTOR_END(result, str.substr(lastPosition, position - lastPosition));
		lastPosition = position + separatorLength;
		position = str.find(separator, lastPosition);
	}
	ADD_VECTOR_END(result, str.substr(lastPosition, string::npos));
}

uint64_t getTagMask(uint64_t tag) {
	uint64_t result = 0xffffffffffffffffLL;
	for (int8_t i = 56; i >= 0; i -= 8) {
		if (((tag >> i)&0xff) == 0)
			break;
		result = result >> 8;
	}
	return ~result;
}

string generateRandomString(uint32_t length) {
	string result = "";
	for (uint32_t i = 0; i < length; i++)
		result += alowedCharacters[rand() % alowedCharacters.length()];
	return result;
}

void lTrim(string &value) {
	string::size_type i = 0;
	for (i = 0; i < value.length(); i++) {
		if (value[i] != ' ' &&
				value[i] != '\t' &&
				value[i] != '\n' &&
				value[i] != '\r')
			break;
	}
	value = value.substr(i);
}

void rTrim(string &value) {
	int32_t i = 0;
	for (i = (int32_t) value.length() - 1; i >= 0; i--) {
		if (value[i] != ' ' &&
				value[i] != '\t' &&
				value[i] != '\n' &&
				value[i] != '\r')
			break;
	}
	value = value.substr(0, i + 1);
}

void trim(string &value) {
	lTrim(value);
	rTrim(value);
}

int8_t getCPUCount() {
	return sysconf(_SC_NPROCESSORS_ONLN);
}

map<string, string> mapping(string str, string separator1, string separator2, bool trimStrings) {
	map<string, string> result;

	vector<string> pairs;
	split(str, separator1, pairs);

	FOR_VECTOR_ITERATOR(string, pairs, i) {
		if (VECTOR_VAL(i) != "") {
			if (VECTOR_VAL(i).find(separator2) != string::npos) {
				string key = VECTOR_VAL(i).substr(0, VECTOR_VAL(i).find(separator2));
				string value = VECTOR_VAL(i).substr(VECTOR_VAL(i).find(separator2) + 1);
				if (trimStrings) {
					trim(key);
					trim(value);
				}
				result[key] = value;
			} else {
				if (trimStrings) {
					trim(VECTOR_VAL(i));
				}
				result[VECTOR_VAL(i)] = "";
			}
		}
	}
	return result;
}

void splitFileName(string fileName, string &name, string & extension, char separator) {
	size_t dotPosition = fileName.find_last_of(separator);
	if (dotPosition == string::npos) {
		name = fileName;
		extension = "";
		return;
	}
	name = fileName.substr(0, dotPosition);
	extension = fileName.substr(dotPosition + 1);
}

double getFileModificationDate(string path) {
	struct stat s;
	if (stat(STR(path), &s) != 0) {
		FATAL("Unable to stat file %s", STR(path));
		return 0;
	}
	return (double) s.st_mtime;
}

string normalizePath(string base, string file) {
	char dummy1[PATH_MAX];
	char dummy2[PATH_MAX];
	char *pBase = realpath(STR(base), dummy1);
	char *pFile = realpath(STR(base + file), dummy2);

	if (pBase != NULL) {
		base = pBase;
	} else {
		base = "";
	}

	if (pFile != NULL) {
		file = pFile;
	} else {
		file = "";
	}

	if (file == "" || base == "") {
		return "";
	}

	if (file.find(base) != 0) {
		return "";
	} else {
		if (!fileExists(file)) {
			return "";
		} else {
			return file;
		}
	}
}

bool listFolder(string path, vector<string> &result, bool normalizeAllPaths,
		bool includeFolders, bool recursive) {
	if (path == "")
		path = ".";
	if (path[path.size() - 1] != PATH_SEPARATOR)
		path += PATH_SEPARATOR;

	DIR *pDir = NULL;
	pDir = opendir(STR(path));
	if (pDir == NULL) {
		int err = errno;
		FATAL("Unable to open folder: %s (%d) %s", STR(path), err, strerror(err));
		return false;
	}

	struct dirent *pDirent = NULL;
	while ((pDirent = readdir(pDir)) != NULL) {
		string entry = pDirent->d_name;
		if ((entry == ".")
				|| (entry == "..")) {
			continue;
		}
		if (normalizeAllPaths) {
			entry = normalizePath(path, entry);
		} else {
			entry = path + entry;
		}
		if (entry == "")
			continue;

		if (pDirent->d_type == DT_UNKNOWN) {
			struct stat temp;
			if (stat(STR(entry), &temp) != 0) {
				WARN("Unable to stat entry %s", STR(entry));
				continue;
			}
			pDirent->d_type = ((temp.st_mode & S_IFDIR) == S_IFDIR) ? DT_DIR : DT_REG;
		}

		switch (pDirent->d_type) {
			case DT_DIR:
			{
				if (includeFolders) {
					ADD_VECTOR_END(result, entry);
				}
				if (recursive) {
					if (!listFolder(entry, result, normalizeAllPaths, includeFolders, recursive)) {
						FATAL("Unable to list folder");
						closedir(pDir);
						return false;
					}
				}
				break;
			}
			case DT_REG:
			{
				ADD_VECTOR_END(result, entry);
				break;
			}
			default:
			{
				WARN("Invalid dir entry detected");
				break;
			}
		}
	}

	closedir(pDir);
	return true;
}

bool moveFile(string src, string dst) {
	if (rename(STR(src), STR(dst)) != 0) {
		FATAL("Unable to move file from `%s` to `%s`",
				STR(src), STR(dst));
		return false;
	}
	return true;
}

bool copyFile(string src, string dst, bool overwrite) {
	int32_t src_fd = open(STR(src), O_RDONLY);
	struct stat stat_buf;
	fstat(src_fd, &stat_buf);
	int32_t dst_fd = open(STR(dst), O_WRONLY | O_CREAT, stat_buf.st_mode);
	if ((overwrite || !fileExists(dst)) &&
			(sendfile(dst_fd, src_fd, NULL, stat_buf.st_size) < 0)) {
		FATAL("Unable to copy file `%s` to `%s`", STR(src), STR(dst)); 
		return false;
	}
	return true;
}

bool isAbsolutePath(string &path) {
	return (bool)((path.size() > 0) && (path[0] == PATH_SEPARATOR));
}

void signalHandler(int sig) {
	if (!MAP_HAS1(_signalHandlers, sig))
		return;
	_signalHandlers[sig]();
}

void installSignal(int sig, SignalFnc pSignalFnc) {
	_signalHandlers[sig] = pSignalFnc;
	struct sigaction action;
	action.sa_handler = signalHandler;
	action.sa_flags = 0;
	if (sigemptyset(&action.sa_mask) != 0) {
		ASSERT("Unable to install the quit signal");
		return;
	}
	if (sigaction(sig, &action, NULL) != 0) {
		ASSERT("Unable to install the quit signal");
		return;
	}
}

void installQuitSignal(SignalFnc pQuitSignalFnc) {
	installSignal(SIGTERM, pQuitSignalFnc);
}

void installConfRereadSignal(SignalFnc pConfRereadSignalFnc) {
	installSignal(SIGHUP, pConfRereadSignalFnc);
}

static time_t _gUTCOffset = -1;

void computeUTCOffset() {
	time_t now = time(NULL);
	struct tm *pTemp = localtime(&now);
	_gUTCOffset = pTemp->tm_gmtoff;
}

time_t getlocaltime() {
	if (_gUTCOffset == -1)
		computeUTCOffset();
	return getutctime() + _gUTCOffset;
}

time_t gettimeoffset() {
	if (_gUTCOffset == -1)
		computeUTCOffset();
	return _gUTCOffset;
}

#endif /* LINUX */
