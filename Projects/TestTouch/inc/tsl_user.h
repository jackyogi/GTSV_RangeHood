/**
  ******************************************************************************
  * @file    STM32F0518_Ex01_3TKeys_EVAL\inc\tsl_user.h
  * @author  MCD Application Team
  * @version V1.0.1
  * @date    22-February-2013
  * @brief   Touch-Sensing user configuration and api file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TSL_USER_H
#define __TSL_USER_H

#include "tsl.h"


// Do NOT change the values.
#define GROUP1  (0x0001)
#define GROUP2  (0x0002)
#define GROUP3  (0x0004)
#define GROUP4  (0x0008)
#define GROUP5  (0x0010)
#define GROUP6  (0x0020)
#define GROUP7  (0x0040)
#define GROUP8  (0x0080)
#define GROUP9  (0x0100)
#define GROUP10 (0x0200)


//=======================
// Channel IOs definition
//=======================

//PA6
#define CHANNEL_0_IO_MSK    (TSL_GROUP2_IO1)
#define CHANNEL_0_GRP_MSK   (GROUP2)
#define CHANNEL_0_SRC       (1) // Index in source register (TSC->IOGXCR[])
#define CHANNEL_0_DEST      (0) // Index in destination result array
//PC4
#define CHANNEL_1_IO_MSK    (TSL_GROUP9_IO1)
#define CHANNEL_1_GRP_MSK   (GROUP9)
#define CHANNEL_1_SRC       (8) // Index in source register (TSC->IOGXCR[])
#define CHANNEL_1_DEST      (1) // Index in destination result array
//PB0
#define CHANNEL_2_IO_MSK    (TSL_GROUP3_IO1)
#define CHANNEL_2_GRP_MSK   (GROUP3)
#define CHANNEL_2_SRC       (2) // Index in source register (TSC->IOGXCR[])
#define CHANNEL_2_DEST      (2) // Index in destination result array

//======================
// Shield IOs definition
//======================

#define SHIELD_IO_MSK       (0)	//None

//=================
// Banks definition
//=================

#define BANK_0_NBCHANNELS    (3)
#define BANK_0_MSK_CHANNELS  (CHANNEL_0_IO_MSK | CHANNEL_1_IO_MSK | CHANNEL_2_IO_MSK)
#define BANK_0_MSK_GROUPS    (CHANNEL_0_GRP_MSK | CHANNEL_1_GRP_MSK | CHANNEL_2_GRP_MSK)

// User Parameters
extern CONST TSL_Bank_T MyBanks[];
extern CONST TSL_TouchKey_T MyTKeys[];
extern CONST TSL_Object_T MyObjects[];
extern TSL_ObjectGroup_T MyObjGroup;

void TSL_user_Init(void);
TSL_Status_enum_T TSL_user_Action(void);
void TSL_user_SetThresholds(void);

#endif /* __TSL_USER_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
