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


#ifndef _MEDIAFILE_H
#define	_MEDIAFILE_H

#include "common.h"

//#define MediaFile File

#ifdef HAS_MMAP
#define MediaFile MmapFile
#else
#define MediaFile File
#endif /* HAS_MMAP */

MediaFile* GetFile(string filePath, uint32_t windowSize);
void ReleaseFile(MediaFile *pFile);
bool GetFile(string filePath, uint32_t windowSize, MediaFile &mediaFile);

#endif	/* _MEDIAFILE_H */
