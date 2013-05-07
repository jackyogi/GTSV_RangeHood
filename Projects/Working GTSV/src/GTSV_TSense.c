
#include "GTSV_TSense.h"


uint8_t RisingEdge[5], HighLevel[5], KeyHold[5];
uint16_t delay_hold[5];

void Tsense_key_detect_first(void)
{
	int i;
	for(i=0; i<5; i++){
		if((sSCKeyInfo[i].Setting.b.DETECTED) && (!RisingEdge[i]) && (!HighLevel[i]))
			RisingEdge[i] = 1;
		else if((!sSCKeyInfo[i].Setting.b.DETECTED) && (HighLevel[i]))
			HighLevel[i] = 0;
		if(HighLevel[i]){
			if(delay_hold[i]<55){
				delay_hold[i]++;
			}else{
				KeyHold[i] =1;
			}
		}else{
			KeyHold[i]=0;
			delay_hold[i]=0;
		}
	}
}


uint8_t Tsense_check_rising_edge(enum Tsense_key_enum_t key)
{
	return RisingEdge[key];
}
uint8_t Tsense_check_high_level(enum Tsense_key_enum_t key)
{
	return HighLevel[key];
}
uint8_t Tsense_check_key_hold(enum Tsense_key_enum_t key)
{
	return KeyHold[key];
}
void Tsense_key_detect_last(void)
{
	int i;
	for(i=0; i<5; i++){
		if(RisingEdge[i]){
			RisingEdge[i] =0;
			HighLevel[i] = 1;
			Buzzer_bip();
		}
	}
}




void Tsense_to_default_config(void)
{
	uint8_t i;
	// Enable Comparator clock
  	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_COMP, ENABLE);
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