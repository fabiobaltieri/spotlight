#define LED_1          17 // 31
#define LED_2          18 // 30

#define LEDS_LIST { LED_1, LED_2 }
#define LEDS_NUMBER    2
#define LEDS_ACTIVE_STATE 0

#define POWER_LED_1          19 // 31
#define POWER_LED_2          20 // 30
#define POWER_LED_3          22 // 29
#define POWER_LED_4          23 // 28

#define BUTTON_1       13 // 5
#define BUTTON_2       14 // 6

#define BSP_BUTTON_0   BUTTON_1
#define BSP_BUTTON_1   BUTTON_2

#define BUTTONS_NUMBER 2
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1, BUTTON_2 }

#define TWI_SCL        24
#define TWI_SDA        25
