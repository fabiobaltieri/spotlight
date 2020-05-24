/* Target definition */
#define TARGET_PCA10040 1
#define TARGET_SPOTLIGHT 2
#define TARGET_RETROFIT 3

#define TARGET TARGET_PCA10040

/* SDK config overrides */

#if TARGET == TARGET_SPOTLIGHT
#define NRF_BL_DFU_ENTER_METHOD_BUTTON_PIN 6
#endif
