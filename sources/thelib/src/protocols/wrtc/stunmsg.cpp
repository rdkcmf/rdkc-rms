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

#include "protocols/wrtc/stunmsg.h"
#include "utils/misc/crypto.h"

// static method
bool StunMsg::IsValidStunType(IOBuffer & buf) {
	uint8_t * pBuf = GETIBPOINTER(buf);
	stunHeaderBytes_t * pH = (stunHeaderBytes_t *)pBuf;
	uint16_t stuntype = (pH->typeHi << 8) + pH->typeLo;
	switch (stuntype) {
		case STUN_DATA_INDICATION:  //= 0x0017,
		case STUN_SEND_INDICATION:  //= 0x0016,
		case STUN_BINDING_REQUEST:  //= 0x0001,
		case STUN_ALLOCATE_REQUEST:  //= 0x0003,
		case STUN_REFRESH_REQUEST:  //= 0x0004,
		case STUN_CHANNEL_BIND_REQUEST:  //= 0x0009,
		case STUN_SHARED_SECRET_REQUEST:  //= 0x0002,
		case STUN_CREATE_PERM_REQUEST:  //= 0x0008,
		case STUN_BINDING_INDICATION:  //= 0x0011,
		case STUN_BINDING_RESPONSE:  //= 0x0101,
		case STUN_SHARED_SECRET_RESPONSE:  //= 0x0102,
		case STUN_ALLOCATE_RESPONSE:  //= 0x0103,
		case STUN_REFRESH_RESPONSE:  //= 0x0104,
		case STUN_CREATE_PERM_RESPONSE:  //= 0x0108,
		case STUN_CHANNEL_BIND_RESPONSE:  //= 0x0109,
		case STUN_ALLOCATE_ERROR_RESPONSE:  //= 0x0113,
		case STUN_SHARED_SECRET_ERROR_RESPONSE:  //= 0x0112,
		case STUN_REFRESH_ERROR_RESPONSE:  //= 0x0114,
		case STUN_CREATE_PERM_ERROR_RESPONSE:  //= 0x0118,
		case STUN_BINDING_ERROR_RESPONSE:  //= 0x0111,
		case STUN_CHANNEL_BIND_ERROR_RESPONSE:  //= 0x0119
			return true;
		default:
			return false;
	}
}
// static method
bool StunMsg::IsStun(IOBuffer & buf) {
	uint32_t len = GETAVAILABLEBYTESCOUNT(buf);
	uint8_t * pBuf = GETIBPOINTER(buf);
	return IsStun(pBuf, len);
}

// static method
bool StunMsg::IsStun(uint8_t * pBuf, uint32_t len) {
	if (len < sizeof(stunHeaderBytes_t)) return false; //====>
	stunHeaderBytes_t * pH = (stunHeaderBytes_t *)pBuf;
	
	//UPDATE: We check for the following:
	// 1. First 2 most significat bits is 0
	// 2. Contains the magic cookie 0x2112A442
	// 3. Length of the message itself and not of the buffer
	return (pH->typeHi < 3)
		&& (pH->magic[0] == 0x21)
		&& (pH->magic[1] == 0x12)
		&& (pH->magic[2] == 0xA4)
		&& (pH->magic[3] == 0x42)
		&& (((pH->lenHi << 8) + pH->lenLo) <= MAX_STUN_MSG_LEN)
		;
}

StunMsg::StunMsg() {
	Init();
}

StunMsg::StunMsg(string & inIpStr) {
	Init();
	_fromIpStr = inIpStr;
}

StunMsg::~StunMsg() {
}

void StunMsg::Init(){ 
	_type = 0;
	_len = 0;
	_cookie[0] = 0x21;
	_cookie[1] = 0x12;
	_cookie[2] = 0xA4;
	_cookie[3] = 0x42;
	_eom = 20;	// 
	_zeroCheck = 0;	// it should stay zero, msg shouldn't overflow
	_fromServer = false;
	_wasInTurn = false;	// not encapsulated in TURN STUN DataSend
}

// ParseBuffer is a static call
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//| Type (2)                      | Length (2)                    |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|0 0 0 0 0 0 0 0| Family  =1    | Port                          |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|                         4bytes                                |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
StunMsg * StunMsg::ParseBuffer(IOBuffer & buf, string & ipStr) {
	// assume caller verified it is stun by calling IsStun()!
	// parse the header and copy the message locally
	uint8_t * pBuf = GETIBPOINTER(buf);
	stunHeaderBytes_t *pHeader = (stunHeaderBytes_t *)pBuf;
	StunMsg * pMsg = new StunMsg(ipStr);
	pMsg->_type = (pHeader->typeHi << 8) + pHeader->typeLo; // type in network byte order...
	pMsg->_len = (pHeader->lenHi << 8) + pHeader->lenLo;	// len in networbyte order
	memcpy(&pMsg->_msg[0], pBuf, STUN_HEADER_LEN + pMsg->_len);
	pMsg->_eom += pMsg->_len;	// new _eom
	//
	return pMsg;
}

bool StunMsg::CheckFromServer(string serverIpStr) {
	_fromServer = serverIpStr == _fromIpStr;
	return _fromServer;
}

bool StunMsg::HasAttribute(int attr) {
	return 0 != FindAttribute(attr);
}

int StunMsg::FindAttribute(int attr) {
	// presume caller has called ParseBuffer
	// dance a across the attributes, 
	// attr header is 4 bytes: 2 bytes id, 2 bytes len (len after header)
	int a = STUN_HEADER_LEN; 
	while( a < _eom) {
		int chkAttr = (int)GetMsg16(a); // GET the TYPE code.
		if (attr == chkAttr) {			// IF TYPE_code == desired_type
			return a;					//    then return it!
		}
		int attrLen = GetMsg16(a + 2);  // OTHERWISE, prepare to skip
										// to the next attribute!!!

		// round up attrLen so next attribute is uint32 aligned
		a += 4 + RoundUpMod4(attrLen);
	}
	return 0;
}

// attribute layout:  attr[2],len[2],string[len]
//
bool StunMsg::GetStringAttr(int attr, string & str) {
	int a = FindAttribute(attr);
	if (!a) return false; //====================>
	int len = GetMsg16(a + 2);
	str = "";
	for (int i = 0; i < len; i++) {
		str += _msg[a + 4 + i];
	}
	return true;
}

// Address Attribute:
//  0       2          6       8
//  type[2],len[2],0,1,port[2],ip[4] = 12 bytes
//
// ATTRIB=STUN_ATTR_MAPPED_ADDR=1
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//| Type (2)                      | Length (2)                    |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|0 0 0 0 0 0 0 0| Family        | Port                          |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|                         4bytes                                |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
bool StunMsg::GetAddressAttr(int attr, string & ipPortStr) {
	// Find the address attribute!
	int a = FindAttribute(attr);
	if (!a) return false; //====================>

	// Address attribute found.  Now get the length.  IF this is
	// an IPv4 address, then the length should be 8 bytes.  If this
	// is an IPv6 address, then the length should be 20 bytes!!!!!
	int nAttrLen = GetMsg16(a + 2);
	int nFamily  = GetMsg16(a + 4);
	if (nFamily == 1) { // FAMILY == 1 >> IPv4
		if (nAttrLen < 8) {
			FATAL("Attribute: %d is short (len=%d", attr, nAttrLen);
			return false;
		}
		uint16_t port = GetMsg16(a + 6);
		uint32_t ip = GetMsg32(a + 8);
		ToIpStrV4(ip, port, ipPortStr);
	} else if (nFamily == 2) { // FAMILY == 2 >> IPv6
		if (nAttrLen < 20) {
			FATAL("Attribute: %d is short (len=%d", attr, nAttrLen);
			return false;
		}
		uint16_t port = GetMsg16(a + 6);
		uint8_t  ipv6[16];
		GetMsgIPv6Addr(a + 8, ipv6);
		ToIpStrV6(ipv6, port, ipPortStr);
	} else {
		FATAL("Unknown Address Family: %d", nFamily);
		return false;
	}

	return true;
}

bool StunMsg::GetXORAddressAttr(int attr, string & ipPortStr) {
	int a = FindAttribute(attr);
	if (!a) {
		//WARN("$b2$ GetXORAddressAttr() Failed to find attr: %d (0x%x)", attr, attr);
		return false; //====================>
	}

	// Address attribute found.  Now get the length.  IF this is
	// an IPv4 address, then the length should be 8 bytes.  If this
	// is an IPv6 address, then the length should be 20 bytes!!!!!
	int nAttrLen = GetMsg16(a + 2);
	int nFamily = GetMsg16(a + 4);
	if (nFamily == 1) { // FAMILY == 1 >> IPv4
		if (nAttrLen < 8) {
			FATAL("Attribute: %d is short (len=%d", attr, nAttrLen);
			return false;
		}
		uint16_t port = XORMagic(GetMsg16(a + 6));
		uint32_t ip = XORMagic(GetMsg32(a + 8));
		ToIpStrV4(ip, port, ipPortStr);

	} else if (nFamily == 2) { // FAMILY == 2 >> IPv6
		if (nAttrLen < 20) {
			FATAL("Attribute: %d is short (len=%d", attr, nAttrLen);
			return false;
		}
		uint16_t port = XORMagic(GetMsg16(a + 6));
		uint8_t  ipv6[16];
		GetMsgIPv6Addr(a + 8, ipv6);
		XORMagicv6(ipv6);
		ToIpStrV6(ipv6, port, ipPortStr);
	} else {
		FATAL("Unknown Address Family: %d", nFamily);
		return false;
	}

	return true;
}

// ErrorCode Attr: 
// 0        2       4           6        8
// attr[2], len[2], reserve[2], code[2], text[len-2]
int StunMsg::GetErrorCodeAttr() {
	int a = FindAttribute(STUN_ATTR_ERROR_CODE);
	if (a) {
		a = GetMsg16(a + 6);
	}
	return a;
}
bool StunMsg::GetErrorCodeAttr(int & code, string & errMsg) {
	int a = FindAttribute(STUN_ATTR_ERROR_CODE);
	if (a) {
		code = GetMsg16(a + 6);
		uint32_t alen = GetMsg16(a + 2); // attribute len
		_msg[a + 4 + alen] = 0;	// null terminate the reason "string"
							// note that zero check is after _msg for a reason
		errMsg = (char *)(&_msg[a + 8]);
		return true;	// we set something so returned true
	}
	return false;
}

uint8_t * StunMsg::GetDataAttr(int & len) {
	int a = FindAttribute(STUN_ATTR_DATA);
	if (!a) {
		len = 0;
		return NULL;
	}
	len = (int) GetMsg16(a + 2);	// attribute length
	return &_msg[a+4];	// _msg + attribute_start + hdr
}

//
// Message Makers
//
// MakeMsg() is static!!
//
StunMsg * StunMsg::MakeMsg(int type, uint8_t * pTransId) {
	StunMsg * pMsg = new StunMsg();
	pMsg->MakeHdr(type, pTransId);
	return pMsg;
}

void StunMsg::MakeHdr(int type, uint8_t * pTransId) {
	// fill in the header fields of the msg[] array
	// hdr is: type[2], len[2], cookie[4], transId[12]
	_type = type;
	SetMsg16(0, (uint16_t) type);	// type, 2 bytes big-endian
	SetMsg16(2, 0);		// payload len, 2 bytes, big-endian
	SetMsgBytes(4, _cookie, 4);	// _cookie is set by the constructor
	AddTransId(pTransId);	// sets in _msg[8+]
}

void StunMsg::AddTransId(uint8_t * pTransId) {
	if (pTransId) {
		SetMsgBytes(8, pTransId, 12);
	}
	else {	// create a new id and stuff it in
		//$$ ToDo - make this more sensible??  what is sensible??
		static uint32_t tid = 111111111;	// max: 2,147,483,647
		string xs = format("123%09u", tid++);
		for (int i = 0; i < 12; i++) {
			_msg[8+i] = xs[i];
		}
	}
	return;
}

void StunMsg::AddEom(int len) {
	// add this many bytes to _eom and adjust _len fields!
	_eom += RoundUpMod4(len);	// offset of end of message
	_len = _eom - STUN_HEADER_LEN;	// complete message payload len
	SetMsg16(2, _len);
}
//
// Attribute Setters - AddXXXAttr
// - these append attributes to the end of msg[]
// - most attributes are:  type[2],len[2],string[len],pad[mod4] = len+4
//
void StunMsg::AddStringAttr(int type, string & str) {
	uint16_t len = (uint16_t) str.size();
	if (RoundUpMod4(len) + 4 + _eom > MAX_STUN_MSG_LEN) {
		FATAL("Adding too many attributes: type: %"PRIu16", %s", (uint16_t) type, STR(str));
		return;
	}
	SetMsg16(_eom, (uint16_t) type);// attribute type
	SetMsg16(_eom+2, len);			// len of this attribute
	SetMsgStr(_eom + 4, str);		// the attribute data
	// pad with zeroes
	int k = _eom + 4 + len;
	while (k & 3) {
		_msg[k++] = 0;
	}
	AddEom(len + 4);		// adjust our eom pointer and len fields
}

void StunMsg::AddUser(string & user) {
	if (user.size()) {
		AddStringAttr(STUN_ATTR_USERNAME, user);
	}
}

void StunMsg::AddRealm(string &  realm) {
	if (realm.size()) {
		AddStringAttr(STUN_ATTR_REALM, realm);
	}
}

void StunMsg::AddNonce(string & nonce) {
	if (nonce.size()) {
		AddStringAttr(STUN_ATTR_NONCE, nonce);
	}
}

void StunMsg::AddUserRealmNonce(string & user, string &  realm, string & nonce)  {
	AddUser(user);
	AddRealm(realm);
	AddNonce(nonce);
}


#define _ATTR_MARKER_	2
#define _LEN_MARKER_	4
#define _FAMILY_MARKER_ 5
#define _PORT_MARKER_	6
#define _ADDR_MARKER_	8
//
// Address Attribute:
//  0       2          6       8
//  type[2],len[2],0,1,port[2],ip[4] = 12 bytes
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//| Type (2)                      | Length (2)                    |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|0 0 0 0 0 0 0 0| Family  =1    | Port                          |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|                         4bytes                                |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
void StunMsg::AddAddress(int attr, uint32_t ip, uint16_t port) {
	SetMsg16(_eom, (uint16_t) attr);	// Message Type
	SetMsg16(_eom + 2, 8);				// len of attribute (doesn't include the type & len)
	_msg[_eom + 4] = 0;					// Hardcoded to 0x00
	_msg[_eom + 5] = 1;					// 1=ipv4, 2=ipv6
	SetMsg16(_eom + 6, port);			// Port
	SetMsg32(_eom + 8, ip);				// IP data.
	AddEom(12);		// adjust our eom pointer and len fields
}


// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//| Type (2)                      | Length (2)                    |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|0 0 0 0 0 0 0 0| Family =2     | Port                          |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
//|                          16bytes                              |
//|                                                               |
//|                                                               |
//|                                                               |
//+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+‐+
void StunMsg::AddAddressV6(int attr, uint8_t* ipv6, uint16_t port) {
	const int v6AddressPayloadLength = 20;
	SetMsg16(_eom, (uint16_t)attr);		// Message Type
	SetMsg16(_eom + _ATTR_MARKER_,		// len of attribute (doesn't include the type & len)
			 v6AddressPayloadLength);				
	_msg[_eom + _LEN_MARKER_] = 0;		// Hardcoded to 0x00
	_msg[_eom + _FAMILY_MARKER_] = 2;	// 1=ipv4, 2=ipv6
	SetMsg16(_eom + _PORT_MARKER_,		// Port
			 port);			
	SetMsgBytes(_eom + _ADDR_MARKER_, ipv6, 16); // IP Data
	AddEom(24);							// adjust our eom pointer and len fields
}

//void StunMsg::AddAddressXOR(int attr, uint32_t ip, uint16_t port) {
//	AddAddress(attr, XORMagic(ip), XORMagic(port));
//}

// ipStr = w.x.y.z:pppp
// ipStr = [aa:bb:.....:zz]:pppp
void StunMsg::AddAddressXOR(int attr, string ipPortStr) {
	uint16_t port;
	if (ipPortStr.find("]:") != string::npos) {
		uint8_t ipv6[16];
		SplitIpStrV6(ipPortStr, ipv6, port); // ipv6 <- in6_addr
		XORMagicv6(ipv6); // ipv6 ^ [magic_cookie+txnid];
		AddAddressV6(attr, ipv6, XORMagic(port));
	} else {
		uint32_t ip;	// v4 only!
		//INFO("$b2$ AddAddressXOR calling SplitIpStrV4(%s)", STR(ipStr));
		SplitIpStrV4(ipPortStr, ip, port);
		AddAddress(attr, XORMagic(ip), XORMagic(port));
	}
}

// ipStr = w.x.y.z:pppp
void StunMsg::AddAddress(int attr, string ipPortStr) {
	uint16_t port;
	if (ipPortStr.find("]:") != string::npos) { // we have an [IPv6]:Port combo
		uint8_t ipv6[16];
		SplitIpStrV6(ipPortStr, ipv6, port);
		AddAddressV6(attr, ipv6, port);
	} else { // regular IPv4:port
		uint32_t ip;	// v4 only!
		//INFO("$b2$ AddAddress calling SplitIpStrV4(%s)", STR(ipStr));
		SplitIpStrV4(ipPortStr, ip, port);
		AddAddress(attr, ip, port);
	}
}

void StunMsg::AddAllocateUdp() {
	SetMsg16(_eom, STUN_ATTR_REQ_TRANSPORT);
	SetMsg16(_eom + 2, 4);
	_msg[_eom + 4] = 0x11;	// UDP
	_msg[_eom + 5] = 0;
	_msg[_eom + 6] = 0;
	_msg[_eom + 7] = 0;
	AddEom(8);		// adjust our eom pointer and len fields
}

void StunMsg::AddLifetime(uint32_t secs) {
	SetMsg16(_eom, STUN_ATTR_LIFETIME);	// attr type
	SetMsg16(_eom + 2, 4);	// attr data len
	SetMsg32(_eom + 4, secs);	// first attr field (0=forever?)
	AddEom(8);		// adjust our eom pointer and len fields
}


void StunMsg::AddUseCandidate() {
	SetMsg16(_eom, STUN_ATTR_USE_CANDIDATE);	// attr type
	SetMsg16(_eom + 2, 0);	// attr data len
	AddEom(4);		// adjust our eom pointer and len fields
}

void StunMsg::AddPriority(uint32_t pri) {
	SetMsg16(_eom, STUN_ATTR_PRIORITY);
	SetMsg16(_eom + 2, 4);	// attr data len
	SetMsg32(_eom + 4, pri);
	AddEom(8);		// adjust our eom pointer and len fields
}

void StunMsg::AddIceControlled(uint64_t tiebreaker) {
	SetMsg16(_eom, STUN_ATTR_ICE_CONTROLLED);
	SetMsg16(_eom + 2, 8);	// attr data len
	SetMsg64(_eom + 4, tiebreaker);
	AddEom(12);		// adjust our eom pointer and len fields
}

void StunMsg::AddIceControlling(uint64_t tiebreaker) {
	SetMsg16(_eom, STUN_ATTR_ICE_CONTROLLING);
	SetMsg16(_eom + 2, 8);	// attr data len
	SetMsg64(_eom + 4, tiebreaker);
	AddEom(12);		// adjust our eom pointer and len fields
}

void StunMsg::AddMessageIntegrity(string & key) {
	SetMsg16(_eom, STUN_ATTR_MESSAGE_INTEGRITY);
	SetMsg16(_eom + 2, 20);	// attr data len
	uint8_t * pHMAC = &_msg[_eom+4];	// hash goes here
	AddEom(24);		// ensure length field is how it "will" be
	//WARN("$b2$ calling HMACsha1(key=%s, len=%d", STR(key), (int)key.size());
	HMACsha1((void *)_msg, (uint32_t)_eom-24,
			(void *) STR(key), (uint32_t)key.size(), (void *)pHMAC);
}

void StunMsg::AddFingerprint() {
	SetMsg16(_eom, STUN_ATTR_FINGERPRINT);
	SetMsg16(_eom + 2, 4);	// attr data len
	// CRC32 is 4 bytes next
	AddEom(8);		// ensure length field is how it "will" be
	// get the CRC up to before this attribute and place it at the end
	SetMsg32(_eom - 4, 0x5354554e ^ calcCrc32(_msg, _eom - 8));
}

void StunMsg::AddReqAddrFamily(bool isIpV6) {
	SetMsg16(_eom, STUN_ATTR_REQ_ADDR_TYPE);	// attr type
	SetMsg16(_eom + 2, 4);	// attr len
	
	if (isIpV6) {
		_msg[_eom + 4] = 0x02;	// ipv6
	} else {
		_msg[_eom + 4] = 0x01;	// ipv4
	}
	// Reserved
	_msg[_eom + 5] = 0;
	_msg[_eom + 6] = 0;
	_msg[_eom + 7] = 0;
	
	AddEom(8);
}

bool StunMsg::AddData(uint8_t * pData, uint32_t len) {
	if (RoundUpMod4(len) + 4 + _eom > MAX_STUN_MSG_LEN) {
		FATAL("AddData is too big, len=%d: ", len);
		return false;
	}
	SetMsg16(_eom, (uint16_t) STUN_ATTR_DATA);// attribute type
	SetMsg16(_eom+2, len);			// len of this attribute
	memcpy(&_msg[_eom+4], pData, len); // the attribute data
	AddEom(len + 4);		// adjust our eom pointer and len fields
	return true;
}
//
// Simple Getters and Setters
//
// get the word from msg
//
uint16_t StunMsg::GetMsg16(int index) {
	return (_msg[index] << 8) | _msg[index + 1];
}

uint32_t StunMsg::GetMsg32(int index) {
	return (_msg[index] << 24) | (_msg[index + 1] << 16)
		| (_msg[index + 2] << 8) | _msg[index + 3];
}

void StunMsg::GetMsgIPv6Addr(int index, uint8_t* pBuffer) {
	memcpy(pBuffer, &_msg[index], 16);
}

void StunMsg::GetMsgStr(int index, string & res, int len) {
	for (int i = 0; i < len; i++) {
		res += (char)_msg[index + i];
	}
}

// set the word into msg
void StunMsg::SetMsg16(int index, uint16_t val) {
	_msg[index] = 0xFF & (val >> 8);
	_msg[index + 1] = 0xFF & val;
}

void StunMsg::SetMsg32(int index, uint32_t val) {
	_msg[index]   = 0xFF & (val >> 24);
	_msg[index+1] = 0xFF & (val >> 16);
	_msg[index+2] = 0xFF & (val >> 8);
	_msg[index+3] = 0xFF & val;
}

void StunMsg::SetMsg64(int index, uint64_t val) {
	_msg[index]   = 0xFF & (val >> 56);
	_msg[index+1] = 0xFF & (val >> 48);
	_msg[index+2] = 0xFF & (val >> 40);
	_msg[index+3] = 0xFF & (val >> 32);
	_msg[index+4] = 0xFF & (val >> 24);
	_msg[index+5] = 0xFF & (val >> 16);
	_msg[index+6] = 0xFF & (val >> 8);
	_msg[index+7] = 0xFF & val;
}

void StunMsg::SetMsgStr(int index, string str) {
	if (index + str.size() > (int)MAX_STUN_MSG_LEN) {
		FATAL("Stun Message grew out of control!!");
		return;	// just quietly do nothing I guess
	}
	for (uint32_t i = 0; i < str.size(); i++) {
		_msg[index + i] = str[i];
	}
}

void StunMsg::SetMsgBytes(int index, uint8_t * pData, int len) {
	if (index + len > MAX_STUN_MSG_LEN) {
		FATAL("Stun Message grew out of control!!");
		return;	// just quietly do nothing I guess
	}
	for (uint32_t i = 0; i < (uint32_t)len; i++) {
		_msg[index + i] = pData[i];
	}
}


//
// XOR Magic routines -
// - be careful of ENDIAN, 
// - the val returning pair work high val to low val Endian safe
//
uint32_t StunMsg::XORMagic(uint32_t val) {
	return val ^ 0x2112A442;
}

uint16_t StunMsg::XORMagic(uint16_t val) {
	return val ^ 0x2112;
}

//
// I assume that pBuffer points to a 16byte long array
//
// If the IP address
// family is IPv6, X‐Address is computed by taking the mapped IP address
// in host byte order, XOR'ing it with the concatenation of the magic
// cookie and the 96‐bit transaction ID, and converting the result to
// network byte order.
void StunMsg::XORMagicv6(uint8_t *pBuffer) {
	// XOR the magic number first....
	//uint32_t _xor32 = XORMagic(((uint32_t *) pBuffer)[0]);
	//memcpy(pBuffer, &_xor32, sizeof(uint32_t));

	// Set the correct order
	uint32_t tmp = (pBuffer[0] << 24) | (pBuffer[1] << 16)
		| (pBuffer[2] << 8) | pBuffer[3];	
	uint32_t _xor32 = XORMagic(tmp);

	// The order should then be re-arranged to get the correct value
	pBuffer[0] = (_xor32 & 0xFF000000) >> 24;
	pBuffer[1] = (_xor32 & 0x00FF0000) >> 16;
	pBuffer[2] = (_xor32 & 0x0000FF00) >> 8;
	pBuffer[3] = (_xor32 & 0x000000FF);

	// now to XOR the transaction ID
	// FIRST let's get the transaction ID.  According to RFC5389, it's located
	// at index 8 from the message.
	uint8_t *pTxnId = &_msg[8];
	for (int nIndex = 0; nIndex < 12; nIndex++) {
		pBuffer[4 + nIndex] ^= pTxnId[nIndex];
	}
}

// assume final endian!
//void StunMsg::XORMagic(uint8_t * p, int len) {
//	int i = 0;
//	int j = 0;
//	while (i++ < len) {
//		*(p++) ^= _cookie[j++];
//		if (j > 3) j = 0;
//	}
//}


