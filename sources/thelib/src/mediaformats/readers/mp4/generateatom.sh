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
CLASSNAME="Atom`echo $1|tr '[:lower:]' '[:upper:]'`"
FILENAME="`echo $CLASSNAME|tr '[:upper:]' '[:lower:]'`.cpp"
HEADER="`echo $CLASSNAME|tr '[:upper:]' '[:lower:]'`.h"

echo "/*" >$FILENAME
echo " * ============================================================================ " >>$FILENAME
echo " * Copyright 2019 RDK Management " >>$FILENAME
echo " * ============================================================================ " >>$FILENAME
echo " * Licensed under the Apache License, Version 2.0 (the "License");  " >>$FILENAME
echo " * you may not use this file except in compliance with the License.  " >>$FILENAME
echo " * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0." >>$FILENAME
echo " * ============================================================================ " >>$FILENAME
echo " * Copyright 2019 RDK Management " >>$FILENAME
echo " * ============================================================================ " >>$FILENAME
echo " */" >>$FILENAME
echo "" >>$FILENAME
echo "#include \"mediaformats/readers/mp4/$HEADER\"" >>$FILENAME
echo "" >>$FILENAME
echo "" >>$FILENAME
echo "$CLASSNAME::$CLASSNAME(MP4Document *pDocument, uint32_t type, uint32_t size, uint32_t start)" >>$FILENAME
echo ": BaseAtom(pDocument, type, size, start) {" >>$FILENAME
echo "" >>$FILENAME
echo "}" >>$FILENAME
echo "" >>$FILENAME
echo "$CLASSNAME::~$CLASSNAME() {" >>$FILENAME
echo "}" >>$FILENAME
echo "" >>$FILENAME
echo "bool $CLASSNAME::Read() {" >>$FILENAME
echo "    return SkipRead();" >>$FILENAME
echo "}" >>$FILENAME
echo "" >>$FILENAME
echo "string $CLASSNAME::Hierarchy(uint32_t indent) {" >>$FILENAME
echo "    return string(4 * indent, ' ') + GetTypeString();" >>$FILENAME
echo "}" >>$FILENAME
echo "" >>$FILENAME




