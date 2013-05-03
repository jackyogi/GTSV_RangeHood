
#include "IRremote.h"
#include "main.h"


#define GPIOx_IDR_OFFSET	0x10   		//the offset of IDR register
#define IRR_SENS_DATA_ADDRESS		((volatile uint8_t *) \
			(BITBAND_PERI(GPIOB_BASE+GPIOx_IDR_OFFSET, IRR_SENS_PIN)))
			
#define IRR_TIMER_TICK_us	50		//microseconds per clock interupt tick


#define IRR_MIN_GAP 		(5000/50) //minimum gap between transmission




// Pulse parms are *50-100 for the Mark and *50+100 for the space
// First MARK is the one after the long gap
// pulse parameters in usec
#define NEC_HDR_MARK	9000
#define NEC_HDR_SPACE	4500
#define NEC_BIT_MARK	560
#define NEC_ONE_SPACE	1600
#define NEC_ZERO_SPACE	560
#define NEC_RPT_SPACE	2250

#define IRR_NEC_BITS 32

// Marks tend to be 100us too long, and spaces 100us too short
// when received due to sensor lag.
#define IRR_MARK_EXCESS	100
#define IRR_ERR_CODE	0
#define IRR_SUCCESS_CODE	1

#define TOLERANCE 25  // percent tolerance in measurements
#define LTOL (1.0 - TOLERANCE/100.) 
#define UTOL (1.0 + TOLERANCE/100.) 


#define TICKS_LOW(us) (int) (((us)*LTOL/IRR_TIMER_TICK_us))
#define TICKS_HIGH(us) (int) (((us)*UTOL/IRR_TIMER_TICK_us+ 1))




struct irrparams_t irrparams;
struct irr_decode_results_t irr_decode_results;


void Irr_init(void)
{
  	irrparams.recv_pin = IRR_SENS_DATA_ADDRESS;

}

int MATCH(int measured_ticks, int desired_us)
{
	return ( (measured_ticks>= TICKS_LOW(desired_us)) 
		  && (measured_ticks<= TICKS_HIGH(desired_us)) );
}

int MATCH_MARK(int measure_ticks, int desired_us)
{
	return MATCH(measure_ticks, (desired_us + IRR_MARK_EXCESS));
}

int MATCH_SPACE(int measure_ticks, int desired_us)
{
	return MATCH(measure_ticks, (desired_us - IRR_MARK_EXCESS));
}

int decodeNEC(struct irr_decode_results_t *results){
	uint32_t data = 0;
	int offset =1; //Skip first space
	int i;
	
	//Initial mark
	if(!MATCH_MARK(results->rawbuff[offset], NEC_HDR_MARK)){
		return IRR_ERR_CODE;
	}
	offset++;
	//check for repeat
	if(irrparams.rawbuff_len == 4 &&
		MATCH_SPACE(results->rawbuff[offset], NEC_RPT_SPACE) &&
		MATCH_MARK(results->rawbuff[offset+1], NEC_BIT_MARK)){
		results->decode_type = IRR_DECODE_NEC;
		results->value = IRR_NEC_REPEAT;
		results->value_bit_len = 0;
		return IRR_SUCCESS_CODE;		
	}
	
	if(irrparams.rawbuff_len < 2*IRR_NEC_BITS+4){
		return IRR_ERR_CODE;
	}
	//check initial space
	if(!MATCH_SPACE(results->rawbuff[offset], NEC_HDR_SPACE)){
		return IRR_ERR_CODE;
	}
	offset++;
	for(i = 0; i< IRR_NEC_BITS; i++){
		//check for first mark
		if(!MATCH_MARK(results->rawbuff[offset],NEC_BIT_MARK)){
			return IRR_ERR_CODE;
		}
		offset++;
		//now get the data depending on the length of the space
		if(MATCH_SPACE(results->rawbuff[offset], NEC_ONE_SPACE)){
			data = (data<<1) | 1;  // insert one
		} else if(MATCH_SPACE(results->rawbuff[offset], NEC_ZERO_SPACE)){
			data <<= 1;  //insert zero
		} else {
			return IRR_ERR_CODE;
		}
		offset++;
	}
	//success!
	results->value_bit_len = IRR_NEC_BITS;
	results->value = data;
	results->decode_type = IRR_DECODE_NEC;
	return IRR_SUCCESS_CODE;
	
}


//return 0 if no data ready, 1 if decode ok
//the decode results are store in results
int Irr_decode(struct irr_decode_results_t *results)
{
	results->rawbuff = irrparams.rawbuff;
	results->rawbuff_len = irrparams.rawbuff_len;

	if(irrparams.recv_state != IRR_STATE_STOP){
		return IRR_ERR_CODE;
	}
	if(decodeNEC(results)){
		return IRR_SUCCESS_CODE;
	}



	//Throw away & start over
	Irr_resume();
	return IRR_ERR_CODE;
}
void Irr_resume(void)
{
	irrparams.recv_state = IRR_STATE_IDLE;
	irrparams.rawbuff_len = 0;
}


