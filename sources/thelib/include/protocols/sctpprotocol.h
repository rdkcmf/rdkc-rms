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

#ifndef _SCTPPROTOCOL_H
#define	_SCTPPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "usrsctp.h"
#include "protocols/wrtc/wrtcconnection.h"

#ifdef HAS_PROTOCOL_WEBRTC
class WrtcConnection;
#endif

#define SCTP_PATH_MTU 1200
#define SCTP_PPID_CONTROL (50UL<<24)
#define SCTP_PPID_TEXT (51UL<<24)
#define SCTP_PPID_DATA (53UL<<24)
#define SCTP_MAX_RECEIVE_BUFFER 1024*1024
#define SCTP_DEFAULT_SEND_SPACE (1024*1024)
#define SCTP_DATA_CHANNEL_OPEN 0x03
#define SCTP_DATA_CHANNEL_ACK 0x02
#define SCTP_DATA_CHANNEL_RELIABLE 0x00
//https://tools.ietf.org/html/draft-ietf-rtcweb-data-channel-12#section-6.4
#define SCTP_DATA_CHANNEL_PRIORITY 256
#define SCTP_MAX_CHANNELS_COUNT 512
#define SCTP_MAX_CHANNEL_ID_HEADER 0xffff
#define SCTP_COMMAND_CHANNEL "rmsCommandChannel"
#define SCTP_STREAM_CHANNEL "rmsStreamChannel"
#define SCTP_MAX_CHANNEL_CHECK 50

typedef enum {
	CHANNEL_STATE_UNINITIALIZED,
	CHANNEL_STATE_CREATE_REQUEST_SENT,
	CHANNEL_STATE_PENDING_FOR_REMOVAL,
	CHANNEL_STATE_ONLINE,
	CHANNEL_STATE_OFFLINE
} ChannelState;

#define CLOSE_FLAGS_COMPLETE(x) (((x)&(SCTP_STREAM_RESET_INCOMING | SCTP_STREAM_RESET_OUTGOING))==(SCTP_STREAM_RESET_INCOMING | SCTP_STREAM_RESET_OUTGOING))

struct SctpChannel {
	uint32_t _id;
	uint16_t _sid;
	string _name;
	ChannelState _state;
	uint16_t _sentCloseFlags;
	uint16_t _receivedCloseFlagsInitiate;
	uint16_t _receivedCloseFlagsFinalize;
	bool _remoteClose;
	bool _connectedOnce;

	SctpChannel(uint32_t id, const string &name, ChannelState state) {
		_id = id;
		_sid = (uint16_t) (id & 0xffff);
		_name = name;
		_state = state;
		_sentCloseFlags = 0;
		_receivedCloseFlagsInitiate = 0;
		_receivedCloseFlagsFinalize = 0;
		_remoteClose = false;
		_connectedOnce = false;
	}

	~SctpChannel() {

	}

	bool MustSendClose() {
		if (_remoteClose) {
			return CLOSE_FLAGS_COMPLETE(_receivedCloseFlagsInitiate) //we have received the close for both in/out
					&&(_sentCloseFlags == 0) //we did not sent any close so far
					;
		} else {
			return (_sentCloseFlags == 0) //we did not sent any close so far
					;
		}
	}

	bool CanBeDeleted() {
		return CLOSE_FLAGS_COMPLETE(_sentCloseFlags)
				&& CLOSE_FLAGS_COMPLETE(_receivedCloseFlagsFinalize);
	}

	void StoreReceivedCloseFlags(uint16_t flags) {
		if (_remoteClose) {
			if (CLOSE_FLAGS_COMPLETE(_sentCloseFlags))
				_receivedCloseFlagsFinalize |= flags;
			else
				_receivedCloseFlagsInitiate |= flags;
		} else {
			_receivedCloseFlagsFinalize |= flags;
		}
	}
};

typedef map<uint32_t, SctpChannel *> ChannelsMap;
typedef ChannelsMap::iterator ChannelsMapIterator;

struct ReceiveContext {
	int lastErrno;
	ssize_t amount;
	uint8_t *pBuffer;
	size_t bufferSize;
	struct sockaddr_in6 from; //TODO: why _in6?
	socklen_t fromLength;
	struct sctp_rcvinfo info;
	socklen_t infoLength;
	unsigned int infoType;
	int flags;
};

class IOHandler;

class SCTPProtocol
: public BaseProtocol {
public:
	SCTPProtocol();
	virtual ~SCTPProtocol();
	virtual bool Initialize(Variant &parameters);
	virtual IOHandler *GetIOHandler();
	virtual void SetIOHandler(IOHandler *pIOHandler);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual IOBuffer *GetInputBuffer();
	virtual IOBuffer *GetOutputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress);
	virtual bool SignalInputData(IOBuffer &buffer);

	// for RTCP use. passthrough to wrtc connection
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	virtual bool EnqueueForOutbound();
	virtual uint64_t GetDecodedBytesCount();
	virtual void SignalEvent(string eventName, Variant &args);

	bool IsDataChannelValid(uint32_t channelId);
	bool IsDataChannelConnectedOnce(uint32_t channelId);
	int32_t SendData(uint32_t channelId, const uint8_t *pBuffer, uint32_t length, bool isBinary = true);
	int8_t CreateOutboundChannel(const string &name);
	int8_t CloseChannel(uint32_t cid);
#ifdef HAS_PROTOCOL_WEBRTC
	bool Start(WrtcConnection *pWrtc);
#endif 
private:
	static void CallbackDebugPrint(const char *format, ...);
	static int CallbackSendData(void *addr, void *buffer, size_t length,
		uint8_t tos, uint8_t set_df);
	static int CallbackReceiveData(struct socket *sock, union sctp_sockstore addr,
		void *data, size_t length, struct sctp_rcvinfo rcv, int flags, void* ulp_info);
	bool CreateInboundChannel();
	void HandleNotifications();
	void HandleNotificationAssocChange(const struct sctp_assoc_change *pNotification);
	void HandleNotificationStreamReset(const struct sctp_stream_reset_event *pNotification);
	int32_t SendData(uint16_t channelSid, uint32_t ppid, const uint8_t *pBuffer,
		uint32_t length);
	void DoCreateOutboundChannels();
	int8_t DoCreateOutboundChannel(const string &name);
	int8_t EnqueueChannelForDelete(uint32_t cid, bool remoteClose);
	void DoChannelsCloseRequest();
	void DoChannelsCleanup();
	static void VerboseLogPacket(void *addr, size_t length, int direction);
	void SetChannelSessionValidity(uint16_t channelSid, bool isValid);

private:
	IOHandler *_pCarrier;
	IOBuffer _inputBuffer;
	IOBuffer _outputBuffer;
	uint64_t _decodedBytesCount;

	bool _isReadyForInitialization;

	static uint32_t _initCounter;
#ifdef SCTP_USE_SENDSPACE
	static uint32_t _sendSpace;
#endif
	struct socket *_pSCTPSocket;
	uint16_t _maxSctpChannels;
	ReceiveContext _received;
	SctpChannel **_ppAllChannelsBySID;
	bool _oddChannels;
	uint16_t _channelIdGenerator;
	ChannelsMap _pendingForRemoval;
	vector<string> _channelsPendingForCreate;
	bool _initAckReceived;

	//TODO: For now, SCTP is aware of the upper protocol
#ifdef HAS_PROTOCOL_WEBRTC
	WrtcConnection *_pWrtc;
#endif
	bool CreateDefaultChannel();
	
	uint32_t _channelValdityCheck;
};

#endif	/* _SCTPPROTOCOL_H */


