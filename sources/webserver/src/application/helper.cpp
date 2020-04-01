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
#include "common.h"
#include "application/helper.h"

using namespace webserver;

string Helper::ConvertToLocalSystemTime(time_t const& theTime) {
#pragma warning ( disable : 4996 )	// allow use of localtime
	struct tm tim = *(localtime(&theTime));
	char s[30];
	strftime(s, 30, "%Y-%m-%dT%H-%M-%S", &tim);
	return (string)s;
}

#ifdef WIN32
string Helper::GetProgramDirectory() {
	char buff[MAX_PATH + 1];
	char *lb;
	DWORD nsize = sizeof(buff)/sizeof(char);
	DWORD n = GetModuleFileName(NULL, buff, nsize);
	if (n == 0 || n == nsize || (lb = strrchr(buff, '\\')) == NULL)
		return "";
	else
		*lb = '\0';
	return buff;
}
#endif

string Helper::NormalizeFolder(string const &folder) {
#ifdef WIN32
	string progDir = GetProgramDirectory();
	return progDir + "/" + folder;
#else
	string progDir = normalizePath(".", "");
	return progDir + folder;
#endif
}

string Helper::GetTemporaryDirectory() {
#ifdef WIN32
	TCHAR lpTempPathBuffer[MAX_PATH];
	DWORD dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer); // buffer for path 
	if (!(dwRetVal > MAX_PATH || (dwRetVal == 0)))
		return string(lpTempPathBuffer);
#else
	return "/tmp";
#endif
	return "";
}

struct sockaddr *Helper::GetNetworkAddress(struct sockaddr *addr, uint16_t port, string ipAddress, bool ipv6) {
	if (ipv6) {
		struct sockaddr_in6 *addrV6 = (sockaddr_in6 *)addr;
		addrV6->sin6_family = AF_INET6;
		addrV6->sin6_port = htons(port);
		inet_pton(AF_INET6, STR(ipAddress), &addrV6->sin6_addr);
	} else {
		struct sockaddr_in *addrV4 = (sockaddr_in *)addr;
		addrV4->sin_family = AF_INET;
		addrV4->sin_port = htons(port);
		inet_pton(AF_INET, STR(ipAddress), &addrV4->sin_addr);
	}
	return addr;
}

string Helper::GetIPAddress(struct sockaddr const *addr, uint16_t &port, bool ipv6) {
	if (ipv6) {
		char ipv6Address[INET6_ADDRSTRLEN];
		if (NULL == inet_ntop(AF_INET6, &((sockaddr_in6 *) addr)->sin6_addr, 
			ipv6Address, INET6_ADDRSTRLEN)) {
			return "";	
		}
		port = ((sockaddr_in6 *) addr)->sin6_port;
		return ipv6Address;
	} else {
		char ipv4Address[INET_ADDRSTRLEN];
		if (NULL == inet_ntop(AF_INET, &((sockaddr_in *) addr)->sin_addr, 
			ipv4Address, INET_ADDRSTRLEN)) {
			return "";
		}
		port = ((sockaddr_in *) addr)->sin_port;
		return ipv4Address;
	}
}

void Helper::SplitFileName(string const& filePath, string &folder, string &name, string &extension) {
	if (filePath.empty())
		return;
	//get the last folder
	size_t pos = filePath.find_last_of("\\/");
	if (string::npos != pos) {
		if (filePath.size() == pos - 1) {	//we only have a folder, e.g., ./test1/test2/
			folder = filePath;
		} else {
			folder = filePath.substr(0, pos);
			string nameext = filePath.substr(pos + 1);
			pos = nameext.find_last_of(".");
			if (string::npos == pos) { //not found
				name = nameext;
				return;
			} else {
				name = nameext.substr(0, pos);
				extension = nameext.substr(pos + 1);
			}
		}
	} else {	//we only have a file name
		pos = filePath.find_last_of(".");
		if (string::npos == pos) { //not found
			name = filePath;
			return;
		} else {
			name = filePath.substr(0, pos);
			extension = filePath.substr(pos + 1);
		}
	}
}

//split using 2 or more delimiters, separators string will have the string containing these delimiters
void Helper::Split(string const &str, string const &separators, vector<string> &result) {
	result.clear();
	size_t position = str.find_first_of(separators);
	size_t lastPosition = 0;

	while (position != string::npos) {
		if (position - lastPosition > 0)  
			ADD_VECTOR_END(result, str.substr(lastPosition, position - lastPosition));
		lastPosition = position + 1;
		position = str.find_first_of(separators, lastPosition);
	}
	ADD_VECTOR_END(result, str.substr(lastPosition, string::npos));
}

void Helper::ListFiles(string const &path, vector<string> &files) {
	vector<string> fileList;
	listFolder(path, fileList, true, true, false);
	if (fileList.empty()) {
		return;
	}
	FOR_VECTOR_ITERATOR(string, fileList, iterFiles) {
		string file = VECTOR_VAL(iterFiles);
		files.push_back(file);
#ifdef WIN32
		struct _stat64 buf;
#else
		struct stat64 buf;
#endif
		if (GetFileInfo(file, buf) && (buf.st_mode & S_IFDIR)) {
			file += "/";
			ListFiles(file, files);
		}
	}
}

bool Helper::GetFileInfo(string const &filePath,
#ifdef WIN32
struct _stat64 &buf
#else
struct stat64 &buf
#endif
	) {
#ifdef WIN32
		memset(&buf, 0, sizeof (struct _stat64));
		return (0 == _stat64(STR(filePath), &buf));
#else
		memset(&buf, 0, sizeof (struct stat64));
		return (0 == stat64(STR(filePath), &buf));
#endif
}
