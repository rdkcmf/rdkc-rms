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

#ifndef _INBOUNDDTLSPROTOCOL_H
#define	_INBOUNDDTLSPROTOCOL_H

#include "protocols/ssl/basesslprotocol.h"

struct SRTPProtectionProfile {
	string profileName;
	string cipherName;
	uint8_t cipherKeyLength;
	uint8_t cipherSaltLength;
	string authMethodName;
	uint8_t authKeyLength;
	uint8_t authTagLength;
	uint8_t rtcpAuthTagLength;
};
struct CryptoContext {
	IOBuffer serverWriteMasterKey;
	IOBuffer clientWriteMasterKey;
	IOBuffer serverWriteMasterSalt;
	IOBuffer clientWriteMasterSalt;
	uint32_t masterKeyLength;
	uint32_t masterSaltLength;
	uint32_t kdr; /* key deriviation rate*/
	IOBuffer serverMki;
	IOBuffer clientMki;
};
union sequenceDetails {
	struct {
		uint16_t base;
		uint32_t rollOverCounter;
		uint16_t lastSequence;
		uint32_t v;
		uint64_t GetPacketIndex() {
			return (rollOverCounter << 16 | lastSequence) & 0x0000FFFFFFFFFFFFL;;
		}

		// refer https://www.cisco.com/web/about/security/intelligence/05_07_voip.html#7 Section 3.1 Algorithm 2
		uint64_t GetReceivedPacketIndex(uint16_t seq) {
			if (abs(lastSequence - seq) < 32768) {
				v = rollOverCounter;
			}
			// out-of-order RTP packets
			else if(lastSequence < seq) {
				v = (rollOverCounter - 1) % 0x80000000L;
			}
			// roll over
			else {
				v = (rollOverCounter + 1) % 0x80000000L;
			}

			return (v << 16 | seq) & 0x0000FFFFFFFFFFFFL;
                }

                void update(uint16_t seq) {
                        if (v == rollOverCounter ) {
                                if( seq > lastSequence) {
                                        lastSequence = seq;
                                }
                        }
                        else if (v == ((rollOverCounter + 1) % 0x80000000L)) {
                                lastSequence = seq;
                                rollOverCounter = v;
                        }
                }
	} srtpIndex;
	uint32_t srtcpIndex;
};
struct CryptoKeys {
	IOBuffer sessionKey;
	IOBuffer sessionAuthKey;
	IOBuffer sesionSalt;
	uint64_t lastRefresh;
	bool isRTCP;
	map<uint32_t, sequenceDetails> packetSequences;
};
class OutboundConnectivity;
class InboundDTLSProtocol
: public BaseSSLProtocol {
public:
	InboundDTLSProtocol(bool includeSrtp = false);
	virtual ~InboundDTLSProtocol();
	
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	
	/**
	 * Initially, DTLS is disabled. This method will enable DTLS and start the
	 * handshake mechanism.
	 * 
     * @return True on success, false otherwise
     */
	bool Start();

	bool InitSrtp();
//	bool FeedRtpData(MSGHDR mediaData, uint32_t ssrc);
//	bool FeedRtcpData(MSGHDR mediaData, uint32_t ssrc);
	bool CreateOutSRTPStream(string streamName);
	virtual IOBuffer * GetOutputBuffer();
	bool HasSRTP();
	bool FeedData(IOBuffer &data);

protected:
	virtual bool InitGlobalContext(Variant &parameters);
	virtual bool DoHandshake();

private:
	bool _isStarted;
	IOBuffer _serverWriteKey;
	IOBuffer _clientWriteKey;
	OutboundConnectivity *_pOutboundConnectivity;
	string _streamName;

	CryptoKeys _srtpKeys;
	CryptoKeys _inboundSrtpKeys;
	CryptoKeys _outboundSrtcpKeys;
	CryptoKeys _inboundSrtcpKeys;
	CryptoContext _cryptoContext;
	SRTPProtectionProfile &_srtpProfile;

	IOBuffer _outputBuffer;
	bool _mkiCheckDone;
	bool _hasSrtp;
	bool _includeSrtp;

	void ProcessRtpPacket(IOBuffer &rtpPacket, uint32_t rtpLength);
	void DeriveSessionKeys(uint64_t packetIndex, CryptoKeys &cryptoKeys, bool outbound = true);
	void EncryptRtpPacket(uint8_t *rtpPacketStart, uint32_t packetLength, uint64_t packetIndex);
	void DecryptRtpPacket(uint8_t *rtpPacketStart, uint32_t packetLength, uint64_t packetIndex);
	void EncryptRtcpPacket(uint8_t *rtcpPacketStart, uint32_t packetLength, uint64_t packetIndex);
	void DecryptRtcpPacket(uint8_t *rtcpPacketStart, uint32_t packetLength, uint64_t packetIndex);
	void AttachMKI(IOBuffer &buffer);
	void AppendAuth(IOBuffer &buffer, uint8_t *rtpPacket, uint32_t packetLength, CryptoKeys &keys);

	void TransformPayload(uint8_t *payloadStart, uint32_t payloadLength, uint64_t packetIndex, uint8_t *pSSRCStart, CryptoKeys &keys);

	void SetEncryptKey(AES_KEY &key);
	bool FeedRtp(IOBuffer &rtpPacket);
	bool FeedRtcp(IOBuffer &rtcpPacket);

};


#endif	/* _INBOUNDDTLSPROTOCOL_H */

