#ifndef __MMSC_BILLING_H__
#define __MMSC_BILLING_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "mmsc/mms_billing.h"
#include "mmlib/mms_util.h"
#include "gwlib/gwlib.h"
#ifdef __cplusplus
}
#endif


static void *mms_billingmodule_init(char *settings);
static int mms_billingmodule_fini(void *module_data);
static int mms_billmsg (
	Octstr *from,
	List *to,
	unsigned long msg_size,
	Octstr *vaspid,
	Octstr *msgid,
	void *module_data);
static int mms_logcdr (MmsCdrStruct *cdr);

#endif
