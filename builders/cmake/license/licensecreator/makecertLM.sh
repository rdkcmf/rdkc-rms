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
#  Create a certificate & key for LM using OpenSSL
#  120810
#  In:  rmsca.key    -- RDKC CA key
#  In:  rmsca.crt    -- RDKC CA certificate
#  Out: lm.key       -- LM key
#  Out: lm.csr       -- LM certificate signing request
#  Out: lm.crt       -- LM certificate (signed by RDKC CA)

#  Generate an LM key
openssl genrsa -out lm.key 1024

#  Request and sign a server (LM) certificate using RDKC CA
openssl req -new -key lm.key -out lm.csr -subj "/C=PH/ST=NCR/L=ORTIGAS/O=RMS/OU=LM/CN=localhost"
openssl x509 -req -days 365 -in lm.csr -CA rmsca.crt -CAkey rmsca.key -set_serial 01 -out lm.crt

