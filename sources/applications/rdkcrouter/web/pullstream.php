/*
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

*/
<html>
	<head>
	</head>
	<body>
		<form action="action.php" method="post">
			<p>
				<label>URI: <input type="text" name="uri" id="uri" style="width: 500px"></label><br>
				Examples:<br>
				<code>
					RTMP:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;rtmp://192.168.2.3/live/myStream<br>
					RTMP (with auth):&nbsp;&nbsp;rtmp://john:password@192.168.2.3/live/myStream<br>
					RTSP:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;rtsp://129.168.1.2/test.sdp<br>
					RTSP (with auth):&nbsp;&nbsp;rtsp://john:password@129.168.1.2/test.sdp<br>
					MPEG-TS unicast:&nbsp;&nbsp;&nbsp;dmpegtsudp://0.0.0.0:8765<br>
					MPEG-TS multicast:&nbsp;dmpegtsudp://239.1.2.3:7654<br>
				</code>
			</p>
			<p>
				<label>Keep alive: <input type="checkbox" name="keepAlive" id="keepAlive"></label><br>
				Check this box to do auto reconnect and retry
			</p>
			<p>
				<label>Local stream name:<input type="text" name="localStreamName" id="localStreamName"></label><br>
				After the stream is successfully pulled in, it will be named with the value above.<br>
				If no value is present, a default random value is going to be generated.<br>
				The local stream name must be unique.<br>
			</p>
			<input type="hidden" name="operation" id="operation" value="pull">
			<input type="submit" value="Pull stream">
		</form>
	</body>
</html>
