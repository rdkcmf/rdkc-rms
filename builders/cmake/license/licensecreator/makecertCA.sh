#!/bin/bash
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
#  Create a certificate & key for RDKC CA using OpenSSL (for testing only)
#  120810
#  In:  --
#  Out: rmsca.key      -- RDKC CA key
#  Out: rmsca.csr      -- RDKC CA certificate signing request
#  Out: rmsca.crt      -- RDKC CA certificate (self-signed)

#  Generate an RDKC CA key
openssl genrsa -out rmsca.key 1024 

#  Create a RDKC CA certificate
openssl req -new -key rmsca.key -out rmsca.csr -subj "/C=PH/ST=NCR/L=ORTIGAS/O=RMS/OU=CA/CN=localhost"
openssl x509 -req -days 365 -in rmsca.csr -signkey rmsca.key -out rmsca.crt
