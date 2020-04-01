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
 * candidate.h - a remote or local ICE candidate instance
 *
 */

#include "protocols/wrtc/candidate.h"
#include "protocols/wrtc/iceutils.h"

// typePri will set the top byte of the priority for outgoing candidates
const int NUM_CAN_TYPES = 6;
//TODO
//static const uint32_t typePri[NUM_CAN_TYPES] = {0, 126, 100, 80,     0,     0 }; // <<24 to get priority MSB
static const uint32_t typePri[NUM_CAN_TYPES] = {1, 2122260223, 1686052607, 2, 41885439, 3 };
static const string cantypes[NUM_CAN_TYPES] = {"???", "host", "srflx", "prflx", "relay", "token"};

Candidate::Candidate() {
	// make sure the basic control vars are inited
	_state = 0;
	_isServer = false;
	_isRemote = true;
	_peerUse = false;
	_retryCount = 0;
	_priority = 0;
	_port = 0;
	_componentId = 0;

	memset(_lastTurnTxId, 0, sizeof(_lastTurnTxId));
}

Candidate::~Candidate() {
}

bool Candidate::IsSame(Candidate * pCan) {
	return (pCan->_ipAddress == _ipAddress)
		&& (pCan->_port == _port)
		&& (pCan->_type == _type);
}

//
// static create routines with convenient defaults
//
Candidate * Candidate::CreateRemote(string & ipStr) {
	Candidate * pCan = new Candidate();
	pCan->Init("srflx", ipStr, true, false);
	return pCan;
}

Candidate * Candidate::CreateReflex(string & ipStr, string & hostIpStr) {
	return Candidate::CreateRaddr("srflx", ipStr, hostIpStr, false);
}

Candidate * Candidate::CreateRelay(string & ipStr, string & hostIpStr) {
	return Candidate::CreateRaddr("relay", ipStr, hostIpStr, true);
}

Candidate * Candidate::CreateRaddr(string type, string & ipStr, string & hostIpStr, bool server) {
	Candidate * pCan = new Candidate();
	pCan->Init(type, ipStr, false, server);
	string ip;
	uint16_t port;
	if (SplitIpStr(hostIpStr, ip, port)) {
		ip = SocketAddress::getIPv4(ip);
		pCan->_rem = format("raddr %s rport %" PRIu16" %s",
				STR(ip), port, STR(pCan->_rem));
	}
	return pCan;
}

Candidate * Candidate::CreateServer(string & ipStr) {
	Candidate * pCan = new Candidate();
	pCan->Init("token", ipStr, true, true);
	return pCan;
}

Candidate * Candidate::CreateHost(string & ipStr) {
	Candidate * pCan = new Candidate();
	pCan->Init("host", ipStr, false, false);
	return pCan;
}

Candidate * Candidate::Clone() {
	Candidate * pC = new Candidate(*this);
	//memcpy((void*) pC, (void*) this, sizeof(Candidate));
	return pC;
}

void Candidate::Init(string type, string ipStr, bool isRemote, bool isServer) {
	static int counter = 1234;
	_state = 0;
	_type = type;
	_isRemote = isRemote;
	_isServer = isServer;

	// Updated for dual stack support
	_theAddress.setIPPortAddress(ipStr);
	_ipAddress = _theAddress.getIPv4();
	_port = _theAddress.getSockPort();
	_ipStr = _theAddress.getIPwithPort();

	int i = typeStrIndex(type);
	// There are some cases that multiple unknown candidates are detected
	// we use the latest unknown candidate that arrives instead.
	// We do this by using the counter to make the priority higher
	_priority = typePri[i] + counter; 

	_foundation = format("%d", counter++);
	_componentId = 1;
	_transport = "udp";
	_tcpType = "";
	_rem = "generation 0";	// $ToDo - what is this???
	//_rem = "";	// $ToDo - what is this???
}

bool Candidate::IsIpStr(string ipStr) {
	// return true if matches
	return ipStr == _ipStr;
}

// return true if both ip matches byte wise
bool Candidate::IsSameIp(string ipStr) {
	uint16_t port;
	uint16_t _port;

	// both IPV6
	if(IpPortStrIsV6(ipStr) && IpPortStrIsV6(_ipStr)) {
		uint8_t ipv6[16] = {0};
		uint8_t _ipv6[16] = {0};
                SplitIpStrV6(ipStr, ipv6, port); // ipv6 <- in6_addr
                SplitIpStrV6(_ipStr, _ipv6, _port); // _ipv6 <- in6_addr

		// check port
		if(port != _port) {
			return false;
		}

		// check ipv6 bytewise
		for(uint32_t byteCount = 0; byteCount < 16; byteCount++) {
			if(ipv6[byteCount] != _ipv6[byteCount]) {
				return false;
			}
		}

		return true;
	}
	// both IPV4
	else if(IpStrIsIP4(ipStr) && IpStrIsIP4(_ipStr)) {
		uint32_t ipv4;    // 4 bytes for ipv4
		uint32_t _ipv4;   // 4 bytes for ipv4
                SplitIpStrV4(ipStr, ipv4, port);
                SplitIpStrV4(_ipStr, _ipv4, _port);

		// check port
                if(port != _port) {
                        return false;
                }

		// check ipv4
                if(ipv4 != _ipv4) {
                        return false;
                }

		return true;

	}
	// version mismatch
	else {
		return false;
	}

}

bool Candidate::IsIpPort(string ip, uint16_t port) {
	// return true if matches
	return (ip == _ipAddress) && (port == _port);
}

bool Candidate::Set(string & str, bool remote) {
	vector <string> fields;
	split(str, " ", fields);
	
	uint32_t fieldSize = (uint32_t) fields.size();
	if (fieldSize < 8) {
		WARN("Invalid candidate string: %s", STR(str));
		return false;
	}
	_foundation = fields[0];
	_componentId = (uint32_t) atoi(STR(fields[1]));
	_transport = lowerCase(fields[2]);
	_priority = (uint32_t) atoi(STR(fields[3]));
	_ipAddress = fields[4];
	_port = (uint16_t) atoi(STR(fields[5]));
	// field[6] = "typ"
	_type = lowerCase(fields[7]);
	
	if (fieldSize > 8) {
		uint8_t index = 8;
		_rem = lowerCase(fields[index]);

		// if fields[8] == tcptype
		if (_rem == "tcptype") {
			_tcpType = lowerCase(fields[index + 1]);
			index += 2;
		}

		// Get and form back the remaining fields
		for (uint32_t i = 0; fieldSize > (index + i); i++) {
			_rem = fields[index + i];
			if (fieldSize > (index + i + 1)) {
				_rem += " "; // return the space delimiter
			}
		}
 	}

	_isRemote = remote;

	if (!_theAddress.setIPAddress(_ipAddress, _port)) {
		FATAL("Cannot set IP(%s)/PORT(%d)",STR(_ipAddress), _port);
	}

	_ipStr = _theAddress.getIPwithPort();
//	ToIpStr(_ipAddress, _port, _ipStr);

	//WARN("$b2$ Candidate Created: ip:%s, priString:%s, pri#:%d",
	//		STR(_ipStr), STR(fields[3]), _priority);
	//
	return true;
}

/**
 * For RTCP mux, component ID is incremented for a candidate
 */
void Candidate::IncrementComponentId() {
	_componentId++;
}

bool Candidate::Get(string & str) { 
	if (_tcpType.empty()) {
         str = format("%s %u %s %u %s %u typ %s %s", STR(_foundation), _componentId, 
                 STR(_transport), _priority, STR(_ipAddress), _port, STR(_type), 
                 STR(_rem));
	} else {
		str = format("%s %u %s %u %s %u typ %s tcptype %s %s", STR(_foundation), _componentId, 
                STR(_transport), _priority, STR(_ipAddress), _port, STR(_type), STR(_tcpType), 
                STR(_rem));
	}

	return true;
}

void Candidate::SetLastTurnTxId(uint8_t * pTxId) {
	for(int i = 0; i < 12; i++) {
		_lastTurnTxId[i] = pTxId[i];
	}
}

uint8_t* Candidate::GetLastTurnTxId() {
	for(int i = 0; i < 12; i++) {
		if (_lastTurnTxId[i] != 0) {
			return _lastTurnTxId;
		}
	}
	
	// Initial value, return null
	return NULL;
}

bool Candidate::IsLastTurnTxId(uint8_t * pTxId) {
	for(int i = 0; i < 12; i++) {
		if (_lastTurnTxId[i] != pTxId[i]) {
			return false; //==================>
		}
	}
	return true;
}


int Candidate::typeStrIndex(string str) {
	int i;
	int typ = 0;
	for (i = 1; i < NUM_CAN_TYPES; i++) {
		if (str == cantypes[i]) {
			typ = i;
			break;
		}
	}
	return typ;
}

uint32_t Candidate::GetPriority() {
	// return priority adjusted by "goodness"
	uint32_t pri = _priority;	// start out with supplied priority
	if (!IsStatePierceGood()) {
		if (IsStateTurnGood()) {
			// at least we can relay via TURN
			pri &= 0x0000FFFF;	// clear top 2 bytes
		}
		else { // got nothing, we are dead
			pri = 0;
		}
	}
	// should I adjust if Peer has set USE attribute??
	// this will force us to select this candidate
	if (_peerUse) {
		pri |= 0x80000000;	// set top bit?!
	}

	return pri;
}
bool Candidate::IsBetterThan(Candidate * pCan) {
	if (!pCan) return true;	// handle null or "first" case
	return GetPriority() > pCan->GetPriority();
}

string & Candidate::GetShortString() {
	// return short informmation string
	_shortString = format("type: %s, ip: %s, state: 0x%x", STR(_type), STR(_ipStr), _state);
	return _shortString;
}

string & Candidate::GetLongString() {
	// return short informmation string
	_longString = "Can: ip:" + _ipStr;
	_longString += format(" Raw Pri:%d, Typ:", _priority);
	_longString += STR(_type);
	_longString += ", Pierce St: ";
	if (IsStatePierceGood()) {
		_longString += "Good";
	}
	else if (IsStatePierceIn()) {
		_longString += "IN";
	}
	else if (IsStatePierceOut()){
		_longString += "OUT";
	}
	_longString += ", Turn St:";
	if (IsStateTurnGood()) {
		_longString += "GOOD";
	}
	return _longString;
}

void Candidate::SetTransport(bool isUdp) {
	if (isUdp) {
		_transport = "udp";
	} else {
		_transport = "tcp";
		_tcpType = "passive";//TODO: currently, passive is default
	}
}
