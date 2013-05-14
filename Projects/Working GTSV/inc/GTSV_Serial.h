/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GTSV_SERIAL_H_INCLUDED
#define __GTSV_SERIAL_H_INCLUDED

/* Includes ------------------------------------------------------------------*/

#define SERIAL_COM_PORT		USART1


enum Serial_Cmd_Enum_t {
	SERIAL_CMD_LIGHT,
	SERIAL_CMD_PLUS,
	SERIAL_CMD_MINUS,
	SERIAL_CMD_TIMER,
	SERIAL_CMD_AUTO,
	SERIAL_CMD_UID,
	SERIAL_CMD_ACK,
	SERIAL_TOTAL_CMD
};




#define SERIAL_RX_CMD_SIZE		(88)
#define SERIAL_RX_NUM_OF_CMD		(8)

//#define SERIAL_TX_BUFF_SIZE		(88)


struct Serial_Parrams_t {
	uint8_t buff[SERIAL_RX_NUM_OF_CMD][SERIAL_RX_CMD_SIZE];
	uint8_t nbr_of_cmd;  //number of unprocessed cmd in buff
	uint8_t next_cmd_idx;
	uint8_t receiving_cmd_idx;
	
	uint32_t other_uid[3];
	unsigned other_uid_valid:1;
};


struct Serial_Cmd_Result_t {
	enum Serial_Cmd_Enum_t cmd;
	uint8_t data[SERIAL_RX_CMD_SIZE - 5];
	uint8_t data_len;   //num of bytes in data
	volatile uint8_t *raw_buff;
	uint8_t raw_buff_len;  //num of bytes in raw buff
	
};

extern struct Serial_Parrams_t _serial_parrams;
extern struct Serial_Cmd_Result_t _serial_cmd_results;

/* Exported constants --------------------------------------------------------*/

void Usart_to_default_config(void);
void Serial_rx_ISR(void);
void Serial_tx_ISR(void);
void Serial_cmd_detect(void);
bool Serial_check_cmd(enum Serial_Cmd_Enum_t cmd);
void Serial_cmd_detect(void);



#endif

