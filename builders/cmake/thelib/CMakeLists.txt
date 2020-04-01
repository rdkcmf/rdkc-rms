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
#########################################################################
cmake_minimum_required(VERSION 2.6)
PROJECT(thelib)

#make the list with all the source files
FILE(GLOB_RECURSE SOURCES "${SOURCES_ROOT}/thelib/src/*.cpp" )

IF(SOCKREACTOR MATCHES "NET_KQUEUE")
	#MESSAGE(STATUS "socket reactor: kqueue")
	SET(KEEP ".*/netio/kqueue/.*")
ELSEIF(SOCKREACTOR MATCHES "NET_EPOLL")
	#MESSAGE(STATUS "socket reactor: epoll")
	SET(KEEP ".*/netio/epoll/.*")
ELSEIF(SOCKREACTOR MATCHES "NET_SELECT")
	#MESSAGE(STATUS "socket reactor: select")
	SET(KEEP ".*/netio/select/.*")
ENDIF(SOCKREACTOR MATCHES "NET_KQUEUE")

FOREACH(CURRENT ${SOURCES})
	IF(${CURRENT} MATCHES ".*/netio/(epoll|select|kqueue|iocp)/.*")
		#MESSAGE(STATUS "Evaluate ${CURRENT}")
		IF(${CURRENT} MATCHES "${KEEP}")
			#MESSAGE(STATUS "KEEP ${CURRENT}")
		ELSE(${CURRENT} MATCHES "${KEEP}")
			#MESSAGE(STATUS "REMOVE ${CURRENT}")
			LIST(APPEND TOBEREMOVED "${CURRENT}")
		ENDIF(${CURRENT} MATCHES "${KEEP}")
	ENDIF(${CURRENT} MATCHES ".*/netio/(epoll|select|kqueue|iocp)/.*")
ENDFOREACH(CURRENT)

FOREACH(CURRENT ${TOBEREMOVED})
	#MESSAGE(STATUS "*** REMOVE ${CURRENT}")
	LIST(REMOVE_ITEM SOURCES "${CURRENT}")
ENDFOREACH(CURRENT ${TOBEREMOVED})

LIST(SORT SOURCES)
FOREACH(CURRENT ${SOURCES})
	#MESSAGE(STATUS "+++ ${CURRENT}")
ENDFOREACH(CURRENT ${SOURCES})


#prepare the include directories
INCLUDE_DIRECTORIES(
	${OPENSSL_INCLUDE_PATH}
	${LUA_INCLUDE_PATH}
	${SOURCES_ROOT}/common/include
	${SOURCES_ROOT}/thelib/include
	${SCTP_INCLUDE_PATH}
	)

#create the library
ADD_LIBRARY(${PROJECT_NAME} ${LIB_TYPE} ${SOURCES})
ADD_PRECOMPILED_HEADER(${PROJECT_NAME} "${SOURCES_ROOT}/common/include/common.h")
IF(${LIB_TYPE} MATCHES "SHARED")
	TARGET_LINK_LIBRARIES(${PROJECT_NAME}
		common
		${DL_LIBRARY_PATH}
		${OPENSSL_LIBRARY_PATH}
		${CRYPTO_LIBRARY_PATH}
		${Z_LIBRARY_PATH}
		${SCTP_LIBRARY_PATH}
		#/opt/local/lib/libiconv.dylib
		)
	INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${RDKCMEDIASERVER_ROOT})
ENDIF(${LIB_TYPE} MATCHES "SHARED")

