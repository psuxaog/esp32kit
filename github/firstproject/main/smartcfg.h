/*
 * smartcfg.h
 *
 *  Created on: 2019年1月13日
 *      Author: linx
 */

#ifndef GITHUB_FIRSTPROJECT_MAIN_SMARTCFG_H_
#define GITHUB_FIRSTPROJECT_MAIN_SMARTCFG_H_
typedef enum
{
	SmartCfgNone, SmartCfgStp1, SmartCfgStp2, SmartCfgOver,
} SmartCfg_t;
extern void SC_StartSmart(void);
extern SmartCfg_t SC_GetSmartCfgState(void);

#endif /* GITHUB_FIRSTPROJECT_MAIN_SMARTCFG_H_ */
