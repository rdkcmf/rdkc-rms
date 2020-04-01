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

#include <string.h>


#ifdef WIN32
	void *memmem(const void *l, size_t l_len, const void *s, size_t s_len) throw() {
#else
	void *memmem(const void *l, size_t l_len, const void *s, size_t s_len) {
#endif /* WIN32 */
	register char *cur, *last;
	const char *cl = (const char *) l;
	const char *cs = (const char *) s;

	/* we need something to compare */
	if (l_len == 0 || s_len == 0)
		return NULL;

	/* "s" must be smaller or equal to "l" */
	if (l_len < s_len)
		return NULL;

	/* special case where s_len == 1 */
	if (s_len == 1)
		return (void *) memchr(l, (int) *cs, l_len);

	/* the last position where its possible to find "s" in "l" */
	last = (char *) cl + l_len - s_len;

	for (cur = (char *) cl; cur <= last; cur++)
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
			return cur;

	return NULL;
}
