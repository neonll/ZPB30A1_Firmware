# Module interface
* NAME_init(): Initialize hardware and variables. Set module to a safe state
* NAME_timer(): Called when the systick timer counts from the main loop.
* NAME_timer_irq(): Called when the systick timer counts directly from the IRQ
    handler. Should return as fast as possible!
* NAME_handler(): Called from the main loop. Used for functions which should
    be called as fast as possible.

# Timers
* TIM1: CCR1: I-set
* TIM2: Systick
* TIM3: CCR2: Fan
* TIM4:
