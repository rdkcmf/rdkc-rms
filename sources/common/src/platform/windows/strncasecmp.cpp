/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/
#ifdef WIN32

#include <string.h>
#include <ctype.h>
#include <stddef.h>

int strcasecmp(const char *s1, const char *s2)
{
	const unsigned char *us1 = (const unsigned char *)s1, *us2 = (const unsigned char *)s2;

	while (tolower(*us1) == tolower(*us2)) {
		if (*us1++ == '\0')
			return (0);
		us2++;
	}
	return (tolower(*us1) - tolower(*us2));
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    while(n > 0
	  && toupper((unsigned char)*s1) == toupper((unsigned char)*s2))
    {
	if(*s1 == '\0')
	    return 0;
	s1++;
	s2++;
	n--;
    }
    if(n == 0)
	return 0;
    return toupper((unsigned char)*s1) - toupper((unsigned char)*s2);
}

#endif /* WIN32 */


