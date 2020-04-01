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


#include "mediaformats/readers/basemediadocument.h"

BaseMediaDocument::BaseMediaDocument(Metadata &metadata)
: _metadata(metadata) {
	_audioSamplesCount = 0;
	_videoSamplesCount = 0;
	_keyframeSeek = false;
}

BaseMediaDocument::~BaseMediaDocument() {
}

bool BaseMediaDocument::Process() {
	double startTime = 0;
	double endTime = 0;
	GETCLOCKS(startTime, double);

	//1. Compute the names
	_mediaFilePath = _metadata.mediaFullPath();
	_metaFilePath = _metadata.metaFileFullPath();
	_seekFilePath = _metadata.seekFileFullPath();
	_keyframeSeek = _metadata.storage().keyframeSeek();
	_seekGranularity = _metadata.storage().seekGranularity();

	//1. Open the media file
	if (!GetFile(_mediaFilePath, 4 * 1024 * 1024, _mediaFile)) {
		FATAL("Unable to open media file: %s", STR(_mediaFilePath));
		return false;
	}

	//4. Read the document
	if (!ParseDocument()) {
		FATAL("Unable to parse document");
		return false;
	}

	//5. Build the frames
	if (!BuildFrames()) {
		FATAL("Unable to build frames");
		return false;
	}

	//6. Save the seek file
	if (!SaveSeekFile()) {
		FATAL("Unable to save seeking file");
		return false;
	}

	//7. Build the meta
	if (!SaveMetaFile()) {
		FATAL("Unable to save meta file");
		return false;
	}

	GETCLOCKS(endTime, double);

	uint64_t framesCount = _audioSamplesCount + _videoSamplesCount;
	if (framesCount == 0)
		framesCount = (uint64_t) _frames.size();

	INFO("%"PRIu64" frames computed in %.2f seconds at a speed of %.2f FPS",
			framesCount,
			(endTime - startTime) / (double) CLOCKS_PER_SECOND,
			(double) framesCount / ((endTime - startTime) / (double) CLOCKS_PER_SECOND));
	if (_frames.size() != 0) {
		uint32_t totalSeconds = (uint32_t) (((uint32_t) _frames[_frames.size() - 1].dts) / 1000);
		uint32_t hours = totalSeconds / 3600;
		uint32_t minutes = (totalSeconds - hours * 3600) / 60;
		uint32_t seconds = (totalSeconds - hours * 3600 - minutes * 60);
		INFO("File size: %"PRIu64" bytes; Duration: %u:%u:%u (%u sec); Optimal bandwidth: %.2f kB/s",
				_mediaFile.Size(),
				hours, minutes, seconds,
				totalSeconds,
				(double) _streamCapabilities.GetTransferRate() / 8192);
	}

	moveFile(_seekFilePath + ".tmp", _seekFilePath);
	moveFile(_metaFilePath + ".tmp", _metaFilePath);

	Chmod(STR(_seekFilePath), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	Chmod(STR(_metaFilePath), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	return true;
}

Variant BaseMediaDocument::GetMetadata() {
	return _metadata;
}

MediaFile &BaseMediaDocument::GetMediaFile() {
	return _mediaFile;
}

bool BaseMediaDocument::CompareFrames(const MediaFrame &frame1, const MediaFrame &frame2) {
	if (frame1.dts == frame2.dts)
		return frame1.start < frame2.start;
	return frame1.dts < frame2.dts;
}

bool BaseMediaDocument::SaveSeekFile() {
	if (_frames.size() <= 2) {
		FATAL("No frames found");
		return false;
	}

	File seekFile;

	//1. Open the file
	if (!seekFile.Initialize(_seekFilePath + ".tmp", FILE_OPEN_MODE_TRUNCATE)) {
		FATAL("Unable to open seeking file %s", STR(_seekFilePath));
		return false;
	}

	//2. Setup the bandwidth hint in bytes/second
	uint32_t totalSeconds = (uint32_t) (((uint32_t) _frames[_frames.size() - 1].dts) / 1000);
	_streamCapabilities.SetTransferRate((double) _mediaFile.Size() / (double) totalSeconds * 8.0);

	//2. Serialize the stream capabilities
	IOBuffer raw;
	if (!_streamCapabilities.Serialize(raw)) {
		FATAL("Unable to serialize stream capabilities");
		return false;
	}
	if (!seekFile.WriteUI32(GETAVAILABLEBYTESCOUNT(raw))) {
		FATAL("Unable to serialize stream capabilities");
		return false;
	}
	if (!seekFile.WriteBuffer(GETIBPOINTER(raw), GETAVAILABLEBYTESCOUNT(raw))) {
		FATAL("Unable to serialize stream capabilities");
		return false;
	}

	//2. Write the number of frames
	uint32_t framesCount = (uint32_t) _frames.size();
	if (!seekFile.WriteUI32(framesCount)) {
		FATAL("Unable to write frame count");
		return false;
	}

	//3. Write the frames
	bool hasVideo = false;
	uint64_t maxFrameSize = 0;

	FOR_VECTOR(_frames, i) {
		MediaFrame &frame = _frames[i];
		if (maxFrameSize < frame.length) {
			//WARN("maxFrameSize bumped up: %"PRIu64" -> %"PRIu64, maxFrameSize, frame.length);
			maxFrameSize = frame.length;
		}
		hasVideo |= (frame.type == MEDIAFRAME_TYPE_VIDEO);
		if (!seekFile.WriteBuffer((uint8_t *) & frame, sizeof (frame))) {
			FATAL("Unable to write frame");
			return false;
		}
	}
	_keyframeSeek &= hasVideo;
	//4. Write the seek granularity
	if (!seekFile.WriteUI32(_seekGranularity)) {
		FATAL("Unable to write sampling rate");
		return false;
	}

	//4. create the time to frame index table. First, see what is the total time
	double totalTime = 0;

	if (framesCount >= 1) {
		totalTime = _frames[framesCount - 1].dts;

		//5. build the table
		uint32_t frameIndex = 1;
		uint32_t seekPoint = 0;
		if (totalTime > 345600000) { //max for 4 days (4days*24hrs/day*3600sec/hr*1000ms/sec)
			totalTime = 345600000;
		}
		uint64_t maxSeekPoints = (uint64_t) (totalTime / _seekGranularity);
		if (maxSeekPoints > 1209600) { //max for 1 wk at 1 sec (7days*24hrs/day*3600sec/hr*2tracks)
			_seekGranularity = (uint32_t) (totalTime / 1209600);
			maxSeekPoints = (uint64_t) (totalTime / _seekGranularity);
		}
		for (double timestamp = 0; timestamp <= totalTime; timestamp += _seekGranularity) {
			while (frameIndex < framesCount) {
				if (_keyframeSeek) {
					if (_frames[frameIndex].type != MEDIAFRAME_TYPE_VIDEO) {
						frameIndex++;
						continue;
					}
					if (_frames[frameIndex].isKeyFrame) {
						seekPoint = frameIndex;
					}
					if (_frames[frameIndex].dts <= timestamp) {
						frameIndex++;
						continue;
					}
					break;
				} else {
					if (_frames[frameIndex].dts <= timestamp) {
						frameIndex++;
						continue;
					}
					seekPoint = frameIndex - 1;
					break;
				}
			}
			if (seekPoint < framesCount) {
				if (!seekFile.WriteUI32(seekPoint)) {
					FATAL("Unable to write frame index");
					return false;
				}
			}
		}
	}

	//6. Save the max frame size
	if (!seekFile.WriteUI64(maxFrameSize)) {
		FATAL("Unable to write frame count");
		return false;
	}

	//7. Done
	return true;
}

bool BaseMediaDocument::SaveMetaFile() {
	double duration = 0;
	double startTimestamp = 0;
	double endTimestamp = 0;
	if (_frames.size() > 0) {
		startTimestamp = _frames[0].dts;
		endTimestamp = _frames[_frames.size() - 1].dts;
		duration = endTimestamp - startTimestamp;
	} else {
		duration = 0;
	}

	PublicMetadata publicMetadata = GetPublicMeta();

	publicMetadata.duration(duration / 1000.00);
	publicMetadata.startTimestamp(startTimestamp / 1000.0);
	publicMetadata.endTimestamp(endTimestamp / 1000.0);
	publicMetadata.bandwidth(_streamCapabilities.GetTransferRate() / 1024.0);
	publicMetadata.audioFramesCount(_audioSamplesCount);
	publicMetadata.videoFramesCount(_videoSamplesCount);
	publicMetadata.totalFramesCount((uint32_t) _frames.size());
	publicMetadata.fileSize((uint64_t) _mediaFile.Size());

	_metadata.publicMetadata(publicMetadata);

	publicMetadata.mediaFullPath(_metadata.mediaFullPath());
	return publicMetadata.SerializeToXmlFile(_metaFilePath + ".tmp");
}
