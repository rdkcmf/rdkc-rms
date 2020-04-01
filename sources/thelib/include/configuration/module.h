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


#ifndef _MODULE_H
#define	_MODULE_H

#include "common.h"

class BaseClientApplication;
class BaseProtocolFactory;
class IOHandler;

typedef BaseClientApplication * (*GetApplicationFunction_t)(Variant configuration);
//typedef void (*ReleaseApplicationFunction_t)(BaseClientApplication * pApplication);

typedef BaseProtocolFactory * (*GetFactoryFunction_t)(Variant configuration);
//typedef void (*ReleaseFactoryFunction_t)(BaseProtocolFactory *pFactory);

struct Module {
	Variant config;
	GetApplicationFunction_t getApplication;
	GetFactoryFunction_t getFactory;
	BaseClientApplication *pApplication;
	BaseProtocolFactory *pFactory;
	LIB_HANDLER libHandler;
	vector<IOHandler *> acceptors;

	Module();
	void Release();
	bool Load();
	bool LoadLibrary();
	bool ConfigFactory();
	bool BindAcceptors();
	bool BindAcceptor(Variant & node);
	bool ConfigApplication();
};

#endif	/* _MODULE_H */

