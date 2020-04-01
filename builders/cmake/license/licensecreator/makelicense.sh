#!/bin/bash
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
#  Create license for RMS using licensecreator
#  121107
#  In : rms.key      -- RMS key
#  In : rms.crt      -- RMS certificate (signed by RDKC CA)
#  In : rmsca.crt    -- RDKC CA certificate (self-signed)
#  In : note.txt     -- File containing remarks (optional)
#  Out: License.lic  -- RMS license
#
#  Note: the -pw parameter shown below (ABCD1234) should be corrected first

#  Prepare note for RMS license (optional)

if [ ! -s note.txt ]; then
  echo 'THIS IS A SAMPLE NOTE' > note.txt
fi

#  Create RMS license
# ./licensecreator -c=License.lic -pw=ABCD1234 -type=RUNTIME -time=10 -demo=1 -note=note.txt -lm=1 -id=52A54032-F525364B-9116449A-D548619E-2EAE8738-D6A93333-7F901C5C-12FE3A7E
# ./licensecreator -c=License.lic -pw=ABCD1234 -type=EXPIRATION -time=10 -demo=0 -note=note.txt -lm=1 -id=52A54032-F525364B-9116449A-D548619E-2EAE8738-D6A93333-7F901C5C-12FE3A7E
./licensecreator -c=License.lic -pw=ABCD1234 -type=OPEN -shared=5 -demo=0 -note=note.txt

#  Sign license
./licensecreator -s=License.lic -pw=ABCD1234

#  Failed test cases
# ./licensecreator -c=License.lic -pw=ABCD1234 -type=OPEN -shared=0 -demo=0 -note=note.txt
# ./licensecreator -c=License.lic -pw=ABCD1234 -type=OPEN -shared=11 -demo=0 -note=note.txt
# ./licensecreator -c=License.lic -pw=ABCD1234 -type=RUNTIME -time=0 -demo=1 -note=note.txt
# ./licensecreator -c=License.lic -pw=ABCD1234 -type=RUNTIME -time=87601 -demo=1 -note=note.txt
