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

#ifndef _OUTBOUNDDTLSPROTOCOL_H
#define	_OUTBOUNDDTLSPROTOCOL_H

#include "protocols/ssl/basesslprotocol.h"

class OutboundDTLSProtocol
: public BaseSSLProtocol {
public:
	OutboundDTLSProtocol();
	OutboundDTLSProtocol(X509Certificate *pCertificate);
	virtual ~OutboundDTLSProtocol();
	
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);
	
	/**
	 * Initially, DTLS is disabled. This method will enable DTLS and start the
	 * handshake mechanism.
	 * 
     * @return True on success, false otherwise
     */
	bool Start();
protected:
	virtual bool InitGlobalContext(Variant &parameters);
	virtual bool DoHandshake();
	
private:
	bool _isStarted;
};


#endif	/* _OUTBOUNDDTLSPROTOCOL_H */

