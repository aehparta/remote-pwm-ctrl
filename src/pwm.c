/*
 * Basic pwm driver for esp32
 */

#include <driver/mcpwm.h>
#include "pwm.h"


int pwm_init(void)
{
	mcpwm_config_t cfg;

	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, 5);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, 18);
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, 23);

	cfg.frequency = 1000;
	cfg.cmpr_a = 0;
	cfg.cmpr_b = 0;
	cfg.counter_mode = MCPWM_UP_COUNTER;
	cfg.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &cfg);

	cfg.frequency = 1000;
	cfg.cmpr_a = 0;
	cfg.cmpr_b = 0;
	cfg.counter_mode = MCPWM_UP_COUNTER;
	cfg.duty_mode = MCPWM_DUTY_MODE_0;
	mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &cfg);

	return 0;
}

void pwm_set_duty(int channel, float duty)
{
	switch (channel) {
	case 0:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);
		break;
	case 1:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, duty);
		break;
	case 2:
		mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty);
		break;
	}
}

