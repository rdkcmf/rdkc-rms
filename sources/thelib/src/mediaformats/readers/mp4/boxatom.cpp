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



#ifdef HAS_MEDIA_MP4
#include "mediaformats/readers/mp4/boxatom.h"
#include "mediaformats/readers/mp4/mp4document.h"

BoxAtom::BoxAtom(MP4Document *pDocument, uint32_t type, uint64_t size, uint64_t start)
: BaseAtom(pDocument, type, size, start) {

}

BoxAtom::~BoxAtom() {
}

bool BoxAtom::Read() {
	while (CurrentPosition() != _start + _size) {
		BaseAtom *pAtom = GetDoc()->ReadAtom(this);
		if (pAtom == NULL) {
			FATAL("Unable to read atom. Parent atom is %s",
					STR(GetTypeString()));
			return false;
		}
		if (!pAtom->IsIgnored()) {
			if (!AtomCreated(pAtom)) {
				FATAL("Unable to signal AtomCreated for atom %s (%"PRIx64")",
						STR(GetTypeString()), _start);
				return false;
			}
		}
		ADD_VECTOR_END(_subAtoms, pAtom);
	}
	return true;
}

string BoxAtom::Hierarchy(uint32_t indent) {
	string result = string(indent * 4, ' ') + GetTypeString() + "\n";
	if (_subAtoms.size() == 0) {
		result += string((indent + 1) * 4, ' ') + "[empty]";
		return result;
	}
	for (uint32_t i = 0; i < _subAtoms.size(); i++) {
		result += _subAtoms[i]->Hierarchy(indent + 1);
		if (i != _subAtoms.size() - 1)
			result += "\n";
	}
	return result;
}

BaseAtom * BoxAtom::GetPath(uint32_t depth, ...) {
	vector<uint32_t> path;
	va_list arguments;
	va_start(arguments, depth);
	for (uint8_t i = 0; i < depth; i++) {
		uint32_t pathElement = va_arg(arguments, uint32_t);
		ADD_VECTOR_END(path, pathElement);
	}
	va_end(arguments);
	if (path.size() == 0)
		return NULL;
	return GetPath(path);
}

BaseAtom * BoxAtom::GetPath(vector<uint32_t> path) {
	if (path.size() == 0)
		return NULL;

	uint32_t search = path[0];
	path.erase(path.begin());
	for (uint32_t i = 0; i < _subAtoms.size(); i++) {
		if (_subAtoms[i]->GetTypeNumeric() == search) {
			if (path.size() == 0)
				return _subAtoms[i];
			return _subAtoms[i]->GetPath(path);
		}
	}

	return NULL;
}

#endif /* HAS_MEDIA_MP4 */
