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

#include "netio/netio.h"
#include "configuration/configfile.h"
#include "utils/misc/license.h"
#include "protocols/protocolmanager.h"
#include "application/clientapplicationmanager.h"
#include "protocols/protocolfactorymanager.h"
#include "protocols/defaultprotocolfactory.h"

#include "utils/misc/license.h"
#include "utils/misc/licensehw.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <openssl/conf.h>
#include <string>
#include <iostream>
#include <fstream>
#ifdef WIN32
#include <windows.h>
#endif /* WIN32 */

bool AutoBindLicense();
string GetMacAddresses();
bool AutoDelete (string applicationFile);
#ifdef WIN32
bool SelfDelete();
#endif /* WIN32 */

int main(int, char **) {
	if (AutoBindLicense()) {
		bool isdeleted = false;
		isdeleted = AutoDelete("autobindlicense");
		if (isdeleted)
			return 0;
		else
			return 1;
	} else {
		return 1;
	}
}

bool AutoBindLicense() {
	string command;
	string adresse = GetMacAddresses();

#ifdef WIN32/* WIN32 */
	INFO("Windows platform");
	command = "licensecreator -c config/License.lic 3 0 false ";
	command = command + adresse.c_str();
	int result = 0;
	result = system(command.c_str());
	//delete licensecreator
	command = "DEL licensecreator.exe";
	result = system(command.c_str());
	if (result != 0)
		return false;
#else /* LINUX *//* OSX */
	INFO("Linux platform");
	command = "sudo ./licensecreator -c License.lic 3 0 false ";
	command = command + adresse.c_str();
	int result = 0;
	result = system(command.c_str());
	//delete licensecreator
	command = "sudo rm -f licensecreator";
	result = system(command.c_str());
	if (result != 0)
		return false;
#endif 

	return true;
}

string GetMacAddresses(){
	//map<string, vector<string> > MapVector; 
	vector<string> MacVector; 
#ifdef HAS_LICENSE
	LicenseHW::GetHWMarkers(MacVector);
#endif
	string address = "";
//	for( map<string, vector<string> >::iterator iter = MapVector.begin(); iter != MapVector.end(); ++iter ) {
//		vector<string> tempVec = (*iter).second;
//		if(tempVec.size() > 0)
//		{
//			adresse = tempVec[0];
//			break;
//		}	   
//	}
	
	if (MacVector.size() > 0) {
		address = MacVector[0];
	}
	
	return address;
}

bool AutoDelete (string applicationFile) {  

	#ifdef WIN32 /* WIN32 */
		if(!SelfDelete())
			return false;
	#else	/* LINUX */  /* OSX */
		string command;
		command = "sudo rm -f ";
		command = command + applicationFile;
		int result = 0;
		result = system(command.c_str());
		if(result != 0)
			return false;
	#endif   
  return true;
}

#ifdef WIN32
bool SelfDelete()
{
	TCHAR fileName[MAX_PATH];
	TCHAR command[MAX_PATH];

	// retrieve file name and path
	if((GetModuleFileName(0, fileName, MAX_PATH) != 0) &&
		(GetShortPathName(fileName, fileName, MAX_PATH) != 0)) {

		// Below lines ensure to run a single command to delete
		// `fileName` (full path and name of the file) and redirect the output to nowhere (NUL)
		lstrcpy(command,"/c del ");
		lstrcat(command,fileName);
		lstrcat(command," >> NUL");

		// Invoking the command shell to delete the file
		if((GetEnvironmentVariable("ComSpec", fileName, MAX_PATH) != 0) &&
	  	  ((INT)ShellExecute(0, 0, fileName, command, 0, SW_HIDE) > 32)) {
			return TRUE;
		}
        }
	else {

		return FALSE;
        }
}
#endif /* WIN32 */

