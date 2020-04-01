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


#ifdef HAS_PROTOCOL_LIVEFLV

#include "protocols/liveflv/inboundliveflvprotocol.h"
#include "application/baseclientapplication.h"
#include "protocols/protocoltypes.h"
#include "netio/netio.h"
#include "protocols/liveflv/innetliveflvstream.h"
#include "protocols/rtmp/amf0serializer.h"
#include "streaming/baseoutstream.h"

InboundLiveFLVProtocol::InboundLiveFLVProtocol()
: BaseProtocol(PT_INBOUND_LIVE_FLV) {
	_pStream = NULL;
	_headerParsed = false;
	_waitForMetadata = false;
	memset(&_timestamps, 0, sizeof (_timestamps));
}

InboundLiveFLVProtocol::~InboundLiveFLVProtocol() {
	if (_pStream != NULL) {
		delete _pStream;
		_pStream = NULL;
	}
}

bool InboundLiveFLVProtocol::Initialize(Variant &parameters) {
	GetCustomParameters() = parameters;
	//FINEST("parameters:\n%s", STR(parameters.ToString()));
	if (parameters.HasKey("waitForMetadata"))
		_waitForMetadata = (bool)parameters["waitForMetadata"];
	else
		_waitForMetadata = false;
	//FINEST("_waitForMetadata: %d", _waitForMetadata);

	return true;
}

bool InboundLiveFLVProtocol::AllowFarProtocol(uint64_t type) {
	return (type == PT_TCP)
			|| (type == PT_INBOUND_HTTP)
			|| (type == PT_OUTBOUND_HTTP);
}

bool InboundLiveFLVProtocol::AllowNearProtocol(uint64_t type) {
	return false;
}

bool InboundLiveFLVProtocol::SignalInputData(int32_t recvAmount) {
	ASSERT("OPERATION NOT SUPPORTED");
	return false;
}

//#define LIVEFLV_DUMP_PTSDTS
#ifdef LIVEFLV_DUMP_PTSDTS
uint32_t lastPts = 0;
uint32_t lastDts = 0;
#endif /* LIVEFLV_DUMP_PTSDTS */

bool InboundLiveFLVProtocol::SignalInputData(IOBuffer &buffer) {
	//1. Initialize the stream
	if (_pStream == NULL) {
		if (!_waitForMetadata) {
			if (!InitializeStream("")) {
				FATAL("Unable to initialize the stream");
				return false;
			}
		}
	}
	for (;;) {
		//2. ignore the first 13 bytes of header
		if (!_headerParsed) {
			if (GETAVAILABLEBYTESCOUNT(buffer) < 13) {
				break;
			}
			buffer.Ignore(13);
			_headerParsed = true;
		}

		//3. Read the frame header
		if (GETAVAILABLEBYTESCOUNT(buffer) < 11) {
			break;
		}

		//4. Read the type, length and the timestamp
		uint8_t type;
		uint32_t length;
		type = GETIBPOINTER(buffer)[0];
		length = ENTOHLP((GETIBPOINTER(buffer) + 1)) >> 8; //----MARKED-LONG---
		if (length >= 1024 * 1024) {
			FATAL("Frame too large: %u", length);
			return false;
		}
		Dts &dts = (type == 8) ? _timestamps._audio : (type == 9 ? _timestamps._video : _timestamps._meta);
		dts.current = ENTOHAP((GETIBPOINTER(buffer) + 4)); //----MARKED-LONG---
		if ((dts.current < dts.last)&&((dts.last >> 24) == 0xff)) {
			dts.rollOverCount++;
		}
		dts.value = (double) ((dts.rollOverCount << 32) | dts.current);
		dts.last = dts.current;

		//5. Do we have enough data? 15 bytes are the lead header and the trail length (11+4)
		if (GETAVAILABLEBYTESCOUNT(buffer) < length + 15) {
			return true;
		}

		//6. Ignore the 11 bytes of pre-header
		buffer.Ignore(11);

		//7. Ok, we have enough data. Pump it :)
		switch (type) {
			case 8:
			{
				//audio data
				if (_pStream != NULL) {
					if (!_pStream->FeedData(GETIBPOINTER(buffer), length, 0,
							length, dts.value, dts.value, true)) {
						FATAL("Unable to feed audio");
						return false;
					}
				}
				break;
			}
			case 9:
			{
				//video data
				if (_pStream != NULL) {
					double pts = dts.value + (ENTOHLP(GETIBPOINTER(buffer) + 2) >> 8);
#ifdef LIVEFLV_DUMP_PTSDTS
					FINEST("pts: %8.2f\tdts: %8.2f\tcts: %4"PRIu32"\tptsd: %+"PRId32"\tdtsd: %+"PRId32"\t%s",
							(double) pts, (double) dts,
							pts - dts,
							pts - lastPts, dts - lastDts,
							pts == dts ? "" : "DTS Present");
					lastPts = pts;
					lastDts = dts;
#endif /* LIVEFLV_DUMP_PTSDTS */
					if (!_pStream->FeedData(GETIBPOINTER(buffer), length, 0,
							length, pts, dts.value, false)) {
						FATAL("Unable to feed video");
						return false;
					}
					//FINEST("---------------------------");
				}
				break;
			}
			case 18:
			{
				AMF0Serializer amf0;

				//1. get the raw buffer
				IOBuffer metaBuffer;
				metaBuffer.ReadFromBuffer(GETIBPOINTER(buffer), length);

				//2. Read the notify function
				Variant notifyFunction;
				if (!amf0.Read(metaBuffer, notifyFunction)) {
					FATAL("Unable to read notify function");
					return false;
				}
				if (notifyFunction != V_STRING) {
					FATAL("Unable to read notify function");
					return false;
				}

				//3. Read the rest of the parameters
				Variant parameters;
				string streamName = "";
				while (GETAVAILABLEBYTESCOUNT(metaBuffer) > 0) {
					Variant v;
					if (!amf0.Read(metaBuffer, v)) {
						FATAL("Unable to read metadata item");
						return false;
					}
					if (v.HasKeyChain(V_STRING, false, 1, "streamName"))
						streamName = (string) v.GetValue("streamName", false);
					parameters.PushToArray(v);
				}

				if (_pStream == NULL) {
					if (!InitializeStream(streamName)) {
						FATAL("Unable to initialize the stream");
						return false;
					}
				}

				//INFO("Stream metadata:\n%s", STR(parameters.ToString()));

				//4. Send the notify
				if (_pStream != NULL) {
					if (!_pStream->SendStreamMessage(notifyFunction, parameters,
							(lowerCase(notifyFunction) == "onmetadata"))) {
						FATAL("Unable to send the notify");
						return false;
					}
				}

				break;
			}
			default:
			{
				WARN("Invalid frame type: %"PRIu8, type);
				break;
			}
		}

		//8. ignore what we've just fed
		buffer.Ignore(length + 4);
	}

	//9. Done
	return true;
}

void InboundLiveFLVProtocol::GetStats(Variant &info, uint32_t namespaceId) {
	BaseProtocol::GetStats(info, namespaceId);
	info["streams"].IsArray(true);
	Variant si;
	if (GetApplication() != NULL) {
		StreamsManager *pStreamsManager = GetApplication()->GetStreamsManager();
		map<uint32_t, BaseStream*> streams = pStreamsManager->FindByProtocolId(GetId());

		FOR_MAP(streams, uint32_t, BaseStream *, i) {
			si.Reset();
			MAP_VAL(i)->GetStats(si, namespaceId);
			info["streams"].PushToArray(si);
		}
	}
}

bool InboundLiveFLVProtocol::InitializeStream(string streamName) {
	streamName = ComputeStreamName(streamName);

	//Get the public stream name
	string publicStreamName = GetApplication()->GetIngestPointPublicName(streamName);
	if (publicStreamName == "") {
		FATAL("No ingest point found for %s", STR(streamName));
		return false;
	}

	//Ensure the stream name to be ingested is available
	if (!GetApplication()->StreamNameAvailable(streamName, this, false)) {
		FATAL("Stream %s already taken", STR(streamName));
		return false;
	}

	//Create the in stream
	_pStream = new InNetLiveFLVStream(this, publicStreamName);
	if (!_pStream->SetStreamsManager(GetApplication()->GetStreamsManager())) {
		FATAL("Unable to set the streams manager");
		delete _pStream;
		_pStream = NULL;
		return false;
	}

	//Get the list of waiting subscribers
	map<uint32_t, BaseOutStream *> subscribedOutStreams =
			GetApplication()->GetStreamsManager()->GetWaitingSubscribers(
			publicStreamName, _pStream->GetType(), true);

	//Bind the waiting subscribers

	FOR_MAP(subscribedOutStreams, uint32_t, BaseOutStream *, i) {
		BaseOutStream *pBaseOutStream = MAP_VAL(i);
		pBaseOutStream->Link(_pStream);
	}
	return true;
}

string InboundLiveFLVProtocol::ComputeStreamName(string suggestion) {
	//1. validate the suggestion
	trim(suggestion);
	if (suggestion != "")
		return suggestion;

	//2. Pick it up from the acceptor
	Variant &customParameters = GetCustomParameters();
	if (customParameters.HasKeyChain(V_STRING, true, 1, "localStreamName")) {
		string streamName = customParameters["localStreamName"];
		trim(streamName);
		if (streamName != "")
			return streamName;
	}

	//3. Compute a stream name based on the nature of the carrier (if any...)
	if (GetIOHandler() != NULL) {
		//we have a carrier
		if (GetIOHandler()->GetType() == IOHT_TCP_CARRIER) {
			//this is a tcp carrier
			return format("%s_%hu",
					STR(((TCPCarrier *) GetIOHandler())->GetFarEndpointAddressIp()),
					((TCPCarrier *) GetIOHandler())->GetFarEndpointPort());
		} else {
			//this is not a TCP carrier
			return format("flv_%u", GetId());
		}
	} else {
		//we don't have a carrier. This protocl might be artificially fed
		return format("flv_%u", GetId());
	}
}
#endif /* HAS_PROTOCOL_LIVEFLV */
