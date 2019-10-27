#if 1
#define LED_1          7
#define LED_2          8

#define LEDS_LIST { LED_1, LED_2 }
#define LEDS_NUMBER    2
#define LEDS_ACTIVE_STATE 0

#define POWER_LED_1          28
#define POWER_LED_2          29
#define POWER_LED_3          30
#define POWER_LED_4          31

#define BUTTON_1       5
#define BUTTON_2       6

#define BSP_BUTTON_0   BUTTON_1
#define BSP_BUTTON_1   BUTTON_2

#define BUTTONS_NUMBER 2
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1, BUTTON_2 }

#define TWI_SCL        27
#define TWI_SDA        26
#else /* PCA10040 pins */
#define LED_1          17
#define LED_2          18

#define LEDS_LIST { LED_1, LED_2 }
#define LEDS_NUMBER    2
#define LEDS_ACTIVE_STATE 0

#define POWER_LED_1          19
#define POWER_LED_2          20
#define POWER_LED_3          22
#define POWER_LED_4          23

#define BUTTON_1       13
#define BUTTON_2       14

#define BSP_BUTTON_0   BUTTON_1
#define BSP_BUTTON_1   BUTTON_2

#define BUTTONS_NUMBER 2
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1, BUTTON_2 }

#define TWI_SCL        24
#define TWI_SDA        25
#endif
