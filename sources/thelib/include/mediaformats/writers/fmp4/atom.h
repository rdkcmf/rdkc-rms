/*
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
 */

#ifndef _ATOM_H
#define	_ATOM_H

#include "common.h"

enum HttpStreamingType {
    HDS = 1,
    MSS,
    MSSINGEST,
    DASH
};

/**
 * Abstract base class for all atom types.
 */
class Atom {
public:
    /**
     * Atom constructor
     *
     * @param type Type of the atom represented by characters
     * @param isFull Flag to indicate that atom is a full version (has flags
     * and version fields, fields default to zero)
     */
    Atom(uint32_t type, bool isFull);

    // Used to initialize the version and flags values
    Atom(uint32_t type, uint8_t version, uint32_t flags);
    Atom(uint32_t type, uint64_t uuidHi, uint64_t uuidLo, uint8_t version, uint32_t flags);

    // destructor
    virtual ~Atom();

    uint64_t GetHeaderSize();

    /**
     * Returns the current size of the atom (header + payload)
     *
     * @return Size of the atom
     */
    uint64_t GetSize();

    bool UpdateSize();

    // parent/child relationship methods
    bool SetParent(Atom* parent);
    Atom* GetParent();

    /**
     * Write the contents of this atom directly to file
     *
     * @return true on success, false otherwise
     */
    bool Write(bool update = false);

    bool WriteBufferToFile(bool update = false);

    void ReadContent(IOBuffer& content);

    void ReserveSpace(uint64_t space);

    bool SetFile(File* file);

    void SetPosition(uint64_t position);
    uint64_t GetStartPosition();
    uint64_t GetEndPosition();
    void SetDirectFileWrite(bool isDirect);

    virtual void Initialize(bool writeDirect = false, File* pFile = NULL,
            uint64_t position = 0);

    // Atom header size = 4 bytes 'atom size' + 4 bytes 'type'
    static const uint8_t ATOM_HEADER_SIZE = 8;

    // Version (3 bytes) + flags (1 byte)
    static const uint8_t ATOM_FULL_VERSION_SIZE = 4;

    // Atoms size limit for the 4 byte container
    static const uint32_t HUGE_ATOM_SIZE = 0x0FFFFFFFF;

    // In case the default size container is maxed out, 8 bytes are added
    static const uint8_t ATOM_EXTENDED_SIZE = 8;

    // 16 bytes are added if a UUID is present in the header
    static const uint8_t ATOM_UUID_SIZE = 16;

    bool _isAudio;
protected:
    bool WriteUI32(uint32_t data, uint64_t position);
    bool ReadUI32(uint32_t* pData, uint64_t position);
    bool WriteString(string& value);
    uint32_t GetStringSize(string& value);

    // Atom generic fields
    uint64_t _size;
    uint32_t _type;
    uint8_t _version;
    uint32_t _flags;
    uint64_t _uuidHi;
    uint64_t _uuidLo;

    // Indicates if the atom has flags and version fields (full format)
    bool _isFull;

    // Pointer to the atom's parent/container
    Atom* _pParent;

    // Storage of the atom's content before being written to a file
    IOBuffer _buffer;

    // Additional members
    bool _hasExtendedSize;
    bool _hasUuid;

    uint64_t _reservedSpace;

    File* _pFile;
    uint64_t _position;
    bool _isDirectFileWrite;

private:
    /**
     * Writes the header fields of the atom directly to file
     *
     * @return true on success, false otherwise
     */
    bool WriteHeader();

    /*
     * Transfers the internal fields of the atom to a buffer and should be
     * implemented by each type of atom class
     */
    virtual bool WriteFields() = 0;

    virtual uint64_t GetFieldsSize() = 0;
};
#endif	/* _ATOM_H */
