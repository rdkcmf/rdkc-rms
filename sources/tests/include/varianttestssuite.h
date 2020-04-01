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




#ifndef __VARIANTTESTSSUITE_H
#define __VARIANTTESTSSUITE_H

#include "common.h"
#include "basetestssuite.h"

class VariantTestsSuite
: public BaseTestsSuite {
private:
	Variant _noParamsVar;
	Variant _boolVar1;
	Variant _boolVar2;
	Variant _int8Var;
	Variant _int16Var;
	Variant _int32Var;
	Variant _int64Var;
	Variant _uint8Var;
	Variant _uint16Var;
	Variant _uint32Var;
	Variant _uint64Var;
	Variant _doubleVar;
	Variant _dateVar;
	Variant _timeVar;
	Variant _timestampVar1;
	Variant _timestampVar2;
	Variant _pcharVar;
	Variant _stringVar;
	Variant _mapVar;
	Variant _namedMapVar;
	Variant _referenceVariant1;
	Variant _referenceVariant2;
	Variant _referenceVariant3;
	Variant _referenceVariant4;
	Variant _referenceVariant5;
	Variant _referenceVariant6;
	Variant _referenceVariant7;
	Variant _referenceVariant8;
	Variant _referenceVariant9;
	Variant _referenceVariant10;
	Variant _referenceVariant11;
	Variant _referenceVariant12;
	Variant _referenceVariant13;
	Variant _referenceVariant14;
	Variant _referenceVariant15;
	Variant _referenceVariant16;
	Variant _referenceVariant17;
	Variant _referenceVariant18;
	Variant _referenceVariant19;
	Variant _referenceVariant20;
public:
	VariantTestsSuite();
	virtual ~VariantTestsSuite();
	virtual void Run();
private:
	void test_Constructors();
	void test_Reset();
	void test_ToString();
	void test_OperatorAssign();
	void test_OperatorCast();
	void test_OperatorEquality();
	void test_Compact();
	void test_SerializeDeserializeBin();
	void test_SerializeDeserializeXml();
	void test_LoadFromLua();
};

#endif /* __VARIANTTESTSSUITE_H */

