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


#ifdef HAS_PROTOCOL_MSS
#ifndef _MSSMANIFEST_H
#define _MSSMANIFEST_H

#include "common.h"
#include "streaming/streamcapabilities.h"
#include "streaming/xmlelements.h"

// Since the manifest uses URL format, the separator should be a consistent 'slash'
#define DEFAULT_MSS_MANIFEST_NAME "manifest.ismc"
#define MSS_TIMESCALE (1000)
#define MSS_AUDIO_FOLDER_NAME "audio"
#define MSS_VIDEO_FOLDER_NAME "video"
#define MSS_LOOKAHEAD (2)
#define MSS_DVRWINDOW (0)

/**
 * MSSManifest class is responsible for writing the manifest file that is
 * compliant with the MSSSTR Manifest File Format Specification. It is
 * directly controlled by OutFileMSSStream class.
 *
 * See [MS-SSTR].pdf
 */
class DLLEXP MSSManifest {
public:
    MSSManifest(bool isLive = true);
    virtual ~MSSManifest();

    /**
     * Initializes the Manifest class
     *
     * @param settings Contains the generic
     * @param pStreamCapabs Describes the input stream capabilities
     * @return true on success, false otherwise
     */
    bool Init(Variant &settings, StreamCapabilities *& pStreamCapabs);

    /**
     * Adds an XML element but has special consideration for elements that should
     * appear only once on manifest (TODO: include samples here)
     *
     * @param elementName Name of the element to be added
     * @param elementValue Value contained by the element
     * @param isUnique Flag to set indicating that this element is unique
     */
    template<typename T>
    inline XmlElements* AddElement(string const& elementName, T const& elementValue, bool isUnique = false) {
        XmlElements* element = new XmlElements(elementName, elementValue);
        element->SetIsUnique(isUnique);
        AddElementToList(element);

        return element;
    }

    XmlElements* FindElement(string const& elementName, multimap<string, XmlElements*> const& elementCollection, uint16_t depth = 1) const;

    XmlElements* FindElement(string const& elementName, multimap<string, XmlElements*> const& elementCollection, string const& elementId) const;
    /**
     * Creates the actual Manifest file based on the current state of the
     * Manifest class
     *
     * @param path Path to the manifest file to be created
     * @return true on success, false otherwise
     */
    bool Create();

    void WriteXmlElements(multimap<string, XmlElements*>& xmlElements, uint16_t level = 1);

    /**
     * Forms a working manifest file with the bare necessities
     */
    void FormDefault();

    /**
     * Actual creation of default manifest file as formed by FormDefault().
     *
     * @return true on success, false otherwise
     */
    bool CreateDefault();

    /**
     * Resets the manifest internal data to defaults
     */
    void Reset();

    /**
     * Returns child playlist path (stream level manifest)
     */
    inline string GetManifestPath() const {
        return _manifestAbsolutePath;
    }

    inline bool GetLive() const {
        return _isLive;
    }

    inline uint16_t GetReferenceCount() const {
        return _referenceCount;
    }

	inline void IncrementReferenceCount() {
		++_referenceCount; 
	}

	inline void SetDuration(uint64_t duration) {
		_duration = duration;
	}

	bool AddStreamIndexEntry(XmlElements*& pStreamIndexEntry, uint16_t streamindexLoc);
	bool RemoveStreamIndexEntry(string const& tValue, uint16_t streamindexLoc);
	uint32_t GetChunkCount(bool isAudio);
	void SetChunkCount(bool isAudio, uint32_t value);
private:

	string GetVideoCodecPrivateData();
	string GetAudioCodecPrivateData();
private:

    /**
     * Before adding elements on the manifest file, it is checked if the
     * element to be added was tagged as 'unique'. If this is the case, if
     * an element already exists on the list, its values would be overridden.
     *
     * @param element Element to be added on the list
     */
    void AddElementToList(XmlElements* element);

    // Header of the xml file. Normally includes the version and encoding
    string _xmlHeader;

    // Name of the stream where this DASH stream is pulling from
    string _localStreamName;

    // Bit rate of the source stream
    uint32_t _bandwidth;

    string _majorVersion;
    string _minorVersion;
	uint64_t _duration;
    uint32_t _index;
    string _type;
    uint16_t _videoQualityLevels;
    string _videoFourCC;
    uint16_t _maxWidth;
    uint16_t _maxHeight;
    string _codecPrivateDataVideo;
    uint16_t _nalUnitLengthField;
    uint16_t _audioQualityLevels;
    uint16_t _audioTag;
    uint32_t _samplingRate;
    uint16_t _channels;
    uint16_t _bitsPerSample;
    string _codecPrivateDataAudio;
    uint16_t _packetSize;

    // String representation of the absolute path and file name of the manifest
    string _manifestAbsolutePath;

    // Actual container of the manifest file
    ostringstream _xmlDoc;
    multimap<string, XmlElements*> _xmlElements;

    // Necessary for creating the metadata for the manifest
    StreamCapabilities *_pStreamCapabs;

    const string _newLine;

    bool _isLive;
    uint16_t _referenceCount;
	bool _isAutoMSS;
	Variant _settings;

    // Pointers to each level of xml tags
	XmlElements* _pSmoothStreamingMedia;
    XmlElements* _pStreamIndex;
    XmlElements* _pQualityLevel;
};

#endif /* _MSSMANIFEST_H */
#endif /* HAS_PROTOCOL_MSS */
