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

#ifdef WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#include "application/webserverapplication.h"
#include "protocols/protocolmanager.h"
#include "protocols/timer/jobstimerprotocol.h"
#include "protocols/defaultprotocolfactory.h"
#include "eventlogger/eventlogger.h"
#include "common.h"
#include "application/helper.h"
#include "application/refcountedcacherule.h"
#ifdef WIN32
#include <strsafe.h>
#endif
#include "application/phpengine.h"
#include "application/mutex.h"
#include "application/clientapplicationmanager.h"

using namespace webserver;
using namespace app_rdkcrouter;

#ifdef WIN32
#define LIBMICROHTTPDLIBRARY        "libmicrohttpd-12.dll"
#endif

#define POSTBUFFERSIZE              1024
#define NOCONNECTIONTIMEOUT         0
#define DEFAULTCONNECTIONHTTPPORT   8080
#define DEFAULTCONNECTIONHTTPSPORT  443
#define DEFAULTCONNECTIONPAGESIZE   32*1024
#define DEFAULTMEDIAFILEDOWNLOADTIMEOUT	30

#define NOCONNECTIONLIMIT           0
#define DEFAULTTHREADPOOLSIZE       1
#define SSLMODE_ALWAYS              "always"
#define SSLMODE_AUTO                "auto"
#define SSLMODE_DISABLED            "disabled"
#define DEFAULTSSLMODE              SSLMODE_ALWAYS
#define PHPEXT                      ".php"
#define DEFAULTCHUNKLENGTH          10
#define ORPHANSESSIONTIMER          1
#define ORPHANSESSIONCLEANUPPERIOD  6
#define LINEDELIMITER               "\r\n"
#define FOLDERSEPARATORS            "\\/"
#define FOLDERSEPARATORWIN          "\\"
#define FOLDERSEPARATORLIN          "/"
#define WEBUIFOLDER                 "RMS_Web_UI"
#define INSTALLLICENSEFILE          WEBUIFOLDER"/install_license.php"
#define INDEXFILE                   "index.php"

//MANIFEST TYPES
#define STREAMTYPE_HLS              "hls"
#define STREAMTYPE_HDS              "hds"
#define STREAMTYPE_MSS              "mss"
#define STREAMTYPE_DASH             "dash"

//VOD INDICATORS
#define HLS_END_INDICATOR           "#EXT-X-ENDLIST"
#define HDS_END_INDICATOR           "<streamType>recorded</streamType>"
#define MSS_END_INDICATOR           "IsLive=\"FALSE\""

//ALIAS INDICATORS
#define ALIAS_BEGIN_INDICATOR       "[[["
#define ALIAS_END_INDICATOR         "]]]"

#define HLS_SEGMENT_INDICATOR       "segment"

#define MANIFEST_GROUPTARGETFOLDER  "groupTargetFolder"
#define MANIFEST_MASTERPLAYLISTPATH "masterPlaylistPath"
#define MANIFEST_NAME               "manifestName"

//configuration file entries
#define CONFIG_PORT                 "port"
#define CONFIG_BINDTOIP             "bindToIP"
#define CONFIG_SSLMODE              "sslMode"
#define CONFIG_MAXMEMSIZEPERCONN    "maxMemSizePerConnection"
#define CONFIG_MAXCONCURCONN        "maxConcurrentConnections"
#define CONFIG_CONNTIMEOUT          "connectionTimeout"
#define CONFIG_MAXCONCURCONNSAMEIP  "maxConcurrentConnectionsSameIP"
#define CONFIG_THREADPOOLSIZE       "threadPoolSize"
#define CONFIG_ENABLEIPFILTER       "enableIPFilter"
#define CONFIG_WHITELISTFILE        "whitelistFile"
#define CONFIG_BLACKLISTFILE        "blacklistFile"
#define CONFIG_SSLKEYFILE           "sslKeyFile"
#define CONFIG_SSLCERTFILE          "sslCertFile"
#define CONFIG_USEIPV6              "useIPV6"
#define CONFIG_ENABLECACHE          "enableCache"
#define CONFIG_HASGROUPNAMEALIASES  "hasGroupNameAliases"
#define CONFIG_ENABLERANGEREQUESTS	"enableRangeRequests"
#define CONFIG_WEBROOTFOLDER        "webRootFolder"
#define CONFIG_WEBUILICENSEONLY     "webUilicenseOnly"
#define CONFIG_CACHESIZE            "cacheSize"
#define CONFIG_MEDIAFILEDOWNLOADTIMEOUT  "mediaFileDownloadTimeout"
#define CONFIG_SUPPORTEDMIMETYPES   "supportedMimeTypes"
#define CONFIG_MIME_EXTENSIONS      "extensions"
#define CONFIG_MIME_TYPE            "mimeType"
#define CONFIG_MIME_STREAMTYPE      "streamType"
#define CONFIG_MIME_ISMANIFEST      "isManifest"
#define CONFIG_INCRESPHDRS          "includeResponseHeaders"
#define CONFIG_AUTHENTICATION       "auth"
#define CONFIG_AUTH_DOMAIN			"domain"
#define CONFIG_AUTH_DIGESTFILE		"digestFile"
#define CONFIG_AUTH_ENABLE			"enable"
#define CONFIG_RESPHDR_HEADER       "header"
#define CONFIG_RESPHDR_CONTENT      "content"
#define CONFIG_RESPHDR_OVERRIDE     "override"
#define CONFIG_APIPROXY             "apiProxy"
#define CONFIG_APIPROXY_PSEUDO      "pseudoDomain"
#define CONFIG_APIPROXY_ADDRESS     "address"
#define CONFIG_APIPROXY_PORT        "port"
#define CONFIG_APIPROXY_AUTH        "authentication"
#define CONFIG_APIPROXY_USERNAME    "userName"
#define CONFIG_APIPROXY_PASSWORD    "password"

#define APIPROXY_BASICAUTH          "basic"
#define APIPROXY_DEFAULTPORT        7777
#define APIPROXY_DEFAULTADDRESS     "127.0.0.1"
#define APIPROXY_DEFAULTPSEUDO      "pseudoDomain"

#ifdef WIN32 
#define FILESEP FOLDERSEPARATORWIN
#else
#define FILESEP FOLDERSEPARATORLIN
#endif
#define WEBSERVER_DEBUGMODE
//#define CACHE_PLAYLIST

#define FOR_MAP_CONST(m,k,v,i) for(map< k , v >::const_iterator i=(m).begin();i!=(m).end();i++)
#define VECTOR_HAS(c, v) ((bool)(find(c.begin(), c.end(), v)!=c.end()))

uint16_t WebServerApplication::Metric::_tps = 0;
uint16_t WebServerApplication::Metric::_activeThreads = 0;

WebServerApplication::WebServerApplication(Variant &configuration)
: BaseClientApplication(configuration),
_maxMemSizePerConnection(DEFAULTCONNECTIONPAGESIZE), _useIPV6(false), _enableIPFilter(false),
_enableCache(false), _mediaFileDownloadTimeout(DEFAULTMEDIAFILEDOWNLOADTIMEOUT), _port(0), _hasGroupNameAliases(false), _enableRangeRequests(false),
_webUiLicenseOnly(false), _server(NULL), _keySSL(NULL), _certSSL(NULL),
_memCache(new MemCache(new RefCountedCacheRule()))
#ifdef WIN32
, _hMod(NULL)
#endif
, _sessionsMutex(new Mutex), _streamingSessionsMutex(new Mutex), 
_groupNameAliasesMutex(new Mutex), _temporaryFilesMutex(new Mutex),
_evtLogMutex(new Mutex), _connsMutex(new Mutex),
_pVariantHandler(NULL), _pAcceptor(NULL), _newVariantHandler(false), _enableAPIProxy(false), _useWhiteList(false), _useBlackList(false) {
#ifdef WIN32
	_parentHandle = NULL;
#endif
}

WebServerApplication::~WebServerApplication() {
    Stop();
	DeleteTemporaryFiles();
    if (_keySSL)
        delete[] _keySSL;
    if (_certSSL)
        delete[] _certSSL;
    if (_memCache)
        delete _memCache;
#ifdef WIN32
    if (_hMod)
        ::FreeLibrary(_hMod);
#endif

    FOR_MAP_CONST(_sessions, int, Session *, i) {
        Session *ssn = MAP_VAL(i);
        if (ssn->_postProc != NULL)
            DestroyPostProcessor(&ssn->_postProc);
    }
    if (_sessionsMutex)
        delete _sessionsMutex;
    if (_streamingSessionsMutex)
        delete _streamingSessionsMutex;
    if (_groupNameAliasesMutex)
        delete _groupNameAliasesMutex;
	if (_temporaryFilesMutex)
		delete _temporaryFilesMutex;
    if (_evtLogMutex)
        delete _evtLogMutex;
    if (_connsMutex)
        delete _connsMutex;

    ClientApplicationManager::UnRegisterApplication(this);
    
    UnRegisterAppProtocolHandler(PT_BIN_VAR);
    if (_pVariantHandler != NULL) {
        delete _pVariantHandler;
        _pVariantHandler = NULL;
    }

    if (_pAcceptor != NULL)
        delete _pAcceptor;
#ifdef WIN32
	if (_parentHandle != NULL) {
		CloseHandle(_parentHandle);
	}
#endif
}

bool WebServerApplication::GetNodeValue(TiXmlDocument &doc, string const& node, string &value) {

	vector<string> elems;
	split(node, "/", elems);
	if (elems.size() < 1) {
		return false;
	}
	TiXmlElement *currentElement = doc.RootElement();
	for (size_t i = 0; i < elems.size(); ++i) {
		if (currentElement == NULL) {
			return false;
		}
		string elem = elems[i];
		if (i == elems.size() - 1) {	//last one: the attribute
			currentElement->QueryValueAttribute(elem, &value);
			break;
		} else {
			currentElement = currentElement->FirstChildElement(elem);
		}
	}
	return true;
}

string WebServerApplication::GetGroupAndLocalStreamNameFromPath(string const& filePath) {
    //<root_folder><groupname><localstreamname><artifact>
    string withoutRootFolder = filePath.substr(_webRootFolder.size() + 1); //remove root folder
    if (!withoutRootFolder.empty()) {
        size_t pos = withoutRootFolder.find_last_of(FOLDERSEPARATORS);
        if (string::npos != pos)
            return withoutRootFolder.substr(0, pos);
    }
    return "";
}

bool WebServerApplication::isVOD(File *file, SupportedMimeType smt) {
    if (file) {
        string content;
        if (file->ReadAll(content)) {
            if ((smt._streamType == STREAMTYPE_HLS && string::npos != content.find(HLS_END_INDICATOR)) ||
                    //(smt._manifest == MANIFEST_DASH && string::npos != content.find(DASH_END_INDICATOR)) ||
                    (smt._streamType == STREAMTYPE_HDS && string::npos != content.find(HDS_END_INDICATOR)) ||
                    (smt._streamType == STREAMTYPE_MSS && string::npos != content.find(MSS_END_INDICATOR)))
                return true;
        }
    }
    return false;
}

ssize_t WebServerApplication::FileContentReaderCallback(void *cls, uint64_t pos,
        char *buf, size_t max) {
    ContentReaderContext *crc = (ContentReaderContext *) cls;
    if (crc) {
        File *file = (File *) crc->_reader;
        if (file && file->IsOpen()) {
            if (crc->_begin == -1) {
                crc->_begin = getutctime();
			}
			uint64_t newPos = pos + crc->_fileOffset;	//range request support
            if (file->SeekTo(newPos)) {
				uint64_t readSize = min((uint64_t)max, file->Size() - newPos);
                bool readSuccess = file->ReadBuffer((uint8_t *) buf, readSize);
				if (!readSuccess) {
                    return 0;
				}
                if (file->IsEOF())
                    return (ssize_t)(file->Size() - newPos);
                return max;
            }
        }
    }
    return 0;
}

void WebServerApplication::FileFreeResourceCallback(void *cls) {
    ContentReaderContext *crc = (ContentReaderContext *) cls;
    if (crc) {
        File *file = (File *) crc->_reader;
        if (file) {
            crc->_end = getutctime();
            string filePath = file->GetPath();
            if (!crc->_isTemporaryFile && crc->_app->_enableCache) {
                string grpls = crc->_app->GetGroupAndLocalStreamNameFromPath(filePath);
                if (find(crc->_app->_grplsNameForCache.begin(), crc->_app->_grplsNameForCache.end(), grpls) !=
                        crc->_app->_grplsNameForCache.end()) {
                    crc->_app->_memCache->Store(filePath);
                } else {
                    bool isSupportedMimeType = false;
                    bool isManifest = false;
                    bool isNotLive = false;
                    //we only cache if a manifest indicates not live streaming 
                    size_t pos = filePath.find_last_of(".");
                    if (string::npos != pos) {
                        string fileExt = filePath.substr(pos + 1);

                        FOR_MAP_CONST(crc->_app->_mimes, string, SupportedMimeType, i) {
                            if (fileExt == MAP_KEY(i)) {
                                isSupportedMimeType = true;
                                SupportedMimeType smt = MAP_VAL(i);
                                if (smt._streamType == STREAMTYPE_HLS || smt._streamType == STREAMTYPE_HDS || smt._streamType == STREAMTYPE_MSS || smt._streamType == STREAMTYPE_DASH) {
                                    isManifest = true;
#ifdef CACHE_PLAYLIST
                                    bool vod = isVOD(file, smt);
#else
                                    bool vod = false;
#endif
                                    if (vod)
                                        ADD_VECTOR_END(crc->_app->_grplsNameForCache, grpls);
                                    else
                                        crc->_app->_grplsNameForCache.erase(remove(crc->_app->_grplsNameForCache.begin(),
                                            crc->_app->_grplsNameForCache.end(), grpls), crc->_app->_grplsNameForCache.end());
                                    isNotLive = vod;
                                }
                                break;
                            }
                        }
                    }
                    if (!isSupportedMimeType || !isManifest || isNotLive)
                        crc->_app->_memCache->Store(filePath);
                }
            }
            delete file;
            if (crc->_isTemporaryFile) { //do not cache
                AutoMutex am(crc->_app->_temporaryFilesMutex);
				crc->_app->_temporaryFiles.push_back(filePath);
            }
            if (crc->_isMediaFile) {

                AutoMutex am(crc->_app->_evtLogMutex);
				
                Variant downloadInfo;
				downloadInfo["clientIP"] = crc->_ipAddress;
				downloadInfo["mediaFile"] = filePath;
				downloadInfo["startTime"] = Helper::ConvertToLocalSystemTime(crc->_begin);
				downloadInfo["stopTime"] = Helper::ConvertToLocalSystemTime(crc->_end);
				downloadInfo["end"] = (uint32_t)crc->_end;
				downloadInfo["elapsed"] = (uint32_t)(crc->_end - crc->_begin);
				downloadInfo["type"] = "mediaFileDownloaded";
				if (!crc->_isRange) {
					//send event
					crc->_app->_pVariantHandler->SendEventLog(downloadInfo);
					DEBUG("Media File Downloaded: (%s)", STR(filePath));
				} else {
					//update
					crc->_app->_mediaFileLastAccess[crc->_urlFile] = downloadInfo;
				}
            }
        }
        delete crc;
    }
}

ssize_t WebServerApplication::CacheContentReaderCallback(void *cls, uint64_t pos, char *buf, size_t max) {
    ContentReaderContext *crc = (ContentReaderContext *) cls;
    if (crc) {
        MemCache::StreamInfo *si = (MemCache::StreamInfo *)crc->_reader;
        if (si) {
            memcpy(buf, (char *) (si->_mappedView) + pos, max);
            if (pos + max > si->_fileSize)
                return (ssize_t)(si->_fileSize - pos);
            return max;
        }
    }
    return 0;
}

void WebServerApplication::CacheFreeResourceCallback(void *cls) {
    ContentReaderContext *crc = (ContentReaderContext *) cls;
    if (crc) {
        MemCache::StreamInfo *si = (MemCache::StreamInfo *)crc->_reader;
        if (si) {
            Helper::AtomicDecrement(&si->_refCtr);
            crc->_end = getutctime();
            delete crc;
        }
    }
}

int WebServerApplication::KeyValuePairsCallback(void *cls, enum MHD_ValueKind /*kind*/, const char *key, const char *value) {
    map<string, string> *kvPairs = (map<string, string> *)cls;
    if (key && value && kvPairs) {
        (*kvPairs)[key] = value;
        return MHD_YES;
    }
    return MHD_NO;
}

int WebServerApplication::PostIteratorCallback(void *cls, enum MHD_ValueKind /*kind*/, const char *key, const char *filename, const char * /*content_type*/,
        const char * /*transfer_encoding*/, const char *data, uint64_t /*off*/, size_t size) {
    Session *ssn = (Session *) cls;
    string path;
    if (filename) {
        File &upload = ssn->_upload;
        if (!upload.IsOpen()) {
            path = Helper::GetTemporaryDirectory() + FILESEP + string(filename);
            upload.Initialize(path, FILE_OPEN_MODE_TRUNCATE);
            if (!upload.IsOpen())
                return MHD_NO;
        }
        if (size > 0) {
            if (!upload.WriteBuffer((uint8_t *) data, size))
                return MHD_NO;
        }
    }
    map<string, string>& kvPairs = ssn->_postGetParams;
    if (!MAP_HAS1(kvPairs, key)) {
        if (filename != NULL) {
            if (!MAP_HAS1(kvPairs, "uploadedFile"))
                kvPairs["uploadedFile"] = path;
        } else
            kvPairs[key] = data;
    }
    return MHD_YES;
}

string WebServerApplication::GetSessionFilePath(string &urlFile, bool returnAlias) {
    size_t posBegin = urlFile.find(ALIAS_BEGIN_INDICATOR);
    if (posBegin != string::npos) { 
        size_t posEnd = urlFile.find(ALIAS_END_INDICATOR);
        if (posEnd != string::npos) {
            if (posBegin < posEnd) {
                string urlTemp = urlFile;
                string urlFile1 = urlFile.substr(0, posBegin);
                static size_t const beginIndicatorSize = string(ALIAS_BEGIN_INDICATOR).size();
                static size_t const endIndicatorSize = string(ALIAS_END_INDICATOR).size();
                string alias =  urlTemp.substr(posBegin + beginIndicatorSize, posEnd - endIndicatorSize - posBegin);
                string urlFile2;
                if (urlTemp.size() > (posEnd + endIndicatorSize)) {
                    urlFile2 = urlTemp.substr(posEnd + endIndicatorSize + 1);
                }
                urlFile = urlFile1 + urlFile2;
				//Handle double alias in path; HLSv4 paths depends on player; VLC relative, iOS absolute
				//Without this, iOS fails.
				size_t posBegin1 = urlFile.find(ALIAS_BEGIN_INDICATOR);		
				if (posBegin1 != string::npos) { 
					size_t posEnd1 = urlFile.find(ALIAS_END_INDICATOR);
					if (posEnd1 != string::npos) {
						string urlFile3 = urlFile.substr(0, posBegin1);
						string urlFile4 = urlFile.substr(posEnd1 + endIndicatorSize + 1);
						size_t posDummy = urlFile4.find_first_of(FOLDERSEPARATORS);
						if (posDummy != string::npos) {
							urlFile4 = urlFile4.substr(posDummy + 1);
						}
						urlFile = urlFile3 + urlFile4;
					}
				}
                if (returnAlias) {
                    return alias;
                }
            }
        }
    }
    return "";
}

bool WebServerApplication::IsProxyCall(string const& url, 
struct MHD_Connection *connection, string &outputPath, bool &authFail) {
    string pseudoDomain, command;
    size_t e = url.find(FILESEP);
    if (e != string::npos) {
        pseudoDomain = url.substr(0, e);
        command = url.substr(e + 1);
        if (pseudoDomain.compare(STR(_apiProxy._pseudoDomain)) == 0) {
            char *pass = NULL;
            char *user = BasicAuthGetUsernamePassword(connection, &pass);
            int fail = 
                (0 == strcmp ("basic", STR(_apiProxy._authentication))) && 
                (((user == NULL) || 
                (0 != strcmp (user, STR(_apiProxy._userName))) || 
                (0 != strcmp (pass, STR(_apiProxy._password)))));
#ifndef DEBUG
            if (user != NULL) { 
                free (user);
            }
            if (pass != NULL) {
                free (pass);
            }
#endif
            authFail = true;
            if (!fail) {
                struct sockaddr_in servAddr;
                SOCKET sockId;
                uint32_t const msgSize = 1*1024;
                char message[msgSize] = {0};
                int32_t msgLen;
                string marker = "\r\n\r\n"; 
                string request = format("GET /%s HTTP/1.1", STR(command));
                if((sockId = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                    return true;
                }

                memset(&servAddr, 0, sizeof(servAddr));

                servAddr.sin_addr.s_addr = inet_addr(
                    STR(_apiProxy._address));
                servAddr.sin_port = htons(_apiProxy._port);
                servAddr.sin_family = AF_INET;

                if(connect(sockId, (struct sockaddr *)&servAddr, 
                    sizeof(servAddr)) != 0) {
                        CLOSE_SOCKET(sockId);
                        return true;
                }
                string poststr;
                string req = request + marker;
#ifdef WIN32
                int32_t written = send((int32_t)sockId, STR(req), 
                    (uint32_t)req.size(), 0);
#else
                int32_t written = WRITE_FD((int32_t)sockId, STR(req), 
                    (uint32_t)req.size());            
#endif
                if (written > 0) {
                    while(true) {
#ifdef WIN32
                        msgLen = recv((int32_t)sockId, message, 
                            msgSize - 1, 0);
#else
                        msgLen = READ_FD((int32_t)sockId, message, 
                            msgSize - 1);
#endif
                        if (msgLen == 0) {
                            break;
                        } else if (msgLen != -1) {
                            message[msgLen] = '\0';
                            poststr += message;
                        } 
                    }
                    size_t startPoint = poststr.find(marker);
                    if (startPoint != string::npos) {
                        poststr = poststr.substr(startPoint + 
                            marker.size());
                    }
                }
                CLOSE_SOCKET(sockId);
                outputPath = _tempFolder;
                GenerateRandomOutputFile(outputPath, ".proxy");
                File fp;
                if (!fp.Initialize(outputPath, FILE_OPEN_MODE_TRUNCATE)) {
                    AutoMutex am(_temporaryFilesMutex);
                    _temporaryFiles.push_back(outputPath);
                    return true;
                }
                fp.WriteBuffer((uint8_t *) STR(poststr), poststr.length());
            } else {
                authFail = true;
                return true;
            }
            authFail = false;
            return true;
        } else {
            authFail = false;
        }
    }
    return false;
}

bool WebServerApplication::Authenticate(string const& url, struct MHD_Connection *connection, Authentication auth, string &userName) {
	userName = "";
	string domain;
	size_t e = url.find(FILESEP);
	if (e != string::npos) {
		domain = url.substr(0, e);
		if (domain.compare(STR(auth._domain)) == 0 && auth._type.compare("basic") == 0) {
			char *pass = NULL;
			char *user = BasicAuthGetUsernamePassword(connection, &pass);
			bool authFail = true;
			if (user) {
				string userTmp = user;
				auth._users = LoadUsersFromDigestFile(auth._digestFile);
				if (MAP_HAS1(auth._users, userTmp)) {
					string userPwd = auth._users[userTmp];
					string hashMD5 = md5(pass, true);
					if (userPwd.compare(hashMD5) == 0) {
						userName = user;
						authFail = false;
					}
				}
			}
#ifndef DEBUG
			if (user != NULL) {
				free(user);
			}
			if (pass != NULL) {
				free(pass);
			}
#endif
			return !authFail;
		}
	}
	return true;
}

int WebServerApplication::RequestHandlerCallback(void *cls, struct MHD_Connection *connection,
        char const *url, char const *method, char const * /*version*/, char const *upload_data,
        size_t *upload_data_size, void **con_cls) {
    struct MHD_Response *response;
    int ret;
	uint32_t statusCode = MHD_HTTP_OK;
	string contentRange;

    Metric metric;

    WebServerApplication *hs = (WebServerApplication *) cls;
    if (hs == NULL)
        return MHD_NO;

    string clientIPAddress;
    bool isGet = (0 == strcmp(method, MHD_HTTP_METHOD_GET));
    bool isPost = (0 == strcmp(method, MHD_HTTP_METHOD_POST));
    if (!(isGet || isPost))
        return MHD_NO; /* unexpected method */

    MHD_ConnectionInfo const *infoConn = hs->GetConnectionInfo(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    if (infoConn == NULL)
        return MHD_NO;
    uint16_t port;
    clientIPAddress = Helper::GetIPAddress(infoConn->client_addr, port, 
        hs->_useIPV6);
    AutoMutex conns(hs->_connsMutex);
    if (!VECTOR_HAS(hs->_connections, clientIPAddress))
        return hs->GenerateResponse(connection, MHD_HTTP_FORBIDDEN);

    Session *ssn = NULL;
    if (*con_cls == NULL) {
        /* do never respond on first call */
        MHD_ConnectionInfo const *infoSockCtx = hs->GetConnectionInfo(connection, MHD_CONNECTION_INFO_SOCKET_CONTEXT);
        if (infoSockCtx) {
            ssn = reinterpret_cast<Session *>(infoSockCtx->socket_context);
			*con_cls = reinterpret_cast<void *>(ssn);
            if (isPost) {
                ssn->_postProc = hs->CreatePostProcessor(connection, POSTBUFFERSIZE,
                        &WebServerApplication::PostIteratorCallback, ssn);
                if (NULL == ssn->_postProc) {   
                    ssn->_postData = new stringstream;
                }
            }
        }
        return MHD_YES;
    } else {
        ssn = reinterpret_cast<Session *>(*con_cls);
    }

    if (isPost) {
        /* evaluate POST data */
        if (ssn->_postProc) {
            hs->ProcessPost(ssn->_postProc, upload_data, *upload_data_size);
            if (0 != *upload_data_size) {
                *upload_data_size = 0;
                return MHD_YES;
            }
            /* done with POST data, serve response */
            hs->DestroyPostProcessor(&ssn->_postProc);
        } else {    //we have a RAW POST
            if (*upload_data_size != 0) {
                //receive post data
                *ssn->_postData << string(upload_data, *upload_data_size);
                *upload_data_size = 0;
                return MHD_YES;
            } else {
                //receive complete, process postData
                string data = ssn->_postData->str();
                delete ssn->_postData;
                ssn->_postData = NULL;
                ssn->_postGetParams[""]=data;   //blank key means RAW POST
            }
        }
    }
    if (ssn->_postGetParams.empty())
        hs->GetConnectionValues(connection, isGet ? MHD_GET_ARGUMENT_KIND : MHD_POSTDATA_KIND,
            &WebServerApplication::KeyValuePairsCallback, &ssn->_postGetParams);

    ContentReaderContext *crc = new ContentReaderContext;
    if (crc == NULL) {
        return MHD_NO;
    }
    crc->_app = hs;
    crc->_isTemporaryFile = false;
    crc->_isMediaFile = false;
    crc->_ssn = ssn;
	crc->_begin = -1;

#ifdef WIN32
    struct _stat64 buf;
#else
    struct stat64 buf;
#endif
    File *file = NULL;
    string urlFile = &url[1];
#ifdef WIN32
    replace(urlFile, "/", FILESEP);
    replace(urlFile, "%2F", FILESEP);
#else
    replace(urlFile, "\\", FILESEP);
    replace(urlFile, "%2F", FILESEP);   
#endif
    replace(urlFile, "%5B", "[");
    replace(urlFile, "%5D", "]");
    string alias = hs->GetSessionFilePath(urlFile, true);
    string filePath = hs->_webRootFolder + urlFile;
	string tempFilePath = normalizePath(filePath, "");
	if (!tempFilePath.empty()) {
		filePath = tempFilePath;
	}
    string uTgf, uName, uExt;
    Helper::SplitFileName(filePath, uTgf, uName, uExt);
    string targetFileName = uName + "." + uExt; 
#ifdef WIN32    
    replace(filePath, "/", FILESEP);
#endif
    bool authFail = false;
    bool isProxyCall = hs->_enableAPIProxy && hs->IsProxyCall(urlFile, connection, filePath, authFail);
    if (authFail) {
        response = hs->CreateResponse(MHD_HTTP_UNAUTHORIZED, false);
		if (response == NULL) {
			delete crc;
			return MHD_NO;
		}
        ret = hs->QueueBasicAuthFailResponse(connection, "RDKC Web Server", response);
        hs->DestroyResponse(response);
		delete crc; 
		return ret;
    }

    bool webUiRoot = false;
    size_t pos = urlFile.find_last_of(FOLDERSEPARATORS);
    if (urlFile.empty()) {  //redirect to RMS_Web_UI
        map<string, string> header;
        hs->GetConnectionValues(connection, MHD_HEADER_KIND, &WebServerApplication::KeyValuePairsCallback, &header);
        if (MAP_HAS1(header, MHD_HTTP_HEADER_HOST)) {   //remote, but just in case
            char const *dummy = "";
			response = hs->CreateResponseFromBuffer(strlen(dummy), (void *)dummy, MHD_RESPMEM_PERSISTENT);
            string forwardTo = format("http%s://%s/%s/%s", hs->_sslMode == SSLMODE_DISABLED ? "" : "s", 
                STR(header[MHD_HTTP_HEADER_HOST]), WEBUIFOLDER, INDEXFILE);
            hs->AddResponseHeader(response, MHD_HTTP_HEADER_LOCATION, STR(forwardTo));
            ret = hs->QueueResponse(connection, MHD_HTTP_FOUND, response);
            hs->DestroyResponse(response);
			delete crc; 
			return ret;
        }
    } else if (!isProxyCall && pos == urlFile.length() - 1) {
        filePath += INDEXFILE;
        urlFile += INDEXFILE;
        webUiRoot = true;
    }

	map<string, string> header;
	hs->GetConnectionValues(connection, MHD_HEADER_KIND, &WebServerApplication::KeyValuePairsCallback,
		&header);

	string authUser;
	FOR_MAP_CONST(hs->_auths, string, Authentication, iter) {
		Authentication auth = MAP_VAL(iter);
		if (auth._enable) {
			bool authenticated = hs->Authenticate(urlFile, connection, auth, authUser);
			if (!authenticated && !MAP_HAS1(header, MHD_HTTP_HEADER_REFERER)) {
				response = hs->CreateResponse(MHD_HTTP_UNAUTHORIZED, false);
				if (response == NULL) {
					delete crc;
					return MHD_NO;
				}
				ret = hs->QueueBasicAuthFailResponse(connection, "RDKC Web Server", response);
				hs->DestroyResponse(response);
				delete crc; 
				return ret;
			}
			//return the authenticated user
			ssn->_postGetParams["user"] = authUser;
		}
	}

	bool applyRangeRequest = hs->_enableRangeRequests && MAP_HAS1(header, MHD_HTTP_HEADER_RANGE);
    if (hs->_webUiLicenseOnly && Helper::GetFileInfo(filePath, buf) && (urlFile.compare(INSTALLLICENSEFILE) != 0)) {
        bool referred = false;
        referred = (!webUiRoot && MAP_HAS1(header, MHD_HTTP_HEADER_REFERER));
        if (!referred) {
            FATAL("Only WebUI license page is allowed: urlFile: %s", STR(urlFile));
			delete crc; 
			return hs->GenerateResponse(connection, MHD_HTTP_FORBIDDEN, true);
        }
    }   
	MemCache::StreamInfo *si = NULL;
    map<string, string> derivedResponseHeader;
    if ((!hs->_enableCache || ((si = hs->_memCache->Retrieve(filePath)) == NULL))) { //cache disabled or file not cached
        bool validHTTPStream = false;
        bool isStreamPath = false;
        string originalFile;
        bool isTempFile = false;
        if (hs->_hasGroupNameAliases) {
			AutoMutex am(hs->_groupNameAliasesMutex);
            if (MAP_HAS1(hs->_groupNameAliases, urlFile)) { //urlFile passed is an alias
                if (!hs->IsAliasAvailable(clientIPAddress, urlFile)) {
                    DEBUG("Alias Is Unavailable: %s", STR(urlFile));
					delete crc; 
					return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
                }
                string groupName = hs->_groupNameAliases[urlFile];
                string streamType;
                vector<string> localStreamNames;
                filePath = hs->GetPlayListByGroupName(groupName, localStreamNames, streamType, targetFileName, true);
				if (!streamType.empty()) {
					string userAgent;
					if (MAP_HAS1(header, MHD_HTTP_HEADER_USER_AGENT)) {
						userAgent = header[MHD_HTTP_HEADER_USER_AGENT];
					} else if (MAP_HAS1(header, "user-agent")) {		//mx player exhibits this
						userAgent = header["user-agent"];
					} 
					string targetFolder, name, ext;
					Helper::SplitFileName(filePath, targetFolder, name, ext);
					//alias = format("%s:%s:%s,%s", STR(clientIPAddress), STR(userAgent), STR(targetFolder), STR(urlFile));
					alias = format("%s:%s,%s", STR(clientIPAddress), STR(targetFolder), STR(urlFile));
					string tempFile;
					hs->GenerateRandomOutputFile(tempFile, "." + ext);
					originalFile = filePath;
					hs->CopyFile(filePath, tempFile);
					filePath = tempFile;
					isTempFile = true;
					isStreamPath = true;
					validHTTPStream = true;
				}
            } else {
				string userAgent;
				if (MAP_HAS1(header, MHD_HTTP_HEADER_USER_AGENT)) {
					userAgent = header[MHD_HTTP_HEADER_USER_AGENT];
				} else if (MAP_HAS1(header, "user-agent")) {		//mx player exhibits this
					userAgent = header["user-agent"];
				}
				//string key = format("%s:%s", STR(clientIPAddress), STR(userAgent));
				string key = format("%s", STR(clientIPAddress));
				bool found = false;
				FOR_MAP_CONST(hs->_streamingSessions, string, StreamingSession, iter) {
					if (MAP_KEY(iter).find(key) == 0) {
						StreamingSession ssn = MAP_VAL(iter);
						vector<string> files;
						string targetFolder = ssn._targetFolder + FILESEP;
						Helper::ListFiles(targetFolder, files);	
						if (!files.empty()) {
							FOR_VECTOR_ITERATOR(string, files, iterFiles) {
								string file = VECTOR_VAL(iterFiles);
								size_t pos = file.find(urlFile);
								if (pos != string::npos) {
									size_t diff =  file.size() - urlFile.size();
									if (pos == diff) {
										filePath = file;
										found = true;
										break;
									}
								}
							}
							if (found) {
								alias = MAP_KEY(iter);
								break;
							}
						}
					}
				}
            }
        } else {    //not GNA
			string userAgent;
			if (MAP_HAS1(header, MHD_HTTP_HEADER_USER_AGENT)) {
				userAgent = header[MHD_HTTP_HEADER_USER_AGENT];
			} else if (MAP_HAS1(header, "user-agent")) {		//mx player exhibits this
				userAgent = header["user-agent"];
			} 
			string targetFolder, name, ext;
			Helper::SplitFileName(filePath, targetFolder, name, ext);
			alias = format("%s:%s:%s", STR(clientIPAddress), STR(targetFolder), STR(userAgent));
			if (MAP_HAS1(hs->_mimes, ext)) {
				if (hs->_mimes[ext]._isManifest) {
					string tempFile;
					hs->GenerateRandomOutputFile(tempFile, "." + ext);
					hs->CopyFile(filePath, tempFile);
					originalFile = filePath;
					filePath = tempFile;
					isTempFile = true;
				}
			}
			isStreamPath = true;
		}

		if (filePath.empty()) {
			delete crc; 
			return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
		}
        else if (!Helper::GetFileInfo(filePath, buf)) { //file does not exist
            INFO("Requesting Invalid File: %s", STR(filePath));
			delete crc; 
			return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
        } else {
            string folder, name, ext;
            Helper::SplitFileName(urlFile, folder, name, ext);
            if (string(".") + ext == PHPEXT) { //if file is php, pass to php interpreter
                string outputPHPFile = hs->_tempFolder;
                //make sure that all uploaded files are closed prior to passing it to php
                if (ssn->_upload.IsOpen())
                    ssn->_upload.Close();
                if (!hs->RunPHPInterpreter(filePath, outputPHPFile, isGet,
                        ssn->_postGetParams, derivedResponseHeader, 
                        hs->GetConfiguration()["program"], clientIPAddress)) {
					delete crc; 
					return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
                }
                isTempFile = true;
                filePath = outputPHPFile;
                if (!Helper::GetFileInfo(filePath, buf)) {
					delete crc; 
					return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
				}
            } else if (isStreamPath && hs->_hasGroupNameAliases && !validHTTPStream) { //attempt to access the playlist via direct folder access
				delete crc; 
				return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
            } else if (hs->IsStreamingFile(filePath)) {
				bool isManifest = false;
				if (!hs->StreamFile(isTempFile ? originalFile : filePath, *ssn, 
                    alias.empty() ? urlFile : alias, !alias.empty(), targetFileName, isManifest)) { //if we cannot stream the file, send a 404
                    if (hs->IsMediaFile(filePath)) {
						crc->_urlFile = urlFile;
                        if (hs->_hasGroupNameAliases) {
							AutoMutex am(hs->_groupNameAliasesMutex);
							if (!MAP_HAS1(hs->_groupNameAliases, urlFile)) {
								delete crc; 
								return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
							} else {
								crc->_isMediaFile = true;
							} 
						} else {
							crc->_isMediaFile = true; 
						}
                    } else {
						if (isTempFile) {
							deleteFile(filePath);
						}
						delete crc; 
						return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
                    }
				} else {	//we only do this for media files that are not part of manifests
					if (!isManifest && hs->IsMediaFile(filePath)) {
						crc->_isMediaFile = true;
					}
				}
            } else if (hs->IsMediaFile(filePath)) {
                crc->_isMediaFile = true;
            }
            crc->_isTemporaryFile = isProxyCall || isTempFile;
            file = new File;
            if (file == NULL) {
				delete crc; 
				return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
            }
            else {
                file->Initialize(filePath, FILE_OPEN_MODE_READ);
                if (!file->IsOpen()) {
                    delete file;
                    //if we could not open the temporary file, just delete it
                    if (crc->_isTemporaryFile) {
                        AutoMutex am(hs->_temporaryFilesMutex);
                        hs->_temporaryFiles.push_back(filePath);
                    }
					delete crc; 
					return hs->GenerateResponse(connection, MHD_HTTP_NOT_FOUND);
                } else {
                    INFO("Requesting Valid File: %s", STR(filePath));
                    crc->_reader = file;
                    crc->_ipAddress = clientIPAddress;
					crc->_fileOffset = 0;
					uint64_t sizeToRead = buf.st_size;
					/*support range requests:
						  Valid Range: "Range: bytes=x-(y)" --> 206 (where x <= y)
						  Valid Range: "Range: bytes=x-" --> 206 (where x < filesize)
						Invalid Range: "Range: bytes=x- --> 416 Requested Range Not Satisfiable (where x >= filesize)
						Invalid Range: not having valid range format --> 200, equivalent to Bytes=0- 
					*/
					if (applyRangeRequest) {
						uint64_t offset = 0;
						applyRangeRequest = GetRanges(header[MHD_HTTP_HEADER_RANGE], offset, sizeToRead);
						crc->_isRange = applyRangeRequest;
						if (applyRangeRequest) {
							if (offset < sizeToRead) {
								crc->_fileOffset = offset;
								statusCode = MHD_HTTP_PARTIAL_CONTENT;
								int adj = (sizeToRead == (uint64_t)buf.st_size ? 0 : 1);
								contentRange = format("bytes %"PRIu64"-%"PRIu64"/%"PRIu64"", crc->_fileOffset, sizeToRead - (adj == 0 ? 1 : 0), buf.st_size);
								//FATAL(STR(contentRange));
								sizeToRead = sizeToRead - crc->_fileOffset + adj;
							} else {
							    if (file->IsOpen()) {
                                    delete file;
									delete crc; 
									return hs->GenerateResponse(connection, MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE);
                                } 
                            }
						} else {
							crc->_fileOffset = 0;
							sizeToRead = buf.st_size;	//the range request is invalid, fetch as usual
						}
					}
                    response = hs->CreateResponseFromCallback(sizeToRead, hs->_maxMemSizePerConnection,
                            &WebServerApplication::FileContentReaderCallback, (void *) crc,
                            &WebServerApplication::FileFreeResourceCallback);
                }
            }
        }
    } else {
        crc->_reader = (void *) si;
        crc->_ipAddress = clientIPAddress;
        response = hs->CreateResponseFromCallback(si->_fileSize, hs->_maxMemSizePerConnection,
                &WebServerApplication::CacheContentReaderCallback, (void *) crc,
                &WebServerApplication::CacheFreeResourceCallback);
    }

    if (response == NULL) {
		delete crc; 
		return MHD_NO;
    }

    //add the content type to the response header if the mime type for a particular
    //extension is supported (in the configuration file)
    pos = filePath.find_last_of(".");
    if (string::npos != pos) {
        string ext = filePath.substr(pos + 1);
        if (MAP_HAS1(hs->_mimes, ext)) {
            string mimeType = hs->_mimes[ext]._mimeType;
            hs->AddResponseHeader(response, MHD_HTTP_HEADER_CONTENT_TYPE, STR(mimeType));
        }
    }
    if (!derivedResponseHeader.empty()) {

        FOR_MAP_CONST(derivedResponseHeader, string, string, i) {
			hs->AddResponseHeader(response, STR(MAP_KEY(i)), STR(MAP_VAL(i)));
        }
    }

    if (!hs->_incRespHdrs.empty()) {

        FOR_MAP_CONST(hs->_incRespHdrs, string, IncludeResponseHeader, i) {
            IncludeResponseHeader irh = MAP_VAL(i);
            if (char const *content = hs->GetResponseHeader(response, STR(irh._header))) { //response header exists
                if (irh._override) {
                    hs->DeleteResponseHeader(response, STR(irh._header), content);
                } else
                    continue;
            }
            hs->AddResponseHeader(response, STR(irh._header), STR(irh._content));
        }
    }
	
	if (applyRangeRequest) {
		hs->AddResponseHeader(response, MHD_HTTP_HEADER_ACCEPT_RANGES, "bytes");
		if (statusCode == MHD_HTTP_PARTIAL_CONTENT) {
			DEBUG("Partial Content: %s", STR(contentRange));
			hs->AddResponseHeader(response, MHD_HTTP_HEADER_CONTENT_RANGE, STR(contentRange));
		}
	}
	ret = hs->QueueResponse(connection, statusCode, response);
    hs->DestroyResponse(response);
    return ret;
}

bool WebServerApplication::CopyFile(string const &src, string const &dst) {
    uint64_t fileSize = 0;
    uint8_t *buffer = LoadFile(STR(src), false, &fileSize);
    if (buffer == NULL) {
        return false;
    }
    File fDst;
    if (!fDst.Initialize(dst, FILE_OPEN_MODE_TRUNCATE)) {
        return false;        
    }
    if (!fDst.WriteBuffer(buffer, fileSize)) {
        return false;
    }
    return true;
}

bool WebServerApplication::GetRanges(string const &ranges, uint64_t &start, uint64_t &end) {
	string r = ranges;
	trim(r);
	static string const bytes =  "bytes=";
	size_t index = r.find(bytes);
	if (index == 0) {
		string s = r.substr(bytes.length());
		size_t sep_index = s.find("-");
		if (sep_index != string::npos) {
			start = atoll(STR(s.substr(0, sep_index)));
			if (sep_index < (s.length()-1)) {	//Range: bytes=x-y	
				uint64_t tEnd = atoll(STR(s.substr(sep_index + 1)));
				if (tEnd < end) {
					end = tEnd;
				}
				return start <= end;
			} else {	//Range: bytes=x-
				return true;
			}
		}
	}
	return false;
}

bool WebServerApplication::RunPHPInterpreter(string const& inputPHPFile, string &outputPHPFile, bool isGet, map<string, string> const& params,
        map<string, string> &derivedResponseHeader, string const& program, string const &clientIPAddress) {

    GenerateRandomOutputFile(outputPHPFile, PHPEXT);
    string inputFile = normalizePath(inputPHPFile, "");

    string uriParams;

    FOR_MAP_CONST(params, string, string, i) {
        if (!uriParams.empty()) {
            uriParams += "&";
        }
        if (!MAP_KEY(i).empty()) {
            uriParams += MAP_KEY(i) + "=" + MAP_VAL(i);
        } else {
            uriParams += MAP_VAL(i);    //blank key means RAW POST
        }
    }

    PHPEngine engine;
    if (engine.Run(inputFile, uriParams, outputPHPFile, isGet, program, clientIPAddress)) {
        PostProcessPHPFile(STR(outputPHPFile), derivedResponseHeader);
        return true;
    }
    return false;
}

int WebServerApplication::GenerateResponse(struct MHD_Connection *connection, int code, bool needLicense) {
    MHD_Response *response = CreateResponse(code, needLicense);
    if (response == NULL)
        return MHD_NO;
    int ret = QueueResponse(connection, code, response);
    DestroyResponse(response);
    return ret;
}

/*
 ****IMPORTANT****
        We always return MHD_YES so that in RequestHandlerCallback we can show the forbidden page.
        However, if we want to drop the connection right away, we return with MHD_NO.
 */
int WebServerApplication::AcceptPolicyCallback(void *cls, struct sockaddr const *addr, socklen_t /*addrlen*/) {
    WebServerApplication *hs = (WebServerApplication *) cls;
    if (hs == NULL)
        return MHD_NO;
    uint16_t port;
    string clientIPAddress = Helper::GetIPAddress(addr, port, hs->_useIPV6);
    if (clientIPAddress.empty()) {
        return MHD_NO;
    }
    if (hs->_enableIPFilter) {
        if (hs->_useWhiteList && !VECTOR_HAS(hs->_whitelist, clientIPAddress)) {
            WARN("The whitelist restricted access to %s.", STR(clientIPAddress));        
            return MHD_YES;
        }
        if (hs->_useBlackList && VECTOR_HAS(hs->_blacklist, clientIPAddress)) {
            WARN("The blacklist restricted access to %s.", STR(clientIPAddress));
            return MHD_YES; 
        }
    }
    AutoMutex conns(hs->_connsMutex);    
    if (!VECTOR_HAS(hs->_connections, clientIPAddress)) {
        hs->_connections.push_back(clientIPAddress);
    }
    return MHD_YES;
}

void WebServerApplication::RequestCompletedCallback(void *cls, 
    struct MHD_Connection *connection, void ** con_cls, 
    enum MHD_RequestTerminationCode toe) {
    WebServerApplication *hs = (WebServerApplication *) cls;
    if (hs) {
        MHD_ConnectionInfo const *infoConn = hs->GetConnectionInfo(connection, 
            MHD_CONNECTION_INFO_CLIENT_ADDRESS);
        if (infoConn) {
            uint16_t port;
            string clientIPAddress = Helper::GetIPAddress(infoConn->client_addr, 
                port, hs->_useIPV6);
            string result = (toe == MHD_REQUEST_TERMINATED_COMPLETED_OK ? "OK" :
                    (toe == MHD_REQUEST_TERMINATED_WITH_ERROR ? "Error" :
                    (toe == MHD_REQUEST_TERMINATED_TIMEOUT_REACHED ? "Timeout" : 
                    (toe == MHD_REQUEST_TERMINATED_DAEMON_SHUTDOWN ? 
                    "Daemon Shutdown" :
                    (toe == MHD_REQUEST_TERMINATED_READ_ERROR ? "Read Error" : 
                    "Client Abort"
                    ))))); 
            DEBUG("%s:%u: Request Completed.[%s]", STR(clientIPAddress),
                    port, STR(result));
            Session *ssn = reinterpret_cast<Session *>(*con_cls);
            if (ssn) {
                if (ssn->_postProc != NULL) {
                    hs->DestroyPostProcessor(&ssn->_postProc);
                }
                ssn->_postGetParams.clear();
                if (ssn->_upload.IsOpen()) {
                    ssn->_upload.Close();
                }
            }
        }
    }
}

void WebServerApplication::Logger(void *arg, char const *fmt, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    DEBUG("MHD: %s\n", buf);
}

void WebServerApplication::NotifyConnectionCallback(void *cls, 
    struct MHD_Connection *connection, void **socket_context, 
    enum MHD_ConnectionNotificationCode toe) {
    WebServerApplication *hs = (WebServerApplication *) cls;
    if (hs) {
        MHD_ConnectionInfo const *infoConn = hs->GetConnectionInfo(connection, 
            MHD_CONNECTION_INFO_CLIENT_ADDRESS);
        if (infoConn) {
            uint16_t port;
            string clientIPAddress = Helper::GetIPAddress(infoConn->client_addr, 
                port, hs->_useIPV6);
            if (toe == MHD_CONNECTION_NOTIFY_STARTED) {
                DEBUG("%s:%d Connected.", STR(clientIPAddress), port);
                static int id;
                Session *ssn = new Session;
                if (ssn) {
                    *socket_context = reinterpret_cast<void *>(ssn);
                    ssn->_ipAddress = clientIPAddress;
                    ssn->_postProc = NULL;
                    ssn->_postData = NULL;
                    ssn->_id = ++ id;
                    AutoMutex am(hs->_sessionsMutex);
                    hs->_sessions[ssn->_id] = ssn;
                }
            } else if (toe == MHD_CONNECTION_NOTIFY_CLOSED) {
                DEBUG("%s:%u Disconnected.", STR(clientIPAddress), port);
                Session *ssn = reinterpret_cast<Session *>(*socket_context);
				if (ssn) {
                    if (ssn->_postProc != NULL) {
                        hs->DestroyPostProcessor(&ssn->_postProc);
                    }
                    ssn->_postGetParams.clear();
                    if (ssn->_upload.IsOpen()) {
                        ssn->_upload.Close();
                    }
                    AutoMutex am(hs->_sessionsMutex);
                    hs->_sessions.erase(ssn->_id);
				    delete ssn;
                }
            }
        }
    }
}

#ifdef WIN32

bool WebServerApplication::LoadLibraryAndFunctions() {
    _hMod = ::LoadLibrary(LIBMICROHTTPDLIBRARY);
    if (_hMod == NULL) {
        FATAL("Unable to load http server library");
        return false;
    }
    _pMHD_get_connection_info = (PMHDGCI)::GetProcAddress(_hMod, "MHD_get_connection_info");
    if (_pMHD_get_connection_info == NULL) {
        FATAL("Unable to load function: MHD_get_connection_info");
        return false;
    }
    _pMHD_add_response_header = (PMHDARH)::GetProcAddress(_hMod, "MHD_add_response_header");
    if (_pMHD_add_response_header == NULL) {
        FATAL("Unable to load function: MHD_add_response_header");
        return false;
    }
    _pMHD_delete_response_header = (PMHDDRH)::GetProcAddress(_hMod, "MHD_del_response_header");
    if (_pMHD_delete_response_header == NULL) {
        FATAL("Unable to load function: MHD_del_response_header");
        return false;
    }
    _pMHD_get_response_header = (PMHDGRH)::GetProcAddress(_hMod, "MHD_get_response_header");
    if (_pMHD_get_response_header == NULL) {
        FATAL("Unable to load function: MHD_get_response_header");
        return false;
    }
    _pMHD_get_response_headers = (PMHDGRHS)::GetProcAddress(_hMod, "MHD_get_response_headers");
    if (_pMHD_get_response_headers == NULL) {
        FATAL("Unable to load function: MHD_get_response_headers");
        return false;
    }
    _pMHD_create_response_from_callback = (PMHDCRFC)::GetProcAddress(_hMod, "MHD_create_response_from_callback");
    if (_pMHD_create_response_from_callback == NULL) {
        FATAL("Unable to load function: MHD_create_response_from_callback");
        return false;
    }
    _pMHD_destroy_response = (PMHDDR)::GetProcAddress(_hMod, "MHD_destroy_response");
    if (_pMHD_destroy_response == NULL) {
        FATAL("Unable to load function: MHD_destroy_response");
        return false;
    }
    _pMHD_queue_response = (PMHDQR)::GetProcAddress(_hMod, "MHD_queue_response");
    if (_pMHD_queue_response == NULL) {
        FATAL("Unable to load function: MHD_queue_response");
        return false;
    }
    _pMHD_start_daemon = (PMHDSD)::GetProcAddress(_hMod, "MHD_start_daemon");
    if (_pMHD_start_daemon == NULL) {
        FATAL("Unable to load function: MHD_start_daemon");
        return false;
    }
    _pMHD_stop_daemon = (PMHDSTD)::GetProcAddress(_hMod, "MHD_stop_daemon");
    if (_pMHD_stop_daemon == NULL) {
        FATAL("Unable to load function: MHD_stop_daemon");
        return false;
    }
    _pMHD_create_response_from_buffer = (PMHDCRFB)::GetProcAddress(_hMod, "MHD_create_response_from_buffer");
    if (_pMHD_create_response_from_buffer == NULL) {
        FATAL("Unable to load function: MHD_create_response_from_buffer");
        return false;
    }
    _pMHD_get_connection_values = (PMHDGCV)::GetProcAddress(_hMod, "MHD_get_connection_values");
    if (_pMHD_get_connection_values == NULL) {
        FATAL("Unable to load function: MHD_get_connection_values");
        return false;
    }
    _pMHD_set_connection_value = (PMHDSCV)::GetProcAddress(_hMod, "MHD_set_connection_value");
    if (_pMHD_set_connection_value == NULL) {
        FATAL("Unable to load function: MHD_set_connection_value");
        return false;
    }
    _pMHD_create_post_processor = (PMHDCPP)::GetProcAddress(_hMod, "MHD_create_post_processor");
    if (_pMHD_create_post_processor == NULL) {
        FATAL("Unable to load function: MHD_create_post_processor");
        return false;
    }
    _pMHD_post_process = (PMHDPP)::GetProcAddress(_hMod, "MHD_post_process");
    if (_pMHD_post_process == NULL) {
        FATAL("Unable to load function: MHD_post_process");
        return false;
    }
    _pMHD_destroy_post_processor = (PMHDDPP)::GetProcAddress(_hMod, "MHD_destroy_post_processor");
    if (_pMHD_destroy_post_processor == NULL) {
        FATAL("Unable to load function: MHD_destroy_post_processor");
        return false;
    }
    _pMHD_digest_auth_get_username = (PMHDDAGU)::GetProcAddress(_hMod, "MHD_digest_auth_get_username");
    if (_pMHD_digest_auth_get_username == NULL) {
        FATAL("Unable to load function: MHD_digest_auth_get_username");
        return false;
    }
    _pMHD_queue_auth_fail_response = (PMHDQAFR)::GetProcAddress(_hMod, "MHD_queue_auth_fail_response");
    if (_pMHD_queue_auth_fail_response == NULL) {
        FATAL("Unable to load function: MHD_queue_auth_fail_response");
        return false;
    }
    _pMHD_digest_auth_check = (PMHDDAC)::GetProcAddress(_hMod, "MHD_digest_auth_check");
    if (_pMHD_digest_auth_check == NULL) {
        FATAL("Unable to load function: MHD_digest_auth_check");
        return false;
    }
    _pMHD_basic_auth_get_username_password = (PMHDBAGU)::GetProcAddress(_hMod, "MHD_basic_auth_get_username_password");
    if (_pMHD_basic_auth_get_username_password == NULL) {
        FATAL("Unable to load function: MHD_basic_auth_get_username_password");
        return false;
    }
    _pMHD_queue_basic_auth_fail_response = (PMHDQBAFR)::GetProcAddress(_hMod, "MHD_queue_basic_auth_fail_response");
    if (_pMHD_queue_basic_auth_fail_response == NULL) {
        FATAL("Unable to load function: MHD_queue_basic_auth_fail_response");
        return false;
    }
    return true;
}
#endif

bool WebServerApplication::TimePeriodElapsed() {
#ifndef WIN32
    if (getppid() != _parentProcessId)  {   //the parent process has quitted
#else
	DWORD dwRes;
	if (dwRes = WaitForSingleObject(_parentHandle, 0) == WAIT_OBJECT_0) {
#endif
        INFO("RMS has quitted. There is no reason to stay alive.");
        EventLogger::SignalShutdown();
		return true;
    }

	if (_newVariantHandler) {
        _pVariantHandler->SendHello();
    }
    
	bool found = true;
	while(found) {
		found = false;
		AutoMutex am(_streamingSessionsMutex);
		FOR_MAP(_streamingSessions, string, StreamingSession, j) {
			StreamingSession const &streamSsn = MAP_VAL(j);
			time_t currentTime = getutctime();
			uint32_t elapsed = (uint32_t)(currentTime - (streamSsn._startTime + streamSsn._elapsed));// -(ORPHANSESSIONTIMER * 10);
			if (elapsed > (uint32_t)(ORPHANSESSIONCLEANUPPERIOD * streamSsn._chunkLength)) {
				//DEBUG("TimePeriodElapsed elapsed: %u ct(%u) st(%u) el(%u)", elapsed, currentTime, streamSsn._startTime, streamSsn._elapsed);
				//send event
                AutoMutex am(_evtLogMutex);
				Variant sessionInfo;
				sessionInfo["clientIP"] = streamSsn._ipAddress;
				sessionInfo["sessionId"] = streamSsn._sessionId;
				sessionInfo["playlist"] = streamSsn._playList;
				sessionInfo["startTime"] = Helper::ConvertToLocalSystemTime(streamSsn._startTime);
				sessionInfo["stopTime"] = Helper::ConvertToLocalSystemTime(currentTime);
				sessionInfo["type"] = "streamingSessionEnded";
				_pVariantHandler->SendEventLog(sessionInfo);
				DEBUG("Session (%s) received no request after %"PRIu32" seconds (timeout: %"PRIu32").", STR(streamSsn._identifier), elapsed,
					(uint32_t)(ORPHANSESSIONCLEANUPPERIOD * streamSsn._chunkLength));
				DEBUG("Streaming Session Ended: (%s)", STR(streamSsn._identifier));

				if (_hasGroupNameAliases && !streamSsn._identifier.empty()) { //if it is an alias, delete from group name aliases
					size_t sep = streamSsn._identifier.find_last_of(',');
					if (sep != string::npos) {
						string alias = streamSsn._identifier.substr(sep + 1); 
						AutoMutex am(_groupNameAliasesMutex);
						_groupNameAliases.erase(alias);
					}
				}
				MAP_ERASE1(_streamingSessions, MAP_KEY(j));
				found = true;
				break;
			}
		}
	}

	found = true;
    AutoMutex am(_evtLogMutex);
	while(found) {
		found = false;
		FOR_MAP(_mediaFileLastAccess, string, Variant, iter) {
			Variant vt = MAP_VAL(iter);
			uint32_t end = (uint32_t)vt["end"];
			if (getutctime() - end > _mediaFileDownloadTimeout) {
				//send event
				_pVariantHandler->SendEventLog(vt);
				string mediaFile = vt["mediaFile"];
				DEBUG("Media File Downloaded: (%s) [Timeout=%"PRIu16"]", STR(mediaFile), _mediaFileDownloadTimeout);

				AutoMutex am(_groupNameAliasesMutex);
				_groupNameAliases.erase(MAP_KEY(iter));
				_mediaFileLastAccess.erase(iter);

				found = true;
				break;
			}
		}
	}
	DeleteTemporaryFiles();
    return true;
}

void WebServerApplication::DeleteTemporaryFiles() {
	vector<string> tempFiles;
	{
		AutoMutex am(_temporaryFilesMutex);
		tempFiles.swap(_temporaryFiles);
	}
	vector<string>::iterator iter = tempFiles.begin();
	while(iter != tempFiles.end()) {
		string tempFile = *iter;
		if (!tempFile.empty()) {
#ifdef WIN32
			struct _stat64 buf;
#else
			struct stat64 buf;
#endif
			//check if file still exists, it could have been manually deleted
			if (Helper::GetFileInfo(tempFile, buf)) {		
				if (!deleteFile(STR(tempFile))) {
					++iter;
					//not deleted, put back
					AutoMutex am(_temporaryFilesMutex);
					_temporaryFiles.push_back(tempFile);
					continue;
				}
			}
		}
		iter = tempFiles.erase(iter);
	}
}

bool WebServerApplication::ProcessOriginMessage(Variant &message) {
    bool ret = false;
    string type = message["type"];
    if (type.compare("getHttpStreamingSessions") == 0) {
        INFO("Origin Request: getHttpStreamingSessions");
        message["httpStreamingSessions"].IsArray(true);
        message["result"] = true;
        AutoMutex am(_streamingSessionsMutex);
        if (!_streamingSessions.empty()) {

            FOR_MAP_CONST(_streamingSessions, string, StreamingSession, j) {
                StreamingSession const &streamingSsn = MAP_VAL(j);
                Variant streamSsn;
                streamSsn["clientIP"] = streamingSsn._ipAddress;
                streamSsn["sessionId"] = streamingSsn._sessionId;
                streamSsn["targetFolder"] = streamingSsn._targetFolder;
                streamSsn["startTime"] = Helper::ConvertToLocalSystemTime(streamingSsn._startTime);
                streamSsn["elapsedTime"] = (uint32_t)(getutctime() - streamingSsn._startTime);
                message["httpStreamingSessions"].PushToArray(streamSsn);
            }
        }
        ret = _pVariantHandler->SendResultToOrigin("httpStreamingSessions", message);
    } else if (type.compare("getHttpClientsConnected") == 0) {
		INFO("Origin Request: getHttpClientsConnected");
		uint32_t httpStreamingSessionsCount = 0;
		AutoMutex am(_streamingSessionsMutex);
		string groupName = message["payload"]["groupName"];
		if (!_streamingSessions.empty()) {
			FOR_MAP_CONST(_streamingSessions, string, StreamingSession, j) {
				StreamingSession const &streamingSsn = MAP_VAL(j);
				if (groupName != "") {
					vector<string> stringVector;
					Helper::Split(streamingSsn._targetFolder, FOLDERSEPARATORS, stringVector);
					if (find(stringVector.begin(), stringVector.end(), groupName) != stringVector.end()) { 
						httpStreamingSessionsCount++;
					}
				} else {
					httpStreamingSessionsCount++;
				}
			}
		}
		message["result"] = (bool) true;
		message["httpClientsConnected"]["groupName"] = (string) groupName;
		message["httpClientsConnected"]["httpStreamingSessionsCount"] = (uint32_t) httpStreamingSessionsCount;
		ret = _pVariantHandler->SendResultToOrigin("httpClientsConnected", message);
	} else if (type.compare("addGroupNameAlias") == 0) {
        INFO("Origin Request: addGroupNameAlias");
        string alias = message["payload"]["aliasName"];
        string groupName = message["payload"]["groupName"];
        //verify that the group name exists from streams configurations
        message["result"] = false;
        if (_hasGroupNameAliases) { 
            AutoMutex am(_groupNameAliasesMutex);
            bool found = MAP_HAS1(_groupNameAliases, alias);
            if (!found) {
                _groupNameAliases[alias] = groupName;
            }
            message["result"] = !found; 
        }
        ret = _pVariantHandler->SendResultToOrigin("addGroupNameAlias", message);
    } else if (type.compare("getGroupNameByAlias") == 0) {
        INFO("Origin Request: getGroupNameByAlias");
        string alias = message["payload"]["aliasName"];
        //bool remove = message["remove"];
        message["result"] = false;
        AutoMutex am(_groupNameAliasesMutex);
        if (_hasGroupNameAliases && MAP_HAS1(_groupNameAliases, alias)) {
            message["result"] = true;
            message["groupName"] = _groupNameAliases[alias];
        }
        ret = _pVariantHandler->SendResultToOrigin("getGroupNameByAlias", message);
    } else if (type.compare("removeGroupNameAlias") == 0) {
        INFO("Origin Request: removeGroupNameAlias");
        string alias = message["payload"]["aliasName"];
        AutoMutex am(_groupNameAliasesMutex);
        message["result"] = (_hasGroupNameAliases && _groupNameAliases.erase(alias) == 1);
        ret = _pVariantHandler->SendResultToOrigin("removeGroupNameAlias", message);
    } else if (type.compare("flushGroupNameAliases") == 0) {
        INFO("Origin Request: flushGroupNameAliases");
        AutoMutex am(_groupNameAliasesMutex);
        _groupNameAliases.clear();
        message["result"] = _hasGroupNameAliases;
        ret = _pVariantHandler->SendResultToOrigin("flushGroupNameAliases", message);
    } else if (type.compare("listGroupNameAliases") == 0) {
        INFO("Origin Request: listGroupNameAliases");
        message["result"] = false;
        if (_hasGroupNameAliases) {
            message["groupNameAliases"].IsArray(true);
            AutoMutex am(_groupNameAliasesMutex);
            FOR_MAP_CONST(_groupNameAliases, string, string, i) {
                Variant groupNameAlias;
                groupNameAlias["aliasName"] = MAP_KEY(i);
                groupNameAlias["groupName"] = MAP_VAL(i);
                message["groupNameAliases"].PushToArray(groupNameAlias);
            }
            message["result"] = true;
        }
        ret = _pVariantHandler->SendResultToOrigin("listGroupNameAliases", message);
    }
    return ret;
}

void WebServerApplication::RegisterProtocol(BaseProtocol *pProtocol) {
    BaseClientApplication::RegisterProtocol(pProtocol);
    _pVariantHandler->RegisterInfoProtocol(pProtocol->GetId());
    _newVariantHandler = false;
    //origin has started, the license is set up successfully
    _webUiLicenseOnly = false;
    INFO("Got connected from Origin.");
}

void WebServerApplication::UnRegisterProtocol(BaseProtocol *pProtocol) {
    _pVariantHandler->UnRegisterInfoProtocol(pProtocol->GetId());
    BaseClientApplication::UnRegisterProtocol(pProtocol);
    //got disconnected from origin, then there is no reason to stay
    INFO("Got disconnected from Origin. Reconnecting...");
	CreateVariantHandler();
}

bool WebServerApplication::ActivateAcceptor(IOHandler *pIOHandler) {
    switch (pIOHandler->GetType()) {
    case IOHT_UDP_CARRIER: {
            UDPCarrier *pUDPCarrier = (UDPCarrier *) pIOHandler;
            if (pUDPCarrier->GetProtocol() != NULL)
                pUDPCarrier->GetProtocol()->GetNearEndpoint()->EnqueueForDelete();
            return true;
        }
    case IOHT_ACCEPTOR: {
            TCPAcceptor *pAcceptor = (TCPAcceptor *) pIOHandler;
            Variant &params = pAcceptor->GetParameters();
            if ((params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_RTMP)
                && (params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_BIN_VARIANT)
                && (params[CONF_PROTOCOL] != CONF_PROTOCOL_INBOUND_RTMPS)
                )
                return true;
            pAcceptor->SetApplication(this);
            return true;
        }
    default: {
            return true;
        }
    }
}

void WebServerApplication::CreateVariantHandler() {
    
    Variant varAcceptor;
    varAcceptor[CONF_IP] = "127.0.0.1";
    varAcceptor[CONF_PORT] = (uint32_t) 0;
    varAcceptor[CONF_PROTOCOL] = CONF_PROTOCOL_INBOUND_BIN_VARIANT;
    vector<uint64_t> chain = ProtocolFactoryManager::ResolveProtocolChain(
        CONF_PROTOCOL_INBOUND_BIN_VARIANT);
    if (chain.size() == 0) {
        FATAL("Unable to get chain "CONF_PROTOCOL_INBOUND_BIN_VARIANT);
        EventLogger::SignalShutdown();
    }

    if (_pAcceptor) {
        delete _pAcceptor;
    }

    _pAcceptor = new TCPAcceptor(
        varAcceptor[CONF_IP],
        varAcceptor[CONF_PORT],
        varAcceptor,
        chain);
    if (!_pAcceptor->Bind()) {
        FATAL("Unable to bind acceptor");
        EventLogger::SignalShutdown();
    }
    if (!ActivateAcceptor(_pAcceptor)) {
        FATAL("Unable to activate acceptor");
        EventLogger::SignalShutdown();
    }
    
    if (_pVariantHandler) {
        UnRegisterAppProtocolHandler(PT_BIN_VAR);
        delete _pVariantHandler;
    }
    
    _pVariantHandler = new WebServerVariantAppProtocolHandler(_configuration);
    RegisterAppProtocolHandler(PT_BIN_VAR, _pVariantHandler);

    _pVariantHandler->SetConnectorInfo(_pAcceptor->GetParameters());
    _newVariantHandler = true;
}

bool WebServerApplication::Initialize() {
    if (!BaseClientApplication::Initialize()) {
        FATAL("Unable to initialize application");
        return false;
    }
#ifdef WIN32
    if (!LoadLibraryAndFunctions())
        return false;
#endif

    _parentProcessId = GetConfiguration()["parentPId"];
#ifdef WIN32
	if (_parentProcessId > 0) {
		_parentHandle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _parentProcessId);
	}
#endif

    CreateVariantHandler();
    EnqueueForTimeEvent(ORPHANSESSIONTIMER);
    
    uint16_t maxConcurrentConnections = FD_SETSIZE - 4;
    uint16_t connectionTimeout = NOCONNECTIONTIMEOUT;
    uint16_t maxConcurrentConnectionsSameIP = NOCONNECTIONLIMIT;
    uint16_t threadPoolSize = DEFAULTTHREADPOOLSIZE;
    _sslMode = DEFAULTSSLMODE;
    string whitelistFile, blacklistFile;
    string sslKeyFile, sslCertFile;

    if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_MAXMEMSIZEPERCONN))
        _maxMemSizePerConnection = (uint32_t) _configuration.GetValue(CONFIG_MAXMEMSIZEPERCONN, false);
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_MAXCONCURCONN))
        maxConcurrentConnections = (uint16_t) _configuration.GetValue(CONFIG_MAXCONCURCONN, false);
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_CONNTIMEOUT))
        connectionTimeout = (uint16_t) _configuration.GetValue(CONFIG_CONNTIMEOUT, false);
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_MAXCONCURCONNSAMEIP))
        maxConcurrentConnectionsSameIP = (uint16_t) _configuration.GetValue(CONFIG_MAXCONCURCONNSAMEIP, false);
    if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_THREADPOOLSIZE))
        threadPoolSize = (uint16_t) _configuration.GetValue(CONFIG_THREADPOOLSIZE, false);
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_SSLMODE))
        _sslMode = (string) _configuration.GetValue(CONFIG_SSLMODE, false);
    if (_configuration.HasKeyChain(V_BOOL, false, 1, CONFIG_USEIPV6))
        _useIPV6 = (bool)_configuration.GetValue(CONFIG_USEIPV6, false);
    if (_configuration.HasKeyChain(V_BOOL, false, 1, CONFIG_ENABLEIPFILTER))
        _enableIPFilter = (bool)_configuration.GetValue(CONFIG_ENABLEIPFILTER, false);
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_WHITELISTFILE)) {
        whitelistFile = (string) _configuration.GetValue(CONFIG_WHITELISTFILE, false);
        whitelistFile = normalizePath(whitelistFile, "");
    }
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_BLACKLISTFILE)) {
        blacklistFile = (string) _configuration.GetValue(CONFIG_BLACKLISTFILE, false);
        blacklistFile = normalizePath(blacklistFile, "");
    }
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_SSLKEYFILE)) {
        sslKeyFile = (string) _configuration.GetValue(CONFIG_SSLKEYFILE, false);
        sslKeyFile = normalizePath(sslKeyFile, "");
    }
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_SSLCERTFILE)) {
        sslCertFile = (string) _configuration.GetValue(CONFIG_SSLCERTFILE, false);
        sslCertFile = normalizePath(sslCertFile, "");
    }
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_WEBROOTFOLDER)) {
        _webRootFolder = (string) _configuration.GetValue(CONFIG_WEBROOTFOLDER, false);
        if (_webRootFolder.empty()) {
            FATAL("Web root folder could not be found.");
            return false;
        }
        _webRootFolder = normalizePath(_webRootFolder, "");
        size_t pos = _webRootFolder.find_last_of("\\/");
        if (pos != _webRootFolder.size() - 1)
            _webRootFolder += FILESEP;
    }
    
    _tempFolder = Helper::GetTemporaryDirectory();

    if (_configuration.HasKeyChain(V_BOOL, false, 1, CONFIG_ENABLECACHE)) {
        _enableCache = (bool)_configuration.GetValue(CONFIG_ENABLECACHE, false);
        if (_enableCache && _configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_CACHESIZE)) {
            uint64_t cacheSize = (uint64_t) _configuration.GetValue(CONFIG_CACHESIZE, false);
            _memCache->SetCacheSize(cacheSize);
        }
    }

    if (_configuration.HasKeyChain(V_BOOL, false, 1, CONFIG_HASGROUPNAMEALIASES))
        _hasGroupNameAliases = (bool)_configuration.GetValue(CONFIG_HASGROUPNAMEALIASES, false);

	if (_configuration.HasKeyChain(V_BOOL, false, 1, CONFIG_ENABLERANGEREQUESTS)) {
		_enableRangeRequests = (bool)_configuration.GetValue(CONFIG_ENABLERANGEREQUESTS, false);
	}

	if (_configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_MEDIAFILEDOWNLOADTIMEOUT)) {
		_mediaFileDownloadTimeout = (uint16_t)_configuration.GetValue(CONFIG_MEDIAFILEDOWNLOADTIMEOUT, false);
		if (_mediaFileDownloadTimeout < DEFAULTMEDIAFILEDOWNLOADTIMEOUT) {	//give it at least 30 seconds
			_mediaFileDownloadTimeout = DEFAULTMEDIAFILEDOWNLOADTIMEOUT;
		}
	}

    if (_configuration.HasKeyChain(V_BOOL, false, 1, CONFIG_WEBUILICENSEONLY))
        _webUiLicenseOnly = (bool)_configuration.GetValue(CONFIG_WEBUILICENSEONLY, false);
    
    Variant supportedMimeTypes;
    supportedMimeTypes.IsArray(true);
    if (_configuration.HasKeyChain(V_MAP, false, 1, CONFIG_SUPPORTEDMIMETYPES)) {
        FOR_MAP(_configuration[CONFIG_SUPPORTEDMIMETYPES], string, Variant, i) {
            if (MAP_VAL(i) != V_MAP) {
                FATAL("Invalid mime type:\n%s", STR(MAP_VAL(i).ToString()));
                return false;
            }
            supportedMimeTypes.PushToArray(MAP_VAL(i));
        }
    }
    
    string extensions;
    FOR_MAP(supportedMimeTypes, string, Variant, i) {
        if (MAP_VAL(i).HasKeyChain(V_STRING, false, 1, CONFIG_MIME_EXTENSIONS)) {
            extensions = (string) MAP_VAL(i).GetValue(CONFIG_MIME_EXTENSIONS, false);
            //erase all spaces in extensions
            //linux - needs explicit type for isspace as it has multiple overloads in C++ standard library  
            extensions.erase(remove_if(extensions.begin(), extensions.end(), (int(*)(int))isspace), extensions.end());
            SupportedMimeType smt;
            smt._extensions = extensions;
            smt._mimeType = (string) MAP_VAL(i).GetValue(CONFIG_MIME_TYPE, false);
            smt._streamType = (string) MAP_VAL(i).GetValue(CONFIG_MIME_STREAMTYPE, false);
            smt._isManifest = (bool) MAP_VAL(i).GetValue(CONFIG_MIME_ISMANIFEST, false);
            if (smt._extensions.empty() || smt._mimeType.empty()) //do not include empty values
                continue;
#pragma warning ( disable : 4996 )  // allow use of strtok
            char *pch = strtok((char *)smt._extensions.c_str(), ",");
            while (pch != NULL) {
                _mimes[pch] = smt;
                //if (smt._streamType.empty()) {
                if (!smt._isManifest) {
                    string media(pch);
                    if (media.compare("xml") != 0)
                        ADD_VECTOR_END(_supportedNonStreamMediaFiles, pch);
                }
                pch = strtok(NULL, ",");
            }
        }
    }

    Variant includeResponseHeaders;
    includeResponseHeaders.IsArray(true);
    if (_configuration.HasKeyChain(V_MAP, false, 1, CONFIG_INCRESPHDRS)) {
        FOR_MAP(_configuration[CONFIG_INCRESPHDRS], string, Variant, i) {
            if (MAP_VAL(i) != V_MAP) {
                FATAL("Invalid response header:\n%s", STR(MAP_VAL(i).ToString()));
                return false;
            }
            includeResponseHeaders.PushToArray(MAP_VAL(i));
        }
    }

    FOR_MAP(includeResponseHeaders, string, Variant, i) {
        if (MAP_VAL(i).HasKeyChain(V_STRING, false, 1, CONFIG_RESPHDR_HEADER)) {
            IncludeResponseHeader irh;
            irh._header = (string) MAP_VAL(i).GetValue(CONFIG_RESPHDR_HEADER, false);
            irh._content = (string) MAP_VAL(i).GetValue(CONFIG_RESPHDR_CONTENT, false);
            irh._override = (bool) MAP_VAL(i).GetValue(CONFIG_RESPHDR_OVERRIDE, false);
            if (irh._header.empty()) //do not include empty headers
                continue;
            _incRespHdrs[irh._header] = irh;
        }
    }

    if (_configuration.HasKeyChain(V_MAP, false, 1, CONFIG_APIPROXY)) {
        Variant apiProxyConf = _configuration.GetValue(CONFIG_APIPROXY, false);
        _apiProxy._authentication = apiProxyConf.HasKeyChain(V_STRING, false, 1, CONFIG_APIPROXY_AUTH) ?
            (string)apiProxyConf.GetValue(CONFIG_APIPROXY_AUTH, false) : "";
        if (!_apiProxy._authentication.empty()) {
            std::transform(_apiProxy._authentication.begin(), _apiProxy._authentication.end(),
                _apiProxy._authentication.begin(), ::tolower);
            if (_apiProxy._authentication.compare(APIPROXY_BASICAUTH) == 0) {
                _apiProxy._pseudoDomain = apiProxyConf.HasKeyChain(V_STRING, false, 1, CONFIG_APIPROXY_PSEUDO) ?
                    (string)apiProxyConf.GetValue(CONFIG_APIPROXY_PSEUDO, false) : APIPROXY_DEFAULTPSEUDO;
                _apiProxy._address = apiProxyConf.HasKeyChain(V_STRING, false, 1, CONFIG_APIPROXY_ADDRESS) ?
                    (string)apiProxyConf.GetValue(CONFIG_APIPROXY_ADDRESS, false) : APIPROXY_DEFAULTADDRESS;
                _apiProxy._port = apiProxyConf.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_APIPROXY_PORT) ?
                    (uint16_t) apiProxyConf.GetValue(CONFIG_APIPROXY_PORT, false) : APIPROXY_DEFAULTPORT;
                _apiProxy._userName = apiProxyConf.HasKeyChain(V_STRING, false, 1, CONFIG_APIPROXY_USERNAME) ?
                    (string)apiProxyConf.GetValue(CONFIG_APIPROXY_USERNAME, false) : "";
                _apiProxy._password = apiProxyConf.HasKeyChain(V_STRING, false, 1, CONFIG_APIPROXY_PASSWORD) ?
                    (string)apiProxyConf.GetValue(CONFIG_APIPROXY_PASSWORD, false) : "";
                if (!_apiProxy._userName.empty() && !_apiProxy._password.empty()) {
                    _enableAPIProxy = true;
                } else {
                    WARN("Username and password are required to enable API proxy basic authentication.");
                }
            }
        }
    }

	Variant authentications;
	authentications.IsArray(true);
	if (_configuration.HasKeyChain(V_MAP, false, 1, CONFIG_AUTHENTICATION)) {
		FOR_MAP(_configuration[CONFIG_AUTHENTICATION], string, Variant, i) {
			if (MAP_VAL(i) != V_MAP) {
				FATAL("Invalid authentication:\n%s", STR(MAP_VAL(i).ToString()));
				return false;
			}
			authentications.PushToArray(MAP_VAL(i));
		}
	}

	FOR_MAP(authentications, string, Variant, i) {
		if (MAP_VAL(i).HasKeyChain(V_STRING, false, 1, CONFIG_AUTH_DOMAIN)) {
			Authentication auth;
			auth._domain = (string)MAP_VAL(i).GetValue(CONFIG_AUTH_DOMAIN, false);
			auth._digestFile = (string)MAP_VAL(i).GetValue(CONFIG_AUTH_DIGESTFILE, false);
			auth._enable = (bool)MAP_VAL(i).GetValue(CONFIG_AUTH_ENABLE, false);
			if (auth._domain.empty()) //do not include empty domains
				continue;
			_auths[auth._domain] = auth;
		}
	}

    struct MHD_OptionItem ops[] = {
        {MHD_OPTION_CONNECTION_LIMIT, maxConcurrentConnections, NULL},
        {MHD_OPTION_PER_IP_CONNECTION_LIMIT, maxConcurrentConnectionsSameIP, NULL},
        {MHD_OPTION_CONNECTION_TIMEOUT, connectionTimeout, NULL},
        {MHD_OPTION_THREAD_POOL_SIZE, threadPoolSize, NULL},
        {MHD_OPTION_EXTERNAL_LOGGER, (intptr_t) & WebServerApplication::Logger, this},
        {MHD_OPTION_NOTIFY_COMPLETED, (intptr_t) & WebServerApplication::RequestCompletedCallback, this},
        {MHD_OPTION_NOTIFY_CONNECTION, (intptr_t) & WebServerApplication::NotifyConnectionCallback, this},
        {MHD_OPTION_END, 0, NULL}
    };
    if (_enableIPFilter && ((whitelistFile.empty() && blacklistFile.empty()) || !LoadLists(whitelistFile, blacklistFile))) {
        FATAL("IP filtering is enabled but the whitelist and blacklist files are not specified or are " \
            "both missing or corrupted.");
        return false;
    }

    Metric::_tps = threadPoolSize;

    uint8_t daemonFlags = 0;
    if (_useIPV6)
        daemonFlags = MHD_USE_IPv6;
    bool startHttp = true;
    if (0 == strcmp(STR(_sslMode), SSLMODE_DISABLED))
        StartHTTPServer(daemonFlags, ops);
    else if (0 == strcmp(STR(_sslMode), SSLMODE_ALWAYS)) {
        startHttp = false;
        //load SSL files
        _keySSL = LoadFile(STR(sslKeyFile));
        if (_keySSL == NULL) {
            FATAL("Could not load server key file.");
            return false;
        }
        _certSSL = LoadFile(STR(sslCertFile));
        if (_certSSL == NULL) {
            FATAL("Could not load server certificate file.");
            return false;
        }
        StartHTTPSServer(daemonFlags, ops);
    } else if (0 == strcmp(STR(_sslMode), SSLMODE_AUTO)) { //try https first
        startHttp = false;
        //load SSL files
        _keySSL = LoadFile(STR(sslKeyFile));
        if (_keySSL == NULL)
            startHttp = true;
        else {
            _certSSL = LoadFile(STR(sslCertFile));
            if (_certSSL == NULL)
                startHttp = true;
        }
        if (startHttp) //failed to load key and cert files, no need to try https
            StartHTTPServer(daemonFlags, ops);
        else {
            StartHTTPSServer(daemonFlags, ops);
            if (_server == NULL) { //failed to start https, try http
                if (_useIPV6)
                    daemonFlags = MHD_USE_IPv6;
                StartHTTPServer(daemonFlags, ops);
            }
        }
    } else {
        FATAL("Invalid SSL Mode.");
        return false;
    }

    if (_server == NULL) {
        WARN("Web Server was not instantiated.");
        return false;
    } else
        INFO("Web Server(%s) was instantiated successfully. Listening on port: %u", startHttp ? STR("HTTP") : STR("HTTPS"), _port);

    return true;
}

bool WebServerApplication::IsAliasAvailable(string const &clientIPAddress, string const &alias) {
	bool available = true;
    AutoMutex am(_streamingSessionsMutex);
    FOR_MAP_CONST(_streamingSessions, string, StreamingSession, iter) {
		StreamingSession streamSsn = MAP_VAL(iter);
		size_t sep = streamSsn._identifier.find_last_of(',');
		if (sep != string::npos) {
			string aliasPart = streamSsn._identifier.substr(sep + 1);
			//the assumption being that a device with the same ip can stream as many copy at the same time
			if (aliasPart.compare(alias) == 0) {	//same alias
				available = (streamSsn._ipAddress.compare(clientIPAddress) == 0);	//same ip address
				break;
			}
		}
    }
    return available;
}

void WebServerApplication::StartHTTPServer(uint8_t daemonFlags, struct MHD_OptionItem ops[]) {
    daemonFlags |= MHD_USE_SELECT_INTERNALLY;
#ifdef WEBSERVER_DEBUGMODE
    daemonFlags |= MHD_USE_DEBUG;
#endif /*WEBSERVER_DEBUGMODE*/
    _port = _configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_PORT) ?
            (uint16_t) _configuration.GetValue(CONFIG_PORT, false) : DEFAULTCONNECTIONHTTPPORT;
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_BINDTOIP))
        _bindToIP = (string) _configuration.GetValue(CONFIG_BINDTOIP, false);
    if (_bindToIP.empty()) {    //use all ips
#ifdef WIN32
        _server = _pMHD_start_daemon(daemonFlags, _port, &WebServerApplication::AcceptPolicyCallback, this,
                &WebServerApplication::RequestHandlerCallback, this,
                MHD_OPTION_ARRAY, ops,
                MHD_OPTION_END);
#else
        _server = MHD_start_daemon(daemonFlags, _port, &WebServerApplication::AcceptPolicyCallback, this,
                &WebServerApplication::RequestHandlerCallback, this,
                MHD_OPTION_ARRAY, ops,
                MHD_OPTION_END);
#endif
    } else {        
        struct sockaddr *addr = NULL;
        if (_useIPV6) {
            struct sockaddr_in6 addrV6;
            addr = Helper::GetNetworkAddress((sockaddr *)&addrV6, _port, _bindToIP, true);
        } else {
            struct sockaddr_in addrV4;
            addr = Helper::GetNetworkAddress((sockaddr *)&addrV4, _port, _bindToIP);
        }
#ifdef WIN32
        _server = _pMHD_start_daemon(daemonFlags, _port, &WebServerApplication::AcceptPolicyCallback, this,
            &WebServerApplication::RequestHandlerCallback, this,
            MHD_OPTION_ARRAY, ops,
            MHD_OPTION_SOCK_ADDR, addr,
            MHD_OPTION_END);
#else
        _server = MHD_start_daemon(daemonFlags, _port, &WebServerApplication::AcceptPolicyCallback, this,
            &WebServerApplication::RequestHandlerCallback, this,
            MHD_OPTION_ARRAY, ops,
            MHD_OPTION_SOCK_ADDR, addr,
            MHD_OPTION_END);
#endif
    }
}

void WebServerApplication::StartHTTPSServer(uint8_t daemonFlags, struct MHD_OptionItem ops[]) {
    daemonFlags |= MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL;
#ifdef WEBSERVER_DEBUGMODE
    daemonFlags |= MHD_USE_DEBUG;
#endif /*WEBSERVER_DEBUGMODE*/
    _port = _configuration.HasKeyChain(_V_NUMERIC, false, 1, CONFIG_PORT) ?
            (uint16_t) _configuration.GetValue(CONFIG_PORT, false) : DEFAULTCONNECTIONHTTPSPORT;
    if (_configuration.HasKeyChain(V_STRING, false, 1, CONFIG_BINDTOIP))
        _bindToIP = (string) _configuration.GetValue(CONFIG_BINDTOIP, false);
    if (_bindToIP.empty()) {    //use all ips
#ifdef WIN32
        _server = _pMHD_start_daemon(daemonFlags, _port,
                &WebServerApplication::AcceptPolicyCallback, this,
                &WebServerApplication::RequestHandlerCallback, this,
                MHD_OPTION_ARRAY, ops,
                MHD_OPTION_HTTPS_MEM_KEY, _keySSL,
                MHD_OPTION_HTTPS_MEM_CERT, _certSSL,
                MHD_OPTION_END);
#else
        _server = MHD_start_daemon(daemonFlags, _port,
                &WebServerApplication::AcceptPolicyCallback, this,
                &WebServerApplication::RequestHandlerCallback, this,
                MHD_OPTION_ARRAY, ops,
                MHD_OPTION_HTTPS_MEM_KEY, _keySSL,
                MHD_OPTION_HTTPS_MEM_CERT, _certSSL,
                MHD_OPTION_END);
#endif
    } else {
        struct sockaddr *addr = NULL;
        if (_useIPV6) {
            struct sockaddr_in6 addrV6;
            addr = Helper::GetNetworkAddress((sockaddr *)&addrV6, _port, _bindToIP, true);
        } else {
            struct sockaddr_in addrV4;
            addr = Helper::GetNetworkAddress((sockaddr *)&addrV4, _port, _bindToIP);
        }
#ifdef WIN32
        _server = _pMHD_start_daemon(daemonFlags, _port,
            &WebServerApplication::AcceptPolicyCallback, this,
            &WebServerApplication::RequestHandlerCallback, this,
            MHD_OPTION_ARRAY, ops,
            MHD_OPTION_HTTPS_MEM_KEY, _keySSL,
            MHD_OPTION_HTTPS_MEM_CERT, _certSSL,
            MHD_OPTION_SOCK_ADDR, addr,
            MHD_OPTION_END);
#else
        _server = MHD_start_daemon(daemonFlags, _port,
            &WebServerApplication::AcceptPolicyCallback, this,
            &WebServerApplication::RequestHandlerCallback, this,
            MHD_OPTION_ARRAY, ops,
            MHD_OPTION_HTTPS_MEM_KEY, _keySSL,
            MHD_OPTION_HTTPS_MEM_CERT, _certSSL,
            MHD_OPTION_SOCK_ADDR, addr,
            MHD_OPTION_END);
#endif
    }
}

void WebServerApplication::Stop() {
    if (_server)
        StopDaemon(_server);
    INFO("Web Server stopped.");
}

uint8_t *WebServerApplication::LoadFile(char const *filename, bool isTempFile, uint64_t *fileSize) {
    File fp;
    if (!fp.Initialize(filename, FILE_OPEN_MODE_READ)) {
        if (isTempFile) {
            AutoMutex am(_temporaryFilesMutex);
            _temporaryFiles.push_back(filename);
        }
        return NULL;
    }

    uint64_t size = fp.Size();
    if (size == 0) {
        if (fileSize) {
            *fileSize = 0;  //let caller know it is just an empty file
        }
        return NULL;
    }

    uint8_t *buffer = new uint8_t[(uint32_t)size + 1]; // added coercion to get it to compile
    if (buffer == NULL)
        return NULL;

    if (!fp.ReadBuffer(buffer, size)) {
        delete[] buffer;
        return NULL;
    }

    buffer[size] = '\0';
    if (fileSize)
        *fileSize = size;
    return buffer;
}

/*
        The file generated by php-cgi is pre-pended with a couple of response headers. 
        PostProcessPHPFile takes out and stores these headers.
    
        Sample:

        X-Powered-By: PHP/5.5.15
        Content-Type:text/plain:charset=utf-8

        {"zcurl":"http:\/\/127.0.0.1:7777\/Version"}{"url":true}{"httpheader":true}{"xfer":true}{"timeout":true}{"zcurl$error":"Connection timed out after 10515 milliseconds","zcurl$err_no":28,"data":false}{"connected":false,"description":"<br \/> Either RMS is stopped or the firewall settings (and\/or SeLinux) of the target machine does not permit connection to port 7777. <br \/> Kindly consult the user manual."}*Trying to connect ....
        ...
        ...

 */
void WebServerApplication::PostProcessPHPFile(char const *filename, map<string, string> &phpCGIDerivedResponseHeader) {
    if (filename == NULL)
        return;
    uint64_t fileSize = 0;
    uint8_t *buffer = LoadFile(filename, true, &fileSize);
    if (buffer == NULL) //blank file
        return;
    uint8_t const headerLines = 2;
    char const lineDelimiter[] = LINEDELIMITER;

    //extract and remove header
    char *pchLine = strtok((char *) buffer, lineDelimiter);
    while (pchLine != NULL && phpCGIDerivedResponseHeader.size() < headerLines) {
        string line(pchLine);
        size_t pos = line.find_first_of(":");
        if (string::npos != pos)
            phpCGIDerivedResponseHeader[line.substr(0, pos)] = line.substr(pos + 1);
        pchLine = strtok(NULL, lineDelimiter);
    }

    //in case we have a file with only headers
    if (pchLine) {
        //update file contents
        File f;
        if (f.Initialize(filename, FILE_OPEN_MODE_TRUNCATE)) {
            if (f.IsOpen()) {
                if (pchLine) {
                    uint64_t count = fileSize - ((uint8_t*) pchLine - buffer);
                    f.WriteBuffer((uint8_t*) pchLine, count);
                }
            }
        }
    }
    delete buffer;
}

bool WebServerApplication::LoadLists(string const &whitelistFile, string const &blacklistFile) {
    uint8_t *whitelistBuffer = NULL, *blacklistBuffer = NULL;
    uint64_t fileSize = 1;
    if (!whitelistFile.empty()) {
        whitelistBuffer = LoadFile(STR(whitelistFile), false, &fileSize);
    }
    //fileSize==0 means a valid but empty file
    _useWhiteList = (whitelistBuffer != NULL || fileSize == 0);
    if (!blacklistFile.empty()) {
        fileSize = 1;
        blacklistBuffer = LoadFile(STR(blacklistFile), false, &fileSize);
    }
    _useBlackList = (blacklistBuffer != NULL || fileSize == 0);

    char const delimiter[] = LINEDELIMITER;

    char *pch;
    if (whitelistBuffer) {
        pch = strtok((char *) whitelistBuffer, delimiter);
        while (pch != NULL) {
            ADD_VECTOR_END(_whitelist, pch);
            pch = strtok(NULL, delimiter);
        }
    }
    if (blacklistBuffer) {
        pch = strtok((char *) blacklistBuffer, delimiter);
        while (pch != NULL) {
            ADD_VECTOR_END(_blacklist, pch);
            pch = strtok(NULL, delimiter);
        }
    }
    if (whitelistBuffer)
        delete whitelistBuffer;
    if (blacklistBuffer)
        delete blacklistBuffer;
    return (_useBlackList || _useWhiteList);
}

struct MHD_Response *WebServerApplication::CreateResponse(int code, bool needLicense) {
    static char const httpNotFound[] = "<html><head><title>RDKC Web Server</title></head><body>File not found</body></html>";
    static char const httpForbidden[] = "<html><head><title>RDKC Web Server</title></head><body>Forbidden</body></html>";
    static char const httpAccessDenied[] = "<html><head><title>RDKC Web Server</title></head><body>Access Denied</body></html>";
    static char const httpNeedLicense[] = "<html><head><title>RDKC Web Server</title></head><body><a href=\"/RMS_Web_UI/install_license.php\">Click to install a license.</a></body></html>";
    static char const httpRequestedRangeNotSatisfiable[] = "<html><head><title>RDKC Web Server</title></head><body>416: Requested Range Not Satisfiable</body></html>";
    char *text = NULL;
    if (needLicense) {
        text = (char *) httpNeedLicense;
    } else {
        switch (code) {
            case MHD_HTTP_NOT_FOUND:
                text = (char *) httpNotFound;
                break;
            case MHD_HTTP_FORBIDDEN:
                text = (char *) httpForbidden;
                break;
            case MHD_HTTP_UNAUTHORIZED:
                text = (char *) httpAccessDenied;
                break;
            case MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE:
                text = (char *) httpRequestedRangeNotSatisfiable;
                break;
            default:
                return NULL;
        }
    }
    return CreateResponseFromBuffer(strlen(text), (void *) text, MHD_RESPMEM_PERSISTENT);
}

int WebServerApplication::DestroyPostProcessor(struct MHD_PostProcessor **pp) {
#ifdef WIN32        
    int ret = _pMHD_destroy_post_processor(*pp);
#else
    int ret = MHD_destroy_post_processor(*pp);
#endif
    *pp = NULL;
    return ret;
}

int WebServerApplication::ProcessPost(struct MHD_PostProcessor *pp, const char *upload_data, size_t upload_data_size) {
#ifdef WIN32
    return _pMHD_post_process(pp, upload_data, upload_data_size);
#else
    return MHD_post_process(pp, upload_data, upload_data_size);
#endif
}

char *WebServerApplication::DigestAuthGetUserName(struct MHD_Connection *connection) {
#ifdef WIN32
    return _pMHD_digest_auth_get_username(connection);
#else
    return MHD_digest_auth_get_username(connection);
#endif
}

int WebServerApplication::QueueAuthFailResponse(struct MHD_Connection *connection, const char *realm, const char *opaque, 
    struct MHD_Response *response, int signal_stale) {
#ifdef WIN32
        return _pMHD_queue_auth_fail_response(connection, realm, opaque, response, signal_stale);
#else
        return MHD_queue_auth_fail_response(connection, realm, opaque, response, signal_stale);
#endif
}

int WebServerApplication::DigestAuthCheck(struct MHD_Connection *connection, const char *realm, const char *username,
    const char *password, unsigned int nonce_timeout) {
#ifdef WIN32
        return _pMHD_digest_auth_check(connection, realm, username, password, nonce_timeout);
#else
        return MHD_digest_auth_check(connection, realm, username, password, nonce_timeout);
#endif
}

char *WebServerApplication::BasicAuthGetUsernamePassword(
struct MHD_Connection *connection, char **password) {
#ifdef WIN32
    return _pMHD_basic_auth_get_username_password(connection, password);
#else
    return MHD_basic_auth_get_username_password(connection, password);
#endif
}

int WebServerApplication::QueueBasicAuthFailResponse(
struct MHD_Connection *connection, const char *realm, 
struct MHD_Response *response) {
#ifdef WIN32
    return _pMHD_queue_basic_auth_fail_response(connection, realm, response);
#else
    return MHD_queue_basic_auth_fail_response(connection, realm, response);
#endif
}

const union MHD_ConnectionInfo *WebServerApplication::GetConnectionInfo(struct MHD_Connection *connection, enum MHD_ConnectionInfoType info_type) {
#ifdef WIN32
    return _pMHD_get_connection_info(connection, info_type);
#else
    return MHD_get_connection_info(connection, info_type);
#endif
}

int WebServerApplication::QueueResponse(struct MHD_Connection *connection, uint32_t statusCode, struct MHD_Response *response) {
#ifdef WIN32
    return _pMHD_queue_response(connection, statusCode, response);
#else
    return MHD_queue_response(connection, statusCode, response);
#endif
}

void WebServerApplication::DestroyResponse(struct MHD_Response *response) {
#ifdef WIN32
    _pMHD_destroy_response(response);
#else
    MHD_destroy_response(response);
#endif
}

int WebServerApplication::AddResponseHeader(struct MHD_Response *response, char const *header, char const *content) {
#ifdef WIN32
    return _pMHD_add_response_header(response, header, content);
#else
    return MHD_add_response_header(response, header, content);
#endif
}

int WebServerApplication::DeleteResponseHeader(struct MHD_Response *response, char const *header, char const *content) {
#ifdef WIN32
    return _pMHD_delete_response_header(response, header, content);
#else
    return MHD_del_response_header(response, header, content);
#endif
}

char const *WebServerApplication::GetResponseHeader(struct MHD_Response *response, char const *header) {
#ifdef WIN32
    return _pMHD_get_response_header(response, header);
#else
    return MHD_get_response_header(response, header);
#endif
}

int WebServerApplication::GetResponseHeaders(struct MHD_Response *response, MHD_KeyValueIterator iterator, void *iterator_cls) {
#ifdef WIN32
    return _pMHD_get_response_headers(response, iterator, iterator_cls);
#else
    return MHD_get_response_headers(response, iterator, iterator_cls);
#endif
}

struct MHD_Response *WebServerApplication::CreateResponseFromCallback(uint64_t size, size_t block_size,
        MHD_ContentReaderCallback crc, void *crc_cls, MHD_ContentReaderFreeCallback crfc) {
#ifdef WIN32
    return _pMHD_create_response_from_callback(size, block_size, crc, crc_cls, crfc);
#else
    return MHD_create_response_from_callback(size, block_size, crc, crc_cls, crfc);
#endif
}

void WebServerApplication::StopDaemon(struct MHD_Daemon *daemon) {
#ifdef WIN32
    _pMHD_stop_daemon(daemon);
#else
    MHD_stop_daemon(daemon);
#endif
}

struct MHD_Response *WebServerApplication::CreateResponseFromBuffer(size_t size, void *buffer, enum MHD_ResponseMemoryMode mode) {
#ifdef WIN32
    return _pMHD_create_response_from_buffer(size, buffer, mode);
#else
    return MHD_create_response_from_buffer(size, buffer, mode);
#endif
}

int WebServerApplication::GetConnectionValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, MHD_KeyValueIterator iterator,
        void *iterator_cls) {
#ifdef WIN32
    return _pMHD_get_connection_values(connection, kind, iterator, iterator_cls);
#else
    return MHD_get_connection_values(connection, kind, iterator, iterator_cls);
#endif
}

int WebServerApplication::SetConnectionValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, char const *key, char const *value) {
#ifdef WIN32
    return _pMHD_set_connection_value(connection, kind, key, value);
#else
    return MHD_set_connection_value(connection, kind, key, value);
#endif
}

struct MHD_PostProcessor *WebServerApplication::CreatePostProcessor(struct MHD_Connection *connection, size_t buffer_size,
        MHD_PostDataIterator iter, void *iter_cls) {
#ifdef WIN32
    return _pMHD_create_post_processor(connection, buffer_size, iter, iter_cls);
#else
    return MHD_create_post_processor(connection, buffer_size, iter, iter_cls);
#endif
}

void WebServerApplication::GenerateRandomOutputFile(string &outputPath, string const &extension) {
    string tempOutputFile = Helper::GetTemporaryDirectory() + FILESEP + generateRandomString(32) + extension;
    while (fileExists(tempOutputFile)) {
        tempOutputFile = Helper::GetTemporaryDirectory() + FILESEP + generateRandomString(32) + extension;
    }
    outputPath = tempOutputFile;
}

bool WebServerApplication::StreamFile(string const &filePath, Session &ssn, 
	string const &identifier, bool isSession, string const& targetFileName, bool &isManifest) {
    if (_hasGroupNameAliases) {
        AutoMutex am(_groupNameAliasesMutex);
        FOR_MAP_CONST(_groupNameAliases, string, string, i) {
            string targetFolder = _webRootFolder + MAP_VAL(i);
            targetFolder = normalizePath(targetFolder, "");
            string streamType;
            string playListFile = GetPlayListFileFromStream(targetFolder, streamType, targetFileName, true);
			if (!playListFile.empty() && streamType.empty()) {
				return false;	//this is a media file possibly using GNA
			}
            if (filePath.compare(playListFile) == 0) { //accessing the master playlist
                AutoMutex am(_streamingSessionsMutex);
                string id = identifier;
                if (!MAP_HAS1(_streamingSessions, id)) {
					//send event
                    AutoMutex am(_evtLogMutex);
					Variant sessionInfo = CreateSessionInfo(ssn._ipAddress, playListFile, targetFolder, id);
					_pVariantHandler->SendEventLog(sessionInfo);
					return true;
				} else {
					string sid = _streamingSessions[id]._identifier;
					//we can have many aliases referencing the same target folder
					if (sid.compare(identifier) == 0) {
						string folder, name, ext;
						Helper::SplitFileName(filePath, folder, name, ext);
						if (MAP_HAS1(_mimes, ext)) {
							if (!_mimes[ext]._streamType.empty()) {
								isManifest = _mimes[ext]._isManifest;
							}
						}
						StreamingSession &streamSsn = _streamingSessions[id];
						if (isManifest) {
							streamSsn._chunkLength = GetChunkLengthFromManifest(filePath);
							DEBUG("Streaming Session Chunk Length Updated: (%s):(%d):(%s)", STR(filePath), streamSsn._chunkLength, STR(streamSsn._identifier));
						}
						streamSsn._elapsed = (uint32_t)(getutctime() - streamSsn._startTime);
						streamSsn._hasStartedStreaming = true;
						return true;
					}
				}
            } else {
                string folder, name, ext;
                Helper::SplitFileName(filePath, folder, name, ext);
                string target = folder + FILESEP + name;
                size_t pos = target.find(targetFolder);
                if (pos == 0) { //accessing the child playlist or segments
                    AutoMutex am(_streamingSessionsMutex);
                    //string id = targetFolder + "," + identifier;
                    string id = identifier;
					if (MAP_HAS1(_streamingSessions, id)) {
                        //update streaming session
                        StreamingSession &streamSsn = _streamingSessions[id];
                        if (!streamSsn._identifier.empty()) {   //wrong stream type if empty
                            streamSsn._elapsed = (uint32_t) (getutctime() - streamSsn._startTime);
							streamSsn._hasStartedStreaming = true;
                            //INFO("Streaming Session Updated: (%s)", STR(streamSsn._identifier)); //too noisy
							isManifest = true;
                            return true;
						}
					}
				}
            }
        }
    } else if (isSession) {
		isManifest = false;
		string folder, name, ext;
		Helper::SplitFileName(filePath, folder, name, ext);
		if (MAP_HAS1(_mimes, ext)) {
			if (_mimes[ext]._streamType.empty()) {
				return true;// false;
			}
			isManifest = _mimes[ext]._isManifest;
		}
        AutoMutex am(_streamingSessionsMutex);
        string id = /*filePath + */identifier;
		//see if it is actually for a child playlist
		bool found = MAP_HAS1(_streamingSessions, id);
		if (!found) {
			string lowerFolder;
			Helper::SplitFileName(folder, lowerFolder, name, ext);
			FOR_MAP_CONST(_streamingSessions, string, StreamingSession, iter) {
				string sessionTargetFolder = MAP_VAL(iter)._targetFolder;
				string sessionLowerFolder;
				Helper::SplitFileName(sessionTargetFolder, sessionLowerFolder, name, ext);
				if (lowerFolder.find(sessionLowerFolder) == 0) {
					found = true;
					id = MAP_KEY(iter);
					isManifest = true;
					break;
				}
			}
		}
		if (!found) {
			if (isManifest) {
                AutoMutex am(_evtLogMutex);
				//start streaming session
				Variant sessionInfo = CreateSessionInfo(ssn._ipAddress, filePath, filePath, id);
				//send event
				_pVariantHandler->SendEventLog(sessionInfo);
				return true;
			} else {
				return true;
			}
        } else {    //update streaming session
            StreamingSession &streamSsn = _streamingSessions[id];
            if (!streamSsn._identifier.empty()) {   //wrong stream type if empty
				if (isManifest) {
					streamSsn._chunkLength = GetChunkLengthFromManifest(filePath);
					DEBUG("Streaming Session Chunk Length Updated: (%s):(%d):(%s)", STR(filePath), streamSsn._chunkLength, STR(streamSsn._identifier));
				}
				streamSsn._hasStartedStreaming = true;
                streamSsn._elapsed = (uint32_t) (getutctime() - streamSsn._startTime);
                //INFO("Streaming Session Updated: (%s)", STR(streamSsn._alias)); //too noisy
                return true;
            }
        }   
    }
    
    return false;
}

Variant WebServerApplication::CreateSessionInfo(string const &ipAddress, string const &playlistFile, string const &targetFolder, string const &identifier) {
    static uint32_t ssnId = 1;
    StreamingSession streamSsn;
    streamSsn._ipAddress = ipAddress;
	streamSsn._chunkLength = GetChunkLengthFromManifest(playlistFile);
    streamSsn._playList = playlistFile;
    streamSsn._elapsed = 0;
    streamSsn._sessionId = ssnId++;
    streamSsn._startTime = getutctime();
    streamSsn._targetFolder = targetFolder;
    streamSsn._identifier = identifier;
    streamSsn._hasStartedStreaming = false;
    _streamingSessions[identifier] = streamSsn;
    INFO("Streaming Session Started --> Session Id: (%s)", STR(streamSsn._identifier));
    //send event
    Variant sessionInfo;
    sessionInfo["type"] = "streamingSessionStarted";
    sessionInfo["clientIP"] = streamSsn._ipAddress;
    sessionInfo["sessionId"] = streamSsn._sessionId;
    sessionInfo["playlist"] = streamSsn._playList;
    sessionInfo["startTime"] = Helper::ConvertToLocalSystemTime(streamSsn._startTime);
    return sessionInfo;
}

uint32_t WebServerApplication::GetChunkLengthFromManifest(string const& manifestPath) {
	string streamType;
	string folder, name, ext;
	Helper::SplitFileName(manifestPath, folder, name, ext);
	if (MAP_HAS1(_mimes, ext)) {
		if (_mimes[ext]._isManifest) {
			streamType = _mimes[ext]._streamType;
		}
	}
	uint32_t chunkLength = 0;
	if (!streamType.empty()) {
		if (streamType.compare(STREAMTYPE_HLS) == 0) {
			uint8_t *buffer = LoadFile(STR(manifestPath));
			if (buffer == NULL) {
				return DEFAULTCHUNKLENGTH;
			}
			char const lineDelimiter[] = LINEDELIMITER;
			string newText;
			char *pchLine = strtok((char *)buffer, lineDelimiter);
			while (pchLine != NULL) {
				string line(pchLine);
				// try hls
				static string const hls = "#EXT-X-TARGETDURATION:";
				size_t posDurationBegin = line.find(hls);
				if (posDurationBegin != string::npos) { //hls
					string cl = line.substr(hls.size());
					chunkLength = atol(cl.c_str());
					break;
				}
				pchLine = strtok(NULL, lineDelimiter);
			}
			delete buffer;
		} else if (streamType.compare(STREAMTYPE_MSS) == 0) {
			TiXmlDocument doc(manifestPath);
			if (doc.LoadFile()) {
				string sTimeScale;
				if (GetNodeValue(doc, "TimeScale", sTimeScale)) {
					string d;
					if (GetNodeValue(doc, "StreamIndex/c/d", d)) {
						chunkLength = atol(STR(d)) / atol(STR(sTimeScale));
					}
				}
			}
		} else if (streamType.compare(STREAMTYPE_DASH) == 0) {
			//dynamic
			TiXmlDocument doc(manifestPath);
			if (doc.LoadFile()) {
				string sTimeScale;
				if (GetNodeValue(doc, "Period/AdaptationSet/Representation/SegmentTemplate/timescale", sTimeScale)) {
					string d;
					if (GetNodeValue(doc, "Period/AdaptationSet/Representation/SegmentTemplate/SegmentTimeline/S/d", d)) {
						chunkLength = atol(STR(d)) / atol(STR(sTimeScale));
					}
				}
				if (chunkLength == 0) {
					//static
					string sTimeScale;
					if (GetNodeValue(doc, "Period/AdaptationSet/Representation/SegmentList/timescale", sTimeScale)) {
						string d;
						if (GetNodeValue(doc, "Period/AdaptationSet/Representation/SegmentList/duration", d)) {
							chunkLength = atol(STR(d)) / atol(STR(sTimeScale));
						}
					}
				}
			}
		} else if (streamType.compare(STREAMTYPE_HDS) == 0) {
			//there is no way to determine chunk length directly from an HDS manifest since the metadata is encoded
			chunkLength = DEFAULTCHUNKLENGTH;
		}
	}

	//cannot return 0 or session will immediately end
	//if chunk length cannot be deduced, use the default
	return (chunkLength == 0 ? DEFAULTCHUNKLENGTH : chunkLength);
}

string WebServerApplication::GetPlayListByGroupName(string const &groupName, 
    vector<string> &localStreamNames, string &streamType, 
    string const& targetFileName, bool isAlias) {
    string targetFolder = _webRootFolder + groupName;
    string playlist = GetPlayListFileFromStream(targetFolder, streamType, 
        targetFileName, isAlias);
    if (!playlist.empty() && !streamType.empty()) {
		if (streamType == STREAMTYPE_MSS) {
			string rest = playlist.substr(targetFolder.length() + 1);
			size_t pos = rest.find_first_of(FOLDERSEPARATORS);
			if (pos != string::npos) {
				string localStreamName = rest.substr(0, pos);
				if (!localStreamName.empty() && 
                    !VECTOR_HAS(localStreamNames, localStreamName)) {
                    ADD_VECTOR_END(localStreamNames, localStreamName);
                }
			}
		} else {
			uint8_t *buffer = LoadFile(STR(playlist));
			if (buffer == NULL) {
				return "";
			}

			size_t index = playlist.find_last_of(FOLDERSEPARATORS);
			if (index != string::npos) {
				string target1, target2;
				if (streamType == STREAMTYPE_HLS) {
					target1 = playlist.substr(index+1);
				} else if (streamType == STREAMTYPE_HDS) {
					target1 = "href=\"";
                    target2 = "url=\"";
				} else if (streamType == STREAMTYPE_MSS) {
					target1 = "Url=\"";
				} else if (streamType == STREAMTYPE_DASH) {
					target1 = "media=\"";
				}
				char const lineDelimiter[] = LINEDELIMITER;
				string newText;
				char *pchLine = strtok((char *) buffer, lineDelimiter);
				while (pchLine != NULL) {
					string line(pchLine);
					size_t indexTarget = line.find(target1);
                    string localStreamName;
					if (indexTarget != string::npos) {
						if (streamType == STREAMTYPE_HLS) {
							localStreamName = line.substr(0, indexTarget - 1);
						} else if (streamType == STREAMTYPE_HDS || streamType == STREAMTYPE_DASH) {
							localStreamName = line.substr(indexTarget + target1.size());
							size_t end = localStreamName.find(FOLDERSEPARATORLIN);
							if (end != string::npos) {
								localStreamName = localStreamName.substr(0, end);
							}
						}
                        if (!localStreamName.empty() && 
                            !VECTOR_HAS(localStreamNames, localStreamName)) {
                            ADD_VECTOR_END(localStreamNames, localStreamName);
                        }
					} else if (!target2.empty()) {
                        if (streamType == STREAMTYPE_HDS) {
                            indexTarget = line.find(target2);
                            if (indexTarget != string::npos) {
                                localStreamName = line.substr(indexTarget + target2.size());
                                size_t end = localStreamName.find(FOLDERSEPARATORLIN);
                                if (end != string::npos) {
                                    localStreamName = localStreamName.substr(0, end);
                                    if (!localStreamName.empty() &&
                                        !VECTOR_HAS(localStreamNames, localStreamName)) {
                                        ADD_VECTOR_END(localStreamNames, localStreamName);
                                    }
                                }
                            }
                        }    
                    }
					pchLine = strtok(NULL, lineDelimiter);
				}
			}
			delete buffer;
		}
    }
    return playlist;
}

string WebServerApplication::GetPlayListFileFromStream(
    string const &targetFolder, string &streamType, 
    string const& targetFileName, bool isAlias) {
    //we can only make assumption on the name of the playlist file
    //since the type (HLS,HDS, etc) is not provided in the addgroupnamealias call
    string playListFile;
    string groupNameFolder = targetFolder + FILESEP;
    groupNameFolder = normalizePath(groupNameFolder, "");
    vector<string> files;
    listFolder(groupNameFolder, files, true, false, false);
    if (files.empty()) {	//this happens for mss
		//try next folder
		listFolder(groupNameFolder, files, true, true, false);
		if (!files.empty()) {
			groupNameFolder = files[0] + FILESEP;
			files.clear();
			listFolder(groupNameFolder, files, true, false, false);
		}
	}
	if (!files.empty()) {
        FOR_VECTOR(files, i) {
            string file = files[i];
            string folder, name, ext;
            Helper::SplitFileName(file, folder, name, ext);
            string tfn = format("%s.%s", STR(name), STR(ext));
            bool isMatch = isAlias ? true : tfn.compare(targetFileName) == 0;
            if (isMatch) {
                if (MAP_HAS1(_mimes, ext)) {
                    if (_mimes[ext]._isManifest) {
                        streamType = _mimes[ext]._streamType;
                        playListFile = file;
                        break;
                    } else if (_mimes[ext]._streamType.empty()) { //media file
						playListFile = file;
						break;
					}
                }
            }
        }
    }
    
    return playListFile;
}

bool WebServerApplication::IsStreamingFile(string const &filePath) {
    string folder, name, ext;
    Helper::SplitFileName(filePath, folder, name, ext);
    if (ext.empty())
        return true;
    return (ext.compare("xml") == 0) ? false : (MAP_HAS1(_mimes, ext));
}

bool WebServerApplication::IsMediaFile(string const &filePath) {
    string folder, name, ext;
    Helper::SplitFileName(filePath, folder, name, ext);
    bool isDashMp4 = false;
    size_t pos = filePath.find_last_of(FOLDERSEPARATORS);
    if (pos != string::npos) {
        name = filePath.substr(pos + 1);
        isDashMp4 = (name.compare("seg_init.mp4") == 0);
    }
    return (ext.empty() || isDashMp4) ? false : 
        (find(_supportedNonStreamMediaFiles.begin(), _supportedNonStreamMediaFiles.end(), ext) != 
            _supportedNonStreamMediaFiles.end());
}

map<string, string> WebServerApplication::LoadUsersFromDigestFile(string const &digestFile) {
	map<string, string> auths;
	string digestFileTmp = normalizePath(digestFile, "");
	uint8_t *buffer = LoadFile(STR(digestFileTmp));
	if (buffer == NULL) { //blank file
		return auths;
	}

	char const lineDelimiter[] = LINEDELIMITER;

	//extract and remove header
	char *pchLine = strtok((char *)buffer, lineDelimiter);
	while (pchLine != NULL) {
		string line(pchLine);
		size_t pos = line.find_first_of(":");
		if (string::npos != pos) {
			auths[line.substr(0, pos)] = line.substr(pos + 1);
		}
		pchLine = strtok(NULL, lineDelimiter);
	}
	return auths;
}
