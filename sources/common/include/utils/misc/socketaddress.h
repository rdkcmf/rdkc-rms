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



#ifndef _SOCKETADDRESS_H
#define _SOCKETADDRESS_H

#if defined ( RMS_PLATFORM_RPI )
#include "platform/platform.h"
#include <sys/socket.h>
#include <netinet/in.h>
#endif

typedef union {
  struct sockaddr sa;
  struct sockaddr_in s4;
  struct sockaddr_in6 s6;
  struct sockaddr_storage ss;
} AddressType;

class SocketAddress {
private:
  AddressType _address;
  static bool _ipv6Supported;
  static bool _ipv6Only;
  bool _forBinding;

  void _reset();

public:
  SocketAddress ();

  static bool isV6(string ip);
  bool isMulticast();

  bool setIPAddress ( string ip);
  bool setIPAddress ( string ip, uint16_t port);
  bool setIPPortAddress(string ipPortAddress);
  string toString ();

  uint16_t  getFamily();
  sockaddr* getSockAddr();
  uint16_t  getSockPort();
  void      setSockPort(uint16_t port);
  uint16_t  getLength();
  
  string getIPv4();
  string getIPv6();
  string getIPwithPort();
  static string getIPv4(string currentIP);
  static string getIPv6(string currentIP, bool dualStack);
  static string getIPwithPort(string currentIP, uint16_t port);

  void setIPv6Support(bool enabled, bool dualStack, bool forBinding = false);
  void getIPv6Support(bool& isEnabled, bool& isDualStack);

  void operator = (const SocketAddress& _socketAddress);
};


#endif //_SOCKETADDRESS_H
