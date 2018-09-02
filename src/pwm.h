/*
 * Basic pwm driver for esp32
 */

#ifndef _PWM_H_
#define _PWM_H_

int pwm_init(void);
void pwm_set_duty(int channel, float duty);

#endif /* _PWM_H_ */
