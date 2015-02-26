#ifndef _BILLING_AUXILIARY_H_
#define _BILLING_AUXILIARY_H_

#include "mmsc-billing.h"

#ifdef __cplusplus
extern "C" {
#endif

void * billing_init (const char *p_pszSettings);

int billing_fini (void *p_pModuleData);

struct SList {
	char *m_pszValue;
	struct SList *m_psoNext;
};
int billing_billmms (
	const char *p_pszFrom,
	struct SList * p_psoTo,
	unsigned long p_uiMsgSize,
	const char *p_pszVASPid,
	const char *p_pszMsgId,
	void *p_pModuleData);

int billing_logcdr (MmsCdrStruct *p_psoCDR);

#ifdef __cplusplus
}
#endif

#endif