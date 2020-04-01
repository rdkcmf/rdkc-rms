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


#ifdef HAS_PROTOCOL_DASH
#ifndef _DASHMANIFEST_H
#define	_DASHMANIFEST_H

#include "common.h"
#include "streaming/streamcapabilities.h"
#include "streaming/xmlelements.h"

// Since the manifest uses URL format, the separator should be a consistent 'slash'
#define DEFAULT_DASH_MANIFEST_NAME "manifest.mpd"
#define DASH_TIMESCALE (1000)
#define DASH_AUDIO_FOLDER_NAME "audio"
#define DASH_VIDEO_FOLDER_NAME "video"

/**
 * DASHManifest class is responsible for writing the manifest file that is
 * compliant with the ISO Manifest File Format Specification. It is
 * directly controlled by OutFileDASHStream class.
 *
 * See std_2012_dash_ISO_IEC_23009-1.pdf
 */
class DLLEXP DASHManifest {
public:
	DASHManifest(bool isLive = true);
    virtual ~DASHManifest();

    /**
     * Initializes the Manifest class
     *
     * @param settings Contains the generic
     * @param pStreamCapabs Describes the input stream capabilities
     * @return true on success, false otherwise
     */
	bool Init(Variant &settings, StreamCapabilities*& pStreamCapabs);

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

    inline void UpdateReferenceCount() {
        ++_referenceCount;
    }

	inline  void SetDuration(double duration) {
		_duration = duration;
	}

    bool AddSegmentListEntry(XmlElements*& pSegmentEntry, uint16_t adaptationLoc, string const& representationId);
    bool RemoveSegmentListURL(string const& mediaValue, uint16_t adaptationLoc, string const& representationId);
    bool AddSegmentTimelineEntry(XmlElements*& pSegmentEntry, uint16_t adaptationLoc, string const& representationId);
    bool RemoveSegmentTimelineEntry(string const& tValue, uint16_t adaptationLoc, string const& representationId);
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

    // Main XML container tag
    string _xmlParentTag;

    // Name of the stream where this DASH stream is pulling from
    string _localStreamName;

    // Header(MPD) attributes
    string _type;
    string _profile;
    string _xmlNameSpace;
    string _xsi;
    string _xsiSchemaLocation;
    string _minBufferTime;
    string _mediaPresentationDuration;
    // Used when live playback
    string _minimumUpdatePeriod;
    string _timeShiftBufferDepth;
    string _availabilityStartTime;
    string _suggestedPresentationDelay;

    // Period attributes
    string _start;
    string _periodDuration;

    // AdaptationSet attributes common
    string _segmentAlignment;
    uint16_t _startWithSAP;

    // AdaptationSet attributes for video
    uint16_t _maxFrameRate;
    string _codecs;
    uint16_t _maxWidth;
    uint16_t _maxHeight;
    string _mimeType;
    string _par;

    // AdaptationSet attributes for audio
    string _codecsAudio;
    string _mimeTypeAudio;
    string _lang;

    // Representation attributes
    uint32_t _bandwidth;
    uint32_t _frameRate;
    uint32_t _height;
    uint32_t _width;
    uint32_t _audioSamplingRate;
	uint16_t _channels;
    string _sar;
    ostringstream _repId;

    // SegmentList attributes for video
    uint16_t _segmentDuration;

    // String representation of the absolute/relative path and file name of the manifest
    string _manifestAbsolutePath;
    string _audioInitialize;
    string _audioMedia;
    string _videoInitialize;
    string _videoMedia;
    // Actual container of the manifest file
    ostringstream _xmlDoc;
    multimap<string, XmlElements*> _xmlElements;

    // Necessary for creating the metadata for the manifest
    StreamCapabilities *_pStreamCapabs;

    const string _newLine;
    bool _isLive;
    uint16_t _referenceCount;
	bool _isAutoDASH;
	double _duration;

    // Pointers to each level of xml tags
    XmlElements* _pPeriod;
    XmlElements* _pAdaptationSet;
    XmlElements* _pRepresentation;
    XmlElements* _pSegmentList;
    XmlElements* _pSegmentTemplate;
    XmlElements* _pSegmentTimeline;
};

#endif	/* _DASHMANIFEST_H */
#endif	/* HAS_PROTOCOL_DASH */
