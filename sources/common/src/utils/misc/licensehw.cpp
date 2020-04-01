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


#ifdef HAS_LICENSE
#include "utils/misc/licensehw.h"

//string LicenseHW::MAC_ADDR = "MAC";
//string LicenseHW::AMI_ID = "AMIID";

size_t LicenseHW::GetHWMarkers(vector<string> & markers) {
	// just make sure markers is empty
	markers.clear();

	// For now, this function only gets and returns MAC addresses.
	if (getMACAddresses(markers) > 0)
		return markers.size();
	else
		return 0;
}

uint64_t LicenseHW::GetAvailableMemory() {
	return getTotalSystemMemory();
}

string LicenseHW::GetGID() {
	return getGID();
}

string LicenseHW::GetKernelVersion() {
	return getKernelVersion();
}

string LicenseHW::GetUID() {
	return getUID();
}

bool LicenseHW::IsAMI(Variant &licenseTerms) {
	return licenseTerms.HasKey("AMI_ID", false);
}

string LicenseHW::GetAMIMarker() {
	string productCodes;
	GetProductCodes(productCodes);
	return productCodes;
}

bool LicenseHW::GetProductCodes(string &productCodes) {
    struct sockaddr_in servAddr;
    SOCKET sockId;
    uint32_t const msgSize = 1*1024;
    char message[msgSize] = {0};
    int32_t msgLen;
    string marker = "\r\n\r\n";	
    string request = "GET /latest/meta-data/product-codes HTTP/1.1";
	if((sockId = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return false;
    
    memset(&servAddr, 0, sizeof(servAddr));

    //connect to EC2 meta-data cloud server
    servAddr.sin_addr.s_addr = inet_addr(STR("169.254.169.254"));
    servAddr.sin_port = htons(80);
    servAddr.sin_family = AF_INET;

    if(connect(sockId, (struct sockaddr *)&servAddr, sizeof(servAddr)) != 0) {
        CLOSE_SOCKET(sockId);
        return false;
    }

    string req = request + marker;
    int32_t written = WRITE_FD((int32_t)sockId, STR(req), (uint32_t)req.size());
	if (written > 0) {
		msgLen = READ_FD((int32_t)sockId, message, msgSize);
		if (msgLen > 0) {
	    	string contents = message;
	    	size_t startPoint = contents.find(marker);
            if (startPoint != string::npos) 
	        	productCodes = contents.substr(startPoint + marker.size());
		}
    }
    CLOSE_SOCKET(sockId);
    return (!productCodes.empty());
}


#endif /* HAS_LICENSE */
