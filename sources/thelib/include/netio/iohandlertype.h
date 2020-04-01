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


#ifndef _IOHANDLERTYPE_H
#define	_IOHANDLERTYPE_H

typedef enum _IOHandlerType {
	IOHT_ACCEPTOR,
	IOHT_TCP_CONNECTOR,
	IOHT_TCP_CARRIER,
	IOHT_UDP_CARRIER,
	IOHT_INBOUNDNAMEDPIPE_CARRIER,
#ifdef HAS_UDS //UNIX Domain Sockets
	IOHT_UDS_ACCEPTOR,
	IOHT_UDS_CARRIER,
#endif /* HAS_UDS */
#ifdef HAS_PROTOCOL_API
	IOHT_API_FD,
#endif /* HAS_PROTOCOL_API */			
	IOHT_TIMER,
	IOHT_STDIO
} IOHandlerType;

#endif	/* _IOHANDLERTYPE_H */
