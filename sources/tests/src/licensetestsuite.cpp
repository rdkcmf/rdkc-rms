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
#include "licensetestssuite.h"
#include "utils/misc/license.h"

#define LICENSE_FILE "./License.lic"

LicenseTestsSuite::LicenseTestsSuite()
: BaseTestsSuite() {

}

LicenseTestsSuite::~LicenseTestsSuite() {

}

void LicenseTestsSuite::Run() {
	License::SetLicenseFile(LICENSE_FILE);
	License *pLicense = License::GetLicenseInstance();
	LICENSE_VALIDATION retval = pLicense->ValidateLicense();
	printf("ValidateLicense: %s\n", (pLicense->InterpretValidationCode(retval)).c_str());
	TS_ASSERT((retval == VALID) || (retval == FOR_LM_VERIFICATION));
	string licContents;
	TS_ASSERT(pLicense->StringifyLicense(licContents) > 0);
	License::ResetLicense();
}
#endif /* HAS_LICENSE */

