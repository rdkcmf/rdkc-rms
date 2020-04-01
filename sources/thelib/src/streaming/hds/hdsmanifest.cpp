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


#ifdef HAS_PROTOCOL_HDS

#include "streaming/hds/hdsmanifest.h"
#include "protocols/rtmp/amf0serializer.h"

HDSManifest::HDSManifest() : _newLine("\n") {
    _xmlHeader = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
    _xmlParentTag = "manifest";
    _overwriteDestination = true;
    _bandwidth = 0;
    _nameSpaceDefault = "http://ns.adobe.com/f4m/1.0";
    _nameSpace = _nameSpaceDefault;
    _xmlElements.clear();
    _pStreamCapabs = NULL;
    _bootStrapName = "bootstrap";
    _updateMasterPlaylist = false;
    _cleanupDestination = false;
    _createMasterPlaylist = true;
    _bootStrapId = "bootstrapId";
    _isRolling = false;
}

HDSManifest::~HDSManifest() {
    // Replace the manifest file with recorded type
    ConvertToRecorded();

    _xmlElements.clear();
}

bool HDSManifest::Init(Variant &settings, StreamCapabilities *pStreamCapabs) {

    // Set here all settings from the user
    _targetFolder = (string) settings["targetFolder"];
    _overwriteDestination = (bool) settings["overwriteDestination"];
    _baseName = (string) settings["chunkBaseName"];
    _bandwidth = (uint32_t) settings["bandwidth"];
    _localStreamName = (string) settings["localStreamName"];
    _cleanupDestination = (bool) settings["cleanupDestination"];
    _createMasterPlaylist = (bool) settings["createMasterPlaylist"];
    _isRolling = (bool) (settings["playlistType"] == "rolling");

    // Set the stream capabilities
    _pStreamCapabs = pStreamCapabs;

    /*
     * We want the manifest file to be inside the group name folder and separate
     * from all the fragments.
     *
     * Fragments are located inside the stream name folder
     * 
     * UPDATE: Modified to support set-level (master playlist) and stream level
     * manifest files
     * 
     * UPDATE: Older manifest specification will be generated as well
     * 
     */
    string manifestName = (string) settings["manifestName"];
    if (manifestName == "") {
        // Use local stream name if manifest name is empty
        manifestName = _localStreamName + ".f4m";
    }

    _manifestAbsolutePath = _targetFolder + manifestName;
    //FINEST("_manifestAbsolutePath: %s", STR(_manifestAbsolutePath));
    _manifestRelativePath = _localStreamName + HDS_PATH_SEPARATOR + manifestName;
    //FINEST("_manifestRelativePath: %s", STR(_manifestRelativePath));
    _masterPlaylistPath = (string) settings["groupTargetFolder"] + "manifest.f4m";
    //FINEST("_masterPlaylistPath: %s", STR(_masterPlaylistPath));
    _v1PlaylistPath = (string) settings["groupTargetFolder"] + "manifest_v1.f4m";
    _v1Url = _localStreamName + HDS_PATH_SEPARATOR + _baseName;

    // Check the cleanup destination flag
    if (_cleanupDestination) {
        // Delete the fragments, bootstrap and manifest file
        vector<string> files;
        if (listFolder(_targetFolder, files)) {

            FOR_VECTOR_ITERATOR(string, files, i) {
                string &file = VECTOR_VAL(i);
                string::size_type pos = file.find("Seg1-");
                if (((pos != string::npos)
                        || (file.find(".f4m") != string::npos)
                        || (file == _bootStrapName))
                        && fileExists(file)) {
                    if (!deleteFile(file)) {
                        FATAL("Unable to delete file %s", STR(file));
                        return false;
                    }
                }
            }
        }
    }

    // Update the bandwidth
    if (_bandwidth == 0) {
        _bandwidth = (uint32_t) (_pStreamCapabs->GetTransferRate());
    }

    // Check if we can write the stream level manifest file (child playlist)
    if (fileExists(_manifestAbsolutePath) || fileExists(_targetFolder + _bootStrapName)) {
        if (_overwriteDestination) {
            if (fileExists(_masterPlaylistPath))
                _updateMasterPlaylist = true;
        } else {
            // We are not allowed anything
            FATAL("Manifest file %s exists and overwrite is not permitted!",
                    STR(_manifestAbsolutePath));
            return false;
        }
    }

    // Check if we want to create a set level manifest file (master playlist)
    if (_createMasterPlaylist) {
        // Check if we need to update the master manifest OR
        // Check if master playlist already exists as well
        if (_updateMasterPlaylist || fileExists(_masterPlaylistPath)) {
            if (!AppendMedia()) {
                FATAL("Unable to append <media> element!");
                return false;
            }
        } else {
            if (!CreateMasterPlaylist()) {
                FATAL("Unable to create master playlist!");
                return false;
            }

            if (!CreatePlaylistV1()) {
                FATAL("Unable to create version 1 manifest!");
                return false;
            }
        }
    }

    return true;
}

void HDSManifest::AddElement(string elementName, string elementValue) {
    // Instantiate an element
    XmlElements element = XmlElements(elementName, elementValue);

    // Add the element to the list...
    AddElementToList(element);
}

void HDSManifest::AddElement(string elementName, string elementValue, bool isUnique) {
    // Instantiate, set the _isUnique flag and add it the element to the list
    XmlElements element = XmlElements(elementName, elementValue);
    element.SetIsUnique(isUnique);
    AddElementToList(element);
}

bool HDSManifest::AddAttribute(string attrName, string attrValue) {
    // Check first if we have already some content
    if (_xmlElements.empty()) {
        // Invalid use of AddAttribute, no parent element exists yet
        return false;
    }

    // Add the entered attributes to the just added element
    (_xmlElements.back()).AddAttribute(attrName, attrValue);

    return true;
}

bool HDSManifest::AddAttribute(string attrName, uint32_t attrValue) {

    // convert attrValue to string
    string strAttrValue = format("%"PRIu32, attrValue);
    return AddAttribute(attrName, strAttrValue);
}

void HDSManifest::SetNameSpace(string nameSpace) {
    _nameSpace = nameSpace;
}

bool HDSManifest::AddChildElement(string elementName, string elementValue) {
    // Check first if we have already some content
    if (_xmlElements.empty()) {
        // Invalid use of AddChild, no parent element exists yet
        return false;
    }

    // Create the string representation of this element
    string child = "<" + elementName + ">" + elementValue
            + "</" + elementName + ">";

    // Get the last entry on the list of XML elements
    // Now add this string to the parent element
    (_xmlElements.back()).AppendValue(child);

    return true;
}

string HDSManifest::CreateMetaData() {
    /*
     * First generate the SCRIPTDATA tag
     * see E.4.4.1 of Adobe Flash Video File Format Specification (v10.1)
     */
    Variant metadata;

    _pStreamCapabs->GetRTMPMetadata(metadata);
    metadata["canSeekToEnd"] = (bool) true; //TODO, check!
    metadata["creationdate"] = GetCreationTime();
    metadata["duration"]; //TODO computed when all fragments are created
    metadata.IsArray(true);

    // Instantiate the AMF serializer and a buffer that will contain the data
    AMF0Serializer amf;
    IOBuffer buff;

    // Serialize it
    string temp = "onMetaData";
    amf.WriteShortString(buff, temp, true);
    amf.Write(buff, metadata);

    // Now convert this to a base64 encoded string
    return b64(GETIBPOINTER(buff), GETAVAILABLEBYTESCOUNT(buff));
}

bool HDSManifest::Create(string path) {
    File file;
    //FINEST("Creating Manifest...");

    if (!file.Initialize(path, FILE_OPEN_MODE_TRUNCATE)) {
        FATAL("Unable to create a manifest file");
        return false;
    }

    XmlElements element;

    // Create the xml header
    _xmlDoc = _xmlHeader + _newLine;

    // Create the manifest container
    _xmlDoc += "<" + _xmlParentTag + " xmlns=\"" + _nameSpace + "\">" + _newLine;

    // Create each available element with their attributes and children

    FOR_VECTOR(_xmlElements, i) {
        element = _xmlElements[i];

        // Add the name of the element
        _xmlDoc += "<" + element.GetName();

        // Append the attributes. If there is no attributes, it would return
        // an empty string anyway
        _xmlDoc += element.GenerateAttrString();

        // Append the closing angular bracket
        _xmlDoc += ">";

        // Now, add the value of this element including child elements if available
        _xmlDoc += element.GetValue();

        // Close the tag of this element
        _xmlDoc += "</" + element.GetName() + ">" + _newLine;
    }

    // Close the manifest file
    _xmlDoc += "</" + _xmlParentTag + ">" + _newLine;

    if (!file.WriteString(_xmlDoc)) {
        FATAL("Could not create Manifest!");

        return false;
    }

    file.Close();

    //FINEST("Manifest created.");

    return true;
}

void HDSManifest::FormDefault() {
    // Reset the contents of the manifest
    Reset();

    AddElement("id", _baseName);

    // Add only the DVR info if it's appending
    if (!_isRolling) {
        AddElement("streamType", "liveOrRecorded");

        // Add the dvrInfo element
        AddElement("dvrInfo", "");
        //AddAttribute("windowDuration", "-1"); // only for v2
        AddAttribute("beginOffset", "0"); // only for v1
        AddAttribute("endOffset", "0"); // only for v1
    } else {
        AddElement("streamType", "live");
    }

    if (_bootStrapName.empty()) {
        FATAL("Empty bootstrap name not supported!");
        //TODO get the base64 encoded value of the bootstrap
        //		string bootStrapInfoB64 = "base64 encoded bootstrap goes here...";
        //
        //		AddElement("bootstrapInfo", bootStrapInfoB64);
    }

    AddElement("bootstrapInfo", "");
    // Add these attributes within bootstrapInfo
    AddAttribute("id", _bootStrapId);
    AddAttribute("url", _bootStrapName);
    AddAttribute("profile", "named"); //TODO modify "named" to get value from user

    AddElement("media", "");
    AddAttribute("bitrate", _bandwidth);
    AddAttribute("url", _baseName);
    AddAttribute("streamId", _localStreamName);
    AddAttribute("bootstrapInfoId", _bootStrapId);
    AddChildElement("metadata", CreateMetaData());
}

bool HDSManifest::CreateDefault() {
    // Form the default structure
    FormDefault();

    return Create(_manifestAbsolutePath);
}

void HDSManifest::Reset() {
    // Clear all elements
    _xmlElements.clear();

    // Clear contents of the xml
    _xmlDoc = "";

    // Reset the name space
    _nameSpace = _nameSpaceDefault;
}

bool HDSManifest::CreateTestManifest() {
    AddElement("id", _baseName);
    AddElement("streamType", "live");
    AddElement("duration", "10");

    AddElement("bootstrapInfo", "");
    AddAttribute("profile", "named");
    AddAttribute("id", _bootStrapId);
    AddAttribute("url", "/bootstrap");

    AddElement("media", "");
    AddAttribute("streamId", _localStreamName);
    AddAttribute("url", _baseName);
    AddAttribute("bootstrapInfoId", _bootStrapId);

    Create(_manifestAbsolutePath);

    return true;
}

void HDSManifest::SetBootStrapName(string bootStrap) {
    _bootStrapName = bootStrap;
}

bool HDSManifest::ReplaceContent(string path, string replaceWhat, string replaceWith, string marker) {
    // Read manifest file
    string manifestString;
    if (!ReadManifest(path, manifestString)) return false;

    // Replace the target content
    replace(manifestString, replaceWhat, replaceWith);
    manifestString += marker;

    //FINEST("Updating Manifest...");
    return SaveManifest(path, manifestString);
}

void HDSManifest::AddElementToList(XmlElements &element) {
    bool matchFound = false;
    int index = 0;

    // Check if we have the same element and has a 'unique' flag set

    FOR_VECTOR(_xmlElements, i) {
        if ((_xmlElements[i]).GetName().compare(element.GetName())) {
            // We skip those that does not match with the element to be added
            continue;
        } else {
            matchFound = true;
            index = i;
            break;
        }
    }

    if (matchFound && ((_xmlElements[index].GetIsUnique()) || element.GetIsUnique())) {
        /**
         * A match has been found! And this is supposed to be unique.
         * We just need to directly assign that element to update it.
         * NOTE: This behavior can be changed as desired
         * (e.g. instead of overwriting it, we could return an error to the user.
         */
        _xmlElements[index] = element;
    } else {
        // Otherwise add it normally to the end of the list
        ADD_VECTOR_END(_xmlElements, element);
    }
}

string HDSManifest::GetCreationTime() {
    // Compute the creation date relative to the manifest file
    double manifestCreationTime;

    GETCLOCKS(manifestCreationTime, double);
    manifestCreationTime = manifestCreationTime / CLOCKS_PER_SECOND;

    Timestamp tm;
    memset(&tm, 0, sizeof (tm));

    time_t timestamp = (time_t) (manifestCreationTime);
    tm = *gmtime(&timestamp);

    char tempBuff[128] = {0};
    strftime(tempBuff, 127, "%Y-%m-%dT%H:%M:%S", &tm);

    return tempBuff;
}

bool HDSManifest::AppendMedia() {
    string id;

    // Append to the v2 manifest
    string entry = format("<media href=\"%s\" bitrate=\"%"PRIu32"\"></media>",
            STR(_manifestRelativePath), _bandwidth);
    id = "href=\"" + _manifestRelativePath + "\"";

    if (!AppendMediaElement(_masterPlaylistPath, entry, id)) {
        return false;
    }

    // Append to the v1 manifest but check first if it does exist!
    if (fileExists(_v1PlaylistPath)) {
        entry = "";
        string bootstrap = _localStreamName + HDS_PATH_SEPARATOR + _bootStrapName;
        if (!_isRolling) {
            // Add DVR info if appending type
            entry += format("<dvrInfo id=\"%s\" beginOffset=\"0\" endOffset=\"0\"></dvrInfo>",
                    STR(_localStreamName));
            entry += _newLine;
        }
        entry += format("<bootstrapInfo id=\"%s\" url=\"%s\" profile=\"named\"></bootstrapInfo>",
                STR(_localStreamName), STR(bootstrap));
        entry += _newLine;
        entry += format("<media url=\"%s\" bitrate=\"%"PRIu32"\" streamId=\"%s\" "
                "bootstrapInfoId=\"%s\" dvrInfoId=\"%s\"><metadata>%s</metadata></media>",
                STR(_v1Url), _bandwidth, STR(_localStreamName),
                STR(_localStreamName), STR(_localStreamName), STR(CreateMetaData()));

        // Use local stream name as ID for v1 entries
        if (!AppendV1Entry(_v1PlaylistPath, entry, _localStreamName)) {
            return false;
        }
    }

    return true;
}

bool HDSManifest::AppendMediaElement(string manifestPath, string media, string id) {
    // Read the contents of the manifest file
    string manifestString;
    if (!ReadManifest(manifestPath, manifestString)) return false;

    // Check first if we have a <media> element with the same ID, remove it
    RemoveMediaElement(manifestString, id);

    // Look for </manifest>, replace it and then append <media> element
    replace(manifestString, "</manifest>", media);
    manifestString += ("</manifest>" + _newLine);

    //FINEST("Updating Manifest...");
    return SaveManifest(manifestPath, manifestString);
}

bool HDSManifest::RemoveMediaElement(string &manifestContent, string id) {
    // Find the media element
    string mediaElement = "<media " + id + " ";
    //DEBUG("Searching \"%s\" in manifestContent", STR(id));
    //DEBUG("manifestContent: %s", STR(manifestContent));
    string::size_type pos1 = manifestContent.find(mediaElement);
    if (pos1 == string::npos) return false;

    // Find the media end tag
    string mediaEndTag = "</media>";
    string::size_type pos2 = manifestContent.find(mediaEndTag, pos1 + mediaElement.length());
    if (pos2 == string::npos) return false;

    // Remove that media element starting from '<media xxxxx>' to '</media>'
    pos2 += mediaEndTag.length();
    if (manifestContent[pos2] != '<') pos2 += 1; // include whatever is between > and <
    manifestContent.erase(pos1, (pos2 - pos1));

    return true;
}

bool HDSManifest::AppendV1Entry(string manifestPath, string entry, string entryId) {
    // Read the contents of the manifest file
    string manifestString;
    if (!ReadManifest(manifestPath, manifestString)) return false;

    // Check first if we have an entry with the same ID, remove it
    RemoveV1Entry(manifestString, entryId);

    // Look for </manifest>, replace it and then append <media> element
    replace(manifestString, "</manifest>", entry);
    manifestString += ("</manifest>" + _newLine);

    // Replace the stream type based on playlist type
    string newStreamType = "<streamType>liveOrRecorded</streamType>";

    if (_isRolling) {
        newStreamType = "<streamType>live</streamType>";
    }

    replace(manifestString, "<streamType>recorded</streamType>", newStreamType);

    //FINEST("Updating Manifest...");
    return SaveManifest(manifestPath, manifestString);
}

bool HDSManifest::RemoveV1Entry(string &manifestContent, string entryId) {
    // Find the start tag which is bootstrapInfo
    string v1EntryStart = "<bootstrapInfo id=\"" + entryId + "\" ";
    //DEBUG("Searching \"%s\" in manifestContent", STR(entryId));
    //DEBUG("manifestContent: %s", STR(manifestContent));
    string::size_type pos1 = manifestContent.find(v1EntryStart);
    if (pos1 == string::npos) return false;

    // Find the end tag which is a media end tag
    string v1EntryEnd = "</media>";
    string::size_type pos2 = manifestContent.find(v1EntryEnd, pos1 + v1EntryStart.length());
    if (pos2 == string::npos) return false;

    // Remove that media element starting from '<media xxxxx>' to '</media>'
    pos2 += v1EntryEnd.length();
    if (manifestContent[pos2] != '<') pos2 += 1; // include whatever is between > and <
    manifestContent.erase(pos1, (pos2 - pos1));

    return true;
}

bool HDSManifest::RemoveMediaAndSave(string manifestPath, string id) {
    // Read Manifest
    string manifestString;
    if (!ReadManifest(manifestPath, manifestString)) return false;

    RemoveMediaElement(manifestString, id);

    //FINEST("Updating Manifest...");
    if (!SaveManifest(manifestPath, manifestString)) return false;

    return true;
}

bool HDSManifest::RemoveMediaAndSave() {
    // Check first if we have a master playlist
    if (!_createMasterPlaylist || !fileExists(_masterPlaylistPath)) {
        return true;
    }

    string id;

    // Remove media for v2 manifest
    id = "href=\"" + _manifestRelativePath + "\"";
    if (!RemoveMediaAndSave(_masterPlaylistPath, id)) return false;

    // Remove media for v1 manifest
    if (fileExists(_v1PlaylistPath)) {
        string manifestString;
        if (!ReadManifest(_v1PlaylistPath, manifestString)) return false;
        RemoveV1Entry(manifestString, _localStreamName);
        if (!SaveManifest(_v1PlaylistPath, manifestString)) return false;
    }

    return true;
}

void HDSManifest::RemoveMediaAndSave(Variant &settings) {
    string masterPlaylistPath = (string) settings["groupTargetFolder"] + "manifest.f4m";

    // Check first if we have a master playlist
    if (!(bool)settings["createMasterPlaylist"] || !fileExists(masterPlaylistPath)) {
        return;
    }

    // Variables to be used
    string localStreamName = (string) settings["localStreamName"];
    string manifestName = (string) settings["manifestName"];
    if (manifestName == "") {
        // Use local stream name if manifest name is empty
        manifestName = localStreamName + ".f4m";
    }
    string v1PlaylistPath = (string) settings["groupTargetFolder"] + "manifest_v1.f4m";
    string manifestRelativePath = localStreamName + HDS_PATH_SEPARATOR + manifestName;

    string id;

    // Remove media for v2 manifest
    id = "href=\"" + manifestRelativePath + "\"";
    RemoveMediaAndSave(masterPlaylistPath, id);

    // Remove media for v1 manifest
    if (fileExists(v1PlaylistPath)) {
        string manifestString;
        ReadManifest(v1PlaylistPath, manifestString);
        RemoveV1Entry(manifestString, localStreamName);
        SaveManifest(v1PlaylistPath, manifestString);
    }
}

bool HDSManifest::AdjustBitRate() {
    if ((_bandwidth != 0) || (_pStreamCapabs == NULL)) return false;

    // Check the bandwidth
    uint32_t bw = (uint32_t) _pStreamCapabs->GetTransferRate();
    if (bw == 0) return false;

    // Adjust the stream level manifest
    string mediaElementOld = format("<media bitrate=\"%"PRIu32"\"", _bandwidth);
    string mediaElementNew = format("<media bitrate=\"%"PRIu32"\"", bw);
    ReplaceContent(_manifestAbsolutePath, mediaElementOld, mediaElementNew, "");

    if (_createMasterPlaylist && fileExists(_masterPlaylistPath)) {
        // Adjust the set level manifest
        mediaElementOld = format("<media href=\"%s\" bitrate=\"%"PRIu32"\">",
                STR(_manifestRelativePath), _bandwidth);
        mediaElementNew = format("<media href=\"%s\" bitrate=\"%"PRIu32"\">",
                STR(_manifestRelativePath), bw);
        ReplaceContent(_masterPlaylistPath, mediaElementOld, mediaElementNew, "");

        // Adjust the v1 manifest
        if (fileExists(_v1PlaylistPath)) {
            mediaElementOld = format("<media url=\"%s\" bitrate=\"%"PRIu32"\"",
                    STR(_v1Url), _bandwidth);
            mediaElementNew = format("<media url=\"%s\" bitrate=\"%"PRIu32"\"",
                    STR(_v1Url), bw);
            ReplaceContent(_v1PlaylistPath, mediaElementOld, mediaElementNew, "");
        }
    }

    _bandwidth = bw;

    //DEBUG("Bitrate: %"PRIu32, _bandwidth);

    return true;
}

bool HDSManifest::ConvertToRecorded() {
    // Read manifest file
    string manifestString;
    if (!ReadManifest(_manifestAbsolutePath, manifestString)) return false;

    // Try looking for this
    string streamType = "<streamType>liveOrRecorded</streamType>";
    string::size_type pos = manifestString.find(streamType);
    if (pos == string::npos) {
        // not found, streamtype is live
        streamType = "<streamType>live</streamType>";
    }

    // Replace the stream type
    replace(manifestString, streamType, "<streamType>recorded</streamType>");

    //FINEST("Updating Manifest...");
    if (!SaveManifest(_manifestAbsolutePath, manifestString)) return false;

    if (_createMasterPlaylist && fileExists(_v1PlaylistPath)) {
        // For v1, read the v1 manifest, convert to recorded, remove the DVR info then save it
        if (!ReadManifest(_v1PlaylistPath, manifestString)) return false;

        streamType = "<streamType>liveOrRecorded</streamType>";
        pos = manifestString.find(streamType);
        if (pos == string::npos) {
            // not found, streamtype is live
            streamType = "<streamType>live</streamType>";
        }

        // Replace the stream type
        replace(manifestString, streamType, "<streamType>recorded</streamType>");

        string dvrInfo = format("<dvrInfo id=\"%s\" beginOffset=\"0\" endOffset=\"0\"></dvrInfo>",
                STR(_localStreamName));
        dvrInfo += _newLine;

        replace(manifestString, dvrInfo, "");

        if (!SaveManifest(_v1PlaylistPath, manifestString)) return false;
    }

    return true;
}

bool HDSManifest::CreateMasterPlaylist() {
    // Add the initial stream
    AddElement("media", "");
    AddAttribute("href", _manifestRelativePath);
    AddAttribute("bitrate", _bandwidth);

    // Current master playlist is v2
    _nameSpace = "http://ns.adobe.com/f4m/2.0";
    return Create(_masterPlaylistPath);
}

bool HDSManifest::CreatePlaylistV1() {
    // Reset the contents of the manifest, for now settle with this...
    Reset();

    AddElement("id", _baseName);

    // Add the dvrInfo element if it's appending type
    if (!_isRolling) {
        AddElement("streamType", "liveOrRecorded");
        AddElement("dvrInfo", "");
        AddAttribute("id", _localStreamName);
        AddAttribute("beginOffset", "0");
        AddAttribute("endOffset", "0");
    } else {
        AddElement("streamType", "live");
    }

    if (_bootStrapName.empty()) {
        FATAL("Empty bootstrap name not supported!");
        return false;
        //TODO get the base64 encoded value of the bootstrap
        //		string bootStrapInfoB64 = "base64 encoded bootstrap goes here...";
        //
        //		AddElement("bootstrapInfo", bootStrapInfoB64);
    }

    AddElement("bootstrapInfo", "");
    // Add these attributes within bootstrapInfo
    AddAttribute("id", _localStreamName);
    AddAttribute("url", _localStreamName + HDS_PATH_SEPARATOR + _bootStrapName);
    AddAttribute("profile", "named"); //TODO modify "named" to get value from user

    AddElement("media", "");
    AddAttribute("url", _v1Url);
    AddAttribute("bitrate", _bandwidth);
    AddAttribute("streamId", _localStreamName);
    AddAttribute("bootstrapInfoId", _localStreamName);
    AddAttribute("dvrInfoId", _localStreamName);
    AddChildElement("metadata", CreateMetaData());

    return Create(_v1PlaylistPath);
}

bool HDSManifest::ReadManifest(string path, string &content) {
    File file;

    // Open the manifest file
    if (!file.Initialize(path, FILE_OPEN_MODE_READ)) {
        FATAL("Unable to initialize manifest file for reading!");
        return false;
    }

    // Read the contents of the existing manifest file
    file.ReadAll(content);
    file.Close();

    return true;
}

bool HDSManifest::SaveManifest(string path, string content) {
    File file;

    // Open the manifest file for writing
    if (!file.Initialize(path, FILE_OPEN_MODE_TRUNCATE)) {
        FATAL("Unable to initialize manifest file for update!");
        return false;
    }

    // Write the passed contents
    if (!file.WriteString(content)) {
        FATAL("Unable to update manifest file!");
        return false;
    }

    file.Close();

    return true;
}

string HDSManifest::GetMasterManifestPath() {
    return _masterPlaylistPath;
}

string HDSManifest::GetManifestPath() {
    return _manifestAbsolutePath;
}

#endif /* HAS_PROTOCOL_HDS */
