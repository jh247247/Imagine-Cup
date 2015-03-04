#ifndef TIMER_H
#define TIMER_H

#define sysTicksPerMillisecond 439
extern volatile unsigned int g_adcFlag;
extern volatile unsigned int g_sysTick;

int TIM_init();
int TIM_initTimeout(int timeout);
int TIM_checkTimeout();

#endif /* TIMER_H */
