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
#ifndef _RMSLICENSE_H_
#define _RMSLICENSE_H_

#include "common.h"

enum DLLEXP LICENSE_VALIDATION {
	VALID = 1,
	EXPIRED = 2,
	UNAUTHORIZED_COMPUTER = 3,
	TAMPERED = 4,
	INCOMPAT_VERSION = 5,
	PARSE_FAILURE = 6,
	INVALID_FILE = 7,
	FOR_LM_VERIFICATION = 8
};

enum DLLEXP TYPE_LICENSE {
	RUNTIME = 1,
	EXPIRATION = 2,
	OPEN = 3,
	CUMULATIVE = 4
};

/*!
 * Flags masks
 */
#define LICENSE_FLAGS_SPS_OBFUSCATED 0x0000000000000001LL

class DLLEXP License {
private:
	string _licenseFile;
	unsigned int _errCode;
	Variant _licenseTerms;
	bool _isDemo;
	bool _isOnlineLicense;
	int32_t _expiration;
	uint64_t _flags;
	uint32_t _connectionCount;
	double _inBandwidth;
	double _outBandwidth;
	uint32_t _runtime;

	string _runningUID;
	string _runningGID;
	string _startingUID;
	string _startingGID;
	string _rawMachineID;
private:
	License(const string & licensePath);
	virtual ~License();
public:
	/*!
	 * Set the location of the license file.  This must be called before getting
	 * the first instance of the license
	 */
	static void SetLicenseFile(const string & licensePath);

	/*!
	 * Get a pointer to the active License manager.
	 */
	static License* GetLicenseInstance();

	/*! This function will delete the existing license instance and create a new one
	 *   This will cause a reparsing of the license file set by SetLicenseFile
	 *   This can be used to parse a second license file or to just reparse the original file.
	 */
	static void ResetLicense();

	/*! This is the primary call for this class.  Verifies that the passed license provides
		 sufficient permission to run the application.
		Calling this function a second time will NOT rerun the logic, the previous result will be returned.
	 */
	LICENSE_VALIDATION ValidateLicense();

	/**
	 * Sets the starting GID class member which is the GID when RMS started.
	 *
	 */
	void SetStartingGID();

	/**
	 * Sets the starting UID class member which is the UID when RMS started.
	 *
	 */
	void SetStartingUID();

	/**
	 * Sets the running GID class member which is the GID when RMS
	 * applies the --gid parameter.
	 *
	 */
	void SetRunningGID();

	/**
	 * Sets the running UID class member which is the UID when RMS
	 * applies the --uid parameter.
	 *
	 */
	void SetRunningUID();

	/**
	 * Generates the machineID based on this computer.
	 *
	 * Here is a sample of the details in key-value format.
	 * OS_TYPE=Linux
	 * OS_NAME=Ubuntu
	 * OS_VERSION=12.04
	 * OS_ARCH=x86_64
	 * HW_MOBO_SERIAL=24332432
	 * HW_MAC_ADDRESS=445
	 * HW_CPUID=32434"
	 *
	 * The machineID generated will be MD5'd as the return value.
	 *
	 * @return
	 */
	string GenerateMachineId();

	/**
	 * Generates extended machin details.
	 *
	 * Here is a sample of the details in key-value format.
	 * RUNTIME_START_UID=0
	 * RUNTIME_START_GID=0
	 * RUNTIME_RUN_UID=511
	 * RUNTIME_RUN_GID=511
	 * RUNTIME_CPU_COUNT=8
	 * RUNTIME_AVAILABLE_MEMORY=76567565
	 * RUNTIME_KERNEL_VERSION=3.2.12-3.2.4.amzn1.x86_64
	 * RUNTIME_RMS_VERSION=RDKC Media Server (www.comcast.com) version 1.5.1 build 1279 - 2012-06-29T09:30:54.000 (Barbarian)
	 *
	 * @return
	 */
	string GenerateExtendedMachineDetails();

	/*! This function checks the expiration time of the license against "now"
	 *   Use this function to recheck expiration during runtime
	 *   If the license has expired, the state of this class will be changed to EXPIRED and ValidateLicense will
	 *     return EXPIRED from that point forward
	 */
	bool IsExpired();

	/*!
	 * Does the License specify that the application should run as a demo
	 */
	bool IsDemo();

	/*! Returns the time and date (in unix seconds) that the license will expire
	 *  0 is returned if no expiration time is specified
	 */
	uint32_t GetExpirationTime();

	/*!
	 * Get all the flags associated with this license
	 */
	uint64_t GetFlags();

	/*!
	 * Provide a user-friendly name for the passed license validation code/enum
	 */
	string InterpretValidationCode(const LICENSE_VALIDATION code);

	/*! Get a string representation of the license file (excluding the signature)
	 *  0 will be returned if the license failed to be parsed.
	 *  otherwise the size of the license string will be returned
	 */
	size_t StringifyLicense(string & license);

	/*! Gets the signature from the file
	 *  0 will be returned if there is no signature
	 *  otherwise the size of the signature will be returned.
	 */
	size_t GetSignature(string & license);

	/*!
	 * Returns the last error code
	 */
	uint32_t GetError();

	/*!
	 * Create the the license file with or not signature (License.lic)
	 * By Nivonjy
	 */
	static void CreateLicenseFile(const string & licensePath,
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
			const uint32_t & runtime);

#ifdef HAS_LICENSE_MANAGER
	/*!
	 * Gets the RMS certificate from the license file
	 * empty string will be returned if there is no RMS certificate
	 * otherwise the size of the RMS certificate will be returned.
	 */
	string GetLMCertificate();

	/*!
	 * Gets the RMS certificate from the license file
	 * empty string will be returned if there is no RMS certificate
	 * otherwise the size of the RMS certificate will be returned.
	 */
	string GetRMSCertificate();

	/*! Gets the RMS key from the license file
	 *  empty string will be returned if there is no RMS key
	 *  otherwise the size of the RMS key will be returned.
	 */
	string GetRMSKey();

	/*! Gets the CA certificate from the license file
	 *  empty string will be returned if there is no CA certificate
	 *  otherwise the size of the CA certificate will be returned.
	 */
	string GetCACertificate();

	/*! Gets the license ID from the license file
	 *  empty string will be returned if there is no license ID
	 *  otherwise the size of the license ID will be returned.
	 */
	string GetLicenseId();

	bool IsTolerant();

	uint32_t GetRuntime();
#endif /* HAS_LICENSE_MANAGER */

	uint32_t GetConnectionLimit();
	double GetInboundBandwidthLimit();
	double GetOutboundBandwidthLimit();

private:
	bool ParseTime(void);
	bool ValidateSignature(void);
	bool ValidateNode(void);
	string ConvertMAC(string mac);
};
#endif /* HAS_LICENSE */
#endif /* HAS_LICENSE */
