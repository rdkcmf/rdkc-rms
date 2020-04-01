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


#ifdef HAS_MEDIA_LST

#include "mediaformats/readers/lst/lstdocument.h"
#include "application/clientapplicationmanager.h"
#include "protocols/external/rmsprotocol.h"

LSTDocument::LSTDocument(Metadata &metadata)
: BaseMediaDocument(metadata) {
}

LSTDocument::~LSTDocument() {
}

bool LSTDocument::ParseDocument() {
	//1. Go to the beginning document
	if (!_mediaFile.SeekBegin()) {
		FATAL("Unable to seek in file");
		return false;
	}

	//2. Take no more than 4K worth of data. That is more than enough to
	//store all the info about the stream
	if (_mediaFile.Size() > _metadata.storage().maxPlaylistFileSize()) {
		FATAL("Invalid *.lst file size detected (%"PRIu32">%"PRIu32")",
				(uint32_t) _mediaFile.Size(),
				_metadata.storage().maxPlaylistFileSize());
		return false;
	}
	IOBuffer temp;
	temp.ReadFromRepeat(0, _metadata.storage().maxPlaylistFileSize());
	if (!_mediaFile.ReadBuffer(GETIBPOINTER(temp), _mediaFile.Size())) {
		FATAL("Unable to read the file content");
		return false;
	}
	string content((char *) GETIBPOINTER(temp), (string::size_type)_mediaFile.Size());

	//3. Split the content into kvp and cerate the variant
	replace(content, "\r\n", "\n");

	vector<string> lines;
	split(content, "\n", lines);
	string::size_type pos1;
	string::size_type pos2;
	string startStr;
	string lengthStr;
	string name;
	Variant elements;
	for (uint32_t i = 0; i < lines.size(); i++) {
		string &line = lines[i];
		trim(line);
		if ((line.size() == 0) || (line[0] == '#'))
			continue;
		//FINEST("Line: `%s`", STR(line));
		if (((pos1 = line.find(",")) == string::npos)
				|| (pos1 == 0)
				|| ((pos2 = line.find(",", pos1 + 1)) == string::npos)
				|| (pos2 == (line.size() - 1))
				) {
			WARN("Invalid playlist line in file %s: %s", STR(_mediaFilePath), STR(line));
			continue;
		}
		startStr = line.substr(0, pos1);
		lengthStr = line.substr(pos1 + 1, pos2 - pos1 - 1);
		name = line.substr(pos2 + 1);
		trim(startStr);
		trim(lengthStr);
		trim(name);

		const char *pStartStr = startStr.c_str();
		const char *pLengthStr = lengthStr.c_str();
		char *pErr = NULL;

		double start = strtod(pStartStr, &pErr);
		if (pErr == pStartStr) {
			WARN("Invalid playlist line in file %s: %s", STR(_mediaFilePath), STR(line));
			continue;
		}

		double length = strtod(pLengthStr, &pErr);
		if (pErr == pLengthStr) {
			WARN("Invalid playlist line in file %s: %s", STR(_mediaFilePath), STR(line));
			continue;
		}

		if (name == "") {
			WARN("Invalid playlist line in file %s: %s", STR(_mediaFilePath), STR(line));
			continue;
		}

		if ((start < 0)&&(start != -2)&&(start != -1))
			start = -2;

		if ((length < 0)&&(length != -1)&&(length != -2))
			length = -2;

		start *= 1000;
		length *= 1000;

		Variant element;
		element["start"] = start;
		element["length"] = length;
		element["name"] = name;
		elements.PushToArray(element);

		//FINEST("\n%s", STR(element.ToString()));
		//FINEST("%.2f; %.2f `%s`", start, length, STR(name));
	}

	if (elements.MapSize() == 0) {
		FATAL("No valid elements found in file %s", STR(_mediaFilePath));
		return false;
	}
	_metadata.playlistElements(elements);
	return true;
}

bool LSTDocument::BuildFrames() {
	//1. Insert 3 dummy frames to keep the seek file happy
	MediaFrame mf;
	memset(&mf, 0, sizeof (mf));
	_frames.push_back(mf);
	_frames.push_back(mf);
	_frames.push_back(mf);

	return true;
}

Variant LSTDocument::GetPublicMeta() {
	Variant result;
	result["playlistElements"] = _metadata.playlistElements();
	return result;
}

#endif /* HAS_MEDIA_LST */
