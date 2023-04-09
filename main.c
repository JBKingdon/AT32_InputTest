/**
 **************************************************************************
 * Test input capture
 * Derived from AT32 bsp examples for pwm output and input capture.
 * Connect the pwm output to the input capture input
 * See the #defines for configuring the input
 *
 * Artery original copyright below...
 **************************************************************************
 *                       Copyright notice & Disclaimer
 *
 * The software Board Support Package (BSP) that is made available to
 * download from Artery official website is the copyrighted work of Artery.
 * Artery authorizes customers to use, copy, and distribute the BSP
 * software and its related documentation for the purpose of design and
 * development in conjunction with Artery microcontrollers. Use of the
 * software is governed by this copyright notice and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
 * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
 * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
 * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
 * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
 *
 **************************************************************************
 */

#include "at32f435_437_board.h"
#include "at32f435_437_clock.h"

// The example code was originally written with timer3 ch2 on A07

// Timers 1, 8 and 20 are advanced timers and should be usable. Might be different to 2 through 5
// Timers 2 through 5 are all the same class of general purpose timers. They should all work the same way

// Timers 6 and 7 are basic timers and don't support input capture
// Timers 9 through 14 don't support dma and can't be used

// tmr	ch	mux	pin	pass/fail

// 2
// 3	2	2	A07	pass
// .	4	2	C09	pass
// 4
// 5	1	2	A00	fail	reads 4394


// 1
// 8	3	3	C08	fail	reads 4394
// .	4	3	C09	fail	reads 4394
// 20


// Configure the input timer

// Original config is timer3, ch2, A07 - works, returns 36k
// NB if changing the timer, make sure there is an interrupt handler for it

#define INPUT_TIMER		TMR3
#define INPUT_PORT		GPIOA
#define INPUT_PIN		GPIO_PINS_7
#define INPUT_SEL_CH	TMR_SELECT_CHANNEL_2
#define INPUT_CH_FLAG	TMR_C2_FLAG
#define INPUT_INT		TMR_C2_INT
#define INPUT_IRQn		TMR3_GLOBAL_IRQn

#define INPUT_TIMER_CLK	CRM_TMR3_PERIPH_CLOCK
#define INPUT_PIN_CLK	CRM_GPIOA_PERIPH_CLOCK

#define INPUT_MUX_SRC	GPIO_PINS_SOURCE7
#define INPUT_MUXn		GPIO_MUX_2

//// tim3, ch4, c09, mux2 - works
//#define INPUT_TIMER		TMR3
//#define INPUT_PORT		GPIOC
//#define INPUT_PIN		GPIO_PINS_9
//#define INPUT_SEL_CH	TMR_SELECT_CHANNEL_4
//#define INPUT_CH_FLAG	TMR_C4_FLAG
//#define INPUT_INT		TMR_C4_INT
//#define INPUT_IRQn		TMR3_GLOBAL_IRQn
//
//#define INPUT_TIMER_CLK	CRM_TMR3_PERIPH_CLOCK
//#define INPUT_PIN_CLK	CRM_GPIOC_PERIPH_CLOCK
//
//#define INPUT_MUX_SRC	GPIO_PINS_SOURCE9
//#define INPUT_MUXn		GPIO_MUX_2

// tim5, mux2 is pins A0 through A3 - check at-start schematic
// A0 is user key, should be ok, but has a 100k pull down on it
// Also reads 4394 instead of the expected 36k
//#define INPUT_TIMER		TMR5
//#define INPUT_PORT		GPIOA
//#define INPUT_PIN		GPIO_PINS_0
//#define INPUT_SEL_CH	TMR_SELECT_CHANNEL_1
//#define INPUT_CH_FLAG	TMR_C1_FLAG
//#define INPUT_INT		TMR_C1_INT
//#define INPUT_IRQn		TMR5_GLOBAL_IRQn
//
//#define INPUT_TIMER_CLK	CRM_TMR5_PERIPH_CLOCK
//#define INPUT_PIN_CLK	CRM_GPIOA_PERIPH_CLOCK
//
//#define INPUT_MUX_SRC	GPIO_PINS_SOURCE0
//#define INPUT_MUXn		GPIO_MUX_2


//// tim8, ch4, c09, mux3	-- returns the wrong result, 4394. why?
//// Try time 8 ch3 for comparison, on C08 mux3. 4394 on ch3 too, so something odd about tmr8
//// tim8 is an advanced timer, maybe try another general purpose timer (2 through 5)
//#define INPUT_TIMER		TMR8
//#define INPUT_PORT		GPIOC
//#define INPUT_PIN		GPIO_PINS_8
//#define INPUT_SEL_CH	TMR_SELECT_CHANNEL_3
//#define INPUT_CH_FLAG	TMR_C3_FLAG
//#define INPUT_INT		TMR_C3_INT
//#define INPUT_IRQn		TMR8_CH_IRQn
//
//#define INPUT_TIMER_CLK	CRM_TMR8_PERIPH_CLOCK
//#define INPUT_PIN_CLK	CRM_GPIOC_PERIPH_CLOCK
//
//#define INPUT_MUX_SRC	GPIO_PINS_SOURCE8
//#define INPUT_MUXn		GPIO_MUX_3


// Globals
__IO uint32_t inputfreq = 0;
__IO uint32_t sys_counter = 0;
__IO uint16_t capturenumber = 0;
__IO uint16_t ic3readvalue1 = 0, ic3readvalue2 = 0;
__IO uint32_t capture = 0;

tmr_output_config_type tmr_oc_init_structure;
crm_clocks_freq_type crm_clocks_freq_struct = {0};

uint16_t ccr1_val = 249;
uint16_t prescalervalue = 0;

void crm_configuration(void);
void gpio_configuration(void);
void startOutputPWM(void);
void startInputCapture(void);



/**
 * Setup a pwm output and an input capture input to read it.
 *
 * Output signal is on B08 driven by timer10
 *
 * Connect the output to the input (physically with a wire), the measured frequency
 * will be written to uart every 2 seconds. LED4 will toggle with each debug string.
 * Expected frequency is 36kHz
 */
int main(void)
{
	system_clock_config();	// modified to match the input example, should be equivalent

	/* system peripheral configuration */
	crm_configuration();

	at32_board_init();
	at32_led_on(LED4);

	/* get system clock */
	crm_clocks_freq_get(&crm_clocks_freq_struct);

	/* gpio configuration */
	gpio_configuration();

	uart_print_init(115200);
	printf("Input capture test, expected result is 36k\n");

	startOutputPWM();

	startInputCapture();

	while(1)
	{
		delay_ms(2000);
		printf("frequency : %ld\n", inputfreq);
		inputfreq = 0;
		at32_led_toggle(LED4);
	}

} // main()

/**
 *  input timer configuration: input capture mode
 *
 *  Original config: tmr3, ch2, pin A07
 *
 *  Config now set by #defines at the top of the file
 *
 *  the rising edge is used as active edge,
 *  see the interrupt handler for how the delta between edges is used to compute the freq
*/
void startInputCapture(void)
{
	tmr_input_config_type  tmr_input_config_struct;

	/* timer counter mode configuration */
	tmr_base_init(INPUT_TIMER, 0x8FFF, 0);
	tmr_cnt_dir_set(INPUT_TIMER, TMR_COUNT_UP);

	/* configure timer channel to get input signal */
	tmr_input_config_struct.input_channel_select = INPUT_SEL_CH;
	tmr_input_config_struct.input_mapped_select = TMR_CC_CHANNEL_MAPPED_DIRECT;
	tmr_input_config_struct.input_polarity_select = TMR_INPUT_RISING_EDGE;
	tmr_input_config_struct.input_filter_value = 2;
	tmr_input_channel_init(TMR3, &tmr_input_config_struct, TMR_CHANNEL_INPUT_DIV_1);

	tmr_interrupt_enable(INPUT_TIMER, INPUT_INT, TRUE);

	/* timer trigger interrupt nvic init */
	nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
	nvic_irq_enable(INPUT_IRQn, 1, 0);

	/* enable timer */
	tmr_counter_enable(INPUT_TIMER, TRUE);
}

/**
 * Generate a pwm output signal at 36kHz using tmr10, ch1,
 */
void startOutputPWM(void)
{
	// compute the prescaler value
	// comes out to 11 (divide by 12, ahbclk is 288MHz, so clock runs at 24MHz)
	prescalervalue = (uint16_t) ((crm_clocks_freq_struct.apb2_freq * 2) / 24000000) - 1;

	/* tmr10 time base configuration */
	tmr_base_init(TMR10, 665, prescalervalue); // measured freq is 36kHz (24MHz/666 = 36kHz)
	tmr_cnt_dir_set(TMR10, TMR_COUNT_UP);
	tmr_clock_source_div_set(TMR10, TMR_CLOCK_DIV1);

	tmr_output_default_para_init(&tmr_oc_init_structure);
	tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
	tmr_oc_init_structure.oc_idle_state = FALSE;
	tmr_oc_init_structure.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
	tmr_oc_init_structure.oc_output_state = TRUE;
	tmr_output_channel_config(TMR10, TMR_SELECT_CHANNEL_1, &tmr_oc_init_structure);
	tmr_channel_value_set(TMR10, TMR_SELECT_CHANNEL_1, ccr1_val);
	tmr_output_channel_buffer_enable(TMR10, TMR_SELECT_CHANNEL_1, TRUE);

	tmr_period_buffer_enable(TMR10, TRUE);

	/* tmr enable counter */
	tmr_counter_enable(TMR10, TRUE);
}



/**
 * @brief  configure the pins for the output and input timers
 * Split this up so it's easier to configure the input side?
 *
 * Output is on B08
 *
 * Original input is on A07
 *
 * @param  none
 * @retval none
 */
void gpio_configuration(void)
{
	gpio_init_type gpio_init_struct;

	gpio_default_para_init(&gpio_init_struct);

	// PWM output pin config for B08
	gpio_init_struct.gpio_pins = GPIO_PINS_8;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
	gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init(GPIOB, &gpio_init_struct);

	gpio_pin_mux_config(GPIOB, GPIO_PINS_SOURCE8, GPIO_MUX_3);

	// timer input pin Configuration
	gpio_init_struct.gpio_pins = INPUT_PIN;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
	gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init(INPUT_PORT, &gpio_init_struct);

	gpio_pin_mux_config(INPUT_PORT, INPUT_MUX_SRC, INPUT_MUXn);
}

/**
 * @brief  configures the different peripheral clocks.
 * @param  none
 * @retval none
 */
void crm_configuration(void)
{
	// for PWM output
	/* tmr10 clock enable */
	crm_periph_clock_enable(CRM_TMR10_PERIPH_CLOCK, TRUE);

	/* gpioa gpiob clock enable */
	crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

	// For timer input
	/* enable timer and pin clocks */
	crm_periph_clock_enable(INPUT_TIMER_CLK, TRUE);
	crm_periph_clock_enable(INPUT_PIN_CLK, TRUE);
}

void genericTimerInterrupt(void)
{
	if(tmr_flag_get(INPUT_TIMER, INPUT_CH_FLAG) == SET)
	{
		tmr_flag_clear(INPUT_TIMER, INPUT_CH_FLAG);
		if(capturenumber == 0)
		{
			/* get the Input Capture value */
			ic3readvalue1 = tmr_channel_value_get(INPUT_TIMER, INPUT_SEL_CH);
			capturenumber = 1;
		}
		else if(capturenumber == 1)
		{
			/* get the Input Capture value */
			ic3readvalue2 = tmr_channel_value_get(INPUT_TIMER, INPUT_SEL_CH);

			/* capture computation */
			if (ic3readvalue2 > ic3readvalue1)
			{
				capture = (ic3readvalue2 - ic3readvalue1);
			}
			else
			{
				capture = ((0xFFFF - ic3readvalue1) + ic3readvalue2);
			}
			/* frequency computation */
			inputfreq = (uint32_t) crm_clocks_freq_struct.sclk_freq / capture;
			capturenumber = 0;
		}
	}
}

/**
 * Maps from the required handler names to the generic code.
 * One of these will be needed for each timer used as an input
 */
void TMR3_GLOBAL_IRQHandler(void)
{
	genericTimerInterrupt();
}

void TMR5_GLOBAL_IRQHandler(void)
{
	genericTimerInterrupt();
}


//void TMR8_CH_IRQHandler(void)
//{
//	genericTimerInterrupt();
//}

