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



/*  TODO:
 *
 * Figure out how to digitally sign the file (and check it)
 * Look at calls to get Amazon ID, and find out if we are an EC2 or not!
 * 		Maybe we can lock to an instance OR AMI
 *
 */
#ifdef HAS_LICENSE
#include "common.h"
#include "utils/misc/license.h"
#include "utils/misc/licensehw.h"
#include <bitset>
#include <openssl/conf.h>

#include <iostream>
#include <fstream>

#include <stdio.h>
#include <time.h>
#include <ctime>

#define LICENSE_CURRENT_VERSION 5
#define LICENSE_CURRENT_VERSION_STR std::string("5")

/*
 * This is our public key, it MUST have been generated from the private key
 * that is held in our LicenseSigning application!
 */
unsigned char gPublicKeyDer[] = {
  0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
  0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
  0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xb4, 0x23, 0xc5,
  0x2c, 0xe4, 0x15, 0x96, 0x80, 0x82, 0x7d, 0x28, 0x96, 0xf7, 0x7e, 0x2d,
  0x27, 0xe9, 0xc5, 0xba, 0xda, 0x35, 0x4f, 0x49, 0xea, 0xd5, 0x4d, 0xa8,
  0xd7, 0xd2, 0x33, 0xa2, 0x49, 0x3f, 0xdd, 0x26, 0xd2, 0x5c, 0xa3, 0xe4,
  0x30, 0x7a, 0xdb, 0xac, 0xb3, 0x32, 0x6c, 0xc5, 0x5e, 0xdf, 0x05, 0xa6,
  0x15, 0xbe, 0xbd, 0x91, 0xe3, 0xe0, 0xad, 0x6e, 0xca, 0x02, 0x43, 0xd0,
  0xa7, 0xee, 0xf0, 0x05, 0xa2, 0xad, 0x87, 0x00, 0x08, 0xc6, 0xe6, 0xbc,
  0xf4, 0xd7, 0x3f, 0x08, 0x8d, 0x6f, 0x24, 0xd8, 0x86, 0x63, 0x68, 0xb4,
  0xa8, 0x40, 0x04, 0x60, 0x50, 0x6f, 0x1e, 0xf7, 0x71, 0x6b, 0x89, 0xe5,
  0x71, 0xa4, 0x6b, 0x66, 0x3b, 0xe3, 0x7b, 0x23, 0x6e, 0xc4, 0xe3, 0x20,
  0x4d, 0x43, 0x96, 0xf3, 0x87, 0x91, 0x88, 0x5d, 0x1e, 0xf8, 0x2d, 0x5a,
  0x8c, 0xd6, 0xd1, 0xfe, 0x69, 0x86, 0x4c, 0x93, 0x42, 0xb6, 0x08, 0x98,
  0x96, 0xaf, 0xaf, 0x89, 0x9b, 0x60, 0xf0, 0x7c, 0x74, 0x03, 0xdb, 0x74,
  0xf5, 0x99, 0xaf, 0x23, 0x81, 0x0e, 0xd1, 0xcc, 0x87, 0x58, 0x7c, 0xa6,
  0x34, 0xc5, 0x41, 0x02, 0x0c, 0x3a, 0xc2, 0x88, 0x81, 0x62, 0x92, 0x28,
  0x55, 0x74, 0xcb, 0x66, 0x40, 0x9b, 0xf4, 0xfd, 0xd3, 0x89, 0x13, 0xd2,
  0x68, 0xcf, 0x43, 0x9a, 0x49, 0xd7, 0x20, 0xe5, 0x94, 0x52, 0x7c, 0x32,
  0xdf, 0x2e, 0x51, 0x30, 0x5d, 0xad, 0x10, 0x3a, 0xc3, 0xcd, 0x1a, 0x4b,
  0x1b, 0xc6, 0xf9, 0xd0, 0x6a, 0x68, 0x98, 0x59, 0x5b, 0x88, 0x58, 0x3a,
  0x37, 0x99, 0x01, 0x87, 0x91, 0x91, 0x8b, 0xe0, 0x19, 0xa2, 0xd7, 0x3f,
  0x10, 0x07, 0x1e, 0x6e, 0x17, 0x3c, 0xf2, 0x4c, 0x2c, 0xde, 0x49, 0x67,
  0xe2, 0x79, 0xd3, 0x44, 0xe5, 0xc1, 0x67, 0x13, 0xeb, 0xc6, 0x75, 0x07,
  0x0b, 0x02, 0x03, 0x01, 0x00, 0x01
};

using namespace std;

static string _slicenseFile = "";
static License * thisLicense = NULL;

License::License(const string & licensePath) {
	_licenseFile = licensePath;
	_errCode = 0;
	_isDemo = true;
	_isOnlineLicense = false;
	_expiration = -1;
	_flags = (uint64_t) 0xffffffffffffffffLL;

	_runningUID = "";
	_runningGID = "";
	_startingUID = "";
	_startingGID = "";
	_rawMachineID = "";
	_connectionCount = 0;
	_inBandwidth = 0;
	_outBandwidth = 0;
	_runtime = 0;
	bool file_found = fileExists(_licenseFile);

	if (file_found) {
		// Use LUA to parse the License file
		if (!ReadLuaFile(licensePath, "license", _licenseTerms)) {
			_errCode = PARSE_FAILURE;
		}
	} else {
		_errCode = INVALID_FILE;
	}
}

License::~License() {

}

void License::SetLicenseFile(const string & licensePath) {
	_slicenseFile = licensePath;
}

License* License::GetLicenseInstance() {
	if (thisLicense == NULL)
		thisLicense = new License(_slicenseFile);
	return thisLicense;
}

void License::ResetLicense() {
	if (thisLicense != NULL) {
		delete thisLicense;
		thisLicense = NULL;
	}
}

LICENSE_VALIDATION License::ValidateLicense() {
	// Don't re-run the check. The file is static (or better be!) so just short-circuit the evaluation
	if (_errCode != 0)
		return (LICENSE_VALIDATION) _errCode;

//	uint32_t currentVersion = LICENSE_CURRENT_VERSION + Version::GetBindId() * 100;

	// Separate bind ID and license version
	uint32_t currentVersion = ((uint32_t)_licenseTerms["VERSION"]) % 100;
	uint32_t bindId = ((uint32_t)_licenseTerms["VERSION"]) / 100;

	// Moved to the very beginning so that the flag _isOnlineLicense is properly set
	// Check if it needs to connect to License Manager
	if (_licenseTerms.HasKey("LICENSE_MANAGER", false)) {
		if ((_licenseTerms.HasKey("LICENSE_ID", false)) &&
				(_errCode == 0)) {
			_errCode = FOR_LM_VERIFICATION;
			_isOnlineLicense = true;
			_isDemo = false;
		} else {
			// If no license ID entry, bail out
			_errCode = PARSE_FAILURE;
		}

		if (_licenseTerms.HasKeyChain(_V_NUMERIC, false, 2, "LICENSE_MANAGER", "RUNTIME")) {
			_runtime = (uint32_t) _licenseTerms["LICENSE_MANAGER"]["RUNTIME"];
		}
	}

	//Verify the minimum set of keys are present
	// There must be a signature, a version and either a term or a node
	// (or both) as long as it is not LM-enabled
	if (!_licenseTerms.HasKey("SIGNATURE", false) ||
			!_licenseTerms.HasKey("VERSION", false) ||
			(!_licenseTerms.HasKey("TERM", false) &&
			!_licenseTerms.HasKey("NODE", false) &&
			!_isOnlineLicense)) {
		// Early quit for bad files
		_errCode = PARSE_FAILURE;
	} else if (currentVersion != LICENSE_CURRENT_VERSION) {
		// Verify Version number
		FATAL("Incompatible License Version! File Version: %d Server Version: %d \n",
				(int32_t) _licenseTerms["VERSION"], currentVersion);
		_errCode = INCOMPAT_VERSION;
	} else if(bindId != 0 && (!Version::HasBindId() || bindId != Version::GetBindId())){
		_errCode = UNAUTHORIZED_COMPUTER;
	} else if (!ValidateSignature()) {
		// Verify the file signature (make sure the file has not been tampered with)
		_errCode = TAMPERED;
	}

	// If we have set the error code at this point, we need to bail because there is a major problem
	if (_errCode != 0)
		return (LICENSE_VALIDATION) _errCode;

	// Check if this is a demo, also check if this is NOT lm-enabled
	_isDemo = (_licenseTerms.HasKey("DEMO", false) && _licenseTerms[ "DEMO" ] &&
			!_isOnlineLicense);

	// Check that the license has not expired
	if (IsExpired())
		_errCode = EXPIRED;

	// Check that if the license is node locked, we are running on the appropriate computer
	if (_licenseTerms.HasKey("NODE", false)) {
		if (!ValidateNode()) {
			_errCode = UNAUTHORIZED_COMPUTER;
		}
	}

	// Check if license has connection limit
	if (_licenseTerms.HasKey("CONNECTION_LIMIT")) {
		_connectionCount = _licenseTerms.GetValue("CONNECTION_LIMIT", false);
	}

	// Check if license has bandwidth limit
	if (_licenseTerms.HasKey("BANDWIDTH_LIMIT", false)) {
		if (_licenseTerms.HasKeyChain(_V_NUMERIC, false, 2, "BANDWIDTH_LIMIT", "IN")) {
			_inBandwidth = _licenseTerms["BANDWIDTH_LIMIT"]["IN"];
		}
		if (_licenseTerms.HasKeyChain(_V_NUMERIC, false, 2, "BANDWIDTH_LIMIT", "OUT")) {
			_outBandwidth = _licenseTerms["BANDWIDTH_LIMIT"]["OUT"];
		}
	}

	// If the errCode has not been set yet, we must have a valid file!
	if (_errCode == 0)
		_errCode = VALID;

	return (LICENSE_VALIDATION) _errCode;
}

void License::SetStartingGID() {
	_startingGID = LicenseHW::GetGID();
}

void License::SetStartingUID() {
	_startingUID = LicenseHW::GetUID();
}

void License::SetRunningGID() {
	_runningGID = LicenseHW::GetGID();
}

void License::SetRunningUID() {
	_runningUID = LicenseHW::GetUID();
}

string License::GenerateMachineId() {
	/*
	 * At this moment, we are only using MAC address for the machine ID
	 * This will change as we add more details
	 */

	//1.  Get MAC Address
	string macAdd;
	vector<string> hwNode;
	if (LicenseHW::IsAMI(_licenseTerms)) {
		return LicenseHW::GetAMIMarker();
	}
	else
		LicenseHW::GetHWMarkers(hwNode);

	if (hwNode.size() > 0) {
		// Sort the mac addresses to be sure that we'll always get the same address
		sort(hwNode.begin(), hwNode.end());

		// Get the first mac address after sorting
		macAdd = upperCase(hwNode[0]);
	} else {
		// Drop everything if don't have any valid mac address
		ASSERT("Could not generate Machine ID!");
	}

	//2. Encrypt
	return upperCase(md5(macAdd, true));
	/*	string returnString = "";

		//3.  Return machineID
		replace(returnString, " ", "");
		return returnString;*/
}

string License::GenerateExtendedMachineDetails() {

	//1.  Get number of cores
	int8_t coreCount = getCPUCount();

	//2.  Get available memory
	uint64_t availableMemory = LicenseHW::GetAvailableMemory();

	//3.  Get kernel version
	string kernelVersion = LicenseHW::GetKernelVersion();

	//4.  Form key-value pairing for each detail
	string rtStartingUID = format("RUNTIME_START_UID=%s\n", STR(_startingUID));
	string rtStartingGID = format("RUNTIME_START_GID=%s\n", STR(_startingGID));
	string rtRunningUID = format("RUNTIME_RUN_UID=%s\n", STR(_runningUID));
	string rtRunningGID = format("RUNTIME_RUN_GID=%s\n", STR(_runningGID));
	string rtCoreCount = format("RUNTIME_CPU_COUNT=%"PRId8"\n", coreCount);
	string rtAvailableMem = format("RUNTIME_AVAILABLE_MEMORY=%"PRIu64"\n", availableMemory);
	string rtKernelVersion = format("RUNTIME_KERNEL_VERSION=%s\n", STR(kernelVersion));
	string rtRMSVersion = format("RUNTIME_RMS_VERSION=%s", STR(Version::GetBanner()));

	//4.  Generate extended using all generated key-value pairs
	string extendedDetails = format("%s%s%s%s%s%s%s%s", STR(rtStartingUID),
			STR(rtStartingGID), STR(rtRunningUID), STR(rtRunningGID),
			STR(rtCoreCount), STR(rtAvailableMem), STR(rtKernelVersion),
			STR(rtRMSVersion));

	//5.  Done
	return extendedDetails;
}

bool License::IsExpired() {
	if (_isOnlineLicense)
		return false;

	if (_expiration < 0)
		ParseTime();

	// If we have an expiration to compare against, check to see if the expiration time has passed
	bool expired(false);
	if (_expiration > 0) {
		expired = (_expiration <= time(NULL));
	}

	if (expired)
		_errCode = EXPIRED;

	return expired;
}

bool License::IsDemo() {
	if (_errCode == 0)
		ValidateLicense();
	return _isDemo;
}

uint32_t License::GetExpirationTime() {
	return _expiration;
}

uint64_t License::GetFlags() {
	if (_flags == 0xffffffffffffffffLL) {
		if (_licenseTerms.HasKeyChain(_V_NUMERIC, false, 1, "FLAGS")) {
			_flags = (uint64_t) _licenseTerms.GetValue("FLAGS", false);
		} else {
			_flags = 0;
		}
	}
	return _flags;
}

string License::InterpretValidationCode(const LICENSE_VALIDATION code) {
	switch (code) {
		case VALID:
			return "License is Valid";
		case EXPIRED:
			return "The License has Expired";
		case UNAUTHORIZED_COMPUTER:
			return "The License is bound to a different computer or instance";
		case TAMPERED:
			return "The License has been modified and is therefore no longer valid";
		case INCOMPAT_VERSION:
			return "The License Version is incorrect. This version of the RMS requires License Version " + LICENSE_CURRENT_VERSION_STR;
		case PARSE_FAILURE:
			return "The License file is bad or incomplete";
		case INVALID_FILE:
			return "The License file could not be found";
		case FOR_LM_VERIFICATION:
			return "The License file still needs to be verified by the LM";
		default:
			break;
	}
	return "Unknown License state";
}

size_t License::StringifyLicense(string & license) {
	license.clear();

	// Make sure the file is generally ok (we dont care if it is a valid license, just a good file)
	if (_errCode == PARSE_FAILURE ||
			_errCode == INVALID_FILE)
		return 0;

	// Return the serialized license terms, excluding the signature
	Variant serializer(_licenseTerms);
	if (serializer.HasKeyChain(V_STRING, true, 1, "SIGNATURE"))
		serializer.RemoveKey("SIGNATURE");

	serializer.Compact();
	serializer.SerializeToBin(license);

	return license.size();
}

size_t License::GetSignature(string & license) {
	license.clear();

	// Make sure the file is generally ok (we dont care if it is a valid license, just a good file)
	if (_errCode == PARSE_FAILURE ||
			_errCode == INVALID_FILE)
		return 0;

	// Verify that the SIGNATURE key is available
	if (!_licenseTerms.HasKey("SIGNATURE", false))
		return 0;

	// The SIGNATURE key only has one value, the signature, so get it and we are done;
	license = (string) _licenseTerms["SIGNATURE"];
	return license.size();
}

uint32_t License::GetError() {
	return _errCode;
}

void License::CreateLicenseFile(const string & licensePath,
		const string & licenseId,
		const int & licType,
		const int & expTime,
		const int & shared,
		const int & demo,
		const int & version,
		const int & tolerant,
		const string & node,
		const string & notes,
		const string & signature,
		const string & rmsCert,
		const string & rmsKey,
		const string & caCert,
		const uint32_t & connectionLimit,
		const double & inboundBandwidthLimit,
		const double & outboundBandwithLimit,
		const uint32_t & runtime) {
	ofstream files(licensePath.c_str(), ios::out | ios::trunc);
	if (files) {
		files << "license =" << endl;
		files << "{" << endl;
		files << "	VERSION=" << version << "," << endl;

		// Flag to determine if license is the old type or lm enabled
		bool oldLicense = true;

#ifdef HAS_LICENSE_MANAGER
		if ((rmsCert != "") && (rmsKey != "") && (caCert != "")) {
			files << "	LICENSE_ID=\"" << licenseId << "\"," << endl;
			files << "	LICENSE_MANAGER=" << endl;
			files << "	{" << endl;
			files << "		KEY1=\"" << rmsCert << "\"," << endl;
			files << "		KEY2=\"" << rmsKey << "\"," << endl;
			files << "		KEY3=\"" << caCert << "\"," << endl;
			if (runtime > 0)
				files << "		RUNTIME=" << runtime << endl;
			files << "	}," << endl;

			// Unset flag
			oldLicense = false;
		}
#endif /* HAS_LICENSE_MANAGER */

		time_t now;
		time(&now);
		struct tm timeNow = *gmtime(&now);

		if (oldLicense) {
			files << "	TERM=" << endl;
			files << "	{" << endl;
			switch (licType) {
				case RUNTIME:
				{
					files << "		TYPE=\"RUNTIME\"," << endl;
					files << "		TIME=" << expTime * 3600 << ", -- " << expTime << " hour limit" << endl;
					break;
				}
				case EXPIRATION:
				{
					files << "		TYPE=\"EXPIRATION\"," << endl;
					struct tm t;
					now += (expTime * 3600);
					t = *gmtime(&now);
					files << "		TIME={year=" << t.tm_year + 1900 << ", month=" << t.tm_mon + 1 << ", day=" << t.tm_mday << ", hour=" << t.tm_hour << ", min=" << t.tm_min << ", sec=" << t.tm_sec << "}," << endl;
					break;
				}
				case OPEN:
				{
					files << "		TYPE=\"OPEN\"," << endl;
					break;
				}
				case CUMULATIVE:
				{
					files << "		TYPE=\"CUMULATIVE\"," << endl;
					break;
				}
			}

			//#ifdef HAS_LICENSE_MANAGER
			//		if (shared != 0) {
			//			files << "		SHARED=" << shared << endl;
			//		}
			//#endif /* HAS_LICENSE_MANAGER */

			files << "	}," << endl;
		}

		if (oldLicense) {
			if (connectionLimit != 0) {
				files << "	CONNECTION_LIMIT=" << connectionLimit << ", -- " << connectionLimit << " connections limit" << endl;
			}
			if (inboundBandwidthLimit != 0 || outboundBandwithLimit != 0) {
				files << "	BANDWIDTH_LIMIT=" << endl;
				files << "	{" << endl;
				files << "		IN=" << inboundBandwidthLimit << ", -- maximum inbound bandwidth" << endl;
				files << "		OUT=" << outboundBandwithLimit << ", -- maximum outbound bandwidth" << endl;
				files << "	}," << endl;
			}

			files << "	DEMO=" << demo << "," << endl;

			if (node != "") {
				files << "	NODE=" << endl;
				files << "	{" << endl;
				files << "		VALUE=\"" << node << "\"" << endl;
				files << "	}," << endl;
			}
		}

		if (notes != "") {
			files << "	NOTES=" << endl;
			files << "	{" << endl;
			files << "		ISSUE_DATE={year=" << timeNow.tm_year + 1900 << ", month=" << timeNow.tm_mon + 1 << ", day=" << timeNow.tm_mday << ", hour=" << timeNow.tm_hour << ", min=" << timeNow.tm_min << ", sec=" << timeNow.tm_sec << "}," << endl;
			files << notes;
			files << "	}," << endl;
		}

		if (tolerant) {
			files << "	TOLERANT=" << tolerant << "," << endl;
		}

		if (signature != "") {
			files << "	SIGNATURE=\"" << signature << "\"" << endl;
		}
		files << "}" << endl;
		files.close();
	}
}

#ifdef HAS_LICENSE_MANAGER

string License::GetLMCertificate() {
	// Make sure the file is generally ok (we dont care if it is a valid license, just a good file)
	if (_errCode == PARSE_FAILURE || _errCode == INVALID_FILE)
		return "";

	// Verify that the RMS certificate key is available
	if (!_licenseTerms.HasKeyChain(V_STRING, false, 2, "LICENSE_MANAGER", "KEY3"))
		return "";

	return (string) _licenseTerms["LICENSE_MANAGER"]["KEY3"];
}

string License::GetRMSCertificate() {
	if (_errCode == PARSE_FAILURE || _errCode == INVALID_FILE)
		return "";

	// Verify that the RMS certificate key is available
	if (!_licenseTerms.HasKeyChain(V_STRING, false, 2, "LICENSE_MANAGER", "KEY1"))
		return "";

	return (string) _licenseTerms["LICENSE_MANAGER"]["KEY1"];
}

string License::GetRMSKey() {
	if (_errCode == PARSE_FAILURE || _errCode == INVALID_FILE)
		return "";

	// Verify that the RMS licenseKey key is available
	if (!_licenseTerms.HasKeyChain(V_STRING, false, 2, "LICENSE_MANAGER", "KEY2"))
		return "";

	return (string) _licenseTerms["LICENSE_MANAGER"]["KEY2"];
}

string License::GetCACertificate() {
	// Make sure the file is generally ok (we dont care if it is a valid license, just a good file)
	if (_errCode == PARSE_FAILURE || _errCode == INVALID_FILE)
		return "";

	// Verify that the CA certificate key is available
	if (!_licenseTerms.HasKeyChain(V_STRING, false, 2, "LICENSE_MANAGER", "KEY3"))
		return "";

	return (string) _licenseTerms["LICENSE_MANAGER"]["KEY3"];
}

string License::GetLicenseId() {
	// Make sure the file is generally ok (we dont care if it is a valid license, just a good file)
	if (_errCode == PARSE_FAILURE || _errCode == INVALID_FILE)
		return "";

	// Verify that the licenseId key is available
	if (!_licenseTerms.HasKey("LICENSE_ID", false))
		return "";

	return (string) _licenseTerms["LICENSE_ID"];
}

bool License::IsTolerant() {
	bool isTolerant = false;
	// Make sure the file is generally ok (we dont care if it is a valid license, just a good file)
	if (_errCode == PARSE_FAILURE || _errCode == INVALID_FILE)
		return false;

	isTolerant = _licenseTerms.HasKey("TOLERANT", false) && _licenseTerms[ "TOLERANT" ];

	return (bool) isTolerant;
}

uint32_t License::GetRuntime() {
	// Make sure the file is generally ok (we dont care if it is a valid license, just a good file)
	if (_errCode == PARSE_FAILURE || _errCode == INVALID_FILE)
		return 0;

	// Verify that the licenseId key is available
	if (!_licenseTerms.HasKey("LICENSE_ID", false))
		return 0;

	return _runtime;
}
#endif /* HAS_LICENSE_MANAGER */

uint32_t License::GetConnectionLimit() {
	return _connectionCount;
}

double License::GetInboundBandwidthLimit() {
	return _inBandwidth;
}

double License::GetOutboundBandwidthLimit() {
	return _outBandwidth;
}

bool License::ParseTime(void) {
	// Reset the expiration time to the invalid/preset value
	_expiration = -1;

	// Find the expiration of the license
	if (_licenseTerms.HasKey("TERM", false)) {
		if (_licenseTerms["TERM"].HasKey("TYPE", false)) {
			// There could be multiple Term Types, so these are explicitly NOT "else if's"
			// TODO: Actually, there can't be multiple right now until i figure out what that will
			//        look like in the actual license file...
			if (_licenseTerms["TERM"]["TYPE"] == "OPEN") {
				// For open, set _expiration to 0 since there is no expiration
				_expiration = 0;
			}

			if (_licenseTerms["TERM"]["TYPE"] == "RUNTIME" &&
					_licenseTerms["TERM"].HasKey("TIME", false)) {
				// For runtime types, add the time to "now" since we want to expire in TIME seconds from now
				_expiration = _licenseTerms["TERM"]["TIME"];
				_expiration += (int32_t) time(NULL);
			}

			if (_licenseTerms["TERM"]["TYPE"] == "EXPIRATION" &&
					_licenseTerms["TERM"].HasKey("TIME", false)) {
				// For this type, the time value is the expiration time/date
				Timestamp ts = _licenseTerms["TERM"]["TIME"];
				// In case of multiple time expirations use the nearest/soonest time
				if (_expiration > 0) {
					int32_t ttime = (int32_t) timegm(&ts);
					_expiration = (ttime > _expiration ? _expiration : ttime);
				} else
					_expiration = (int32_t) mktime(&ts);
			}

			// If we have not set expiration then something went wrong!
			if (_expiration < 0) {
				FATAL("Mal-formed TERM clause!");
				_errCode = PARSE_FAILURE;
			}
		} else {
			FATAL("There is no TERM TYPE for this License!");
			_errCode = PARSE_FAILURE;
		}
	} else {
		// Treat no TERM key as an "OPEN" type license
		_expiration = 0;
	}

	// Tell everyone when this license expires
	if (_expiration > 0) {
		time_t texp = _expiration;
		tm * ts = localtime(&texp);
		char buf[80];
		strftime(buf, sizeof (buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
		fprintf(stdout, "The Server License Expires On: %s\n", buf);
	} else {
		fprintf(stdout, "This Server License will not expire.\n");
	}

	// expiration is set to -1 on construction, and it will be 0 or greater if we have set it.
	return ( _expiration >= 0);
}

bool License::ValidateSignature(void) {
	//return true;
	//Load the default config file, this is a recommended step
	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x10100000L
	OPENSSL_config(NULL);
	#endif

	// Convert the private key into the key structure
	const unsigned char *pTemp = gPublicKeyDer;
	EVP_PKEY *pPubKey = d2i_PUBKEY(NULL, &pTemp, sizeof (gPublicKeyDer));
	if (pPubKey == NULL) {
		FATAL("License validation failed");
		return false;
	}

	// Get the string representation of the license file
	string licContents;
	if (StringifyLicense(licContents) <= 0) {
		FATAL("License validation failed");
		return false;
	}

	// initializes a signing context
	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x10100000L
	OpenSSL_add_all_digests();
	#endif

	const EVP_MD *pMdType = EVP_get_digestbyname("sha1");
	if (pMdType == NULL) {
		FATAL("License validation failed");
		return false;
	}

	//Redmine 2269
	EVP_MD_CTX* ctx;
	#if OPENSSL_API_COMPAT < 0x10100000L
	ctx = EVP_MD_CTX_create();
	#else
	ctx = EVP_MD_CTX_new();
	#endif

	if (EVP_VerifyInit(ctx, pMdType) != 1) {
		FATAL("License validation failed");
		return false;
	}

	// Add the license contents into the context
	if (EVP_VerifyUpdate(ctx, licContents.data(), licContents.size()) != 1) {
		FATAL("License validation failed");
		return false;
	}

	// Get the signature
	string hrSig;
	if (GetSignature(hrSig) <= 0) {
		FATAL("License validation failed");
		return false;
	}

	//Convert the stringified signature back into binary
	string sig = unb64(hrSig);

	/*/testing
	 *
	 * TODO: replace cout with fprintf(stdout,...)
	 *
		cout << "Binary MD5  : " << md5( sig, true ) << endl;
		cout << "Readable MD5: " << md5( hrSig, true ) << endl;
		cout << "LicContents : " << md5( licContents, true ) << endl;
	 */

	// Verify the signature
	int success = EVP_VerifyFinal(ctx, (const unsigned char *) sig.data(),
			(unsigned int) sig.size(), pPubKey);

#if 0 //debugging EVP Errors
	/*
	 *
	 *
	 * TODO: replace cout with fprintf(stdout,...)
	 *
	 *
	 * */
	cout << "Verify: " << success << endl;
	if (success < 0) {
		ERR_load_crypto_strings();
		/*
		 *
		 *
		 * TODO: replace cout with fprintf(stdout,...)
		 *
		 *
		 */
		cout << "******" << endl;

		unsigned long osl_err = ERR_get_error();
		while (osl_err != 0) {
			/*
			 *
			 *
			 * TODO: replace cout with fprintf(stdout,...)
			 *
			 *
			 */
			cout << "Trying to print error: " << osl_err << endl;
			//cout << ERR_reason_error_string( ERR_GET_REASON( osl_err ) ) << endl;
			//cout << ERR_func_error_string( ERR_GET_FUNC( osl_err ) ) << endl;
			cout << ERR_error_string(osl_err, NULL) << endl;
			cout << "*" << endl;
			osl_err = ERR_get_error();
		}

		/*
		 *
		 *
		 * TODO: replace cout with fprintf(stdout,...)
		 *
		 *
		 */
		cout << "******" << endl;
		ERR_free_strings();
	}
#endif

	// Cleanup
	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x10100000L
	EVP_MD_CTX_cleanup(ctx);
	EVP_MD_CTX_destroy(ctx);
	#else
	EVP_MD_CTX_free(ctx);
	#endif

	EVP_PKEY_free(pPubKey);

	//Redmine 2269
	#if OPENSSL_API_COMPAT < 0x10100000L
	ERR_remove_state(0);
	#endif

	return (success == 1);
}

bool License::ValidateNode(void) {
	if (!_licenseTerms["NODE"].HasKey("VALUE")) {
		FATAL("Mal-Formed NODE key.");
		return false;
	}

	// TODO: This will eventually be encrypted and consist of more than just one element!

	// Get the node from the license file
	string licNode = _licenseTerms["NODE"]["VALUE"];
	licNode = ConvertMAC(licNode);

	// Get the info from this computer/instance
	vector<string> hwNode;
	if (LicenseHW::IsAMI(_licenseTerms)) {
		return true;
	} 
	else
		LicenseHW::GetHWMarkers(hwNode);

	bool valid(false);
	// For now, this is a very permissive check. Look for ANY value to match that in the license file

	FOR_VECTOR(hwNode, i) {
		if (licNode == ConvertMAC(hwNode[i])) {
			valid = true;
			break;
		}
	}

	return valid;
}

string License::ConvertMAC(string mac) {
	const int SIZE = 12;
	char macValue[SIZE + 1];
	int index = 0;

	// Convert to uppercase
	mac = upperCase(mac);

	// Valid MAC format are the following:
	// xx:yy:zz:xx:yy:zz
	// xx-yy-zz-xx-yy-zz
	// xxxx.yyyy.zzzz
	for (std::size_t i = 0; i < mac.length(); i++) {
		if ((mac[i] == ':') || (mac[i] == '-') || (mac[i] == '.')) continue;
		macValue[index++] = mac[i];
	}

	// Null terminate
	macValue[SIZE] = 0;

	return string(macValue);
}

#endif /* HAS_LICENSE */
