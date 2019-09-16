#include "mmsc-billing-cdr.h"
#include "mmsc_billing_module_data.h"
#include "billing-auxiliary.h"

#include "utils/log/log.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string>
#include <semaphore.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static sem_t    *g_psoSem;
static pthread_t g_threadCDR;
static int       g_iFile;
static char      g_mcFileName[ PATH_MAX ];

static u_int32_t   g_uiCDRInterval;
static const char *g_pszCDRDir;
static const char *g_pszCDRComplDir;
static const char *g_pszMainService;
std::string g_strTitle = "datetime;addr_init;addr_term;msg_id;vas_id;device_init;device_term;volume;senderProxy;recpntProxy\r\n";

#define CDR_FILE_FLAG (O_CREAT | O_APPEND | O_RDWR)
#define CDR_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define CDR_SEM_NAME  "/mmsc-billing-cdr-module.sem"

extern SModuleData *g_psoMuduleData;

static bool mmsc_billing_is_main_service();

static int mmsc_billing_cdr_make_timestamp( char *p_pszFileName, size_t p_stSize, const char *p_pszFileNameTmplt )
{
  int iRetVal = 0;
  size_t stLen;
  time_t tmTm;
  tm soTm;

  if ( static_cast<time_t>( -1 ) != time( &tmTm ) ) {
  } else {
    iRetVal = errno;
    return iRetVal;
  }
  if ( NULL != localtime_r( &tmTm, &soTm ) ) {
  } else {
    iRetVal = -1;
    return iRetVal;
  }
  stLen = snprintf( p_pszFileName, p_stSize, p_pszFileNameTmplt,
    soTm.tm_year + 1900, soTm.tm_mon + 1, soTm.tm_mday,
    soTm.tm_hour, soTm.tm_min, soTm.tm_sec );
  if ( 0 < stLen && stLen < p_stSize) {
  } else {
    iRetVal = -1;
    return iRetVal;
  }

  return iRetVal;
}

static int mmsc_billing_cdr_write_record( const int iFD, std::string &p_strData )
{
  if ( 0 < p_strData.length() ) {
    write( iFD, p_strData.data(), p_strData.length() );
  }

  return 0;
}

static int mmsc_billing_cdr_make_filename( const char *p_pszDir, char *p_pszFileName, size_t p_stSize, const char *p_pszFileNameTmplt )
{
  if ( NULL != p_pszDir ) {
  } else {
    return EINVAL;
  }

  int iRetVal = 0;
  char mcTimeStamp[ 256 ];
  int iFnRes;

  iFnRes = mmsc_billing_cdr_make_timestamp( mcTimeStamp, sizeof( mcTimeStamp ), p_pszFileNameTmplt );
  if ( 0 == iFnRes ) {
  } else {
    return iFnRes;
  }

  iFnRes = snprintf( p_pszFileName, p_stSize, "%s/%s", p_pszDir, mcTimeStamp );
  if ( 0 < iFnRes ) {
    if ( p_stSize > iFnRes ) {
    } else {
      return EINVAL;
    }
  } else {
    return -1;
  }

  return iRetVal;
}

static void mmsc_billing_cdr_file_completed( const char *p_pszFileName )
{
  if ( NULL != g_pszCDRComplDir ) {
    char mcNewFileName[ 1024 ];

    if ( 0 == mmsc_billing_cdr_make_filename( g_pszCDRComplDir, mcNewFileName, sizeof( mcNewFileName ), "%04u%02u%02u%02u%02u%02u_cdr.txt" ) ) {
    } else {
      return;
    }

    if ( 0 == rename( p_pszFileName, mcNewFileName ) ) {
    } else {
      UTL_LOG_D( g_psoMuduleData->m_coLog, "can not to move file: %d", errno );
    }
  }
}

static void * mmsc_billing_cdr_recreate_file( void *p_vParam )
{
  char mcFileName[ PATH_MAX ];
  size_t stLen;
  time_t tmTm;
  tm soTm;
  timeval soTimeVal;
  timespec soTimeSpec;
  int iFnRes;

  /* готовим ожидание */
  iFnRes = gettimeofday( &soTimeVal, NULL );
  if ( 0 == iFnRes ) {
  } else {
    pthread_exit( NULL );
  }
  soTimeSpec.tv_sec = soTimeVal.tv_sec;
  soTimeSpec.tv_sec += g_uiCDRInterval;
  soTimeSpec.tv_nsec = 0;

  int iNewFile = -1;
  int iTmpFile = -1;

  while ( -1 == ( iFnRes = sem_timedwait( g_psoSem, &soTimeSpec ) ) ) {
    iFnRes = errno;
    if ( ETIMEDOUT != iFnRes ) {
      UTL_LOG_D( g_psoMuduleData->m_coLog, "wait for semaphore failed: error code: %d; descr: %s", iFnRes, strerror( iFnRes ) );
      break;
    }
    if ( -1 != iTmpFile ) {
      close( iTmpFile );
    }
    mmsc_billing_cdr_file_completed( g_mcFileName );
    iFnRes = mmsc_billing_cdr_make_filename( g_pszCDRDir, mcFileName, sizeof( mcFileName ), "cdr.txt" );
    if ( 0 == iFnRes ) {
    } else {
      break;
    }
    iNewFile = open( mcFileName, CDR_FILE_FLAG, CDR_FILE_MODE );
    if ( -1 != iNewFile ) {
    } else {
      break;
    }

    mmsc_billing_cdr_write_record( iNewFile, g_strTitle );

    iTmpFile = g_iFile;
    g_iFile = iNewFile;

    strcpy( g_mcFileName, mcFileName );
    /* готовим ожидание */
    iFnRes = gettimeofday( &soTimeVal, NULL );
    if ( 0 == iFnRes ) {
    } else {
      break;
    }
    soTimeSpec.tv_sec = soTimeVal.tv_sec;
    soTimeSpec.tv_sec += g_uiCDRInterval;
    if ( 0 != ( soTimeSpec.tv_sec % g_uiCDRInterval ) ) {
      soTimeSpec.tv_sec /= g_uiCDRInterval;
      soTimeSpec.tv_sec *= g_uiCDRInterval;
    }
    soTimeSpec.tv_nsec = 0;
  }

  if ( -1 != iTmpFile ) {
    close( iTmpFile );
  }

  pthread_exit( NULL );
}

int mmsc_billing_cdr_init()
{
  int iRetVal = 0;
  int iFnRes;

  std::string strValue;

  if ( 0 == g_psoMuduleData->m_coConf.GetParamValue( "CDRInterval", strValue ) && 0 != strValue.length() ) {
    g_uiCDRInterval = std::stol( strValue );
  }
  if ( 0 != g_uiCDRInterval ) {
  } else {
    g_uiCDRInterval = 60;
  }

  if ( 0 == g_psoMuduleData->m_coConf.GetParamValue( "CDRDir", strValue ) && 0 != strValue.length() ) {
    g_pszCDRDir = gw_strdup( strValue.c_str() );
  } else {
    g_pszCDRDir = "/usr/local/var/log/mmsc/mmsc-billing/cdr";
  }

  if ( 0 == g_psoMuduleData->m_coConf.GetParamValue( "CDRComplDir", strValue ) && 0 != strValue.length() ) {
    g_pszCDRComplDir = gw_strdup( strValue.c_str() );
  } else {
    g_pszCDRComplDir = "/usr/local/var/log/mmsc/mmsc-billing/cdr/completed";
  }

  if ( 0 == g_psoMuduleData->m_coConf.GetParamValue( "MainService", strValue ) && 0 != strValue.length() ) {
    g_pszMainService = gw_strdup( strValue.c_str() );
  } else {
    g_pszMainService = "/usr/local/bin/mmsc";
  }

  if ( NULL != g_pszCDRDir ) {
  } else {
    return EINVAL;
  }

  iFnRes = mmsc_billing_cdr_make_filename( g_pszCDRDir, g_mcFileName, sizeof( g_mcFileName ), "cdr.txt" );
  if ( 0 == iFnRes ) {
  } else {
    return iFnRes;
  }
  bool bNewFile = ( 0 != access( g_mcFileName, F_OK ) );
  g_iFile = open( g_mcFileName, CDR_FILE_FLAG, CDR_FILE_MODE );
  if ( -1 != g_iFile ) {
  } else {
    iRetVal = errno;
    return iRetVal;
  }
  if ( bNewFile ) {
    mmsc_billing_cdr_write_record( g_iFile, g_strTitle );
  }

  if ( mmsc_billing_is_main_service() ) {
    g_psoSem = sem_open( CDR_SEM_NAME, O_CREAT, CDR_FILE_MODE, 0 );
    if ( SEM_FAILED == g_psoSem ) {
      UTL_LOG_D( g_psoMuduleData->m_coLog, "can not to open semaphore: error code: %d; descr: %s", errno, strerror( errno ) );
    } else {
      iFnRes = pthread_create( &g_threadCDR, NULL, mmsc_billing_cdr_recreate_file, NULL );
      if ( 0 == iFnRes ) {
      } else {
        sem_unlink( CDR_SEM_NAME );
        return iFnRes;
      }
      iFnRes = pthread_detach( g_threadCDR );
      if ( 0 == iFnRes ) {
      } else {
        sem_unlink( CDR_SEM_NAME );
        return iFnRes;
      }
    }
  }

  UTL_LOG_N( g_psoMuduleData->m_coLog, "CDR module is initialized successfully" );

  return iRetVal;
}

void mmsc_billing_cdr_fini()
{
  if ( SEM_FAILED != g_psoSem ) {
    mmsc_billing_cdr_file_completed( g_mcFileName );
    if ( 0 == sem_post( g_psoSem ) && 0 == sem_close( g_psoSem ) && 0 == sem_unlink( CDR_SEM_NAME ) ) {
      UTL_LOG_D( g_psoMuduleData->m_coLog, "can not to close semaphore: error code: %d; descr: %s", errno, strerror( errno ) );
    } else {
      UTL_LOG_D( g_psoMuduleData->m_coLog, "can not to close semaphore: error code: %d; descr: %s", errno, strerror( errno ) );
    }
  }
  close( g_iFile );
  UTL_LOG_N( g_psoMuduleData->m_coLog, "CDR module: work completed" );
}

void mmsc_billing_write_cdr(
  const char *p_pszTimeStamp,
  const char *p_pszFrom,
  const char *p_pszTo,
  const char *p_pszMsbId,
  const char *p_pszVASPId,
  const char *p_pszSrcInterface,
  const char *p_pszDstInterface,
  int p_iMsgSize,
	const char *p_pszSenderProxy,
	const char *p_pszRecpntProxy )
{
  std::string strRecCont;

  strRecCont;

  strRecCont  = p_pszTimeStamp;
  strRecCont += ';';
  strRecCont += p_pszFrom;
  strRecCont += ';';
  strRecCont += p_pszTo;
  strRecCont += ';';
  strRecCont += p_pszMsbId;
  strRecCont += ';';
  strRecCont += p_pszVASPId;
  strRecCont += ';';
  strRecCont += p_pszSrcInterface;
  strRecCont += ';';
  strRecCont += p_pszDstInterface;
  strRecCont += ';';
  strRecCont += std::to_string( static_cast<long long int>( p_iMsgSize ) );
  strRecCont += ';';
  strRecCont += p_pszSenderProxy;
  strRecCont += ';';
  strRecCont += p_pszRecpntProxy;
  strRecCont += "\r\n";

  if ( -1 != g_iFile ) {
    mmsc_billing_cdr_write_record( g_iFile, strRecCont );
  }
}

static bool mmsc_billing_is_main_service()
{
  int iRetVal = 0;
  pid_t tPId;
  std::string str;
  char mcPath[ PATH_MAX ];
  size_t tLen;

  tPId = getpid();

  str = "/proc/" + std::to_string( static_cast<long long unsigned> ( tPId ) );
  str += "/exe";

  tLen = readlink( str.c_str(), mcPath, PATH_MAX );
  if ( static_cast<size_t>( -1 ) != tLen && tLen < PATH_MAX ) {
    mcPath[ tLen ] = '\0';
    UTL_LOG_D( g_psoMuduleData->m_coLog, "module is started from: %s", mcPath );
  } else {
    mcPath[ 0 ] = '\0';
    UTL_LOG_D( g_psoMuduleData->m_coLog, "error occurred: code: %d; descr: %s\n", errno, strerror( errno ) );
  }

  return ( 0 == strcmp( mcPath, g_pszMainService ) );
}
