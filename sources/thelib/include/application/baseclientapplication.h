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



#ifndef _BASECLIENTAPPLICATION_H
#define	_BASECLIENTAPPLICATION_H

#include "common.h"
#include "streaming/streamsmanager.h"
#include "metadata/metadatamanager.h"
#include "netio/netio.h"

class BaseProtocol;
class BaseAppProtocolHandler;
class BaseStream;
class IOHandler;
class EventLogger;
class BaseEventSink;
class StreamMetadataResolver;

enum OperationType {
	OPERATION_TYPE_STANDARD = 0,
	OPERATION_TYPE_PULL,
	OPERATION_TYPE_PUSH,
	OPERATION_TYPE_HLS,
	OPERATION_TYPE_HDS,
	OPERATION_TYPE_MSS,
    OPERATION_TYPE_DASH,
	OPERATION_TYPE_RECORD,
	OPERATION_TYPE_LAUNCHPROCESS,
	OPERATION_TYPE_WEBRTC,
	OPERATION_TYPE_METADATA,
	OPERATION_TYPE_RESTARTSERVICE
};

class StreamAlias
: public Variant {
public:
	VARIANT_GETSET(string, localStreamName, "");
	VARIANT_GETSET(string, aliasName, "");
	VARIANT_GETSET(uint64_t, creationTime, 0);
	VARIANT_GETSET(int64_t, expirePeriod, 0);
	VARIANT_GETSET(bool, permanent, false);
	VARIANT_GETSET(bool, oneShot, false);

	bool Expired() {
		if (permanent())
			return false;
		else
			return (creationTime() == 0)
			|| (expirePeriod() == 0)
			|| (time(NULL)>(time_t) (creationTime() + expirePeriod()))
			;
	}
};

struct RecordingSession {
    uint8_t hitOnce;
	static const uint8_t FLAG_MP4 = 0x01;
	static const uint8_t FLAG_FLV = 0x02;
	static const uint8_t FLAG_TS = 0x04;
	RecordingSession() : hitOnce(0) {}
	uint8_t GetFlag(string type) {
		if (type == "mp4") {
			return FLAG_MP4;
		}
		else if (type == "flv") {
			return FLAG_FLV;
		}
		else if (type == "ts") {
			return FLAG_TS;
		}
		return 0;
	}
	void SetFlag(uint8_t flag) {
		hitOnce |= flag;
	}
	void SetFlagForType(string type) {
		SetFlag(GetFlag(type));
	}
	void ResetFlag(uint8_t flag) {
		hitOnce &= ~flag;
	}
	void ResetFlags() {
		hitOnce = 0;
	}
	void ResetFlagForType(string type) {
		ResetFlag(GetFlag(type));
	}
	bool IsFlagSet(string type) {
		uint8_t bitFlag = (hitOnce & GetFlag(type));
		return bitFlag > 0;
	}
};

/*!
        @brief
 */
class DLLEXP BaseClientApplication {
private:
	static uint32_t _idGenerator;
	bool _isOrigin;
	uint32_t _id;
	string _name;
	vector<string> _aliases;
	map<uint64_t, BaseAppProtocolHandler *> _protocolsHandlers;
	StreamsManager _streamsManager;
    map<string, StreamAlias> _streamAliases;
	bool _hasStreamAliases;
    uint32_t _streamAliasTimer;
	map<string, string> _ingestPointsPrivateToPublic;
	map<string, string> _ingestPointsPublicToPrivate;
	bool _hasIngestPoints;
	EventLogger *_pEventLogger;
	Variant _dummy;
	StreamMetadataResolver *_pStreamMetadataResolver;
	// different than above metadata, this is "sidecar" stuff
    map<string, RecordingSession> _dummyRet;
    bool _vodRedirectRtmp;
    string _backupRmsAddress;
	map <string, double> _rtspStreamsWithScale;
protected:
	MetadataManager * _pMetadataManager;
	Variant _configuration;
	bool _isDefault;
	Variant _authSettings;
public:
	BaseClientApplication(Variant &configuration);
	virtual ~BaseClientApplication();

	/*!
		@brief Returns the application's id. The id is auto-generated in the constructor
	 */
	uint32_t GetId();

	/*!
		@brief Returns the name of the application, taken from the configuration file.
	 */
	string GetName();

	/*!
		@brief Returns the variant that contains the configuration information about the application.
	 */
	Variant &GetConfiguration();

	/*!
		@brief Returns the alias of the application from the configuration file
	 */
	vector<string> GetAliases();

	/*!
		@brief Returns the boolean that tells if the application is the default application.
	 */
	bool IsDefault();

	virtual bool IsEdge();
	virtual bool IsOrigin();
	StreamsManager *GetStreamsManager();
	StreamMetadataResolver *GetStreamMetadataResolver();

	virtual MetadataManager * GetMetadataManager() {return _pMetadataManager;};

	virtual void SetMetadata(string & mdStr, string & streamName) 	// hook for "watching" metadata stream
				{(void)(mdStr); (void)(streamName);}; // avoid compiler whine about unused parms

	virtual bool Initialize();

	virtual bool ActivateAcceptors(vector<IOHandler *> &acceptors);
	virtual bool ActivateAcceptor(IOHandler *pIOHandler);
	string GetServicesInfo();
	virtual bool AcceptTCPConnection(TCPAcceptor *pTCPAcceptor);

	/*!
		@brief Registers this application to the BaseAppProtocolHandler.
		@protocolType - Type of protocol
		@pAppProtocolHandler
	 */
	void RegisterAppProtocolHandler(uint64_t protocolType,
			BaseAppProtocolHandler *pAppProtocolHandler);
	/*!
		@brief Erases this application to the BaseAppProtocolHandler by setting it to NULL.
		@param protocolType - Type of protocol
	 */
	void UnRegisterAppProtocolHandler(uint64_t protocolType);

	/*!
		@brief Checks and see if the duplicate inbound network streams are available. Always returns true if allowDuplicateNetworkStreams is set to true inside the config file
		@param streamName - The stream name we want to see is free or not
		@param pProtocol - The protocol associated with this request (can be NULL)
		@param bypassIngestPoints - Whether the name check should disregard the ingest points list
	 */
	virtual bool StreamNameAvailable(string streamName, BaseProtocol *pProtocol,
			bool bypassIngestPoints);

	template<class T>
	T *GetProtocolHandler(BaseProtocol *pProtocol) {
		if (pProtocol == NULL)
			return NULL;
		return (T *) GetProtocolHandler(pProtocol);
	}
	BaseAppProtocolHandler *GetProtocolHandler(BaseProtocol *pProtocol);
	BaseAppProtocolHandler *GetProtocolHandler(uint64_t protocolType);
	bool HasProtocolHandler(uint64_t protocolType);

	template<class T>
	T *GetProtocolHandler(string &scheme) {
		return (T *) GetProtocolHandler(scheme);
	}
    virtual BaseAppProtocolHandler *GetProtocolHandler(string &scheme);

	/*!
		@brief This is called bt the framework when an outbound connection failed to connect
		@param customParameters
	 */
	virtual bool OutboundConnectionFailed(Variant &customParameters);

	/*!
		@brief Registers the protocol to the client application
		@param pProtocol
	 */
	virtual void RegisterProtocol(BaseProtocol *pProtocol);

	/*!
		@brief Erases the protocol to the client application
		@param pProtocol
	 */
	virtual void UnRegisterProtocol(BaseProtocol *pProtocol);

	/*!
		@brief Displays the registered stream's ID, type, and name in the logs
		@param pStream
	 */
	virtual void SignalStreamRegistered(BaseStream *pStream);

	/*!
		@brief Displays the unregistered stream's ID, type, and name in the logs
		@param pStream
	 */
	virtual void SignalStreamUnRegistered(BaseStream *pStream);

	virtual bool PullExternalStreams();
	virtual bool PullExternalStream(Variant &streamConfig);
	virtual bool PushLocalStream(Variant &streamConfig);
	bool ParseAuthentication();

	virtual void SignalLinkedStreams(BaseInStream *pInStream, BaseOutStream *pOutStream);

	virtual void SignalUnLinkingStreams(BaseInStream *pInStream, BaseOutStream *pOutStream);

	/*!
		@brief Deletes all active protocols and IOHandlers bound to the application.
		@param pApplication
	 */
	static void Shutdown(BaseClientApplication *pApplication);

	/*!
	 * Stream aliasing for playback
	 */
	virtual string GetStreamNameByAlias(const string &streamName, bool remove = true);
    bool SetStreamAlias(string &localStreamName, string &streamAlias, int64_t expirePeriod, StreamAlias *pValue);
	bool RemoveStreamAlias(const string &streamAlias);
    map<string, StreamAlias> & GetAllStreamAliases();
    bool HasExpiredAlias(string streamAlias);

	/*!
	 * Ingest points management
	 */
	virtual string GetIngestPointPublicName(string &privateStreamName);
	virtual string GetIngestPointPrivateName(string &publicStreamName);
	virtual bool CreateIngestPoint(string &privateStreamName, string &publicStreamName);
	virtual void RemoveIngestPoint(string &privateStreamName, string &publicStreamName);
	virtual map<string, string> & ListIngestPoints();

	/*!
	 * @brief returns the event logger associated with the application
	 */
	EventLogger * GetEventLogger();

	/*!
	 * @brief When event logger sinks are created, this function is called
	 * to inform the application about new sinks. This is the perfect place
	 * to inspect the type of the sink and do more appropriate in-depth
	 * setup for that sink. Example, set up protocol handlers on it
	 * if the sink needs to do network I/O
	 */
	virtual void SignalEventSinkCreated(BaseEventSink *pSink);

	/*!
	 * @brief this is triggered every time an event is about to be logged
	 *
	 * @param pEventSink the sink receiving the event
	 * @param event - the event to be logged
	 *
	 * @discussion the event will not be logged to any sink if it is reset
	 * using event.Reset()
	 */
	virtual void EventLogEntry(BaseEventSink *pEventSink, Variant &event);

	OperationType GetOperationType(BaseProtocol *pProtocol, Variant &streamConfig);
	OperationType GetOperationType(Variant &allParameters, Variant &streamConfig);
	void StoreConnectionType(Variant &dest, BaseProtocol *pProtocol);
	Variant &GetStreamSettings(Variant &src);

	/*!
	 * @brief this is triggered every time a message is sent by another application
	 * via ClientApplicationManager
	 *
	 * @param message
	 *
	 */
	virtual void SignalApplicationMessage(Variant &message);

	/*!
	 * @brief Function called when RMS is ready. Used to start timers, launch
	 * processes, etc.
	 */
	virtual void SignalServerBootstrapped();
    virtual map<string, RecordingSession>& GetRecordSession();
    void VodRedirectRtmpEnabling();
    bool IsVodRedirectRtmpEnabled();
    string GetBackupRmsAddress();
	void InsertRtspStreamWithScale(string localStreamName, double rtspScale);
	map<string, double> GetRtspStreamsWithScale();
private:
    string GetServiceInfo(IOHandler *pIOHander);
};


#endif	/* _BASECLIENTAPPLICATION_H */


