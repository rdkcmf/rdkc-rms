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
#include "mediaformats/writers/fmp4/atom.h"

Atom::Atom(uint32_t type, bool isFull) {
	_size = 0;
	_type = type;
	_version = 0;
	_flags = 0;
	_isFull = isFull;
	_pParent = NULL;
	_hasExtendedSize = false;
	_reservedSpace = 0;
	_pFile = NULL;
	_position = 0;
	_isDirectFileWrite = false;
	_isAudio = true;
	_uuidHi = 0;
	_uuidLo = 0;
	_hasUuid = false;
}

Atom::Atom(uint32_t type, uint8_t version, uint32_t flags) {
	_size = 0;
	_type = type;
	_version = version;
	_flags = flags;
	_isFull = true;
	_pParent = NULL;
	_hasExtendedSize = false;
	_reservedSpace = 0;
	_pFile = NULL;
	_position = 0;
	_isDirectFileWrite = false;
	_isAudio = true;
	_uuidHi = 0;
	_uuidLo = 0;
	_hasUuid = false;
}

Atom::Atom(uint32_t type, uint64_t uuidHi, uint64_t uuidLo, uint8_t version, uint32_t flags) {
	_size = 0;
	_type = type;
	_version = version;
	_flags = flags;
	_isFull = true;
	_pParent = NULL;
	_hasExtendedSize = false;
	_reservedSpace = 0;
	_pFile = NULL;
	_position = 0;
	_isDirectFileWrite = false;
	_isAudio = true;
	_uuidHi = uuidHi;
	_uuidLo = uuidLo;
	_hasUuid = true;
}

Atom::~Atom() {

}

uint64_t Atom::GetHeaderSize() {

	uint64_t size = ATOM_HEADER_SIZE;

	if (_isFull) {
		size += ATOM_FULL_VERSION_SIZE;
	}

	if (_hasUuid) {
		size += ATOM_UUID_SIZE;
	}

	return size;
}

uint64_t Atom::GetSize() {

	// Get the field sizes of the atom
	_size = GetFieldsSize();

	// Add the header size
	_size += GetHeaderSize();

	if (_size > HUGE_ATOM_SIZE) {
		_size += ATOM_EXTENDED_SIZE;
		_hasExtendedSize = true;
	} else {
		_hasExtendedSize = false;
	}

	return _size;
}

bool Atom::UpdateSize() {

	// Call GetSize() to update the value of _size and use it
	GetSize();

	if (_isDirectFileWrite) {
		// First, go to the target location on the file
		if (!_pFile->SeekTo(_position)) {
			FATAL("Could not seek to desired file position!");
			return false;
		}

		WriteHeader();

		// Write the header to file
		if (!_pFile->WriteBuffer(GETIBPOINTER(_buffer),
				GETAVAILABLEBYTESCOUNT(_buffer))) {
			FATAL("Could not write header to file!");
			return false;
		}

		if (!_pFile->Flush()) {
			FATAL("Could not flush atom to file!");
			return false;
		}
	}

	return true;
}

bool Atom::SetParent(Atom* parent) {

	_pParent = parent;

	return true;
}

Atom* Atom::GetParent() {

	return _pParent;
}

bool Atom::SetFile(File* file) {

	if (NULL == file) {
		FATAL("File assigned is not initialized!");
		return false;
	}

	_pFile = file;

	return true;
}

void Atom::ReserveSpace(uint64_t space) {

	_reservedSpace = space;
}

bool Atom::WriteHeader() {

	_buffer.IgnoreAll();
	_buffer.ReadFromRepeat(0, 8);

	// Read the headers to buffer
	if (_hasExtendedSize) {
		// Write the extended size indicator
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
				(uint32_t) 0x01);
	} else {
		// Write the size
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8,
				(uint32_t) _size);
	}

	// Write the type
	EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4, _type);

	if (_hasExtendedSize) {
		_buffer.ReadFromRepeat(0, 8);
		// Write the actual extended size
		EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8, _size);
	}

	if (_hasUuid) {
		_buffer.ReadFromRepeat(0, 8);
		EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8, _uuidHi);
		_buffer.ReadFromRepeat(0, 8);
		EHTONLLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 8, _uuidLo);
	}

	if (_isFull) {
		_buffer.ReadFromRepeat(0, 4);

		// Write version and flags
		EHTONLP(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4, _flags);
		*(GETIBPOINTER(_buffer) + GETAVAILABLEBYTESCOUNT(_buffer) - 4) = _version;
	}

	return true;
}

bool Atom::Write(bool update) {

	// Call GetSize() to update the value of _size and use it from here
	GetSize();

	// write the header
	if (!WriteHeader()) {
		FATAL("Unable to write atom header!");
		return false;
	}

	if (_isDirectFileWrite) {
		if (_pFile == NULL) {
			FATAL("File is not initialized!");
			return false;
		}

		/*
		 * First, go to the location on the file
		 */
		if (update) {
			if (!_pFile->SeekTo(_position)) {
				FATAL("Could not seek to desired file position!");
				return false;
			}
		} else {
			if (!_pFile->SeekEnd()) {
				FATAL("Could not seek to end of file!");
				return false;
			}
		}

		// Write the whole headers to file
		if (!_pFile->WriteBuffer(GETIBPOINTER(_buffer),
				GETAVAILABLEBYTESCOUNT(_buffer))) {
			FATAL("Could not write header to file!");
			return false;
		}

		if (!_pFile->Flush()) {
			FATAL("Could not flush atom to file!");
			return false;
		}

		// Now ignore the buffer for fields
		_buffer.IgnoreAll();
	}

	// write the fields
	if (!WriteFields()) {
		FATAL("Unable to write atom fields!");
		return false;
	}

	if (_isDirectFileWrite) {
		// Write the fields to file
		if (!_pFile->WriteBuffer(GETIBPOINTER(_buffer),
				GETAVAILABLEBYTESCOUNT(_buffer))) {
			FATAL("Could not write header to file!");
			return false;
		}

		if (!_pFile->Flush()) {
			FATAL("Could not flush atom to file!");
			return false;
		}
	}

	return true;
}

bool Atom::WriteBufferToFile(bool update) {

	if (_pFile == NULL) {
		FATAL("File is not initialized!");
		return false;
	}

	// Write the contents of this atom to buffer
	Write();

	if (update) {
		if (!_pFile->SeekTo(_position)) {
			FATAL("Could not seek to desired file position!");
			return false;
		}
	} else {
		if (!_pFile->SeekEnd()) {
			FATAL("Could not seek to end of file!");
			return false;
		}
	}

	if (!_pFile->WriteBuffer(GETIBPOINTER(_buffer), GETAVAILABLEBYTESCOUNT(_buffer))) {
		FATAL("Could not write atom to file!");

		return false;
	}

	if (!_pFile->Flush()) {
		FATAL("Could not flush atom to file!");

		return false;
	}

	return true;
}

void Atom::ReadContent(IOBuffer &buffer) {

	// Write the contents of this atom to buffer
	Write();

	// Copy it to the current input buffer
	buffer.ReadFromInputBuffer(_buffer, GETAVAILABLEBYTESCOUNT(_buffer));
}

void Atom::SetPosition(uint64_t position) {

	_position = position;
}

uint64_t Atom::GetStartPosition() {

	return _position;
}

uint64_t Atom::GetEndPosition() {

	return _position + GetSize();
}

void Atom::SetDirectFileWrite(bool isDirect) {

	_isDirectFileWrite = isDirect;
}

bool Atom::WriteUI32(uint32_t data, uint64_t position) {

	// Go to the desired location on the file
	if (!_pFile->SeekTo(position)) {
		FATAL("Could not seek to desired file position!");
		return false;
	}

	// Write the content to file
	if (!_pFile->WriteUI32(data)) {
		FATAL("Could not write content to file!");
		return false;
	}

	return true;
}

bool Atom::ReadUI32(uint32_t* pData, uint64_t position) {

	// Go to the desired location on the file
	if (!_pFile->SeekTo(position)) {
		FATAL("Could not seek to desired file position!");
		return false;
	}

	// Write the content to file
	if (!_pFile->ReadUI32(pData)) {
		FATAL("Could not write content to file!");
		return false;
	}

	return true;
}

bool Atom::WriteString(string& value) {

	if (value.empty()) {
		_buffer.ReadFromByte(0x00);
	} else {
		_buffer.ReadFromString(value);
	}

	return true;
}

uint32_t Atom::GetStringSize(string& value) {

	if (value.empty()) {
		// If string is empty, return a byte size
		return 1;
	} else {
		return (uint32_t) value.length();
	}
}

void Atom::Initialize(bool writeDirect, File* pFile, uint64_t position) {

	// Set if this atom/box will be written directly to file or not
	_isDirectFileWrite = writeDirect;

	// Set the file and file location to be used (as needed)
	_pFile = pFile;
	_position = position;
}
