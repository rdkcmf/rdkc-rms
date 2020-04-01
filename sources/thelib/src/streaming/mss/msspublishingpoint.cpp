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

#include "streaming/mss/msspublishingpoint.h"
#include "streaming/codectypes.h"
#include "streaming/streamstypes.h"
#include "streaming/baseinstream.h"
#include <iomanip>
#define DEFAULT_VBANDWIDTH 500000
#define DEFAULT_ABANDWIDTH 64000

MSSPublishingPoint::MSSPublishingPoint()
: _xmlHeader("<?xml version=\"1.0\" encoding=\"utf-8\"?>"),
_xmlParentTag("smil"),
_xmlString(""),
_newLine("\n"),
_audioSamplingRate(48000),
_channels(2),
_bitsPerSample(16),
_packetSize(4),
_audioTag(255), //AAC
_maxHeight(1080),
_maxWidth(1920),
_height(1080),
_width(1920),
_trackId(1),
_audioCodec("AACL"),
_videoCodec("H264"),
_codecPrivateDataAudio("119056E500"),
_codecPrivateDataVideo("000000016742C00DDB0E37EE1000000300100000030301F142AE0000000168CA8CB2"),
_referenceCount(0) {
    _xmlElements.clear();
}

MSSPublishingPoint::~MSSPublishingPoint() {
    
    FOR_MULTIMAP(_xmlElements, string, XmlElements*, i) {
        if (MAP_VAL(i) != NULL) {
            delete MAP_VAL(i);
        }
    }
    _xmlElements.clear();
}

bool MSSPublishingPoint::Init(Variant &settings, BaseClientApplication* pApplication) {

    // Set here all settings from the user
    _bandwidths = settings["bitrates"];
    _audioBitrates = settings["audioBitrates"];
    _videoBitrates = settings["videoBitrates"];
    _streamNames = settings["streamNames"];

    _pApplication = pApplication;

    return true;
}

void MSSPublishingPoint::Reset() {
    // Clear all elements
    _xmlElements.clear();

    // Clear contents of the XML
    _xmlDoc.clear();
    _xmlDoc.str(std::string());
}

XmlElements* MSSPublishingPoint::FindElement(string const& elementName,
        multimap<string, XmlElements*> const& elementCollection,
        uint16_t depth) const {
    uint16_t levelCount = 0;
    size_t elemCount = elementCollection.count(elementName);

    if (depth > elemCount) {
        FATAL("depth cannot be greater than the number of elements");
        return NULL;
    }

    if (elemCount >= 1) {
        pair<multimap<string, XmlElements*>::const_iterator, multimap<string, XmlElements*>::const_iterator> rangeIterator;
        rangeIterator = elementCollection.equal_range(elementName);
        uint16_t rangeCount = (uint16_t)distance(rangeIterator.first, rangeIterator.second);
        for (multimap<string, XmlElements*>::const_iterator eIterate = rangeIterator.first;
                eIterate != rangeIterator.second;
                ++eIterate) {
            ++levelCount;
            if (levelCount == depth ||
                    (depth == 0 && levelCount == rangeCount)) {
                    // depth = 0 is default where the element found will be the last element in a sequence of same element names
                return eIterate->second;
            }
        }
    }
    return NULL;
}

void MSSPublishingPoint::WriteXmlElements(multimap<string, XmlElements*>& xmlElements, uint16_t level) {
    ostringstream strStream;
    strStream.clear();
    strStream.str(std::string());
    strStream << setw(level * 4) << " ";
    string strPadding = strStream.str();

    FOR_MULTIMAP(xmlElements, string, XmlElements*, i) {
        // Add the name of the element
        _xmlDoc << _newLine << strPadding << "<" << MAP_VAL(i)->GetName();
        // Append the attributes. If there is no attributes, it would return
        // an empty string anyway

        _xmlDoc << MAP_VAL(i)->GenerateAttrString();
        bool isSingleLine = false;
        bool isChild = true;

        // Now, add the value of this element including child elements if available
        if (MAP_VAL(i)->GetChildElements().empty()) {
            string elementValue = MAP_VAL(i)->GetValue();
            if (elementValue != "") {
                // Append the closing angular bracket
                _xmlDoc << ">";
                _xmlDoc << elementValue;
                isChild = false;
            } else {
                _xmlDoc << "/>";
                isSingleLine = true;
            }
        } else {
            _xmlDoc << ">";
            WriteXmlElements(MAP_VAL(i)->GetChildElements(), level + 1);
        }

        // Close the tag of this element
        if (!isSingleLine) {
            if (isChild) {
                _xmlDoc << _newLine << strPadding << "</" << MAP_VAL(i)->GetName() << ">";
            } else {
                _xmlDoc << "</" << MAP_VAL(i)->GetName() << ">";
            }
        }
    }
}

bool MSSPublishingPoint::Create() {

    XmlElements element;

    // Create the XML header
    _xmlDoc << _xmlHeader << _newLine;

    // Create the publishing point container
    _xmlDoc << format("<%s xmlns=\"http://www.w3.org/2001/SMIL20/Language\">",
        STR(_xmlParentTag)) << _newLine;

    WriteXmlElements(_xmlElements);

    // Close the publishing point file
    _xmlDoc << _newLine << "</" << _xmlParentTag << ">" << _newLine;

    string writeToFile = _xmlDoc.str();
    _xmlString = RemoveExtraSpaces(writeToFile);
    SwapHeadAndBody(_xmlString);

    //FINEST("Publishing point created.");

    return true;
}

bool MSSPublishingPoint::CreateDefault() {

    // Reset the contents of the publishing point
    Reset();

    XmlElements* pBody;

    AddElement("head", "");

    XmlElements* pMeta = AddChildElement("head", "meta", "", _xmlElements);

    if (pMeta) {
		pMeta->AddAttribute("name", "creator");
		pMeta->AddAttribute("content", "RDKC Media Server(www.comcast.com)");
    }

    pBody = AddElement("body", "");

	XmlElements* pSwitch = AddChildElement("body", "switch", "", _xmlElements);
	multimap<string, XmlElements*> childElements;
	if (pBody)
		childElements = pBody->GetChildElements();

	XmlElements* pAudioVideo;
	XmlElements* pParam;

	StreamCapabilities* pStreamCapabs = NULL;
	// Update the bandwidth
	if (_streamNames.MapSize() == 0) {
		map<uint32_t, BaseStream*> streams = _pApplication->GetStreamsManager()->FindByTypeByName(ST_IN_NET, _streamNames[(uint32_t)0], true, false);
		pStreamCapabs = MAP_VAL(streams.begin())->GetCapabilities();
		if (pStreamCapabs)
			_bandwidths.PushToArray((uint32_t) (pStreamCapabs->GetTransferRate()));
	}

	//loop through the bandwidths
	for (uint32_t i = 0; i < _streamNames.MapSize(); i++) {
        uint32_t currentAudioBw = (uint32_t)_bandwidths[(uint32_t)i];
        uint32_t currentVideoBw = (uint32_t)_bandwidths[(uint32_t)i];
        if (_audioBitrates.MapSize() == _streamNames.MapSize())
            currentAudioBw = (uint32_t)_audioBitrates[(uint32_t)i];
        if (_videoBitrates.MapSize() == _streamNames.MapSize())
            currentVideoBw = (uint32_t)_videoBitrates[(uint32_t)i];

		string currentStreamName = _streamNames[(uint32_t)i];
		map<uint32_t, BaseStream*> streams = _pApplication->GetStreamsManager()->FindByTypeByName(ST_IN_NET, currentStreamName, true, false);

        BaseInStream* mssInStream = NULL;
        mssInStream = (BaseInStream*)MAP_VAL(streams.begin());

		InitializeVaryingValues();

		AudioCodecInfo* audioCodecInfo = NULL;
		VideoCodecInfo* videoCodecInfo = NULL;
		if (mssInStream)
			pStreamCapabs = mssInStream->GetCapabilities();

		if (pStreamCapabs) {
			audioCodecInfo = pStreamCapabs->GetAudioCodec();
			if (audioCodecInfo) {
				_channels = audioCodecInfo->_channelsCount;
				_audioSamplingRate = audioCodecInfo->_samplingRate;
				if (audioCodecInfo->_bitsPerSample > 0)
					_bitsPerSample = audioCodecInfo->_bitsPerSample;
			}
			_codecPrivateDataVideo = GetVideoCodecPrivateData(pStreamCapabs);
			_codecPrivateDataAudio = GetAudioCodecPrivateData(pStreamCapabs);
			videoCodecInfo = pStreamCapabs->GetVideoCodec();
			if (videoCodecInfo) {
				_height =  pStreamCapabs->GetVideoCodec()->_height;
				_width = pStreamCapabs->GetVideoCodec()->_width;
			}
		}
        _maxHeight = _height;
        _maxWidth = _width;

		//check if the fragment is an audio or video fragment
		pAudioVideo = AddChildElement("switch", "video", "", childElements);
		if (pAudioVideo) {
			if (currentVideoBw == 0)
				currentVideoBw = DEFAULT_VBANDWIDTH; // dummy value for compatibility with Smooth server 
												     // that requires bitrate info e.g. MediaWarp USP
			pAudioVideo->AddAttribute("systemBitrate", currentVideoBw);
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "systemBitrate");
				pParam->AddAttribute("value", currentVideoBw);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "trackID");
				pParam->AddAttribute("value", _trackId++);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "CodecPrivateData");
				pParam->AddAttribute("value", _codecPrivateDataVideo);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "FourCC");
				pParam->AddAttribute("value", _videoCodec);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "MaxWidth");
				pParam->AddAttribute("value", _maxWidth);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "MaxHeight");
				pParam->AddAttribute("value", _maxHeight);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "DisplayWidth");
				pParam->AddAttribute("value", _width);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("video", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "DisplayHeight");
				pParam->AddAttribute("value", _height);
				pParam->AddAttribute("valueType", "data");
			}
		}
		pAudioVideo = AddChildElement("switch", "audio", "", childElements);
		if (pAudioVideo) {
			if (currentAudioBw == 0 || currentAudioBw == DEFAULT_VBANDWIDTH)
				currentAudioBw = DEFAULT_ABANDWIDTH; // dummy value for compatibility with Smooth server 
												     // that requires bitrate info e.g. MediaWarp USP
			pAudioVideo->AddAttribute("systemBitrate", currentAudioBw);
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "systemBitrate");
				pParam->AddAttribute("value", currentAudioBw);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "trackID");
				pParam->AddAttribute("value", _trackId++);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "FourCC");
				pParam->AddAttribute("value", _audioCodec);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "SamplingRate");
				pParam->AddAttribute("value", _audioSamplingRate);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "Channels");
				pParam->AddAttribute("value", _channels);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "BitsPerSample");
				pParam->AddAttribute("value", _bitsPerSample);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "PacketSize");
				pParam->AddAttribute("value", _packetSize);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "AudioTag");
				pParam->AddAttribute("value", _audioTag);
				pParam->AddAttribute("valueType", "data");
			}
			pParam = AddChildElement("audio", "param", "", pSwitch->GetChildElements());
			if (pParam) {
				pParam->AddAttribute("name", "CodecPrivateData");
				pParam->AddAttribute("value", _codecPrivateDataAudio);
				pParam->AddAttribute("valueType", "data");
			}
		}
	}

    return Create();
}

//
// XML manipulation
//

void MSSPublishingPoint::AddElementToList(XmlElements* element) {
    bool matchFound = false;
    string elementName = element->GetName();

    multimap<string, XmlElements*>::iterator matchIterate;
    matchIterate = _xmlElements.find(elementName);

    matchFound = (matchIterate != _xmlElements.end());

    if (matchFound && ((matchIterate->second->GetIsUnique()) || element->GetIsUnique())) {
        matchIterate->second = element;
    } else {
        _xmlElements.insert(pair<string, XmlElements*>(elementName, element));
    }
}

XmlElements* MSSPublishingPoint::AddElement(string const& elementName, string const& elementValue, bool isUnique) {
    // Instantiate, set the _isUnique flag and add it the element to the list
    XmlElements *pElement = new XmlElements(elementName, elementValue);

    if (isUnique) {
        pElement->SetIsUnique(isUnique);
    }
    AddElementToList(pElement);
    return pElement;
}

XmlElements* MSSPublishingPoint::AddChildElement(string const& parentElement, string const& elementName, string const& elementValue, multimap<string, XmlElements*> &elementsList) {
    // Check first if we have already some content
    if (elementsList.empty()) {
        // Invalid use of AddChild, no parent element exists yet
        return NULL;
    }

    // Get the last entry on the list of XML elements
    // Now add this string to the parent element
	XmlElements* parentIndex = FindElement(parentElement, elementsList);
	if (parentIndex == NULL) {
		WARN("No element %s found!", STR(parentElement));
		return NULL;
	}

	XmlElements* childElement = new XmlElements(elementName, elementValue);
	parentIndex->AddChildElement(childElement);
    return childElement;
}

string MSSPublishingPoint::RemoveExtraSpaces(string xmlString) {
	bool outsideTag = true;
	bool dotPlaced = true;
	string output;
	for (uint16_t i = 0; i < xmlString.length(); i++) {
		if (xmlString[i] == '<') {
			outsideTag = false;
			output += xmlString[i];
			dotPlaced = false;
		} else if (xmlString[i] == '>') {
			outsideTag = true;
			output += xmlString[i];
		// remove unnecessary spaces if outside tag
		} else if (outsideTag == true && dotPlaced == false) {
			output += 0x0A;
			dotPlaced = true;
		} else if (outsideTag == false) {
			output += xmlString[i];
		}
	}
	return output;
}

void MSSPublishingPoint::SwapHeadAndBody(string &xmlString) {
	
	string output;

	uint32_t bodyPos = (uint32_t) xmlString.find("<body>");
	uint32_t bodyEnd = (uint32_t) xmlString.find("</body>") + 7;
	uint32_t bodyLength = bodyEnd - bodyPos;

	string body(xmlString, bodyPos, bodyLength);

	uint32_t headPos = (uint32_t) xmlString.find("<head>");
	uint32_t headEnd = (uint32_t) xmlString.find("</head>") + 7;
	uint32_t headLength = headEnd - headPos;

	string head(xmlString, headPos, headLength);

	for (uint32_t i=0; i < bodyPos; i++) {
		output += xmlString[i];
	}

	output += head;
	output += 0x0A;
	output += body;
	output += 0x0A;
	for (uint32_t i=headEnd + 1; i < xmlString.length(); i++) {
		output += xmlString[i];
	}

	xmlString = output;
}

string MSSPublishingPoint::GetXmlString() {
	return _xmlString;
}

string MSSPublishingPoint::GetVideoCodecPrivateData(StreamCapabilities* pStreamCapabs) {
    if (pStreamCapabs == NULL)
        return "";

    switch (pStreamCapabs->GetVideoCodecType()) {
        case CODEC_VIDEO_H264:
        {
            VideoCodecInfoH264 *pInfo = pStreamCapabs->GetVideoCodec<VideoCodecInfoH264>();
            if (pInfo == NULL)
                return "";
            IOBuffer &pps = pInfo->GetPPSBuffer();
            IOBuffer &sps = pInfo->GetSPSBuffer();

            return "00000001" + hex(GETIBPOINTER(sps), GETAVAILABLEBYTESCOUNT(sps)) +
                    "00000001" + hex(GETIBPOINTER(pps), GETAVAILABLEBYTESCOUNT(pps));
        }
        default:
        {
            FATAL("Video codec not supported");
            return "";
        }
    }
}

string MSSPublishingPoint::GetAudioCodecPrivateData(StreamCapabilities* pStreamCapabs) {
    if (pStreamCapabs == NULL)
        return "";
    switch (pStreamCapabs->GetAudioCodecType()) {
        case CODEC_AUDIO_AAC:
        {
            AudioCodecInfoAAC *pInfo = pStreamCapabs->GetAudioCodec<AudioCodecInfoAAC>();
            if (pInfo == NULL)
                return "";
            return hex(pInfo->_pCodecBytes, pInfo->_codecBytesLength);
        }
        default:
        {
            FATAL("Audio codec not supported");
            return "";
        }
    }
}

void MSSPublishingPoint::InitializeVaryingValues() {
	//initialize values just in case pStreamCapabs parameters are NULL
	_channels = 2;
	_audioSamplingRate = 48000;
	_bitsPerSample = 16;
	_codecPrivateDataVideo = "000000016742C00DDB0E37EE1000000300100000030301F142AE0000000168CA8CB2";
	_codecPrivateDataAudio = "119056E500";
	_height = 720;
	_width = 1280;
}

#endif /* HAS_PROTOCOL_MSS */
