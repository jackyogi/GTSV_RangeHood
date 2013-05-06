

uint16_t _gTick_ms, _gTick_sec;

/**
  * @brief  Management of the timing module interrupt service routine.
  * @param  None
  * @retval None
  */
void Time_tick_processIT(void)
{
	static uint16_t count_1s = 0;
	// Count 1 global tick every xxx ms (defined by TIME_TICK_FREQ_Hz parameter)
	_gTick_ms++;
	// Check if 1 second has elapsed
	count_1s++;
	if (count_1s > (TIME_TICK_FREQ_Hz - 1))
	{
		count_1s = 0;
		_gTick_sec++;

	}
}


/**
  * @brief  Check if a delay (in ms) has elapsed.
  * This function must be called regularly due to counter Roll-over only managed one time.
  * @param[in] delay_ms  Delay in ms
  * @param[in] last_tick Variable holding the last tick value
  * @retval Status
  */
Status_enum_t Time_tick_CheckDelay_ms(uint16_t delay_ms, __IO uint16_t *last_tick)
{
  uint16_t tick;
  uint16_t diff;

  //disableInterrupts();

  tick = _gTick_ms;

  if (delay_ms == 0)
  {
    //enableInterrupts();
    return STATUS_ERROR;
  }

  // Counter Roll-over management
  if (tick >= *last_tick)
  {
    diff = tick - *last_tick;
  }
  else
  {
    diff = (0xFFFF - *last_tick) + tick + 1;
  }

#if (TSLPRM_TICK_FREQ == 125)
  if (diff >= (TSL_tTick_ms_T)(delay_ms >> 3)) // Divide by 8 for 8ms tick
#endif
#if (TSLPRM_TICK_FREQ == 250)
  if (diff >= (TSL_tTick_ms_T)(delay_ms >> 2)) // Divide by 4 for 4ms tick
#endif
#if (TSLPRM_TICK_FREQ == 500)
  if (diff >= (TSL_tTick_ms_T)(delay_ms >> 1)) // Divide by 2 for 2ms tick
#endif
#if (TSLPRM_TICK_FREQ == 1000)
  if (diff >= (TSL_tTick_ms_T)delay_ms) // Direct value for 1ms tick
#endif
#if (TSLPRM_TICK_FREQ == 2000)
  if (diff >= (TSL_tTick_ms_T)(delay_ms << 1)) // Multiply by 2 for 0.5ms tick
#endif
  {
    // Save current time
    *last_tick = tick;
    //enableInterrupts();
    return TSL_STATUS_OK;
  }

  //enableInterrupts();
  return TSL_STATUS_BUSY;

}


/**
  * @brief  Check if a delay (in s) has elapsed.
  * @param[in] delay_sec  Delay in seconds
  * @param[in] last_tick Variable holding the last tick value
  * @retval Status
  */
TSL_Status_enum_T TSL_tim_CheckDelay_sec(TSL_tTick_sec_T delay_sec, __IO TSL_tTick_sec_T *last_tick)
{
  TSL_tTick_sec_T tick;
  TSL_tTick_sec_T diff;

  disableInterrupts();

  tick = TSL_Globals.Tick_sec;

  if (delay_sec == 0)
  {
    enableInterrupts();
    return TSL_STATUS_ERROR;
  }

  // Counter Roll-over management
  if (tick >= *last_tick)
  {
    diff = (TSL_tTick_sec_T)(tick - *last_tick);
  }
  else
  {
    diff = (TSL_tTick_sec_T)((63 - *last_tick) + tick + 1); // DTO counter is on 6 bits
  }

  if (diff >= delay_sec)
  {
    // Save current time
    *last_tick = tick;
    enableInterrupts();
    return TSL_STATUS_OK;
  }

  enableInterrupts();
  return TSL_STATUS_BUSY;

}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

