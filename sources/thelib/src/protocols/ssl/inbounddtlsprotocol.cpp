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

//TODO: Need to refactor SSL-based protocols to make it readable and optimize
// code reuse.

#include "application/baseclientapplication.h"
#include "protocols/ssl/inbounddtlsprotocol.h"
#include "streaming/baseinstream.h"
#include "streaming/streamsmanager.h"
#include "streaming/streamstypes.h"
#include "utils/misc/timeprobemanager.h"
#include "utils/misc/x509certificate.h"
#include <openssl/evp.h>

#define SRTP_MASTER_KEY_LEN 16 // 128 bits
#define SRTP_MASTER_SALT_LEN 14 // 112 bits
#define AES_BLOCK_SIZE 16 // AES-128

#define SRTP_NULL_ENC_80 "SRTP_NULL_SHA1_80"
#define SRTP_AES_ENC_80 "SRTP_AES128_CM_SHA1_80"
#define SRTP_NULL_ENC_32 "SRTP_NULL_SHA1_32"
#define SRTP_AES_ENC_32 "SRTP_AES128_CM_SHA1_32"
#define SRTCP_DEFAULT_AUTH_TAG_LEN 80

#define SRTP_LABEL_ENC 0x00
#define SRTP_LABEL_AUTH 0x01
#define SRTP_LABEL_SALT 0x02
#define SRTCP_LABEL_ENC 0x03
#define SRTCP_LABEL_AUTH 0x04
#define SRTCP_LABEL_SALT 0x05

#define KEY_LENGTH_ENC 16
#define KEY_LENGTH_SALT 14
#define KEY_LENGTH_AUTH 20

#define RTP_HEADER_LENGTH 12
#define RTP_SSRC_OFFSET 8
#define RTCP_HEADER_LENGTH 8
#define RTCP_SSRC_OFFSET 4

#if 0
void printBytes(string label, uint8_t *buffer, uint32_t length) {
	string out = label + ":";
	for (uint32_t i = 0; i < length; i++) {
		out += format(" %02"PRIx8, buffer[i]);
	}
	WARN("\t%s", STR(out));
}

#define PRINTBYTES(s, b, len) printBytes(s, b, len)
#else
#define PRINTBYTES(s, b, len)
#endif
SRTPProtectionProfile srtpAES128CM_SHA1_80 = { SRTP_AES_ENC_80, "AES128CM", 128, 112, "HMAC-SHA1", 160, 80, SRTCP_DEFAULT_AUTH_TAG_LEN };
SRTPProtectionProfile srtpAES128CM_SHA1_32 = { SRTP_AES_ENC_32, "AES128CM", 128, 112, "HMAC-SHA1", 160, 32, SRTCP_DEFAULT_AUTH_TAG_LEN };
SRTPProtectionProfile srtpNULL_SHA1_80 = { SRTP_NULL_ENC_80, "NULL", 0, 0, "HMAC-SHA1", 160, 80, SRTCP_DEFAULT_AUTH_TAG_LEN };
SRTPProtectionProfile srtpNULL_SHA1_32 = { SRTP_NULL_ENC_32, "NULL", 0, 0, "HMAC-SHA1", 160, 32, SRTCP_DEFAULT_AUTH_TAG_LEN };

InboundDTLSProtocol::InboundDTLSProtocol(bool includeSrtp)
	: BaseSSLProtocol(PT_INBOUND_DTLS), _srtpProfile(srtpAES128CM_SHA1_80) {
	_isStarted = false;
	_pOutboundConnectivity = NULL;
	_streamName = "";
	_mkiCheckDone = false;
	_hasSrtp = false;
	_includeSrtp = includeSrtp;
}

InboundDTLSProtocol::InboundDTLSProtocol(X509Certificate *pCertificate, bool includeSrtp)
	: BaseSSLProtocol(PT_INBOUND_DTLS), _srtpProfile(srtpAES128CM_SHA1_80) {
	_isStarted = false;
	_pOutboundConnectivity = NULL;
	_streamName = "";
	_mkiCheckDone = false;
	_hasSrtp = false;
	_includeSrtp = includeSrtp;
	_pCertificate = pCertificate;
}

InboundDTLSProtocol::~InboundDTLSProtocol() {
	if(_pGlobalSSLContext != NULL) {
		SSL_CTX_free(_pGlobalSSLContext);
		_pGlobalSSLContext = NULL;
	}
}

bool InboundDTLSProtocol::InitGlobalContext(Variant &parameters) {
        //1. Comput the hash on key/cert pair and see
	//if we have a global context with that hash
	string dtlsmd5string = "Inbounddtlskeycertpair";
	bool useprebuiltcert = false;

	if (parameters["hash"] != V_STRING) {
		_hash = md5(dtlsmd5string, true);
		parameters["hash"] = _hash;
	} else {
		_hash = (string) parameters["hash"];
	}
	useprebuiltcert = (bool) parameters["usePrebuiltCert"];

	if( MAP_HAS1(_pGlobalContexts, _hash )) {
		_pGlobalSSLContext = _pGlobalContexts[_hash];
	}

	//2. Initialize the global context based on the specified
	//key/cert pair if we don't have it
	if (_pGlobalSSLContext == NULL) {
		//3. prepare the global ssl context

		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		_pGlobalSSLContext = SSL_CTX_new(DTLSv1_server_method());
		#else
		_pGlobalSSLContext = SSL_CTX_new(DTLS_server_method());
		#endif

		if (_pGlobalSSLContext == NULL) {
			FATAL("Unable to create global SSL context");
			return false;
		}
		
		// Enable/disable the extensions
		SSL_CTX_clear_options(_pGlobalSSLContext, SSL_CTX_get_options(_pGlobalSSLContext));
		// Disable tickets, ssl versions, etc.
		const long flags = SSL_OP_NO_TICKET | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_CIPHER_SERVER_PREFERENCE;
		SSL_CTX_set_options(_pGlobalSSLContext, flags);
		// Enable use_srtp
		SSL_CTX_set_tlsext_use_srtp(_pGlobalSSLContext, STR(_srtpProfile.profileName));

		if(useprebuiltcert) {
			string key = parameters[CONF_SSL_KEY];
			string cert = parameters[CONF_SSL_CERT];

			//4. setup the certificate
                        if (SSL_CTX_use_certificate_file(_pGlobalSSLContext, STR(cert),
                                        SSL_FILETYPE_PEM) <= 0) {
                                FATAL("Unable to load certificate %s; Error(s) was: %s",
						STR(cert),
						STR(GetSSLErrors()));
                                SSL_CTX_free(_pGlobalSSLContext);
                                _pGlobalSSLContext = NULL;
                                return false;
                        }

			//5. setup the private key
			if (SSL_CTX_use_PrivateKey_file(_pGlobalSSLContext, STR(key),
					SSL_FILETYPE_PEM) <= 0) {
				FATAL("Unable to load key %s; Error(s) was: %s",
						STR(key),
						STR(GetSSLErrors()));
				SSL_CTX_free(_pGlobalSSLContext);
				_pGlobalSSLContext = NULL;
				return false;
			}
		} else {
                        //4. setup the certificate
                        if (SSL_CTX_use_certificate(_pGlobalSSLContext, _evpcert) != 1) {
                                FATAL("SSL_CTX_use_certificate(): %s.", ERR_error_string(ERR_get_error(), NULL));
				SSL_CTX_free(_pGlobalSSLContext);
				_pGlobalSSLContext = NULL;
				return false;
			}

                        //5. setup the private key
                        if (SSL_CTX_use_PrivateKey(_pGlobalSSLContext, _evpkey) != 1) {
                                FATAL("SSL_CTX_use_PrivateKey(): %s.", ERR_error_string(ERR_get_error(), NULL));
                                SSL_CTX_free(_pGlobalSSLContext);
                                _pGlobalSSLContext = NULL;
                                return false;
                        }
                }

		//6. enable client certificate authentication
		//TODO: add the callback to properly verify the peer
		//SSL_CTX_set_verify(_pGlobalSSLContext, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
		SSL_CTX_set_verify(_pGlobalSSLContext, SSL_VERIFY_NONE, NULL);

		string cipherSuite = "";
		if (parameters.HasKeyChain(V_STRING, false, 1, CONF_SSL_CIPHERSUITE))
			cipherSuite = (string) parameters[CONF_SSL_CIPHERSUITE];
		trim(cipherSuite);
		if (cipherSuite != "") {
			INFO("Apply cipher suite `%s` on %s %s:%"PRIu16,
					STR(cipherSuite),
					STR(parameters[CONF_PROTOCOL]),
					STR(parameters[CONF_IP]),
					(uint16_t) parameters[CONF_PORT]
					);
			if (SSL_CTX_set_cipher_list(_pGlobalSSLContext, STR(cipherSuite)) == 0) {
				FATAL("Unable to apply cipher suite `%s`: Error was: `%s`",
						STR(cipherSuite),
						STR(GetSSLErrors()));
				SSL_CTX_free(_pGlobalSSLContext);
				_pGlobalSSLContext = NULL;
				return false;
			}
		} else {
			// Set default cipher list
			SSL_CTX_set_cipher_list(_pGlobalSSLContext, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
		}

		// Fix the non-responding DTLS server issue
		//It seems that this is no longer needed
		SSL_CTX_set_read_ahead(_pGlobalSSLContext, 1);

		// For newer versions of Firefox. FUUUUUU....
		//Redmine 2269
		#if OPENSSL_API_COMPAT < 0x10100000L
		SSL_CTX_set_ecdh_auto(_pGlobalSSLContext, 1);
		#endif

		//7. Store the global context for later usage
		//_pGlobalContexts[_hash] = _pGlobalSSLContext;
		INFO("SSL server context initialized in InboundDTLSProtocol");
	}

	if (parameters.HasKeyChain(V_STRING, false, 1, "streamName")) {
		_streamName = (string) parameters.GetValue("streamName", false);
	}
	return true;
}

bool InboundDTLSProtocol::DoHandshake() {
	if (_sslHandshakeCompleted || !_isStarted)
		return true;

	int32_t errorCode = SSL_ERROR_NONE;

	errorCode = SSL_accept(_pSSL);
	if (errorCode < 0) {
		int32_t error = SSL_get_error(_pSSL, errorCode);
		if (error != SSL_ERROR_WANT_READ &&
				error != SSL_ERROR_WANT_WRITE) {
			FATAL("Unable to accept SSL connection: %d; %s", error, STR(GetSSLErrors()));
			return false;
		}
	}

	if (!PerformIO()) {
		FATAL("Unable to perform I/O");
		return false;
	}

	_sslHandshakeCompleted = SSL_is_init_finished(_pSSL);
	if (_sslHandshakeCompleted && _includeSrtp) {
		INFO("ssl handshake is completed, signal the wrtc connection");
		Variant srtpInitEvent;
		srtpInitEvent["result"] = InitSrtp();
		_pNearProtocol->SignalEvent("srtpInitEvent", srtpInitEvent);
	}

	if (_sslHandshakeCompleted){
		PROBE_POINT("webrtc", "sess1", "dtls_handshake_complete", false);
 	}

	return true;
}

void InboundDTLSProtocol::ProcessRtpPacket(IOBuffer &rtpPacket, uint32_t rtpLength) {

	// get ssrc
        uint32_t *pSSRC = (uint32_t *)(GETIBPOINTER(rtpPacket) + 8);
        uint32_t ssrc = ENTOHL(*pSSRC);

        uint8_t *pRtpPacket = GETIBPOINTER(rtpPacket);

	// get seq number
	uint16_t seqNumber = pRtpPacket[2] << 8 | pRtpPacket[3];
	//INFO("Processing RTP packet, Seqence no: %lu", seqNumber);

	if (!MAP_HAS1(_inboundSrtpKeys.packetSequences, ssrc)) {
		_inboundSrtpKeys.packetSequences[ssrc].srtpIndex.lastSequence = seqNumber;
		_inboundSrtpKeys.packetSequences[ssrc].srtpIndex.rollOverCounter = 0;
		_inboundSrtpKeys.packetSequences[ssrc].srtpIndex.v = 0;
		//INFO("Processing RTP packet, Initial sequence no: %u", seqNumber);
	}

	uint64_t index = _inboundSrtpKeys.packetSequences[ssrc].srtpIndex.GetReceivedPacketIndex(seqNumber);
	//INFO("Processing RTP packet,  packet index %llu", index);

        DeriveSessionKeys(index, _inboundSrtpKeys, false);
        DecryptRtpPacket(pRtpPacket, rtpLength, index);

	_inboundSrtpKeys.packetSequences[ssrc].srtpIndex.update(seqNumber);
}

bool InboundDTLSProtocol::SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress) {
	//TODO: Probably need to take note of the peer address?
	uint8_t first = *(GETIBPOINTER(buffer));
	if ((first & 0xC0) == 0x80 && _sslHandshakeCompleted) { // 127 < B < 192
		// decrypt before passing up
		uint8_t pt = *(GETIBPOINTER(buffer) + 1);
		if (pt == 0xC8 || pt == 0xC9) { // inbound SRTCP

			// Get E flag and packetIndex
			uint32_t rtpLength = GETAVAILABLEBYTESCOUNT(buffer) - _srtpProfile.rtcpAuthTagLength/8 - 4;
			uint32_t *pIndexPos = (uint32_t *)(GETIBPOINTER(buffer) + rtpLength);
			uint32_t srtcpIndex = ENTOHL(*pIndexPos);
			//bool encrypted = (srtcpIndex & 0x80000000) != 0;
			srtcpIndex &= 0x7FFFFFFF;
			DeriveSessionKeys(srtcpIndex, _inboundSrtcpKeys, false);
			DecryptRtcpPacket(GETIBPOINTER(buffer), rtpLength, srtcpIndex);
			buffer._published -= _srtpProfile.rtcpAuthTagLength / 8 + 4; // trim off SRTCP index and auth tag
		} else { // inbound SRTP

			//uint32_t rtpLength = GETAVAILABLEBYTESCOUNT(buffer) - _srtpProfile.authTagLength/8 - 4;
			uint32_t rtpLength = GETAVAILABLEBYTESCOUNT(buffer) - _srtpProfile.authTagLength/8;
			uint8_t *pBuffer = (uint8_t *)(GETIBPOINTER(buffer));
			//INFO("First 4 bytes of the packet header: 0x%x 0x%x 0x%x 0x%x ", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3]);
			//uint32_t rtpLength = GETAVAILABLEBYTESCOUNT(buffer) - _srtpProfile.authTagLength/8;
			//INFO("Inbound SRTP packet size %d", rtpLength);

			// process only g711 rtp packets. refer https://tools.ietf.org/html/rfc3551#page-32 , Section 6: Payload Type Definitions 
			if ((pBuffer[1] & 0x7F) == 0x0) {
				ProcessRtpPacket(buffer, rtpLength);
				//buffer._published -= _srtpProfile.rtcpAuthTagLength / 8 + 4; // trim off MKI and auth tag
				buffer._published -= _srtpProfile.rtcpAuthTagLength / 8; // trim off auth tag
			}
		}
		return _pNearProtocol->SignalInputData(buffer, pPeerAddress);
	}
	return BaseSSLProtocol::SignalInputData(buffer);
}

IOBuffer *InboundDTLSProtocol::GetOutputBuffer() {
	IOBuffer *out = BaseSSLProtocol::GetOutputBuffer();
	if (out == NULL) {
		return GETAVAILABLEBYTESCOUNT(_outputBuffer) == 0 ? NULL : &_outputBuffer;
	}
	else {
		return out;
	}
}

bool InboundDTLSProtocol::Start() {
	_isStarted = true;
	INFO("Do ssl handshake as dtls protocol has been started");
	return DoHandshake();
}
bool InboundDTLSProtocol::FeedRtp(IOBuffer &rtpPacket) {
	// get ssrc
	uint32_t *pSSRC = (uint32_t *)(GETIBPOINTER(rtpPacket) + 8);
	uint32_t ssrc = ENTOHL(*pSSRC);

	uint8_t *pRtpPacket = GETIBPOINTER(rtpPacket);
	uint32_t rtpPacketSize = GETAVAILABLEBYTESCOUNT(rtpPacket);

	//2. get sequence number, ROC, and encrypt payload
	uint16_t seqNumber = pRtpPacket[2] << 8 | pRtpPacket[3];

	if (!MAP_HAS1(_srtpKeys.packetSequences, ssrc)) {
		_srtpKeys.packetSequences[ssrc].srtpIndex.rollOverCounter = 0;
		_srtpKeys.packetSequences[ssrc].srtpIndex.base = seqNumber;
		seqNumber = 0;
	}
	else {
		// checking for rollover by checking MSB change from 1 to 0
		seqNumber -= _srtpKeys.packetSequences[ssrc].srtpIndex.base;
		if ((_srtpKeys.packetSequences[ssrc].srtpIndex.lastSequence >> 15) > (seqNumber >> 15)) {
			_srtpKeys.packetSequences[ssrc].srtpIndex.rollOverCounter = (_srtpKeys.packetSequences[ssrc].srtpIndex.rollOverCounter + 1) & 0xFFFFFFFF;
		}
	}
	_srtpKeys.packetSequences[ssrc].srtpIndex.lastSequence = seqNumber;

	// override index
	pRtpPacket[2] = (seqNumber >> 8) & 0x00FF;
	pRtpPacket[3] = (seqNumber)& 0x00FF;

	uint64_t index = _srtpKeys.packetSequences[ssrc].srtpIndex.GetPacketIndex();

	DeriveSessionKeys(index, _srtpKeys);

	EncryptRtpPacket(pRtpPacket, rtpPacketSize, index);

	//3. transfer all to output
	_outputBuffer.ReadFromBuffer(pRtpPacket, rtpPacketSize);

	//4. Append MKI
	AttachMKI(_outputBuffer);

	//5. Attach Auth, append ROC first
	uint32_t roc = _srtpKeys.packetSequences[ssrc].srtpIndex.rollOverCounter;
	for (uint8_t i = 0; i < 4; i++) {
		rtpPacket.ReadFromByte((roc >> (24 - i * 8)) & 0x000000FF);
	}
	AppendAuth(_outputBuffer, pRtpPacket, rtpPacketSize + 4, _srtpKeys);

	//6. cleanup
//	delete pRtpPacket;

	if (_pFarProtocol != NULL) {
		return _pFarProtocol->EnqueueForOutbound();
	}
	return true;
}

bool InboundDTLSProtocol::FeedRtcp(IOBuffer &rtcpPacket) {

	// extract SSRC
	uint32_t *pSSRC = (uint32_t *)(GETIBPOINTER(rtcpPacket) + 4);
	uint32_t ssrc = ENTOHL(*pSSRC);

	// RTCP doesn't have sequence number so SRTCP index is explicit
	// No rollover is required
	if (!MAP_HAS1(_outboundSrtcpKeys.packetSequences, ssrc)) {
		_outboundSrtcpKeys.packetSequences[ssrc].srtcpIndex = 1;
	}

	uint64_t packetIndex = (uint64_t)_outboundSrtcpKeys.packetSequences[ssrc].srtcpIndex;
	DeriveSessionKeys(packetIndex, _outboundSrtcpKeys);

	EncryptRtcpPacket(GETIBPOINTER(rtcpPacket), GETAVAILABLEBYTESCOUNT(rtcpPacket), packetIndex);

	_outputBuffer.ReadFromBuffer(GETIBPOINTER(rtcpPacket), GETAVAILABLEBYTESCOUNT(rtcpPacket));

	// append [ E | SRTCP Index ]
	uint32_t srtcpIndexBlock = _outboundSrtcpKeys.packetSequences[ssrc].srtcpIndex | 0x80000000;

	for (uint8_t i = 0; i < 4; i++) {
		_outputBuffer.ReadFromByte((uint8_t)((srtcpIndexBlock >> (24 - 8 * i)) & 0x000000FF));
	}

	AttachMKI(_outputBuffer);

	AppendAuth(_outputBuffer, GETIBPOINTER(_outputBuffer), GETAVAILABLEBYTESCOUNT(_outputBuffer), _outboundSrtcpKeys);

	if (_pFarProtocol != NULL) {
		if (!_pFarProtocol->EnqueueForOutbound())
			return false;
	}

	// increment SRTCP index after sending
	_outboundSrtcpKeys.packetSequences[ssrc].srtcpIndex++;

	// fix to 31-bit
	_outboundSrtcpKeys.packetSequences[ssrc].srtcpIndex &= 0x7FFFFFFF;

	return true;
}

void InboundDTLSProtocol::DeriveSessionKeys(uint64_t packetIndex, CryptoKeys &cryptoKeys, bool outbound) {
	if (_cryptoContext.kdr == 0 && GETAVAILABLEBYTESCOUNT(cryptoKeys.sessionAuthKey) > 0)
		return;
	/// RFC 3711 p. 26
	//INFO("Deriving %s keys", cryptoKeys.isRTCP ? "SRTCP" : "SRTP");
	uint64_t r = _cryptoContext.kdr == 0 ? 0 : packetIndex / _cryptoContext.kdr;
	uint8_t labelAuth = cryptoKeys.isRTCP ? SRTCP_LABEL_AUTH : SRTP_LABEL_AUTH;
	uint8_t labelEnc = cryptoKeys.isRTCP ? SRTCP_LABEL_ENC : SRTP_LABEL_ENC;
	uint8_t labelSalt = cryptoKeys.isRTCP ? SRTCP_LABEL_SALT : SRTP_LABEL_SALT;

	IOBuffer &writeMasterKey = outbound ? _cryptoContext.serverWriteMasterKey : _cryptoContext.clientWriteMasterKey;
	IOBuffer &writeMasterSalt = outbound ? _cryptoContext.serverWriteMasterSalt : _cryptoContext.clientWriteMasterSalt;

	if (cryptoKeys.lastRefresh != r) {
		
		cryptoKeys.lastRefresh = r;
		uint8_t rBuff[AES_BLOCK_SIZE] = { 0 };
		for (uint8_t i = 8; i < 14; i++) {
			rBuff[14 - i] = r & 0x00000000000000FF;
			r = r >> 8;
		}
		
		// Get session auth key
		rBuff[7] = labelAuth;
		
		uint8_t *pR = rBuff;
		uint8_t *pSalt = GETIBPOINTER(writeMasterSalt);
		for (uint8_t i = 0; i < 14; i++) {
			*pR = *pR ^ *pSalt;
			pR++;
			pSalt++;
		}
		PRINTBYTES("Master Key", GETIBPOINTER(writeMasterKey), GETAVAILABLEBYTESCOUNT(writeMasterKey));
		PRINTBYTES("Master Salt", GETIBPOINTER(writeMasterSalt), GETAVAILABLEBYTESCOUNT(writeMasterSalt));
		PRINTBYTES("IV-Auth", rBuff, 16);

		AES_KEY masterKey;
		uint8_t out[16] = { 0 };
		AES_set_encrypt_key(GETIBPOINTER(writeMasterKey), 128, &masterKey);
		AES_encrypt(rBuff, out, &masterKey);
		// get the first 128-bits (16 bytes) of the 160 bits needed
		cryptoKeys.sessionAuthKey.ReadFromBuffer(out, AES_BLOCK_SIZE);
		
		rBuff[15] = 1;
		PRINTBYTES("IV-Auth_1", rBuff, 16);
		memset(out, 0, sizeof(out));
		AES_encrypt(rBuff, out, &masterKey);
		// get the remaining 32-bits (4 bytes)
		cryptoKeys.sessionAuthKey.ReadFromBuffer(out, 4);
		PRINTBYTES("Session Auth Key", GETIBPOINTER(cryptoKeys.sessionAuthKey), GETAVAILABLEBYTESCOUNT(cryptoKeys.sessionAuthKey));

		if (_srtpProfile.cipherKeyLength > 0) {
			rBuff[15] = 0;
			rBuff[7] = labelEnc ^ GETIBPOINTER(writeMasterSalt)[7];
			memset(out, 0, sizeof(out));
			AES_encrypt(rBuff, out, &masterKey);
			cryptoKeys.sessionKey.ReadFromBuffer(out, 16);
			PRINTBYTES("IV-Auth", rBuff, 16);
			PRINTBYTES("Session Enc Key", GETIBPOINTER(cryptoKeys.sessionKey), GETAVAILABLEBYTESCOUNT(cryptoKeys.sessionKey));

			rBuff[7] = labelSalt ^ GETIBPOINTER(writeMasterSalt)[7];
			memset(out, 0, sizeof(out));
			AES_encrypt(rBuff, out, &masterKey);
			cryptoKeys.sesionSalt.ReadFromBuffer(out, 14);
			PRINTBYTES("Session Enc Salt", GETIBPOINTER(cryptoKeys.sesionSalt), GETAVAILABLEBYTESCOUNT(cryptoKeys.sesionSalt));
		}
		

	}
}

void InboundDTLSProtocol::EncryptRtpPacket(uint8_t *rtpPacketStart, uint32_t packetLength, uint64_t packetIndex) {
	if (_srtpProfile.cipherKeyLength == 0)
		return;

	// 1. Find payload start

	uint32_t payloadOffset = RTP_HEADER_LENGTH;
	uint8_t cc = ((*rtpPacketStart) & 0x0F);  // CC Mask
	if ( cc == 1) {
		payloadOffset += 4 * cc;
	}
	if (((*rtpPacketStart) & 0x10) == 1) {  // X flag
		uint8_t *pExt = rtpPacketStart + payloadOffset;
		pExt += 2; // skip ext definition;
		uint16_t extLength = *pExt << 8 | *(pExt + 1);
		payloadOffset += extLength;
	}

	// 2. Initialize IV			RFC 3711 p. 21
	uint32_t payloadLength = packetLength - payloadOffset;
	uint8_t *pPayload = rtpPacketStart + payloadOffset;

	uint8_t *pSSRC = rtpPacketStart + RTP_SSRC_OFFSET;

	TransformPayload(pPayload, payloadLength, packetIndex, pSSRC, _srtpKeys);
}

void InboundDTLSProtocol::DecryptRtpPacket(uint8_t *rtpPacketStart, uint32_t packetLength, uint64_t packetIndex) {
	if (_srtpProfile.cipherKeyLength == 0)
		return;

	// 1. Find payload start

	uint32_t payloadOffset = RTP_HEADER_LENGTH;
	uint8_t cc = ((*rtpPacketStart) & 0x0F);  // CC Mask
	if ( cc == 1) {
		payloadOffset += 4 * cc;
	}

	if (((*rtpPacketStart) & 0x10) == 1) {  // X flag
		uint8_t *pExt = rtpPacketStart + payloadOffset;
		pExt += 2; // skip ext definition;
		uint16_t extLength = *pExt << 8 | *(pExt + 1);
		payloadOffset += extLength;
	}

	// 2. Initialize IV			RFC 3711 p. 21
	uint32_t payloadLength = packetLength - payloadOffset;
	uint8_t *pPayload = rtpPacketStart + payloadOffset;

	uint8_t *pSSRC = rtpPacketStart + RTP_SSRC_OFFSET;

	TransformPayload(pPayload, payloadLength, packetIndex, pSSRC, _inboundSrtpKeys);
}

void InboundDTLSProtocol::EncryptRtcpPacket(uint8_t *rtcpPacketStart, uint32_t packetLength, uint64_t packetIndex) {
	if (_srtpProfile.cipherKeyLength == 0)
		return;

	uint8_t *pPayload = rtcpPacketStart + RTCP_HEADER_LENGTH;
	uint32_t payloadLength = packetLength - RTCP_HEADER_LENGTH;
	uint8_t *pSSRC = rtcpPacketStart + RTCP_SSRC_OFFSET;

	TransformPayload(pPayload, payloadLength, packetIndex, pSSRC, _outboundSrtcpKeys);
}

void InboundDTLSProtocol::DecryptRtcpPacket(uint8_t *rtcpPacketStart, uint32_t packetLength, uint64_t packetIndex) {
	if (_srtpProfile.cipherKeyLength == 0)
		return;

	uint8_t *pPayload = rtcpPacketStart + RTCP_HEADER_LENGTH;
	uint32_t payloadLength = packetLength - RTCP_HEADER_LENGTH;
	uint8_t *pSSRC = rtcpPacketStart + RTCP_SSRC_OFFSET;

	TransformPayload(pPayload, payloadLength, packetIndex, pSSRC, _inboundSrtcpKeys);
}

void InboundDTLSProtocol::TransformPayload(uint8_t *payloadStart, uint32_t payloadLength, uint64_t packetIndex, uint8_t *pSSRCStart, CryptoKeys &keys) {
	uint8_t iv[AES_BLOCK_SIZE] = { 0 };
	uint8_t saltBlock[AES_BLOCK_SIZE] = { 0 };
	uint8_t ssrcIndexBlock[AES_BLOCK_SIZE] = { 0 };

//	FATAL("Encrypting %s", keys.isRTCP ? "RTCP" : "RTP");
//	FATAL("Index: %"PRIu64, packetIndex);

	memcpy(saltBlock, GETIBPOINTER(keys.sesionSalt), _cryptoContext.masterSaltLength);
	for (uint8_t i = 0; i < 4; i++) {
		ssrcIndexBlock[4 + i] = *(pSSRCStart++) & 0xFF;
	}
	for (uint8_t i = 0; i < 6; i++) {
		ssrcIndexBlock[8 + i] = (packetIndex >> (40 - i * 8)) & 0xFF;
	}
	for (uint8_t i = 0; i < AES_BLOCK_SIZE; i++) {
		iv[i] = saltBlock[i] ^ ssrcIndexBlock[i];
	}
//	PRINTBYTES("Salt Block", saltBlock, AES_BLOCK_SIZE);
//	PRINTBYTES("ssrcIndexBlock", ssrcIndexBlock, AES_BLOCK_SIZE);
//	PRINTBYTES("iv", iv, AES_BLOCK_SIZE);

	// 3. derive keystream		RFC 3711 p. 19
	uint16_t blocksCount = payloadLength / AES_BLOCK_SIZE;
	if (payloadLength % AES_BLOCK_SIZE > 0) {
		blocksCount++;
	}
	uint8_t *keyStream = new uint8_t[AES_BLOCK_SIZE * blocksCount];
	AES_KEY sessionKey;
	AES_set_encrypt_key(GETIBPOINTER(keys.sessionKey), 128, &sessionKey);

	uint8_t *pKeyStream = keyStream;
	for (uint16_t blockCounter = 0; blockCounter < blocksCount; blockCounter++) {
		// since the last 2 bytes of ssrcIndexBlock is always zero, the last 2 bytes of
		// IV can be modified without recomputing.
		iv[AES_BLOCK_SIZE - 2] = (uint8_t)((blockCounter >> 8) & 0x00FF);
		iv[AES_BLOCK_SIZE - 1] = (uint8_t)(blockCounter & 0x00FF);
		AES_encrypt(iv, pKeyStream, &sessionKey);
		pKeyStream += AES_BLOCK_SIZE;
	}

	// 4. encrypt RTP Payload (rtpPayload ^ keystream)
	pKeyStream = keyStream;
	for (uint32_t i = 0; i < payloadLength; i++) {
		payloadStart[i] ^= pKeyStream[i];
	}

	// 5. Cleanup
	delete[] keyStream;
}

void InboundDTLSProtocol::AttachMKI(IOBuffer &buffer) {
	if (GETAVAILABLEBYTESCOUNT(_cryptoContext.serverMki) > 0) {
		buffer.ReadFromBuffer(GETIBPOINTER(_cryptoContext.serverMki), GETAVAILABLEBYTESCOUNT(_cryptoContext.serverMki));
	}
}
void InboundDTLSProtocol::AppendAuth(IOBuffer &buffer, uint8_t *rtpPacket, uint32_t packetLength, CryptoKeys &keys) {
//	PRINTBYTES("Session Auth Key", rtpPacket, packetLength);
	
	uint8_t *digest = HMAC(EVP_sha1(),
		GETIBPOINTER(keys.sessionAuthKey), GETAVAILABLEBYTESCOUNT(keys.sessionAuthKey),
		rtpPacket, packetLength,
		NULL, NULL);
	
	for (uint8_t i = 0; i < _srtpProfile.authTagLength / 8; i++) {
		buffer.ReadFromByte(digest[i]);
	}
}

bool InboundDTLSProtocol::InitSrtp() {
	// do we need to initialize DTLS here?
	string label = "EXTRACTOR-dtls_srtp";
	uint8_t dtlsBuffer[2 * (SRTP_MASTER_KEY_LEN + SRTP_MASTER_SALT_LEN)] = { 0 };
	//SRTP_PROTECTION_PROFILE * srtpProfile = SSL_get_selected_srtp_profile(_pSSL);
	if (SSL_export_keying_material(_pSSL, dtlsBuffer, sizeof(dtlsBuffer), STR(label), label.size(), NULL, 0, 0) != 1) {
		FATAL("Failed to get keys");
		return false;
	}
	_cryptoContext.kdr = 0;
	_cryptoContext.masterKeyLength = SRTP_MASTER_KEY_LEN;
	_cryptoContext.masterSaltLength = SRTP_MASTER_SALT_LEN;
//	_cryptoContext.serverMki.ReadFromByte(0);  // OpenSSL hardcodes MKI value

	uint8_t *pBuffer = dtlsBuffer;
	_cryptoContext.clientWriteMasterKey.ReadFromBuffer(pBuffer, SRTP_MASTER_KEY_LEN);
	pBuffer += SRTP_MASTER_KEY_LEN;
	_cryptoContext.serverWriteMasterKey.ReadFromBuffer(pBuffer, SRTP_MASTER_KEY_LEN);
	pBuffer += SRTP_MASTER_KEY_LEN;
	_cryptoContext.clientWriteMasterSalt.ReadFromBuffer(pBuffer, SRTP_MASTER_SALT_LEN);
	pBuffer += SRTP_MASTER_SALT_LEN;
	_cryptoContext.serverWriteMasterSalt.ReadFromBuffer(pBuffer, SRTP_MASTER_SALT_LEN);

	_srtpKeys.lastRefresh = 0xFFFFFFFF;
	_srtpKeys.isRTCP = false;

	_inboundSrtpKeys.lastRefresh = 0xFFFFFFFF;
	_inboundSrtpKeys.isRTCP = false;

	_outboundSrtcpKeys.lastRefresh = 0xFFFFFFFF;
	_outboundSrtcpKeys.isRTCP = true;

	_inboundSrtcpKeys.lastRefresh = 0xFFFFFFFF;
	_inboundSrtcpKeys.isRTCP = true;

	_hasSrtp = true;
	INFO("Master keys acquired");
	return true;
}

void InboundDTLSProtocol::SetEncryptKey(AES_KEY &key) {
	
//	HMAC(EVP_sha1(),)
}
bool InboundDTLSProtocol::CreateOutSRTPStream(string streamName) {
//	InitSrtp();
//	_pOutboundConnectivity->Enable(true);
	return true;
}

bool InboundDTLSProtocol::HasSRTP() {
	return _hasSrtp;
}

bool InboundDTLSProtocol::FeedData(IOBuffer &data) {
	if (GETAVAILABLEBYTESCOUNT(data) < 2) {
		data.IgnoreAll();
		return false;
	}
	uint8_t secondByte = GETIBPOINTER(data)[1];
	if (secondByte >= 192 && secondByte <= 223) { // known RTCP payload types
		FeedRtcp(data);
	}
	else {
		FeedRtp(data);
	}
	data.IgnoreAll();
	return true;
}
