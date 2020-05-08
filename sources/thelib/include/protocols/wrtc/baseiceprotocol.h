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

#ifdef HAS_PROTOCOL_WEBRTC
#ifndef __BASEICEPROTOCOL_H
#define __BASEICEPROTOCOL_H

#include "iceutils.h"
#include "protocols/baseprotocol.h"

class WrtcConnection;
class WrtcSigProtocol;
class Candidate;
class StunMsg;

class DLLEXP BaseIceProtocol
	: public BaseProtocol, public IceUtils {
public:
	BaseIceProtocol(uint64_t type);
	virtual ~BaseIceProtocol();

	virtual void Start(time_t ms); // start prepping outgoing traffic
	void Initialize(string bindIp, WrtcConnection * pWrtc, WrtcSigProtocol * pSig);

	// Timers now have the option to reset the counters and/or logic within
	// Add isConnected to signal already connected state
	bool FastTick(bool reset, bool &isConnected);
	bool SlowTick(bool reset);

	uint32_t Select();	// perform final selection, return best priority
	bool IsBetterThan(BaseIceProtocol * pStun);	// is our best better than his best?
	string & GetHostIpStr() { return _hostIpStr; };
	string & GetBestCanIpStr() {
		if (_bestCan) return _bestIpStr;
		return _turnBestIpStr;
	};
	Candidate * GetBestCandidate() {
		if (_bestCan) return _bestCan;
		return _turnBestCan;
	};
	void ReportBestCandidate(); // send STUN message selecting this candidate
	//
	// Config calls from WrtcConnection
	//
	bool SetStunServer(string stunServerIpStr);
	bool SetTurnServer(string username, string credential, string ipPort);
	void SetIceUser(string usr, string pwd, string peerUsr, string peerPwd);
	void SetIceFingerprints(const string myfp, const string peerfp);
	void SetIceControlling(bool is = true) {_iceControlling = is;};

	bool AddCandidate(Candidate *& pCan, bool clone = true);

	// to check the host ip address's version
	bool IsIpv6() { return SocketAddress::isV6(_hostIpStr); };

	// 
	// BaseProtocolOverrides
	// - who knew, looks like UDPCarrier calls the following one:
	virtual bool SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool AllowFarProtocol(uint64_t type) {return true;};
	virtual bool AllowNearProtocol(uint64_t type) { return true; };
	virtual bool SignalInputData(int32_t recvAmount) { return true; };
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool EnqueueForOutbound();
	virtual IOHandler *GetIOHandler();

	/*!
	@brief Gets the input buffers
	*/
	virtual IOBuffer * GetInputBuffer() { return &_inputBuffer; };

	bool IsReflexSent() { return 0 != (_state & ST_REFLEX_SENT); };
	bool IsReflexGood() { return 0 != (_state & ST_REFLEX_GOOD); };
	bool IsTurnAlloc() { return 0 != (_state & ST_TURN_ALLOC);};
	bool IsTurnPerm() { return 0 != (_state & ST_TURN_PERM);};
	bool IsTurnBind() { return 0 != (_state & ST_TURN_BIND);};
	bool IsTurnGood()   { return 0 != (_state & ST_TURN_GOOD); };
	bool IsPierceSent() { return 0 != (_state & ST_PIERCE_SENT); };
	bool IsPierceIn()   { return 0 != (_state & ST_PIERCE_IN); };
	bool IsPierceOut()  { return 0 != (_state & ST_PIERCE_OUT); };
	bool IsPierceGood() { return ST_PIERCE_GOOD == (_state & ST_PIERCE_GOOD); };
	bool HaveGood()     { return 0 != (_state & ST_HAVE_GOOD); };
	bool IsDead()       { return 0 != (_state & ST_DEAD); };

	bool IsTcpType() { return _isTcp; };

	// override default retry count
	void SetMaxRetries(int newMaxRetries) { _maxRetries = newMaxRetries; }

	/**
	 * Terminate this ICE instance and do all needed unlinking.
	 * 
	 * @param externalCall Indicates if called within the class, or externally from another class
	 */
	void Terminate(bool externalCall = true);
protected:
	IOBuffer _inputBuffer;
	IOBuffer _tmpBuffer;
	//
	WrtcConnection * _pWrtc;	// main WebRTC Connection object
	WrtcSigProtocol * _pSig;	// Signalling Protocol (Rendezvous Server)

	SocketAddress _bindAddress;
	string _bindIp;
	uint16_t _bindPort;
	string _hostIpStr;
	string _reflexIpStr;
	//
	// Timer stuff
	time_t _startMs;	// start time delta in milliseconds
	//
	// retries
	bool _allRetriesZero;	// all negotiation is done
	int _stunRetries;
	int _turnRetries;
	int _canSendRetries;
	int _pierceRetries;
	int _maxRetries;
	int _maxRetriesLimit;
	int _connDownCounter;

	int _chromePeerBindRetries;	//

	// Candidates
	vector<Candidate *> _cans;
	Candidate * _hostCan;
	Candidate * _reflexCan;
	Candidate * _relayCan;
	//
	// Selection!!
	Candidate * _bestCan;
	string _bestIpStr;
	uint32_t _bestPriority;
	SocketAddress _bestAddress; // for v6

	// STUN server stuff
	string _stunServerIpStr;	// format is ip:port
	string _stunServerIp;
	SocketAddress _stunServerAddress;
	//uint16_t _stunServerPort;
	bool _stunServerDone;
	// turn config (turn is confusing because it uses STUN messages!!)
	// -- (so we ambiguously call both the stunServer and turnServer the servver)
	// -- ("_stunServerDone" used to disambiguate)
	string _turnUsername;
	string _turnCredential;
	string _turnServerIpStr;
	uint32_t _turnServerIp;
	uint16_t _turnServerPort;
	SocketAddress _turnServerAddress;
	string _turnRealm;
	string _turnNonce;
	string _turnKeyStr;
	string _turnAllocIpStr;	// ip:port of TURN;s Allocation Relayed Transport Addr for us
	Candidate * _turnBestCan;
	string _turnBestIpStr;
	uint32_t _turnBestIp;
	uint8_t _turnBestIpv6[16];
	uint16_t _turnBestPort;
	string _turnRelayedTransportAddr;	// addr of TURN alloc for this STUN - confusing
	bool _usingTurn;
	bool _turnServerDone;
	//
	// ice user stuff
	bool _iceControlling;
	string _iceUser;
	string _icePwd;
	string _iceFingerprint;
	string _peerIceUser;
	string _peerIcePwd;
	string _peerIceFingerprint;
	string _iceUserUser;	// _peerIceUser ':' _iceUser
	// state handling
	bool _started;	// our Start() method was called and we can send now
	uint32_t _state;
	
	bool _isTcp;
	bool _isTerminating;
	
	string _candidateType; // indicates the type of candidates to use

	static const uint32_t ST_NEW = 0;
	static const uint32_t ST_REFLEX_SENT = 1;
	static const uint32_t ST_REFLEX_GOOD = 2;
	static const uint32_t ST_TURN_ALLOC = 4;
	static const uint32_t ST_TURN_PERM = 8;
	static const uint32_t ST_TURN_BIND = 0x10;
	static const uint32_t ST_TURN_GOOD = 0x40;
	static const uint32_t ST_TURN_BAD = 0x80;
	static const uint32_t ST_PIERCE_SENT = 0x100;
	static const uint32_t ST_PIERCE_IN = 0x200;
	static const uint32_t ST_PIERCE_OUT = 0x400;
	static const uint32_t ST_PIERCE_GOOD = 0x600;
	static const uint32_t ST_HAVE_GOOD = 0x800;
	static const uint32_t ST_DEAD = 0x1000;

	// server STUN message handling
	void HandleServerMsg(StunMsg * pMsg, int type, int err, IOBuffer & buffer);
	void HandleReflexMsg(StunMsg * pMsg, int type);
	void HandleTurnAllocErrMsg(StunMsg * pMsg, int type);
	void HandleTurnCreatePermErrMsg(StunMsg * pMsg, int type);
	void HandleTurnDataIndication(StunMsg * pMsg, IOBuffer & buffer);
	void SendTurnAllocUDP();
	void SendTurnAllocUDPV4();
	void SendTurnAllocUDPV6();
	void SendTurnCreatePermission();
	void SendTurnRefresh();	// hmn, same as SendTurnBind()?
	void SendTurnBind();
	
	string & GetTurnKeyStr();	// returns the TURN key string
	uint32_t timeDiff();	// diff between now and startMs
	//
	// Peer STUN message handling
	void HandlePeerMsg(StunMsg * pMsg, int type, int err, bool isUsingRelay);
	void SendPeerBindMsg(Candidate * pCan, bool select, bool isUsingRelay);
	//
	// Message Sending
	//
	bool SendStunMsgTo(StunMsg * pMsg, string & ipStr);
	virtual bool SendToV4(uint32_t ip, uint16_t port, uint8_t * pData, int len);
	virtual bool SendTo(string ip, uint16_t port, uint8_t * pData, int len);
	bool SendDataToTurnCandidate(uint8_t * pData, int len, Candidate * pCan);
	bool SendData(StunMsg *pMsg, Candidate *pCan, bool useRelay);
	//
	// misc.
	//
	Candidate * FindCandidate(string ipStr);
	Candidate * FindCandidateByAddressAttr(StunMsg * pMsg);	// find using MAPPED_ADDRESS Attribute!
	Candidate * FindCandidateByTransId(uint8_t * pTxId);
	//
	bool DecRetryToZero(int & counter); // return true if --counter <= 0
	//
	string _username; // attribute for PEER BindingRequest STUN messages
	string & GetUserName();
#ifdef WEBRTC_DEBUG
	void Check();	// a test call
#endif
	/**
	 * Sets the best candidate properties.
	 *
     * @param pCan Candidate to set as best
     */
	void SetBestCandidate(Candidate *pCan);
	
	virtual void FinishInitialization();
 };

#endif // __BASEICEPROTOCOL_H
#endif //HAS_PROTOCOL_WEBRTC
