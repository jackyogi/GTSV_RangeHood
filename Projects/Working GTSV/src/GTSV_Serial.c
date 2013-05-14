
#include "main.h"
#include "GTSV_serial.h"



struct Serial_Parrams_t _serial_parrams = {
	.nbr_of_cmd = 0,
	.next_cmd_idx = 0,
	.receiving_cmd_idx = 0,
	.other_uid = {0,0,0},
	.other_uid_valid = 0,
};

struct Serial_Cmd_Result_t _serial_cmd_results;

struct Serial_Cmd_Detect_t {
	unsigned active:1;
	unsigned hold:1;
} _serial_cmd_detect[SERIAL_TOTAL_CMD];


bool Serial_cmd_do_decode(struct Serial_Cmd_Result_t *results)
{
	static uint8_t tmp_cmd;
	tmp_cmd = results->cmd;
	switch(tmp_cmd){
	case SERIAL_CMD_LIGHT:
	case SERIAL_CMD_PLUS:
	case SERIAL_CMD_MINUS:
	case SERIAL_CMD_TIMER:
	case SERIAL_CMD_AUTO:
		return TRUE;
		break;
	case SERIAL_CMD_UID:
		//save uid
		//make uid valid
		return TRUE;
		break;
	default:
		return FALSE;
		break;
	}

}


//first byte is len, 2nd byte is cmd
bool Serial_cmd_decode(struct Serial_Cmd_Result_t *results)
{
	if(_serial_parrams.nbr_of_cmd>0){  //if has at least one cmd in buff
		results->raw_buff_len = _serial_parrams.buff[_serial_parrams.next_cmd_idx][0];  //the first byte is buf len
		results->raw_buff = &(_serial_parrams.buff[_serial_parrams.next_cmd_idx][1]);
		results->cmd = *results->raw_buff;
		_serial_parrams.nbr_of_cmd--;
		_serial_parrams.next_cmd_idx++;
		if(_serial_parrams.next_cmd_idx == SERIAL_RX_NUM_OF_CMD)
			_serial_parrams.next_cmd_idx = 0;

		if(Serial_cmd_do_decode(results)){
			return TRUE;
		}
		return FALSE;
	}else{
		return FALSE;
	}
}




void Serial_cmd_detect(void)
{
	static uint8_t tmp_cmd;
	int i;

	if(Serial_cmd_decode(&_serial_cmd_results)){
		tmp_cmd = _serial_cmd_results.cmd;

		switch(tmp_cmd){
		case SERIAL_CMD_LIGHT:
			_serial_cmd_detect[SERIAL_CMD_LIGHT].active = 1;
			Buzzer_bip();
			break;
		case SERIAL_CMD_PLUS:
			_serial_cmd_detect[SERIAL_CMD_PLUS].active = 1;
			Buzzer_bip();
			break;
		case SERIAL_CMD_MINUS:
			_serial_cmd_detect[SERIAL_CMD_MINUS].active = 1;
			Buzzer_bip();
			break;
		case SERIAL_CMD_TIMER:
			_serial_cmd_detect[SERIAL_CMD_TIMER].active = 1;
			Buzzer_bip();
			break;
		case SERIAL_CMD_AUTO:
			_serial_cmd_detect[SERIAL_CMD_AUTO].active = 1;
			Buzzer_bip();
			break;
		case SERIAL_CMD_UID:
			_serial_cmd_detect[SERIAL_CMD_UID].active = 1;
			Buzzer_bip();
			break;
		default:
			tmp_cmd = 0;
			break;
		}
		
	}else{
		tmp_cmd = 0;
		for(i=0; i < SERIAL_TOTAL_CMD; i++){
			_serial_cmd_detect[i].active = 0;
		}
	}

}

void Serial_tx_ISR(void)
{

}

#define SERIAL_RX_FRAME_SOF	1
#define SERIAL_RX_FRAME_EOF	0

#define SERIAL_RX_STATE_IDLE	0
#define SERIAL_RX_STATE_RECEIVING 	1
void Serial_rx_ISR(void)
{
	static uint8_t serial_rx_state = SERIAL_RX_STATE_IDLE;
	uint8_t recv_tmp;
	recv_tmp = (uint8_t)(USART_ReceiveData(SERIAL_COM_PORT) & 0x00FF);
	if(serial_rx_state == SERIAL_RX_STATE_IDLE){
		if(recv_tmp == SERIAL_RX_FRAME_SOF){
			serial_rx_state = SERIAL_RX_STATE_RECEIVING;
		}
	} else if (serial_rx_state == SERIAL_RX_STATE_RECEIVING){
		if(recv_tmp != SERIAL_RX_FRAME_EOF){ //not end of frame
			_serial_parrams.buff
		}else{
			
		}
	}
}

bool Serial_check_cmd(enum Serial_Cmd_Enum_t cmd)
{
	return _serial_cmd_detect[cmd].active;
	
}

bool Serial_get_other_UID(uint32_t * p_other_uid)
{
	if(_serial_parrams.other_uid_valid){
		*p_other_uid = _serial_parrams.other_uid[0];
		*(p_other_uid+1) = _serial_parrams.other_uid[1];
		*(p_other_uid+2) = _serial_parrams.other_uid[2];
		return TRUE;
	}else{
		return FALSE;
	}
	
}

void Usart_to_default_config(void)
{
	USART_InitTypeDef USART_InitStructure;
  	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* USARTx configuration ----------------------------------------------------*/
	/* USARTx configured as follow:
	- BaudRate = 57600 baud  
	- Word Length = 8 Bits
	- One Stop Bit
	- No parity
	- Hardware flow control disabled (RTS and CTS signals)
	- Receive and transmit enabled
	*/
	USART_InitStructure.USART_BaudRate = 57600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	/* Enable the USARTx Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = INT_PRIORITY_USART1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable USART */
	USART_Cmd(USART1, ENABLE);
	

}

