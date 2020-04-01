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



#ifndef _IOBUFFER_H
#define _IOBUFFER_H

#include "platform/platform.h"
#include "utils/misc/file.h"
#include "utils/misc/mmapfile.h"
#include "utils/misc/crypto.h"
#include "utils/misc/socketaddress.h"

#define GETAVAILABLEBYTESCOUNT(x)     ((x)._published - (x)._consumed)
#define GETIBPOINTER(x)     ((uint8_t *)((x)._pBuffer + (x)._consumed))
#define SETIBSENDLIMIT(x,y) \
{ \
	if(((x)._sendLimit!=0)&&((x)._sendLimit!=0xffffffff)){ \
		ASSERT("Setting a IOBufer send limit over an existing limit"); \
	} \
	(x)._sendLimit=(y); \
}
#define GETIBSENDLIMIT(x) ((x)._sendLimit)

/*!
	@class IOBuffer
	@brief Base class which contains all the necessary functions involving input/output buffering.
 */
class DLLEXP IOBuffer {
public:
	uint8_t *_pBuffer;
	uint32_t _size;
	uint32_t _published;
	uint32_t _consumed;
	uint32_t _minChunkSize;
	socklen_t _dummy;
	uint32_t _sendLimit;
#ifdef WITH_SANITY_INPUT_BUFFER
	File *_pInWitnessFile;
	File *_pOutWitnessFile;
#endif /* WITH_SANITY_INPUT_BUFFER */
public:
	//constructor/destructor and initialization
	IOBuffer();
	virtual ~IOBuffer();

	/*!
	 * @brief Sets up an witness file for storring the traffic
	 * @param path - the full path for the witness file
	 */
	void SetInWitnessFile(const string &path);
	void SetOutWitnessFile(const string &path);

	/*!
		@brief initializes the buffer
		@param expected - The expected size of this buffer
	 */
	void Initialize(uint32_t expected);

	/*!
		@brief Read from TCP File Descriptor and saves it. This function is advisable for connection-oriented sockets.
		@param fd - Descriptor that contains the data
		@param expected - Expected size of the receiving buffer
		@param recvAmount - Size of data received
	 */
	bool ReadFromTCPFd(int32_t fd, uint32_t expected, int32_t &recvAmount, int &err);

	/*!
		@brief Read from UDP File Descriptor and saves it. This function is advisable for connectionless-oriented sockets.
		@param fd - Descriptor that contains the data
		@param recvAmount - Size of data received
		@param peerAddress - The source's address
	 */
	bool ReadFromUDPFd(int32_t fd, int32_t &recvAmount, SocketAddress &peerAddress);

	/*!
		@brief Read from  Standard IO and saves it.
		@param fd - Descriptor that contains the data
		@param expected - Expected size of the receiving buffer
		@param recvAmount - Size of data received
	 */
	bool ReadFromStdio(int32_t fd, uint32_t expected, int32_t &recvAmount);

	/*!
		@brief Read from File Stream and saves it
		@param fs - Descriptor that contains the data
		@param size - Size of the receiving buffer
	 */
	bool ReadFromFs(File &file, uint32_t size);
#ifdef HAS_MMAP
	bool ReadFromFs(MmapFile &file, uint32_t size);
#endif /* HAS_MMAP */
	/*!
		@brief Read data from buffer and copy it.
		@param pBuffer - Buffer that contains the data to be read.
		@param size - Size of data to read.
	 */
	bool ReadFromBuffer(const uint8_t *pBuffer, const uint32_t size);

	/*!
		@brief Read data from an input buffer starting at a designated point
		@param pInputBuffer - Pointer to the input buffer
		@param start - The point where copying starts
		@param size - Size of data to read
	 */
	void ReadFromInputBuffer(IOBuffer *pInputBuffer, uint32_t start, uint32_t size);

	/*!
		@brief Read data from an input buffer
		@param buffer - Buffer that contains the data to be read.
		@param size - Size of data to read.
	 */
	bool ReadFromInputBuffer(const IOBuffer &buffer, uint32_t size);

	/*!
		@brief read data from input buffer and also removes it. If the conditions are right,
		this can be actually reduced to a simple exchange of internal buffers which is the
		fastest way
	 */
	bool ReadFromInputBufferWithIgnore(IOBuffer &src);

	/*!
		@brief Read data from buffer from a string
		@param binary - The string that will be read
	 */
	bool ReadFromString(string binary);

	/*!
		@brief Read data from buffer byte data
		@param byte
	 */
	void ReadFromByte(uint8_t byte);

	/*!
		@brief Read data from a memory BIO
		@param pBIO
	 */
	bool ReadFromBIO(BIO *pBIO);

	/*!
		@brief
	 */
	void ReadFromRepeat(uint8_t byte, uint32_t size);

	//Read from this buffer and put to a destination
	/*!
		@brief Read data from buffer and store it.
		@param fd
		@param size - Size of buffer
		@param sentAmount - Size of data sent
	 */
	bool WriteToTCPFd(int32_t fd, uint32_t size, int32_t &sentAmount, int &err);

	/*!
		@brief Read data from standard IO and store it.
		@param fd
		@param size
	 */
	bool WriteToStdio(int32_t fd, uint32_t size, int32_t &sentAmount);

	//Utility functions

	/*!
		@brief Returns the minimum chunk size. This value is initialized in the constructor.
	 */
	uint32_t GetMinChunkSize();
	/*!
		@brief Sets the value of the minimum chunk size
		@param minChunkSize
	 */
	void SetMinChunkSize(uint32_t minChunkSize);

	/*!
		@brief Returns the size of the buffer that has already been written on.
	 */
	uint32_t GetCurrentWritePosition();

	/*!
		@brief Returns the pointer to the buffer
	 */
	uint8_t *GetPointer();

	//memory management

	/*!
		@brief Ignores a specified size of data in the buffer
		@param size
	 */
	bool Ignore(uint32_t size);

	/*!
		@brief Ignores all data in the buffer
	 */
	bool IgnoreAll();

	/*!
		@brief Moves the data in a buffer to optimize memory space
	 */
	bool MoveData();

	/*!
		@brief Makes sure that there is enough memory space in the buffer. If space is not enough, a new buffer is created.
		@param expected - This function makes sure that the size indicated by this parameter is accommodated.
		@discussion In the case that a new buffer is created and the current buffer has data, the data is copied to the new buffer.
	 */
	bool EnsureSize(uint32_t expected);

	/*!
		@brief
	 */

	static string DumpBuffer(const uint8_t *pBuffer, uint32_t length);
	static string DumpBuffer(MSGHDR &message, uint32_t limit = 0);
	string ToString(uint32_t startIndex = 0, uint32_t limit = 0);
	operator string();

	static void ReleaseDoublePointer(char ***pppPointer);
protected:
	void Cleanup();
	void Recycle();
private:
	void CloseInWitnessFile();
	void CloseOutWitnessFile();
};
#endif //_IOBUFFER_H
