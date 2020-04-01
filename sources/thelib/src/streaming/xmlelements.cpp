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


#include "streaming/xmlelements.h"

XmlElements::XmlElements()
: _isUnique(false) {
}

XmlElements::~XmlElements() {
	FOR_MULTIMAP(_childElements, string, XmlElements*, i) {
		if (MAP_VAL(i) != NULL) {
			delete MAP_VAL(i);
		}
	}
    _attributes.clear();
}

string XmlElements::GenerateAttrString() {
    // Check if we have indeed some attributes
    if (_attributes.empty()) {
        // No attributes here return empty string
        return "";
    }

    // Add all those attributes present
    string attrString = "";

    FOR_MAP(_attributes, string, string, i) {
        // Add the name of the attribute (e.g. id=)
        attrString += " " + MAP_KEY(i) + "=";
        // Add the value of the attribute (e.g. "stream1")
        attrString += "\"" + MAP_VAL(i) + "\"";
    }

    // return the attributes to the caller
    return attrString;
}

string XmlElements::GetAttribute(string const& attributeName) const {
    if (_attributes.empty()) {
        FATAL("this element has no attributes");
        return "";
    }
    map<string, string>::const_iterator attrIter;
    attrIter = _attributes.find(attributeName);
    if (attrIter == _attributes.end()) {
        FATAL("attribute not found");
        return "";
    } else {
        return attrIter->second;
    }
}
