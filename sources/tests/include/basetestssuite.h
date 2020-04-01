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



#ifndef _BASETESTSSUITE_H
#define	_BASETESTSSUITE_H

#include <assert.h>


#define __TESTS_VALIDATE_FROMAT_SPECIFIERS(...) \
do { \
   char ___tempLocation[1024]; \
   snprintf(___tempLocation,1023,__VA_ARGS__); \
}while (0)

#define TS_PRINT(...) do{__TESTS_VALIDATE_FROMAT_SPECIFIERS(__VA_ARGS__);printf(__VA_ARGS__);}while(0)

#define TS_ASSERT_NO_INCREMENT(x) \
do { \
    if(!(x)) { \
        TS_PRINT("Test failed. %s:%d\n",__FILE__,__LINE__); \
        exit(-1); \
    } \
} while(0)

#define TS_ASSERT(x) \
do { \
    if(!(x)) { \
        TS_PRINT("Test %s failed. %s:%d\n",#x,__FILE__,__LINE__); \
        exit(-1); \
    } else { \
        _testsCount++; \
    } \
} while(0)

#define EXECUTE_SUITE(x) \
do { \
        TS_PRINT("Executing %s... ",#x); \
        int before = BaseTestsSuite::_testsCount; \
        x().Run(); \
        int testsCount = BaseTestsSuite::_testsCount - before; \
        TS_PRINT("%d tests completed successfuly\n", testsCount); \
} while(0)

class BaseTestsSuite {
public:
	static int _testsCount;
public:
	BaseTestsSuite();
	virtual ~BaseTestsSuite();

	virtual void Run() = 0;
};


#endif	/* _BASETESTSSUITE_H */

