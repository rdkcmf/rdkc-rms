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
#include "protocols/wrtc/baseiceprotocol.h"
#include "protocols/wrtc/wrtcsigprotocol.h"
#include "protocols/wrtc/stunmsg.h"
#include "protocols/wrtc/candidate.h"
#include "protocols/wrtc/wrtcconnection.h"
#include "netio/netio.h"

const int MAX_RETRIES_LIMIT = 6; // max amount of retries limit, around 24s ( 6 * MAX_RETRIES(40) * 100ms per tick)
const int MAX_RETRIES = 40; // max amount of retries, around 4s (40 * 100ms per tick)
const int CANDIDATE_RETRIES = 20; // max amount of candidate pierce retry - 2s, not counting possible reset when new candidates arrive
const int STUN_RETRIES = 20; // max amount of stun binding retry
// timer based on receipt of STUN_BINDING_REQUEST.  Happens every second for mobile
// but only once every 10 second from browsers.  _connDownCounter is incremented
// every slow tick (once every 15 seconds), so we wait for at least two browser periods
const int DELETION_TIMER = 1;

BaseIceProtocol::BaseIceProtocol(uint64_t type) 
: BaseProtocol(type) {
	_bindIp = "";
	_bindPort = 0;
	_pWrtc = NULL;
	_pSig = NULL;
	_startMs = 0;
	_started = false;
	_hostCan = NULL;
	_reflexCan = NULL;
	_relayCan = NULL;
	_bestCan = NULL;
	_turnBestCan = NULL;
	_stunServerDone = false;
	_turnServerDone = false;
	_stunRetries = STUN_RETRIES;
	_turnRetries = STUN_RETRIES;
	_canSendRetries = CANDIDATE_RETRIES;
	_pierceRetries = CANDIDATE_RETRIES;
	_maxRetries = MAX_RETRIES;
	_maxRetriesLimit = MAX_RETRIES_LIMIT;
	_chromePeerBindRetries = 0;
	_allRetriesZero = false;
	_state = 0;
	_bestPriority = 0;
	_turnBestIp = 0;
	_turnBestPort = 0;
	_usingTurn = false;
	_connDownCounter = 0;
	
	_isTerminating = false;
	
	_candidateType = "all";
}

BaseIceProtocol::~BaseIceProtocol() {
//#ifdef WEBRTC_DEBUG
	DEBUG("BaseIceProtocol deleted.");
//#endif

	if (_reflexCan != NULL) {
		delete _reflexCan;
		_reflexCan = NULL;
	}
	
	if (_hostCan != NULL) {
		delete _hostCan;
		_hostCan = NULL;
	}
	
	if (_relayCan != NULL) {
		delete _relayCan;
		_relayCan = NULL;
	}
	
	// Delete all candidates
	FOR_VECTOR(_cans, i)  delete _cans[i];
	_cans.clear();

	_bestCan = NULL;
	_turnBestCan = NULL;

	// If webrtc session instance is still active, remove this instance
	if (_pWrtc) {
		_pWrtc->RemoveIceInstance(this);
	}
}

void BaseIceProtocol::Initialize(string bindIp, WrtcConnection * pWrtc, WrtcSigProtocol * pSig) {
	_bindIp = bindIp;
	_pWrtc = pWrtc;
	_pSig = pSig;
	
	Variant &customParam = _pWrtc->GetCustomParameters();
	if (customParam.HasKey("candidateType", false)) {
    	_candidateType = (string) customParam.GetValue("candidateType", false);
    }

	FinishInitialization();
}

void BaseIceProtocol::Start(time_t ms) {
	NYI;
	return;
}

// FastTick()
// - sorry for this huge routine we really only do 4 things
// 1) Send out STUN messages to the STUN server (if we aren't done)
// 2) Re-Send Pierce attempts to each Candidate
// 3) Re-Send most recently sent message to the Turn servrer
// Each step has a Retry Counter
// At the end, we summarize the status of the Candidates
// @return true if we still need a retry tick
//
IOHandler *BaseIceProtocol::GetIOHandler() {
	return NULL;
}

//TODO: this function needs to be refactored to adapt the changes on the latest
// google webrtc stack behaviour
bool BaseIceProtocol::FastTick(bool reset, bool &isConnected) {
	// Bail out early if we are shutting down
	if (_isTerminating) {
		return false;
	}
	
	isConnected = false;
	if (reset) {
		// If we are asked to reset, retry the previous candidates in case we
		// are lucky enough to pierce it
		_allRetriesZero = false;
		_stunRetries = STUN_RETRIES;
		_turnRetries = STUN_RETRIES;
		_pierceRetries = CANDIDATE_RETRIES;
		//_maxRetries = MAX_RETRIES; // do not include the max retry, as we want it to be the deciding factor
	}

	// Reset to happen after max amount of retries, around 4s (40 * 100ms per tick) upto the limit of _maxRetriesLimit(6 times).
	if ((_maxRetries <= 0) && (_maxRetriesLimit > 0)) {
		_maxRetriesLimit--;

		// Reset everything including _maxRetries
		_allRetriesZero = false;
		_stunRetries = STUN_RETRIES;
		_turnRetries = STUN_RETRIES;
		_pierceRetries = CANDIDATE_RETRIES;
		_maxRetries = MAX_RETRIES;
	}

	//UPDATE: we make sure that we already exhausted the TURN retries as well
	// note that _turnRetries are assigned to 0 when a successful host or reflex was established
	if ((_maxRetries <= 0) && (_turnRetries <= 0)) {
		return false; // we have reached the max timeout
	}
	_maxRetries--;

	if (IsDead()) {
		return false; // we are done!
	}

	if (_allRetriesZero) {
		// $ToDo$ Send STUN Refreshes over the selected channel
		// At a very slow (15 second??) interval
		//
		WARN("Exhausted all retries for %s [%s]!", STR(_hostIpStr), (_isTcp ? STR("TCP") : STR("UDP")));
		return false;
	}
//#ifdef WEBRTC_DEBUG
	DEBUG("IceProtocol::Tick({%d,%d,%d,%d}): %s [%s]",
			_stunRetries, _canSendRetries, _pierceRetries, _turnRetries, STR(_hostIpStr), (_isTcp ? STR("TCP") : STR("UDP")));
//#endif
	//
	//
	int goodPierceCount = 0;
	int goodTurnCount = 0;
	// resend stuff as needed
	//
	// 1) check that we have reflected via STUN
	if (!IsReflexGood()) {
		// let's try again, we need our NAT'd address!!
		if (!DecRetryToZero(_stunRetries)) {
			// Fix for issue where we have a TURN server entry but no STUN
			// The above case will result to a very delayed reflex candidate!
			if (_stunServerIpStr.empty()) {
				_stunServerIpStr = _turnServerIpStr;
			}

			uint8_t transId[] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x01};
			StunMsg * pMsg = StunMsg::MakeMsg(STUN_BINDING_REQUEST, transId);
//#ifdef WEBRTC_DEBUG
			DEBUG("Sending STUN Bind Request to server: %s", STR(_stunServerIpStr));
//#endif
			SendStunMsgTo(pMsg, _stunServerIpStr);
		}
	} else {
//#ifdef WEBRTC_DEBUG
		DEBUG("Reflex is good");
//#endif
		_stunRetries = 0;
	}

	// return if we haven't really started
	if (!_started) {
		return true;
	}

	// 2) now the outgoing binding requests, one per Candidate
	FOR_VECTOR(_cans, i) {
		// Also try to bind directly to the remote relay candidate
		//if (!_cans[i]->IsRelay()) {
			// Keep sending peer bind request
			SendPeerBindMsg(_cans[i], false, false);

			if (_cans[i]->IsStatePierceGood()) {
//#ifdef WEBRTC_DEBUG
				DEBUG("Candidate Pierce: good, %s", STR(_cans[i]->GetShortString()));
//#endif
				// Only exit early if this is NOT a relay. otherwise, if it's a
				// relay, we wait for the max timeout (MAX_RETRIES)
				if (!_cans[i]->IsRelay()) {
					goodPierceCount++;
				}

				// Cancel TURN if we have a good pierce!!
				_turnRetries = 0;
			}
		//}
	}
	
	_pierceRetries--;

	// 3) finally try turnServer
	// We first do an AllocateUDP (creating a TURN Allocation)
	// - 2 phases, first bar message, fail, then use returned NONCE
	// Then we do a CREATE PERMISSION for the target peer
	// - the only sensible target peer is a REFLEX Candidate
	// - so I guess we send for each REFLEX candidate
	// Note: perhaps we should reuse the TransId of when message first sent
	// - but I think we are fine for now just making a new one
	if (!IsTurnGood()) {
		if (!DecRetryToZero(_turnRetries)) {
			if (!IsTurnAlloc()) {
				// we need to send another Allocate
				SendTurnAllocUDP();
			} else if (!IsTurnPerm()) {
				// we are at the PERMISSIONs phase
				SendTurnCreatePermission();
			} else {
				SendTurnBind(); // resend binds over turn
			}
		} else { // we timed out our retries
			FOR_VECTOR(_cans, i) {
				if (_cans[i]->IsStateTurnGood()) {
					goodTurnCount++;
					_state |= ST_TURN_GOOD;
				}
			}
			//INFO("$b2$ Tick completed TurnServer retries, good #=%d", goodTurnCount);
		}
	} else {
//#ifdef WEBRTC_DEBUG
		DEBUG("TURN is good");
//#endif
		_turnRetries = 0;

		// shouldn't use the remote relay candidates till 2s (20 * 100ms per tick)
		if ((_maxRetries < STUN_RETRIES) || (_maxRetriesLimit != MAX_RETRIES_LIMIT)) {
			_pierceRetries = 0;
		}
	} // end Turn segment

	//_allRetriesZero = (0 == _canSendRetries) && (0 == _pierceRetries) && (0 == _turnRetries);
	_allRetriesZero = (0 == _pierceRetries) && (0 == _turnRetries);

	if (_allRetriesZero) { // we've retried out, so mark outgoing guys as dead
		goodPierceCount = 0;
		goodTurnCount = 0;

		FOR_VECTOR(_cans, i) {
			// We should be checking all candidates including remote relay
			//if (!_cans[i]->IsRelay()) {
				if (!_cans[i]->IsStatePierceOut()) {
					_cans[i]->SetStatePierceBad();
				} else if (_cans[i]->IsStatePierceGood()) {
//#ifdef WEBRTC_DEBUG
					DEBUG("Candidate Pierce: good, %s", STR(_cans[i]->GetShortString()));
//#endif
					goodPierceCount++;
				}
			//}

			if (_cans[i]->IsStateTurnGood()) {
				goodTurnCount++;
			}
		}
//#ifdef WEBRTC_DEBUG
		DEBUG("Tick completed: Pierce Out (%"PRIu32"), TurnServer (%"PRIu32")", goodPierceCount, goodTurnCount);
//#endif
	}
	
	// so set our global flag to good if we have at least 1 good candidate!
	if (goodTurnCount) {
		_state |= ST_TURN_GOOD; //!!!
	}
	
	if (goodPierceCount && IsPierceIn()) {
		_state |= ST_PIERCE_GOOD;
//#ifdef WEBRTC_DEBUG
		DEBUG("Got Pierce Good state.");
//#endif
		isConnected = true;
		return false;
	}

	//INFO("Tick(%s) returning %s", STR(_hostIpStr), _allRetriesZero ? "true" : "false");

	// return true means continue with FastTicks
	return !_allRetriesZero;
}

bool BaseIceProtocol::SlowTick(bool reset) {
	// Bail out early if we are shutting down
	if (_isTerminating) {
		return false;
	}
	
	// check if we should be expiring the connection
	if( _connDownCounter++ >= DELETION_TIMER && !_pWrtc->IsEnqueueForDelete()) {
		WARN("WebRTC session timed out, quitting connection");
		_pWrtc->EnqueueForDelete();
	}

	// send out a refresh
	if (_bestCan) {
		SendPeerBindMsg(_bestCan, true, false);
	} else {
		if (IsTurnGood()) {
			// RDKC-4118: This was disabled because it was causing the Twilio turn server
			// to close our connections when sent (when local candidate is relay: aka "Reporting Best Turn:")
			// This SHOULD be sent per spec, but we're disabling now given the behavior presented by the 
			// twilio servers
			//SendTurnRefresh(); // do a bind refresh to TURN
			SendTurnCreatePermission(); // do this as well
			SendPeerBindMsg(_turnBestCan, true, true); // send bind to actual peer
		} else {
			// No candidate connected
			return false;
		}
	}

	return true;
}
//
// Final Selection calls
//
uint32_t BaseIceProtocol::Select() {
	// UPDATE: Only do a select(), if this ICE actually started
	// If not, we return immediately
	if (!_started) return 0;

//#ifdef WEBRTC_DEBUG
	INFO("Stun selecting [%s (%s)]", STR(_hostIpStr), (_isTcp ? STR("TCP") : STR("UDP")));
//#endif
	// cruise our candidates and choose the best good one
	// return false if we didn't find one
	_bestPriority = 0;
	FOR_VECTOR(_cans, i) {
		Candidate * pCan = _cans[i];
//#ifdef WEBRTC_DEBUG
		INFO("Checking Can:[%s]", STR(pCan->GetLongString()));
//#endif
		if (pCan->IsStatePierceGood()) {
			if (_bestCan) {	// have a best, so check it with this one
				if (pCan->IsBetterThan(_bestCan)) {
					SetBestCandidate(pCan);
				}
			} else {
				// We removed the check for the non relay candidates
				// Remote relay candidates should also be included!
				SetBestCandidate(pCan);
			}
		}
	}

	// Only process TURN if we weren't able to pierce normally
	if (IsTurnGood() && (_bestCan == NULL)) {
		FOR_VECTOR(_cans, i) {
			if (_cans[i]->IsStateTurnGood()) {
				if (_cans[i]->IsBetterThan(_turnBestCan)) {
					_turnBestCan = _cans[i];
					_turnBestIpStr = _turnBestCan->GetIpStr();
					if (IpPortStrIsV6(_turnBestIpStr)) {
						SplitIpStrV6(_turnBestIpStr, _turnBestIpv6, _turnBestPort);
					} else {
						SplitIpStrV4(_turnBestIpStr, _turnBestIp, _turnBestPort);
					}
					if (_turnBestCan->GetPriority() > _bestPriority) {
						_bestPriority = _turnBestCan->GetPriority();
					}
				}
			}
		}
	}
	//
	return _bestPriority;
}

void BaseIceProtocol::ReportBestCandidate() {
	if (_bestCan) {
		INFO("Reporting Best: %s", STR(_bestCan->GetShortString()));
		SendPeerBindMsg(_bestCan, true, false);
	} else if (_turnBestCan) {
		// Select this candidate
		INFO("Reporting Best TURN: %s", STR(_turnBestCan->GetShortString()));
		SendPeerBindMsg(_turnBestCan, true, true);
	} else {
		FATAL("No best candidate found!");
	}
}

bool BaseIceProtocol::IsBetterThan(BaseIceProtocol * pStun) {
	return (_bestPriority > pStun->_bestPriority);
}

//
// some misc config calls
//

bool BaseIceProtocol::SetStunServer(string stunServerIpStr) {
	// INFO("[ICE-%s (%s)] Got STUN server ip: %s", (_isTcp ? STR("TCP") : STR("UDP")),
	// 		STR(_bindIp), STR(stunServerIpStr));

	_stunServerAddress.setIPPortAddress(stunServerIpStr);
	_stunServerIpStr = _stunServerAddress.getIPwithPort();
	//
	bool ok = true;
	if (_started) {
		// set out a bind request to get our reflex (NAT) address
		uint8_t transId[] = { 0x00, 0x00, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x00 };
		StunMsg * pMsg = StunMsg::MakeMsg(STUN_BINDING_REQUEST, transId);
		ok = SendStunMsgTo(pMsg, _stunServerIpStr);
		_state |= ST_REFLEX_SENT;
	}

	return ok;
}

bool BaseIceProtocol::SetTurnServer(string username, string credential, string ipPort) {
	// INFO("[ICE-%s (%s)] Got TURN {ip,user,cred}: {%s,%s,%s}", (_isTcp ? STR("TCP") : STR("UDP")),
	// 	STR(_bindIp), STR(ipPort), STR(username), STR(credential));

	_turnUsername = username;
	_turnCredential = credential;
	_turnServerAddress.setIPPortAddress(ipPort);	// for v6... and v4, eventually
	_turnServerIpStr = _turnServerAddress.getIPwithPort();
	//
	// $ToDo - send Turn server handshake
	//	- do I send it Now?  or wait say 500ms to see if we need it?
	//  - will await the first tick
	return true;
}

void BaseIceProtocol::SetIceUser(string usr, string pwd, string peerUsr, string peerPwd) {
	_iceUser = usr;
	_icePwd = pwd;
	_peerIceUser = peerUsr;
	_peerIcePwd	= peerPwd;
	_iceUserUser = peerUsr + ":" + usr;
}

void BaseIceProtocol::SetIceFingerprints(const string myfp, const string peerfp) {
	_iceFingerprint = myfp;
	_peerIceFingerprint = peerfp;
}

bool BaseIceProtocol::AddCandidate(Candidate *&pCan, bool clone) {
	// Sanity check
	if (pCan == NULL) {
		WARN("Candidate to be added is null.");
		return false;
	}
	
	FOR_VECTOR(_cans, i) {
		if (pCan->IsSame(_cans[i])) {
//#ifdef WEBRTC_DEBUG
			DEBUG("Ignoring duplicate Candidate: %s %"PRIu32" %x %x", STR(pCan->GetIpStr()), i, pCan, _cans[i]);
//#endif
			if (!clone) {
				// Only delete if we are not cloning
				// and then assign the previous duplicate
				delete pCan;
				pCan = _cans[i];
			}
			return true;
		}
	}

	FINEST("[ICE-%s] Adding %s", (_isTcp ? STR("TCP") : STR("UDP")), STR(pCan->GetShortString()));

	if (clone) {
		// Clone the candidate and take ownership
		Candidate *pCanCopy = pCan->Clone();
		_cans.push_back(pCanCopy);
	} else {
		_cans.push_back(pCan);
	}

	return true;
}

bool BaseIceProtocol::SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress) {
	NYIR;
}

bool BaseIceProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	NYIR;
}

bool BaseIceProtocol::SignalInputData(IOBuffer &buffer) {
	NYIR;
}

bool BaseIceProtocol::EnqueueForOutbound() {
	bool ret = false;

	if (_pNearProtocol == NULL) {
		return true;
	}

	// Get the output buffer of the top protocol, in this case, DTLS
	IOBuffer *pBuffer = _pNearProtocol->GetOutputBuffer();
	if( NULL != pBuffer ) {
		uint8_t *pData = GETIBPOINTER(*pBuffer);
		if ( NULL != pData ) {
			uint32_t length = GETAVAILABLEBYTESCOUNT(*pBuffer);
			if ( length > 0 ) {
				if (_bestCan) {
					// We have pierced our peer's NAT, send direct!!
					//ret = SendToV4(_bestIp, _bestPort, pData, length);
					//TODO: sanity check in case bestAddress is invalid
					ret = SendTo(_bestAddress.getIPv4(), _bestAddress.getSockPort(), pData, length);
				} else if (_turnBestCan) {
					// We have to bounce off to the relay
					ret = SendDataToTurnCandidate(pData, length, _turnBestCan);
				}
			} else {
				WARN("[BaseIceEnqueueForOutbound] : Empty IOBuffer");
			}
		} else {
			WARN("[BaseIceEnqueueForOutbound] : Invalid IOBuffer data");
		}
		// Clear the contents of the buffer after sending
		pBuffer->IgnoreAll();
	} else {
		WARN("[BaseIceEnqueueForOutbound] : Invalid IOBuffer Object");
	}
	return ret;
}

//
// server message methods - pMsg deleted by caller
//
void BaseIceProtocol::HandleServerMsg(StunMsg * pMsg, int type, int err, IOBuffer & buffer) {
//#ifdef WEBRTC_DEBUG
	DEBUG("[ICE-%s] Got STUN Message(0x%x) from server", (_isTcp ? STR("TCP") : STR("UDP")), (uint32_t)type);
//#endif
	switch (type) {
	case STUN_BINDING_REQUEST:  //= 0x0001,
	case STUN_ALLOCATE_REQUEST:  //= 0x0003,
	case STUN_REFRESH_REQUEST:  //= 0x0004,
	case STUN_CHANNEL_BIND_REQUEST:  //= 0x0009,
	case STUN_SHARED_SECRET_REQUEST:  //= 0x0002,
	case STUN_CREATE_PERM_REQUEST:  //= 0x0008,
		WARN("[ICE-%s] Unexpected Request Message from server, type: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
		return;
	case STUN_BINDING_INDICATION:  //= 0x0011,
	case STUN_SEND_INDICATION:  //= 0x0016,
		// Data To TURN Server - this is very odd
		WARN("[ICE-%s] Unexpected Indication Message from server, type: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
		return;
	case STUN_DATA_INDICATION:  //= 0x0017,
		// Data FROM (via) TURN Server
		HandleTurnDataIndication(pMsg, buffer);
		return;
	case STUN_BINDING_RESPONSE:  //= 0x0101,
		// good reflex echo of our external address
//#ifdef WEBRTC_DEBUG
		DEBUG("[ICE-%s] Got STUN_BINDING_RESPONSE from Server", (_isTcp ? STR("TCP") : STR("UDP")));
//#endif
		HandleReflexMsg(pMsg, type);
		return;
	case STUN_SHARED_SECRET_RESPONSE:  //= 0x0102,
	case STUN_ALLOCATE_RESPONSE:  //= 0x0103,
		// now send the permission request
//#ifdef WEBRTC_DEBUG
		DEBUG("[ICE-%s] Got STUN_ALLOCATE_RESPONSE from Server", (_isTcp ? STR("TCP") : STR("UDP")));
//#endif
		if (IsTurnAlloc()) {
//#ifdef WEBRTC_DEBUG
		DEBUG("[ICE-%s] Got redundant STUN_ALLOCATE_RESPONSE from Server", (_isTcp ? STR("TCP") : STR("UDP")));
//#endif
			return;
		}

		if (!pMsg->GetXORAddressAttr(STUN_ATTR_XOR_RELAYED_ADDR,_turnAllocIpStr)) {
			WARN("[ICE-%s] TURN Allocate Response has missing RELAYED_ADDR.", (_isTcp ? STR("TCP") : STR("UDP")));
		}

		// send our relay candidate now that we have an allocation
		if (!_relayCan) {
			_relayCan = Candidate::CreateRelay(_turnAllocIpStr, _reflexIpStr);
		} else {
			WARN("[ICE-%s] Relay candidate was already created.", (_isTcp ? STR("TCP") : STR("UDP")));
		}

		_pSig->SendCandidate(_relayCan);
		//
		_state |= ST_TURN_ALLOC;	// got allocate
		SendTurnCreatePermission();	// now go get permission
		return;
	case STUN_REFRESH_RESPONSE:  //= 0x0104,
		break;	// be happy we got one
	case STUN_CREATE_PERM_RESPONSE:  //= 0x0108,
//#ifdef WEBRTC_DEBUG
		DEBUG("[ICE-%s] Got STUN_CREATE_PERM_RESPONSE from Server (%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(pMsg->GetTransIdStr()));
//#endif
		// we included all candidates in our PERM request
		_state |= ST_TURN_PERM;
		SendTurnBind();
		break;
	case STUN_CHANNEL_BIND_RESPONSE:  //= 0x0109,
		DEBUG("[ICE-%s] Unexpected Server Response: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
		return;
	case STUN_ALLOCATE_ERROR_RESPONSE:  //= 0x0113,
	{
//#ifdef WEBRTC_DEBUG
		string msg;
		int code;
		if(!pMsg->GetErrorCodeAttr(code, msg)) {
			// If the error code for some reason returned false, set the variables
			// to prevent any weird values
			code = 0;
			msg = "";
		}
		DEBUG("[ICE-%s] STUN Allocate Error [0x%x - 0x%x], %s transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), err, code, STR(msg), STR(pMsg->GetTransIdStr()));
//#endif
		// probably need realm & nonce
		HandleTurnAllocErrMsg(pMsg, type);
		return;
	}
	case STUN_SHARED_SECRET_ERROR_RESPONSE:  //= 0x0112,
	case STUN_REFRESH_ERROR_RESPONSE:  //= 0x0114,
	case STUN_CREATE_PERM_ERROR_RESPONSE:  //= 0x0118,
	  {
		string msg;
		int code;
		pMsg->GetErrorCodeAttr(code, msg);
		DEBUG("[ICE-%s] STUN CreatePermission Error [0x%x], %s, %s", (_isTcp ? STR("TCP") : STR("UDP")), code, STR(msg), STR(pMsg->GetTransIdStr()));
		//HandleTurnCreatePermErrMsg(pMsg, type);
		return;
	  }
	case STUN_BINDING_ERROR_RESPONSE:  //= 0x0111,
	case STUN_CHANNEL_BIND_ERROR_RESPONSE:  //= 0x0119
		DEBUG("[ICE-%s] Unexpected server error response: 0x%x, Err: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type, err);
		return;
	default:
		WARN("[ICE-%s] Unknown server message: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
	}
}

void BaseIceProtocol::HandleReflexMsg(StunMsg * pMsg, int type) {
	// get our NAT/Reflex IP addr as seen by the stun
	if (!pMsg->GetXORAddressAttr(STUN_ATTR_XOR_MAPPED_ADDR, _reflexIpStr)) {
		if (!pMsg->GetAddressAttr(STUN_ATTR_MAPPED_ADDR, _reflexIpStr)) {
			WARN("[ICE-%s] Reflex message has missing MAPPED ADDR!", (_isTcp ? STR("TCP") : STR("UDP")));
		}
	}
	// form a candidate to report to our peer via RRS (RDKC Rendezvous Server)
	if (_reflexIpStr.size()) {
		if (!IsReflexGood()) { // new knowledge of our reflex addr
//#ifdef WEBRTC_DEBUG
			DEBUG("Setting Reflex ip: %s", STR(_reflexIpStr));
//#endif
			_reflexCan = Candidate::CreateReflex(_reflexIpStr, GetHostIpStr());
			_pSig->SendCandidate(_reflexCan);
			_state |= ST_REFLEX_GOOD;
			//
			// also send out a RELAY Candidate
			//b2: moved to case ALLOCATE_RESPONSE above!
		} else {
//#ifdef WEBRTC_DEBUG
			DEBUG("[ICE-%s] Got Redundant Binding Response from STUN Server", (_isTcp ? STR("TCP") : STR("UDP")));
//#endif
		}
	}
}

void BaseIceProtocol::HandleTurnAllocErrMsg(StunMsg * pMsg, int type) {
	// Resend the allocation request for each error
	if (!pMsg->GetStringAttr(STUN_ATTR_REALM, _turnRealm)) {
		DEBUG("Missing Realm attribute.");
	}
	else {
		FINE("[ICE-%s] HandleTurnAllocErrMsg Realm(%s) transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(_turnRealm), STR(pMsg->GetTransIdStr()));
	}
	
	if (!pMsg->GetStringAttr(STUN_ATTR_NONCE, _turnNonce)) {
		DEBUG("Missing Nonce attribute.");
	}
	else {
		FINE("[ICE-%s] HandleTurnAllocErrMsg Nonce(%s) transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(_turnNonce), STR(pMsg->GetTransIdStr()));
	}

	//
	SendTurnAllocUDP(); // send with this new info
}

void BaseIceProtocol::HandleTurnCreatePermErrMsg(StunMsg * pMsg, int type) {
	//TODO: check if there is something else needed
	// Resend the allocation request for each error
	if (!pMsg->GetStringAttr(STUN_ATTR_REALM, _turnRealm)) {
		DEBUG("Missing Realm attribute.");
	}
	else {
		FINE("[ICE-%s] HandleTurnCreatePermErrMsg Realm(%s) transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(_turnRealm), STR(pMsg->GetTransIdStr()));
	}
	
	if (!pMsg->GetStringAttr(STUN_ATTR_NONCE, _turnNonce)) {
		DEBUG("Missing Nonce attribute.");
	}
	else {
		FINE("[ICE-%s] HandleTurnCreatePermErrMsg Nonce(%s) transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(_turnNonce), STR(pMsg->GetTransIdStr()));
	}

	SendTurnCreatePermission();
}

void BaseIceProtocol::HandleTurnDataIndication(StunMsg * pMsg, IOBuffer & buffer) {
	// unwrap the data
	// if it's not stun, pass it up
	// otherwise, we only  do something for a a BIND_REQUEST
	int len;
	uint8_t * pData = pMsg->GetDataAttr(len);
	if (!len) {
		return; // false alarm I guess? or malformed
	}
	
	//UPDATE: At this point, we're manipulating the buffer contents ready to be
	// sent out. Since we now support aggregated stun packets, we need to make
	// sure that only the corresponding message gets processed.
	_tmpBuffer.IgnoreAll();
	_tmpBuffer.ReadFromInputBuffer(buffer, pMsg->GetMessageLen());
	
	// fixup our IOBuffer to send just the Data Attribute's data up
	int a = pMsg->FindAttribute(STUN_ATTR_DATA);
	_tmpBuffer.Ignore(a + 4); // cut out everything down to data attribute value field

	if (!StunMsg::IsStun(pData, len)) {
		// Not STUN, assume it is data, send it up!
		if (_pNearProtocol != NULL) {
			_pNearProtocol->SignalInputData(_tmpBuffer);
		}
		return;
	}

	// find the candidate referenced herein
	string ipStr;
	if (!pMsg->GetXORAddressAttr(STUN_ATTR_XOR_PEER_ADDR, ipStr)) {
		WARN("Missing Address attribute!");
		return; // then we don;t know this candidate, just ignore it
	}
	// it is a STUN message embedded in the TURN::STUN SendIndication message
	// pMsg gets deleted outside this function, use another variable!
	StunMsg *pMsgParsed = StunMsg::ParseBuffer(_tmpBuffer, ipStr);

	int err = pMsgParsed->GetErrorCodeAttr();
	int type = pMsgParsed->GetType();

	Candidate * pCan = FindCandidate(ipStr);
	if (pCan) {
		// Set TURN good for bind requests or responses.
		if ((type == STUN_BINDING_REQUEST ||
				type == STUN_BINDING_RESPONSE) &&
				((_candidateType == "all") ||
				(_candidateType == pCan->GetType()))) {
			pCan->SetStateTurnGood();
			_state |= ST_TURN_GOOD;
		}
		
		// For unknown candidates, we let the HandlePeerMsg() take care of that
	}

	// Handle the data
	HandlePeerMsg(pMsgParsed, type, err, true);

	// Clean-up
	delete pMsgParsed;
}

void BaseIceProtocol::SendTurnAllocUDP() {
	// send the allocate request for both ipv4 and ipv6 relay candidate
	SendTurnAllocUDPV4();
	SendTurnAllocUDPV6();
}

void BaseIceProtocol::SendTurnAllocUDPV4() {
	uint8_t transId[] = { 0x00, 0x00, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x02 };
	if (_turnNonce.size()) {
		transId[11] = 0x03;
	}
	
	StunMsg * pMsg = StunMsg::MakeMsg(STUN_ALLOCATE_REQUEST, transId);
	pMsg->AddAllocateUdp();
	//pMsg->AddReqAddrFamily(SocketAddress::isV6(_turnServerIpStr));
	pMsg->AddReqAddrFamily(false);
	//hmm, chrome works without the Lifetime....
	pMsg->AddLifetime(3600);
	// our first alloc request is simple,
	// he responds with our realm and nonce
	if (_turnNonce.size()) {
		FINE("[ICE-%s] Send allocate request over UDP for IPV4 relay candidate to Turn Server with Username(%s) Realm(%s) Nonce(%s) transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(_turnUsername), STR(_turnRealm), STR(_turnNonce), STR(pMsg->GetTransIdStr()));
		pMsg->AddUserRealmNonce(_turnUsername, _turnRealm, _turnNonce);
		pMsg->AddMessageIntegrity(GetTurnKeyStr());
	}
	else {
		FINE("[ICE-%s] Send allocate request over UDP for IPV4 relay candidate to Turn Server without Username, Realm, Nonce; transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(pMsg->GetTransIdStr()));
	}

	SendStunMsgTo(pMsg, _turnServerIpStr);
}

void BaseIceProtocol::SendTurnAllocUDPV6() {
	uint8_t transId[] = { 0x00, 0x00, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef, 0x00, 0x00, 0x00, 0x02};
	if (_turnNonce.size()) {
		transId[11] = 0x03;
	}

	StunMsg * pMsg = StunMsg::MakeMsg(STUN_ALLOCATE_REQUEST, transId);
	pMsg->AddAllocateUdp();
	//pMsg->AddReqAddrFamily(SocketAddress::isV6(_turnServerIpStr));
	pMsg->AddReqAddrFamily(true);
	//hmm, chrome works without the Lifetime....
	pMsg->AddLifetime(3600);
	// our first alloc request is simple,
	// he responds with our realm and nonce
	if (_turnNonce.size()) {
		FINE("[ICE-%s] Send allocate request over UDP for IPV6 relay candidate to Turn Server with Username(%s) Realm(%s) Nonce(%s) transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(_turnUsername), STR(_turnRealm), STR(_turnNonce), STR(pMsg->GetTransIdStr()));
		pMsg->AddUserRealmNonce(_turnUsername, _turnRealm, _turnNonce);
		pMsg->AddMessageIntegrity(GetTurnKeyStr());
	}
	else {
		FINE("[ICE-%s] Send allocate request over UDP for IPV6 relay candidate to Turn Server without Username, Realm, Nonce; transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(pMsg->GetTransIdStr()));
	}

	SendStunMsgTo(pMsg, _turnServerIpStr);
}

// Send permission request for all candidates
void BaseIceProtocol::SendTurnCreatePermission() {
	// Presumably, we only do this once allocation is already successful.
	// However, it seems that this gets called even though allocation is not yet
	// successful, or that the realm and nonce are still empty somehow
	// Do a sanity check
	if (_turnUsername.empty() || _turnRealm.empty() || _turnNonce.empty()) {
		// Bail out early if the needed info we need are missing
		return;
	}
	
	FOR_VECTOR(_cans, i) {
		Candidate * pCan = _cans[i];

		StunMsg * pMsg = StunMsg::MakeMsg(STUN_CREATE_PERM_REQUEST);
//#ifdef WEBRTC_DEBUG
		DEBUG("SendTurnCreatePermission: %s, %s", STR(pCan->GetIpStr()), STR(pMsg->GetTransIdStr()));
//#endif
		INFO("Send create permission to Turn Server for peer ip: %s, with tranID: %s", STR(pCan->GetIpStr()), STR(pMsg->GetTransIdStr()));
		FINE("[ICE-%s] Send create permission to Turn Server with Username(%s) Realm(%s) Nonce(%s) transID(%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(_turnUsername), STR(_turnRealm), STR(_turnNonce), STR(pMsg->GetTransIdStr()));
		pMsg->AddAddressXOR(STUN_ATTR_XOR_PEER_ADDR, pCan->GetIpStr());
		pMsg->AddUserRealmNonce(_turnUsername, _turnRealm, _turnNonce);
// Lifetime is rejected by COTURN as an invalid attribute.
// Also, commenting this out produced no ill effects on RESTUND
// TURN.
//		pMsg->AddLifetime(300); 
		pMsg->AddMessageIntegrity(GetTurnKeyStr());

		SendStunMsgTo(pMsg, _turnServerIpStr);
	}
}

void BaseIceProtocol::SendTurnRefresh() {
	// this is a REFRESH, rather than a Bind
	//StunMsg * pMsg = StunMsg::MakeMsg(STUN_REFRESH_REQUEST);

	// The message was changed to a bind to appease twilio, but per
	// RDKC-4118 neither the refresh or the bind made twilio happy
	// Send a complete peer bind request packet to TURN
	StunMsg * pMsg = StunMsg::MakeMsg(STUN_BINDING_REQUEST);
	pMsg->AddStringAttr(STUN_ATTR_USERNAME, _iceUserUser);
	pMsg->AddMessageIntegrity(_peerIcePwd);
	pMsg->AddFingerprint();
	pMsg->AddUserRealmNonce(_turnUsername, _turnRealm, _turnNonce);
	SendStunMsgTo(pMsg, _turnServerIpStr);
}

void BaseIceProtocol::SendTurnBind() {
	// Bind to candidates using a relay
	FOR_VECTOR(_cans, i) {
		Candidate * pCan = _cans[i];
		// If this loop tries to connect to a reflex IP, there are cases when
		// the connection will just hang and won't fail.  (case in point --
		// qwr's external ip.)
		// Also, since this _is_ a TURNbind operation, it should be safe
		// to restrict peerbind messages to relay-type candidates.
		//if (pCan->IsRelay()) {
		//	SendPeerBindMsg(pCan, false, true);
		//}
		// UPDATE:
		// We should be trying all candidates over TURN and not just wait for a 
		// request packet
		SendPeerBindMsg(pCan, false, true);
	}
}

//
// Peer Message Methods - pMsg deleted by caller
//
void BaseIceProtocol::HandlePeerMsg(StunMsg * pMsg, int type, int err, bool isUsingRelay) {
	string fromIpStr = pMsg->GetFromIpStr();
//#ifdef WEBRTC_DEBUG
	FINE("[ICE-%s] Got STUN Message(0x%x) from peer [%s] %s", (_isTcp ? STR("TCP") : STR("UDP")), (uint32_t)type, STR(fromIpStr), (isUsingRelay ? STR("using relay") : STR("not relayed")));
//#endif

	Candidate * pCan = FindCandidate(fromIpStr);
	if (!pCan) {
		WARN("[ICE-%s] Unknown candidate: %s (%s)", (_isTcp ? STR("TCP") : STR("UDP")), STR(fromIpStr), (isUsingRelay ? STR("using relay") : STR("not relayed")));
		// If we find an unknown candidate then we might be dealing with
		// a symmetric NAT.  If so, then we need to build a new candidate
		// based on this new info
		// It is considered a remote candidate in this situation
		pCan = Candidate::CreateRemote(fromIpStr);
		AddCandidate(pCan, false);
		
		// We got it from the peer, so clearly they can pierce us
		// Set state of candidate only if not using a relay!
		if (!isUsingRelay) {
			pCan->SetStatePierceIn();
		} else {
			// If this is a relay, see if we can also do a direct pierce
			SendPeerBindMsg(pCan, false, false);
		}

		// Make sure we immediately send out a response to expedite the peering
		SendPeerBindMsg(pCan, false, isUsingRelay);
		
		_state |= ST_PIERCE_IN;
	} else {
		if (pCan->IsStateDead()) {
			return;
		}
	}

	StunMsg * pRsp = NULL;
	switch (type) {
	case STUN_BINDING_REQUEST:  //= 0x0001,
		//on bind request, reset counter since we've heard from the peer
		_connDownCounter = 0;

		// Filtering logic
		if ((_candidateType != "all") && (_candidateType != pCan->GetType())) {
			//WARN("Not peering with %s", STR(pCan->GetShortString()));
			return;
		}

		pRsp = StunMsg::MakeMsg(STUN_BINDING_RESPONSE, pMsg->GetTransId());
		pRsp->AddAddressXOR(STUN_ATTR_XOR_MAPPED_ADDR, fromIpStr);
		// Use our password for the response!!!
		pRsp->AddMessageIntegrity(_icePwd);
		pRsp->AddFingerprint();

		SendData(pRsp, pCan, isUsingRelay);

		// Set state of candidate only if not using a relay!
		if (!isUsingRelay) {
			pCan->SetStatePierceIn();
		}
		
		if (pMsg->FindAttribute(STUN_ATTR_USE_CANDIDATE)) {
			pCan->SetPeerUse(true);
		}
		_state |= ST_PIERCE_IN;
		return;
	case STUN_ALLOCATE_REQUEST:  //= 0x0003,
	case STUN_REFRESH_REQUEST:  //= 0x0004,
	case STUN_CHANNEL_BIND_REQUEST:  //= 0x0009,
	case STUN_SHARED_SECRET_REQUEST:  //= 0x0002,
	case STUN_CREATE_PERM_REQUEST:  //= 0x0008,
		WARN("[ICE-%s] Unexpected Peer Request: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
		return;
		// I don't know what to do with indications???
	case STUN_BINDING_INDICATION:  //= 0x0011,
	case STUN_SEND_INDICATION:  //= 0x0016,
	case STUN_DATA_INDICATION:  //= 0x0017,
		WARN("[ICE-%s] Unexpected Peer Indication: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
		return;
	case STUN_BINDING_RESPONSE:  //= 0x0101,
		// Set state of candidate only if not using a relay!
		if (!isUsingRelay) {
			pCan->SetStatePierceOut();
		}

		_state |= ST_PIERCE_OUT;
		break;
	case STUN_SHARED_SECRET_RESPONSE:  //= 0x0102,
	case STUN_ALLOCATE_RESPONSE:  //= 0x0103,
	case STUN_REFRESH_RESPONSE:  //= 0x0104,
	case STUN_CREATE_PERM_RESPONSE:  //= 0x0108,
	case STUN_CHANNEL_BIND_RESPONSE:  //= 0x0109,
		WARN("[ICE-%s] Unexpected Peer Response: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
		break;
	case STUN_BINDING_ERROR_RESPONSE:	// = 0x0111,
	case STUN_ALLOCATE_ERROR_RESPONSE:  //= 0x0113,
	case STUN_SHARED_SECRET_ERROR_RESPONSE:  //= 0x0112,
	case STUN_REFRESH_ERROR_RESPONSE:  //= 0x0114,
	case STUN_CREATE_PERM_ERROR_RESPONSE:  //= 0x0118,
	case STUN_CHANNEL_BIND_ERROR_RESPONSE:  //= 0x0119
		{
			WARN("[ICE-%s] Unexpected Peer Error Response: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
			int code = 0;
			string errStr;
			if (pMsg->GetErrorCodeAttr(code, errStr)) {
				WARN("Wrtc: Error: 0x%x, %s", code, STR(errStr));
			}
		}
		break;
	default:
		WARN("[ICE-%s] Unknown Peer Message: 0x%x", (_isTcp ? STR("TCP") : STR("UDP")), type);
	}
}

void BaseIceProtocol::SendPeerBindMsg(Candidate * pCan, bool select, bool isUsingRelay) {
	uint8_t *transId = NULL;

	if ((_candidateType != "all") && (_candidateType != pCan->GetType())) {
		//WARN("Not peering with %s", STR(pCan->GetShortString()));
		return;
	}

	// Reuse transaction ID if we haven't pierced out yet
	if (!pCan->IsStatePierceOut()) {
		transId = pCan->GetLastTurnTxId();
	}

	StunMsg * pMsg = StunMsg::MakeMsg(STUN_BINDING_REQUEST, transId);
	pMsg->AddStringAttr(STUN_ATTR_USERNAME, _iceUserUser);

	// This Priority should be in reflex-candidate range
	// but Firefox is rejecting after 10 of these
	// so maybe it needs to "move"
	static uint32_t differ = 0;	// use this to make pri unique in a given "session"
	pMsg->AddPriority(1853824767 + (0xfff & differ++));
	if (_iceControlling) {
		pMsg->AddIceControlling();
		
		// Only add use candidate if we are controlling
		if (select) {
			pMsg->AddUseCandidate();
		}
	} else {
		pMsg->AddIceControlled();
	}
	pMsg->AddMessageIntegrity(_peerIcePwd);

	// Add the fingerprint for the peer
	pMsg->AddFingerprint();

	// Update the transaction Id. Moved here since pMsg will get deleted.
	pCan->SetLastTurnTxId(pMsg->GetTransId());

	SendData(pMsg, pCan, isUsingRelay);
}

//
// UDP Methods
//
bool BaseIceProtocol::SendDataToTurnCandidate(uint8_t * pData, int len, Candidate * pCan) {
	StunMsg * pMsg = StunMsg::MakeMsg(STUN_SEND_INDICATION);
	// now add the XOR_PEER_ADDRESS for this candidate
	pMsg->AddAddressXOR(STUN_ATTR_XOR_PEER_ADDR, pCan->GetIpStr());
	pMsg->AddData(pData, len);	// can fail, but don't bother
	
	// IPv6 and dual stack support
	bool ok = SendTo(_turnServerAddress.getIPv4(), _turnServerAddress.getSockPort(), 
			pMsg->GetMessage(), pMsg->GetMessageLen());
	delete pMsg;

	return ok;
}

bool BaseIceProtocol::SendData(StunMsg *pMsg, Candidate *pCan, bool useRelay) {
	bool ret = false;

	if (useRelay) {
		ret = SendDataToTurnCandidate(pMsg->GetMessage(), pMsg->GetMessageLen(), pCan);
		delete pMsg; // to be consistent with old code and avoid leak
	} else {
		ret = SendStunMsgTo(pMsg, pCan->GetIpStr());
	}

	return ret;
}

// SendStunMsgTo Owns and deletes pMsg!!!
//
bool BaseIceProtocol::SendStunMsgTo(StunMsg * pMsg, string & ipStr) {
//	INFO("$b2$ [%s]Sending stun type: 0x%x to %s", 
//		STR(_hostIpStr), pMsg->GetType(), STR(ipStr));
	uint8_t * pData = pMsg->GetMessage();
	int len = pMsg->GetMessageLen();
	bool ok = false;

	SocketAddress destAddr;
	destAddr.setIPPortAddress(ipStr);
	ok = SendTo(destAddr.getIPv4(), destAddr.getSockPort(), pData, len);

	delete pMsg;

	return ok;
}

bool BaseIceProtocol::SendToV4(uint32_t ip, uint16_t port, uint8_t * pData, int len) {
	NYIA;
}

bool BaseIceProtocol::SendTo(string ip, uint16_t port, uint8_t * pData, int len) {
	NYIA;
}

//
// worker routines
//
Candidate * BaseIceProtocol::FindCandidate(string ipStr) {
	if (ipStr.size()) {
		// we have an ipStr, go look for it
		FOR_VECTOR(_cans, i) {
			if (_cans[i]->IsSameIp(ipStr)) {
				return _cans[i];
			}
		}
	}
	return NULL;
}

Candidate * BaseIceProtocol::FindCandidateByAddressAttr(StunMsg * pMsg) {
	string ipStr;
	pMsg->GetXORAddressAttr(STUN_ATTR_XOR_MAPPED_ADDR, ipStr);
	if (!ipStr.size()) {
		// hmm, maybe we have it not XOR'd???
		pMsg->GetAddressAttr(STUN_ATTR_MAPPED_ADDR, ipStr);
	}
	return FindCandidate(ipStr);
}

Candidate * BaseIceProtocol::FindCandidateByTransId(uint8_t * pTxId) {
	if (pTxId) {
		// we have an pointer, loop thru and compare
		FOR_VECTOR(_cans, i) {
			if (_cans[i]->IsLastTurnTxId(pTxId)) {
				return _cans[i];
			}
		}
	}
	return NULL;
}

bool BaseIceProtocol::DecRetryToZero(int & counter) {
	if (counter) counter--;
	return 0 == counter;
}

string & BaseIceProtocol::GetUserName() {
	if (!_username.size()) {
		//$ToDo$ find rules on making this name!!
		_username = "fN8HKrDf8gOWWMtv:ZWWKMf0O5KTmexid";
	}
	return _username;
}

uint32_t BaseIceProtocol::timeDiff() {
	// diff between now and startMs
	time_t now;
	GETMILLISECONDS(now);
	return (uint32_t)(0xFFFFFFFF & (now - _startMs));
}

string & BaseIceProtocol::GetTurnKeyStr() {
	if (!_turnKeyStr.size()) {
		string tmp = _turnUsername + ":" + _turnRealm + ":" + _turnCredential;
		//DEBUG("TurnKeyStr = %s", STR(tmp));
		_turnKeyStr = md5(tmp, false);	// true=text result
		//DEBUG("Figured _turnKeyStr: %s", STR(_turnKeyStr));
		//
		// hash check!
		//
		//string t = "user:realm:pass";
		//DEBUG("CHECK!! md5 should be: 0x8493fbc53ba582fb4c044c456bdc40eb");
		//string q = md5(t, true);
		//FATAL("Test md5 is: %s", STR(q));
	}
	return _turnKeyStr;
}
//
/////////////////////////////////////////////////////////
// Check Routines - Compare Created STUN with captured //
// ... chrome Peer Bind Request
/////////////////////////////////////////////////////////

uint8_t char2int(char input) {
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  return 0;	// succeed in any event
}

void strToArray(string s, uint8_t * p) {
	size_t i;
	for (i=0; i < s.size(); i += 2) {
		*p++ = (16 * char2int(s[i]) + char2int(s[i+1]));
	}
}
#ifdef WEBRTC_DEBUG
void BaseIceProtocol::Check() {
	uint8_t transId[12];
	uint8_t integrity[20];
	uint32_t crc = 0xe0257999;
	uint64_t tie = 0xd394f539ec1cba0f;
	uint32_t pri = 1853562623;
	string tidStr = "5a503359733977786a793956";
	string userId = "LmnS5j9e5r+WvDFc:MWuzumtsn11gVdXa";
	string iStr = "69c6aa5da6f773a51ce6b1de373cf3757beefb13";
	//string key = "1438714990:myrealm:sNLcTxHddjaPsBQkhPzr91HD/zs=";
	string key = "qknS1CqWYojzHJ3w0BnZkeGU";
	//
	// make byte arrays
	strToArray(tidStr, transId);
	strToArray(iStr, integrity);
	//
	// construct it
	StunMsg msg;
	msg.MakeHdr(STUN_BINDING_REQUEST, transId);
	msg.AddUser(userId);
	msg.AddIceControlling(tie);
	msg.AddPriority(pri);
	msg.AddMessageIntegrity(key);
	msg.AddFingerprint();
	//
	// now check it
	bool match = true;
	int a = msg.FindAttribute(STUN_ATTR_MESSAGE_INTEGRITY);
	uint8_t * p = msg.GetMessage();
	a += 4;	// bump past header to point at integrity str
	for(int i=0; i<20; i++) {
		if (p[a+i] != integrity[i]) {
			match = false;
			break;
		}
	}
	//
	// put it on the wire
	//
	SendToV4(0xc0a80170, 0xe178, p, msg.GetMessageLen());
	//
	if (!match) {
		FATAL("$b2$$b2$ MESSAGE CHECK FAILED FAILED FAILED");
	}else {
		FATAL("$b2$$b2$ MESSAGE CHECK SUCCEEDED!!!!!!");
	}
	a = msg.FindAttribute(STUN_ATTR_FINGERPRINT);
	uint32_t x = msg.GetMsg32(a+4);
	if (x == crc) {
		FATAL("$b2$$b2$ CRC CHECK SUCCEEDED!!!!!!");
	} else {
		FATAL("$b2$$b2$ CRC CHECK FAILED 0x%x vs 0x%x", x, crc);
	}
	//
	// try newer crc routine
	//uint32_t xxx = crc32(p, a);
	//FATAL("$b2$$b2$ NEW CRC CHECK 0x%x vs 0x%x",0x5354554e ^ xxx, crc);

}
#endif
void BaseIceProtocol::SetBestCandidate(Candidate *pCan) {
//#ifdef WEBRTC_DEBUG
	DEBUG("New Best CAN [%s]", STR(pCan->GetLongString()));
//#endif
	_bestCan = pCan; // new best!!
	_bestIpStr = _bestCan->GetIpStr();
	_bestAddress.setIPPortAddress(_bestIpStr);
	_bestPriority = _bestCan->GetPriority();
}

void BaseIceProtocol::FinishInitialization() {

}

void BaseIceProtocol::Terminate(bool externalCall) {
	_isTerminating = true;

	if (externalCall) {
		// Reset the pointers if called externally
		_pWrtc = NULL;
		_pSig = NULL;
	}

	// Then schedule self for deletion
	EnqueueForDelete();
}

#endif // HAS_PROTOCOL_WEBRTC
