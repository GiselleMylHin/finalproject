#ifndef FSR_H
#define FSR_H
#include "stm32l4xx_hal.h"
#include <stdint.h>

#define FSR_THRESHOLD   700     /* ADC counts above this = pressed (0-4095) */

/*
 * Pin / Channel map:
 *   PA0 = ADC1_CH5  = FSR_UP
 *   PA1 = ADC1_CH6  = FSR_DOWN
 *   PA4 = ADC1_CH9  = FSR_LEFT
 *   PA6 = ADC1_CH11 = FSR_RIGHT
 */
typedef enum {
    FSR_UP    = 0,
    FSR_DOWN  = 1,
    FSR_LEFT  = 2,
    FSR_RIGHT = 3
} FSR_Channel;

void     FSR_init(void);
uint16_t FSR_read(FSR_Channel ch);
uint8_t  FSR_isPressed(FSR_Channel ch);

#endif /* FSR_H */
