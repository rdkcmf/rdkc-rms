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
#ifndef _MSSPUBLISHINGPOINT_H
#define	_MSSPUBLISHINGPOINT_H

#include "common.h"
#include "streaming/streamcapabilities.h"
#include "streaming/xmlelements.h"
#include "application/baseclientapplication.h"

/**
 * MSSPublishingPoint class is responsible for writing the publishing point file.
 * It is directly controlled by OutFileMSSStream class.
 */
class DLLEXP MSSPublishingPoint {
public:
    MSSPublishingPoint();
    virtual ~MSSPublishingPoint();

    /**
     * Initializes the Publishing Point class
     *
     * @param settings Contains the generic
     * @param pStreamCapabs Describes the input stream capabilities
     * @return true on success, false otherwise
     */
    bool Init(Variant &settings, BaseClientApplication* pApplication);

    /**
     * Adds an XML element but has special consideration for elements that should
     * appear only once on publishing point file
     *
     * @param elementName Name of the element to be added
     * @param elementValue Value contained by the element
     * @param isUnique Flag to set indicating that this element is unique
     */
    XmlElements* AddElement(string const& elementName, string const& elementValue, bool isUnique = false);

    /**
     * Add attributes to the just created element
     *
     * @param elementName Name of the element to be added
     * @param attrName Name of the elements attribute to be added
     * @param attrValue String Value of the attribute
     * @return true on success, false otherwise
     */
    template <typename T>
    bool AddAttribute(string const& attrName, T const& attrValue) {
        if (_xmlElements.empty()) {
            // Invalid use of AddAttribute, no parent element exists yet
            return false;
        }

        stringstream strBuffer;
        strBuffer << attrValue;
        // Add the entered attributes to the just added element
        (--_xmlElements.end())->second->AddAttribute(attrName, strBuffer.str());

        return true;
    }

    /**
     * Add a child element to the just recently added element. At this point,
     * only a second level of child is supported. This code need to be modified
     * if in the future it is necessary to support adding a child element
     * that is already a child. It is also assumed that the child element to
     * be added does not have any attribute (refer to moov, metadata and xmpMetadata).
     *
     * @param elementName Name of the element to be added
     * @param elementValue Value contained by the element
     * @param elementAttributes Attributes of the element
     * @return true on success, false otherwise
     */
    XmlElements* AddChildElement(string const& parentElement, string const& elementName, string const& elementValue, multimap<string, XmlElements*> &elementsList);

    void WriteXmlElements(multimap<string, XmlElements*>& xmlElements, uint16_t level = 1);

    /**
     * Creates the current XML doc at a given path
     *
     * @param path Path to the publishing point file to be created
     * @return true on success, false otherwise
     */
    bool Create();

    XmlElements* FindElement(string const& elementName, multimap<string, XmlElements*> const& elementCollection, uint16_t depth = 0) const;

    /**
     * Creates the publishing point string
     *
     * @return true on success, false otherwise
     */
    bool CreateDefault();

    /**
     * Resets the publishing point internal data to defaults
     */
    void Reset();

    /**
     * Remove unnecessary spaces in the xml file so that it can be put in the uuid atom
     */
    string RemoveExtraSpaces(string xmlString);

     /**
     * Returns the xml in string form so that in can be written in uuid atom
     */

    string GetXmlString();

    void SwapHeadAndBody(string &xmlString);

    void InitializeVaryingValues();

	inline uint16_t GetReferenceCount() const {
		return _referenceCount;
	}

	inline void IncrementReferenceCount() {
		++_referenceCount;
	}

	inline void DecrementReferenceCount() {
		--_referenceCount;
	}

protected:
    // Header of the XML file. Normally includes the version and encoding
    string _xmlHeader;

    // Main XML container tag
    string _xmlParentTag;

private:
    string GetVideoCodecPrivateData(StreamCapabilities* pStreamCapabs);
    string GetAudioCodecPrivateData(StreamCapabilities* pStreamCapabs);

private:

    /**
     * Before adding elements to the publishing point file, it is checked if the
     * element to be added was tagged as 'unique'. If this is the case, if
     * an element already exists on the list, its values would be overridden.
     *
     * @param element Element to be added on the list
     */
    void AddElementToList(XmlElements* element);

    // String representation of the xml 
    string _xmlString;
    const string _newLine;
    Variant _bandwidths;
    Variant _audioBitrates;
    Variant _videoBitrates;
    Variant _streamNames;
    uint32_t _audioSamplingRate;
    uint32_t _channels;
    uint32_t _bitsPerSample;
    uint32_t _packetSize;
    uint32_t _audioTag;
    uint32_t _maxHeight;
    uint32_t _maxWidth;
    uint32_t _height;
    uint32_t _width;
    uint32_t _trackId;
    string _audioCodec;
    string _videoCodec;
    string _codecPrivateDataAudio;
    string _codecPrivateDataVideo;
	uint16_t _referenceCount;

    // Actual container of the publishing point file
    ostringstream _xmlDoc;
    multimap<string, XmlElements*> _xmlElements;

    BaseClientApplication* _pApplication;

};

#endif	/* _MSSPUBLISHINGPOINT_H */
#endif	/* HAS_PROTOCOL_MSS */
