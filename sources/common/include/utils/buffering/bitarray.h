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



#ifndef _BITARRAY_H
#define	_BITARRAY_H

#include "utils/buffering/iobuffer.h"

class BitArray
: public IOBuffer {
private:
	uint32_t _cursor;
public:

	BitArray() : IOBuffer() {
		_cursor = 0;
	}

	virtual ~BitArray() {

	}
public:

	template<typename T>
	bool PutBits(T data, uint8_t count) {
		if (!EnsureSize(sizeof (T)))
			return false;

		uint8_t *pBuffer = GETIBPOINTER(*this);

		for (uint8_t i = 0; i < count; i++) {
			uint8_t srcBitIndex = sizeof (T)*8 - count + i;
			uint8_t dstByteIndex = (uint8_t) ((_cursor + i) / 8);
			uint8_t dstBitIndex = (_cursor + i) % 8;
			if (dstBitIndex == 0) {
				pBuffer[dstByteIndex] = 0;
			}
			T flag = (data << srcBitIndex);
			flag = (flag >> (sizeof (T)*8 - 1));
			pBuffer[dstByteIndex] |= (flag << (7 - dstBitIndex));
		}
		_cursor += count;
		_published = (_cursor / 8)+(((_cursor % 8) == 0) ? 0 : 1);
		return true;
	}

	template<typename T>
	T ReadBits(uint8_t count) {
		T result = PeekBits<T > (count);
		IgnoreBits(count);
		return result;
	}

	template<typename T>
	T PeekBits(uint8_t count) {
		T result = 0;

		if (GETAVAILABLEBYTESCOUNT(*this) == 0) {
			o_assert(false);
		}

		if (GETAVAILABLEBYTESCOUNT(*this) < (_cursor + count) / 8) {
			o_assert(false);
		}

		if (count>sizeof (T)*8) {
			o_assert(false);
		}

		for (uint8_t i = 0; i < count; i++) {
			uint32_t currentCursor = _cursor + i;
			uint8_t currentByteIndex = (uint8_t) (currentCursor / 8);
			uint8_t currentBitIndex = currentCursor % 8;
			uint8_t currentByte = GETIBPOINTER(*this)[currentByteIndex];

			result = (result << 1) | ((currentByte >> (7 - currentBitIndex))&0x01);
		}


		return result;
	}

	void IgnoreBits(uint32_t count) {
		if (GETAVAILABLEBYTESCOUNT(*this) == 0) {
			o_assert(false);
		}

		if (GETAVAILABLEBYTESCOUNT(*this) < (_cursor + count) / 8) {
			o_assert(false);
		}

		_cursor += count;
	}

	uint32_t AvailableBits() {
		return GETAVAILABLEBYTESCOUNT(*this)*8 - _cursor;
	}

	bool ReadExpGolomb(uint64_t &value) {
		value = 1;
		uint32_t zeroBitsCount = 0;
		for (;;) {
			if (AvailableBits() == 0) {
				return false;
			}
			if (ReadBits<bool>(1)) {
				break;
			}
			zeroBitsCount++;
		}
		if (AvailableBits() < zeroBitsCount) {
			return false;
		}
		for (uint32_t i = 0; i < zeroBitsCount; i++) {
			value = ((value << 1) | (ReadBits<uint8_t > (1)));
		}
		value = value - 1;
		return true;
	}
};


#endif	/* _BITARRAY_H */


