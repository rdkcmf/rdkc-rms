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


#ifndef _XMLELEMENTS_H
#define	_XMLELEMENTS_H

#ifdef ANDROID
#include "common.h"
#endif

#if defined ( RMS_PLATFORM_RPI )
#include "common.h"
#endif

/**
 * XmlElemets class is responsible for representing each xml element on the
 * manifest file. This class gets intantiated for each xml element/field and
 * its object is destroyed afterwards.
 */
class XmlElements {
public:

    template<typename T>
    XmlElements(string const& elementName, T const& elementValue)
    : _isUnique(false),
    _name(elementName) {
        ostringstream stringConverter;
        stringConverter.clear();
        stringConverter.str(std::string());
        stringConverter << elementValue;
        _value = stringConverter.str();
        _attributes.clear();
    }
    XmlElements();
    virtual ~XmlElements();

    /**
     * [Defunct] - retained for compatibility with MSS, HDS. Use AddChildElement instead.
     * Used when adding child elements to the parent
     *
     * @param value String that contains the child elements to be added
     */
    inline void AppendValue(string value) {
        _value += value;
    }

    /**
     * Returns the value of this element
     *
     * @return The contained value of this element
     */
    inline string& GetValue() {
        return _value;
    }

    inline string& GetName() {
        return _name;
    }

    inline bool GetIsUnique() {
        return _isUnique;
    }

    inline void SetIsUnique(bool value) {
        _isUnique = value;
    }

    string GetAttribute(string const& attributeName) const;

    template<typename T>
    void SetAttribute(string const& attributeName, T const& attributeValue) {
        if (_attributes.empty()) {
            FATAL("this element has no attributes");
            return;
        }
        map<string, string>::iterator attrIter;
        attrIter = _attributes.find(attributeName);
        if (attrIter == _attributes.end()) {
            FATAL("attribute not found");
            return;
        } else {
            ostringstream stringConverter;
            stringConverter.clear();
            stringConverter.str(std::string());
            stringConverter << attributeValue;
            attrIter->second = stringConverter.str();
        }
    }

    /**
     * Add a child element to the current element
     *
     * @param elementName Name of the element to be added
     * @param elementValue Value contained by the element
     * @param isUnique optional flag that indicates whether a child element is unique or not
     * @return true on success, false otherwise
     */
    template<typename T>
    bool AddChildElement(string const& elementName, T const& elementValue, bool isUnique = false) {
        bool matchFound = false;
        matchFound = (_childElements.find(elementName) != _childElements.end());

        if (matchFound && ((_childElements.find(elementName)->second->GetIsUnique()) || isUnique)) {
            return false;
        } else {
            XmlElements* childElement = new XmlElements(elementName, elementValue);
            _childElements.insert(pair<string, XmlElements*>(elementName, childElement));
            return true;
        }
    }

    inline bool AddChildElement(XmlElements* childElement, bool isUnique = false) {
        bool matchFound = false;
        string elementName = childElement->GetName();
        matchFound = (_childElements.find(elementName) != _childElements.end());

        if (matchFound && ((_childElements.find(elementName)->second->GetIsUnique()) || isUnique)) {
            return false;
        } else {
            _childElements.insert(pair<string, XmlElements*>(elementName, childElement));
            return true;
        }
    }

    /**
     * Inserts attributes on this element
     *
     * @param attrName Name of the attribute to be added
     * @param attrValue Value of the attribute
     */
    template<typename T>
    void AddAttribute(string const& attrName, T const& attrValue) {
        ostringstream stringConverter;
        stringConverter.clear();
        stringConverter.str(std::string());
        stringConverter << attrValue;
        _attributes[attrName] = stringConverter.str();
    }

    /**
     * Transform into string the attributes of this element
     *
     * @return String containing the list of attributes
     */
    string GenerateAttrString();

    inline multimap<string, XmlElements*>& GetChildElements() {
        return _childElements;
    }

private:

    // Flag that indicates if this element appears only once
    bool _isUnique;

    // Contained value of this element
    string _value;

    // Name of this element
    string _name;

    multimap<string, XmlElements*> _childElements;

    // Attributes of the element
    map<string, string> _attributes;
};

#endif	/* _XMLELEMENTS_H */
