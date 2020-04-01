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


#ifdef HAS_PROTOCOL_HTTP2
#ifndef _HTTPINTERFACE_H
#define	_HTTPINTERFACE_H

#include "common.h"

#define	HTTP_RESULT_OK 200
#define	HTTP_RESULT_NOT_FOUND 404
#define	HTTP_RESULT_SERVER_ERROR 500

enum HttpMethod {
	HTTP_METHOD_UNKNOWN,
	HTTP_METHOD_REQUEST_GET,
	HTTP_METHOD_REQUEST_POST,
	HTTP_METHOD_RESPONSE
};

enum HttpBodyType {
	HTTP_TRANSFER_UNKNOWN,
	HTTP_TRANSFER_CONTENT_LENGTH,
	HTTP_TRANSFER_CHUNKED,
	HTTP_TRANSFER_CONNECTION_CLOSE
};

class HTTPInterface {
public:

	/*!
	 * @brief keep the compiler happy about destructor
	 */
	virtual ~HTTPInterface() {

	}

	/*!
	 * @brief Called by the framework when a request just begun
	 *
	 * @param method - The method type. Can be either HTTP_METHOD_REQUEST_GET or
	 * HTTP_METHOD_REQUEST_POST.
	 *
	 * @param pUri - The uri in the method call. It has '\0' terminator
	 *
	 * @param uriLength - the length in bytes of pUri not including '\0'
	 * terminator
	 *
	 * @param pHttpVersion - The HTTP version used. It has '\0' terminator
	 *
	 * @param httpVersionLength - the length in bytes of pHttpVersion not
	 * including '\0' terminator
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion This signal is the only way of getting the URI of the HTTP
	 * request and must be stored locally if needed later
	 *
	 * @STATE: STATE_INPUT_START -> STATE_INPUT_REQUEST
	 */
	virtual bool SignalRequestBegin(HttpMethod method, const char *pUri,
			const uint32_t uriLength, const char *pHttpVersion,
			const uint32_t httpVersionLength) = 0;

	/*!
	 * @brief Called by the framework when a response just arrived for a
	 * previously made HTTP request
	 *
	 * @param pHttpVersion - The HTTP version used. It has '\0' terminator
	 *
	 * @param httpVersionLength - the length in bytes of pHttpVersion not
	 * including '\0' terminator
	 *
	 * @param code - The HTTP status code (numeric)
	 *
	 * @param pDescription - The HTTP status code description (string
	 * representation). It has '\0' terminator
	 *
	 * @param descriptionLength - the length in bytes of pDescription not
	 * including '\0' terminator
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion This is the only way of getting the HTTP response status. If
	 * any of this informations are later needed, they should be stored locally
	 *
	 * @STATE: STATE_INPUT_START -> STATE_INPUT_RESPONSE
	 */
	virtual bool SignalResponseBegin(const char *pHttpVersion,
			const uint32_t httpVersionLength, uint32_t code,
			const char *pDescription, const uint32_t descriptionLength) = 0;

	/*!
	 * @brief Called by the framework for each and every HTTP header encountered
	 * in the HTTP request or response headers section
	 *
	 * @param pName - The name of the HTTP header. It has '\0' terminator
	 *
	 * @param nameLength - the length in bytes of pName not including '\0'
	 * terminator
	 *
	 * @param pValue - The value of the HTTP header. It has '\0' terminator
	 *
	 * @param valueLength - the length in bytes of pValue not including '\0'
	 * terminator
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion This is the only way of getting the HTTP request/response
	 * headers and they must be stored locally if needed later
	 *
	 * @STATE: STATE_INPUT_HEADER <-> STATE_INPUT_HEADER_BEGIN
	 */
	virtual bool SignalHeader(const char *pName, const uint32_t nameLength,
			const char *pValue, const uint32_t valueLength) = 0;

	/*!
	 * @brief Called by the framework after the HTTP headers section ended.
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion This is the only way of knowing when the HTTP headers section
	 * ended.
	 *
	 * @STATE: STATE_INPUT_HEADER_BEGIN -> STATE_INPUT_END_CRLF
	 */
	virtual bool SignalHeadersEnd() = 0;

	/*!
	 * @brief Called by the framework when a HTTP body section exists.
	 *
	 * @param bodyType - The type of the content. It can be either one of:<br>
	 * HTTP_TRANSFER_CONTENT_LENGTH<br>
	 * HTTP_TRANSFER_CHUNKED<br>
	 * HTTP_TRANSFER_CONNECTION_CLOSE
	 *
	 * @param length - The length of the payload.
	 * For HTTP_TRANSFER_CONTENT_LENGTH this is the entire content length.<br>
	 * For HTTP_TRANSFER_CHUNKED this has the length of the current chunk.<br>
	 * For HTTP_TRANSFER_CONNECTION_CLOSE it has the value of 0 and it should be
	 * ignored
	 *
	 * @param consumed - How much data was consumed from the outstanding data
	 * block. The outstanding data block is either the entire message when the
	 * bodyType is HTTP_TRANSFER_CONTENT_LENGTH or
	 * HTTP_TRANSFER_CONNECTION_CLOSE, or the current chunk when the bodyType is
	 * HTTP_TRANSFER_CHUNKED.
	 *
	 * @param available - The mount of data guaranteed to be available inside
	 * the buffer on the next call to SignalInputData. This value designates the
	 * maximum amount of data that can be consumed from the buffer on the next
	 * call to SignalInputData. Otherwise, the connection is terminated
	 *
	 * @param remaining - The amount of data yet to be retrieved from network to
	 * finish the outstanding block of data (see definition of a block of data
	 * in the description of consumed parameter).
	 * For HTTP_TRANSFER_CONNECTION_CLOSE, this value is always 0 and has no
	 * meaning
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion If any of this informations are later needed, they should be
	 * stored locally. This values are valid until (and including) the next call
	 * to SignalInputData
	 *
	 * @STATE:	STATE_INPUT_END_CRLF -> STATE_INPUT_BODY_CONTENT_LENGTH
	 *			STATE_INPUT_END_CRLF -> STATE_INPUT_BODY_CONNECTION_CLOSE
	 *			STATE_INPUT_END_CRLF -> STATE_INPUT_BODY_CHUNKED_LENGTH
	 */
	virtual bool SignalBodyData(HttpBodyType bodyType, uint32_t length,
			uint32_t consumed, uint32_t available, uint32_t remaining) = 0;

	/*!
	 * @brief Called by the framework when HTTP body section exists and it just
	 * ended
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion It is the only way of knowing when the body ended
	 * for a HTTP_TRANSFER_CHUNKED or HTTP_TRANSFER_CONNECTION_CLOSE type of
	 * transfer. However, it is also triggered for HTTP_TRANSFER_CONTENT_LENGTH
	 *
	 * @STATE:	STATE_INPUT_BODY_CHUNKED_PAYLOAD -> STATE_INPUT_END
	 *			STATE_INPUT_BODY_CONNECTION_CLOSE -> STATE_INPUT_END
	 *			STATE_INPUT_BODY_CONTENT_LENGTH -> STATE_INPUT_END
	 */
	virtual bool SignalBodyEnd() = 0;

	/*!
	 * @brief Called by the framework when the HTTP request transfer is
	 * completed.
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion It is the only way of knowing when the HTTP request
	 * transfer is completed. Here is the perfect place where a response could
	 * be sent back
	 *
	 * @STATE:	STATE_INPUT_BODY_CHUNKED_PAYLOAD -> STATE_INPUT_END
	 *			STATE_INPUT_BODY_CONNECTION_CLOSE -> STATE_INPUT_END
	 *			STATE_INPUT_BODY_CONTENT_LENGTH -> STATE_INPUT_END
	 */
	virtual bool SignalRequestTransferComplete() = 0;

	/*!
	 * @brief Called by the framework when the HTTP response transfer is
	 * completed
	 *
	 * @return should return true for success or false for errors
	 *
	 * @discussion It is the only way of knowing when the HTTP response
	 * transfer is complete. Here is the perfect place where another HTTP
	 * request could be made or the connection could be closed
	 *
	 * @STATE:	STATE_INPUT_BODY_CHUNKED_PAYLOAD -> STATE_INPUT_END
	 *			STATE_INPUT_BODY_CONNECTION_CLOSE -> STATE_INPUT_END
	 *			STATE_INPUT_BODY_CONTENT_LENGTH -> STATE_INPUT_END
	 */
	virtual bool SignalResponseTransferComplete() = 0;

	/*!
	 * @brief Called by the framework when data is about to be sent. This MUST
	 * write the first line from the HTTP request or response
	 *
	 * @param outputBuffer - the the destination buffer
	 *
	 * @return should return true for success or false for errors
	 *
	 * @STATE:	START -> STATE_OUTPUT_HEADERS_BUILD (acting as a client, write
	 *			the GET/POST request)
	 *			STATE_INPUT_END -> STATE_OUTPUT_HEADERS_BUILD (acting as a
	 *			server, write "200 OK" kind of message)
	 */
	virtual bool WriteFirstLine(IOBuffer &outputBuffer) = 0;

	/*!
	 * @brief Called by the framework when a HTTP request is built. This MUST
	 * write the "Host" header value without the "Host: " string and without
	 * the ending new line. Just the host
	 *
	 * @param outputBuffer - the the destination buffer
	 *
	 * @return should return true for success or false for errors
	 */
	virtual bool WriteTargetHost(IOBuffer &outputBuffer) = 0;

	/*!
	 * @brief Called by the framework when data is about to be sent. This should
	 * write any additional HTTP headers, EXCEPT headers that interfere with the
	 * transfer encoding(ex: content-length, transfer-encoding, etc)
	 *
	 * @param outputBuffer - the the destination buffer
	 *
	 * @return should return true for success or false for errors
	 */
	virtual bool WriteCustomHeaders(IOBuffer &outputBuffer) = 0;

	/*!
	 * @brief Called by the framework when data is about to be sent. This will
	 * tell the HTTP protocol what type of output should generate
	 *
	 * @return should return:<br>
	 * HTTP_TRANSFER_CONTENT_LENGTH for Content-Length based transfer<br>
	 * HTTP_TRANSFER_CHUNKED for chunk based transfer<br>
	 * HTTP_TRANSFER_CONNECTION_CLOSE for connection closed based transfer
	 *
	 * @discussion When the returned value is HTTP_TRANSFER_CONTENT_LENGTH, only
	 * one EnqueueForDelete call is permitted per outputted HTTP message
	 * (request or response). GracefullyEnqueueForDelete will trigger flush on
	 * the output buffer<br><br>
	 * When the returned value is HTTP_TRANSFER_CHUNKED, multiple
	 * EnqueueForDelete calls are permitted per outputted HTTP message (request
	 * or response). The outputted HTTP message (request or response) is
	 * considered finished when the first EnqueueForDelete results in 0 bytes
	 * transfered or (Gracefully)EnqueueForDelete is used<br><br>
	 * When the returned value is HTTP_TRANSFER_CONNECTION_CLOSE,
	 * multiple EnqueueForDelete calls are permitted per outputted HTTP message.
	 * However, the connection can't be reused. The end of data signal is
	 * tearing down the connection with (Gracefully)EnqueueForDelete.
	 */
	virtual HttpBodyType GetOutputBodyType() = 0;

	/*!
	 * @brief Called by the framework when the output body type is
	 * HTTP_TRANSFER_CONTENT_LENGTH
	 *
	 * @return This should return the amount of data in bytes that the body will
	 * have, or a value less than 0 if the framework should determine that
	 * dynamically
	 *
	 * @discussion There are cases where we know for sure how many bytes we are
	 * going to transfer. For example, serving a closed file. However, if the
	 * file is very big (gigabytes), we can't simply read them all into memory
	 * and let the framework determine that size. For this purpose, we just
	 * return the amount of data using this function
	 */
	virtual int64_t GetContentLenth() = 0;

	/*!
	 * @brief Called by the framework when the connection is outbound http and
	 * the complete request/response was finished.
	 *
	 * @return should return true if the connection will be reused or false if
	 * the connection is not needed anymore. When false is returned, the HTTP
	 * protocol will be enqueued for delete.
	 *
	 * @discussion When the HTTP protocol is enqueued for delete, the flags
	 * DeleteNearProtocol and DeleteFarProtocol will be evaluated. Those flags
	 * are true by default so all the stack is teared down.
	 *
	 * @STATE:	STATE_INPUT_END -> STATE_OUTPUT_HEADERS_BUILD (should return true)
	 *			STATE_INPUT_END -> EXIT (should return false)
	 */
	virtual bool NeedsNewOutboundRequest() = 0;

	/*!
	 * @brief Called by the framework when the connection is in chunked mode for
	 * output. It will not be called for the rest of the transfer modes
	 *
	 * @return It should return true if there will be more data available in the
	 * future, or false data will not be available anymore
	 *
	 * @discussion This function is called when the system is attempting to send
	 * data outwards and the output buffer is empty. This translates into 2
	 * things:<br>
	 * <br>
	 * 1. No more data will be available. In this case, this function should
	 * return true <br>
	 * 2. There is no data available to be sent right now, but there will be
	 * more data in the future as part of the current request/response<br>
	 * <br>
	 * All this is necessary because we can't send the chunk length value as 0
	 * that has a special meaning for HTTP: transfer of the request/response
	 * body is complete. If you know for sure there will be more data in the
	 * future as part of the same request/response body, this should return
	 * false
	 *
	 * @STATE:	STATE_OUTPUT_BODY_SEND -> STATE_OUTPUT_HEADERS_BUILD_CHUNKED_CONTENT
	 *			STATE_OUTPUT_BODY_SEND -> STATE_OUTPUT_HEADERS_BUILD_LAST_CHUNKED_CONTENT
	 */
	virtual bool HasMoreChunkedData() = 0;

	/*!
	 * @brief Pretty printing function for HttpMethod enum
	 * @param method the value that needs pretty printing
	 * @return Will output the string representation for the method value
	 */
	static inline const char * HttpMethodToString(HttpMethod method) {
		switch (method) {
			case HTTP_METHOD_REQUEST_GET:
				return "GET";
			case HTTP_METHOD_REQUEST_POST:
				return "POST";
			case HTTP_METHOD_RESPONSE:
				return "RESPONSE";
			case HTTP_METHOD_UNKNOWN:
			default:
				return "#(unknown)#";
		}
	};

	/*!
	 * @brief Pretty printing function for HttpBodyType enum
	 * @param type the value that needs pretty printing
	 * @return Will output the string representation for the type value
	 */
	static inline const char * HttpBodyTypeToString(HttpBodyType type) {
		switch (type) {
			case HTTP_TRANSFER_UNKNOWN:
				return "HTTP_TRANSFER_UNKNOWN";
			case HTTP_TRANSFER_CONTENT_LENGTH:
				return "HTTP_TRANSFER_CONTENT_LENGTH";
			case HTTP_TRANSFER_CHUNKED:
				return "HTTP_TRANSFER_CHUNKED";
			case HTTP_TRANSFER_CONNECTION_CLOSE:
				return "HTTP_TRANSFER_CONNECTION_CLOSE";
			default:
				return "#(unknown)#";
		}
	};
};

#endif	/* _HTTPPROTOCOLINTERFACE_H */
#endif /* HAS_PROTOCOL_HTTP2 */
