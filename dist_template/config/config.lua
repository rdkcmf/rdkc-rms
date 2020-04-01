--[[remove
	__STANDARD_LOG_FILE_APPENDER__ - ../logs/rms
	__STANDARD_TIME_PROBE_APPENDER__ - ../logs/timeprobe
	__STANDARD_MEDIA_FOLDER__ - ../media
	__STANDARD_CONFIG_PERSISTANCE_FILE__ - ../config/pushPullSetup.xml
	__STANDARD_AUTH_PERSISTANCE_FILE__ - ../config/auth.xml
	__STANDARD_CONN_LIMITS_PERSISTANCE_FILE__ - ../config/connlimits.xml
	__STANDARD_BW_LIMITS_PERSISTANCE_FILE__ - ../config/bandwidthlimits.xml
	__STANDARD_INGEST_POINTS_PERSISTANCE_FILE__ - ../config/ingestpoints.xml
	__STANDARD_AUTO_DASH_FOLDER__ - ../rms-webroot
	__STANDARD_AUTO_HLS_FOLDER__ - ../rms-webroot
	__STANDARD_AUTO_HDS_FOLDER__ - ../rms-webroot
	__STANDARD_AUTO_MSS_FOLDER__ - ../rms-webroot
	__STANDARD_RTMP_USERS_FILE__ - ../config/users.lua
	__STANDARD_RTSP_USERS_FILE__ - ../config/users.lua
	__STANDARD_EVENT_FILE_APPENDER__ - ../logs/events.txt
	__STANDARD_TRANSCODER_SCRIPT__ - rmsTranscoder.sh
	__STANDARD_MP4WRITER_BIN__ - ./rms-mp4writer
	__STANDARD_SERVERKEY_FILE__ - ../config/server.key
	__STANDARD_SERVERCERT_FILE__ - ../config/server.cert
	__STANDARD_TURSTEDCERTS_FILE__ - ../config/trustedCerts.db
remove]]--
configuration=
{
	daemon=false,
	instancesCount=-1,
	pathSeparator="/",
	timeProbeFilePathPrefix="__STANDARD_TIME_PROBE_APPENDER__",
	logAppenders=
	{
		{
			name="console appender",
			type="coloredConsole",
			level=6
		},
		{
			name="file appender",
			type="file",
			level=6,
			fileName="__STANDARD_LOG_FILE_APPENDER__",
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
			appDir="./",
			name="rdkcms",
			description="RDKC MEDIA SERVER",
			protocol="dynamiclinklibrary",
			default=true,
			pushPullPersistenceFile="__STANDARD_CONFIG_PERSISTANCE_FILE__",
			authPersistenceFile="__STANDARD_AUTH_PERSISTANCE_FILE__",
			connectionsLimitPersistenceFile="__STANDARD_CONN_LIMITS_PERSISTANCE_FILE__",
			bandwidthLimitPersistenceFile="__STANDARD_BW_LIMITS_PERSISTANCE_FILE__",
			ingestPointsPersistenceFile="__STANDARD_INGEST_POINTS_PERSISTANCE_FILE__",
			streamsExpireTimer=10,
			rtcpDetectionInterval=15,
			hasStreamAliases=false,
			hasIngestPoints=false,
			validateHandshake=false,
			aliases={"er", "live", "vod"},
			maxRtmpOutBuffer=512*1024,
			hlsVersion=3,
			useSourcePts=false,
			enableCheckBandwidth=true,
			vodRedirectRtmpIp="",
			forceRtmpDatarate=false,
			mediaStorage = {
				recordedStreamsStorage="__STANDARD_MEDIA_FOLDER__",
				{
					description="Default media storage",
					mediaFolder="__STANDARD_MEDIA_FOLDER__",
				},
				--[[
				-- the following is an example and contains all
				-- available properties along with their default
				-- values(except paths, of course)
				sample={
					description="Storage example",
					mediaFolder="/some/media/folder",
					metaFolder="/fast/discardable/separate/folder",
					enableStats=false,
					clientSideBuffer=15,
					keyframeSeek=true,
					seekGranularity=0.1,
					externalSeekGenerator=false,
				}
				]]--
			},
			acceptors=
			{
				-- CLI acceptors
				{
					ip="127.0.0.1",
					port=1112,
					protocol="inboundJsonCli",
					useLengthPadding=true
				},
				{
					ip="127.0.0.1",
					port=7777,
					protocol="inboundHttpJsonCli"
				},
				{
					ip="127.0.0.1",
					port=9999,
					protocol="inboundHttpFmp4"
				},
				{
					ip="127.0.0.1",
					port=1222,
					protocol="inboundAsciiCli",
					useLengthPadding=true
				},

				-- RTMP and clustering
				{
					ip="0.0.0.0",
					port=1935,
					protocol="inboundRtmp",
				},
				{
					ip="127.0.0.1",
					port=1936,
					protocol="inboundRtmp",
					clustering=true
				},
				{
					ip="127.0.0.1",
					port=1113,
					protocol="inboundBinVariant",
					clustering=true
				},

				-- RTSP
				{
					ip="0.0.0.0",
					port=5544,
					protocol="inboundRtsp",
					--[[
					multicast=
					{
						ip="224.2.1.39",
						ttl=127,
					}
					]]--
				},

				-- LiveFLV ingest
				{
					ip="0.0.0.0",
					port=6666,
					protocol="inboundLiveFlv",
					waitForMetadata=true,
				},

				--[[
				-- Inbound TCP TS
				{
					ip="0.0.0.0",
					port=9999,
					protocol="inboundTcpTs",
				},

				-- Inbound UDP TS
				{
					ip="0.0.0.0",
					port=9999,
					protocol="inboundUdpTs",
				},
				]]--

				-- HTTP
				{
					ip="0.0.0.0",
					port=8080,
					protocol="inboundHttp",
				},

				--RTMPS
				--[[
				{
					ip="0.0.0.0",
					port=4443,
					protocol="inboundRtmp",
					-- cipherSuite="!DEFAULT:RC4-SHA", -- enables/disables a specific set of ciphers. If not specified, it is defaulted to the openssl collection of ciphers
					sslKey="/path/to/some/key",
					sslCert="/path/to/some/cert",
				},
				]]--
				-- JSONMETA
				{
					ip="0.0.0.0",
					port=8100,
					--streamname="test",
					protocol="inboundJsonMeta",
				},
				-- WebSockets JSON Metadata
				{
					ip="0.0.0.0",
					port=8210,
					protocol="wsJsonMeta",
					-- streamname="~0~0~0~"
				},
				-- WebSockets FMP4 Fetch
				{
					ip="0.0.0.0",
					port=8410,
					protocol="inboundWSFMP4"
				},
			},
			--[[
			autoDASH=
			{
				targetFolder= "__STANDARD_AUTO_DASH_FOLDER__",
			},
			autoHLS=
			{
				targetFolder= "__STANDARD_AUTO_HLS_FOLDER__",
			},
			autoHDS=
			{
				targetFolder= "__STANDARD_AUTO_HDS_FOLDER__",
			},
			autoMSS=
			{
				targetFolder= "__STANDARD_AUTO_MSS_FOLDER__",
			},
			]]--
			--[[
			authentication=
			{
				rtmp=
				{
					type="adobe",
					encoderAgents=
					{
						"FMLE/3.0 (compatible; FMSc/1.0)",
						"Wirecast/FM 1.0 (compatible; FMSc/1.0)",
						"RDKC Media Server (www.comcast.com)"
					},
					usersFile="__STANDARD_RTMP_USERS_FILE__"
				},
				rtsp=
				{
					usersFile="__STANDARD_RTSP_USERS_FILE__",
					--authenticatePlay=false,
				}
			},
			]]--
			eventLogger=
			{
				-- the following customData node will be appended to all outgoing
				-- events generated by this logger.
				--customData=123,
				sinks=
				{
					--[=[
					{
						type="file",
						-- the following customData node will be apended to all
						-- events generated by this sink. It overwrides the
						-- customData node defined on the upper level. It can
						-- also be a complex structure like this
						--[[
						customData =
						{
							some="string",
							number=123.456,
							array={1, 2.345, "Hello world", true, nil}
						},
						]]--
						filename="__STANDARD_EVENT_FILE_APPENDER__",
						format="text",
						--format="xml",
						--format="json",
						--format="w3c",
						--[[
						timestamp=true,
						appendTimestamp=true,
						appendInstance=true,
						fileChunkLength=43200, -- 12 hours (in seconds)
						fileChunkTime="18:00:00",
						]]--
						enabledEvents=
						{
							"inStreamCreated",
							"outStreamCreated",
							--[[
							"streamCreated",
							"inStreamCodecsUpdated",
							"outStreamCodecsUpdated",
							"streamCodecsUpdated",
							"inStreamClosed",
							"outStreamClosed",
							"streamClosed",
							"cliRequest",
							"cliResponse",
							"applicationStart",
							"applicationStop",
							"carrierCreated",
							"carrierClosed",
							"serverStarted",
							"serverStopping",
							"protocolRegisteredToApp",
							"protocolUnregisteredFromApp",
							"processStarted",
							"processStopped",
							"timerCreated",
							"timerTriggered",
							"timerClosed",
							"hlsChunkCreated",
							"hlsChunkClosed",
							"hlsChunkError",
							"hlsChildPlaylistUpdated",
							"hlsMasterPlaylistUpdated",
							"hdsChunkCreated",
							"hdsChunkClosed",
							"hdsChunkError",
							"hdsChildPlaylistUpdated",
							"hdsMasterPlaylistUpdated",
							"mssChunkCreated",
							"mssChunkClosed",
							"mssChunkError",
							"mssPlaylistUpdated",
							"recordChunkCreated",
							"recordChunkClosed",
							"recordChunkError",
							"audioFeedStopped",
							"videoFeedStopped",
							"playlistItemStart",
							"firstPlaylistItemStart",
							"lastPlaylistItemStart",
							"streamingSessionStarted",
							"streamingSessionEnded",
							"mediaFileDownloaded",
							"playerJoinedRoom",
							"outStreamFirstFrameSent",
							"webRtcCommandReceived",
							"webRtcServiceConnected",
							]]--
						},
					},
					]=]--
					{
						type="RPC",
						-- IMPORTANT!!!
						-- To set the url below, match the webconfig.lua's ip and port settings
						--    port=8888,
						--    bindToIP="", -- used for binding to a particular IP
						--                    used in cases where a machine has multiple ethernet interfaces
						-- If using a single ethernet interface, use the localhost ip (not loopback);
						-- otherwise, use what is defined in bindToIP
						url="http://127.0.0.1:8888/rmswebservices/rmswebservices.php",
						serializerType="JSON",
						-- serializerType="XML"
						-- serializerType="XMLRPC"
						enabledEvents=
						{  --These are the events sent by default and tend to be the most commonly used
							"inStreamCreated",
							"outStreamCreated",
							"inStreamClosed",
							"outStreamClosed",
							"streamingSessionStarted",
							"streamingSessionEnded",
							"recordChunkClosed",
							"hlsChunkClosed",
							"pullStreamFailed",
							"playerJoinedRoom",
							"outStreamFirstFrameSent",

						--[[ A list of additional events that can be sent
							"streamCreated",
							"inStreamCodecsUpdated",
							"outStreamCodecsUpdated",
							"streamCodecsUpdated",
							"streamClosed",
							"cliRequest",
							"cliResponse",
							"applicationStart",
							"applicationStop",
							"carrierCreated",
							"carrierClosed",
							"serverStarted",
							"serverStopping",
							"protocolRegisteredToApp",
							"protocolUnregisteredFromApp",
							"processStarted",
							"processStopped",
							"timerCreated",
							"timerTriggered",
							"timerClosed",
							"hlsChunkCreated",
							"hlsChunkClosed",
							"hlsChunkError",
							"hlsChildPlaylistUpdated",
							"hlsMasterPlaylistUpdated",
							"hdsChunkCreated",
							"hdsChunkClosed",
							"hdsChunkError",
							"hdsChildPlaylistUpdated",
							"hdsMasterPlaylistUpdated",
							"dashChunkCreated",
							"dashChunkClosed",
							"dashChunkError",
							"dashPlaylistUpdated",
							"mssChunkCreated",
							"mssChunkClosed",
							"mssChunkError",
							"mssPlaylistUpdated",
							"recordChunkCreated",
							"recordChunkClosed",
							"recordChunkError",
							"audioFeedStopped",
							"videoFeedStopped",
							"playlistItemStart",
							"firstPlaylistItemStart",
							"lastPlaylistItemStart",
							"mediaFileDownloaded",
							"webRtcCommandReceived",
							"webRtcServiceConnected",]]--
						},
					},
				},
			},
			transcoder = {
				scriptPath="__STANDARD_TRANSCODER_SCRIPT__",
				srcUriPrefix="rtsp://localhost:5544/",
				dstUriPrefix="-f flv tcp://localhost:6666/"
			},
			mp4BinPath="__STANDARD_MP4WRITER_BIN__",
			--WebRTC
			webrtc = {
				sslKey="__STANDARD_SERVERKEY_FILE__",
				sslCert="__STANDARD_SERVERCERT_FILE__",
				--trustedCerts="__STANDARD_TURSTEDCERTS_FILE__"
			},
			--[[
			drm={
				type="verimatrix",
				requestTimer=1,
				reserveKeys=10,
				reserveIds=10,
				-- urlPrefix="http://server1.rms1.com:12684/CAB/keyfile"
				urlPrefix="http://vcas3multicas1.verimatrix.com:12684/CAB/keyfile"
			},
			]]--
		},
	}
}
