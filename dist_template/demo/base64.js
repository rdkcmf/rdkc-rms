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

/*
Copyright (c) 2007-2016 Kevin van Zonneveld (https://kvz.io) and Contributors (https://locutus.io/authors)
*/

/*! base64_encode */
function base64_encode (stringToEncode) {
  //  discuss at: https://locutus.io/php/base64_encode/
  // original by: Tyler Akins (https://rumkin.com)
  // improved by: Bayron Guevara
  // improved by: Thunder.m
  // improved by: Kevin van Zonneveld (https://kvz.io)
  // improved by: Kevin van Zonneveld (https://kvz.io)
  // improved by: Rafal Kukawski (https://blog.kukawski.pl)
  // bugfixed by: Pellentesque Malesuada
  // improved by: Indigo744
  //   example 1: base64_encode('Kevin van Zonneveld')
  //   returns 1: 'S2V2aW4gdmFuIFpvbm5ldmVsZA=='
	// mozilla has this native
	// - but breaks in 2.0.0.12!
	//if (typeof this.window['btoa'] == 'function') {
	//    return btoa(stringToEncode);
	//}
	var b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	var o1, o2, o3, h1, h2, h3, h4, bits, i = 0,
	ac = 0,
	enc = "",
	tmpArr = [];

	if (!stringToEncode) {
		return stringToEncode;
	}

	do { // pack three octets into four hexets
		o1 = stringToEncode.charCodeAt(i++);
		o2 = stringToEncode.charCodeAt(i++);
		o3 = stringToEncode.charCodeAt(i++);

		bits = o1 << 16 | o2 << 8 | o3;

		h1 = bits >> 18 & 0x3f;
		h2 = bits >> 12 & 0x3f;
		h3 = bits >> 6 & 0x3f;
		h4 = bits & 0x3f;

		// use hexets to index into b64, and append result to encoded string
		tmpArr[ac++] = b64.charAt(h1) + b64.charAt(h2) + b64.charAt(h3) + b64.charAt(h4);
	} while (i < stringToEncode.length);

	enc = tmpArr.join('');

	var r = stringToEncode.length % 3;

	return (r ? enc.slice(0, r - 3) : enc) + '==='.slice(r || 3);
}
