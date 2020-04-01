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
13-Apr-2016

REQUIREMENTS:
- Ubuntu 14.04, 15.04, or 16.04 (for Sanity Test only)
  (Target RMS can be any OS supported by RMS)
- RUBY 1.9.3 or later
- Bundler
- Install all gems listed in Gemfile (run "bundle")
- VLC 2.2.0 or later
- FFMPEG 2.5.8 or later
- CHROME 46 or later
- chromedriver 2.0 or later
- bunny.mp4

SETTING UP THE TARGET RMS:
1. Copy "License.lic" to the "config" folder of the target RMS.
2. Copy "bunny.mp4" to the "media" folder of the target RMS.
3. Edit "config.lua" in the "config" folder of the target RMS:
     instancesCount=1,
     hasStreamAliases=false,
     hasIngestPoints=false,
     ip="0.0.0.0",    -- for protocol="inboundJsonCli"
     ip="0.0.0.0",    -- for protocol="inboundHttpJsonCli"
4. Edit "webconfig.lua" in the "config" folder of the target RMS:
     hasGroupNameAliases=false,

SETTING UP THE SANITY TESTER:
1. Copy Sanity Test files to a folder named "sanity".
2. Edit "settings.yml" in folder "sanity" according to the target RMS:
     Set "type" to "service" (apt/yum), "tarball", or "windows".
     Set "release" to "1.7.0" (latest) or "1.6.6" (previous).
     Set "RMS_IP" to the IP address of the target RMS.

STARTING THE TARGET RMS:
- For tarball type of installation on Linux:
    $ ./run_console_rms.sh
- For service type of installation (apt/yum) on Linux:
    $ sudo service rdkcms start_console
- For normal Windows installation:
    Double-click "run_console_rms.bat" in the "C:\RDKC"

RUNNING SANITY TEST:
- For a quick test (less than 1 minute):
    $ ./sanity.rb fast
- For a full test (less than 10 minutes):
    $ ./sanity.rb

CHECKING THE RESULTS:
- Test results are shown on the console like this:

    -- SANITY TEST STARTED
    1, features/version.feature, PASSED, 1 pass, 0 fail, 4 sec
    2, features/streamplay.feature, PASSED, 2 pass, 0 fail, 33 sec
    3, features/launchtransplay.feature, PASSED, 3 pass, 0 fail, 47 sec
    4, features/createhlsplay.feature, PASSED, 4 pass, 0 fail, 107 sec
    5, features/recordplay.feature, PASSED, 5 pass, 0 fail, 105 sec
    6, features/webrtcplay.feature, PASSED, 6 pass, 0 fail, 108 sec
    7, features/websocketplay.feature, PASSED, 7 pass, 0 fail, 145 sec
    -- SANITY TEST COMPLETED (549 sec)
    Tested = 7
    Failed = 0 (0.0%)
    Passed = 7 (100.0%)

- Detailed results are saved to HTML files.

