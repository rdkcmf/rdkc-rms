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

#ifndef __ICEUTILS_H
#define __ICEUTILS_H

#include "common.h"

class DLLEXP IceUtils {
public:
	// Check if the string is of IPv4:port format.
	static bool IpStrIsIP4(string ip);

	// Check if the string is of [ipv6]:port format
	static bool IpPortStrIsV6(string ip);

	//static bool SplitIpStr(string ipStr, string & ip, uint16_t & port); 

	// Take 1 string wich is in IP:Port frmat, then split it to an IP string
	// and a uint16 port.
	static bool SplitIpStr(string ipPortStr, string & ip, uint16_t & port);

	// Take 1 string which is in IP:Port format, split it up to a uint16 port
	// and uint32 IP
	static bool SplitIpStrV4(string ipStr, uint32_t & ip, uint16_t & port);

	// Take 1 string which is in IP:Port format, split it up to a uint16 port
	// and uint8_t[16] ipv6 array
	static bool SplitIpStrV6(string ipPortStr, uint8_t* ipv6, uint16_t & port);

	//static bool ToIpStr(string & ip, uint16_t port, string & ipStr);
	static bool ToIpStrV4(uint32_t ip, uint16_t port, string & ipStr);
	static bool ToIpStrV6(uint8_t *ipv6, uint16_t port, string & ipStr);

	static string ToIpPortStr(SocketAddress theAddress);
	static bool IpStrToInt(string ipStr, uint32_t & ipInt);
	static bool IpIntToStr(uint32_t ipInt, string & ipStr);
};

#endif // __ICEUTILS_H

