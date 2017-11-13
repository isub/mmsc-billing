#ifndef _MMSC_BILLING_CDR_H_
#define _MMSC_BILLING_CDR_H_

#ifdef __cplusplus
extern "C" {
#endif

int mmsc_billing_cdr_init();
void mmsc_billing_cdr_fini();

void mmsc_billing_write_cdr(
  const char *p_pszTimeStamp,
  const char *p_psszFrom,
  const char *p_pszTo,
  const char *p_pszMsbId,
  const char *p_pszVASPId,
  const char *p_pszSrcInterface,
  const char *p_pszDstInterface,
  int p_iMsgSize );

#ifdef __cplusplus
}
#endif

#endif /* _MMSC_BILLING_CDR_H_ */
