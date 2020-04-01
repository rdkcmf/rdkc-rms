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


#ifndef _HTTPSERVERAPPLICATION_H
#define _HTTPSERVERAPPLICATION_H

#include "application/baseclientapplication.h"
#include "protocols/timer/basetimerprotocol.h"
#include "protocols/variant/webservervariantappprotocolhandler.h"

#ifdef WIN32
#include <winsock2.h> // socket related definitions
#include <ws2tcpip.h> // socket related definitions
#define _SSIZE_T_DEFINED //  5/15/15: does this work?
typedef SSIZE_T ssize_t; //convey Windows type to unix type
typedef UINT64 uint64_t; //convey Windows type to unix type
typedef UINT16 uint16_t; //convey Windows type to unix type
#endif
#include <microhttpd.h>
#include "memcache.h"
#include "../../3rdparty/tinyxml/tinyxml.h"
#include "application/helper.h"

namespace webserver {

    class DLLEXP WebServerApplication :
    public BaseClientApplication, 
    public BaseTimerProtocol {
    private:
        struct Session;

        class Metric {
        public:
            static uint16_t _activeThreads;
            static uint16_t _tps;    
        public:
            Metric() {
                Helper::AtomicIncrement(&_activeThreads);
                DEBUG("Metric:Active Threads(%"PRIu16"/%"PRIu16")", _activeThreads, _tps);        
            }

            ~Metric() {
                Helper::AtomicDecrement(&_activeThreads);
                DEBUG("Metric:Active Threads(%"PRIu16"/%"PRIu16")", _activeThreads, _tps);
            }
        };
        
        struct ContentReaderContext {
            void *_reader;
            WebServerApplication *_app;
            string _ipAddress;
            Session *_ssn;
            time_t _begin;
            time_t _end;
            bool _isTemporaryFile;
            bool _isMediaFile;
			bool _isRange;
			string _urlFile;
            string _originalFile;
			uint64_t _fileOffset;
        };

        struct SupportedMimeType {
            string _extensions;
            string _mimeType;
            string _streamType;
            bool _isManifest;
        };

        struct IncludeResponseHeader {
            string _header;
            string _content;
            bool _override;
        };

		struct Authentication {
			string _domain;
			string _digestFile;
			bool _enable;
			string _type;	//always basic for now
			map<string /*username*/, string /*password*/> _users;
			Authentication() {
				_type = "basic";
			}
		};

        struct APIProxy {
            string _pseudoDomain;
            string _address;
            uint16_t _port;
            string _authentication;
            string _userName;
            string _password;
        };

    private: //config-related attributes
        string _sslMode;
        uint32_t _maxMemSizePerConnection;
        bool _useIPV6;
        bool _enableIPFilter;
        bool _enableCache;
		uint16_t _mediaFileDownloadTimeout;
        string _webRootFolder;
        string _tempFolder;
        uint16_t _port;
        string _bindToIP;
        bool _hasGroupNameAliases;
		bool _enableRangeRequests;
        bool _webUiLicenseOnly;
        vector<string> _supportedNonStreamMediaFiles;
        APIProxy _apiProxy;
    private:
        struct MHD_Daemon *_server;
        uint8_t *_keySSL, *_certSSL;
        mutable vector<string> _whitelist, _blacklist;

        struct Session {
            int _id;
            stringstream* _postData;
            string _ipAddress;
            uint16_t _httpCode;
            MHD_PostProcessor *_postProc;
            map<string, string> _postGetParams;
            File _upload;
        };

        struct StreamingSession {   //id'ed as targetFolder+alias
            uint32_t _sessionId;
            string _ipAddress;
            time_t _startTime, _stopTime;
            uint32_t _elapsed;
            uint32_t _chunkLength;
            string _targetFolder;
            string _playList;
            string _identifier;
            bool _hasStartedStreaming;
        };

        map<string/*target folder*/, StreamingSession> _streamingSessions;
        map<int /*id*/, Session *> _sessions;
        vector<string /*ipaddress*/> _connections;
        MemCache *_memCache;
        map<string/*extension*/, SupportedMimeType> _mimes;
        map<string/*header*/, IncludeResponseHeader> _incRespHdrs;
		map<string/*domain*/, Authentication> _auths;
        vector<string/*<groupname><localstreamname>*/> _grplsNameForCache;
        Mutex *_sessionsMutex, *_streamingSessionsMutex, 
            *_groupNameAliasesMutex, *_temporaryFilesMutex, *_evtLogMutex,
            *_connsMutex;
        map<string, string> _groupNameAliases;
        WebServerVariantAppProtocolHandler *_pVariantHandler;
        TCPAcceptor *_pAcceptor;
        bool _newVariantHandler;
        bool _enableAPIProxy;
		vector<string> _temporaryFiles;
		map<string, Variant> _mediaFileLastAccess;
        bool _useWhiteList, _useBlackList;
        int _parentProcessId;
#ifdef WIN32
		HANDLE _parentHandle;
#endif
#ifdef WIN32
        HMODULE _hMod;
        typedef const union MHD_ConnectionInfo *(WINAPI *PMHDGCI)(struct MHD_Connection *, enum MHD_ConnectionInfoType, ...);
        PMHDGCI _pMHD_get_connection_info;
        typedef int (WINAPI *PMHDARH)(struct MHD_Response *, char const *, char const *);
        PMHDARH _pMHD_add_response_header;
        typedef int (WINAPI *PMHDDRH)(struct MHD_Response *, char const *, char const *);
        PMHDDRH _pMHD_delete_response_header;
        typedef char const * (WINAPI *PMHDGRH)(struct MHD_Response *, char const *);
        PMHDGRH _pMHD_get_response_header;
        typedef int (WINAPI *PMHDGRHS)(struct MHD_Response *, MHD_KeyValueIterator, void *);
        PMHDGRHS _pMHD_get_response_headers;
        typedef struct MHD_Response * (WINAPI *PMHDCRFC)(uint64_t, size_t, MHD_ContentReaderCallback,
                void *, MHD_ContentReaderFreeCallback);
        PMHDCRFC _pMHD_create_response_from_callback;
        typedef void (WINAPI *PMHDDR)(struct MHD_Response *);
        PMHDDR _pMHD_destroy_response;
        typedef int (WINAPI *PMHDQR)(struct MHD_Connection *, uint32_t, struct MHD_Response *);
        PMHDQR _pMHD_queue_response;
        typedef struct MHD_Daemon * (WINAPI *PMHDSD)(uint32_t, uint16_t, MHD_AcceptPolicyCallback, void *,
                MHD_AccessHandlerCallback, void *, ...);
        PMHDSD _pMHD_start_daemon;
        typedef struct MHD_Response * (WINAPI *PMHDCRFB)(size_t, void *, enum MHD_ResponseMemoryMode);
        PMHDCRFB _pMHD_create_response_from_buffer;
        typedef void (WINAPI *PMHDSTD)(struct MHD_Daemon *);
        PMHDSTD _pMHD_stop_daemon;
        typedef int (WINAPI *PMHDGCV)(struct MHD_Connection *, enum MHD_ValueKind, MHD_KeyValueIterator, void *);
        PMHDGCV _pMHD_get_connection_values;
        typedef int (WINAPI *PMHDSCV)(struct MHD_Connection *, enum MHD_ValueKind, const char *, const char *);
        PMHDSCV _pMHD_set_connection_value;
        typedef struct MHD_PostProcessor *(WINAPI *PMHDCPP)(struct MHD_Connection *, size_t, MHD_PostDataIterator, void *);
        PMHDCPP _pMHD_create_post_processor;
        typedef int (WINAPI *PMHDPP)(struct MHD_PostProcessor *, const char *, size_t);
        PMHDPP _pMHD_post_process;
        typedef int (WINAPI *PMHDDPP)(struct MHD_PostProcessor *);
        PMHDDPP _pMHD_destroy_post_processor;
        typedef char * (WINAPI *PMHDDAGU)(struct MHD_Connection *);
        PMHDDAGU _pMHD_digest_auth_get_username;
        typedef int (WINAPI *PMHDQAFR)(struct MHD_Connection *, const char *, const char *, struct MHD_Response *, int);
        PMHDQAFR _pMHD_queue_auth_fail_response;
        typedef int (WINAPI *PMHDDAC)(struct MHD_Connection *, const char *, const char *, const char *, unsigned int);
        PMHDDAC _pMHD_digest_auth_check;
        typedef char * (WINAPI *PMHDBAGU)(struct MHD_Connection *, char **);
        PMHDBAGU _pMHD_basic_auth_get_username_password;
        typedef int (WINAPI *PMHDQBAFR)(struct MHD_Connection *, const char *, 
        struct MHD_Response *);
        PMHDQBAFR _pMHD_queue_basic_auth_fail_response;
#endif
    public:
        WebServerApplication(Variant &configuration);
        WebServerApplication();
        virtual ~WebServerApplication();
        virtual bool Initialize();
        virtual bool TimePeriodElapsed();
        bool ProcessOriginMessage(Variant &message);
        virtual void RegisterProtocol(BaseProtocol *pProtocol);
        virtual void UnRegisterProtocol(BaseProtocol *pProtocol);
        virtual bool ActivateAcceptor(IOHandler *pIOHandler);
    protected:
        virtual void Stop();
    private:
        /*struct MHD_Daemon *StartDaemon(unsigned int flags, uint16_t port, MHD_AcceptPolicyCallback apc, void *apc_cls,
                MHD_AccessHandlerCallback dh, void *dh_cls, ...);*/
        void StopDaemon(struct MHD_Daemon *daemon);
        int GetConnectionValues(struct MHD_Connection *connection, enum MHD_ValueKind kind, MHD_KeyValueIterator iterator,
                void *iterator_cls);
        int SetConnectionValue(struct MHD_Connection *connection, enum MHD_ValueKind kind, char const *key, char const *value);
        struct MHD_Response *CreateResponseFromBuffer(size_t size, void *buffer, enum MHD_ResponseMemoryMode mode);
        struct MHD_Response *CreateResponseFromCallback(uint64_t size, size_t block_size, MHD_ContentReaderCallback crc,
                void *crc_cls, MHD_ContentReaderFreeCallback crfc);
        int AddResponseHeader(struct MHD_Response *response, char const *header, char const *content);
        int DeleteResponseHeader(struct MHD_Response *response, char const *header, char const *content);
        char const *GetResponseHeader(struct MHD_Response *response, char const *header);
        int GetResponseHeaders(struct MHD_Response *response, MHD_KeyValueIterator iterator, void *iterator_cls);
        void DestroyResponse(struct MHD_Response *response);
        int QueueResponse(struct MHD_Connection *connection, uint32_t statusCode, struct MHD_Response *response);
        const union MHD_ConnectionInfo *GetConnectionInfo(struct MHD_Connection *connection, enum MHD_ConnectionInfoType info_type);
        struct MHD_PostProcessor *CreatePostProcessor(struct MHD_Connection *connection, size_t buffer_size,
                MHD_PostDataIterator iter, void *iter_cls);
        int DestroyPostProcessor(struct MHD_PostProcessor **pp);
        int ProcessPost(struct MHD_PostProcessor *pp, const char *upload_data, size_t upload_data_size);
        char *DigestAuthGetUserName(struct MHD_Connection *connection);
        int QueueAuthFailResponse(struct MHD_Connection *connection, const char *realm, const char *opaque, 
                struct MHD_Response *response, int signal_stale);
        int DigestAuthCheck(struct MHD_Connection *connection, const char *realm, const char *username,
                const char *password, unsigned int nonce_timeout);
        char *BasicAuthGetUsernamePassword(struct MHD_Connection *connection, 
            char **password);
        int QueueBasicAuthFailResponse(struct MHD_Connection *connection, 
            const char *realm, struct MHD_Response *response);
#ifdef WIN32
        bool LoadLibraryAndFunctions();
#endif
        struct MHD_Response *CreateResponse(int code, bool needLicense);
        bool LoadLists(string const &whitelistFile, string const &blacklistFile);
        string GetGroupAndLocalStreamNameFromPath(string const& filePath);
        void StartHTTPServer(uint8_t daemonFlags, struct MHD_OptionItem ops[]);
        void StartHTTPSServer(uint8_t daemonFlags, struct MHD_OptionItem ops[]);
        bool StreamFile(string const &filePath, Session &ssn, 
            string const &identifier, bool isSession, 
            string const& targetFileName, bool &isManifest);
        string GetPlayListByGroupName(string const &groupName, 
            vector<string> &localStreamNames, string &streamType, 
            string const& targetFileName, bool isAlias);
        string GetPlayListFileFromStream(string const &targetFolder, string &streamType, string const& targetFileName, bool isAlias);
        bool IsAliasAvailable(string const &clientIPAddress, string const &alias);
        void CreateVariantHandler();
        bool IsStreamingFile(string const &filePath);
        bool IsMediaFile(string const &filePath);
        bool IsProxyCall(string const &url, struct MHD_Connection *connection, 
            string &outputPath, bool &authFail);
		bool Authenticate(string const &url, struct MHD_Connection *connection, Authentication auth, string &userName);
        string GetSessionFilePath(string &urlFile, bool returnAlias = false);
        Variant CreateSessionInfo(string const &ipAddress, string const &playlistFile, string const &targetFolder, string const &identifier);
        uint32_t GetChunkLengthFromManifest(string const& manifestPath);
		void DeleteTemporaryFiles();
    private: //callbacks
        static ssize_t FileContentReaderCallback(void *cls, uint64_t pos, char *buf, size_t max);
        static void FileFreeResourceCallback(void *cls);
        static ssize_t CacheContentReaderCallback(void *cls, uint64_t pos, char *buf, size_t max);
        static void CacheFreeResourceCallback(void *cls);
        static int RequestHandlerCallback(void *cls, struct MHD_Connection *connection,
                char const *url, char const *method, char const *version, char const *upload_data,
                size_t *upload_data_size, void **ptr);
        static int AcceptPolicyCallback(void *cls, struct sockaddr const *addr, socklen_t addrlen);
        static void RequestCompletedCallback(void *cls, struct MHD_Connection *connection,
                void **con_cls, enum MHD_RequestTerminationCode toe);
        static void Logger(void *arg, char const *fmt, va_list ap);
        static void NotifyConnectionCallback(void *cls, struct MHD_Connection *connection, void **socket_context, enum MHD_ConnectionNotificationCode toe);
        static int KeyValuePairsCallback(void *cls, enum MHD_ValueKind kind, char const *key, char const *value);
        static int PostIteratorCallback(void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type,
                const char *transfer_encoding, const char *data, uint64_t off, size_t size);
    private:
        void GenerateRandomOutputFile(string &outputFile, string const &extension);
        uint8_t *LoadFile(char const *filename, bool isTempFile = false, uint64_t *fileSize = NULL);
        bool isVOD(File *file, SupportedMimeType smt);
        int GenerateResponse(struct MHD_Connection *connection, int code, bool needLicense = false);
        void PostProcessPHPFile(char const *filename, map<string, string> &phpCGIDerivedResponseHeader);
        bool RunPHPInterpreter(string const& inputPHPFile, string &outputPHPFile, bool isGet, map<string, string> const& params,
                map<string, string> &derivedResponseHeader, string const &program, string const &clientIPAddress);
		static bool GetRanges(string const &ranges, uint64_t &start, uint64_t &end);
        bool CopyFile(string const &src, string const &dst);
		static bool GetNodeValue(TiXmlDocument &doc, string const& node, string &value);
		map<string, string> LoadUsersFromDigestFile(string const &digestFile);
	};
}
#endif /* _HTTPSERVERAPPLICATION_H */

