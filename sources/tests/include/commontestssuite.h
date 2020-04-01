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



#ifndef __COMMONTESTSSUITE_H
#define __COMMONTESTSSUITE_H

#include "common.h"
#include "basetestssuite.h"

class CommonTestsSuite
: public BaseTestsSuite {
public:
	CommonTestsSuite();
	virtual ~CommonTestsSuite();

	virtual void Run();
private:
	//genericfunctionality.h
	void test_Endianess();
	void test_isNumeric();
	void test_lowerCase();
	void test_upperCase();
	void test_ltrim();
	void test_rtrim();
	void test_trim();
	void test_replace();
	void test_split();
	void test_mapping();
	void test_format();
	void test_splitFileName();
	void test_generateRandomString();
	void test_GetHostByName();
	void test_md5();
	void test_HMACsha256();
	void test_b64();
	void test_unb64();
	void test_unhex();
	void test_ParseURL();
	void test_setFdOptions();
};

#endif /* __COMMONTESTSSUITE_H */


