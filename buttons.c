#include "buttons.h"

#define START_PIN  GPIO_PIN_0
#define SELECT_PIN GPIO_PIN_1
#define RESET_PIN  GPIO_PIN_2
#define BUTTON_PORT GPIOB

static uint8_t read_button(uint16_t pin)
{
    if (HAL_GPIO_ReadPin(BUTTON_PORT, pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(30);

        if (HAL_GPIO_ReadPin(BUTTON_PORT, pin) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(BUTTON_PORT, pin) == GPIO_PIN_RESET)
            {
                HAL_Delay(10);
            }

            return 1;
        }
    }

    return 0;
}

void Buttons_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = START_PIN | SELECT_PIN | RESET_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
}

uint8_t Button_StartPressed(void)
{
    return read_button(START_PIN);
}

uint8_t Button_SelectPressed(void)
{
    return read_button(SELECT_PIN);
}

uint8_t Button_ResetPressed(void)
{
    return read_button(RESET_PIN);
}
