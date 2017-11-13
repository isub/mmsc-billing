#ifndef _MMSC_BILLING_MODULE_DATA_H_
#define _MMSC_BILLING_MODULE_DATA_H_

#include "utils/config/config.h"
#include "utils/log/log.h"

struct SModuleData {
  CConfig	m_coConf;
  CLog	m_coLog;
  int		m_iDebug;
  SModuleData() { m_iDebug = 0; }
};

#endif /* _MMSC_BILLING_MODULE_DATA_H_ */
