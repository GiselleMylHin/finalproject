#ifndef DFPLAYER_H
#define DFPLAYER_H

#include "main.h"

void DFPlayer_Init(UART_HandleTypeDef *huart, UART_HandleTypeDef *huart_debug);
void DFPlayer_Play(const char *filename);
void DFPlayer_Pause(void);
void DFPlayer_Resume(void);
void DFPlayer_Stop(void);
void DFPlayer_ToggleButton(const char *filename);
void DFPlayer_SetVolume(uint8_t vol);

#endif
