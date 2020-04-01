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
#  Create certificate & key for RMS using OpenSSL
#  120810
#  In:  rmsca.key    -- RDKC CA key
#  In:  rmsca.crt    -- RDKC CA certificate
#  Out: rms.key      -- RMS key
#  Out: rms.csr      -- RMS certificate signing request
#  Out: rms.crt      -- RMS certificate (signed by RDKC CA)

#  Generate an RMS key
openssl genrsa -out rms.key 1024

#  Request and sign a client (RMS) certificate using RDKC CA
openssl req -new -key rms.key -out rms.csr -subj "/C=PH/ST=NCR/L=ORTIGAS/O=RMS/OU=RMS/CN=localhost"
openssl x509 -req -days 365 -in rms.csr -CA rmsca.crt -CAkey rmsca.key -set_serial 02 -out rms.crt

#  Convert RMS certificate to PKCS#12 format
openssl pkcs12 -export -in rms.crt -out rms.p12 -name "RMSCertificate" -inkey rms.key
