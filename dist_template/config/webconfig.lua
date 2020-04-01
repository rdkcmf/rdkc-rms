--[[remove
	__STANDARD_WEBSERVER_LOG_FILE_APPENDER__ - ../logs/rms-webserver
	__STANDARD_WHITELIST_FILE__ - ../config/whitelist.txt
	__STANDARD_BLACKLIST_FILE__ - ../config/blacklist.txt
	__STANDARD_SERVERKEY_FILE__ - ../config/server.key
	__STANDARD_SERVERCERT_FILE__ - ../config/server.cert
	__STANDARD_WEBROOT_FOLDER__ - ../rms-webroot
remove]]--
configuration=
{
	logAppenders=
	{
		{
			name="console appender",
			type="coloredConsole",
			level=6
		},
		{
			name="file appender",
			type="delayedFile",
			level=6,
			fileName="__STANDARD_WEBSERVER_LOG_FILE_APPENDER__",
			newLineCharacters="\n",
			fileHistorySize=100,
			fileLength=1024*1024,
			singleLine=true
		},
	},
	applications=
	{
		rootDirectory="./",
		{
			name="webserver",
			description="Built-In Web Server",
			port=8888,
			rmsPort=1113,		--should match config.lua's inboundBinVariant acceptor
			bindToIP="",
			sslMode="disabled", -- always, auto, disabled
			maxMemSizePerConnection=32*1024, --32*1024,
			maxConcurrentConnections=5000,
			connectionTimeout=0, -- 0 - no timeout
			maxConcurrentConnectionsSameIP=1000,
			threadPoolSize=8,
			useIPV6=false,
			enableIPFilter=false,	--if true, reads white and black lists
			whitelistFile="__STANDARD_WHITELIST_FILE__",
			blacklistFile="__STANDARD_BLACKLIST_FILE__",
			sslKeyFile="__STANDARD_SERVERKEY_FILE__",
			sslCertFile="__STANDARD_SERVERCERT_FILE__",
			enableCache=false,
			cacheSize=1*1024*1024*1024,	--1GB
			hasGroupNameAliases=false,
			webRootFolder="__STANDARD_WEBROOT_FOLDER__",
			enableRangeRequests=true,
			mediaFileDownloadTimeout=30,
			supportedMimeTypes=
			{
				-- non-streaming
				{	
					extensions="mp4,mp4v,mpg4",
					mimeType="video/mp4",
					streamType="",
					isManifest=false,
				},
				{
					extensions="flv",
					mimeType="video/x-flv",
					streamType="",
					isManifest=false,
				},
				-- streaming
				{
					extensions="m3u,m3u8",
					mimeType="audio/x-mpegurl",
					streamType="hls",
					isManifest=true,
				},
				{
					extensions="ts",
					mimeType="video/mp2t",
					streamType="hls",
					isManifest=false,
				},
				{
					extensions="aac",
					mimeType="audio/aac",
					streamType="hls",
					isManifest=false,
				},
				{
					extensions="f4m",
					mimeType="application/f4m+xml",
					streamType="hds",
					isManifest=true,
				},
				{
					extensions="ismc,isma,ismv",
					mimeType="application/octet-stream",
					streamType="mss",
					isManifest=true,
				},
				{
					extensions="fmp4",
					mimeType="video/mp4",
					streamType="mss",
					isManifest=false,
				},
				{
					extensions="mpd",
					mimeType="application/dash+xml",
					streamType="dash",
					isManifest=true,
				},
				{
					extensions="m4s",
					mimeType="video/mp4",
					streamType="dash",
					isManifest=false,
				},
				{ -- needed for supporting adobe player's crossdomain.xml
					extensions="xml",
					mimeType="application/xml",
					streamType="",
					isManifest=false,
				},
			},
			--[[
			includeResponseHeaders=
			{
				{
					header="Access-Control-Allow-Origin",
					content="*",
					override=true,
				},
				{
					header="User-Agent",
					content="RDKC",
					override=false,
				},
			},
			]]--
			--[[
			apiProxy=
			{
				authentication="basic", -- none, basic			
				pseudoDomain="<domain>",
				address="127.0.0.1",
				port=7777,
				userName="<username>",
				password="<password>",
			},
			]]--
			--[[
			auth=
			{
				{
					domain="RMS_Web_UI", --the domain folder
     				digestFile="../rms-webroot/RMS_Web_UI/settings/passwords/.htdigest", --relative path to digest file
     				enable=false,
				},
			},
			]]--
		},
	}
}

