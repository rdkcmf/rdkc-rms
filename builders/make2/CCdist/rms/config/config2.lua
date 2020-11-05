configuration=
{
	daemon=false,
	instancesCount=-1,
	pathSeparator="/",
	timeProbeFilePathPrefix="/usr/local/rms/logs/timeprobe",
	logAppenders=
	{
		--[[{
			name="console appender",
			type="coloredConsole",
			level=6
		},]]--
		--[[{
			name="file appender",
			type="file",
			level=6,
			fileName="/opt/logs/rms",
			newLineCharacters="\n",
			fileHistorySize=100,
			fileLength=1024*1024,
			singleLine=true
		},]]--
                {
                        name="rdk appender",
                        type="apiRdk",
                        level=6,
                        debugIniPath="/etc/debug.ini",
                        devdebugIniPath="/opt/debug.ini"
                },
	},
	applications=
	{
		rootDirectory="/usr/local/rms/",
		{
			appDir="/usr/local/rms/bin/",
			name="rdkcms",
			description="RDKC MEDIA SERVER",
			protocol="dynamiclinklibrary",
			default=true,
			pushPullPersistenceFile="/usr/local/rms/config/pushPullSetup_stream2.xml",
			authPersistenceFile="/usr/local/rms/config/auth.xml",
			connectionsLimitPersistenceFile="/usr/local/rms/config/connlimits.xml",
			bandwidthLimitPersistenceFile="/usr/local/rms/config/bandwidthlimits.xml",
			ingestPointsPersistenceFile="/usr/local/rms/config/ingestpoints.xml",
			videoconfigPersistenceFile="/usr/local/rms/config/videoconfig.xml",
			rfcvideoconfigPersistenceFile="/opt/usr_config/videoconfig.xml",
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
				recordedStreamsStorage="/usr/local/rms/media",
				{
					description="Default media storage",
					mediaFolder="/usr/local/rms/media",
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
					port=1222,
					protocol="inboundAsciiCli",
					useLengthPadding=true
				},
      --[[
				{
					ip="127.0.0.1",
					port=7777,
					protocol="inboundHttpJsonCli"
				},
				-- RTMP and clustering
				{
					ip="127.0.0.1",
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
					ip="127.0.0.1",
					port=5544,
					protocol="inboundRtsp",
				},

				-- LiveFLV ingest
				{
					ip="127.0.0.1",
					port=6666,
					protocol="inboundLiveFlv",
					waitForMetadata=true,
				},
      ]]--
				--[[
				-- Inbound TCP TS
				{
					ip="127.0.0.1",
					port=9999,
					protocol="inboundTcpTs",
				},

				-- Inbound UDP TS
				{
					ip="127.0.0.1",
					port=9999,
					protocol="inboundUdpTs",
				},
				]]--


				--RTMPS
				--[[
				{
					ip="127.0.0.1",
					port=4443,
					protocol="inboundRtmp",
					-- cipherSuite="!DEFAULT:RC4-SHA", -- enables/disables a specific set of ciphers. If not specified, it is defaulted to the openssl collection of ciphers
					sslKey="/path/to/some/key",
					sslCert="/path/to/some/cert",
				},
				]]--
        --[[
				-- JSONMETA
				{
					ip="127.0.0.1",
					port=8100,
					--streamname="test",
					protocol="inboundJsonMeta",
				},
				-- WebSockets JSON Metadata
				{
					ip="127.0.0.1",
					port=8210,
					protocol="wsJsonMeta",
					-- streamname="~0~0~0~"
				},
				-- WebSockets FMP4 Fetch
				{
					ip="127.0.0.1",
					port=8410,
					protocol="inboundWSFMP4"
				},
        ]]--
			},
			--[[
			autoDASH=
			{
				targetFolder= "/usr/local/rms/rms-webroot",
			},
			autoHLS=
			{
				targetFolder= "/usr/local/rms/rms-webroot",
			},
			autoHDS=
			{
				targetFolder= "/usr/local/rms/rms-webroot",
			},
			autoMSS=
			{
				targetFolder= "/usr/local/rms/rms-webroot",
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
					usersFile="/usr/local/rms/config/users.lua"
				},
				rtsp=
				{
					usersFile="/usr/local/rms/config/users.lua",
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
						filename="/usr/local/rms/logs/events.txt",
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
							]]--
						},
					},
					]=]--
					{
						--type="RPC",
						-- IMPORTANT!!!
						-- To set the url below, match the webconfig.lua's ip and port settings
						--    port=8888,
						--    bindToIP="", -- used for binding to a particular IP
						--                    used in cases where a machine has multiple ethernet interfaces
						-- If using a single ethernet interface, use the localhost ip (not loopback);
						-- otherwise, use what is defined in bindToIP
						--url="http://127.0.0.1:8888",
						--serializerType="JSON",
						-- serializerType="XML"
						-- serializerType="XMLRPC"
						--enabledEvents=
						--{  --These are the events sent by default and tend to be the most commonly used
						--	"serverStarted",
						--},
					},
				},
			},
			transcoder = {
				scriptPath="./rmsTranscoder.sh",
				srcUriPrefix="rtsp://localhost:5544/",
				dstUriPrefix="-f flv tcp://localhost:6666/"
			},
			mp4BinPath="./rms-mp4writer",
			--WebRTC
			webrtc = {
				rrsOverSsl=true,
				sslKey="/etc/ssl/certs/server.key",
				sslCert="/etc/ssl/certs/server.cert",
				trustedCerts="/etc/ssl/certs/trustedCerts.db"
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
