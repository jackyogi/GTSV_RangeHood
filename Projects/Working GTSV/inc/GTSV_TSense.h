#ifndef __GTSV_TSENSE_H_INCLUDED
#define __GTSV_TSENSE_H_INCLUDED

#include "main.h"
#include "stm32_tsl_api.h"

#define TS_KEY01	sSCKeyInfo[0]
#define TS_KEY02	sSCKeyInfo[1]
#define TS_KEY03	sSCKeyInfo[2]
#define TS_KEY04	sSCKeyInfo[3]
#define TS_KEY05	sSCKeyInfo[4]

#define TS_KEY01_DETECTED (TS_KEY01.Setting.b.DETECTED)
#define TS_KEY02_DETECTED (TS_KEY02.Setting.b.DETECTED)
#define TS_KEY03_DETECTED (TS_KEY03.Setting.b.DETECTED)
#define TS_KEY04_DETECTED (TS_KEY04.Setting.b.DETECTED)
#define TS_KEY05_DETECTED (TS_KEY05.Setting.b.DETECTED)

#define KEY_MINUS_DETECTED	(TS_KEY01_DETECTED)
#define KEY_AUTO_DETECTED	(TS_KEY02_DETECTED)
#define KEY_PLUS_DETECTED	(TS_KEY03_DETECTED)
#define KEY_TIMER_DETECTED	(TS_KEY04_DETECTED)
#define KEY_LIGHT_DETECTED	(TS_KEY05_DETECTED)



void Tsense_to_default_config(void);
void Tsense_action(void);

#endif /* __GTSV_TSENSE_H_INCLUDED */
/******************* (C) COPYRIGHT 2013 Global Tech Solution Co., Ltd. *****END OF FILE****/
