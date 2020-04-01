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



#ifndef _BASEPROTOCOL_H
#define	_BASEPROTOCOL_H


#include "common.h"
#include "protocols/protocoltypes.h"
#include "utils/readyforsendinterface.h"


class IOBuffer;
class IOHandler;
class BaseClientApplication;
class HTTPInterface;
class EventLogger;
class WSInterface;
class SocketAddress;
class BaseInStream;

/*!
	@class BaseProtocol
	@brief The base class on which all atomic protocols must derive from.
 */
class DLLEXP BaseProtocol
: public ReadyForSendInterface {
private:
	static uint32_t _idGenerator;
	uint32_t _id;
	BaseClientApplication *_pApplication;
	uint32_t _lastKnownApplicationId;
protected:
	uint64_t _type;
	BaseProtocol *_pFarProtocol;
	BaseProtocol *_pNearProtocol;
	bool _deleteFar;
	bool _deleteNear;
	bool _enqueueForDelete;
	bool _gracefullyEnqueueForDelete;
	Variant _customParameters;
	double _creationTimestamp;
public:

	BaseProtocol(uint64_t type);
	virtual ~BaseProtocol();

	//
	//general purpose, fixed methods
	//

	/*!
		@abstract Returns the type of the protocol
	 */
	uint64_t GetType();

	/*!
		@brief Returns the id of the protocol. Each protocol instance has an id. The id is unique across all protocols, no matter the type
	 */
	uint32_t GetId();

	/*!
		@brief Returns the creation timestamp expressed in milliseconds for 1970 epoch
	 */
	double GetSpawnTimestamp();

	/*!
		@brief Gets the far protocol
	 */
	BaseProtocol *GetFarProtocol();

	/*!
		@brief Sets the far protocol
		@param pProtocol
	 */
	void SetFarProtocol(BaseProtocol *pProtocol);

	/*!
		@brief Break the far side of the protocol chain by making far protocol NULL
	 */
	void ResetFarProtocol();

	/*!
		@brief Gets the near protocol
	 */
	BaseProtocol *GetNearProtocol();

	/*!
		@brief Sets the near protocol
		@param pProtocol
	 */
	void SetNearProtocol(BaseProtocol *pProtocol);

	/*!
		@brief Break the near side of the protocol chain by making near protocol NULL
	 */
	void ResetNearProtocol();

	//Normally, if this protocol will be deleted, all the stack will be deleted
	//Using the next 2 functions we can tell where the protocol deletion will stop
	/*!
		@brief Deletes the protocol
		@param deleteNear: Indicates which protocol to delete.
		@discussion Normally, if this protocol will be deleted, all the stack will be deleted. Using the DeleteNearProtocol and DeleteFarProtocol, we can tell where the protocol deletion will stop.
	 */
	void DeleteNearProtocol(bool deleteNear);
	/*!
		@brief Deletes the protocol
		@param deleteNear: Indicates which protocol to delete.
		@discussion Normally, if this protocol will be deleted, all the stack will be deleted. Using the DeleteNearProtocol and DeleteFarProtocol, we can tell where the protocol deletion will stop.
	 */
	void DeleteFarProtocol(bool deleteFar);

	/*!
		@brief Gets the far-most protocol in the protocl chain.
		@discussion Far-end protocol - is the one close to the transport layer
	 */

	BaseProtocol *GetFarEndpoint();

	/*!
		@brief Gets the near-most protocol in the protocl chain.
		@discussion Near-end protocol - is the one close to the business rules layer
	 */
	BaseProtocol *GetNearEndpoint();

	/*!
		@brief Tests to see if the protocol is enqueued for delete
	 */
	bool IsEnqueueForDelete();

	/*!
		@brief Gets the protocol's application
	 */
	BaseClientApplication * GetApplication();

	/*!
		@brief Gets the protocol's last known application. Same as GetApplication if the protocol is currently bound to any application
	 */
	BaseClientApplication * GetLastKnownApplication();

	//This are functions for set/get the costom parameters
	//in case of outbound protocols
	/*!
		@brief Sets the custom parameters in case of outbound protocols
		@param customParameters: Variant contaitning the custom parameters
	 */
	void SetOutboundConnectParameters(Variant &customParameters);

	/*!
		@brief Gets the custom parameters
	 */
	Variant &GetCustomParameters();

	/*!
		@brief This will return complete information about all protocols in the current stack, including the carrier if available.
		@param info
	 */
	void GetStackStats(Variant &info, uint32_t namespaceId = 0);

	//utility functions
	operator string();

	//
	//virtuals
	//

	/*!
		@brief This is called to initialize internal resources specific to each protocol. This is called before adding the protocol to the stack.
		@param parameters
	 */
	virtual bool Initialize(Variant &parameters);

	/*!
	 * @brief called by the framework to insert a witness file
	 * @param path - the full path to the witness file
	 */
	virtual void SetWitnessFile(string path);

	/*!
		@brief Enqueues the protocol to the delete queue imediatly. All transport related routines called from now on, will be ignored.
	 */
	virtual void EnqueueForDelete();

	/*!
		@brief Enqueues the protocol to the delete queue if there is no outstanding outbound data.
		@param fromFarSide
	 */
	virtual void GracefullyEnqueueForDelete(bool fromFarSide = true);

	/*!
		@brief Returns the IO handler of the protocol chain.
	 */
	virtual IOHandler *GetIOHandler();

	/*!
		@brief Sets the IO Handler of the protocol chain.
		@param pCarrier
	 */
	virtual void SetIOHandler(IOHandler *pCarrier);

	/*!
		@brief Gets the input buffers
	 */
	virtual IOBuffer * GetInputBuffer();

	/*!
		@brief Gets the output buffers
	 */
	virtual IOBuffer * GetOutputBuffer();

	virtual SocketAddress* GetDestInfo();

	/*!
		@brief Get the total amount of bytes that this protocol transported (raw bytes).
	 */
	virtual uint64_t GetDecodedBytesCount();

	/*!
		@brief This function can be called by anyone who wants to signal the transport layer that there is data ready to be sent. pExtraParameters usually is a pointer to an OutboundBuffer, but this depends on the protocol type.
	 */
	virtual bool EnqueueForOutbound();

	/*!
		@brief Enqueue the current protocol stack for timed event
		@param seconds
	 */
	virtual bool EnqueueForTimeEvent(uint32_t seconds);

	/*!
		@brief This is invoked by the framework when the protocol is enqueued for timing oprrations
	 */
	virtual bool TimePeriodElapsed();

	/*!
		@brief This is invoked by the framework when the underlaying system is ready to send more data
	 */
	virtual void ReadyForSend();

	/*!
		@brief Sets the protocol's application
		@param application
	 */
	virtual void SetApplication(BaseClientApplication *pApplication);

	/*!
		@brief This is called by the framework when data is available for processing, when making use of connection-less protocols
		@param buffer
		@param pPeerAddress
	 */
	virtual bool SignalInputData(IOBuffer &buffer, SocketAddress *pPeerAddress);


	virtual bool SignalInputData(IOBuffer &buffer, uint32_t rawBufferLength, SocketAddress *pPeerAddress);

	/*!
		@brief This is called by the framework when data is available for processing, directly from the network i/o layer
		@brief recvAmount
		@param pPeerAddress
	 */
	virtual bool SignalInputData(int32_t recvAmount, SocketAddress *pPeerAddress);

	/*!
		@brief This will return a Variant containing various statistic information. This should be overridden if more/less info is desired
		@param info
	 */
	virtual void GetStats(Variant &info, uint32_t namespaceId = 0);

	/*!
		@brief This is called by the framework when an arbitrary entity wants to send data over this protocol
		@param buffer
		@discussion This function must be implemented by the class that inherits this class only if it has support for out of band data injection
	 */
	virtual bool SendOutOfBandData(const IOBuffer &buffer, void *pUserData);
	virtual void SignalOutOfBandDataEnd(void *pUserData);
	virtual void SignalEvent(string eventName, Variant &args);
	//
	//must be implemented by the class that inherits this class
	//

	/*!
		@brief Should return true if this protocol can be linked with a far protocol of type 'type'. Otherwise should return false
		@param type
		@discussion This function must be implemented by the class that inherits this class
	 */
	virtual bool AllowFarProtocol(uint64_t type) = 0;

	/*!
		@brief Should return true if this protocol can be linked with a near protocol of type 'type'. Otherwise should return false
		@param type
		@discussion This function must be implemented by the class that inherits this class
	 */
	virtual bool AllowNearProtocol(uint64_t type) = 0;

	/*!
		@brief This is called by the framework when data is available for processing, directly from the network i/o layer
		@param recvAmount
		@discussion This function must be implemented by the class that inherits this class
	 */
	virtual bool SignalInputData(int32_t recvAmount) = 0;

	/*!
		@brief This is called by the framework when data is available for processing, from the underlaying protocol (NOT i/o layer)
		@param buffer
		@discussion This function must be implemented by the class that inherits this class
	 */
	virtual bool SignalInputData(IOBuffer &buffer) = 0;

	/*!
	 * @brief returns the event logger associated with the application
	 */
	EventLogger * GetEventLogger();
#ifdef HAS_PROTOCOL_HTTP2
	/*!
	 * @brief This is called by the framework when the protocol is bound to a
	 * HTTP channel. If that happens and the returned value is NULL, than the
	 * protocol is not HTTP compatible and it will be closed
	 *
	 * @return A pointer to an instance of a HTTPInterface class
	 *
	 * @discussion The returned value must be alive for the entire existence of
	 * the underlying HTTP protocol. Usually, this should be implemented as
	 * double inheritance because the HTTPInterface only has pure virtual
	 * functions. Here is some sample code:<br><br>
	 * <code>
	 * class MyProtocol<br>
	 * :public BaseProtocol,HTTPInterface<br>
	 * {<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;...<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;virtual const HTTPInterface* GetHTTPInterface() {<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;return this;<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;}<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;...<br>
	 * }<br>
	 * </code><br>
	 */
	virtual HTTPInterface* GetHTTPInterface();
#endif /* HAS_PROTOCOL_HTTP2 */
#ifdef HAS_PROTOCOL_WS
	/*!
	 * @brief This is called by the framework when the protocol is bound to a
	 * WebSocket channel. If that happens and the returned value is NULL, than the
	 * protocol is not WebSocket compatible and it will be closed
	 *
	 * @return A pointer to an instance of a WSInterface class
	 *
	 * @discussion The returned value must be alive for the entire existence of
	 * the underlying WebSocket protocol. Usually, this should be implemented as
	 * double inheritance because the WSInterface only has pure virtual
	 * functions. Here is some sample code:<br><br>
	 * <code>
	 * class MyProtocol<br>
	 * :public BaseProtocol,WSInterface<br>
	 * {<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;...<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;virtual WSInterface* GetWSInterface() {<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;return this;<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;}<br>
	 * &nbsp;&nbsp;&nbsp;&nbsp;...<br>
	 * }<br>
	 * </code><br>
	 */
	virtual WSInterface* GetWSInterface();
#endif /* HAS_PROTOCOL_WS */
private:
	//utility function
	string ToString(uint32_t currentId);
};

#endif	/* _BASEPROTOCOL_H */
