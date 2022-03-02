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
/**
* wrtcsigprotocol-h - basic Signaling Server Interface
* This module drives a given signaling server interface over TCP.
* It must be the only instance for that server (IP:PORT duple).
* The module first establishes a connection with the RRS Signal Server
* Then it accepts an offer for connection.
* With each offer it spawns a new WrtcConnection,
* and then spawns a new version of itself.
* Afterwards it handles the WebSocket traffic between
* WrtcConnection/Stun and RRS:signaling server.
* Note that the signaling protocol uses WebSocket!
**/
#ifdef HAS_PROTOCOL_WEBRTC

#ifndef __WRTCSIGPROTOCOL_H
#define __WRTCSIGPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/timer/basetimerprotocol.h"
#include "protocols/wrtc/wrtcappprotocolhandler.h"
#include "protocols/wrtc/candidate.h"
#include "protocols/wrtc/wrtcconnection.h"

class WrtcConnection;

#define MAX_WS_HDR_LEN 14  // 2(flag+len)+8(huge_len)+4(mask array)
#define MAX_HEARTBEAT_CHECK 11 // Check is every 5 seconds, we set it to 55 seconds

#define STUN_CREDENTIALS_TIMEOUT_BASE 20*60*60
#define STUN_CREDENTIALS_TIMEOUT_OFFSET (3*60*60)

class DLLEXP WrtcSigProtocol
: public BaseProtocol {
private:
	class HeartBeatCheckTimer
	: public BaseTimerProtocol {
	private:
		uint32_t _counter;
		WrtcSigProtocol* _wrtcSigProtocol;
	public:
		HeartBeatCheckTimer(WrtcSigProtocol* wrtcSigProtocol);
		virtual ~HeartBeatCheckTimer();
		virtual bool TimePeriodElapsed();
		void ResetCounter() { _counter = 0; };
	};

	class StunCredentialCheckTimer
        : public BaseTimerProtocol {
        private:
                WrtcSigProtocol* _wrtcSigProtocol;
        public:
                StunCredentialCheckTimer(WrtcSigProtocol* wrtcSigProtocol);
                virtual ~StunCredentialCheckTimer();
                virtual bool TimePeriodElapsed();
        };
public:
	WrtcSigProtocol();
	virtual ~WrtcSigProtocol();

	const string &GetioSocketId() const;
	const string &GetrrsIp() const;
	// sends which will wrap with appropriate JSON
	bool SendMessage(string msg);
	bool SendCommand(string cmd);
	bool SendSDP(string sdp, bool offer);
	bool SendCandidate(string & candidate);
	bool SendCandidate(Candidate * pCan);
	bool SendJoin();

	// sends this hunk of data out as a discrete WebSocket packet
	bool SendData(string & data, char * errStr);

	//
	// Inherited from BaseProtocol
	//
	virtual bool Initialize(Variant &parameters);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	// pure virtuals to satisfy
	virtual bool AllowFarProtocol(uint64_t type) { return true; };
	virtual bool AllowNearProtocol(uint64_t type) { return false; };
	//
	// over-ride to cause auto remove of WrtcConnection
	//virtual void EnqueueForDelete();

	virtual IOBuffer * GetOutputBuffer();
	IOBuffer _outBuffer;	// output data carrier fetches this

	void SetSdpMid(string &sdpMid);	// get this from offer SDP

	uint32_t GetConfigId() {return _configId;};
	void UnlinkConnection(WrtcConnection *connection, bool external = true);
	bool SendWebRTCCommand(Variant& settings);
	
	/**
	 * Tears down this signalling protocol.
	 * 
	 * @param isPermanent Indicates if no new instance is need to spawn
	 * @param closeWebrtc Indicates if linked webrtc session should be closed as well
	 */
	void Shutdown(bool isPermanent, bool closeWebrtc);

	void SetPeerState(PeerState peerState) { _peerState = peerState; }

	string GetPeerState();

	void SetIceState(IceState iceState) { _iceState = iceState; }

	string GetIceState();

	static uint32_t _activeClients;
	static uint32_t GetActiveClientsCount() { return _activeClients; }

private:
	uint32_t _configId;	// save configuration ID
	string _ioSocketId;
	int _wsNumReq;	// number of times we sent the WebSocket Request
	struct WS_HDR {	// "normalized" fields for WebSocket header
		bool fin;	//??
		int op;		// opcode
		int payloadLen;
		int headerLen;
		bool hasMask;
		char mask[4];
	};

	bool wsHandshakeDone;
	//
	// Connection Info
	string _rrsIp;
	uint16_t _rrsPortOrig;	// original port we connect to
	uint16_t _rrsPort;		// actual port we end up on
	string _roomId;
	string _rmsToken;
	string _rmsId;
	//
	// Client Info
	string _clientId;	// sent by RRS in clients message
	//
	// Ids from Candidate message
	void ParseCanIds(string & msg);	// parse & set: _sid, _myCanId, _hisCanId
	string _sid;	// session ID, 13 chars, I think we create this?
	string _myCanId;
	string _hisCanId;
	//
	// Co-Objects
	WrtcAppProtocolHandler * _pProtocolHandler;
	WrtcConnection * _pWrtc;

	bool _sendMasked;
	string _sdpMid;	// from SDP put in Candidate, must MATCH!
	
	bool _hasTurn; // indicates if a turn server is present

	bool _ipv6onlyflag;

	/* Valid Peer States enum { 0: "new", 1: "sent_offer", 2: "received_offer", 3: "sent_answer", 4: "received_answer", 5: "active", 6: "closed" } */
	PeerState _peerState;

	/* Valid Ice States enum { 0: "new", 1: "checking", 2: "connected", 3: "failed", 4: "disconnected", 5: "closed" } */
	IceState _iceState;
	
	// Spawn again - this is an internal call
	void ReSpawn(bool doKeepAlive, bool clientActivated);	

	HeartBeatCheckTimer* _pHBcheckTimer; // RRS heartbeat checker
	uint8_t _hb_count;
	time_t _hb_elapsedT;

	StunCredentialCheckTimer* _pSCcheckTimer; // Stun expiry checker

	//For User Agent
	bool _hasUserAgent;
	UserAgent _userAgent;

	//  camera adds index parameter in message for RRS while joining room so that RRS can be configured to cache and reuse the most recent websocket connection.
	uint64_t _rrsRoomIndex;

	uint32_t WS_GetPayload(IOBuffer & buf, string & msg);
	uint32_t WS_ParseHeader(uint8_t * pBuf, int len, WS_HDR & hdr);
	
	// WebSocket+IOSocket methods
	bool WS_SendRequest();
	bool WS_CheckResponse(IOBuffer & buf);
	//
	// Worker Routines
	bool SendRaw(string & str, char * errStr = (char *)"?");
	bool SendIOBuffer(char * errStr = (char *)"?");
	//
	// Misc.
	bool GetQuotedSubString(string & src, string match, string & res);
	bool GetJsonString(string & src, string key, string & res);

	// Connect to just received client ID from RRS
	void ConnectToClient(string msgReceived);
	
	/**
	 * Contains proper clean-up of the webrtc session
	 * 
	 * @param force true if webrtc is to be closed immediately
	 */
	void CleanUpWebrtc(bool force);
};

#endif // __WRTCSIGPROTOCOL_H
#endif // HAS_PROTOCOL_WEBRTC

