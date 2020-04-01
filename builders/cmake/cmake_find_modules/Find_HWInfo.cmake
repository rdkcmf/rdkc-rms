#get platform specific paths
SET(HWINFO_INCLUDE_PATH "")
SET(HWINFO_LIBRARY_PATH "")

IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	MESSAGE(STATUS "WE ARE ADDING PATHS FOR DARWIN")
#	SET(HWINFO_INCLUDE_PATH "/System/Library/Frameworks/IOKit.framework/Headers")
#	SET(HWINFO_INCLUDE_PATH ${HWINFO_INCLUDE_PATH} "/System/Library/Frameworks/CoreFoundation.framework/Headers")
	SET(HWINFO_LIBRARY_PATH "-framework IOKit")
	SET(HWINFO_LIBRARY_PATH ${HWINFO_LIBRARY_PATH} "-framework CoreFoundation")
ELSE(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	MESSAGE(STATUS "Platform ${CMAKE_SYSTEM_NAME} does not need any special paths")
ENDIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
