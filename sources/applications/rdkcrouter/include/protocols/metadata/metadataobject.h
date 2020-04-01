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
/**
 * metadataobject.h - the base MetaDataObject
 * a MetadataObject encapsulates a "hunk" of Metadata
 * as either or string or a variant.
 * Future derivatives may be aware of the semantics of the Metadata
 * but this base class gleefully ignores the contents
 */
#ifndef __METADATAOBJECT_H__
#define __METADATAOBJECT_H__

/// MetadataObject - just a string really
//
class MetadataObject {
public:
	MetadataObject() {m_strObj = "";};	// init to null string so it doesn't throw
	MetadataObject(string obj) {setString(obj);};

	void setString(string obj) {m_strObj = obj;};

	const char * getCharPtr() {return STR(m_strObj);}

	string getString() {return m_strObj;};

	//$ToDo:  add variant support

protected:
	string m_strObj;

};



#endif // __METADATAOBJECT_H__
