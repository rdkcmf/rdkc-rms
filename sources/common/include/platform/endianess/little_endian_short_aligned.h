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



#ifndef _LITTLE_ENDIAN_SHORT_ALIGNED_H
#define	_LITTLE_ENDIAN_SHORT_ALIGNED_H

//64 bit
#ifndef DONT_DEFINE_HTONLL
#define htonll(x) \
					((uint64_t)( \
                    ((((uint64_t)(x)) & 0xff00000000000000LL) >> 56) | \
                    ((((uint64_t)(x)) & 0x00ff000000000000LL) >> 40) | \
                    ((((uint64_t)(x)) & 0x0000ff0000000000LL) >> 24) | \
                    ((((uint64_t)(x)) & 0x000000ff00000000LL) >> 8) | \
                    ((((uint64_t)(x)) & 0x00000000ff000000LL) << 8) | \
                    ((((uint64_t)(x)) & 0x0000000000ff0000LL) << 24) | \
                    ((((uint64_t)(x)) & 0x000000000000ff00LL) << 40) | \
                    ((((uint64_t)(x)) & 0x00000000000000ffLL) << 56) \
					))
#define ntohll(x)   htonll(x)
#endif /* DONT_DEFINE_HTONLL */

//64 bit
#define EHTONLL(x) htonll(x)
#define ENTOHLL(x) ntohll(x)

//double
#ifdef APPLY_DOUBLE_QUIRK
#define EHTOND(hostDoubleVal,networkUI64Val) \
do { \
	networkUI64Val=EHTONLL((*((uint64_t *)(&(hostDoubleVal))))); \
	networkUI64Val=((networkUI64Val<<32)|(networkUI64Val>>32)); \
} while(0)

#define ENTOHD(networkUI64Val,hostDoubleVal) \
do {\
        uint64_t ___tempHostENTOHD=ENTOHLL(networkUI64Val); \
	___tempHostENTOHD=((___tempHostENTOHD<<32)|(___tempHostENTOHD>>32)); \
        hostDoubleVal=(double)(*((double *)&___tempHostENTOHD)); \
} while(0)
#else /* APPLY_DOUBLE_QUIRK */
#define EHTOND(hostDoubleVal,networkUI64Val) networkUI64Val=EHTONLL((*((uint64_t *)(&(hostDoubleVal)))))
#define ENTOHD(networkUI64Val,hostDoubleVal) \
do {\
	uint64_t ___tempHostENTOHD=ENTOHLL(networkUI64Val); \
	hostDoubleVal=(double)(*((double *)&___tempHostENTOHD)); \
} while(0)
#endif /* APPLY_DOUBLE_QUIRK */

//32 bit
#define EHTONL(x) htonl(x)
#define ENTOHL(x) ntohl(x)

//16 bit
#define EHTONS(x) htons(x)
#define ENTOHS(x) ntohs(x)

//adobe
#define EHTONA(x)    ((EHTONL(x)>>8)|((x) & 0xff000000))
#define ENTOHA(x)    EHTONA(x)

//64 bit pointer
#define EHTONLLP(pNetworkPointer,hostLongLongValue) \
do{ \
	((uint8_t *)(pNetworkPointer))[0]=(uint8_t)(((uint64_t)(hostLongLongValue))>>56); \
	((uint8_t *)(pNetworkPointer))[1]=(uint8_t)((((uint64_t)(hostLongLongValue))>>48)&0xff); \
	((uint8_t *)(pNetworkPointer))[2]=(uint8_t)((((uint64_t)(hostLongLongValue))>>40)&0xff); \
	((uint8_t *)(pNetworkPointer))[3]=(uint8_t)((((uint64_t)(hostLongLongValue))>>32)&0xff); \
	((uint8_t *)(pNetworkPointer))[4]=(uint8_t)((((uint64_t)(hostLongLongValue))>>24)&0xff); \
	((uint8_t *)(pNetworkPointer))[5]=(uint8_t)((((uint64_t)(hostLongLongValue))>>16)&0xff); \
	((uint8_t *)(pNetworkPointer))[6]=(uint8_t)((((uint64_t)(hostLongLongValue))>>8)&0xff); \
	((uint8_t *)(pNetworkPointer))[7]=(uint8_t)(((uint64_t)(hostLongLongValue))&0xff); \
}while(0)

#define ENTOHLLP(pNetworkPointer) \
	((uint64_t)( \
	(((uint64_t)(((uint8_t *)(pNetworkPointer))[0]))<<56)| \
	(((uint64_t)(((uint8_t *)(pNetworkPointer))[1]))<<48)| \
	(((uint64_t)(((uint8_t *)(pNetworkPointer))[2]))<<40)| \
	(((uint64_t)(((uint8_t *)(pNetworkPointer))[3]))<<32)| \
	(((uint64_t)(((uint8_t *)(pNetworkPointer))[4]))<<24)| \
	(((uint64_t)(((uint8_t *)(pNetworkPointer))[5]))<<16)| \
	(((uint64_t)(((uint8_t *)(pNetworkPointer))[6]))<<8)| \
	((uint64_t)(((uint8_t *)(pNetworkPointer))[7])) \
	))

//double pointer
#ifdef APPLY_DOUBLE_QUIRK
#define EHTONDP(hostDoubleVal,pNetworkPointer) \
do{ \
	uint64_t ___tempEHTONDP=*((uint64_t *)(&(hostDoubleVal))); \
	___tempEHTONDP=(___tempEHTONDP>>32)|(___tempEHTONDP<<32); \
	EHTONLLP(pNetworkPointer,___tempEHTONDP); \
} while(0)
#define ENTOHDP(pNetworkPointer,hostDoubleVal) \
do{ \
	uint64_t ___tempENTOHDP=ENTOHLLP(pNetworkPointer); \
	___tempENTOHDP=(___tempENTOHDP>>32)|(___tempENTOHDP<<32); \
	hostDoubleVal=*((double *)&___tempENTOHDP); \
}while(0)
#else
#define EHTONDP(hostDoubleVal,pNetworkPointer) \
do{ \
        uint64_t ___tempEHTONDP=*((uint64_t *)(&(hostDoubleVal))); \
        EHTONLLP(pNetworkPointer,___tempEHTONDP); \
} while(0)
#define ENTOHDP(pNetworkPointer,hostDoubleVal) \
do{ \
        uint64_t ___tempENTOHDP=ENTOHLLP(pNetworkPointer); \
        hostDoubleVal=*((double *)&___tempENTOHDP); \
}while(0)
#endif

//32 bit pointer
#define EHTONLP(pNetworkPointer,hostLongValue) \
do{ \
        ((uint8_t *)(pNetworkPointer))[0]=(uint8_t)(((uint32_t)(hostLongValue))>>24); \
        ((uint8_t *)(pNetworkPointer))[1]=(uint8_t)((((uint32_t)(hostLongValue))>>16)&0xff); \
        ((uint8_t *)(pNetworkPointer))[2]=(uint8_t)((((uint32_t)(hostLongValue))>>8)&0xff); \
        ((uint8_t *)(pNetworkPointer))[3]=(uint8_t)(((uint32_t)(hostLongValue))&0xff); \
}while(0)

#define ENTOHLP(pNetworkPointer) \
	((uint32_t)( \
        (((uint32_t)(((uint8_t *)(pNetworkPointer))[0]))<<24)| \
        (((uint32_t)(((uint8_t *)(pNetworkPointer))[1]))<<16)| \
        (((uint32_t)(((uint8_t *)(pNetworkPointer))[2]))<<8)| \
        ((uint32_t)(((uint8_t *)(pNetworkPointer))[3])) \
        ))


//16 bit pointer
#define EHTONSP(pNetworkPointer,hostShortValue) \
do{ \
        ((uint8_t *)(pNetworkPointer))[0]=(uint8_t)(((uint16_t)(hostShortValue))>>8); \
        ((uint8_t *)(pNetworkPointer))[1]=(uint8_t)(((uint16_t)(hostShortValue))&0xff); \
}while(0)

#define ENTOHSP(pNetworkPointer) \
	((uint16_t)( \
        (((uint16_t)(((uint8_t *)(pNetworkPointer))[0]))<<8)| \
        ((uint16_t)(((uint8_t *)(pNetworkPointer))[1])) \
        ))

//adobe pointer
#define EHTONAP(pNetworkPointer,hostAdobeValue) \
do{ \
        ((uint8_t *)(pNetworkPointer))[3]=(uint8_t)(((uint32_t)(hostAdobeValue))>>24); \
        ((uint8_t *)(pNetworkPointer))[0]=(uint8_t)((((uint32_t)(hostAdobeValue))>>16)&0xff); \
        ((uint8_t *)(pNetworkPointer))[1]=(uint8_t)((((uint32_t)(hostAdobeValue))>>8)&0xff); \
        ((uint8_t *)(pNetworkPointer))[2]=(uint8_t)(((uint32_t)(hostAdobeValue))&0xff); \
}while(0)

#define ENTOHAP(pNetworkPointer) \
	((uint32_t)( \
        (((uint32_t)(((uint8_t *)(pNetworkPointer))[3]))<<24)| \
        (((uint32_t)(((uint8_t *)(pNetworkPointer))[0]))<<16)| \
        (((uint32_t)(((uint8_t *)(pNetworkPointer))[1]))<<8)| \
        ((uint32_t)(((uint8_t *)(pNetworkPointer))[2])) \
        )) 

#endif	/* _LITTLE_ENDIAN_SHORT_ALIGNED_H */


