/* real board pins */
#if !APP_DEBUGGING_ON_PCA10040

#define LED_1          7
#define LED_2          8

#define LEDS_LIST { LED_1, LED_2 }
#define LEDS_NUMBER    2
#define LEDS_ACTIVE_STATE 0

#define POWER_LED_1          28
#define POWER_LED_2          29
#define POWER_LED_3          30
#define POWER_LED_4          31
#define POWER_LED_POLARITY   APP_PWM_POLARITY_ACTIVE_HIGH

#define BUTTON_1       6

#define BSP_BUTTON_0   BUTTON_1

#define BUTTONS_NUMBER 1
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1 }

#define TWI_SCL        27
#define TWI_SDA        26

#define BATTERY_SENSE_INPUT NRF_SAADC_INPUT_AIN3

/* PCA10040 pins */
#else

#define LED_1          17
#define LED_2          18

#define LEDS_LIST { LED_1, LED_2 }
#define LEDS_NUMBER    2
#define LEDS_ACTIVE_STATE 0

#define POWER_LED_1          19
#define POWER_LED_2          20
#define POWER_LED_3          22
#define POWER_LED_4          23
#define POWER_LED_POLARITY   APP_PWM_POLARITY_ACTIVE_LOW

#define BUTTON_1       13

#define BSP_BUTTON_0   BUTTON_1

#define BUTTONS_NUMBER 1
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1 }

#define TWI_SCL        24
#define TWI_SDA        25

#define BATTERY_SENSE_INPUT NRF_SAADC_INPUT_VDD

#endif
