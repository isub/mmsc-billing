#include "billing-auxiliary.h"
#include "mmsc-billing-cdr.h"
#include "mmsc_billing_module_data.h"

#include "utils/dbpool/dbpool.h"

#include <string>
#include <time.h>
#include <errno.h>

SModuleData *g_psoMuduleData;

int billing_apply_settings (
	const char *p_pszSettings,
	CConfig &p_coConfig)
{
	return p_coConfig.LoadConf(p_pszSettings, 0);
}

void * billing_init (const char *p_pszSettings)
{
	SModuleData *psoRetVal = NULL;

	do {
    int iFnRes;

    /* выделяем память для структуры данных модуля */
		psoRetVal = new SModuleData;
    g_psoMuduleData = psoRetVal;

		/* разбираем строку инициализации */
		iFnRes = billing_apply_settings (
			p_pszSettings,
			psoRetVal->m_coConf);
		if (iFnRes) {
			delete psoRetVal;
			psoRetVal = NULL;
			break;
		}

		/* уровень отладки */
		std::string strParamVal;
		psoRetVal->m_coConf.GetParamValue ("debug", strParamVal);
		if (strParamVal.length ()) {
			psoRetVal->m_iDebug = atoi (strParamVal.c_str ());
		}

#ifdef DEBUG
    psoRetVal->m_iDebug = 2;
#endif

		/* инициализируем логгер */
		std::string strParamValue;
		iFnRes = psoRetVal->m_coConf.GetParamValue ("log_file_mask", strParamValue);
		if (iFnRes) {
			delete psoRetVal;
			psoRetVal = NULL;
			break;
		}
		iFnRes = psoRetVal->m_coLog.Init (strParamValue.c_str ());
		if (iFnRes) {
			delete psoRetVal;
			psoRetVal = NULL;
			break;
		}

    iFnRes = mmsc_billing_cdr_init();
    if ( iFnRes ) {
      delete psoRetVal;
      psoRetVal = NULL;
      break;
    }

		/* инициализируем пул подключений к БД */
		iFnRes = db_pool_init (&(psoRetVal->m_coLog), &(psoRetVal->m_coConf));
		if (iFnRes) {
			delete psoRetVal;
			psoRetVal = NULL;
			break;
		}
	} while (0);

	if (psoRetVal) {
		psoRetVal->m_coLog.WriteLog ("%s: mms billing module is initialized successfully", __FUNCTION__);
	}

	return (void *) psoRetVal;
}

int billing_fini (void *p_pModuleData)
{
	if (NULL == p_pModuleData)
		return EINVAL;

  SModuleData *psoModuleData = reinterpret_cast<SModuleData *> ( p_pModuleData );

  db_pool_deinit ();

  mmsc_billing_cdr_fini();

	delete psoModuleData;

	return 0;
}

int billing_billmms (
	const char *p_pszFrom,
	SList * p_psoTo,
	unsigned long p_uiMsgSize,
	const char *p_pszVASPid,
	const char *p_pszMsgId,
	void *p_pModuleData)
{
	if (NULL == p_pModuleData)
		return EINVAL;

	int iRetVal = 0;

  SModuleData *psoModuleData = reinterpret_cast<SModuleData *>( p_pModuleData );

	ENTER_ROUT (psoModuleData->m_iDebug, psoModuleData->m_coLog);

	if (0 < psoModuleData->m_iDebug) {
		std::string strToList;
		SList *psoTmp = p_psoTo;
		while (psoTmp) {
			if (psoTmp->m_pszValue) {
				if (strToList.length ()) {
					strToList += ',';
				}
				strToList += psoTmp->m_pszValue;
			}
			psoTmp = psoTmp->m_psoNext;
		}

		psoModuleData->m_coLog.WriteLog (
			"%s;%s;%s;%s;%u",
			p_pszFrom,
			strToList.c_str (),
			p_pszMsgId,
			p_pszVASPid,
			p_uiMsgSize);
		iRetVal = 1;
	}

	LEAVE_ROUT (psoModuleData->m_iDebug, psoModuleData->m_coLog, iRetVal);

	return iRetVal;
}

int billing_logcdr (MmsCdrStruct *p_psoCDR)
{
	int iRetVal = 0;
	otl_connect *pcoDBConn;
  SModuleData *psoModuleData = reinterpret_cast<SModuleData *>( p_psoCDR->module_data );

	ENTER_ROUT (psoModuleData->m_iDebug, psoModuleData->m_coLog);

	do {
    int iFnRes;
    tm soTm;
		char mcTimeStamp[256];

    if ( NULL == localtime_r (&(p_psoCDR->sdate), &soTm)) {
			iRetVal = -2;
			break;
		}
    iFnRes = gw_strftime( mcTimeStamp, sizeof( mcTimeStamp ), "%d.%m.%Y %H:%M:%S", &soTm );
    if ( 0 < iFnRes  && iFnRes < sizeof( mcTimeStamp ) ) {
    } else {
      iRetVal = -3;
      break;
    }
    /* записываем cdr в файл */
    mmsc_billing_write_cdr( mcTimeStamp, p_psoCDR->from, p_psoCDR->to, p_psoCDR->msgid, p_psoCDR->vaspid, p_psoCDR->src_interface, p_psoCDR->dst_interface, p_psoCDR->msg_size );
		/* запрашиваем объект подключения к БД */
		pcoDBConn = db_pool_get ();
		if (NULL == pcoDBConn) {
			iRetVal = -1;
			break;
		}

		otl_datetime coTime;
		coTime.year = 1900 + soTm.tm_year;
		coTime.month = soTm.tm_mon + 1;
		coTime.day = soTm.tm_mday;
		coTime.hour = soTm.tm_hour;
		coTime.minute = soTm.tm_min;
		coTime.second = soTm.tm_sec;
		try {
			otl_stream coStream;
			coStream.open (
				1,
				"insert "
					"into ps.mms_cdr "
					"(mms_time, mms_from, mms_to, mms_id, vas_id, src_interface, dst_interface, msg_size) "
				"values "
					"(:mms_time /* timestamp */, :mms_from /* char[256] */, :mms_to /* char[256] */, :mms_id /* char[256] */, :vas_id /* char[256] */, :src_interface /* char[256] */, :dst_interface /* char[256] */, :msg_size /* unsigned */)",
				*pcoDBConn);
			coStream.set_commit (0);
			coStream
				<< coTime
				<< p_psoCDR->from
				<< p_psoCDR->to
				<< p_psoCDR->msgid
				<< p_psoCDR->vaspid
				<< p_psoCDR->src_interface
				<< p_psoCDR->dst_interface
				<< (unsigned int) p_psoCDR->msg_size;
			pcoDBConn->commit ();
			coStream.close ();
		} catch (otl_exception &coExept) {
			psoModuleData->m_coLog.WriteLog ("%s: error: code: '%d'; description: '%s'", __FUNCTION__, coExept.code, coExept.msg);
			pcoDBConn->rollback ();
		}
	} while (0);

	if (pcoDBConn) {
		db_pool_release (pcoDBConn);
	}

	LEAVE_ROUT (psoModuleData->m_iDebug, psoModuleData->m_coLog, iRetVal);

	return iRetVal;
}
