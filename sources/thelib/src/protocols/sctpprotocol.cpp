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

#include "protocols/sctpprotocol.h"
#include "netio/netio.h"
#include "usrsctp.h"

uint32_t SCTPProtocol::_initCounter = 0;
#ifdef SCTP_USE_SENDSPACE
uint32_t SCTPProtocol::_sendSpace = 0;
#endif

SCTPProtocol::SCTPProtocol()
: BaseProtocol(PT_SCTP) {
	_decodedBytesCount = 0;
	_pCarrier = NULL;
	
	_isReadyForInitialization = false;

	_pSCTPSocket = NULL;
	_maxSctpChannels = 10;
	memset(&_received, 0, sizeof (_received));
	_received.bufferSize = SCTP_MAX_RECEIVE_BUFFER;
	//_received.pBuffer = new uint8_t[_received.bufferSize];
	_received.fromLength = sizeof (_received.from);
	_received.infoLength = sizeof (_received.info);
	_oddChannels = false;
	_channelIdGenerator = 1;
	_initAckReceived = false;
	_channelValdityCheck = 0;
	_pWrtc = NULL;
}

SCTPProtocol::~SCTPProtocol() {
//#ifdef WEBRTC_DEBUG
	FINE("SCTPProtocol deleted.");
//#endif
	if (_pCarrier != NULL) {
		IOHandler *pCarrier = _pCarrier;
		_pCarrier = NULL;
		pCarrier->SetProtocol(NULL);
		delete pCarrier;
	}
	
	// Unregister the ULP
	usrsctp_deregister_address(this);

	// Close the socket
	if (_pSCTPSocket != NULL) {
		usrsctp_set_ulpinfo(_pSCTPSocket, NULL);
		usrsctp_shutdown(_pSCTPSocket, SHUT_RDWR);
		usrsctp_close(_pSCTPSocket);
		_pSCTPSocket = NULL;
	}

	// Decrement the instance count and kill the library if we hit 0
	_initCounter--;
	if (_initCounter == 0)
		usrsctp_finish();

	memset(&_received, 0, sizeof (_received));

	// Cleanup the channels
	for (uint16_t i = 0; i < _maxSctpChannels; i++) {
		if (_ppAllChannelsBySID[i] != NULL)
			delete _ppAllChannelsBySID[i];
	}

	// Clean-up
	delete[] _ppAllChannelsBySID;

        //set sctp stop on wrtcconnection object
	if(NULL != _pWrtc) {
		_pWrtc->SetSctpAlive(false);
	}
}

bool SCTPProtocol::Initialize(Variant &parameters) {
	if (!_isReadyForInitialization) {
		_customParameters = parameters;
		return true;
	}

	//TODO: Add checking here of the parameters
	uint16_t sctpPort = _customParameters["sctpPort"];
	_maxSctpChannels = _customParameters["sctpMaxChannels"];
	_oddChannels = !_customParameters["sctpClient"];
	
	// Allocate the space required for all channels
	_ppAllChannelsBySID = new SctpChannel *[_maxSctpChannels];
	memset(_ppAllChannelsBySID, 0, sizeof (SctpChannel *) * _maxSctpChannels);

	// Check and see if we need to initialize the library
	if (_initCounter == 0) {
		// Initialize the SCTP library
		usrsctp_init(0, CallbackSendData, CallbackDebugPrint);

		// Disable Explicit Congestion Notification (ECN))
		usrsctp_sysctl_set_sctp_ecn_enable(0);
#ifdef SCTP_USE_SENDSPACE
		// Setup the send space
		if (_sendSpace == 0)
			_sendSpace = SCTP_DEFAULT_SEND_SPACE;
		usrsctp_sysctl_set_sctp_sendspace(_sendSpace);
		_sendSpace = usrsctp_sysctl_get_sctp_sendspace();
#endif
		usrsctp_sysctl_set_sctp_nr_outgoing_streams_default(_maxSctpChannels - 1);
	}

	// Bump the instance counter
	_initCounter++;

	// Create an SCTP socket
	if ((_pSCTPSocket = usrsctp_socket(AF_CONN, SOCK_STREAM, IPPROTO_SCTP,
			CallbackReceiveData, NULL, 0, this)) == NULL) {
		FATAL("Unable to create the SCTP socket");
		return false;
	}

	// Make it non-blocking
	if (usrsctp_set_non_blocking(_pSCTPSocket, 1) != 0) {
		FATAL("Unable to set the SCTP socket in non-blocking mode");
		return false;
	}

	// This ensures that the usrsctp close call deletes the association. This
	// prevents usrsctp from calling CallbackSendData with references to
	// this class as the address.
	struct linger l;
	l.l_onoff = 1;
	l.l_linger = 0;
	if (usrsctp_setsockopt(_pSCTPSocket, SOL_SOCKET, SO_LINGER, &l, sizeof (l))) {
		FATAL("Unable to set the linger options on the SCTP socket");
		return false;
	}

	// Enable stream ID resets (add/remove channels on the fly).
	struct sctp_assoc_value sav;
	sav.assoc_id = SCTP_ALL_ASSOC;
	sav.assoc_value = 1;
	if (usrsctp_setsockopt(_pSCTPSocket, IPPROTO_SCTP, SCTP_ENABLE_STREAM_RESET,
			&sav, sizeof (sav))) {
		FATAL("Unable to set stream ID resets on the SCTP socket");
		return false;
	}

	// Disable Nagle algorithm (no delay)
	uint32_t on = 1;
	if (usrsctp_setsockopt(_pSCTPSocket, IPPROTO_SCTP, SCTP_NODELAY, &on,
			sizeof (on))) {
		FATAL("Unable to set SCTP_NODELAY on the SCTP socket");
		return false;
	}

	// Disable MTU discovery
	struct sctp_paddrparams params;
	memset(&params, 0, sizeof (params));
	params.spp_assoc_id = 0;
	params.spp_flags = SPP_PMTUD_DISABLE;
	params.spp_pathmtu = SCTP_PATH_MTU;
	if (usrsctp_setsockopt(_pSCTPSocket, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &params,
			sizeof (params))) {
		FATAL("Unable to disable MTU discovery on the SCTP socket");
		return false;
	}

	// Subscribe to SCTP event notifications.
	int selectedEvents[] = {
		//to detect when the connection was really made or when disconnected
		//https://tools.ietf.org/html/draft-ietf-tsvwg-sctpsocket-18#section-6.3.10
		SCTP_ASSOC_CHANGE,
		
		// Subscribe to this two events
		SCTP_PEER_ADDR_CHANGE,
		SCTP_SEND_FAILED_EVENT,

		//we need this to inform the upper layers about the readiness of the connection
		//https://tools.ietf.org/html/draft-ietf-tsvwg-sctpsocket-18#section-6.3.10
		SCTP_SENDER_DRY_EVENT,

		//we need this for opening/closing streams (also called channels)
		//https://tools.ietf.org/html/rfc6525#section-6.1.1
		SCTP_STREAM_RESET_EVENT
	};

	struct sctp_event event;
	memset(&event, 0, sizeof (event));
	event.se_assoc_id = SCTP_ALL_ASSOC;
	event.se_on = 1;
	for (size_t i = 0; i < sizeof (selectedEvents) / sizeof (selectedEvents[0]); i++) {
		event.se_type = selectedEvents[i];
		if (usrsctp_setsockopt(_pSCTPSocket, IPPROTO_SCTP, SCTP_EVENT, &event,
				sizeof (event)) < 0) {
			FATAL("Unable to subscribe to event notification %d on the SCTP socket", event.se_type);
			return false;
		}
	}

	// Register the underlying protocol
	usrsctp_register_address(this);

	// Initialize the SCTP connecting address
	struct sockaddr_conn connectAddress;
	memset(&connectAddress, 0, sizeof (struct sockaddr_conn));
#ifdef HAVE_SCONN_LEN
	connectAddress.sconn_len = sizeof(sockaddr_conn);
#endif
	connectAddress.sconn_family = AF_CONN;
	connectAddress.sconn_port = EHTONS(sctpPort);
	connectAddress.sconn_addr = this;

	// Bind the socket
	if (usrsctp_bind(_pSCTPSocket, (struct sockaddr *) &connectAddress,
			sizeof (struct sockaddr_conn)) != 0) {
		int err = errno;
		FATAL("Unable to bind to the SCTP socket: (%d) %s", err, strerror(err));
		return false;
	}	
	
	return true;
}
#ifdef HAS_PROTOCOL_WEBRTC
bool SCTPProtocol::Start(WrtcConnection *pWrtc) {
	_isReadyForInitialization = true;
#ifdef HAS_PROTOCOL_WEBRTC
	_pWrtc = pWrtc;
#endif
	// Do the actual initialization
	if (!Initialize(_customParameters)) {
		return false;
	}
	
	return true;
}
#endif
IOHandler *SCTPProtocol::GetIOHandler() {
	return _pCarrier;
}

void SCTPProtocol::SetIOHandler(IOHandler *pIOHandler) {
	if (pIOHandler != NULL) {
		if (pIOHandler->GetType() != IOHT_UDP_CARRIER) {
			ASSERT("This protocol accepts only UDP carrier");
		}
	}
	_pCarrier = pIOHandler;
}

bool SCTPProtocol::AllowFarProtocol(uint64_t type) {
	// For now, we limit this to be on top of DTLS
	if ((type == PT_OUTBOUND_DTLS) || (type == PT_INBOUND_DTLS)) {
		return true;
	}

	return false;
}

bool SCTPProtocol::AllowNearProtocol(uint64_t type) {
	return true;
}

IOBuffer * SCTPProtocol::GetInputBuffer() {
	return &_inputBuffer;
}

bool SCTPProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("Operation not supported");
	return false;
}

bool SCTPProtocol::SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress) {
	ASSERT("Operation not supported");
	return false;
}

bool SCTPProtocol::SignalInputData(IOBuffer &buffer) {
	uint8_t *pData = GETIBPOINTER(buffer);
	uint32_t length = GETAVAILABLEBYTESCOUNT(buffer);

	// initAck received, and we just got an init
	if ((_initAckReceived)&&(length >= 13)&&(pData[12] == 0x01)) {
		WARN("Invalid packet.");
		buffer.Ignore(length);
		return true;
	}

//	INFO("Input Bytes (%d): %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X -- %2X", length,
//		pData[0], pData[1], pData[2], pData[3],
//		pData[4], pData[5], pData[6], pData[7], pData[12]
//	);
//	VerboseLogPacket(pData, length, SCTP_DUMP_INBOUND);

	if(pData[12] == 1){
		uint16_t sctpPort = _customParameters["sctpPort"];
		
		// Initialize the SCTP connecting address
		struct sockaddr_conn connectAddress;
		memset(&connectAddress, 0, sizeof (struct sockaddr_conn));
	#ifdef HAVE_SCONN_LEN
		connectAddress.sconn_len = sizeof(sockaddr_conn);
	#endif
		connectAddress.sconn_family = AF_CONN;
		connectAddress.sconn_port = EHTONS(sctpPort);
		connectAddress.sconn_addr = this;

		// Connect the socket
		if (usrsctp_connect(_pSCTPSocket, (struct sockaddr *) &connectAddress,
			sizeof (struct sockaddr_conn)) != 0) {
			int err = errno;
			if ((err != EINPROGRESS)&&(err != EWOULDBLOCK)&&(err != EAGAIN)) {
				FATAL("Unable to connect the SCTP socket: (%d) %s", err, strerror(err));
				return false;
			}
		}

		if (!CreateDefaultChannel()) {
			return false;
		}
	}

	if (!_initAckReceived) {
		_initAckReceived = (length >= 13)&&(pData[12] == 0x02);
	}
	
	// Feed the raw data into SCTP stack
	usrsctp_conninput(this, pData, length, 0);

	// Clear the buffer
	buffer.Ignore(length);

	return true;
}

bool SCTPProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	if (_pNearProtocol != NULL)
		return _pNearProtocol->SignalInputData(buffer, pPeerAddress);
	return false;
}

bool SCTPProtocol::EnqueueForOutbound() {

/*	if (_pNearProtocol != NULL && _pNearProtocol->GetType() == PT_INBOUND_DTLS) {
		IOBuffer *pNearBuff = _pNearProtocol->GetOutputBuffer();
		if (pNearBuff != NULL && (GETIBPOINTER(*pNearBuff)[0] & 0x80 == 0x80)) {
			_outputBuffer.ReadFromBuffer(GETIBPOINTER(*pNearBuff), GETAVAILABLEBYTESCOUNT(*pNearBuff));
			pNearBuff->IgnoreAll();
		}
	}*/
	if (_pFarProtocol != NULL) {
		// SCTP is used on top of another protocol
		return _pFarProtocol->EnqueueForOutbound();
	}

	// Otherwise, SCTP is used as a transport and attached to the carrier
	if (_pCarrier == NULL) {
		// Since we're not using this as a carrier for now, remove this error
		//FATAL("SCTPProtocol has no carrier!");
		return false;
	}

	return _pCarrier->SignalOutputData();
}

bool SCTPProtocol::CreateDefaultChannel() {
	if (CreateOutboundChannel(SCTP_STREAM_CHANNEL) < 0) {
		FATAL("Failed to create outbound channel!");
		return false;
	}

	return true;
}

IOBuffer * SCTPProtocol::GetOutputBuffer() {
	return &_outputBuffer;
}

uint64_t SCTPProtocol::GetDecodedBytesCount() {
	return _decodedBytesCount;
}

void SCTPProtocol::CallbackDebugPrint(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	printf("\t");

	vprintf(format, ap);
	va_end(ap);
}

int SCTPProtocol::CallbackSendData(void *pAddress, void *pBuffer, size_t length, 
		uint8_t tos, uint8_t set_df) {
	uint8_t *pData = (uint8_t *) pBuffer;

//	VerboseLogPacket(pAddress, length, SCTP_DUMP_OUTBOUND);
//	DEBUG("Output Bytes (%d): %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X -- %2X", (int)length,
//		pData[0], pData[1], pData[2], pData[3],
//		pData[4], pData[5], pData[6], pData[7], pData[12]
//	);
	
	(((SCTPProtocol *) pAddress)->_outputBuffer).ReadFromBuffer(pData, (uint32_t) length);
	((SCTPProtocol *) pAddress)->EnqueueForOutbound();
	
	return 0;
}

int SCTPProtocol::CallbackReceiveData(struct socket *sock, union sctp_sockstore addr,
		void *data, size_t length, struct sctp_rcvinfo rcv, int flags, void* ulp_info) {
	// Flag that will indicate if we can continue processing the data
	bool keepGoing = true;

//	uint8_t *pData = (uint8_t *) data;
//	DEBUG("Processed Input Bytes (%d): %2X,%2X,%2X,%2X,%2X,%2X,%2X,%2X -- %2X", (int)length,
//		pData[0], pData[1], pData[2], pData[3],
//		pData[4], pData[5], pData[6], pData[7], pData[12]
//	);

	SCTPProtocol *sctp = (SCTPProtocol *) ulp_info;

	ReceiveContext &received = sctp->_received;
	received.fromLength = sizeof (received.from);
	received.infoLength = sizeof (received.info);
	received.amount = length;
	received.pBuffer = (uint8_t *) data;
	received.bufferSize = length;
	received.info = rcv;
	received.flags = flags;

	if ((received.flags & MSG_NOTIFICATION) != 0) {
		sctp->HandleNotifications();
		keepGoing = false; // exit once the notifications are handled
	} else if (received.info.rcv_sid >= sctp->_maxSctpChannels) {
		FATAL("Invalid session ID!");
		keepGoing = false;
	}

	if (keepGoing && (sctp->_ppAllChannelsBySID[received.info.rcv_sid] == NULL)) {
		//try to create the missing channel
		if (!sctp->CreateInboundChannel()) {
			FATAL("Unable to create inbound channel");
		}

		// Exit once inbound channels are created
		keepGoing = false;
	}

	if (keepGoing) {
		SctpChannel *pChannel = sctp->_ppAllChannelsBySID[received.info.rcv_sid];
		switch (pChannel->_state) {
			case CHANNEL_STATE_CREATE_REQUEST_SENT:
			{
				//if the channel was in CHANNEL_STATE_CREATE_REQUEST_SENT state,
				//than this first message must be the confirmation

				//test the message
				if ((received.amount <= 0) || (received.pBuffer[0] != SCTP_DATA_CHANNEL_ACK)) {
					FATAL("Invalid message encountered on channel (%"PRIz"u) %s",
							(size_t) pChannel->_id, pChannel->_name.c_str());
					break;
				}

				//move the channel into active state
				INFO("Received the data on the channel. Moving the channel into active(online) state");
				pChannel->_state = CHANNEL_STATE_ONLINE;
				pChannel->_connectedOnce = true;
#ifdef HAS_PROTOCOL_WEBRTC
				//TODO: For now, we assume that this is always WrtcConnection.
				// Refactor such that it can be reused by whatever upper protocol
				sctp->_pWrtc->SignalDataChannelCreated(pChannel->_name, pChannel->_id);
#endif
				//done
				break;
			}
			case CHANNEL_STATE_ONLINE:
			{
#ifdef HAS_PROTOCOL_WEBRTC
				sctp->_pWrtc->SignalDataChannelInput(pChannel->_id, received.pBuffer,
						(uint32_t) received.amount);
#endif
				break;
			}
			case CHANNEL_STATE_UNINITIALIZED:
			case CHANNEL_STATE_PENDING_FOR_REMOVAL:
			default:
			{
				WARN("Invalid channel state.");
				break;
			}
		}
	}

	free(data);
	return 1;
}

bool SCTPProtocol::CreateInboundChannel() {
//#ifdef WEBRTC_DEBUG
	FINE("CreateInboundChannel sid: %"PRIu16, _received.info.rcv_sid);
//#endif
	//check and see if the new SID which is about to be created
	//is properly selected (odd or even)
	if (((_oddChannels)&&((_received.info.rcv_sid % 2) != 0))
			|| ((!_oddChannels)&&((_received.info.rcv_sid % 2) == 0))) {
		FATAL("Invalid odd/even SID");
		return false;
	}

	// RFC 8832 See section 5.1 "DATA_CHANNEL_OPEN Message" https://tools.ietf.org/html/rfc8832

	//read the low level informational message
	if (_received.amount < 13) {
		FATAL("Invalid channel open message: size is too small");
		return false;
	}

	if (_received.pBuffer[0] != SCTP_DATA_CHANNEL_OPEN) {
		FATAL("Invalid channel open message: incorrect message type: 0x%02"PRIx8, _received.pBuffer[0]);
		return false;
	}

	if (_received.pBuffer[1] != SCTP_DATA_CHANNEL_RELIABLE) {
		FATAL("Invalid channel open message: incorrect channel type: 0x%02"PRIx8, _received.pBuffer[1]);
		return false;
	}

	uint16_t nameLength = ENTOHSP(_received.pBuffer + 8);
	if (_received.amount < (nameLength + 12)) {
		FATAL("Invalid channel open message: size is too small");
		return false;
	}
	string name = string((const char *) (_received.pBuffer + 12), nameLength);

	// Send back the ACK
	uint8_t ack = SCTP_DATA_CHANNEL_ACK;
	int32_t result = SendData(_received.info.rcv_sid, SCTP_PPID_CONTROL, &ack, 1);
	if (result != 1) {
		FATAL("Unable to send back the DATA_CHANNEL_ACK message");
		return false;
	}

	//create the final channel ID
	uint32_t channelId = (_channelIdGenerator << 16) | _received.info.rcv_sid;

	//increment the channelIdgenerator
	_channelIdGenerator++;

	//create the channel itself
	INFO("Create the Inbound Channel with active(online) state");
	_ppAllChannelsBySID[_received.info.rcv_sid] = new SctpChannel(channelId, name, CHANNEL_STATE_ONLINE);
	_ppAllChannelsBySID[_received.info.rcv_sid]->_connectedOnce = true;

	//TODO: For now, we assume that this is always WrtcConnection.
#ifdef HAS_PROTOCOL_WEBRTC
	// Refactor such that it can be reused by whatever upper protocol
	_pWrtc->SignalDataChannelCreated(
			_ppAllChannelsBySID[_received.info.rcv_sid]->_name,
			_ppAllChannelsBySID[_received.info.rcv_sid]->_id);
#endif
	//done
	return true;
}

void SCTPProtocol::HandleNotifications() {
	if (_received.amount < 8) {
		WARN("Invalid SCTP notification received: too few bytes to parse the header");
		return;
	}

	const sctp_notification *pNotification = (const sctp_notification *) _received.pBuffer;
	if ((ssize_t) pNotification->sn_header.sn_length > _received.amount) {
		WARN("Invalid SCTP notification received: too few bytes to parse the notification");
		return;
	}

	switch (pNotification->sn_header.sn_type) {
		case SCTP_PEER_ADDR_CHANGE:
			FINEST("SCTP_PEER_ADDR_CHANGE");
			break;
		case SCTP_ASSOC_CHANGE:
			HandleNotificationAssocChange(&pNotification->sn_assoc_change);
			break;
		case SCTP_REMOTE_ERROR:
			FINEST("SCTP_REMOTE_ERROR");
			break;
		case SCTP_SHUTDOWN_EVENT:
			FINEST("SCTP_SHUTDOWN_EVENT");
			break;
		case SCTP_ADAPTATION_INDICATION:
			FINEST("SCTP_ADAPTATION_INDICATION");
			break;
		case SCTP_PARTIAL_DELIVERY_EVENT:
			FINEST("SCTP_PARTIAL_DELIVERY_EVENT");
			break;
		case SCTP_AUTHENTICATION_EVENT:
			FINEST("SCTP_AUTHENTICATION_EVENT");
			break;
		case SCTP_SENDER_DRY_EVENT:
			if (_ppAllChannelsBySID[_received.info.rcv_sid] != NULL) {
#ifdef HAS_PROTOCOL_WEBRTC
				_pWrtc->SignalDataChannelReady(_ppAllChannelsBySID[_received.info.rcv_sid]->_id);
#endif
			} else {
				WARN("SCTP_SENDER_DRY_EVENT: Channel entry is null!");
			}
			
			break;
		case SCTP_NOTIFICATIONS_STOPPED_EVENT:
			FINEST("SCTP_NOTIFICATIONS_STOPPED_EVENT");
			break;
		case SCTP_SEND_FAILED_EVENT:
			FINEST("SCTP_SEND_FAILED_EVENT");
			break;
		case SCTP_STREAM_RESET_EVENT:
			HandleNotificationStreamReset(&pNotification->sn_strreset_event);
			break;
		case SCTP_ASSOC_RESET_EVENT:
			FINEST("SCTP_ASSOC_RESET_EVENT");
			break;
		case SCTP_STREAM_CHANGE_EVENT:
			FINEST("SCTP_STREAM_CHANGE_EVENT");
			break;
		default:
			FINEST("Unknown SCTP event: %04"PRIx16, pNotification->sn_header.sn_type);
			break;
	}
}

void SCTPProtocol::HandleNotificationAssocChange(const struct sctp_assoc_change *pNotification) {
//#ifdef WEBRTC_DEBUG
	FINE("HandleNotificationAssocChange");
//#endif
	//https://tools.ietf.org/html/draft-ietf-tsvwg-sctpsocket-01#section-5.3.1.1
	switch (pNotification->sac_state) {
		case SCTP_COMM_UP:
			FINE("SCTP association successful.");
			//TODO: Do we need to handle this?
			break;
		case SCTP_COMM_LOST:
		case SCTP_RESTART:
		case SCTP_SHUTDOWN_COMP:
		case SCTP_CANT_STR_ASSOC:
		default:
			WARN("SCTP association error! %"PRIu32, pNotification->sac_state);
			//TODO: Do we need to handle this?
			break;
	}
}

void SCTPProtocol::HandleNotificationStreamReset(const struct sctp_stream_reset_event *pNotification) {
//#ifdef WEBRTC_DEBUG
	FINE("HandleNotificationStreamReset");
//#endif
	//compute the number of element in the strreset_stream_list
	uint32_t count = (pNotification->strreset_length - sizeof (struct sctp_stream_reset_event)) 
			/ sizeof (pNotification->strreset_stream_list[0]);

	//cycle over all elements
	uint16_t sid = 0;
	SctpChannel *pChannel = NULL;
	for (size_t i = 0; i < count; i++) {
		//get the channel SID
		sid = pNotification->strreset_stream_list[i];

		//test for validity
		if ((sid >= _maxSctpChannels) || (_ppAllChannelsBySID[sid] == NULL)) {
			continue;
		}

		//get the Channel
		pChannel = _ppAllChannelsBySID[sid];

		//if this is still an active channel, do close from our side as well
		if (pChannel->_state != CHANNEL_STATE_PENDING_FOR_REMOVAL)
			EnqueueChannelForDelete(pChannel->_id, true);

		//store the just received close flags
		pChannel->StoreReceivedCloseFlags(pNotification->strreset_flags);
	}

	//do channels close request if we have any pending stuff
	DoChannelsCloseRequest();

	//do the real deletion of channels
	DoChannelsCleanup();
}

bool SCTPProtocol::IsDataChannelValid(uint32_t channelId) {
	// Get the sid
	uint16_t sid = (uint16_t) (channelId & 0xffff);

	// If any conditions below is true, channel is invalid
	if ((sid >= _maxSctpChannels)
			|| (_ppAllChannelsBySID[sid] == NULL)
			|| (_ppAllChannelsBySID[sid]->_id != channelId)
			|| (_ppAllChannelsBySID[sid]->_sid != sid)
			|| (_ppAllChannelsBySID[sid]->_state != CHANNEL_STATE_ONLINE) 
			) {
		return false;
	}

	return true;
}

bool SCTPProtocol::IsDataChannelConnectedOnce(uint32_t channelId) {
	// Get the sid
	uint16_t sid = (uint16_t) (channelId & 0xffff);

	// If the channel is invalid or not connected atleast once, we consider it is not yet connected.
	if ((sid >= _maxSctpChannels)
			|| (_ppAllChannelsBySID[sid] == NULL)
			|| (_ppAllChannelsBySID[sid]->_id != channelId)
			|| (_ppAllChannelsBySID[sid]->_sid != sid)
			|| (_ppAllChannelsBySID[sid]->_connectedOnce == false)
			) {
		return false;
	}

	return true;
}

int32_t SCTPProtocol::SendData(uint32_t channelId, const uint8_t *pBuffer, 
		uint32_t length, bool isBinary) {

	// Get the sid
	uint16_t sid = (uint16_t) (channelId & 0xffff);

	// Test the validity of the channel used for sending
	if (!IsDataChannelValid(channelId)) {
		FATAL("Invalid SCTP Channel ID!");
		return -1;
	}

	if (isBinary) {
		return SendData(sid, SCTP_PPID_DATA, pBuffer, length);
	} else {
		return SendData(sid, SCTP_PPID_TEXT, pBuffer, length);
	}
}

int32_t SCTPProtocol::SendData(uint16_t channelSid, uint32_t ppid, const uint8_t *pBuffer,
		uint32_t length) {

	if (_pSCTPSocket == NULL) {
		FATAL("SCTP connection not ready");
		SetChannelSessionValidity(channelSid, false);
		return -1;
	}

	struct sctp_sendv_spa spa;
	memset(&spa, 0, sizeof (spa));
	spa.sendv_flags = SCTP_SEND_SNDINFO_VALID;
	spa.sendv_sndinfo.snd_sid = channelSid;
	spa.sendv_sndinfo.snd_ppid = ppid;
	spa.sendv_sndinfo.snd_flags = 0;
	ssize_t sent = 0;

#ifdef SCTP_USE_SENDSPACE
	size_t totalSent = 0;
	size_t chunk = 0;
	while (totalSent < length) {
		chunk = length - totalSent;
		chunk = (chunk >= _sendSpace ? _sendSpace : chunk);
		sent = usrsctp_sendv(_pSCTPSocket, pBuffer + totalSent, chunk, NULL, 0,
				&spa, sizeof (spa), SCTP_SENDV_SPA, 0);
		
		if (sent < 0) {
			int err = errno;
			if ((err != EINPROGRESS) && (err != EWOULDBLOCK)) {
				WARN("Could not send data!");
				SetChannelSessionValidity(channelSid, false);
				return -1;
			}
			break;
		}

		totalSent += sent;
	}

	return (int32_t) totalSent;
#else
	// We will not fragment
	sent = usrsctp_sendv(_pSCTPSocket, pBuffer, length, NULL, 0, &spa, 
			sizeof (spa), SCTP_SENDV_SPA, 0);

	if (sent < 0) {
		int err = errno;
		if ((err != EINPROGRESS) && (err != EWOULDBLOCK)) {
			WARN("Could not send data!");
			SetChannelSessionValidity(channelSid, false);
			return -1;
		}
	}

	if (sent > 0) {
		SetChannelSessionValidity(channelSid, true); // reset channel validity flag
	}
	
	return (int32_t) sent;
#endif
}

int8_t SCTPProtocol::CreateOutboundChannel(const string &name) {
	//if we are in the middle channels deleting, than queue this channel creation
	if (_pendingForRemoval.size() != 0) {
		_channelsPendingForCreate.push_back(name);
		return 0;
	}

	//we first do the already present queue of channels creation
	DoCreateOutboundChannels();

	//do the final channel
	return DoCreateOutboundChannel(name);
}

int8_t SCTPProtocol::CloseChannel(uint32_t cid) {
//#ifdef WEBRTC_DEBUG
	FINE("Closing channel.");
//#endif
	int res = EnqueueChannelForDelete(cid, false);
	DoChannelsCloseRequest();
	DoChannelsCleanup();
	return res;
}

void SCTPProtocol::DoCreateOutboundChannels() {
//#ifdef WEBRTC_DEBUG
	FINE("Creating outbound channels.");
//#endif
	if (_pendingForRemoval.size() != 0) {
		return;
	}
	
	for (size_t i = 0; i < _channelsPendingForCreate.size(); i++) {
		int err = DoCreateOutboundChannel(_channelsPendingForCreate[i]);
		if (err) {
			//TODO: handle this error???
			//_pWebRTCConnection->GetObserver()->SignalDataChannelCreateError(_pWebRTCConnection,
			//	_channelsPendingForCreate[i], err, GetErrorDescription(err));
			WARN("Could not create outbound channel!");
		}
	}
	
	_channelsPendingForCreate.clear();
}

int8_t SCTPProtocol::DoCreateOutboundChannel(const string &name) {
//#ifdef WEBRTC_DEBUG
	FINE("Create outbound channel (%s)", STR(name));
//#endif
	if (_channelIdGenerator >= _maxSctpChannels) {
		WARN("No more channels available!");
		return -1;
	}

	//search for an empty placeholder for SID
	uint16_t sid = 0xffff;
	for (uint16_t i = _oddChannels ? 1 : 0; i < _maxSctpChannels; i += 2) {
		if (_ppAllChannelsBySID[i] == NULL) {
			sid = i;
			break;
		}
	}

	if (sid == 0xffff) {
		WARN("Invalid session ID!");
		return -1;
	}

	// RFC 8832 See section 5.1 "DATA_CHANNEL_OPEN Message"

	//send the low level informational message

	//allocate the buffer
	uint8_t *pBuffer = new uint8_t[12 + name.length()];

	//populate the buffer
	pBuffer[0] = SCTP_DATA_CHANNEL_OPEN; //Message Type - DATA_CHANNEL_OPEN
	pBuffer[1] = SCTP_DATA_CHANNEL_RELIABLE; //Channel Type - DATA_CHANNEL_RELIABLE
	EHTONSP(pBuffer + 2, SCTP_DATA_CHANNEL_PRIORITY); //Priority - 0!?!?
	EHTONLP(pBuffer + 4, 0); //Reliability Parameter - 0 because of DATA_CHANNEL_RELIABLE
	EHTONSP(pBuffer + 8, (uint16_t) name.size()); //Label Length
	EHTONSP(pBuffer + 10, 0); //Protocol Length - 0!?!?
	memcpy(pBuffer + 12, name.c_str(), name.size());

	//send the buffer
	int32_t result = SendData(sid, SCTP_PPID_CONTROL, pBuffer, 12 + (uint32_t)(name.length()));
	delete[] pBuffer;
	if (result < 0) {
		return -1;
	}

	//create the final channel ID
	uint32_t channelId = (_channelIdGenerator << 16) | sid;

	//increment the channelIdgenerator
	_channelIdGenerator++;

	//create the channel itself
	_ppAllChannelsBySID[sid] = new SctpChannel(channelId, name, CHANNEL_STATE_CREATE_REQUEST_SENT);

	//done
	return 0;
}

int8_t SCTPProtocol::EnqueueChannelForDelete(uint32_t cid, bool remoteClose) {
	//get the sid
	uint16_t sid = (uint16_t) (cid & 0xffff);

	//test the validity of the channel that needs to be closed
	if ((sid >= _maxSctpChannels)
			|| (_ppAllChannelsBySID[sid] == NULL)
			|| (_ppAllChannelsBySID[sid]->_id != cid)
			|| (_ppAllChannelsBySID[sid]->_sid != sid)
			) {
		WARN("Invalid SCTP Channel ID!");
		return -1;
	}

	//set the channel in CHANNEL_STATE_PENDIG_FOR_REMOVAL and also add it to
	//the list of pending for removal
	_ppAllChannelsBySID[sid]->_state = CHANNEL_STATE_PENDING_FOR_REMOVAL;
	_ppAllChannelsBySID[sid]->_remoteClose = remoteClose;
	_pendingForRemoval[cid] = _ppAllChannelsBySID[sid];

	return 0;
}

void SCTPProtocol::DoChannelsCloseRequest() {
//#ifdef WEBRTC_DEBUG
	FINE("Channel close request.");
//#endif
	//do we have any pending for removal?
	if (_pendingForRemoval.size() == 0) {
		return;
	}

	//the number of removals
	size_t count = _pendingForRemoval.size();

	//compute the maximum size of the required structure
	const size_t maxSize = sizeof (struct sctp_reset_streams)+(count * sizeof (uint16_t));

	//create the necessary buffer and cast it to the structure
	uint8_t *pBuffer = new uint8_t[maxSize];
	struct sctp_reset_streams *pStreamsReset = (struct sctp_reset_streams *) pBuffer;

	//populate the structure header
	pStreamsReset->srs_assoc_id = SCTP_ALL_ASSOC;
	pStreamsReset->srs_flags = SCTP_STREAM_RESET_INCOMING | SCTP_STREAM_RESET_OUTGOING;

	//put in the SIDs that needs to be closed
	size_t cursor = 0;
	for (ChannelsMapIterator i = _pendingForRemoval.begin(); i != _pendingForRemoval.end(); i++) {
		//get the channel
		SctpChannel *pChannel = i->second;

		//see if is time to send the close message
		if (!pChannel->MustSendClose())
			continue;

		//put the SID in the message
		pStreamsReset->srs_stream_list[cursor++] = pChannel->_sid;
	}

	//set the size of the structure
	pStreamsReset->srs_number_streams = (uint16_t) cursor;

	//is this an empty message? if so, bail out
	if (cursor == 0) {
		delete[] pBuffer;
		return;
	}

	//compute the final size of the structure
	const size_t size = sizeof (struct sctp_reset_streams)+(cursor * sizeof (uint16_t));

	//do the call
	int result = usrsctp_setsockopt(_pSCTPSocket, IPPROTO_SCTP, SCTP_RESET_STREAMS,
			pStreamsReset, (socklen_t)size);

	//free the buffer
	delete[] pBuffer;

	//see if the call was successful, and bail out if not. We will try this at
	//a later time
	if (result < 0) {
		WARN("usrsctp_setsockopt failed");
		return;
	}

	//if the call was a success, update the requested flags
	for (ChannelsMapIterator i = _pendingForRemoval.begin(); i != _pendingForRemoval.end(); i++) {
		i->second->_sentCloseFlags = SCTP_STREAM_RESET_INCOMING | SCTP_STREAM_RESET_OUTGOING;
	}
}

void SCTPProtocol::DoChannelsCleanup() {
#ifdef WEBRTC_DEBUG
	FINE("Channel clean-up.");
#endif
	ChannelsMapIterator i = _pendingForRemoval.begin();
	while (i != _pendingForRemoval.end()) {
		SctpChannel *pChannel = i->second;

		//see if the channel was closed from both directions
		if (!pChannel->CanBeDeleted()) {
			i++;
			continue;
		}

		//save the iterator, increment it and than remove it from map
		_pendingForRemoval.erase(i++);

		//free the space in _ppAllChannelsBySID
		_ppAllChannelsBySID[pChannel->_sid] = NULL;
#ifdef HAS_PROTOCOL_WEBRTC
		_pWrtc->SignalDataChannelClosed(pChannel->_id);
#endif
		//delete it
		delete pChannel;
	}

	DoCreateOutboundChannels();
}
void SCTPProtocol::SignalEvent(string eventName, Variant &args) {
	if (eventName == "srtpInitEvent" && _pNearProtocol != NULL && _pNearProtocol->GetType() == PT_WRTC_CNX) {
		_pNearProtocol->SignalEvent(eventName, args);
	}
}
void SCTPProtocol::VerboseLogPacket(void *addr, size_t length, int direction) {
    char *dump_buf;
    if ((dump_buf = usrsctp_dumppacket(
             addr, length, direction)) != NULL) {

	  DEBUG("Packet Dump: %s", dump_buf);
      usrsctp_freedumpbuffer(dump_buf);
    }
}

void SCTPProtocol::SetChannelSessionValidity(uint16_t channelSid, bool isValid) {
	if ((channelSid >= _maxSctpChannels)
			|| (_ppAllChannelsBySID[channelSid] == NULL)
			|| (_ppAllChannelsBySID[channelSid]->_sid != channelSid)) {
		// Channel session is already invalid, nothing to do here
		return;
	}
	
	if (isValid) {
		INFO("Set the session validity of the channel to active(online)");
		_ppAllChannelsBySID[channelSid]->_state = CHANNEL_STATE_ONLINE;
		_ppAllChannelsBySID[channelSid]->_connectedOnce = true;
		_channelValdityCheck = 0;
	} else {
		if (_channelValdityCheck++ > SCTP_MAX_CHANNEL_CHECK)
		_ppAllChannelsBySID[channelSid]->_state = CHANNEL_STATE_OFFLINE;
	}
}
