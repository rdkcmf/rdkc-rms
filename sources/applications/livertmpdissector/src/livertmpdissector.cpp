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



#include "application/baseclientapplication.h"
#include "livertmpdissector.h"
#include "livertmpdissectorapplication.h"
#include "protocolfactory.h"
using namespace app_livertmpdissector;

extern "C" BaseClientApplication *GetApplication_livertmpdissector(Variant configuration) {
	return new LiveRTMPDissectorApplication(configuration);
}

extern "C" DLLEXP BaseProtocolFactory *GetFactory_livertmpdissector(Variant configuration) {
	return new ProtocolFactory();
}
