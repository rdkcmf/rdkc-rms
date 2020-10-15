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
 *
 */
#ifndef __CANDIDATE_H
#define __CANDIDATE_H

#include "protocols/wrtc/iceutils.h"

class SocketAddress;

class DLLEXP Candidate 
: public IceUtils {
public:
	Candidate();
	virtual ~Candidate();

	// Candidate States
	// these are the same values as the states in WrtcStun
	static const uint32_t CANST_NEW = 0;
	static const uint32_t CANST_TURN_SENT_BIND = 0x10;
	static const uint32_t CANST_TURN_GOOD = 0x40;
    static const uint32_t CANST_TURN_BAD = 0x80;
    static const uint32_t CANST_PIERCE_SENT = 0x100;
    static const uint32_t CANST_PIERCE_IN = 0x200;
    static const uint32_t CANST_PIERCE_OUT = 0x400;
    static const uint32_t CANST_PIERCE_GOOD = 0x600; // IN | OUT
    static const uint32_t CANST_PIERCE_BAD = 0x800;
    static const uint32_t CANST_DEAD = 0x1000;
	
	//
	// use these create routines for convenient defaults
	//
	static Candidate * CreateHost(string & ipStr);
	static Candidate * CreateReflex(string & ipStr, string & hostIpStr);
	static Candidate * CreateRelay(string & ipStr, string & hostIpStr);
	static Candidate * CreateServer(string & ipStr);
	static Candidate * CreateRemote(string & ipStr);

	Candidate * Clone();

	// build methods
	bool Set(string & canStr, bool remote = true);
	bool Get(string & str);

	// type fields
	int typeStrIndex(string str);
	
	void Init(string type, string ipStr, bool isRemote = true, bool isServer = false);
	bool IsSame(Candidate * pCan);
	bool IsIpStr(string ipStr);	// retrun true if matches
	bool IsSameIp(string ipStr);	// retrun true if ip matches byte wise
	bool IsIpPort(string ip, uint16_t port);	// return true if matches
	bool IsServer() { return _isServer; }
	string & GetIpStr() { return _ipStr; };
	bool IsIpv6() { return _ipAddress.find(':') != string::npos; };
	bool IsUdp() { return _transport == "udp"; };
	bool IsTcp() { return _transport == "tcp"; }

	// USE attribute from peer
	void SetPeerUse(bool set = true) {_peerUse = set;};
	bool IsPeerUse() { return _peerUse;};
	//
	// states - we have the Pierce and Turn categories, Pierce has In and Out (needs both)
	//  - Pierce Out means we've done a STUN BindRequest and gotten a good response
	//  - Pierce In means we've received a STUN BindRequest and sent a good response
	//  - TURN good means the TURN server has walked thru the handshake of Alloc and Permission
	// (CANST is short for CANdidate STate)
	//
	void SetStatePierceSent() { _state |= CANST_PIERCE_SENT; };	// not used I think?
	void SetStatePierceIn()	{ _state |= CANST_PIERCE_IN; };
	void SetStatePierceOut()	{ _state |= CANST_PIERCE_OUT; };
	// Good is both In and Out, so we don't set it directly
	//void SetStateGood() { _state |= CANST_PIERCE_GOOD; };
	void SetStatePierceBad()  { _state |= CANST_PIERCE_BAD; };	// not used?
	void SetStateTurnSentBind()  { _state |= CANST_TURN_SENT_BIND; };
	void SetStateTurnGood()  { _state |= CANST_TURN_GOOD; };
	void SetStateTurnBad()   { _state |= CANST_TURN_BAD; };	// not used?
	bool IsStateTurnSentBind()  { return 0 != ( _state & CANST_TURN_SENT_BIND); };
	bool IsStateTurnGood()  { return 0 != ( _state & CANST_TURN_GOOD); };
	bool IsStateTurnBad()   { return 0 != ( _state & CANST_TURN_BAD); };	// not used?
	bool IsStatePierceSent()  { return 0 != (_state & CANST_PIERCE_SENT); };
	bool IsStatePierceIn()	{ return 0 != (_state & CANST_PIERCE_IN); };
	bool IsStatePierceOut()	{ return 0 != (_state & CANST_PIERCE_OUT); };
	bool IsStatePierceGood() { return CANST_PIERCE_GOOD == (_state & CANST_PIERCE_GOOD); };
	bool IsStateDead() { return 0 != (_state & CANST_DEAD); };
	bool IsStateGood() { return IsStatePierceGood() || IsStateTurnGood(); };
	//
	uint32_t GetRawPriority() { return _priority;};
	uint32_t GetPriority();	// return priority adjusted by "goodness"
	bool IsBetterThan(Candidate * pCan);

	// TURN negotiation helpers
	void SetLastTurnTxId(uint8_t * pTxId);
	uint8_t* GetLastTurnTxId();
	bool IsLastTurnTxId(uint8_t * pTxId);
	bool IsReflex() {return (_type == "srflx");};
	bool IsHost() {return (_type == "host");};
	bool IsRelay() {return (_type == "relay");}
	void IncrementComponentId();

	string & GetShortString();
	string & GetLongString();
	
	void SetTransport(bool isUdp);
	
	string GetType() { return _type; };
private:
	SocketAddress _theAddress;

	// the following are the fields in the candidate record
	string   _foundation;
	uint32_t _componentId;
	string   _transport;
	uint32_t _priority;
	string   _ipAddress;
	uint16_t _port;
	string  _type;	// "host", "srflx", ...
	string   _rem;	// remainder
	string _ipStr;	// w.x.y.z:pppp easy compare format
	string _tcpType; // applicable for TCP transport only, "active, passive, so"

	string _hostIpStr;	// for raddr & rport fields

	bool _isServer;	
	bool _isRemote;
	uint32_t _state;

	bool _peerUse;
	uint8_t _lastTurnTxId[12];	// last transaction ID

	int _retryCount;

	string _shortString;	// short information string
	string _longString;

	static Candidate * CreateRaddr(string type, string & ipStr, string & hostIpStr, bool server);
	bool IsIP4() { return IpStrIsIP4(_ipAddress); };
};

#endif //__CANDIDATE_H

