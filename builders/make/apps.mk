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
#rdkcrouter
RDKCROUTER_INCLUDE=$(THELIB_INCLUDE) -I$(PROJECT_BASE_PATH)/sources/applications/rdkcrouter/include
RDKCROUTER_LIBS=$(THELIB_LIBS) -L$(OUTPUT_DYNAMIC) -lthelib
RDKCROUTER_SRCS=$(shell find $(PROJECT_BASE_PATH)/sources/applications/rdkcrouter/src -type f -name "*.cpp")
RDKCROUTER_OBJS=$(RDKCROUTER_SRCS:.cpp=.rdkcrouter.o)


rdkcrouter: thelib $(RDKCROUTER_OBJS)
	@mkdir -p $(OUTPUT_DYNAMIC)/applications/rdkcrouter/mediaFolder
	@mkdir -p $(OUTPUT_STATIC)/applications/rdkcrouter/mediaFolder
	$(CXXCOMPILER)  -shared $(RDKCROUTER_LIBS) -o $(call dynamic_lib_name,rdkcrouter,/applications/rdkcrouter) $(call dynamic_lib_flags,rdkcrouter) $(RDKCROUTER_OBJS)

%.rdkcrouter.o: %.cpp
	$(CXXCOMPILER) $(COMPILE_FLAGS) $(DEFINES) $(RDKCROUTER_INCLUDE) -c $< -o $@

ALL_APPS_OBJS= $(RDKCROUTER_OBJS)
ACTIVE_APPS= -DHAS_APP_RDKCROUTER
applications: thelib rdkcrouter

