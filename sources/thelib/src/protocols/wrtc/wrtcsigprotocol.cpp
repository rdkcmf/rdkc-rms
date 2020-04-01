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

#define WRTC_CAPAB_STR "capabilities"

#include "protocols/wrtc/wrtcsigprotocol.h"
#include "protocols/baseprotocol.h"
#include "application/clientapplicationmanager.h"
#include "streaming/baseoutstream.h"
#include "protocols/wrtc/wrtcconnection.h"
#include "eventlogger/eventlogger.h"
#include "utils/misc/socketaddress.h"
#include "protocols/cli/basecliappprotocolhandler.h"
#include "mediaformats/readers/streammetadataresolver.h"
#include "utils/misc/timeprobemanager.h"

uint32_t WrtcSigProtocol::_rrsConnections = 0; // count the number of connections
const static string IPV6ONLYFLAGPATH = "/opt/rmsipv6only";

WrtcSigProtocol::HeartBeatCheckTimer::HeartBeatCheckTimer(WrtcSigProtocol* wrtcSigProtocol)
: BaseTimerProtocol() {
	_counter = 0;
	_wrtcSigProtocol = wrtcSigProtocol;
}

WrtcSigProtocol::HeartBeatCheckTimer::~HeartBeatCheckTimer() {
}

bool WrtcSigProtocol::HeartBeatCheckTimer::TimePeriodElapsed() {
	_counter++; // this increments every 10 seconds
	// If we are just about to restart, send the RRS a message
	if (_counter >= (MAX_HEARTBEAT_CHECK-1)) {		
		if (_wrtcSigProtocol) {
			// Send a message to the RRS to see if it receives it (debugging RDKC-2375)
			char * fmt = (char *)"5:1+::{\"name\":\"HBTimeout\",\"args\":[\"Heartbeat Time-out\",{\"RMS\":true}]}";
			string hbto(fmt);
			_wrtcSigProtocol->SendData(hbto, (char *)"Heartbeat Timeout");
			INFO("Sending HBTimeout message to RRS");
		}
	}
	if (_counter >= MAX_HEARTBEAT_CHECK) {
		WARN("RRS heartbeat timeout exceeded");
		// Restart since this is a timeout
		if (_wrtcSigProtocol) {
			WARN("Restarting RRS connection due to loss of heartbeat: %s,%s",STR(_wrtcSigProtocol->GetioSocketId()),STR(_wrtcSigProtocol->GetrrsIp()));
			_wrtcSigProtocol->Shutdown(false, false);
			_wrtcSigProtocol = NULL;
		}
	}
	return true;
}

WrtcSigProtocol::StunCredentialCheckTimer::StunCredentialCheckTimer(WrtcSigProtocol* wrtcSigProtocol)
: BaseTimerProtocol() {
        _wrtcSigProtocol = wrtcSigProtocol;
}

WrtcSigProtocol::StunCredentialCheckTimer::~StunCredentialCheckTimer() {
}

bool WrtcSigProtocol::StunCredentialCheckTimer::TimePeriodElapsed() {

	WARN("Stun credentials timer has expired");
	// Restart since this is a timeout
        if (_wrtcSigProtocol) {
		WARN("Restarting RRS connection due to expiry of STUN credentials: %s,%s",STR(_wrtcSigProtocol->GetioSocketId()),STR(_wrtcSigProtocol->GetrrsIp()));
		_wrtcSigProtocol->Shutdown(false, false);
		_wrtcSigProtocol = NULL;
	}
        return true;
}

WrtcSigProtocol::WrtcSigProtocol()
: BaseProtocol(PT_WRTC_SIG) {
    _pProtocolHandler = NULL;
	wsHandshakeDone = false;
	_rrsPortOrig = 0;
	_rrsPort = 0;
	_sendMasked = true;
	_wsNumReq = 0;
	_pWrtc = NULL;
	_sdpMid = "data";	// this must match the SDP!!
	_configId = 0;	// retrieve from parameters
	_rrsConnections++;
	_hasTurn = false;
	_ipv6onlyflag = false;
	
	// Initialize the strings
	_rmsId = "";
	_clientId = "";
	_sid = "";
	_myCanId = "";
	_hisCanId = "";

	_pHBcheckTimer = NULL;
	_hb_count = 0;
	_hb_elapsedT = time(NULL);

	_pSCcheckTimer = NULL;
	srand((unsigned)time(0));

	_hasUserAgent = false;

	_rrsRoomIndex = 0;

	/* Initial states for both peer and ice */
	_peerState = WRTC_PEER_NEW;
	_iceState = WRTC_ICE_NEW;

	if ( fileExists(IPV6ONLYFLAGPATH) ) {
		_ipv6onlyflag = true;
		INFO("WrtcSigProtocol construct - ipv6only flag is set");
	}
}

WrtcSigProtocol::~WrtcSigProtocol() {
//#ifdef WEBRTC_DEBUG
	FINE("WrtcSigProtocol deleted (%s)", STR(_clientId));
//#endif
	
	if (_pHBcheckTimer) {
		_pHBcheckTimer->EnqueueForDelete();
		_pHBcheckTimer = NULL;
	}

	if (_pSCcheckTimer) {
		INFO("Deleting Stun credentials timer as webrtc protocol is destroyed");
                _pSCcheckTimer->EnqueueForDelete();
                _pSCcheckTimer = NULL;
    }

	_ipv6onlyflag = false;

	// Do a proper clean-up
	CleanUpWebrtc(false);
}

// Initialize()
// is called by the framework when the protocol is standing up
//
bool WrtcSigProtocol::Initialize(Variant &parameters) {
//#ifdef WEBRTC_DEBUG
	FINE("WrtcSigProtocol::Initialize, parms=%s", STR(parameters.ToString()));
//#endif
    _customParameters = parameters;
	string ip;
	uint16_t port = 0;
	
    if (parameters.HasKey("rrsip", false)) {
    	ip = (string) parameters.GetValue("rrsip", false);
		_rrsIp = ip;
    }
	
    if (parameters.HasKey("rrsport", false)) {
    	port = (uint16_t) parameters.GetValue("rrsport", false);
		_rrsPortOrig = port;
    }
	
    if (parameters.HasKey("roomId", false)) {
    	_roomId = (string) parameters.GetValue("roomId", false);
    } else {
		FATAL("WrtcSigProtocol needs roomId parameter!");
		return false;
	}
	
    if (parameters.HasKey("configId", false)) {
    	_configId = (uint32_t) parameters.GetValue("configId", false);
    } else {
    	FATAL("WebRTC Sig Protocol failed to find 'configId' key!");
		//TODO: shouldn't this return immediately then?
    }
	
	if (parameters.HasKey("token", false)) {
    	_rmsToken = (string) parameters.GetValue("token", false);
    }

	// Start the timer for heartbeat check, 10-second interval
	_pHBcheckTimer = new HeartBeatCheckTimer(this);
	_pHBcheckTimer->EnqueueForTimeEvent(5);

	// Start the timer for stun credential check, interval has to be random and in the range of 20-23 hours
	_pSCcheckTimer = new StunCredentialCheckTimer(this);
	uint32_t timeout = STUN_CREDENTIALS_TIMEOUT_BASE + (rand() % STUN_CREDENTIALS_TIMEOUT_OFFSET);
	INFO("Set timer for STUN credentials expiry. timeout value is %d!!!", timeout);
	_pSCcheckTimer->EnqueueForTimeEvent(timeout);
	
    INFO("Started WebRTCSigProtocol[%d], %s:%d, %s",
    		(int)_configId, STR(ip), (int)port, STR(_roomId));

    return true;
}

void WrtcSigProtocol::ReSpawn(bool doKeepAlive, bool clientActivated) {
	// Get the handler of this protocol
	WrtcAppProtocolHandler *handler = (WrtcAppProtocolHandler *) (GetApplication()->GetProtocolHandler(PT_WRTC_SIG));
	
	// Sanity check
	if (handler == NULL) {
		return;
	}
	
	Variant copy = _customParameters;
	if (clientActivated) {
		copy["clientActivated"] = bool(true);
	}
	
	if (!doKeepAlive) {
		copy["keepAlive"] = bool(false);
	}
	// Start another instance
//	handler->StartWebRTC(_customParameters);
	handler->StartWebRTC(copy, false);
}

bool WrtcSigProtocol::SignalInputData(int32_t recvAmount) {
    FATAL("OPERATION NOT SUPPORTED");
    return false;
}

bool WrtcSigProtocol::SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress) {
	FATAL("Somehow unexpected SignalInputData(int) Called!");
	return false;
}

bool WrtcSigProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	if (!_rrsPort) {
		_rrsPort = pPeerAddress->getSockPort();
	}
	return SignalInputData(buffer);
}

/*!
 * SignalInputData() receives data from the carrier.
 * We first vector off to complete the WebSocket Handshake.
 * After that presumably one-time event,
 * we have WebSocket traffic which is JSON Wrapped.
 * The traffic looks like one of the following:
 * 1:::
 * 5:::{"name":"stunservers","args":[[{"url":"stun:162.209.96.37:55555"}]]}
 * 5:::{"name":"turnservers","args":[[{"username":"1432765254","credential":"TG1nW/f9dg7y/yPp9ESRK/leIoc=","url":"turn:162.209.96.37:55555"}]]}
 * 6:::1+[null,{"clients":{"gb-R19A5ADPGZGlhzW2T":{"screen":false,"video":true,"audio":false}}}]
 * 5:::{"name":"message","args":[{"to":"gb-R19A5ADPGZGlhzW2T","sid":"1432678845741","roomType":"video","type":"offer","payload":{"type":"offer","sdp":"v=0\r\no=- 792031585238705889 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r] ...
 * 5:::{"name":"message","args":[{"to":"WOjhulRyTezf7uHYzW2V","sid":"1432678845741","roomType":"video","type":"answer","payload":{"type":"answer","sdp":"v=0\r\no=- 6319748034383864533 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\na=
 * 5:::{"name":"message","args":[{"to":"WOjhulRyTezf7uHYzW2V","sid":"1432678845741","roomType":"video","type":"candidate","payload":{"candidate":{"candidate":"candidate:2952909766 1 udp 2122262783 2600:1012:b118:fdb4:0:2:fbb
 * We quick-parse that JSON to type, then abstract the strings WrtcConnection will use
 */
bool WrtcSigProtocol::SignalInputData(IOBuffer &buffer) {
	// vector off to handle the WebSocket handshake if not done
	if (!wsHandshakeDone) {
		bool ok = WS_CheckResponse(buffer);
		if (!wsHandshakeDone) {
			return ok;
		}// otherwise fall through with remaining data
	}
	//
	// Create a WrtcConnection if we need it
	if (!_pWrtc) {
		_pWrtc = new WrtcConnection(this, _customParameters);
		
		// Set the application
		_pWrtc->SetApplication(GetApplication());
	}
	//
	// loop this to get multiple WebSocket messages
	while (GETAVAILABLEBYTESCOUNT(buffer)) {
		string msg;
		uint32_t usedLen = WS_GetPayload(buffer, msg);
		if (!usedLen) {
			FATAL("WrtcSigProtocol: failed to parse WebSocket header");
			// $ToDo$ make usedLen an integer and distiguish error (-1) from need more bytes (0)
			// for now, assume we just need more bytes
			// buffer.IgnoreAll();
			return true;	// not really fatal to our stack, just ignore it
		}
		//INFO("$b2$ WebSocket data[%d]: %s", (int)msg.size(), STR(msg));
		//INFO("$b2$ Buffer length = %d", (int)GETAVAILABLEBYTESCOUNT(buffer));

		// parse expected messages
		//
		// STUN Server - grab the url and send it to Wrtc
		// * 5:::{"name":"stunservers","args":[[{"url":"stun:162.209.96.37:55555"}]]}
		if (msg.find("stunservers")!= string::npos) {
			string ipPort;
			GetQuotedSubString(msg, "stun:", ipPort);
			INFO("Got stunserver IP Port string: %s", STR(ipPort));

			if (!(ipPort.empty())) {
				_pWrtc->SetStunServer(ipPort);
			}
		}
		// TURN Servver - grab username, credential, and url and send to Wrtc
		// 5:::{"name":"turnservers","args":[[{"username":"1432765254","credential":"TG1nW/f9dg7y/yPp9ESRK/leIoc=","url":"turn:162.209.96.37:55555"}]]}
		else if (msg.find("turnservers") != string::npos) {
			// parse out the 3 pieces we need for turnservver
			string username;
			GetJsonString(msg, "username", username);
			//
			string credential;
			GetJsonString(msg, "credential", credential);
			//
			string ipPort;
			GetQuotedSubString(msg, "turn:", ipPort);
			//
			INFO("Got Turn Server IP Port string: %s", STR(ipPort));
//#ifdef WEBRTC_DEBUG
			DEBUG("Parsed Turnserver: {user:%s, cred:%s, addr:%s}",
				STR(username), STR(credential), STR(ipPort));
//#endif
			// Check if we have indeed a TURN server
			if (!(ipPort.empty())) {
				_pWrtc->SetTurnServer(username, credential, ipPort);
				_hasTurn = true;
			} else {
				_hasTurn = false;
			}
		}
		else if (msg.find("ersid") != string::npos) {
			//insert event
			string rrsid;
			GetJsonString(msg, "id", rrsid);

			Variant params;
			params = _customParameters;
			params["ersid"] = rrsid;
			GetEventLogger()->LogWebRtcServiceConnected(params);

			// next we Join
			SendJoin();
		}
		// Client ID Spec - just grab the Client ID
		// 6:::1+[null,{"clients":{"gb-R19A5ADPGZGlhzW2T":{"screen":false,"video":true,"audio":false}}}]
		else if (msg.find("clients") != string::npos) {
//#ifdef WEBRTC_DEBUG
			DEBUG("Got Clients Msg: %s", STR(msg));
//#endif
			// danger! the following luckily gets that first client!!
			string clientId = "";
			GetJsonString(msg, "clients", clientId);

			if (!(clientId.empty())) {
//#ifdef WEBRTC_DEBUG
				DEBUG("We have a client. Connecting...");
//#endif
				// Sanity check, compare id with existing
				if (_clientId.empty() || (_clientId != clientId)) {
					_clientId = clientId;
					INFO("Connecting with %s", STR(_clientId));
					ConnectToClient(msg); // connect to this client ID
				} else {
					WARN("Existing connection with %s exists.", STR(_clientId));
				}
			}
			else {
				INFO("No connected client. Waiting...");
			}
		}
		// SDP Offer
		// 5:::{"name":"message","args":[{"to":"gb-R19A5ADPGZGlhzW2T","sid":"1432678845741","roomType":"video","type":"offer","payload":{"type":"offer","sdp":"v=0\r\no=- 792031585238705889 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r] ...
		else if (msg.find("offer") != string::npos) {
			ParseCanIds(msg);	// parse & set: _sid, _myCanId, _hisCanId

			INFO("Peer State Transition: %s-received_offer", STR(GetPeerState()));
			_peerState = WRTC_PEER_RECEIVED_OFFER;
//#ifdef WEBRTC_DEBUG
			DEBUG("Got SDP Offer, %s", STR(msg));
			DEBUG("Did respawn the SigProtocol after seeing SDP offer!!");
//#endif
			INFO("Got SDP Offer, %s... Respawning sigprotocol", STR(msg));
			ReSpawn(false, true);
			
			string sdp;
			if (GetJsonString(msg, "sdp", sdp)) {
				_pWrtc->SetSDP(sdp, true); // true=offer
			}
		}
		else if (msg.find("answer") != string::npos) {
			ParseCanIds(msg);	// parse & set: _sid, _myCanId, _hisCanId

			INFO("Peer State Transition: %s-received_answer", STR(GetPeerState()));
			_peerState = WRTC_PEER_RECEIVED_ANSWER;
//#ifdef WEBRTC_DEBUG
			DEBUG("Got SDP Answer, %s", STR(msg));
//#endif
			string sdp;
			if (GetJsonString(msg, "sdp", sdp)) {
				_pWrtc->SetSDP(sdp, false); // false=answer
			}
		}
		// ICE Candidate
		// 5:::{"name":"message","args":[{"to":"WOjhulRyTezf7uHYzW2V","sid":"1432678845741","roomType":"video","type":"candidate","payload":{"candidate":{"candidate":"candidate:2952909766 1 udp 2122262783 2600:1012:b118:fdb4:0:2:fbb
		else if (msg.find("candidate") != string::npos) {

			INFO("Got Candidate Msg: %s", STR(msg));
			if (_iceState == WRTC_ICE_NEW) {
				INFO("Ice State Transition: %s-checking", STR(GetIceState()));
				_iceState = WRTC_ICE_CHECKING;
			}

			// Prior to processing candidate, make sure we are started. this will also reset the fastTick timers
			_pWrtc->Start(true);

			ParseCanIds(msg);// parse & set: _sid, _myCanId, _hisCanId

			// now find the candidate string
			string canStr;
			if (GetQuotedSubString(msg, "candidate:", canStr)) {
				// make a Candidate object and pass it to our connection
//#ifdef WEBRTC_DEBUG
				INFO("Got Candidate: %s", STR(canStr));
//#endif
				Candidate * pCan = new Candidate();
				pCan->Set(canStr);

				if ( (! pCan->IsIpv6() ) &&  _ipv6onlyflag ) { //if candidate is not ipv6 then check the flag if we need to drop
					INFO("_ipv6onlyflag is set - dropping incoming ipv4 candidate");
					//REVIEW This was deleted as fix
					delete pCan;
					pCan = NULL;
				}
				else if (!pCan->IsUdp()) {
//#ifdef WEBRTC_DEBUG
					DEBUG("WrtcTossing non-UDP Candidate!");
//#endif
					delete pCan;
					pCan = NULL;
				} else {
					_pWrtc->SetCandidate(pCan);
				}

				if (pCan != NULL) {
					// Clean-up
					delete pCan;
				}
			}
		}
		else if (msg.find("command") != string::npos) {
			// Command message from RRS
			
			// Extract the payload
			uint32_t pos = (uint32_t) msg.find("payload");
			pos += 9; // skip to first '{'
			
			// Convert to variant
			Variant var;
			if (Variant::DeserializeFromJSON(msg, var, pos)) {
				string command = (string) var["command"];
				FINE("Command received: %s - [%s]", STR(command), STR(var["argument"]));
				FINEST("Message received from RRS: %s", STR(var.ToString()));
				GetEventLogger()->LogWebRTCCommandReceived(var);
				if (command == "Pause") {
					_pWrtc->HandlePauseCommand();
				} else if (command == "Resume") {
					_pWrtc->HandleResumeCommand();
				}
			} else {
				FATAL("Unable to parse command payload!");
			}
		}
		//5:::{"name":"remove","args"::"7hnLKbP9ORCY_CnLzXFz"}]}
		else if (msg.find("remove") != string::npos) {
//#ifdef WEBRTC_DEBUG
			INFO("Wrtc Client Removed (left)");
//#endif
			// We want the shutdown to be not permanent because we want to make
			// sure that we always want to check if there is still an existing
			// connection with RRS
			Shutdown(false, true);
		}
		else if (msg.find("taken") != string::npos) {
			FATAL("Room name is already in use: %s", STR(_roomId));
			Shutdown(true, true);
		} else if (msg.find("invalidRoom") != string::npos) {
			FATAL("Room name (%s) is flagged as invalid!", STR(_roomId));
			// Shutdown permanently
			GetCustomParameters()["keepAlive"] = bool(false);
			Shutdown(true, true);
		} else if (msg.find("invalidRmsToken") != string::npos) {
			FATAL("Token (%s) is flagged as invalid!", STR(_rmsToken));
			// Shutdown temporarily until token gets added from RRS POV
			//GetCustomParameters()["keepAlive"] = bool(false); //turned setting of keepAlive off since this shutdown is only temporary
			Shutdown(true, true);
		}
		// 5:::{"name":"clientJoined","args":[{"id":"iddajhdjahdjas", "streamName":"streamname", "cwa":"true"}]}
		else if (msg.find("clientJoined") != string::npos) {
			PROBE_CLEAR_TIME("wrtcplayback");
			INFO("Client joined!");
			INFO("Roomid:%s", STR(_roomId));
			PROBE_POINT("webrtc", "sess1", "client_join", false);
			// Get the client id
			string clientId = "";
			if (GetJsonString(msg, "id", clientId)) {
				INFO("Player Client ID:%s", STR(clientId));
				_pWrtc->SetClientId(clientId);
			}
			string rmsId = "";
			if (GetJsonString(msg, "rmsid", rmsId)) {
				INFO("RMS Client ID:%s", STR(rmsId));
				_pWrtc->SetRMSClientId(rmsId);
			}

			if (_rmsId.empty()) {
				_rmsId = rmsId;
			}

			// Get the set of capabilities
			string capabilities = "";
			if (GetJsonString(msg, WRTC_CAPAB_STR, capabilities)) {
				INFO("Capabilities: %s", STR(capabilities));
				_pWrtc->SetPeerCapabilities(capabilities);
			}

			// Sanity check, compare id with existing
			if (_clientId.empty() || (_clientId != clientId)) {
				// Only do this if we have no existing connections
				_clientId = clientId;
				if (_pSCcheckTimer) {
					INFO("Deleting Stun credentials timer as client has joined");
					_pSCcheckTimer->EnqueueForDelete();
					_pSCcheckTimer = NULL;
				}
				ConnectToClient(msg); // connect to this client ID
			} else {
				WARN("Existing connection with %s exists.", STR(_clientId));
			}
		}
		else if (msg.find("iceparam") != string::npos) {
//			FATAL("iceparam=%s", STR(msg));
			string strMaxRetries;
			if (GetJsonString(msg, "maxretries", strMaxRetries)) {
				if (strspn(STR(strMaxRetries), "0123456789") == strMaxRetries.size()) {
					int nMaxRetries = atoi(STR(strMaxRetries));
					if (nMaxRetries > 0) {
						if (_pWrtc) {
							INFO("Overriding default 'maxRetries' :%d", nMaxRetries);
							_pWrtc->OverrideIceMaxRetries(nMaxRetries);
						} else {
							// We shouldn't reach this point!
							FATAL("A 'maxRetries' request was received, but _pWrtc is NULL!");
						}
					} else {
						WARN("Parameter 'maxretries' cannot be 0");
					}
				} else {
					FATAL("Invalid 'maxretries' parameter = %s", STR(strMaxRetries));
				}
			}
			else {
				WARN("Unknown parameter received for 'iceparam' event : %s", STR(msg));
			}
		}
		// ?????
		else if (msg.find("1::") == 0) {
//#ifdef WEBRTC_DEBUG
			INFO("Received IO Socket Connect");
//#endif
		}
		else if (msg.find("0::") == 0) {
//#ifdef WEBRTC_DEBUG
			INFO("Received IO Socket Dis-Connect");
//#endif
			// Even if RRS gets somehow disconnected, we won't do a clean-up
			// of wrtc connection. But the RRS class itself needs to be torn-down
			Shutdown(false, false);
		}
		else if (msg.find("validateFailed") != string::npos) {
			// FOR COMCAST!!!!
			// Token failure is a non-recoverable situation for them, so we will 
			// shutdown completely so they can reset their configs
			static uint8_t TOKENFAILCOUNT=0;
			static const uint8_t MAXTOKENFAILCOUNT=25;
			TOKENFAILCOUNT++;
			if( TOKENFAILCOUNT < MAXTOKENFAILCOUNT ) {
				INFO("Received validateFailed. Will shut the server down if it is recieved %"PRIu8" more times", (uint8_t)(TOKENFAILCOUNT-MAXTOKENFAILCOUNT));
				Shutdown(true, true);
			}
			else {
				FATAL("Exceeded the limit of token validation failures, shutting the entire process down");
				EventLogger::SignalShutdown();
			}
		}
		//TODO: hearbeat handling should be on the first of this huge if-else to minimize
		// processing
		else if (msg.find("2::") == 0) {
//#ifdef WEBRTC_DEBUG
			DEBUG("Received IO Socket Heartbeat, responding");
//#endif
			SendData(msg, (char*)"Echoing IOSocket Heartbeat");
			_pHBcheckTimer->ResetCounter();
			// for comcast diagnostics. after 5 min, print out elapsed time and number of heartbeats
			_hb_count++;
			time_t elapsed = time(NULL) - _hb_elapsedT;
			if( elapsed >= 55 ) { // 55 seconds
				INFO("Heartbeat Status:%d,%d,%s,%s,%"PRIu64"", (int)elapsed, _hb_count, STR(_ioSocketId), STR(_rrsIp), _rrsRoomIndex);
				_hb_count = 0;
				_hb_elapsedT = time(NULL);
			}

		}
		else {
			WARN("WrtcSigProtocol: Unexpected message: %s", STR(msg));
		}

		buffer.Ignore((uint32_t) usedLen);
	}
	return true;
}

void WrtcSigProtocol::ConnectToClient(string msgReceived) {
	// Check if we did manage to find the stream
	bool isStreamAvailable = false;

	Variant payload;
		payload["roomName"] = _roomId;
		string clientName;
		if (GetJsonString(msgReceived, "name", clientName)) {
			payload["clientName"] = clientName;
		}
		string clientID;
		if (GetJsonString(msgReceived, "id", clientID)) {
			payload["clientID"] = clientID;
	}
	GetApplication()->GetEventLogger()->LogPlayerJoinedRoom(payload);

	// codec-workaround
	string tmp;
	if (GetJsonString(msgReceived, "cwa", tmp)) {
		INFO("Codec workaround");
		_pWrtc->GetCustomParameters()["codec-workaround"] = tmp;
	}

	// Check what transport is being requested
	bool useSrtp = false;
	string transport;
	if (GetJsonString(msgReceived, "transport", transport)) {
		INFO("Transport: %s", STR(transport));
		if (strcmp("srtp", STR(transport)) == 0) {
			useSrtp = true;
		}
	} else {
		DEBUG("No transport field, default to fmp4.");
	}

	// Get the streamName
	string streamName;
	if (GetJsonString(msgReceived, "streamName", streamName)) {
		INFO("Stream name: %s", STR(streamName));
		string rsn = lowerCase(streamName);
		bool isPlayList = (rsn.rfind(".lst") == rsn.length() - 4);
		bool isLazyPullVod = isPlayList ? false : (rsn.rfind(".vod") ==
			rsn.length() - 4);
		if (!useSrtp && (isPlayList || isLazyPullVod)) {
			//Start now. We will setup ssp/lzp later, so we do not stream early 
			//and lose start of video.
			_pWrtc->Start();
			isStreamAvailable = true;
		} else if (useSrtp) {
			if (_pWrtc->SetStreamName(streamName, useSrtp)) {
				if (!_pWrtc->IsWaiting()) {
					_pWrtc->Start();
					isStreamAvailable = true;
				}
/*			} else {
				if (isPlayList) {
					if (_pWrtc->SetupPlaylist(streamName)) {
						_pWrtc->Start();
					}
				} else if (isLazyPullVod) {
					if (_pWrtc->SetupLazyPull(streamName)) {
						_pWrtc->Start();
					}
				}*/
			} else {
				FATAL("Unable to retrieve stream!");
				SendMessage("Unable to retrieve stream!");
			}
		} else {
			if (!_pWrtc->SetStreamName(streamName, useSrtp)) {
				FATAL("Unable to retrieve stream!");
				SendMessage("Unable to retrieve stream!");
			} else {
				_pWrtc->Start();
				isStreamAvailable = true;
			}
		}
	} else {
		FATAL("Unable to parse stream field!");
		SendMessage("Unable to parse stream field!");
	}
	
	if (isStreamAvailable) {
		// Only at this point do we respawn
		// Since we have started the connection, we have to respawn
		// TODO: maintain a single connection to RRS, there should be no need to respawn
		INFO("Connecting to client, Respawning the sig protocol");
		ReSpawn(false, true);
	} else {
		WARN("Stream setup failed.");
		_clientId = ""; // reset this so that reconnection will be allowed
	}
}

void WrtcSigProtocol::ParseCanIds(string & msg) {
	// parse out the session id, along with to (myID) and from (hisId)
	// do nothing if we've already done it!
	// parse _sid if we don't have one yet
	if (_sid.empty()) { // grab the session id
		GetJsonString(msg, "sid", _sid);
	}
	if (_myCanId.empty()) {
		GetJsonString(msg, "to", _myCanId);
	}
	if (_hisCanId.empty()) {
		GetJsonString(msg, "from", _hisCanId);
//#ifdef WEBRTC_DEBUG
		DEBUG("Client IDs: me: %s, him: %s", STR(_myCanId), STR(_hisCanId));
//#endif
	}
}

// GetQuotedSubString()
// match needs to include the opening quote
// to get value in:  "key:value" call with match="key:"
// to get value in:  "key":"value" call with match="key\":\""
// -- or call GetJsonString(msg, "key", result);
bool WrtcSigProtocol::GetQuotedSubString(string & msg, string match, string & res) {
	size_t i, j;
	res = "";
	//INFO("$b2$ find: %s in %s", STR(match), STR(msg));
	i = msg.find(match);
	if (i != string::npos) {
		i += match.size();	// jump to first char after the string
		//j = msg.find("\"", i);	// find the trailing quote
		size_t z = msg.size();
		for (j = i; j < z; j++) {
			if (msg[j] == '"') break;
		}
		//INFO("$b2$ find: i=%d, j=%d, z=%d", (int)i, int(j), int(z));
		if (j != string::npos) {
			res = msg.substr(i, j-i);
		}
	}

	return res.size() > 0;
}

// GetJsonString()
// to get value in:  "key":"value" 
// it returns res = the string: value (without the quotes)
// danger: we count on this also working with:  "key":{"value".... (brace) (search danger above)
bool WrtcSigProtocol::GetJsonString(string & msg, string key, string & res) {
	size_t i, j;
	res = "";
	string match = "\"" + key + "\"";
	i = msg.find(match);
	if (i == string::npos) return false;
	i += match.size();	// jump to first char after the string
						// this is probably the ':'
	i = msg.find("\"", i);	// find the opening quote
	if (i == string::npos) return false;
	i++; // go past the quote
	j = msg.find("\"", i);	// find the closing quote
	if (j == string::npos) return false;
	res = msg.substr(i, j - i);	// string from char at i to before j

	return res.size() > 0;
}

//TODO: We really need to refactor this and have a more elegant RRS handler
bool WrtcSigProtocol::SendMessage(string msg) {
	static char *fmt = (char *)"5:::{\"name\":\"message\",\"args\":[{\"to\":\"%s\",\"message\":\"%s\",\"prefix\":\"webkit\"}]}";
	string cid = _clientId.empty() ? _hisCanId : _clientId;
	string fmtMsg = format(fmt, STR(cid), STR(msg));

	return SendData(fmtMsg, (char *)"Outgoing Message");
}

bool WrtcSigProtocol::SendCommand(string cmd) {
	static char *fmt = (char *)"5:::{\"name\":\"message\",\"args\":[{\"to\":\"%s\",\"type\":\"command\",\"payload\":%s,\"prefix\":\"webkit\"}]}";
	string cid = _clientId.empty() ? _hisCanId : _clientId;
	string fmtMsg = format(fmt, STR(cid), STR(cmd));

	return SendData(fmtMsg, (char *)"Outgoing Command");
}

bool WrtcSigProtocol::SendSDP(string sdp, bool offer) {
	// client id & _sid are formatted into this first string
	if (_sid.empty()) {
		_sid = "1432678845741";
	}
	static char * fmt1 = (char *)"5:::{\"name\":\"message\",\"args\":[{\"to\":\"%s\",\"sid\":\"%s\",\"roomType\":\"video\",\"type\":\"%s\",\"payload\":{\"type\":\"%s\",\"sdp\":\"";
	// sdp goes in between these 2 formats
	static string fmt2 = "\"},\"prefix\":\"webkit\"}]}";
	char * offerAnswer = offer ? (char *)"offer" : (char *)"answer";
	string cid = _clientId.empty() ? _hisCanId : _clientId;
	string sdpMsg = format(fmt1, STR(cid), STR(_sid), offerAnswer, offerAnswer);
	sdpMsg += sdp;
	sdpMsg += fmt2;

	if (offer) {
		INFO("Peer State Transition: %s-sent_offer", STR(GetPeerState()));
		_peerState = WRTC_PEER_SENT_OFFER;
	}
	else {
		INFO("Peer State Transition: %s-sent_answer", STR(GetPeerState()));
		_peerState =  WRTC_PEER_SENT_ANSWER;
	}

	//offer ? _peerState = WRTC_PEER_SENT_OFFER : _peerState =  WRTC_PEER_SENT_ANSWER;

	return SendData(sdpMsg, (char *)"Outgoing SDP");
}

bool WrtcSigProtocol::SendCandidate(Candidate * pCan) {
	// Sanity check, this instance could have been scheduled to be deleted already
	if (IsEnqueueForDelete()) {
//#ifdef WEBRTC_DEBUG
		DEBUG("Signalling protocol is being torn down.");
//#endif
		return false;
	}


	// sugar call, we send out the string produced by the Candidate object
	string canStr;
	pCan->Get(canStr);
	return SendCandidate(canStr);
}

bool WrtcSigProtocol::SendCandidate(string & candidate) {
	// Check if session ID is still empty, if it is, initialize it
	if (_sid.empty()) {
		_sid = "1432678845741";
	}
	static char * fmt1 = (char *)"5:::{\"name\":\"message\",\"args\":[{\"to\":\"%s\",\"sid\":\"%s\",\"roomType\":\"video\",\"type\":\"candidate\",\"payload\":{\"candidate\":{\"candidate\":\"candidate:";
	// candidate gets stuffed in between
	static string fmt2 = "\",\"sdpMid\":\"";
	// _sdpMid gets stuffed here (pseudo %s)
	static string fmt3 = "\",\"sdpMLineIndex\":0}},\"prefix\":\"webkit\"}]}";

	// Sanity checks
	if (_hisCanId.empty() || _sdpMid.empty() || candidate.empty()) {
//#ifdef WEBRTC_DEBUG
		DEBUG("Remote and/or local candidate is invalid. SDP mid field may also be invalid");
//#endif
		return false;
	}
	
	string canMsg = format(fmt1, STR(_hisCanId), STR(_sid));
	canMsg += candidate;
	canMsg += fmt2;
	canMsg += _sdpMid;
	canMsg += fmt3;
//#ifdef WEBRTC_DEBUG
	INFO("Send Candidate: %s", STR(canMsg));
//#endif

	//UPDATE: for every candidate we send out, we restart the counter
	if (_pWrtc && _pWrtc->HasStarted()) {
		_pWrtc->Start(true);
	}
	
	return SendData(canMsg, (char *)"Outgoing Candidate");
}

bool WrtcSigProtocol::SendJoin() {
	string capabilities = "";
	
#ifdef WRTC_CAPAB_HAS_HEARTBEAT
	capabilities += "hasHeartBeat";
#endif // WRTC_CAPAB_HAS_HEARTBEAT

	_rrsRoomIndex = (uint64_t)getutctime();  // generate unique and incremental index parameter
	
	// This is our Join Room string
	char * fmt = (char *)"5:1+::{\"name\":\"join\",\"args\":[\"%s\",{\"index\":%"PRIu64",\"RMS\":true,\"rmsToken\":\"%s\",\"capabilities\":\"%s\"}]}";
	string join = format(fmt, STR(_roomId), _rrsRoomIndex, STR(_rmsToken), STR(capabilities));

	INFO("RMS joining the RRS with index parameter as %"PRIu64"", _rrsRoomIndex);

	return SendData(join, (char *)"Join Room");
}

bool WrtcSigProtocol::SendData(string & data, char * errStr) {
	//INFO("SendData[%d]:%s,", (int)data.size(), STR(data));
	// create websocket header first 
	// then mask data if flag set
	// then stuff them (hdr and data) straight into _outBuffer
	uint8_t hdr[MAX_WS_HDR_LEN];
	int hdrLen;
	size_t dataLen = data.size();
	hdr[0] = 0x81;	// FIN & Text Opcode - right???
	if (dataLen < 126) {
		hdrLen = 2 + (_sendMasked ? 4 : 0);
		//INFO("$b2 - %d byte header [%d]", hdrLen, (int)dataLen);
		hdr[1] = 0x80 + (uint8_t)dataLen;
	}else if (dataLen <= 65535) {// 16 bit max len
		hdrLen = 4 + (_sendMasked ? 4 : 0);
		//INFO("$b2 -- %d byte header [%d]", hdrLen, (int)dataLen);
		hdr[1] = 0x80 | 126;
		hdr[2] = 0xFF & (dataLen >> 8);
		hdr[3] = 0xFF & dataLen;
	} else {
		hdrLen = 10 + (_sendMasked ? 4 : 0);
		//INFO("$b2 --- %d byte header [%d]", hdrLen, (int)dataLen);
		hdr[1] = 0x80 + 127;
		hdr[2] = 0; hdr[3] = 0;	// just support 32bit lengths!
		hdr[4] = 0; hdr[5] = 0;
		hdr[6] = 0xFF & (dataLen >> 24);
		hdr[7] = 0xFF & (dataLen >> 16);
		hdr[8] = 0xFF & (dataLen >> 8);
		hdr[9] = 0xFF & dataLen;
	}
	// should make send masked optional I guess??
	uint32_t maskNum = 0xFFFFFFFF;	// $ToDo$ make the mask random
	uint8_t mask[4];
	if (!_sendMasked) { // zero it because we always xor below
		for (int z = 0; z < 4; z++) {
			mask[z] = 0;
		}
	} else { // set our mask and mask within header
		size_t i = hdrLen - 4;
		mask[0] = hdr[i] = 0xFF & (maskNum >> 24);
		mask[1] = hdr[i + 1] = 0xFF & (maskNum >> 16);
		mask[2] = hdr[i + 2] = 0xFF & (maskNum >> 8);
		mask[3] = hdr[i + 3] = 0xFF & (maskNum);
	}

	//INFO("$b2$ Hdr[%d] Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X", hdrLen,
	//	(int)hdr[0], (int)hdr[1], (int)hdr[2], (int)hdr[3],
	//	(int)hdr[4], (int)hdr[5], (int)hdr[6], (int)hdr[7]
	//	);
	//INFO("$b2$ Mask Bytes are: %2X,%2X,%2X,%2X",
	//	(int)mask[0], (int)mask[1], (int)mask[2], (int)mask[3]
	//	);
	int j = 0;
	//
	// now mask the data
	string dataStr;
	j = 0;
	for (size_t i = 0; i < dataLen; i++) {
		dataStr += data[i] ^ mask[j++];
		if (j > 3) j = 0;
	}
	// stuff the output buffer
	_outBuffer.ReadFromBuffer(hdr, hdrLen);
	//INFO("Stuffed WS Header: [%d, %d]", hdrLen, GETAVAILABLEBYTESCOUNT(_outBuffer));
	_outBuffer.ReadFromString(dataStr);
	//INFO("Stuffed WS Data: [%d, %d]", 
	//	(int) dataStr.size(), GETAVAILABLEBYTESCOUNT(_outBuffer));
	SendIOBuffer(errStr);

	return true;
}

//get io sicket id
const string &WrtcSigProtocol::GetioSocketId() const {
	return _ioSocketId;
}

//get rrs ip
const string &WrtcSigProtocol::GetrrsIp() const {
	return _rrsIp;
}

/*
 * Our RRS connection uses IOSocket over WebSocket.
 * This requires us to request twice, 
 *     - first passing in the time as a parm
 *     - second passing in the key returned in the first response
 * We use a counter: _wsNumReq, modulo 2, to pace this
 */

bool WrtcSigProtocol::WS_SendRequest() {

	if (!_hasUserAgent) {
		WrtcAppProtocolHandler *handler = (WrtcAppProtocolHandler *) (GetApplication()->GetProtocolHandler(PT_WRTC_SIG));
		if (NULL != handler) {
			if (handler->GetUserAgent(_userAgent)) {
				INFO("User Agent forRMS->RRS connection is %s %s %s %s %s",
					STR(_userAgent.deviceMake), STR(_userAgent.deviceModel), STR(_userAgent.fwVersion), STR(_userAgent.deviceMAC), STR(_userAgent.versionNo));
				_hasUserAgent = true;
			}
		}
	}

	static char * fmt0 = (char *)
		("GET /socket.io/1/?t=%"PRIu64 " HTTP/1.1\r\n"
		"Host: %s:%" PRIu16 "\r\n"	// use forwarded port
		"Connection: keep-alive\r\n"
		"User-Agent: %s %s %s %s %s\r\n"
		"Origin: http://%s:%" PRIu16 "\r\n"	// use original port
		"Accept: */*\r\n"
		"Referer: http://52.6.14.61:5050/demo.html\r\n"
		"Accept-Encoding: gzip, deflate, sdch\r\n"
		"Accept-Language: en-US,en;q=0.8\r\n"
		"\r\n")
		;;;

	static char * fmt1 = (char *)
		("GET /socket.io/1/websocket/%s HTTP/1.1\r\n"
		"Host: %s:%"PRIu16 "\r\n"
		"Connection: Upgrade\r\n"
		"Pragma: no-cache\r\n"
		"Cache-Control: no-cache\r\n"
		"Upgrade: websocket\r\n"
		"Origin: http://%s:%"PRIu16 "\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"User-Agent: %s %s %s %s %s\r\n"
		"Accept-Encoding: gzip, deflate, sdch\r\n"
		"Accept-Language: en-US,en;q=0.8\r\n"
		"Sec-WebSocket-Key: ZrZ6hzVBYtY2ApJ/dcsXPA==\r\n"
		"Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits\r\n"
		"\r\n")
		;;;
	// we start with fmt0, then send fmt1
	string get;
	_wsNumReq++;
	if (_wsNumReq > 12) {
		FATAL("WebSocket upgrade failed!");
		EnqueueForDelete();		// kill us
	}
	//
	if (1 == (1 & _wsNumReq)) { // send first packet of handshake
		//INFO("$b2$ Sending WebSocket Handshake(%d)", _wsNumReq);
		uint64_t t = ((uint64_t)getutctime() * 1000) + 367;  // add in epoch milli-seconds
		// $ToDo - why is _rrsPort bad?
		_rrsPort = _rrsPortOrig;
		get = format(fmt0, t, STR(_rrsIp), _rrsPort, STR(_userAgent.deviceMake), STR(_userAgent.deviceModel),
				STR(_userAgent.fwVersion), STR(_userAgent.deviceMAC), STR(_userAgent.versionNo),
				STR(_rrsIp), _rrsPortOrig);
		FINE("fmt0: %s", STR(get));
	} else {
		//INFO("$b2$ Sending WebSocket Handshake(%d)", _wsNumReq);
		get = format(fmt1, STR(_ioSocketId), STR(_rrsIp), _rrsPort, STR(_rrsIp), _rrsPortOrig,
				STR(_userAgent.deviceMake), STR(_userAgent.deviceModel), STR(_userAgent.fwVersion),
				STR(_userAgent.deviceMAC), STR(_userAgent.versionNo));
		FINE("fmt1: %s", STR(get));
	}
	//
	SendRaw(get, (char *)"WebSocket offer");
	//
	wsHandshakeDone = false;
	return true;
}

// WS_CheckResponse()
// first response looks like: (note "chunked" at end)
	// HTTP/1.1 200 OK \n Content - Type: text / plain 
	// Access-Control-Allow-Origin : http ://52.6.14.61:5050
	// Access-Control-Allow-Credentials : true
	// Date : Tue, 26 May 2015 22:20:54 GMT
	// Connection : keep - alive  \n Transfer - Encoding : chunked\r\n
	// \r\n47\r\nkeyStringtoparse:etc......
// second response looks like: (note: "Sec-WebSocket-Accept" at end)
	// HTTP / 1.1 101 Switching Protocols
	// Upgrade : websocket
	// Connection : Upgrade
	// Sec-WebSocket-Accept : AuqIOFN1rYvYz34izt1ussPZaHs =
//
bool WrtcSigProtocol::WS_CheckResponse(IOBuffer & buf) {
	uint8_t *pBuffer = GETIBPOINTER(buf);
	uint32_t length = GETAVAILABLEBYTESCOUNT(buf);
	if (length == 0) {	// first dummy
		//INFO("$b2$ calling first WS_SendRequest");
		WS_SendRequest();
		return true;
	}
	// copy our response to a string -
	// be careful to end the string with a blank line
	// (without this we suck up the first websocket line)
	string resp;
	//bool seenLF = false;
	for (uint32_t q = 0; q < length; q++) {
		uint8_t c = pBuffer[q];
		resp += c;
	}
	//INFO("$b2 Got response [%d]:%s:", length, STR(resp));
	//
	if (1 == (_wsNumReq & 1)) { // first half of handshake
		// we have to get the ugly string located ..
		_ioSocketId = "12345678901234567890";// arbitrary in case we fail
		size_t i = resp.find("chunked");
		if (i != string::npos) {
			i = resp.find("\n", i);		// first LF after "chunked"
			if (i != string::npos) {
				i = resp.find("\n", i + 1);	// 2nd LF after "chunked"
				if (i != string::npos) {
					i = resp.find("\n", i + 1);	// 3rd LF after "chunked"
					if (i != string::npos) {
						i += 1;	// skip over linefeed to start
						size_t j = resp.find(":", i);
						_ioSocketId = resp.substr(i, (j - i));
//#ifdef WEBRTC_DEBUG
						DEBUG("Chunked socketId=>%s<", STR(_ioSocketId));
//#endif
					}
				}
			}
		} 
		// even if we failed the above parse, respond and see what happens
		WS_SendRequest();
	}
	else { // 2nd half of handshake
		if (resp.find("Sec-WebSocket-Accept") != string::npos) {
//#ifdef WEBRTC_DEBUG
			DEBUG("Got WebSockt Upgrade!!");
//#endif
			wsHandshakeDone = true;
			// nasty issue here
			// RRS will immediately send a WebSocket packet after this "message"
			// so - we need to adjust length to leave the next message in the buffer
			// thus we find the blank line and reset the length to end there
			size_t i = resp.find("\r\n\r\n");
			length = (uint32_t)i + 4;
			//INFO("$b2$ Revised accepted length to: %d", length);
			string rem = resp.substr(0, length);
			//INFO("$b2$ Revised substr = %s", STR(rem));

		} else { // didn't find our success response
			WARN("Didn't get WebSocket Upgrade %d times!", _wsNumReq>>1);
			WS_SendRequest();
		}
	}
	buf.Ignore(length);
	//INFO("$b2$ Exit CheckWSResponse - buffer length = %d", (int) GETAVAILABLEBYTESCOUNT(buf));
	return true;
}

uint32_t WrtcSigProtocol::WS_GetPayload(IOBuffer & buf, string & msg) {
	// parse the header and unmask the data into "msg"
	uint8_t *pBuf = GETIBPOINTER(buf);
	uint32_t bufLen = GETAVAILABLEBYTESCOUNT(buf);
	WS_HDR hdr;
	uint32_t usedLen = WS_ParseHeader(pBuf, bufLen, hdr);
	if (!usedLen) {
		return 0;
	}
	if (usedLen > bufLen) { // do we have it all
		WARN("Short buffer is %d, need %d", bufLen, usedLen);
		return false;   // no - return to hope we get more
	}
	msg = "";	// ensure it starts out empty
	if (!hdr.hasMask) { // just get the string
		for (uint32_t i = hdr.headerLen; i < usedLen; i++) {
			msg += pBuf[i];
		}
	}else{ // xor the mask to get the payload data
		int m = 0;
		for (uint32_t i = hdr.headerLen; i < usedLen; i++) {
			msg += pBuf[i] ^ hdr.mask[m];
			m = m < 3 ? m + 1 : 0;
		}
	}
	return usedLen;
}
// WS_ParseHeader
// return true if it parsed well and we have enough data
uint32_t WrtcSigProtocol::WS_ParseHeader(uint8_t * pBuf, int len, WS_HDR & hdr) {
	//INFO("$b2$ Bytes are: %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X",
	//	(int)pBuf[0], (int)pBuf[1], (int)pBuf[2], (int)pBuf[3],
	//	(int)pBuf[4], (int)pBuf[5], (int)pBuf[6], (int)pBuf[7]
	//	);
	if (len < 2) return 0;	// need 2 at least!
	hdr.fin = 0 != (pBuf[0] & 0x80);	// bit 7 = fin
	int plen = pBuf[1] & 0x7F;
	hdr.hasMask = 0 != (pBuf[1] & 0x80); // bit 7 = hasMask
	// figure header len
	hdr.headerLen = 2;
	if (hdr.hasMask) hdr.headerLen += 4;
	if (plen == 126) hdr.headerLen += 2;  // len is a short
	if (plen == 127) hdr.headerLen += 8;  // len is a long long
	if (hdr.headerLen > len) {
		//WARN("WrtcSigProtocol: Got Short Data for WebSocket Header! Waiting for more");
		//WARN("WrtcSigProtocol: Header says: %d, we have %d!", hdr.headerLen, len);
		return 0;
	}
	// now get the actual payload len
	if (plen < 126) {
		hdr.payloadLen = plen;
	} else if (plen == 126) {
		hdr.payloadLen = (pBuf[2] << 8) + pBuf[3];
	} else { // huge payload - just grab an integer's worth
		hdr.payloadLen = (pBuf[6] << 24) + (pBuf[7] << 16)
					   + (pBuf[8] << 8)  + pBuf[9];
	}
	// get the mask if we have one
	if (hdr.hasMask) {
		int m = hdr.headerLen - 4;	// mask at end of header
		for (int i = 0; i < 4; i++) {
			hdr.mask[i] = pBuf[m + i];
		}
	}
	//INFO("$b2$ Parsed HeaderLen=%d, PayloadLen=%d",
	//	(int)hdr.headerLen, (int)hdr.payloadLen);
	if ((hdr.payloadLen + hdr.headerLen) > len) {
		WARN("Got short data for Payload!");
		return 0;
	}

	return (uint32_t)(hdr.payloadLen + hdr.headerLen);
}

// Worker Routines

bool WrtcSigProtocol::SendRaw(string & str, char * errStr) {
	//INFO("$b2$: Send raw: %s", STR(str));
	_outBuffer.ReadFromString(str);
	bool ok = SendIOBuffer(errStr);
	//INFO("$b2 SendIOBuffer returned(%s)", ok ? "OK" : "bad");
	return ok;
}

bool WrtcSigProtocol::SendIOBuffer(char * errStr) {
	//INFO("Send IO Buffer called: IOBufferLen = %d", GETAVAILABLEBYTESCOUNT(_outBuffer));
	if (_pFarProtocol != NULL) {
		if (!_pFarProtocol->EnqueueForOutbound()) {
			WARN("Failed sending: %s", errStr);
			return false;
		}
	}
	return true;
}

void WrtcSigProtocol::SetSdpMid(string &sdpMid) {
	_sdpMid = sdpMid;
}

void WrtcSigProtocol::Shutdown(bool isPermanent, bool closeWebrtc) {
	/*
	 * This is no longer needed and actually causes issues when same room name usage
	 * is allowed during heartbeat failure
	 * 
	if (!isPermanent) {
		// Now check if we no longer have any connections to RRS
		RespawnIfNeeded();
	} else {
		_rrsConnections--;
	}
	*/

	// Do a proper clean-up of webrtc session
	CleanUpWebrtc(closeWebrtc);

	INFO("Enqueing WRTCSigProtocol for deletion");
	EnqueueForDelete();
}

/* map the peer state enum against the string and return */
string WrtcSigProtocol::GetPeerState() {
	string state = "";
	switch(_peerState) {
		case WRTC_PEER_NEW:
			state = "new";
			break;

		case WRTC_PEER_SENT_OFFER:
			state = "sent_offer";
			break;

		case WRTC_PEER_RECEIVED_OFFER:
			state = "received_offer";
			break;

		case WRTC_PEER_SENT_ANSWER:
			state = "sent_answer";
			break;

		case WRTC_PEER_RECEIVED_ANSWER:
			state = "received_answer";
			break;

		case WRTC_PEER_ACTIVE:
			state = "active";
			break;

		case WRTC_PEER_CLOSED:
			state = "closed";
			break;

		default:
			state = "unknown";
			break;
	}

	return state;

}

/* map the ICE state enum against the string and return */
string WrtcSigProtocol::GetIceState() {
	string state = "";
	switch(_iceState) {
		case WRTC_ICE_NEW:
			state = "new";
			break;

		case WRTC_ICE_CHECKING:
			state = "checking";
			break;

		case WRTC_ICE_CONNECTED:
			state = "connected";
			break;

		case WRTC_ICE_FAILED:
			state = "failed";
			break;

		case WRTC_ICE_DISCONNECTED:
			state = "disconnected";
			break;

		case WRTC_ICE_CLOSED:
			state = "closed";
			break;

		default:
			state = "unknown";
			break;
	}

	return state;

}

IOBuffer * WrtcSigProtocol::GetOutputBuffer() {
	if (GETAVAILABLEBYTESCOUNT(_outBuffer) > 0)
		return &_outBuffer;
	return NULL;
}

void WrtcSigProtocol::RespawnIfNeeded() {
	_rrsConnections--;
	if (_rrsConnections < 1) {
		// Make sure that we have at least one connection listening/waiting
//		ReSpawn();
		ReSpawn(true, false);
	}
}

void WrtcSigProtocol::UnlinkConnection(WrtcConnection *connection, bool external) {
	if (_pWrtc != connection)
		return;
	if (external && _pWrtc != NULL) {
		_pWrtc->UnlinkSignal(this, false);
	}
	_pWrtc = NULL;
}

bool WrtcSigProtocol::SendWebRTCCommand(Variant& settings) {
	// [WRTCSigProtocol]<>--[WRTCConnection]<>--[OutNetFMP4Stream]
	// 
	// WRTCConnection::SendCommandData() IF the connected stream == targetstreamname
	FINEST("WrtcSigProtocol::SendWebRTCCommand()");
	return _pWrtc != NULL && _pWrtc->SendWebRTCCommand(settings);
}

void WrtcSigProtocol::CleanUpWebrtc(bool force) {
	if (_pWrtc) {
#ifdef RTMSG
		_pWrtc->notifyLedManager(0);
#endif
		// Unlink this session
		_pWrtc->UnlinkSignal(this, false);
		
		// Check if we want to force close the webrtc session
		// Also, check if we have NOT started the peering process
		if (force || (_pWrtc->HasStarted() == false)) {
			// If haven't started, also do a clean-up for this
			_pWrtc->EnqueueForDelete();
		}
		
		_pWrtc = NULL;
	}
}
#endif // HAS_PROTOCOL_WEBRTC

