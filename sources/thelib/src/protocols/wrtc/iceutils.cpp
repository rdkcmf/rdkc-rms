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
/*
 * IceUtils.h - useful stuff for NATting via STUN, etc.
 *
 */
 
#include "protocols/wrtc/iceutils.h"


bool IceUtils::IpStrIsIP4(std::string ip) {
	vector<string> fields;
	split(ip, ".", fields);
	if (fields.size() != 4) return false;
	for (int i = 0; i < 4; i++) {
		if (atoi(STR(fields[i])) > 255) return false;
	}
	return true;
}

bool IceUtils::IpPortStrIsV6(string ip) {
	if (ip.find("]:") != string::npos)
		return true;
	else
		return false;
}

//bool IceUtils::SplitIpStr(string ipStr, string & ip, uint16_t & port) {
//	vector<string> fields;
//	split(ipStr, ":", fields);
//	if (fields.size() != 2) return false;	//====>
//	ip = fields[0];
//	port = (uint16_t) atoi(STR(fields[1]));
//	return true;
//}

bool IceUtils::SplitIpStr(string ipPortStr, string & ip, uint16_t & port) {
	vector<string> fields;
	if (ipPortStr.find("]:") != string::npos) { // we have an IPv6 format - [ip]:port
		split(ipPortStr, "]:", fields);
		if (fields.size() != 2) return false;	//====>
		string tempIp = fields[0];
		size_t pos = tempIp.find("[");
		if ((pos != string::npos) && (tempIp.length()>1)){
			ip = tempIp.substr(pos+1);
			port = (uint16_t)atoi(STR(fields[1]));
			return true;
		}
	} else { // we have an IPv4 format - ip:port
		split(ipPortStr, ":", fields);
		if (fields.size() != 2) return false;	//====>
		ip = fields[0];
		port = (uint16_t)atoi(STR(fields[1]));
		return true;
	}
	return false;
}

bool IceUtils::SplitIpStrV4(string ipStr, uint32_t & ip, uint16_t & port) {
	vector<string> fields; // holds ipAddr and port as strings
	split(ipStr, ":", fields);
	if (fields.size() != 2) {
		WARN("Wrong number of fields: %" PRIu32", %s", (uint32_t)fields.size(), STR(ipStr));
		return false;	//====>
	}
	//INFO("$b2$ Split fields: %s : %s", STR(fields[0]), STR(fields[1]));
	vector<string> ipf; // holds string for each ipAddr sub-field
	split(fields[0], ".", ipf);
	if (ipf.size() != 4) {
		WARN("Wrong number of IP pieces: %" PRIu32, (uint32_t)ipf.size());
		return false; //==============>
	}
	ip = (atoi(STR(ipf[0])) << 24) | (atoi(STR(ipf[1])) << 16) | (atoi(STR(ipf[2])) << 8) | (atoi(STR(ipf[3]))) ;
	port = (uint16_t) atoi(STR(fields[1]));
	//INFO("$b2$ returning: 0x%X : %"PRIu16, ip, port);
	return true;
}

// Split the string [IPv6]:port into IPv6 and Port.  Then convert IPv6 into network byte orer
// uint8_t, and Port into uint16_t.
// NOTE -- no need to convert IPv6 into hostbyte because it's handled as an array, unlike a v4
//         address which is handled as a uint32
bool IceUtils::SplitIpStrV6(string ipPortStr, uint8_t* ipv6, uint16_t & port) {
	string tempIP;
	if (SplitIpStr(ipPortStr, tempIP, port)) {
		SocketAddress tempAddress;
		tempAddress.setIPAddress(tempIP);
		sockaddr_in6* pAddr = (sockaddr_in6*)tempAddress.getSockAddr();
		memcpy(ipv6, &(pAddr->sin6_addr), sizeof(in6_addr)); //already in network order...
		return true;
	}
	return false;
}

//bool IceUtils::ToIpStr(string & ip, uint16_t port, string & ipStr) {
//	ipStr = format("%s:%" PRIu16, STR(ip), port);
//	return true;
//}

string IceUtils::ToIpPortStr(SocketAddress theAddress) {
	string result;
	if (theAddress.getFamily() == AF_INET) {
		return format("%s:%" PRIu16, STR(theAddress.toString()), theAddress.getSockPort());
	} else { // AF_INET6
		return format("[%s]:%" PRIu16, STR(theAddress.toString()), theAddress.getSockPort());
	}
	return result;
}

bool IceUtils::ToIpStrV4(uint32_t ip, uint16_t port, string & ipStr) {
	ipStr = format("%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 ":%" PRIu16,
		0xFF & (ip >> 24), 0xFF & (ip >> 16), 0xFF & (ip >> 8), 0xFF & (ip), port);
	//INFO("$b2$ converted: [0x%X : %"PRIu16"] to %s", ip, port, STR(ipStr));
	return true;
}

bool IceUtils::ToIpStrV6(uint8_t* ipv6, uint16_t port, string & ipStr) {
	/*
	SocketAddress tempAddress;
	sockaddr_in6* pAddress = (sockaddr_in6*)tempAddress.getSockAddr();
	memcpy(&(pAddress->sin6_addr), ipv6, sizeof(in6_addr));
	ipStr = format("[%s]:%" PRIu16, STR(tempAddress.toString()), port);
	*/

	string tmp = format("%" PRIx8 "%02" PRIx8 ":%" PRIx8 "%02" PRIx8 ":%" PRIx8 "%02" PRIx8 ":%"
			PRIx8 "%02" PRIx8 ":%" PRIx8 "%02" PRIx8 ":%" PRIx8 "%02" PRIx8 ":%" PRIx8 "%02" PRIx8 ":%"
			PRIx8 "%02" PRIx8, ipv6[0], ipv6[1], ipv6[2], ipv6[3], ipv6[4], ipv6[5], 
			ipv6[6], ipv6[7], ipv6[8], ipv6[9], ipv6[10], ipv6[11], ipv6[12], ipv6[13], 
			ipv6[14], ipv6[15]);
	ipStr = format("[%s]:%" PRIu16, STR(tmp), port);
	return true;
}

bool IceUtils::IpStrToInt(string ipStr, uint32_t & ipInt) {
	vector<string> ipf; // holds string for each ipAddr sub-field
	split(ipStr, ".", ipf);
	
	if (ipf.size() != 4) {
		WARN("Wrong number of IP pieces: %" PRIu32, (uint32_t)ipf.size());
		return false;
	}
	
	ipInt = (atoi(STR(ipf[0])) << 24) | (atoi(STR(ipf[1])) << 16) | (atoi(STR(ipf[2])) << 8) | (atoi(STR(ipf[3]))) ;
	return true;
}

bool IceUtils::IpIntToStr(uint32_t ipInt, string & ipStr) {
	ipStr = format("%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32,
		0xFF & (ipInt >> 24), 0xFF & (ipInt >> 16), 0xFF & (ipInt >> 8), 0xFF & (ipInt));

	return true;
}
