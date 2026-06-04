#include "dfplayer.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef *df_uart = NULL;
static uint8_t playing = 0;

static void send(const char *cmd)
{
    if (!df_uart) return;
    HAL_UART_Transmit(df_uart, (uint8_t*)cmd, strlen(cmd), 500);
    HAL_Delay(300);
}

void DFPlayer_Init(UART_HandleTypeDef *huart, UART_HandleTypeDef *dbg)
{
    df_uart = huart;
    playing = 0;
    (void)dbg;
    HAL_Delay(4000);
    send("AT+SLAVE=OFF\r\n");
    send("AT+VOL=20\r\n");
    send("AT+AMP=ON\r\n");
}

void DFPlayer_Play(const char *filename)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "AT+PLAYFILE=/%s\r\n", filename);
    send(buf);
    playing = 1;
}

void DFPlayer_Pause(void)   { send("AT+PLAY=PP\r\n"); playing = 0; }
void DFPlayer_Resume(void)  { send("AT+PLAY=PP\r\n"); playing = 1; }
void DFPlayer_Stop(void)    { send("AT+STOP\r\n");    playing = 0; }
void DFPlayer_ToggleButton(const char *f) { DFPlayer_Play(f); }
void DFPlayer_SetVolume(uint8_t v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "AT+VOL=%u\r\n", v);
    send(buf);
}
