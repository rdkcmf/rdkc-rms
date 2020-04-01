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

#include "streaming/dash/dashmanifest.h"
#include <iomanip>

DASHManifest::DASHManifest(bool isLive)
: //xml header
_xmlHeader("<?xml version=\"1.0\" encoding=\"utf-8\"?>"),
//MPD
_xmlParentTag("MPD"),
//MPD attributes
_xmlNameSpace("urn:mpeg:dash:schema:mpd:2011"),
_xsi("http://www.w3.org/2001/XMLSchema-instance"),
_xsiSchemaLocation("urn:mpeg:DASH:schema:MPD:2011"),
_minBufferTime("PT10S"),
_mediaPresentationDuration("PT0S"),
_minimumUpdatePeriod("PT5S"),
//Period attributes
_start("PT0S"),
_periodDuration("PT0S"),
//AdaptationSet attributes common
_segmentAlignment("true"),
_startWithSAP(0),
//AdaptationSet attributes for video
_maxFrameRate(3000),
/*_codecs("avc1.4D401E"),*/ //level 3 Progressive Main Profile (http://dashif.org/w/2013/06/DASH-AVC-264-base-v1.03.pdf)
_codecs("avc1.42c01e"), //level 3 Constrained Baseline Profile
_maxWidth(1920),
_maxHeight(1080),
_mimeType("video/mp4"),
_par("16:9"),
//AdaptationSet attributes for audio
_codecsAudio("mp4a.40.2"), //MPEG-4 AAC Profile (http://dashif.org/w/2013/06/DASH-AVC-264-base-v1.03.pdf)
_mimeTypeAudio("audio/mp4"),
_lang("en"),
//Representation attributes
_bandwidth(0),
_frameRate(24),
_height(640),
_width(480),
_audioSamplingRate(44100),
_channels(2),
_sar("1:1"),
//SegmentList attributes for video
_segmentDuration(0),
_pStreamCapabs(NULL),
_newLine("\n"),
_isLive(isLive),
_referenceCount(0),
_isAutoDASH(false),
_duration(0),
_pPeriod(NULL),
_pAdaptationSet(NULL),
_pRepresentation(NULL),
_pSegmentList(NULL),
_pSegmentTemplate(NULL),
_pSegmentTimeline(NULL) {
    if (_isLive) {
        _type = "dynamic";
        _profile = "urn:mpeg:dash:profile:isoff-live:2011";
        _timeShiftBufferDepth = "PT10M";
        _suggestedPresentationDelay = "PT10S";

        if (_availabilityStartTime == "") {
            time_t now = getutctime();
            struct tm* currTime = NULL;
            char availabilityStartTime[30];
            currTime = gmtime(&now);

            strftime (availabilityStartTime, sizeof(availabilityStartTime), "%Y-%m-%dT%H:%M:%SZ", currTime);
            _availabilityStartTime = availabilityStartTime;
        }
    } else {
        _type = "static";
        _profile = "urn:mpeg:dash:profile:isoff-main:2011";
    }

    Reset();
}

DASHManifest::~DASHManifest() {
    Reset();
}

bool DASHManifest::Init(Variant &settings, StreamCapabilities*& pStreamCapabs) {

    // Set here all settings from the user
    bool overwriteDestination = (bool) settings["overwriteDestination"];
    _bandwidth = (uint32_t) settings["bandwidth"];
    _localStreamName = (string) settings["localStreamName"];
    bool cleanupDestination = (bool) settings["cleanupDestination"];
    _segmentDuration = (uint16_t) settings["chunkLength"];

    // Set the stream capabilities
    _pStreamCapabs = pStreamCapabs;
	_isAutoDASH = (bool)settings["isAutoDASH"];

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
        manifestName = DEFAULT_DASH_MANIFEST_NAME;
        settings["manifestName"] = manifestName;
    }

	string manifestFolder = _isAutoDASH ? (string)settings["targetFolder"] : (string)settings["groupTargetFolder"];
	_manifestAbsolutePath = manifestFolder + manifestName;

    // Update the bandwidth
    if (_bandwidth == 0) {
        _bandwidth = (uint32_t) (_pStreamCapabs->GetTransferRate());
    }

	if (_pStreamCapabs->GetVideoCodec() != NULL) {
		_frameRate = (uint32_t)((_pStreamCapabs->GetVideoCodec()->GetFPS() + 1) * 1.5);
		_height = _pStreamCapabs->GetVideoCodec()->_height;
		_width = _pStreamCapabs->GetVideoCodec()->_width;
	}
	if (_pStreamCapabs->GetAudioCodec() != NULL) {
		_audioSamplingRate = _pStreamCapabs->GetAudioCodec()->_samplingRate;
		_channels = _pStreamCapabs->GetAudioCodec()->_channelsCount;
	}

	string videoFolder = format("%s/%s/%"PRIu32"/", STR((string)settings["targetFolder"]), DASH_VIDEO_FOLDER_NAME, _bandwidth);
	string audioFolder = format("%s/%s/%"PRIu32"/", STR((string)settings["targetFolder"]), DASH_AUDIO_FOLDER_NAME, _bandwidth);

    // Check the cleanup destination flag
    // todo: implement cleanupdestination for multiple representation
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
                if ((file.find(".m4s") != string::npos)
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
                if ((file.find(".m4s") != string::npos)
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

    createFolder(videoFolder, true);
    createFolder(audioFolder, true);

    settings["videoFolder"] = videoFolder;
    settings["audioFolder"] = audioFolder;

    return true;
}

bool DASHManifest::Create() {
    File file;
    //FINEST("Creating Manifest...");

    if (!file.Initialize(_manifestAbsolutePath, FILE_OPEN_MODE_TRUNCATE)) {
        FATAL("Unable to create a manifest file");
        return false;
    }
    _xmlDoc.clear();
    _xmlDoc.str(std::string());

	// Update the duration of <MPD> and <Period>
	if (!_isLive) {
		_pPeriod = FindElement("Period", _xmlElements);
		if (_pPeriod) {
			ostringstream tempBuffer;
			tempBuffer.clear();
			tempBuffer.str(std::string());
			tempBuffer << (_duration / 1000.0f);
			_mediaPresentationDuration = "PT" + tempBuffer.str() + "S";

			_pPeriod->SetAttribute("duration", _mediaPresentationDuration);
		}
	}

    // Create the xml header
    _xmlDoc << _xmlHeader << _newLine;
    // Create the MPD tag
    _xmlDoc << "<" << _xmlParentTag << _newLine;
    // Create the MPD attributes
    _xmlDoc << "xmlns:xsi=\"" << _xsi << "\"" << _newLine;
    _xmlDoc << "xsi:schemaLocation=\"" << _xsiSchemaLocation << "\"" << _newLine;
    _xmlDoc << "type=\"" << _type << "\"" << _newLine;
    _xmlDoc << "xmlns=\"" << _xmlNameSpace << "\"" << _newLine;
    _xmlDoc << "profiles=\"" << _profile << "\"" << _newLine;
    if (!_isLive) {
        _xmlDoc << "mediaPresentationDuration=\"" << _mediaPresentationDuration << "\"" << _newLine;
    }
    _xmlDoc << "minBufferTime=\"" << _minBufferTime << "\"";
    if (_isLive) {
        _xmlDoc << _newLine;
        _xmlDoc << "minimumUpdatePeriod=\"" << _minimumUpdatePeriod << "\"" << _newLine;
        _xmlDoc << "timeShiftBufferDepth=\"" << _timeShiftBufferDepth << "\"" << _newLine;
        _xmlDoc << "suggestedPresentationDelay=\"" << _suggestedPresentationDelay << "\"" << _newLine;
		_xmlDoc << "availabilityStartTime=\"" << _availabilityStartTime << "\"";
    }
    _xmlDoc << ">";

    // Recurse and create each available element with their attributes and children
    WriteXmlElements(_xmlElements);

    // Close the manifest file
    _xmlDoc << _newLine + "</" + _xmlParentTag + ">" + _newLine;

    string writeToFile = _xmlDoc.str();
    if (!file.WriteString(writeToFile)) {
        FATAL("Could not create Manifest!");

        return false;
    }

    file.Flush();
    file.Close();

    return true;
}

void DASHManifest::WriteXmlElements(multimap<string, XmlElements*>& xmlElements, uint16_t level) {
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

XmlElements* DASHManifest::FindElement(string const& elementName,
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

XmlElements* DASHManifest::FindElement(string const& elementName,
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

void DASHManifest::FormDefault() {

    // Reset the contents of the manifest
    Reset();

    _pPeriod = AddElement("Period", "");
    _pPeriod->AddAttribute("start", _start);
    _pPeriod->AddAttribute("id", "rms-dash");
    if (!_isLive) {
        _pPeriod->AddAttribute("duration", _periodDuration);
    }

    XmlElements* tempElemPtr = NULL;

	if (_pStreamCapabs->GetVideoCodec() != NULL) {
        //AdaptationSet for video
        _pPeriod->AddChildElement("AdaptationSet", "");
        _pAdaptationSet = FindElement("AdaptationSet", _pPeriod->GetChildElements(), 1);
        if (_pAdaptationSet) {
            _pAdaptationSet->AddAttribute("segmentAlignment", _segmentAlignment);
            _pAdaptationSet->AddAttribute("startWithSAP", _startWithSAP);
            _pAdaptationSet->AddAttribute("maxWidth", _maxWidth);
            _pAdaptationSet->AddAttribute("maxHeight", _maxHeight);
            _pAdaptationSet->AddAttribute("maxFrameRate", _maxFrameRate);
            _pAdaptationSet->AddAttribute("par", _par);

            tempElemPtr = new XmlElements("ContentComponent", "");
            tempElemPtr->AddAttribute("id", 1);
            tempElemPtr->AddAttribute("contentType", "video");
            _pAdaptationSet->AddChildElement(tempElemPtr);

            _pAdaptationSet->AddChildElement("Representation", "");
            _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), 1);
            if (_pRepresentation) {
                _repId.clear();
                _repId.str(std::string());
                _repId << _localStreamName << "Video" << _bandwidth;
                _pRepresentation->AddAttribute("id", _repId.str());
                _pRepresentation->AddAttribute("height", _height);
                _pRepresentation->AddAttribute("width", _width);
                _pRepresentation->AddAttribute("frameRate", _frameRate);
                _pRepresentation->AddAttribute("bandwidth", _bandwidth);
                _pRepresentation->AddAttribute("mimeType", _mimeType);
                _pRepresentation->AddAttribute("codecs", _codecs);
                _pRepresentation->AddAttribute("sar", _sar);
                if (_isLive) {
                    _pRepresentation->AddChildElement("SegmentTemplate", "");
                    _pSegmentTemplate = FindElement("SegmentTemplate", _pRepresentation->GetChildElements(), 1);
                    if (_pSegmentTemplate) {
                        _pSegmentTemplate->AddAttribute("timescale", DASH_TIMESCALE);
						if (!_isAutoDASH) {
							_videoInitialize = format("%s/%s/%"PRIu32"/seg_init.mp4", STR(_localStreamName), DASH_VIDEO_FOLDER_NAME, _bandwidth);
							_videoMedia = format("%s/%s/%"PRIu32"/segment_$Time$.m4s", STR(_localStreamName), DASH_VIDEO_FOLDER_NAME, _bandwidth);
						} else {
							_videoInitialize = format("%s/%"PRIu32"/seg_init.mp4", DASH_VIDEO_FOLDER_NAME, _bandwidth);
							_videoMedia = format("%s/%"PRIu32"/segment_$Time$.m4s", DASH_VIDEO_FOLDER_NAME, _bandwidth);
						}

                        _pSegmentTemplate->AddAttribute("initialization", _videoInitialize);      
						_pSegmentTemplate->AddAttribute("media", _videoMedia);

						_pSegmentTemplate->AddChildElement("SegmentTimeline", "");
                    }
                } else {
                    _pRepresentation->AddChildElement("SegmentList", "");
                    _pSegmentList = FindElement("SegmentList", _pRepresentation->GetChildElements(), 1);
                    if (_pSegmentList) {
                        _pSegmentList->AddAttribute("duration", _segmentDuration * DASH_TIMESCALE);
                        _pSegmentList->AddAttribute("timescale", DASH_TIMESCALE);
                    }
                }
            }
        }
    }

	if (_pStreamCapabs->GetAudioCodec() != NULL) {
        //AdaptationSet for audio
        _pPeriod->AddChildElement("AdaptationSet", "");
        _pAdaptationSet = FindElement("AdaptationSet", _pPeriod->GetChildElements(), 2);
        if (_pAdaptationSet) {
            _pAdaptationSet->AddAttribute("segmentAlignment", _segmentAlignment);
            _pAdaptationSet->AddAttribute("startWithSAP", _startWithSAP);
            _pAdaptationSet->AddAttribute("lang", _lang);

            tempElemPtr = new XmlElements("ContentComponent", "");
            tempElemPtr->AddAttribute("id", 2);
            tempElemPtr->AddAttribute("contentType", "audio");
            _pAdaptationSet->AddChildElement(tempElemPtr);

            _pAdaptationSet->AddChildElement("Representation", "");
            _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), 1);
            if (_pRepresentation) {
                _repId.clear();
                _repId.str(std::string());
                _repId << _localStreamName << "Audio" << _bandwidth;
                _pRepresentation->AddAttribute("id", _repId.str());
                _pRepresentation->AddAttribute("bandwidth", _bandwidth);
                _pRepresentation->AddAttribute("audioSamplingRate", _audioSamplingRate);
                _pRepresentation->AddAttribute("mimeType", _mimeTypeAudio);
                _pRepresentation->AddAttribute("codecs", _codecsAudio);

                tempElemPtr = new XmlElements("AudioChannelConfiguration", "");
                tempElemPtr->AddAttribute("schemeIdUri", "urn:mpeg:dash:23003:3:audio_channel_configuration:2011");
				tempElemPtr->AddAttribute("value", _channels);
                _pRepresentation->AddChildElement(tempElemPtr);

                if (_isLive) {
                    _pRepresentation->AddChildElement("SegmentTemplate", "");
                    _pSegmentTemplate = FindElement("SegmentTemplate", _pRepresentation->GetChildElements(), 1);
                    if (_pSegmentTemplate) {
                        _pSegmentTemplate->AddAttribute("timescale", DASH_TIMESCALE);
						if (!_isAutoDASH) {
							_audioInitialize = format("%s/%s/%"PRIu32"/seg_init.mp4", STR(_localStreamName), DASH_AUDIO_FOLDER_NAME, _bandwidth);
							_audioMedia = format("%s/%s/%"PRIu32"/segment_$Time$.m4s", STR(_localStreamName), DASH_AUDIO_FOLDER_NAME, _bandwidth);
						} else {
							_audioInitialize = format("%s/%"PRIu32"/seg_init.mp4", DASH_AUDIO_FOLDER_NAME, _bandwidth);
							_audioMedia = format("%s/%"PRIu32"/segment_$Time$.m4s", DASH_AUDIO_FOLDER_NAME, _bandwidth);
						}
                        _pSegmentTemplate->AddAttribute("initialization", _audioInitialize); 
						_pSegmentTemplate->AddAttribute("media", _audioMedia);

						_pSegmentTemplate->AddChildElement("SegmentTimeline", "");
                    }
                } else {
                    _pRepresentation->AddChildElement("SegmentList", "");
                    _pSegmentList = FindElement("SegmentList", _pRepresentation->GetChildElements(), 1);
                    if (_pSegmentList) {
                        _pSegmentList->AddAttribute("duration", _segmentDuration * DASH_TIMESCALE);
                        _pSegmentList->AddAttribute("timescale", DASH_TIMESCALE);
                    }
                }
            }
        }
    }
}

bool DASHManifest::CreateDefault() {
    if (_xmlElements.size() > 0) {
        _pPeriod = FindElement("Period", _xmlElements);
        if (_pPeriod) {
            uint8_t adaptationCount = (uint8_t) _pPeriod->GetChildElements().count("AdaptationSet");
            if (adaptationCount == 0) {
                return false;
            }

            if (!_isLive) {
                _mediaPresentationDuration = _pPeriod->GetAttribute("duration");
            }

            for (uint16_t i = 1; i <= adaptationCount; ++i) {
                _pAdaptationSet = FindElement("AdaptationSet", _pPeriod->GetChildElements(), i);
                if (_pAdaptationSet) {
                    uint8_t representationCount = (uint8_t) _pAdaptationSet->GetChildElements().count("Representation");
                    representationCount = representationCount > 0 ? representationCount : 1;
                    bool isIdUnique = true;
                    string contentType = FindElement("ContentComponent", _pAdaptationSet->GetChildElements(), 1)->GetAttribute("contentType");

                    for (uint16_t j = 1; j <= representationCount; ++j) {
                        _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), j);
                        if (_pRepresentation) {
                            if (contentType == "video") {
                                _repId.clear();
                                _repId.str(std::string());
                                _repId << _localStreamName << "Video" << _bandwidth;
                                if (_pRepresentation->GetAttribute("id") == _repId.str()) {
                                    isIdUnique = false;
                                    break;
                                }
                            } else if (contentType == "audio") {
                                _repId.clear();
                                _repId.str(std::string());
                                _repId << _localStreamName << "Audio" << _bandwidth;
                                if (_pRepresentation->GetAttribute("id") == _repId.str()) {
                                    isIdUnique = false;
                                    break;
                                }
                            }
                        }
                    }
                    if (isIdUnique) {
                        _pAdaptationSet->AddChildElement("Representation", "");
                        _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), representationCount + 1);
                        if (_pRepresentation) {
                            _pRepresentation->AddAttribute("bandwidth", _bandwidth);
                            if (contentType == "video") {
                                _repId.clear();
                                _repId.str(std::string());
                                _repId << _localStreamName << "Video" << _bandwidth;
                                _pRepresentation->AddAttribute("id", _repId.str());
                                _pRepresentation->AddAttribute("height", _height);
                                _pRepresentation->AddAttribute("width", _width);
                                _pRepresentation->AddAttribute("frameRate", _frameRate);
                                _pRepresentation->AddAttribute("sar", _sar);
                                _pRepresentation->AddAttribute("mimeType", "video/mp4");
                                _pRepresentation->AddAttribute("codecs", "avc1.4D401E");
                                _pRepresentation->AddAttribute("bandwidth", _bandwidth);
                            } else if (contentType == "audio") {
                                _repId.clear();
                                _repId.str(std::string());
                                _repId << _localStreamName << "Audio" << _bandwidth;
                                _pRepresentation->AddAttribute("id", _repId.str());
                                _pRepresentation->AddAttribute("audioSamplingRate", _audioSamplingRate);
                                _pRepresentation->AddAttribute("mimeType", "audio/mp4");
                                _pRepresentation->AddAttribute("codecs", "mp4a.40.2");
                                _pRepresentation->AddAttribute("bandwidth", _bandwidth);

                                XmlElements* tempElemPtr = new XmlElements("AudioChannelConfiguration", "");
                                tempElemPtr->AddAttribute("schemeIdUri", "urn:mpeg:dash:23003:3:audio_channel_configuration:2011");
								tempElemPtr->AddAttribute("value", _channels);
                                _pRepresentation->AddChildElement(tempElemPtr);
                            }

                            if (_isLive) {
                                _pRepresentation->AddChildElement("SegmentTemplate", "");
                                _pSegmentTemplate = FindElement("SegmentTemplate", _pRepresentation->GetChildElements(), 1);
                                if (_pSegmentTemplate) {
                                    _pSegmentTemplate->AddAttribute("timescale", DASH_TIMESCALE);
                                    if (contentType == "video") {
                                        _pSegmentTemplate->AddAttribute("initialization", _videoInitialize);
                                        _pSegmentTemplate->AddAttribute("media", _videoMedia);
                                    } else if (contentType == "audio") {
                                        _pSegmentTemplate->AddAttribute("initialization", _audioInitialize);
                                        _pSegmentTemplate->AddAttribute("media", _audioMedia);
                                    }
                                    _pSegmentTemplate->AddChildElement("SegmentTimeline", "");
                                }
                            } else {
                                _pRepresentation->AddChildElement("SegmentList", "");
                                _pSegmentList = FindElement("SegmentList", _pRepresentation->GetChildElements(), 1);
                                if (_pSegmentList) {
                                    _pSegmentList->AddAttribute("duration", _segmentDuration * DASH_TIMESCALE);
                                    _pSegmentList->AddAttribute("timescale", DASH_TIMESCALE);
                                }
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

void DASHManifest::Reset() {

    FOR_MULTIMAP(_xmlElements, string, XmlElements*, i) {
        delete MAP_VAL(i);
    }

    _xmlElements.clear();

    _pPeriod = NULL;
    _pAdaptationSet = NULL;
    _pSegmentTemplate = NULL;
    _pSegmentTimeline = NULL;
    _pSegmentList = NULL;
    _pRepresentation = NULL;

    // Clear contents of the xml
    _xmlDoc.clear();
    _xmlDoc.str(std::string());
}

void DASHManifest::AddElementToList(XmlElements* element) {
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

bool DASHManifest::AddSegmentListEntry(XmlElements*& pSegmentEntry, uint16_t adaptationLoc, string const& representationId) {
    _pAdaptationSet = FindElement("AdaptationSet", _pPeriod->GetChildElements(), adaptationLoc);
    if (_pAdaptationSet) {
        _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), representationId);
        if (_pRepresentation) {
            _pSegmentList = FindElement("SegmentList", _pRepresentation->GetChildElements());
            if (_pSegmentList) {
                size_t elemCount = _pSegmentList->GetChildElements().count(pSegmentEntry->GetName());

                if (elemCount >= 1) {
                    pair<multimap<string, XmlElements*>::const_iterator, multimap<string, XmlElements*>::const_iterator> rangeIterator;
                    rangeIterator = _pSegmentList->GetChildElements().equal_range(pSegmentEntry->GetName());
                    for (multimap<string, XmlElements*>::const_iterator eIterate = rangeIterator.first;
                            eIterate != rangeIterator.second;
                            ++eIterate) {
                        string elemName = eIterate->second->GetName();
                        if (elemName == "Initialization") {
                            if (eIterate->second->GetAttribute("sourceURL") == pSegmentEntry->GetAttribute("sourceURL")) {
                                return false;
                            }
                        } else if (elemName == "SegmentURL") {
                            if (eIterate->second->GetAttribute("media") == pSegmentEntry->GetAttribute("media")) {
                                return false;
                            }
                        }
                    }
                }
                _pSegmentList->AddChildElement(pSegmentEntry);
            }
        }
    }

    return true;
}

bool DASHManifest::RemoveSegmentListURL(string const& mediaValue, uint16_t adaptationLoc, string const& representationId) {
    _pAdaptationSet = FindElement("AdaptationSet", _pPeriod->GetChildElements(), adaptationLoc);
    if (_pAdaptationSet) {
        _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), representationId);
        if (_pRepresentation) {
            _pSegmentList = FindElement("SegmentList", _pRepresentation->GetChildElements());
            if (_pSegmentList) {
                size_t elemCount = _pSegmentList->GetChildElements().count("SegmentURL");

                if (elemCount >= 1) {
                    pair<multimap<string, XmlElements*>::iterator, multimap<string, XmlElements*>::iterator> rangeIterator;
                    rangeIterator = _pSegmentList->GetChildElements().equal_range("SegmentURL");
                    for (multimap<string, XmlElements*>::iterator eIterate = rangeIterator.first;
                            eIterate != rangeIterator.second;
                            ++eIterate) {
                        if (eIterate->second->GetAttribute("media") == mediaValue) {
                            delete eIterate->second;
                            _pSegmentList->GetChildElements().erase(eIterate);
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool DASHManifest::AddSegmentTimelineEntry(XmlElements*& pSegmentEntry, uint16_t adaptationLoc, string const& representationId) {
    _pAdaptationSet = FindElement("AdaptationSet", _pPeriod->GetChildElements(), adaptationLoc);
    if (_pAdaptationSet) {
        _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), representationId);
        if (_pRepresentation) {
            _pSegmentTemplate= FindElement("SegmentTemplate", _pRepresentation->GetChildElements());
            if (_pSegmentTemplate) {
                _pSegmentTimeline = FindElement("SegmentTimeline", _pSegmentTemplate->GetChildElements());
                if (_pSegmentTimeline) {
                    _pSegmentTimeline->AddChildElement(pSegmentEntry);
                }
            }
        }
    }

    return true;
}

bool DASHManifest::RemoveSegmentTimelineEntry(string const& tValue, uint16_t adaptationLoc, string const& representationId) {
    _pAdaptationSet = FindElement("AdaptationSet", _pPeriod->GetChildElements(), adaptationLoc);
    if (_pAdaptationSet) {
        _pRepresentation = FindElement("Representation", _pAdaptationSet->GetChildElements(), representationId);
        if (_pRepresentation) {
            _pSegmentTemplate = FindElement("SegmentTemplate", _pRepresentation->GetChildElements());
            if (_pSegmentTemplate) {
                _pSegmentTimeline = FindElement("SegmentTimeline", _pSegmentTemplate->GetChildElements());
                if (_pSegmentTimeline) {
                    size_t elemCount = _pSegmentTimeline->GetChildElements().count("S");

                    if (elemCount >= 1) {
                        pair<multimap<string, XmlElements*>::iterator, multimap<string, XmlElements*>::iterator> rangeIterator;
                        rangeIterator = _pSegmentTimeline->GetChildElements().equal_range("S");
                        for (multimap<string, XmlElements*>::iterator eIterate = rangeIterator.first;
                                eIterate != rangeIterator.second;
                                ++eIterate) {
                            if (eIterate->second->GetAttribute("t") == tValue) {
                                delete eIterate->second;
                                _pSegmentTimeline->GetChildElements().erase(eIterate);
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}
#endif /* HAS_PROTOCOL_DASH */
