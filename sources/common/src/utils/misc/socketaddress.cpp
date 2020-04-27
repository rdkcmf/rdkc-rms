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


#include "utils/misc/socketaddress.h"

#if defined ( RMS_PLATFORM_RPI )
#include "utils/misc/format.h"
#endif

bool SocketAddress::_ipv6Supported = false;
bool SocketAddress::_ipv6Only = false;

void SocketAddress::_reset () {
  memset ( &_address, 0x00, sizeof(AddressType));
}


SocketAddress::SocketAddress() {
  _reset();
  _forBinding = false;
}

bool SocketAddress::isV6(string ip) {
	size_t limit = ip.find_last_of(":");

	// Check if this is "[xxx:yyy:zzz]:port"
	if (limit == string::npos) {
		return false; // definitely not a V6
	}

	// Now search for another instance of ":", excluding the last as found above
	return (ip.find_last_of(":", (limit - 1)) != string::npos);
}


bool SocketAddress::isMulticast() {
  if ( getFamily() == AF_INET6 ) {
	// if (_address.s6.sin6_addr.u.Byte[0] == 0xff) {
	if (_address.s6.sin6_addr.s6_addr[0] == 0xff) {
		return true;
	}
  } else {
    uint32_t testVal = EHTONL(_address.s4.sin_addr.s_addr);
    if ((testVal > 0xe0000000) && (testVal < 0xefffffff)) {
      return true;
    }
  }
  return false;
}

bool SocketAddress::setIPPortAddress(string ipPortAddress) {
	vector<string> fields;
	string ip;
	uint16_t port = 0;
	if (ipPortAddress.find("]:") != string::npos) { // we have an IPv6 format - [ip]:port
		split(ipPortAddress, "]:", fields);
		if (fields.size() != 2) return false;	//====>
		string tempIp = fields[0];
		size_t pos = tempIp.find("[");
		if ((pos != string::npos) && (tempIp.length()>1)) {
			ip = tempIp.substr(pos + 1);
			port = (uint16_t)atoi(STR(fields[1]));
		}
	} else { // we have an IPv4 format - ip:port
		split(ipPortAddress, ":", fields);
		if (fields.size() != 2) return false;	//====>
		ip = fields[0];
		port = (uint16_t)atoi(STR(fields[1]));
	}
	return setIPAddress(ip, port);
}

// Create a new IPv4 or IPv6 address structure from ip.

bool SocketAddress::setIPAddress(string ip) {
	// Dual stack support: create the correct IPv6 address format if this is an
	// IPv4 address and IPv6 is indeed supported
	if (!isV6(ip) && _ipv6Supported) {
		// TODO: for now, dual stack is only for binding
		if (_ipv6Only || !_forBinding) {
			// Create an IPv4-mapped address
			ip = "::ffff:" + ip;
		} else {
			// Create an IPv4-compatible address, since we support dual stack
			ip = "::" + ip;
		}
	}

	if (isV6(ip)) {
		_address.s6.sin6_family = AF_INET6;
		if (inet_pton(AF_INET6, STR(ip), &_address.s6.sin6_addr) <= 0) {
			return false;
		}
	} else {
		_address.s4.sin_family = AF_INET;
		_address.s4.sin_addr.s_addr = inet_addr(STR(ip));
		if (_address.s4.sin_addr.s_addr == INADDR_NONE) {
			return false;
		}
	}
	return true;
}

// Create a new IPv4 or IPv6 address structure from ip:port.
bool SocketAddress::setIPAddress( string ip, uint16_t port ) {
  if ( setIPAddress(ip) ) {
    setSockPort(port);
    return true;
  } 
  return false;
}

string SocketAddress::toString(){
  // Get address type and set up the parameters needed for inet_ntop
  uint16_t family = _address.sa.sa_family;
  void* pSrc = NULL;
  if ( family == AF_INET ) {
    pSrc = &_address.s4.sin_addr;
  } else {
    pSrc = &_address.s6.sin6_addr;
  }

  // call inet_ntop to generate the ip string
  char szTemp[100];
	//inet_ntop(AF_INET6, &(sa.sin6_addr), str, INET6_ADDRSTRLEN);

  if(inet_ntop(family, pSrc, szTemp, sizeof(szTemp))) {
    return string(szTemp);
  }

  return "";
}

uint16_t SocketAddress::getFamily() {
  return _address.sa.sa_family;
}

sockaddr* SocketAddress::getSockAddr() {
  return &_address.sa;
}

uint16_t SocketAddress::getSockPort() {
  uint16_t result = 0;
  if ( getFamily() == AF_INET) {
    result = _address.s4.sin_port;
  } else {
    result = _address.s6.sin6_port;
  }
  return ENTOHS(result); // NOTE -- make sure NOT to call ENTOHS in again for this!
}

void SocketAddress::setSockPort(uint16_t port ) {
  if ( getFamily() == AF_INET){
    _address.s4.sin_port = EHTONS(port);
  } else {
    _address.s6.sin6_port = EHTONS(port);
  }
}

uint16_t SocketAddress::getLength() {
  if ( _address.sa.sa_family == AF_INET) {
    return sizeof(sockaddr_in);
  } else {
    return sizeof(sockaddr_in6);
  }
}

void SocketAddress::setIPv6Support(bool enabled, bool dualStack, bool forBinding) {
	_ipv6Supported = enabled;
	_ipv6Only = !dualStack;
	_forBinding = forBinding;
}

void SocketAddress::getIPv6Support(bool& isEnabled, bool& isDualStack) {
	isEnabled = _ipv6Supported;
	isDualStack = !_ipv6Only;
}

void SocketAddress::operator= (const SocketAddress& _socketAddress) {
	memcpy(&_address, &(_socketAddress._address), sizeof(AddressType));
}

string SocketAddress::getIPv4() {
	string ip = toString();
	
	return getIPv4(ip);
}

string SocketAddress::getIPv6() {
	string ip = toString();
	return getIPv6(ip, !_ipv6Only);
}

string SocketAddress::getIPwithPort() {
	string ip = getIPv4();

	return getIPwithPort(ip, getSockPort());
}

string SocketAddress::getIPwithPort(string currentIP, uint16_t port) {
	if (isV6(currentIP)) {
		return format("[%s]:%" PRIu16, STR(currentIP), port);
	}
	
	return format("%s:%" PRIu16, STR(currentIP), port);
}

string SocketAddress::getIPv4(string currentIP) {
	// Check if we have an IPv6 address with an embedded IPv4
	if ((currentIP.find(":") != string::npos) && (currentIP.find(".") != string::npos)) {
		vector<string> fields;

		split(currentIP, ":", fields);
		int fieldSize = fields.size();

		// Sanity check, there should at least be two
		if (fieldSize > 1) {
			return fields[fieldSize - 1];
		}
	}

	// Otherwise, return as is
	return currentIP;
}

string SocketAddress::getIPv6(string currentIP, bool dualStack) {
	// Check if we have an IPv4 address
	if ((currentIP.find(":") == string::npos) && (currentIP.find(".") != string::npos)) {
		// Check then, if IPv6 only
		if (_ipv6Only) {
			// Create an IPv4-mapped address
			return "::ffff:" + currentIP;
		}
		
		// Otherwise, create an IPv4-compatible address, since we support dual stack
		return "::" + currentIP;
	}

	// Otherwise, return as is
	return currentIP;
}

