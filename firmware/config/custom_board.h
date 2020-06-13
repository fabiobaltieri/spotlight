/* Per target definitions */
#if TARGET == TARGET_PCA10040

#define TARGET_HAS_REMOTE 1
#define TARGET_HAS_EXT_TEMP 0
#define TARGET_HAS_PWM 1

#define LED_1 17
#define LED_2 18
#define BUTTON_1 13
#define POWER_LED_1 19
#define POWER_LED_2 20
#define POWER_LED_3 22
#define POWER_LED_4 23
#define POWER_LED_POLARITY APP_PWM_POLARITY_ACTIVE_LOW
#define PWM_PERIOD_US 20L
#define TWI_SCL 24
#define TWI_SDA 25
#define BATTERY_SENSE_INPUT NRF_SAADC_INPUT_VDD

#elif TARGET == TARGET_SPOTLIGHT

#define TARGET_HAS_REMOTE 1
#define TARGET_HAS_EXT_TEMP 1
#define TARGET_HAS_PWM 1

#define LED_1 7
#define LED_2 8
#define BUTTON_1 6
#define POWER_LED_1 28
#define POWER_LED_2 29
#define POWER_LED_3 30
#define POWER_LED_4 31
#define POWER_LED_POLARITY APP_PWM_POLARITY_ACTIVE_HIGH
#define PWM_PERIOD_US 20L
#define TWI_SCL 27
#define TWI_SDA 26
#define BATTERY_SENSE_INPUT NRF_SAADC_INPUT_AIN3

#elif TARGET == TARGET_RETROFIT

#define TARGET_HAS_REMOTE 0
#define TARGET_HAS_EXT_TEMP 0
#define TARGET_HAS_PWM 0

#define LED_1 22
#define LED_2 20 // nc
#define BUTTON_1 23 // nc
#define POWER_LED_1 24 // nc
#define POWER_LED_2 25 // nc
#define POWER_LED_3 26 // nc
#define POWER_LED_4 27 // nc
#define POWER_LED_POLARITY APP_PWM_POLARITY_ACTIVE_HIGH
#define PWM_PERIOD_US 20L
#define TWI_SCL 18 // nc
#define TWI_SDA 19 // nc
#define BATTERY_SENSE_INPUT NRF_SAADC_INPUT_AIN4

#elif TARGET == TARGET_ACTIK

#define TARGET_HAS_REMOTE 0
#define TARGET_HAS_EXT_TEMP 0
#define TARGET_HAS_PWM 1

#define LED_1 30 // nc
#define LED_2 31
#define BUTTON_1 29
#define POWER_LED_1 22
#define POWER_LED_2 5
#define POWER_LED_3 6
#define POWER_LED_4 11 // nc
#define POWER_LED_POLARITY APP_PWM_POLARITY_ACTIVE_HIGH
#define PWM_PERIOD_US 2000L
#define TWI_SCL 8
#define TWI_SDA 7
#define BATTERY_SENSE_INPUT NRF_SAADC_INPUT_AIN2

#else
#error "TARGET undefined"
#endif

/* Common definitions */

#define LEDS_LIST {LED_1, LED_2}
#define LEDS_NUMBER 2
#define LEDS_ACTIVE_STATE 0

#define BSP_BUTTON_0 BUTTON_1

#define BUTTONS_NUMBER 1
#define BUTTON_PULL NRF_GPIO_PIN_PULLUP
#define BUTTONS_ACTIVE_STATE 0
#define BUTTONS_LIST { BUTTON_1 }
