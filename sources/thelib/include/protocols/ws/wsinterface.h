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

#ifdef HAS_PROTOCOL_WS
#ifndef _WSINTERFACE_H
#define	_WSINTERFACE_H

class WSInterface {
public:

	/*!
	 * @brief keep the compiler happy about destructor
	 */
	virtual ~WSInterface() {

	}

	/*!
	 * @brief Called by the framework when a new WebSocket connection is inbound
	 *
	 * @param request - The initial WS HTTP request
	 *
	 * @discussion This signal should contain code to handle the requested
	 * resource. Things like the existence of a stream for example.
	 */
	virtual bool SignalInboundConnection(Variant &request) = 0;
	virtual bool SignalTextData(const uint8_t *pBuffer, size_t length) = 0;
	virtual bool SignalBinaryData(const uint8_t *pBuffer, size_t length) = 0;
	virtual bool SignalPing(const uint8_t *pBuffer, size_t length) = 0;
	virtual void SignalConnectionClose(uint16_t code, const uint8_t *pReason, size_t length) = 0;
	virtual bool DemaskBuffer() = 0;
	virtual bool IsOutBinary() { return true;};	// return true if output is binary, false if text
};

#endif	/* _WSINTERFACE_H */
#endif /* HAS_PROTOCOL_WS */
