
#include "GTSV_TSense.h"

void Tsense_to_default_config(void)
{
	uint8_t i;
	// Enable Comparator clock
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_COMP, ENABLE);
	/* Init TSL RC */
	TSL_Init();

	#if NUMBER_OF_SINGLE_CHANNEL_KEYS > 0
	for (i = 0; i < NUMBER_OF_SINGLE_CHANNEL_KEYS; i++)
	{
		sSCKeyInfo[i].Setting.b.IMPLEMENTED = 1;
		sSCKeyInfo[i].Setting.b.ENABLED = 1;
		sSCKeyInfo[i].DxSGroup = 0x01;
	}
	#endif
	// Change thresholds of specific keys
	#if defined (KEY01)
	  TS_KEY01.DetectThreshold = 99;
	  TS_KEY01.EndDetectThreshold = 95;
	  TS_KEY01.RecalibrationThreshold = -22;
	#endif
}

void Tsense_action(void)
{
	/* Run TSL RC state machine */
	  TSL_Action();
}