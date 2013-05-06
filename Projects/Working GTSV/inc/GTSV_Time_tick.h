

#define TIME_TICK_FREQ_Hz		(1000)

typedef enum
{
	STATUS_OK = 0,
	STATUS_BUSY = 2,
	STATUS_ERROR = 2
} Status_enum_t;

void Time_tick_processIT(void);