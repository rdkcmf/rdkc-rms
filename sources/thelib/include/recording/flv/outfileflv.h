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


#ifndef _OUTFILEFLV_H
#define	_OUTFILEFLV_H

#include "recording/baseoutrecording.h"

class BaseOutRecording;
class BaseClientApplication;

class DLLEXP OutFileFLV
: public BaseOutRecording {
private:
	File *_pFile;
	uint32_t _prevTagSize;
	uint8_t _tagHeader[11];
	double _chunkLength;
	bool _waitForIDR;
	double _lastVideoDts;
	double _lastAudioDts;
	uint32_t _chunkCount;
	Variant _metadata;
	IOBuffer _buff;
	Variant _chunkInfo;
	string _startTime;
	uint64_t _frameCount;
	BaseClientApplication *_pApp;
public:
	OutFileFLV(BaseProtocol *pProtocol, string name, Variant &settings);
	static OutFileFLV* GetInstance(BaseClientApplication *pApplication,
			string name, Variant &settings);
	virtual ~OutFileFLV();
	void SetApplication(BaseClientApplication *pApplication);
	BaseClientApplication *GetApplication();
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
private:
	bool WriteFLVHeader(bool hasAudio, bool hasVideo);
	bool WriteFLVMetaData();
	bool WriteFLVCodecAudio(AudioCodecInfoAAC *pInfoAudio);
	bool WriteFLVCodecVideo(VideoCodecInfoH264 *pInfoVideo);
	bool InitializeFLVFile(GenericProcessDataSetup *pGenericProcessDataSetup);
	bool WriteMetaData(GenericProcessDataSetup *pGenericProcessDataSetup);
	bool WriteCodecSetupBytes(GenericProcessDataSetup *pGenericProcessDataSetup);
	void UpdateDuration();
	bool SplitFile();
};

#endif	/* _OUTFILEFLV_H */
