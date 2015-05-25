/*
 * Mbuni - Open  Source MMS Gateway
 *
 * Mbuni sample billing handler module
 *
 * Copyright (C) 2003 - 2008, Digital Solutions Ltd. - http://www.dsmagic.com
 *
 * Paul Bagyenda <bagyenda@dsmagic.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License, with a few exceptions granted (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mmsc-billing.h"
#include "billing-auxiliary.h"

/* This module provides a basic biller and CDR implementation. Totally no frills, but a basis
 * for implementing a 'real' module.
 * It does CDR but no billing (of course).
 */

static void *mms_billingmodule_init(char *settings)
{
	return billing_init (settings);
}

static int mms_billingmodule_fini(void *module_data)
{
	return billing_fini (module_data);
}

static int mms_billmsg (
	Octstr *from,
	List *to,
	unsigned long msg_size,
	Octstr *vaspid,
	Octstr *msgid,
	void *module_data)
{
	if (NULL == module_data)
		return EINVAL;

	int iRetVal = 0;
	const char 
		*pszFrom = octstr_get_cstr (from),
		*pszVASId = octstr_get_cstr (vaspid),
		*pszMsgId = octstr_get_cstr (msgid);
	struct SList
		*psoList = NULL,
		*psoNext = NULL;
	const char *pszStrValue;
	Octstr *psoOctstr = NULL;

	int i = 0;
	int l = gwlist_len (to);
	/* формируем список получателей */
	for (; i < l; ++ i) {
		psoOctstr = (Octstr *) gwlist_get (to, i);
		pszStrValue = octstr_get_cstr (psoOctstr);
		if (NULL == psoList) {
			psoList = gw_malloc (sizeof (struct SList));
			psoNext = psoList;
		} else {
			psoNext->m_psoNext = gw_malloc (sizeof (struct SList));
			psoNext = psoNext->m_psoNext;
		}
		memset (psoNext, 0, sizeof (struct SList));
		psoNext->m_pszValue = gw_strdup (pszStrValue);
	}

	/* выполняем проверку */
	iRetVal = billing_billmms (pszFrom, psoList, msg_size, pszVASId, pszMsgId, module_data);

	/* освобождаем память, занятую списком получателей */
	while (psoList) {
		psoNext = psoList->m_psoNext;
		if (psoList->m_pszValue) {
			gw_free (psoList->m_pszValue);
		}
		gw_free (psoList);
		psoList = psoNext;
	}

	return iRetVal;
}

static int mms_logcdr (MmsCdrStruct *cdr)
{
	return billing_logcdr (cdr);
}

/*
MmsCdrStruct *make_cdr_struct (
	void *module_data,
	time_t sdate,
	char *from,
	char *to,
	char *msgid,
	char *vaspid,
	char *src_int,
	char *dst_int,
	unsigned long msg_size)
{

	MmsCdrStruct  *cdr = gw_malloc (sizeof *cdr);

	cdr->module_data = module_data;
	cdr->sdate = sdate;
	strncpy(cdr->from, from ? from : "", sizeof cdr->from);
	strncpy(cdr->to, to ? to : "", sizeof cdr->to);
	strncpy(cdr->msgid, msgid ? msgid : "", sizeof cdr->msgid);
	strncpy(cdr->vaspid, vaspid ? vaspid : "", sizeof cdr->vaspid);
	strncpy(cdr->src_interface, src_int ? src_int : "MM2", sizeof cdr->src_interface);
	strncpy(cdr->dst_interface, dst_int ? dst_int : "MM2", sizeof cdr->dst_interface);     

	cdr->msg_size = msg_size;

	return cdr;
}

*/

/* The function itself. */
MmsBillingFuncStruct mms_billfuncs = {
	mms_billingmodule_init,
	mms_logcdr,
	mms_billmsg,
	mms_billingmodule_fini
};
