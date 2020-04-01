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

#include "streaming/mss/mssmanifest.h"
#include "streaming/codectypes.h"
#include <iomanip>

MSSManifest::MSSManifest(bool isLive)
: //xml header
_xmlHeader("<?xml version=\"1.0\" encoding=\"utf-8\"?>"),
_bandwidth(0),
_majorVersion("2"),
_minorVersion("0"),
_duration(0),
_index(0),
_videoQualityLevels(1),
_videoFourCC("H264"),
_maxWidth(1920),
_maxHeight(1080),
_nalUnitLengthField(4),
_audioQualityLevels(1),
_audioTag(255), //AAC
_samplingRate(48000),
_channels(2),
_bitsPerSample(16),
_packetSize(4),
_pStreamCapabs(NULL),
_newLine("\n"),
_isLive(isLive),
_referenceCount(0),
_isAutoMSS(false),
_pSmoothStreamingMedia(NULL),
_pStreamIndex(NULL),
_pQualityLevel(NULL) {
    Reset();
}

MSSManifest::~MSSManifest() {
    Reset();
}

bool MSSManifest::Init(Variant &settings, StreamCapabilities *& pStreamCapabs) {

	_settings = settings;
    // Set here all settings from the user
    bool overwriteDestination = (bool) settings["overwriteDestination"];
    _bandwidth = (uint32_t) settings["bandwidth"];
    _localStreamName = (string) settings["localStreamName"];
    bool cleanupDestination = (bool) settings["cleanupDestination"];

    // Set the stream capabilities
    _pStreamCapabs = pStreamCapabs;
	_isAutoMSS = (bool)settings["isAutoMSS"];

	if (NULL == _pStreamCapabs) {
		FATAL("Stream %s has unknown capabilities", STR(_localStreamName));
		return false;
	}
    /*
     * We want the manifest file to be inside the group name folder and separate
     * from all the fragments.
     *
     * Fragments are located inside the stream name folder
     */
    string manifestName = (string) settings["manifestName"];
    if (manifestName == "") {
        manifestName = DEFAULT_MSS_MANIFEST_NAME;
        settings["manifestName"] = manifestName;
    }

	string manifestFolder = _isAutoMSS ? (string)settings["targetFolder"] : (string)settings["groupTargetFolder"];
	_manifestAbsolutePath = manifestFolder + manifestName;

    // Update the bandwidth
    if (_bandwidth == 0)
        _bandwidth = (uint32_t) (_pStreamCapabs->GetTransferRate());

	if (_pStreamCapabs->GetVideoCodec() != NULL) {
		_maxHeight = (uint16_t)_pStreamCapabs->GetVideoCodec()->_height;
		_maxWidth = (uint16_t)_pStreamCapabs->GetVideoCodec()->_width;
		_codecPrivateDataVideo = GetVideoCodecPrivateData();
	}
	if (_pStreamCapabs->GetAudioCodec() != NULL) {
		_samplingRate = _pStreamCapabs->GetAudioCodec()->_samplingRate;
		_codecPrivateDataAudio = GetAudioCodecPrivateData();
		_channels = _pStreamCapabs->GetAudioCodec()->_channelsCount;
	}

	string videoFolder = (string)settings["videoFolder"];
	string audioFolder = (string)settings["audioFolder"];

    // Check the cleanup destination flag
    // todo: implement cleanupdestination for abr
    if (cleanupDestination && GetReferenceCount() == 1) {
        // Delete the fragments and manifest file
        vector<string> files;
		if (listFolder(manifestFolder, files)) {

            FOR_VECTOR_ITERATOR(string, files, i) {
                string &file = VECTOR_VAL(i);
                if ((file.find((string) settings["manifestName"]) != string::npos)
                        && fileExists(file)) {
                    if (!deleteFile(file)) {
                        FATAL("Unable to delete file %s", STR(file));
                        return false;
                    }
                }
            }
        }
        // Delete the audio fragments
        files.clear();
        if (listFolder(audioFolder, files)) {
            FOR_VECTOR_ITERATOR(string, files, i) {
                string &file = VECTOR_VAL(i);
                if ((file.find(".fmp4") != string::npos)
                        && fileExists(file)) {
                    if (!deleteFile(file)) {
                        FATAL("Unable to delete file %s", STR(file));
                        return false;
                    }
                }
            }
        }
        // Delete the video fragments
        files.clear();
        if (listFolder(videoFolder, files)) {
            FOR_VECTOR_ITERATOR(string, files, i) {
                string &file = VECTOR_VAL(i);
                if ((file.find(".fmp4") != string::npos)
                        && fileExists(file)) {
                    if (!deleteFile(file)) {
                        FATAL("Unable to delete file %s", STR(file));
                        return false;
                    }
                }
            }
        }
    }

    // Check if we can write the manifest file
    if (fileExists(_manifestAbsolutePath)) {
        if (!overwriteDestination) {
            // We are not allowed anything
            FATAL("Manifest file %s exists and overwrite is not permitted!",
                    STR(_manifestAbsolutePath));
            return false;
        }
    }

    return true;
}

bool MSSManifest::Create() {
    File file;
    //FINEST("Creating Manifest...");

    if (!file.Initialize(_manifestAbsolutePath, FILE_OPEN_MODE_TRUNCATE)) {
        FATAL("Unable to create a manifest file");
        return false;
    }
    _xmlDoc.clear();
    _xmlDoc.str(std::string());

    // Create the xml header
    _xmlDoc << _xmlHeader;

	// Update the manifest duration
	if (!_isLive) {
		_pSmoothStreamingMedia = FindElement("SmoothStreamingMedia", _xmlElements);
		if (_pSmoothStreamingMedia) {
			_pSmoothStreamingMedia->SetAttribute("Duration", _duration);
		}
	}

    // Recurse and create each available element with their attributes and children
    WriteXmlElements(_xmlElements);

    string writeToFile = _xmlDoc.str();
    if (!file.WriteString(writeToFile)) {
        FATAL("Could not create Manifest!");

        return false;
    }

    file.Flush();
    file.Close();

    return true;
}

void MSSManifest::WriteXmlElements(multimap<string, XmlElements*>& xmlElements, uint16_t level) {
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

XmlElements* MSSManifest::FindElement(string const& elementName,
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
        for (multimap<string, XmlElements*>::const_iterator eIterate = rangeIterator.first;
                eIterate != rangeIterator.second;
                ++eIterate) {
            ++levelCount;
            if (levelCount == depth) {
                return eIterate->second;
            }
        }
    }

    return NULL;
}

XmlElements* MSSManifest::FindElement(string const& elementName,
        multimap<string, XmlElements*> const& elementCollection,
        string const& elementId) const {
    size_t elemCount = elementCollection.count(elementName);

    if (elemCount >= 1) {
        pair<multimap<string, XmlElements*>::const_iterator, multimap<string, XmlElements*>::const_iterator> rangeIterator;
        rangeIterator = elementCollection.equal_range(elementName);
        for (multimap<string, XmlElements*>::const_iterator eIterate = rangeIterator.first;
                eIterate != rangeIterator.second;
                ++eIterate) {
            if (eIterate->second->GetAttribute("id") == elementId) {
                return eIterate->second;
            }
        }
    }

    return NULL;
}

void MSSManifest::FormDefault() {

    // Reset the contents of the manifest
    Reset();
	_pSmoothStreamingMedia = AddElement("SmoothStreamingMedia", "");
	_pSmoothStreamingMedia->AddAttribute("MajorVersion", _majorVersion);
	_pSmoothStreamingMedia->AddAttribute("MinorVersion", _minorVersion);
	_pSmoothStreamingMedia->AddAttribute("TimeScale", MSS_TIMESCALE);
	_pSmoothStreamingMedia->AddAttribute("Duration", _duration);
	if (_isLive) {
		_pSmoothStreamingMedia->AddAttribute("LookAheadFragmentCount", MSS_LOOKAHEAD);
		_pSmoothStreamingMedia->AddAttribute("IsLive", "TRUE");
		_pSmoothStreamingMedia->AddAttribute("DVRWindowLength", MSS_TIMESCALE * MSS_DVRWINDOW);
	}

	if (_pStreamCapabs->GetVideoCodec() != NULL) {
		_pSmoothStreamingMedia->AddChildElement("StreamIndex", "");
		_pStreamIndex = FindElement("StreamIndex", _pSmoothStreamingMedia->GetChildElements(), 1);
		_pStreamIndex->AddAttribute("Chunks", 0);
		_pStreamIndex->AddAttribute("QualityLevels", _videoQualityLevels);
		_pStreamIndex->AddAttribute("Type", "video");
		if (!_isAutoMSS && _settings["localStreamNames"].MapSize() == 1)
			_pStreamIndex->AddAttribute("Url", format("%s/%s/{bitrate}/{start_time}.fmp4", STR(_localStreamName), MSS_VIDEO_FOLDER_NAME));
		else if (!_isAutoMSS && _settings["localStreamNames"].MapSize() > 1)
			_pStreamIndex->AddAttribute("Url", format("{bitrate}/%s/{start_time}.fmp4", MSS_VIDEO_FOLDER_NAME));
		else
			_pStreamIndex->AddAttribute("Url", format("%s/{bitrate}/{start_time}.fmp4", MSS_VIDEO_FOLDER_NAME));

		_pStreamIndex->AddChildElement("QualityLevel", "");
		_pQualityLevel = FindElement("QualityLevel", _pStreamIndex->GetChildElements(), 1);
		if (_pQualityLevel) {
			_pQualityLevel->AddAttribute("Index", _index);
			_pQualityLevel->AddAttribute("Bitrate", _bandwidth);
			_pQualityLevel->AddAttribute("FourCC", _videoFourCC);
			_pQualityLevel->AddAttribute("MaxWidth", _maxWidth);
			_pQualityLevel->AddAttribute("MaxHeight", _maxHeight);
			_pQualityLevel->AddAttribute("CodecPrivateData", _codecPrivateDataVideo);
			_pQualityLevel->AddAttribute("NALUnitLengthField", _nalUnitLengthField);
        }
    }

	if (_pStreamCapabs->GetAudioCodec() != NULL) {
		_pSmoothStreamingMedia->AddChildElement("StreamIndex", "");
		_pStreamIndex = FindElement("StreamIndex", _pSmoothStreamingMedia->GetChildElements(), 2);
		_pStreamIndex->AddAttribute("Chunks", 0);
		_pStreamIndex->AddAttribute("QualityLevels", _audioQualityLevels);
		_pStreamIndex->AddAttribute("Type", "audio");
		if (!_isAutoMSS && _settings["localStreamNames"].MapSize() == 1)
			_pStreamIndex->AddAttribute("Url", format("%s/%s/{bitrate}/{start_time}.fmp4", STR(_localStreamName), MSS_AUDIO_FOLDER_NAME));
		else if (!_isAutoMSS && _settings["localStreamNames"].MapSize() > 1)
			_pStreamIndex->AddAttribute("Url", format("{bitrate}/%s/{start_time}.fmp4", MSS_AUDIO_FOLDER_NAME));
		else
			_pStreamIndex->AddAttribute("Url", format("%s/{bitrate}/{start_time}.fmp4", MSS_AUDIO_FOLDER_NAME));

		_pStreamIndex->AddChildElement("QualityLevel", "");
		_pQualityLevel = FindElement("QualityLevel", _pStreamIndex->GetChildElements(), 1);
		if (_pQualityLevel) {
			_pQualityLevel->AddAttribute("Index", _index);
			_pQualityLevel->AddAttribute("Bitrate", _bandwidth);
			_pQualityLevel->AddAttribute("AudioTag", _audioTag);
			_pQualityLevel->AddAttribute("SamplingRate", _samplingRate);
			_pQualityLevel->AddAttribute("Channels", _channels);
			_pQualityLevel->AddAttribute("BitsPerSample", _bitsPerSample);
			_pQualityLevel->AddAttribute("PacketSize", _packetSize);
			_pQualityLevel->AddAttribute("CodecPrivateData", _codecPrivateDataAudio);
		}
	}
}

bool MSSManifest::CreateDefault() {
    if (_xmlElements.size() > 0) {
		_pSmoothStreamingMedia = FindElement("SmoothStreamingMedia", _xmlElements);
		if (_pSmoothStreamingMedia) {
			uint8_t streamIndexCount = (uint8_t)_pSmoothStreamingMedia->GetChildElements().count("StreamIndex");
			if (streamIndexCount == 0)
                return false;

			for (uint16_t i = 1; i <= streamIndexCount; ++i) {
				_pStreamIndex = FindElement("StreamIndex", _pSmoothStreamingMedia->GetChildElements(), i);
				if (_pStreamIndex) {
					uint8_t qualityLevelCount = (uint8_t)_pStreamIndex->GetChildElements().count("QualityLevel");
					qualityLevelCount = qualityLevelCount > 0 ? qualityLevelCount : 1;
                    bool isIdUnique = true;
					string contentType = _pStreamIndex->GetAttribute("Type");

					for (uint16_t j = 1; j <= qualityLevelCount; ++j) {
						_pQualityLevel = FindElement("QualityLevel", _pStreamIndex->GetChildElements(), j);
						if (_pQualityLevel) {
                            if (contentType == "video") {
								ostringstream bitrate;
								bitrate.clear();
								bitrate.str(string());
								bitrate << _bandwidth;

								if (_pQualityLevel->GetAttribute("Bitrate") == bitrate.str()) {
                                    isIdUnique = false;
                                    break;
                                }
                            } else if (contentType == "audio") {
								ostringstream bitrate;
								bitrate.clear();
								bitrate.str(string());
								bitrate << _bandwidth;

								if (_pQualityLevel->GetAttribute("Bitrate") == bitrate.str()) {
									isIdUnique = false;
									break;
								}
                            }
                        }
                    }
                    if (isIdUnique) {
						_pStreamIndex->AddChildElement("QualityLevel", "");
						_pQualityLevel = FindElement("QualityLevel", _pStreamIndex->GetChildElements(), qualityLevelCount + 1);
						if (_pQualityLevel) {
							_pQualityLevel->AddAttribute("Index", _index);
							_pQualityLevel->AddAttribute("Bitrate", _bandwidth);
                            if (contentType == "video") {
								_pQualityLevel->AddAttribute("FourCC", _videoFourCC);
								_pQualityLevel->AddAttribute("MaxWidth", _maxWidth);
								_pQualityLevel->AddAttribute("MaxHeight", _maxHeight);
								_pQualityLevel->AddAttribute("CodecPrivateData", _codecPrivateDataVideo);
								_pQualityLevel->AddAttribute("NALUnitLengthField", _nalUnitLengthField);
                            } else if (contentType == "audio") {
								_pQualityLevel->AddAttribute("AudioTag", _audioTag);
								_pQualityLevel->AddAttribute("SamplingRate", _samplingRate);
								_pQualityLevel->AddAttribute("Channels", _channels);
								_pQualityLevel->AddAttribute("BitsPerSample", _bitsPerSample);
								_pQualityLevel->AddAttribute("PacketSize", _packetSize);
								_pQualityLevel->AddAttribute("CodecPrivateData", _codecPrivateDataAudio);
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Form the default structure
        FormDefault();
    }

    return Create();
}

void MSSManifest::Reset() {

    FOR_MULTIMAP(_xmlElements, string, XmlElements*, i) {
        delete MAP_VAL(i);
    }

    _xmlElements.clear();

    _pSmoothStreamingMedia = NULL;
    _pStreamIndex = NULL;
    _pQualityLevel = NULL;

    // Clear contents of the xml
    _xmlDoc.clear();
    _xmlDoc.str(std::string());
}

void MSSManifest::AddElementToList(XmlElements* element) {
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

bool MSSManifest::AddStreamIndexEntry(XmlElements*& pStreamIndexEntry, uint16_t streamindexLoc) {
	_pStreamIndex = FindElement("StreamIndex", _pSmoothStreamingMedia->GetChildElements(), streamindexLoc);
	if (_pStreamIndex) {
		_pStreamIndex->AddChildElement(pStreamIndexEntry);
    }

    return true;
}

bool MSSManifest::RemoveStreamIndexEntry(string const& tValue, uint16_t streamindexLoc) {
	_pStreamIndex = FindElement("StreamIndex", _pSmoothStreamingMedia->GetChildElements(), streamindexLoc);
	if (_pStreamIndex) {
		size_t elemCount = _pStreamIndex->GetChildElements().count("c");
        if (elemCount >= 1) {
            pair<multimap<string, XmlElements*>::iterator, multimap<string, XmlElements*>::iterator> rangeIterator;
			rangeIterator = _pStreamIndex->GetChildElements().equal_range("c");
            for (multimap<string, XmlElements*>::iterator eIterate = rangeIterator.first;
                    eIterate != rangeIterator.second;
                    ++eIterate) {
                if (eIterate->second->GetAttribute("t") == tValue) {
                    delete eIterate->second;
					_pStreamIndex->GetChildElements().erase(eIterate);
                    return true;
                }
            }
        }
    }

    return false;
}

string MSSManifest::GetVideoCodecPrivateData() {
	if (_pStreamCapabs == NULL)
		return "";

	switch (_pStreamCapabs->GetVideoCodecType()) {
	case CODEC_VIDEO_H264:
	{
		VideoCodecInfoH264 *pInfo = _pStreamCapabs->GetVideoCodec<VideoCodecInfoH264>();
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

string MSSManifest::GetAudioCodecPrivateData() {
	if (_pStreamCapabs == NULL)
		return "";

	switch (_pStreamCapabs->GetAudioCodecType()) {
	case CODEC_AUDIO_AAC:
	{
		AudioCodecInfoAAC *pInfo = _pStreamCapabs->GetAudioCodec<AudioCodecInfoAAC>();
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

uint32_t MSSManifest::GetChunkCount(bool isAudio) {
	uint16_t childDepth = isAudio ? 2 : 1;
	XmlElements* pStreamIndex = FindElement("StreamIndex", _pSmoothStreamingMedia->GetChildElements(), childDepth);
	if (pStreamIndex) {
		string strChunkCount = pStreamIndex->GetAttribute("Chunks");
		return atoi(strChunkCount.c_str());
	}
	return 0;
}

void MSSManifest::SetChunkCount(bool isAudio, uint32_t value) {
	uint16_t childDepth = isAudio ? 2 : 1;
	XmlElements* pStreamIndex = FindElement("StreamIndex", _pSmoothStreamingMedia->GetChildElements(), childDepth);
	if (pStreamIndex) {
		pStreamIndex->SetAttribute("Chunks", value);
	}
}
#endif /* HAS_PROTOCOL_DASH */
