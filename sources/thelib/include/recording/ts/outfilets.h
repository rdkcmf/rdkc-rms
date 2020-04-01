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


#ifndef _OUTFILETS_H
#define	_OUTFILETS_H

#include  "recording/baseoutrecording.h"

class FileTSSegment;
class BaseOutHLSStream;
class BaseClientApplication;

class DLLEXP OutFileTS
: public BaseOutRecording {
private:
	double _lastPatPmtSpsPpsTimestamp;
	FileTSSegment *_pCurrentSegment;
	IOBuffer _segmentVideoBuffer;
	IOBuffer _segmentAudioBuffer;
	IOBuffer _segmentPatPmtAndCountersBuffer;
	double _chunkLength;
	bool _waitForIDR;
	uint32_t _chunkCount;
	double _chunkDts;
	Variant _newSettings;
	Variant _chunkInfo;
	BaseClientApplication *_pApp;
public:
	OutFileTS(BaseProtocol *pProtocol, string name, Variant &settings);
	static OutFileTS* GetInstance(BaseClientApplication *pApplication,
			string name, Variant &settings);
	virtual ~OutFileTS();
	void SetApplication(BaseClientApplication *pApplication);
	BaseClientApplication *GetApplication();
protected:
	virtual bool FinishInitialization(
			GenericProcessDataSetup *pGenericProcessDataSetup);
	virtual bool PushVideoData(IOBuffer &buffer, double pts, double dts, bool isKeyFrame);
	virtual bool PushAudioData(IOBuffer &buffer, double pts, double dts);
	virtual bool IsCodecSupported(uint64_t codec);
private:
	bool WritePatPmt(double ts);
	bool SplitFile();
	bool FormatFileName();
};

#endif	/* _OUTFILETS_H */
