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


#include "commontestssuite.h"
#include "varianttestssuite.h"
#include "thelibtestssuite.h"
#ifdef HAS_LICENSE
#include "licensetestssuite.h"
#endif /* HAS_LICENSE */

void CleanupSSL();

int main(void) {
	TS_PRINT("Begin tests...\n");
	EXECUTE_SUITE(CommonTestsSuite);
	EXECUTE_SUITE(VariantTestsSuite);
	EXECUTE_SUITE(TheLibTestsSuite);
#ifdef HAS_LICENSE
	EXECUTE_SUITE(LicenseTestsSuite);
#endif /* HAS_LICENSE */
	TS_PRINT("A total of %d tests completed successfuly\n", BaseTestsSuite::_testsCount);
	CleanupSSL();
	return 0;
}
