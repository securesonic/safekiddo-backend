/*
 * lousyWebrootStuff.cpp
 *
 *  Created on: 13 Feb 2014
 *      Author: zytka
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "sdkcfg.h"
#include "bclog.h"
#include "bcsdklib.h"

#include "utils/assert.h"
#include "utils/alerts.h"

#include "webrootCpp/api.h"
#include "webrootCpp/config.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace su = safekiddo::utils;

////////////////////
//  get str val
////////////////////
static char * sGetStrVal( const char *szLine, const char *szKey )
{
	static char szStrVal[1024];
	char *p = (char*)szLine;
	p += strlen(szKey) + 1;
	strcpy(szStrVal,p);
	int n = int(strlen(szStrVal));
	if (szStrVal[n-1] == 10)	// stray char
		szStrVal[n-1] = '\0';

	return szStrVal;
}

////////////////////
//  get int val
////////////////////
static int nGetIntVal( const char *szLine, const char *szKey )
{
	char szStrVal[1024];
	char *p = (char*)szLine;
	p += strlen(szKey) + 1;
	strcpy(szStrVal,p);
	int n = int(strlen(szStrVal));
	if (szStrVal[n-1] == 10)	// stray char
		szStrVal[n-1] = '\0';

	return atoi(szStrVal);
}

static void logConfig(sConfigValsStruct const & sCfg)
{
	LOG(INFO, "config: OEM		", sCfg.szOem);
	LOG(INFO, "config: Device		", sCfg.szDevice);
	LOG(INFO, "config: UID		", sCfg.szUid);
	LOG(INFO, "config: Security	", sCfg.szSecurity);
	LOG(INFO, "config: BCAP Server	", sCfg.szBcapServer);
	LOG(INFO, "config: BCAP Port	", sCfg.nBcapPort);
	LOG(INFO, "config: SSL Port	", sCfg.nSslPort);

	LOG(INFO, "config: Database Server		", sCfg.szDbServer);
	LOG(INFO, "config: Database type		", sCfg.szDatabaseType);
	LOG(INFO, "config: Update Interval	Mins	", sCfg.nUpdateIntervalMins);
	LOG(INFO, "config: Lookup Threads		", sCfg.nLookupThreads);
	LOG(INFO, "config: Lookup Mode		", sCfg.nLookupMode);
	LOG(INFO, "config: LogLevel		", sCfg.nLogLevel);
	LOG(INFO, "config: DoBcap			", sCfg.bDoBcap);
	LOG(INFO, "config: DoRtu			", sCfg.bDoRtu);
	LOG(INFO, "config: AsyncMode		", sCfg.bAsyncMode);
	LOG(INFO, "config: DownloadDatabase	", sCfg.bDownloadDatabase);
	LOG(INFO, "config: LocalDatabaseDir	", sCfg.szLocalDatabaseDir);
	LOG(INFO, "config: DatabaseDownloadLog	", sCfg.szDatabaseDownloadLog);
	LOG(INFO, "config: UseCloudFront		", sCfg.bUseCloudFront);
	LOG(INFO, "config: UseSharedMemory		", sCfg.bUseSharedMem);
    LOG(INFO, "config: UnusedMemTimeoutSecs	", sCfg.nUnusedMemTimeoutSecs);
	LOG(INFO, "config: InstanceNumber		", sCfg.nInstanceNumber);
	LOG(INFO, "config: DbCachePercentToLoad	", sCfg.nDbCachePercentToLoad);
	LOG(INFO, "config: LcpLoopLimit		",		sCfg.nLcpLoopLimit);
	LOG(INFO, "config: Checking local DB: ", sCfg.checkLocalDb);
	LOG(INFO, "config: Checking Bcap cache: ", sCfg.checkBcapCache);

	if ( sCfg.szProxyHost[0] )
		LOG(INFO, "config file read: ProxyHost		", sCfg.szProxyHost);
	if ( sCfg.nProxyPort )
		LOG(INFO, "config file read: ProxyPort		", sCfg.nProxyPort);
	if ( sCfg.szProxyUsername[0] )
		LOG(INFO, "config file read: ProxyUsername		", sCfg.szProxyUsername);
	if ( sCfg.szProxyPassword[0] )
		LOG(INFO, "config file read: ProxyPassword		", sCfg.szProxyPassword);
}

static sConfigValsStruct readconfig( const char *szCfgFile )
{
	sConfigValsStruct sCfg;
	void *pCfg = (void*)&sCfg;
	memset( pCfg, 0, sizeof( sCfg ) );

	FILE *f;
	char szLine[1024];

	f = fopen(szCfgFile, "r");
	if ( !f )
	{
		FAILURE("error opening file ", szCfgFile);
	}

	///////////////////
	// default values
	///////////////////
	strcpy( sCfg.szDatabaseType, (const char *)"content" );
	sCfg.nInstanceNumber = 0;
	sCfg.bDoBcap = false;
	sCfg.bDoRtu = false;
	sCfg.checkBcapCache = true; // default setting
	sCfg.checkLocalDb = true; // default setting

	//  process all lines in config file
	//
	while ( fgets(szLine, 1024, f) )
	{
		char *p;
		if ( !strncmp(szLine,"//",2) )
			continue;
		if ( !strncmp(szLine,"[",1) )
			continue;
		if ( !strncmp(szLine,"#",1) )
			continue;

		if ( (p = strstr( szLine, "LogLevel" )) && (p == szLine) )
			sCfg.nLogLevel = nGetIntVal( szLine, "LogLevel" );

		if ( (p = strstr( szLine, "DbServer" )) && (p == szLine) )
			strcpy( sCfg.szDbServer, sGetStrVal( szLine, "DbServer" ) );

		if ( (p = strstr( szLine, "BcapServer" )) && (p == szLine) )
			strcpy( sCfg.szBcapServer, sGetStrVal( szLine, "BcapServer" ) );

		if ( (p = strstr( szLine, "BcapPort" )) && (p == szLine) )
			sCfg.nBcapPort = nGetIntVal( szLine, "BcapPort" );

		if ( (p = strstr( szLine, "SslPort" )) && (p == szLine) )
			sCfg.nSslPort = nGetIntVal( szLine, "SslPort" );

		if ( (p = strstr( szLine, "Device" )) && (p == szLine) )
			strcpy( sCfg.szDevice, sGetStrVal( szLine, "Device" ) );

		if ( (p = strstr( szLine, "Oem" )) && (p == szLine) )
			strcpy( sCfg.szOem, sGetStrVal( szLine, "Oem" ) );

		if ( (p = strstr( szLine, "Uid" )) && (p == szLine) )
			strcpy( sCfg.szUid, sGetStrVal( szLine, "Uid" ) );

		if ( (p = strstr( szLine, "Security" )) && (p == szLine) )
			strcpy( sCfg.szSecurity, sGetStrVal( szLine, "Security" ) );

		if ( (p = strstr( szLine, "DoBcap" )) && (p == szLine) )
		{
			strcpy( sCfg.szDoBcap, sGetStrVal( szLine, "DoBcap" ) );
			if ( !strcmp( sCfg.szDoBcap, "true" ) )
				sCfg.bDoBcap = true;
		}
		if ( (p = strstr( szLine, "DoRtu" )) && (p == szLine) )
		{
			strcpy( sCfg.szDoRtu, sGetStrVal( szLine, "DoRtu" ) );
			if ( !strcmp( sCfg.szDoRtu, "true" ) )
				sCfg.bDoRtu = true;
		}
		if ( (p = strstr( szLine, "AsyncMode" )) && (p == szLine) )
		{
			strcpy( sCfg.szAsyncMode, sGetStrVal( szLine, "AsyncMode" ) );
			if ( !strcmp( sCfg.szAsyncMode, "true" ) )
				sCfg.bAsyncMode = true;
		}
		if ( (p = strstr( szLine, "UseCloudFront" )) && (p == szLine) )
		{
			strcpy( sCfg.szUseCloudFront, sGetStrVal( szLine, "UseCloudFront" ) );
			if ( !strcmp( sCfg.szUseCloudFront, "true" ) )
				sCfg.bUseCloudFront = true;
		}
		if ( (p = strstr( szLine, "UseSharedMem" )) && (p == szLine) )
		{
			strcpy( sCfg.szUseSharedMem, sGetStrVal( szLine, "UseSharedMem" ) );
			if ( !strcmp( sCfg.szUseSharedMem, "true" ) )
				sCfg.bUseSharedMem = true;
		}
		if ( (p = strstr( szLine, "InstanceNumber" )) && (p == szLine) )
			sCfg.nInstanceNumber = nGetIntVal( szLine, "InstanceNumber" );

		if ( (p = strstr( szLine, "DownloadDatabase" )) && (p == szLine) )
		{
			strcpy( sCfg.szDownloadDatabase, sGetStrVal( szLine, "DownloadDatabase" ) );
			if ( !strcmp( sCfg.szDownloadDatabase, "true" ) )
				sCfg.bDownloadDatabase = true;
		}
		if ( (p = strstr( szLine, "LocalDatabaseDir" )) && (p == szLine) )
			strcpy( sCfg.szLocalDatabaseDir, sGetStrVal( szLine, "LocalDatabaseDir" ) );

		if ( (p = strstr( szLine, "DatabaseDownloadLog" )) && (p == szLine) )
			strcpy( sCfg.szDatabaseDownloadLog, sGetStrVal( szLine, "DatabaseDownloadLog" ) );

		if ( (p = strstr( szLine, "DatabaseType" )) && (p == szLine) )
			strcpy( sCfg.szDatabaseType, sGetStrVal( szLine, "DatabaseType" ) );

		if ( (p = strstr( szLine, "ServiceCacheTtl" )) && (p == szLine) )
			sCfg.nServiceCacheTtl = nGetIntVal( szLine, "ServiceCacheTtl" );

		if ( (p = strstr( szLine, "DbCachePercentToLoad" )) && (p == szLine) )
			sCfg.nDbCachePercentToLoad = nGetIntVal( szLine, "DbCachePercentToLoad" );

		if ( (p = strstr( szLine, "UpdateIntervalMins" )) && (p == szLine) )
			sCfg.nUpdateIntervalMins = nGetIntVal( szLine, "UpdateIntervalMins" );

		if ( (p = strstr( szLine, "UnusedMemTimeoutSecs" )) && (p == szLine) )
			sCfg.nUnusedMemTimeoutSecs = nGetIntVal( szLine, "UnusedMemTimeoutSecs" );

		if ( (p = strstr( szLine, "LookupThreads" )) && (p == szLine) )
			sCfg.nLookupThreads = nGetIntVal( szLine, "LookupThreads" );

		if ( (p = strstr( szLine, "LookupMode" )) && (p == szLine) )
			sCfg.nLookupMode = nGetIntVal( szLine, "LookupMode" );

		if ( (p = strstr( szLine, "LcpLoopLimit" )) && (p == szLine) )
			sCfg.nLcpLoopLimit = nGetIntVal( szLine, "LcpLoopLimit" );

		// proxy parameters go here
		if ( (p = strstr( szLine, "ProxyHost" )) && (p == szLine) )
			strcpy( sCfg.szProxyHost, sGetStrVal( szLine, "ProxyHost" ) );

		if ( (p = strstr( szLine, "ProxyPort" )) && (p == szLine) )
			sCfg.nProxyPort = nGetIntVal( szLine, "ProxyPort" );

		if ( (p = strstr( szLine, "ProxyUsername" )) && (p == szLine) )
			strcpy( sCfg.szProxyUsername, sGetStrVal( szLine, "ProxyUsername" ) );

		if ( (p = strstr( szLine, "ProxyPassword" )) && (p == szLine) )
			strcpy( sCfg.szProxyPassword, sGetStrVal( szLine, "ProxyPassword" ) );

	}

	fclose(f);

	//  BcSetConfig sends this data to the SDK library
	//
	return sCfg;
}

namespace safekiddo
{
namespace webroot
{

WebrootConfig BcReadConfig(std::string const webrootConfigFilePath)
{
	return WebrootConfig(readconfig(webrootConfigFilePath.c_str()));
}

void BcSetConfig(WebrootConfig const & config)
{
	::BcSetConfig(config.getRawConfig());
}

static su::LogLevelEnum mapBcLogLevel(BcLogType type)
{
	switch (type)
	{
	case BCERROR:
		return su::LogLevel::ERROR;
	case BCINFO:
		return su::LogLevel::INFO;
	case BCCACHEINFO:
		return su::LogLevel::DEBUG1;
	case BCCONNECTDBG:
		return su::LogLevel::DEBUG2;
	}
	FAILURE("invalid webroot log type: " << static_cast<int>(type));
}

void bcLogOutputFun(BcLogType type, const char *format, const char * file, uint32_t line, va_list ap)
{
	char buf[1024];
	vsnprintf(buf, sizeof(buf), format, ap);
	size_t const len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n')
	{
		buf[len - 1] = '\0';
	}
	su::LogLevelEnum const level = mapBcLogLevel(type);
	LOG_IMPL(level, file, line, "BC: " << buf);
}

void BcInitialize(WebrootConfig const & config)
{
	::BcSetLogOutputFun(bcLogOutputFun);
	::BcSetConfig(config.getRawConfig());
	::BcSdkInit();
	::BcStartRtu();
}

void BcShutdown()
{
	::BcStopRtu();
}

void BcLogConfig(WebrootConfig const & config)
{
	logConfig(config.getRawConfig());
}

void BcSdkInit()
{
	::BcSdkInit();
}

void BcStartRtu()
{
	::BcStartRtu();
}

void BcStopRtu()
{
	::BcStopRtu();
}

void BcWaitForDbLoadComplete(WebrootConfig const & config)
{
	// HACK: in connectionReestablishedTest backend is started when some requests have already been
	// sent to WS and are expected to be processed as soon as backend starts, before timeout on WS.
	// Normally the following condition evaluates to true. In product both parts are true.
	if (config.getRawConfig().bDoBcap || config.getRawConfig().checkLocalDb)
	{
		// In product and in most tests we wait anyway until backend loads webroot database. For
		// debugging reasons, let's verify it by delaying request processing until db is loaded.

		boost::posix_time::ptime const startTime = boost::posix_time::microsec_clock::local_time();
		::BcWaitForDbLoadComplete();
		boost::posix_time::time_duration const duration = boost::posix_time::microsec_clock::local_time() - startTime;
#ifndef NDEBUG
		// debug config
		boost::posix_time::time_duration const maxDuration = boost::posix_time::seconds(20);
#else
		// if updating below value please consider the same in scripts/monit/startBackend.sh
		boost::posix_time::time_duration const maxDuration = boost::posix_time::seconds(10);
#endif
		if (duration > maxDuration)
		{
			ALERT("high webroot database load time: " << duration);
		}
	}
}

void BcStartStatisticsSending(statistics::StatisticsGatherer & statsGatherer)
{
	::BcStartStatisticsSending(statsGatherer);
}

void BcStopStatisticsSending()
{
	::BcStopStatisticsSending();
}

} // namespace webroot
} // namespace safekiddo
