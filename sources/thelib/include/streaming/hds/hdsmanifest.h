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
#ifndef _HDSMANIFEST_H
#define	_HDSMANIFEST_H

#include "common.h"
#include "streaming/streamcapabilities.h"
#include "streaming/xmlelements.h"

// Since the manifest uses URL format, the separator should be a consistent 'slash'
#define HDS_PATH_SEPARATOR '/'

/**
 * HDSManifest class is responsible for writing the manifest file that is
 * compliant with the Flash Media Manifest File Format Specification. It is
 * directly controlled by OutFileHDSStream class.
 *
 * See FlashMediaManifestFileFormatSpecification.pdf
 */
class DLLEXP HDSManifest {
public:
	HDSManifest();
	virtual ~HDSManifest();

	/**
	 * Initializes the Manifest class
	 *
	 * @param settings Contains the generic
	 * @param pStreamCapabs Describes the input stream capabilities
	 * @return true on success, false otherwise
	 */
	bool Init(Variant &settings, StreamCapabilities *pStreamCapabs);

	/**
	 * Adds an XML element to the current instance of the manifest
	 *
	 * @param elementName Name of the element to be added
	 * @param elementValue Value contained by the element
	 */
	void AddElement(string elementName, string elementValue);

	/**
	 * Adds an XML element but has special consideration for elements that should
	 * appear only once on manifest (TODO: include samples here)
	 *
	 * @param elementName Name of the element to be added
	 * @param elementValue Value contained by the element
	 * @param isUnique Flag to set indicating that this element is unique
	 */
	void AddElement(string elementName, string elementValue, bool isUnique);

	/**
	 * Add attributes to the just created element
	 *
	 * @param elementName Name of the element to be added
	 * @param attrName Name of the elements attribute to be added
	 * @param attrValue String Value of the attribute
	 * @return true on success, false otherwise
	 */
	bool AddAttribute(string attrName, string attrValue);

	/**
	 * Add attributes to the just created element
	 *
	 * @param elementName Name of the element to be added
	 * @param attrName Name of the elements attribute to be added
	 * @param attrValue Integer Value of the attribute
	 * @return true on success, false otherwise
	 */
	bool AddAttribute(string attrName, uint32_t attrValue);

	/**
	 * Modifies the default XML name space (http://ns.adobe.com/f4m/1.0)
	 * @param nameSpace
	 */
	void SetNameSpace(string nameSpace);

	/**
	 * Add a child element to the just recently added element. At this point,
	 * only a second level of child is supported. This code need to be modified
	 * if in the future it is necessary to support adding a child element
	 * that is already a child. It is also assumed that the child element to
	 * be added does not have any attribute (refer to moov, metadata and xmpMetadata).
	 *
	 * @param elementName Name of the element to be added
	 * @param elementValue Value contained by the element
	 * @return true on success, false otherwise
	 */
	bool AddChildElement(string elementName, string elementValue);

	/**
	 * Generates the metadata to be used for metadata element within a media
	 * parent element
	 *
	 * @return Base64 encoded string
	 */
	string CreateMetaData();

	/**
	 * Creates the actual Manifest file based on the current state of the
	 * Manifest class
	 *
	 * @param path Path to the manifest file to be created
	 * @return true on success, false otherwise
	 */
	bool Create(string path);

	/**
	 * Forms a working manifest file with the bare necessities and has an
	 * external bootstrap information file (bootstrap).
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
	 * Used for testing the manifest. Remove later on.
	 *
	 * @return true on success, false otherwise
	 */
	bool CreateTestManifest();

	/**
	 * Sets the filename of the bootstrap if it is external. Otherwise, this
	 * should not be used at all if bootstrap info is to be embedded within
	 * the manifest file.
	 *
	 * @param bootStrap
	 */
	void SetBootStrapName(string bootStrap);

	/**
	 * Replaces a string inside an existing manifest file. Note that the
	 * parameter 'replaceWith' should NOT contain any instance of the
	 * string parameter 'replaceWhat'. It should be included on the 'marker'
	 * instead. Any changes are then saved to the manifest file.
	 *
	 * @param path Path to the manifest file to be modified
	 * @param replaceWhat String to be replaced
	 * @param replaceWith String to replace the target
	 * @param marker String marker, normally used to append a string
	 * @return true on success, false otherwise
	 */
	bool ReplaceContent(string path, string replaceWhat, string replaceWith, string marker);

	/**
	 * Removes a media element from the manifest file and then saves the modified
	 * manifest file
	 * 
	 * @return true on success, false otherwise
	 */
	bool RemoveMediaAndSave();

	/**
	 * Removes a media element from the manifest files and then saves the modified
	 * manifest files. Primarily used to call from an external class (specifically
	 * originapplication)
	 * 
	 * @param settings Settings for this stream
	 */
	static void RemoveMediaAndSave(Variant &settings);

	/**
	 * Adjusts the bitrate of this stream to be reflected on the manifest file(s)
	 * 
	 * @return true on when adjusted, false otherwise
	 */
	bool AdjustBitRate();

	/**
	 * Returns the master playlist path (set level manifest)
	 */
	string GetMasterManifestPath();

	/**
	 * Returns child playlist path (stream level manifest)
	 */
	string GetManifestPath();

protected:
	// Header of the xml file. Normally includes the version and encoding
	string _xmlHeader;

	// Main XML container tag. In this case, "manifest"
	string _xmlParentTag;

private:

	/**
	 * Before adding elements on the manifest file, it is checked if the
	 * element to be added was tagged as 'unique'. If this is the case, if
	 * an element already exists on the list, its values would be overridden.
	 *
	 * @param element Element to be added on the list
	 */
	void AddElementToList(XmlElements &element);

	/**
	 * Returns the creation time of this manifest file
	 *
	 * @return String containing the timestamp of the created file
	 */
	string GetCreationTime();

	/**
	 * Appends a media element at the end of the manifest files both version 2
	 * and version 1, before the manifest end tag.
	 *
	 * @return true on success, false otherwise
	 */
	bool AppendMedia();

	/**
	 * Appends media element at the end of a specific manifest file.
	 * 
	 * @param manifestPath Path of the manifest file
	 * @param media Media element complete with closing tag to be added
	 * @param id Identifier for the stream to be added
	 * 
	 * @return true on success, false otherwise
	 */
	bool AppendMediaElement(string manifestPath, string media, string id);

	/**
	 * Removes a media element from a specific manifest file and then saves it.
	 * 
	 * @param manifestPath Path of the manifest file
	 * @param id Identifier for the stream to be removed
	 * 
	 * @return true on success, false otherwise
	 */
	static bool RemoveMediaAndSave(string manifestPath, string id);

	/**
	 * Removes media element from the manifest using an id (for v2, this is 'href=""'
	 * for v1, this is 'url=""').
	 * 
	 * @param manifestContent Actual manifest file content
	 * @param id Identifier for the stream to be removed
	 * 
	 * @return true on success, false otherwise
	 */
	static bool RemoveMediaElement(string &manifestContent, string id);

	/**
	 * Appends additional entry to the target manifest file.
	 * 
	 * @param manifestPath Path of the manifest file
	 * @param entry Entry complete with elements closing tag to be added
	 * @param entryId Identifier used for removing any duplicates
	 * 
	 * @return true on success, false otherwise
	 */
	bool AppendV1Entry(string manifestPath, string entry, string entryId);

	/**
	 * Removes a bootstrapInfo element from a manifest represented by a string.
	 * 
     * @param manifestContent Actual manifest file content
     * @param entryId Identifier used for removing this entry
	 * 
     * @return true on success, false otherwise
     */
	static bool RemoveV1Entry(string &manifestContent, string entryId);

	/**
	 * Converts live stream type entry of HDS manifest file to recorded type
	 *
	 * @return true on success, false otherwise
	 */
	bool ConvertToRecorded();

	/**
	 * Creates a set level manifest file (master playlist)
	 * 
	 * @return true on success, false otherwise
	 */
	bool CreateMasterPlaylist();

	/**
	 * Creates the version 1 compatible manifest file.
	 * 
	 * @return true on success, false otherwise
	 */
	bool CreatePlaylistV1();

	/**
	 * Reads the content of the manifest file and stores it to the passed parameter
	 * 
	 * @param path Path to the manifest file to be read
	 * @param content Actual manifest file content that was read
	 * @return true on success, false otherwise
	 */
	static bool ReadManifest(string path, string &content);

	/**
	 * Saves the content to a manifest file
	 * 
	 * @param path Path to the manifest file to be stored
	 * @param content Actual content to be stored
	 * @return true on success, false otherwise
	 */
	static bool SaveManifest(string path, string content);

	// Target folder of the manifest file
	string _targetFolder;

	// Indicates the manifest can be be overwritten or not
	bool _overwriteDestination;

	// Name of the stream where this HDS stream is pulling from
	string _localStreamName;

	// Name of the fragment, this is the value of chunkBaseName
	string _baseName;

	// Bit rate of the source stream
	uint32_t _bandwidth;

	// String representation of the absolute/relative path and file name of the manifest
	string _manifestAbsolutePath;
	string _manifestRelativePath;

	string _nameSpaceDefault;
	string _nameSpace;

	// Actual container of the manifest file
	string _xmlDoc;
	vector<XmlElements > _xmlElements;

	// Necessary for creating the metadata for the manifest
	StreamCapabilities *_pStreamCapabs;

	const string _newLine;

	// External bootstrap filename
	string _bootStrapName;
	// bootstrap ID
	string _bootStrapId;

	bool _updateMasterPlaylist;
	bool _cleanupDestination;
	bool _createMasterPlaylist;

	string _masterPlaylistPath;

	// v1 manifest file
	string _v1PlaylistPath;
	// v1 url path
	string _v1Url;
	
	bool _isRolling;
};

#endif	/* _HDSMANIFEST_H */
#endif	/* HAS_PROTOCOL_HDS */
