#define LED_1          17 // 31
#define LED_2          18 // 30
#define LED_3          19 // 29
#define LED_4          20 // 28

#define LEDS_LIST { LED_1, LED_2, LED_3, LED_4 }
#define LEDS_NUMBER    4
#define LEDS_ACTIVE_STATE 0

#define BUTTON_1       13 // 5
#define BUTTON_2       14 // 6

#define BSP_BUTTON_0   BUTTON_1
#define BSP_BUTTON_1   BUTTON_2

#define BUTTONS_NUMBER 2
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1, BUTTON_2 }

#define TWI_SCL        27
#define TWI_SDA        26
