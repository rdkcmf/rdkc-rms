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

#ifndef __STUNMSG_H
#define __STUNMSG_H

#include "protocols/wrtc/iceutils.h"
class SocketAddress;

class DLLEXP StunMsg 
: public IceUtils {
public:
	StunMsg(string & inIpStr);
	StunMsg();
	virtual ~StunMsg();

	// Message get/reading routines
	static bool IsStun(IOBuffer & buf);
	static bool IsStun(uint8_t * pBuf, uint32_t len);
	static StunMsg * ParseBuffer(IOBuffer & buf, string & fromIpStr);
	static bool IsValidStunType(IOBuffer & buf);
	
	static const int STUN_HEADER_LEN = 20; // STUN header size
	
	// first call ParseBuffer for the Has/Get Attribute routines
	bool IsFromIpStr(string ipStr) { return ipStr == _fromIpStr; };
	bool CheckFromServer(string serverIpStr);
	bool IsFromServer() { return _fromServer; };
	string & GetFromIpStr() { return _fromIpStr; };
	bool HasAttribute(int attr);
	int FindAttribute(int attr);
	bool GetStringAttr(int attr, string & str);
	bool GetAddressAttr(int attr, string & ipPortStr);
	bool GetXORAddressAttr(int attr, string & ipPortStr);
	int GetErrorCodeAttr();
	bool GetErrorCodeAttr(int & code, string & msg);
	uint8_t * GetDataAttr(int & len);
	//
	// Message Makers
	/// create/writing routines
	static StunMsg * MakeMsg(int type, uint8_t * pTransId = NULL);
	void MakeHdr(int type, uint8_t * pTransId = NULL);
	void AddTransId(uint8_t * pTransId = NULL);	// create and put into msg body
	// Attribute added to bottom of message, call MakeMsg firstt
	void AddEom(int len);
	void AddStringAttr(int type, string & str);
	void AddUser(string & user);
	void AddRealm(string &  realm);
	void AddNonce(string & nonce);
	void AddUserRealmNonce(string & user, string &  realm, string & nonce);
	void AddAddressXOR(int attr, string ipPortStr);	// ipStr = w.x.y.z:pppp
	//void AddAddressXOR(int attr, uint32_t ip, uint16_t port);	// ipStr = w.x.y.z:pppp
	void AddAddress(int attr, uint32_t ip, uint16_t port);	 // 
	void AddAddressV6(int attr, uint8_t* ipv6, uint16_t port); // kludge... need to refactor... 
	void AddAddress(int attr, string ipPortStr);	// ipStr = w.x.y.z:pppp <<-- nobody calls this...
	void AddAllocateUdp();
	void AddLifetime(uint32_t secs = 6000);
	void AddUseCandidate();
	void AddPriority(uint32_t pri);
	void AddIceControlled(uint64_t tiebreaker = 0x2114A2422114A242ull);
	void AddIceControlling(uint64_t tiebreaker= 0x2114A2422114A242ull);
	void AddMessageIntegrity(string & key);
	void AddFingerprint();	// should call this last!
	void AddReqAddrFamily(bool isIpV6);
	bool AddData(uint8_t * pData, uint32_t len);
	//
	// Misc Public
	//
	int GetType() { return (int)_type; };
	// return pointer at the byte array for the message
	uint8_t * GetMessage() { return _msg; };
	// return the complete length of that byte array
	int GetMessageLen() { return STUN_HEADER_LEN + _len; };
	uint8_t * GetTransId() { return &_msg[8]; };
	// For easy debugging as needed
	string GetTransIdStr() {
		string s = format("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", _msg[8],
				_msg[9], _msg[10], _msg[11], _msg[12], _msg[13], _msg[14], _msg[15],
				_msg[16], _msg[17], _msg[18], _msg[19]);
		return s;
	};
	bool WasInTurn() {return _wasInTurn;};
	void SetInTurn(bool set=true) {_wasInTurn = set;};

	// return the Word at msg[index]
	uint16_t GetMsg16(int index);
	uint32_t GetMsg32(int index);
	void     GetMsgIPv6Addr(int index, uint8_t* pBuffer);
	void GetMsgStr(int index, string & res, int len);

private:
	// internal 
	bool _wasInTurn;	// was encapsulated in a TURN STUN DataSend Attribute
	//
	// message header fields
	uint16_t _type;
	uint16_t _len;		// len of data (not including STUN_HEADER_LEN byte header)
	uint8_t _cookie[4];		// init this to 0x2112A442
	//uint8_t  _transId[12];
	// misc
	string _fromIpStr;
	bool _fromServer;
	// msg construction fields
//	static const int MAX_STUN_MSG_LEN = 576;
	// Increasing max message size due to the intermittent "max size exceeded"
	// errors.  NOTE: I picked 2000 based on the max message size received, rounded
	// up to the nearest 1000's.  Might be a bit conservative.  Consider increasing
	// later on.
	static const int MAX_STUN_MSG_LEN = 2000; // 576;

	uint8_t _msg[MAX_STUN_MSG_LEN];
	int _zeroCheck;	// leave this right after _msg
	int _eom;	// end of message - (== len + headerLen)

	void Init();

	// set the word into msg
	void SetMsg16(int index, uint16_t val);
	void SetMsg32(int index, uint32_t val);
	void SetMsg64(int index, uint64_t val);
	void SetMsgStr(int index, string str);
	void SetMsgBytes(int index, uint8_t * pData, int len);
	void SetMsgLen(uint16_t len) { SetMsg16(2, len); };

	//
	// XOR Magic routines
	// - be careful of ENDIAN, 
	// - the val returning pair work high val to low val Endian safe
	//
	uint32_t XORMagic(uint32_t val);	// Endian safe, use before flip
	uint16_t XORMagic(uint16_t val);	// Endian safe, use before flip
	void XORMagicv6(uint8_t *pBuffer);
	void XORMagic(uint8_t * p, int len); // Works big endian assumption
	//
	// Misc. 
	//
	inline uint16_t RoundUpMod4(int attrLen) {
		return (attrLen + 3) & 0xFFFC;
	}
};

//----------------------//
// STUN Message Defines //
//----------------------//
// use this to pick out bytes from incoming message
struct stunHeaderBytes_t {
	uint8_t typeHi;
	uint8_t typeLo;
	uint8_t lenHi;
	uint8_t lenLo;
	uint8_t magic[4];	// 0x2112A442 
	uint8_t transId[12];
	uint8_t data[1];
};

enum stunMsgType {
	STUN_BINDING_REQUEST = 0x0001,
	STUN_BINDING_RESPONSE = 0x0101,
	STUN_BINDING_ERROR_RESPONSE = 0x0111,
	STUN_BINDING_INDICATION = 0x0011,
	STUN_SHARED_SECRET_REQUEST = 0x0002,
	STUN_SHARED_SECRET_RESPONSE = 0x0102,
	STUN_SHARED_SECRET_ERROR_RESPONSE = 0x0112,
	STUN_ALLOCATE_REQUEST = 0x0003,
	STUN_ALLOCATE_RESPONSE = 0x0103,
	STUN_ALLOCATE_ERROR_RESPONSE = 0x0113,
	STUN_REFRESH_REQUEST = 0x0004,
	STUN_REFRESH_RESPONSE = 0x0104,
	STUN_REFRESH_ERROR_RESPONSE = 0x0114,
	STUN_SEND_INDICATION = 0x0016,
	STUN_DATA_INDICATION = 0x0017,
	STUN_CREATE_PERM_REQUEST = 0x0008,
	STUN_CREATE_PERM_RESPONSE = 0x0108,
	STUN_CREATE_PERM_ERROR_RESPONSE = 0x0118,
	STUN_CHANNEL_BIND_REQUEST = 0x0009,
	STUN_CHANNEL_BIND_RESPONSE = 0x0109,
	STUN_CHANNEL_BIND_ERROR_RESPONSE = 0x0119
};
enum stunAttrType {
	STUN_ATTR_MAPPED_ADDR = 0x0001,
	STUN_ATTR_RESPONSE_ADDR = 0x0002,
	STUN_ATTR_CHANGE_REQUEST = 0x0003,
	STUN_ATTR_SOURCE_ADDR = 0x0004,
	STUN_ATTR_CHANGED_ADDR = 0x0005,
	STUN_ATTR_USERNAME = 0x0006,
	STUN_ATTR_PASSWORD = 0x0007,
	STUN_ATTR_MESSAGE_INTEGRITY = 0x0008,
	STUN_ATTR_ERROR_CODE = 0x0009,
	STUN_ATTR_UNKNOWN_ATTRIBUTES = 0x000A,
	STUN_ATTR_REFLECTED_FROM = 0x000B,
	STUN_ATTR_CHANNEL_NUMBER = 0x000C,
	STUN_ATTR_LIFETIME = 0x000D,
	STUN_ATTR_MAGIC_COOKIE = 0x000F,
	STUN_ATTR_BANDWIDTH = 0x0010,
	STUN_ATTR_XOR_PEER_ADDR = 0x0012,
	STUN_ATTR_DATA = 0x0013,
	STUN_ATTR_REALM = 0x0014,
	STUN_ATTR_NONCE = 0x0015,
	STUN_ATTR_XOR_RELAYED_ADDR = 0x0016,
	STUN_ATTR_REQ_ADDR_TYPE = 0x0017,
	STUN_ATTR_EVEN_PORT = 0x0018,
	STUN_ATTR_REQ_TRANSPORT = 0x0019,
	STUN_ATTR_DONT_FRAGMENT = 0x001A,
	STUN_ATTR_XOR_MAPPED_ADDR = 0x0020,
	STUN_ATTR_TIMER_VAL = 0x0021,
	STUN_ATTR_RESERVATION_TOKEN = 0x0022,
	STUN_ATTR_XOR_REFLECTED_FROM = 0x0023,
	STUN_ATTR_PRIORITY = 0x0024,
	STUN_ATTR_USE_CANDIDATE = 0x0025,
	STUN_ATTR_ICMP = 0x0030,
	STUN_ATTR_END_MANDATORY_ATTR,
	STUN_ATTR_START_EXTENDED_ATTR = 0x8021,
	STUN_ATTR_SOFTWARE = 0x8022,
	STUN_ATTR_ALTERNATE_SERVER = 0x8023,
	STUN_ATTR_REFRESH_INTERVAL = 0x8024,
	STUN_ATTR_FINGERPRINT = 0x8028,
	STUN_ATTR_ICE_CONTROLLED = 0x8029,
	STUN_ATTR_ICE_CONTROLLING = 0x802a,
	STUN_ATTR_RESPONSE_ORIGIN = 0x802b, //RFC-5780
	STUN_ATTR_OTHER_ADDRESS = 0x802c,	// ""
	STUN_ATTR_END_EXTENDED_ATTR
};
enum stunStatus {
	STUN_SC_TRY_ALTERNATE = 300,
	STUN_SC_BAD_REQUEST = 400,
	STUN_SC_UNAUTHORIZED = 401,
	STUN_SC_FORBIDDEN = 403,
	STUN_SC_UNKNOWN_ATTRIBUTE = 420,
	STUN_SC_ALLOCATION_MISMATCH = 437,
	STUN_SC_STALE_NONCE = 438,
	STUN_SC_TRANSITIONING = 439,
	STUN_SC_WRONG_CREDENTIALS = 441,
	STUN_SC_UNSUPP_TRANSPORT_PROTO = 442,
	STUN_SC_OPER_TCP_ONLY = 445,
	STUN_SC_CONNECTION_FAILURE = 446,
	STUN_SC_CONNECTION_TIMEOUT = 447,
	STUN_SC_ALLOCATION_QUOTA_REACHED = 486,
	STUN_SC_ROLE_CONFLICT = 487,
	STUN_SC_SERVER_ERROR = 500,
	STUN_SC_INSUFFICIENT_CAPACITY = 508,
	STUN_SC_GLOBAL_FAILURE = 600
};

#endif // __STUNMSG_H

