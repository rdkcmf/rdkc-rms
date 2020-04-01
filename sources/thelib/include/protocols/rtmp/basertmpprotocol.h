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



#ifdef HAS_PROTOCOL_RTMP
#ifndef _BASERTMPPROTOCOL_H
#define	_BASERTMPPROTOCOL_H

#include "protocols/baseprotocol.h"
#include "protocols/rtmp/channel.h"
#include "protocols/rtmp/rtmpprotocolserializer.h"
#include "mediaformats/readers/streammetadataresolver.h"

#define RECEIVED_BYTES_COUNT_REPORT_CHUNK 131072
#define MAX_CHANNELS_COUNT (64+255)
#define MAX_STREAMS_COUNT 256 //200000

#define MIN_AV_CHANNLES 20
#define MAX_AV_CHANNLES MAX_CHANNELS_COUNT

#define MAX_RTMP_OUTPUT_BUFFER 1024*256

typedef enum {
	RTMP_STATE_NOT_INITIALIZED,
	RTMP_STATE_CLIENT_REQUEST_RECEIVED,
	RTMP_STATE_CLIENT_REQUEST_SENT,
	RTMP_STATE_SERVER_RESPONSE_SENT,
	RTMP_STATE_DONE
} RTMPState;

class BaseStream;
class BaseOutStream;
class OutNetRTMPStream;
class InFileRTMPStream;
class InNetRTMPStream;
class BaseInDeviceStream;
class BaseRTMPAppProtocolHandler;
class ClientSO;
class RTMPPlaylist;
class RTMPPlaylistItem;
#ifdef ENFORCE_RTMP_OUTPUT_CHECKS
class RTMPOutputChecks;
#endif  /* ENFORCE_RTMP_OUTPUT_CHECKS */

class DLLEXP BaseRTMPProtocol
: public BaseProtocol {
private:

	struct StreamContainer {
		BaseStream *pStream;
		InFileRTMPStream *pInputFileStream;
		RTMPPlaylist *pPlaylist;
		double clientSideBuffer;

		StreamContainer();
		virtual ~StreamContainer();
		void Clear(bool clearPlaylist);
	};
protected:
	static uint8_t serverKey[];
	static uint8_t playerKey[];
	bool _handshakeCompleted;
	RTMPState _rtmpState;
	IOBuffer _outputBuffer;
#ifdef ENFORCE_RTMP_OUTPUT_CHECKS
	IOBuffer _intermediateBuffer;
	RTMPOutputChecks *_pMonitor;
#endif /* ENFORCE_RTMP_OUTPUT_CHECKS */
	uint64_t _nextReceivedBytesCountReport;
	uint32_t _winAckSize;
	Channel _channels[MAX_CHANNELS_COUNT];
	int32_t _selectedChannel;
	uint32_t _inboundChunkSize;
	uint32_t _outboundChunkSize;
	BaseRTMPAppProtocolHandler *_pProtocolHandler;
	RTMPProtocolSerializer _rtmpProtocolSerializer;
	StreamContainer _streams[MAX_STREAMS_COUNT];
	vector<uint32_t> _channelsPool;
	uint64_t _rxInvokes;
	uint64_t _txInvokes;
	map<string, ClientSO *> _sos;
	string _clientId;
	vector<uint32_t> _signaledIFS;
	uint32_t _streamsMap[MAX_STREAMS_COUNT];
public:
	BaseRTMPProtocol(uint64_t protocolType);
	virtual ~BaseRTMPProtocol();

	virtual void SetWitnessFile(string path);

	string GetClientId();

	virtual BaseRTMPAppProtocolHandler * GetProtocolHandler() {return _pProtocolHandler;};

	ClientSO *GetSO(string &name);
	bool CreateSO(string &name);
	void SignalBeginSOProcess(string &name);
	bool HandleSOPrimitive(string &name, Variant &primitive);
	void SignalEndSOProcess(string &name, uint32_t versionNumber);
	bool ClientSOSend(string &name, Variant &parameters);
	bool ClientSOSetProperty(string &soName, string &propName, Variant &propValue);

	void SignalOutBufferFull(uint32_t outstanding, uint32_t maxValue);

	virtual bool Initialize(Variant &parameters);
	virtual bool AllowFarProtocol(uint64_t type);
	virtual bool AllowNearProtocol(uint64_t type);
	virtual IOBuffer * GetOutputBuffer();
	virtual bool SignalInputData(int32_t recvAmount);
	virtual bool SignalInputData(IOBuffer &buffer);
	virtual bool TimePeriodElapsed();
	virtual void ReadyForSend();
	virtual void SetApplication(BaseClientApplication *pApplication);

	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);

	bool ResetChannel(uint32_t channelId);

	bool SendMessage(Variant &message);
	bool SendRawData(Header &header, Channel &channel, uint8_t *pData, uint32_t length);
	bool SendRawData(uint8_t *pData, uint32_t length);

	void SetWinAckSize(uint32_t winAckSize);

	uint32_t GetOutboundChunkSize();
	bool SetInboundChunkSize(uint32_t chunkSize);
	void TrySetOutboundChunkSize(uint32_t chunkSize);

	RTMPPlaylist *GetPlaylist(uint32_t rtmpStreamId);
	bool HasRTMPStream(uint32_t rtmpStreamId);
	BaseStream * GetRTMPStream(uint32_t rtmpStreamId);
	bool SetClientSideBuffer(uint32_t rtmpStreamId, double value);
	bool CloseStream(uint32_t streamId, bool createNeutralStream);
	bool CreateNeutralStream(uint32_t &streamId);
	InNetRTMPStream * CreateINS(uint32_t channelId, uint32_t streamId,
			string publicStreamName, string privateStreamName);
	OutNetRTMPStream * CreateONS(RTMPPlaylistItem *pPlaylistItem,
			uint32_t streamId, string streamName);
	InFileRTMPStream *CreateIFS(uint32_t rtmpStreamId, Metadata &metadata);
	bool SignalReadyForSendOnIFS(uint32_t rtmpStreamId);
	void SignalPlaylistItemStart(uint32_t index, uint32_t itemCount, string playlistName);
	Channel *ReserveChannel();
	void ReleaseChannel(Channel *pChannel);
	bool HasStreamsOfType(uint64_t kind);

	virtual bool EnqueueForTimeEvent(uint32_t seconds);
protected:
	virtual bool PerformHandshake(IOBuffer &buffer) = 0;
	uint32_t GetDHOffset(uint8_t *pBuffer, uint8_t schemeNumber);
	uint32_t GetDigestOffset(uint8_t *pBuffer, uint8_t schemeNumber);
private:
	uint32_t GetDHOffset0(uint8_t *pBuffer);
	uint32_t GetDHOffset1(uint8_t *pBuffer);
	uint32_t GetDigestOffset0(uint8_t *pBuffer);
	uint32_t GetDigestOffset1(uint8_t *pBuffer);
	bool ProcessBytes(IOBuffer &buffer);
	uint64_t _decodedBytesSubtractor;

};


#endif	/* _BASERTMPPROTOCOL_H */

#endif /* HAS_PROTOCOL_RTMP */

