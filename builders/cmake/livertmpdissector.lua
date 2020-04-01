configuration=
{
	daemon=false,
	pathSeparator="/",
	instancesCount=0,
	logAppenders=
	{
		{
			name="console appender",
			type="coloredConsole",
			level=6
		},
	},
	applications=
	{
		rootDirectory="applications",
		{
			name="livertmpdissector",
			description="Application for showing and logging live RTMP messages",
			protocol="dynamiclinklibrary",
			default=true,
			acceptors =
			{
				{
					ip="0.0.0.0",
					port=1937,
					protocol="rawTcp"
				}
			},
			targetIp="10.0.1.154",
			targetPort=1935
		},
	}
}

