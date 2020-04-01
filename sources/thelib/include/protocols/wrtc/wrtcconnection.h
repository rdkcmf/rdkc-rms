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
* wrtcconnection.h - create and funnel data through a WebRTC Connection
*
**/

#ifdef HAS_PROTOCOL_WEBRTC
#ifndef _WRTCCONNECTION_H
#define	_WRTCCONNECTION_H

#define WRTC_CAPAB_STR_HAS_HEARTBEAT "hasHeartbeat"

#include "protocols/rtp/basertspprotocol.h"
#include "protocols/rtp/rtcpreceiver.h"
#include "protocols/wrtc/iceutils.h"
#include "protocols/timer/basetimerprotocol.h"
#include "protocols/ssl/basesslprotocol.h"
#include "protocols/sctpprotocol.h"
#include "streaming/fmp4/outnetfmp4stream.h"
#include "streaming/mp4/outnetmp4stream.h"
#include "utils/misc/x509certificate.h"
#include "streaming/outdevicetalkdownstream.h"

// RT Msg includes for RMS to communicate to LED Manager
#ifdef RTMSG
#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"
#endif

/* Peer States in Wrtc: Valid Peer States are {"new", "sent_offer", "received_offer", "sent_answer", "received_answer", "active", "closed"} */
typedef enum PeerState {
    WRTC_PEER_NEW,
    WRTC_PEER_SENT_OFFER,
    WRTC_PEER_RECEIVED_OFFER,
    WRTC_PEER_SENT_ANSWER,
    WRTC_PEER_RECEIVED_ANSWER,
    WRTC_PEER_ACTIVE,
    WRTC_PEER_CLOSED
} PeerState;

/* Ice States in Wrtc: Valid Ice States are { "new", "checking", "connected", "failed", "disconnected", "closed" } */
typedef enum IceState {
    WRTC_ICE_NEW,
    WRTC_ICE_CHECKING,
    WRTC_ICE_CONNECTED,
    WRTC_ICE_FAILED,
    WRTC_ICE_DISCONNECTED,
    WRTC_ICE_CLOSED
} IceState;

class WrtcSigProtocol;
class Candidate;
class BaseIceProtocol;
class WrtcSDP;
class SCTPProtocol;
struct SDPInfo;

struct MediaContext{
	bool hasAudio;
	bool hasVideo;
};

/**
 * Supported features for this session
 */
struct Capabilities {
	bool hasHeartbeat;
	//TODO: include other features
};

class OutNetRTPUDPH264Stream;
class OutboundConnectivity;
class InboundConnectivity;
class DLLEXP WrtcConnection
: public BaseRTSPProtocol, public IceUtils {
public:
	WrtcConnection();
	WrtcConnection(WrtcSigProtocol * pSig, Variant & settings);
	virtual ~WrtcConnection();

	void Start(bool hasNewCandidate = false);  // SigProtocol tells us to start ICE
	void Stop();
	bool Tick();	// timer triggered, return true to trigger again???
	bool FastTick(); // FAST timer triggered, return true to trigger again???
	bool SlowTick(); // SLOW timer triggered, return true to trigger again???

	void OverrideIceMaxRetries(int newMaxRetries);

	//
	// setters called from WrtcSigProtocol
	bool SetStunServer(string ipPort);	// URL string: "x.y.z.z:pppp"
	bool SetTurnServer(string username, string credential, string ipPort);
	bool SetSDP(string sdp, bool offer);
	bool SetCandidate(Candidate * pCan); // assume owernership of pCan
	bool SetStreamName(string streamName, bool useSrtp);
	void SetClientId( string clientId );
	void SetRMSClientId( string rmsClientId );
	bool IsWaiting();

	bool SetControlled(bool isControlled = true);
	// Signal Server Access
	WrtcSigProtocol * GetSig() { return _pSig;};
	void UnlinkSignal(WrtcSigProtocol *pSig, bool external = true);
	uint32_t GetCommandChannelId();

	//set sctp protocol tear down status
	bool  IsSctpAlive();
	void  SetSctpAlive(bool sctpalive);

	// BaseProtocolOverrides
	//
	virtual bool Initialize(Variant &parameters);
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool AllowFarProtocol(uint64_t type) ;
	virtual bool AllowNearProtocol(uint64_t type) ;
	virtual bool SignalInputData(int32_t recvAmount) ;
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool EnqueueForOutbound();
	virtual IOBuffer *GetOutputBuffer();
	virtual bool SendOutOfBandData(const IOBuffer &buffer, void *pUserData);
	virtual void SignalOutOfBandDataEnd(void *pUserData);
	virtual void SignalEvent(string eventName, Variant &args);

	/**
	 * Event triggered when SCTP data channel is created.
	 * 
     * @param name Name of channel
     * @param id ID of channel
     * @return 
     */
	bool SignalDataChannelCreated(const string &name, uint32_t id);

	/**
	 * Even triggered when a data is available on a data channel.
	 * 
     * @param id Channel ID
     * @param pBuffer Pointer to the actual data
     * @param size Length of the data in bytes
     * @return 
     */
	bool SignalDataChannelInput(uint32_t id, const uint8_t *pBuffer, const uint32_t size);
	
	/**
	 * Event triggered when channel is ready for sending out data
	 * 
     * @param id Channel ID
     * @return 
     */
	bool SignalDataChannelReady(uint32_t id);
	
	/**
	 * Event triggered when a data channel is closed.
	 * 
     * @param id Channel ID
     * @return 
     */
	bool SignalDataChannelClosed(uint32_t id);

	/**
	 * Handles metadata push through webrtc.
	 * 
	 * @param metadata
	 * @return 
	 */
	bool PushMetaData(Variant &metadata);

	bool HasAudio();
	bool HasVideo();

	bool SendWebRTCCommand(Variant& settings);


	bool SetupPlaylist(string const &streamName);
	bool SetupPlaylist(string const &streamName, Variant &config, bool appendRandomChars = false);
	bool SetupLazyPull(string const &streamName);
	bool HandlePauseCommand();
	bool HandleResumeCommand();
	
	/**
	 * Sets the peer's supported features of this webrtc session.
	 * 
	 * @param capabilities List of features separated by comma
	 */
	void SetPeerCapabilities(string capabilities);
	
	/**
	 * Retrieves the set of supported features 
	 * 
	 * @param isRms Determines if capabilities requested is that of RMS (player if false)
	 * @return Set of supported features.
	 */
	Capabilities GetCapabilities(bool isRms);
	
	/**
	 * Removes the ICE instance from the list that webrtc session manages.
	 * 
	 * @param pIce ICE protocol instance to remove
	 */
	void RemoveIceInstance(BaseIceProtocol *pIce);
	
	/**
	 * Indicates if this webrtc session has started with the peering process.
	 * 
	 * @return true if peering has started, false otherwise
	 */
	bool HasStarted() { return _started; };

#ifdef RTMSG
	void notifyLedManager(int led_stat);
#endif

	bool SetPeerState(PeerState peer_state);

	string GetPeerState();
private:

#ifdef RTMSG
	rtMessage m;
	rtError err;
#endif

	/**
	 * Initializes DTLS and SCTP that will be used for our data channel
	 * 
     * @return 
     */
	void HasAudio(bool value);
	void HasVideo(bool value);
	bool InitializeDataChannel();
	
	bool HandleDataChannelMessage(Variant &msg);
	bool SendStreamAsMp4(string &streamName);
	bool SendCommandData(string &message);
	
	bool SpawnIceProtocols();
	bool GetInterfaceIps(vector<string> & ips);
	
	bool InitializeCertificate();

	bool InitializeSrtp();

	string GetBaseStreamName(string const &streamName);
	bool IsSameStream(string const &localStreamName, string const &streamName);
	bool IsVod(string streamName);
	bool IsLazyPull(string streamName);
	BaseInStream *RetrieveInStream(string streamName, StreamsManager *pSM);
	bool CreateOutStream(string streamName, BaseInStream *pInStream, StreamsManager *pSM);
	bool CreateInStream(string streamName, StreamsManager *pSM);
	BaseOutStream *GetOutboundStream();

	/**
	 * Sets the supported features of RMS for this session
	 */
	void SetRmsCapabilities();

	bool _started;
	bool _isControlled;
	bool _stopping;
	bool _retryConnectionProcess;
	bool _sctpAlive;

	// to check the state of ssl handshake
	bool _checkHSState;

	
	uint8_t _slowTimerCounter;

	// SDP stuff
	bool _gotSDP;
	bool _sentSDP;
	WrtcSDP * _pSDP;
	SDPInfo * _pSDPInfo;

	// the ultimate winner BaseIceProtocol:
	BaseIceProtocol * _bestIce;

	WrtcSigProtocol * _pSig;
	string _stunServerIpStr;
	vector<BaseIceProtocol *> _ices;
	MediaContext _mediaContext;
	string _stunDomain;
	string _turnDomain;
	vector<string> _stunIps;
	vector<string> _turnIps;
	string _turnUser;
	string _turnCreds;
	bool _createdIces;
	//uint64_t _baseMs;

	//TODO: we might not need this reference if we're linked as a protocol chain?
	BaseSSLProtocol *_pDtls;
	SCTPProtocol *_pSctp;
	BaseOutStream *_pOutNetMP4Stream;
	OutboundConnectivity *_pOutboundConnectivity;
	OutNetRTPUDPH264Stream *_pOutStream;
	bool _fullMp4;

	InboundConnectivity *_pInboundConnectivity;
	OutDeviceTalkDownStream *_pTalkDownStream;
	bool _is2wayAudioSupported; // flag to identify the 2 way support of the peer

	IOBuffer _outputBuffer;
	IOBuffer _outbandBuffer;
	uint32_t _streamChannelId; // channel used for streaming the fmp4s
	uint32_t _commandChannelId; // channel used to exchange command or flow control

	bool _dataChannelEstablished; // indicates if a channel has been created
	bool _isStreamAttached; // indicates if there's an active stream attached

	X509Certificate *_pCertificate;

	IOBuffer _moovBuffer;

	string _cmdReceived;
	string _streamName;
	string _clientId;
	string _rmsClientId;
	bool _isPlaylist;
	bool _useSrtp;

	RTCPReceiver _rtcpReceiver;
	
	// Capabilities
	Capabilities _capabPeer;
	Capabilities _capabRms;
	
	//
	// Timer Stuff
	//
	class WrtcTimer : public BaseTimerProtocol {
	public:
		WrtcTimer(WrtcConnection * pWrtc);
		virtual ~WrtcTimer();
		static WrtcTimer * CreateFastTimer(uint32_t ms, WrtcConnection * pWrtc);
		static WrtcTimer * CreateSlowTimer(uint32_t secs, WrtcConnection * pWrtc);
		// the timer override
		virtual bool TimePeriodElapsed();
	private:
		WrtcConnection * _pWrtc;
	};
	WrtcTimer * _pSlowTimer;
	WrtcTimer * _pFastTimer;

	bool _waiting;
	uint32_t _pendingFlag;
	BaseInStream *_pPendingInStream;
	//
	void StartFastTimer();
	void StartSlowTimer();
	void RemovePlaylist();
	
	uint32_t _ticks;
#ifdef WRTC_CAPAB_HAS_HEARTBEAT
	uint8_t _hbNoAckCounter;
	bool SendHeartbeat();
	void ProcessHeartbeat();
#endif // WRTC_CAPAB_HAS_HEARTBEAT
	
	static uint32_t _sessionCounter;

	/**
	 * Resolves an domainName:port string into IP:port string format.
	 * 
	 * @param domainName Domain name:port to resolve
	 * @param resolvedName Resolved string format
	 * @return true on success, false otherwise
	 */
	bool ResolveDomainName(string domainName, vector<string> &resolvedIPs);
};

#endif	/* _WRTCCONNECTION_H */
#endif // HAS_PROTOCOL_WEBRTC

