#ifndef __MMSC_BILLING_H__
#define __MMSC_BILLING_H__

#include "mmsc/mms_billing.h"
#include "mmlib/mms_util.h"
#include "gwlib/gwlib.h"

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
	unsigned long msg_size);
*/

#endif
