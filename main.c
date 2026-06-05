#include "main.h"
#include "ili9341.h"
#include "fsr.h"
#include "dfplayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void GPIO_Config(void);
static void SPI1_Config(void);
static void LPUART1_Config(void);
static void USART2_Config(void);

typedef enum { EASY = 0, MEDIUM = 1, HARD = 2 } Difficulty;
static const char *diff_names[] = { "EASY", "MEDIUM", "HARD" };
static const uint32_t diff_window[] = { 400, 400, 400 };

#define HISTORY_SIZE 20
static uint8_t  hit_history[HISTORY_SIZE];
static uint8_t  history_idx  = 0;
static uint8_t  history_full = 0;
static uint32_t score        = 0;
static uint32_t combo        = 0;
static Difficulty difficulty  = EASY;

static void Score_RecordHit(uint8_t hit)
{
    hit_history[history_idx] = hit;
    history_idx = (history_idx + 1) % HISTORY_SIZE;
    if (history_idx == 0) history_full = 1;
    if (hit) { score += (10 * (combo + 1)); combo++; }
    else      { combo = 0; }
}

static void Score_UpdateDifficulty(void)
{
    uint8_t count = history_full ? HISTORY_SIZE : history_idx;
    if (count < 10) return;
    uint32_t hits = 0;
    for (uint8_t i = 0; i < count; i++) hits += hit_history[i];
    uint32_t accuracy = (hits * 100) / count;
    char buf[64];
    sprintf(buf, "ACCURACY=%lu%% DIFF=%s\r\n", accuracy, diff_names[difficulty]);
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
    if (accuracy >= 85 && difficulty < HARD)      difficulty++;
    else if (accuracy <= 60 && difficulty > EASY) difficulty--;
}

typedef struct { uint32_t time_ms; uint8_t direction; } Note;

static const Note chart_jagger_easy[] = {
    {418,0},{4227,1},{8037,2},{11846,3},{15656,0},{19465,1},
    {23275,2},{27084,3},{30894,0},{34703,1},{38513,2},{42322,3},
    {46132,0},{49941,1},{53751,2},{57560,3},{61370,0},{65179,1},
    {68989,2},{72798,3}
};
static const Note chart_jagger_medium[] = {
    {418,0},{2322,1},{4227,2},{6132,3},{8037,0},{9941,1},
    {11846,2},{13751,3},{15656,0},{17560,1},{19465,2},{21370,3},
    {23275,0},{25179,1},{27084,2},{28989,3},{30894,0},{32798,1},
    {34703,2},{36608,3},{38513,0},{40418,1},{42322,2},{44227,3},
    {46132,0},{48037,1},{49941,2},{51846,3},{53751,0},{55656,1},
    {57560,2},{59465,3}
};
static const Note chart_jagger_hard[] = {
    {418,0},{1370,1},{2322,2},{3275,3},{4227,0},{5179,1},
    {6132,2},{7084,3},{8037,0},{8989,1},{9941,2},{10894,3},
    {11846,0},{12798,1},{13751,2},{14703,3},{15656,0},{16608,1},
    {17560,2},{18513,3},{19465,0},{20418,1},{21370,2},{22322,3},
    {23275,0},{24227,1},{25179,2},{26132,3},{27084,0},{28037,1},
    {28989,2},{29941,3}
};

typedef struct {
    const char  *name;
    const char  *filename;
    const Note  *easy;
    const Note  *medium;
    const Note  *hard;
    uint8_t      easy_count;
    uint8_t      medium_count;
    uint8_t      hard_count;
    uint32_t     duration_ms;
} Song;

static const Song songs[] = {
    { "MOVES LIKE JAGGER", "0001.mp3",
      chart_jagger_easy, chart_jagger_medium, chart_jagger_hard,
      20, 32, 32,
      73000 },
};
static const uint8_t NUM_SONGS = 1;
static uint8_t current_song = 0;

typedef enum { ARROW_UP=0, ARROW_DOWN=1, ARROW_LEFT=2, ARROW_RIGHT=3, ARROW_NONE=99 } Arrow;

void uart_print(char *msg)
{
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

static uint8_t PB0_WasPressed(void)
{
    if ((GPIOB->IDR & GPIO_PIN_0) == 0)
    {
        HAL_Delay(50);
        if ((GPIOB->IDR & GPIO_PIN_0) == 0)
        {
            while ((GPIOB->IDR & GPIO_PIN_0) == 0) HAL_Delay(10);
            return 1;
        }
    }
    return 0;
}

void DrawArrow(Arrow arrow)
{
    switch (arrow)
    {
        case ARROW_UP:    ILI9341_DrawUpArrow(ILI9341_RED);      break;
        case ARROW_DOWN:  ILI9341_DrawDownArrow(ILI9341_GREEN);  break;
        case ARROW_LEFT:  ILI9341_DrawLeftArrow(ILI9341_BLUE);   break;
        case ARROW_RIGHT: ILI9341_DrawRightArrow(ILI9341_YELLOW);break;
        default: break;
    }
}

static void DrawHUD(void)
{
    char buf[80];
    sprintf(buf, "SCORE=%lu COMBO=%lu DIFF=%s\r\n",
            score, combo, diff_names[difficulty]);
    uart_print(buf);

    char sc[20], cb[20], df[20];
    sprintf(sc, "SC:%lu  ", score);
    sprintf(cb, "CB:%lu  ", combo);
    sprintf(df, "%s  ", diff_names[difficulty]);
    ILI9341_DrawString(2,  2,  sc, ILI9341_WHITE,  ILI9341_BLACK, 2);
    ILI9341_DrawString(2,  20, cb, ILI9341_YELLOW, ILI9341_BLACK, 2);
    ILI9341_DrawString(2,  38, df, ILI9341_CYAN,   ILI9341_BLACK, 2);
}

static void DrawFeedback(const char *text, uint16_t color)
{
    ILI9341_FillRect(2, 278, 160, 30, ILI9341_BLACK);
    ILI9341_DrawString(2, 283, text, color, ILI9341_BLACK, 2);
}

static void PlaySong(uint8_t song_idx)
{
    const Song *s = &songs[song_idx];
    const Note *chart;

    if      (difficulty == EASY)   chart = s->easy;
    else if (difficulty == MEDIUM) chart = s->medium;
    else                           chart = s->hard;

    char buf[64];
    sprintf(buf, "SONG: %s | DIFF: %s\r\n", s->name, diff_names[difficulty]);
    uart_print(buf);

    DFPlayer_Play(s->filename);
    uint32_t song_start = HAL_GetTick();
    uint32_t window     = diff_window[difficulty];

    uint8_t note_count = (difficulty == EASY) ? s->easy_count :
                         (difficulty == MEDIUM) ? s->medium_count : s->hard_count;

    for (uint8_t i = 0; i < note_count; i++)
    {
        uint32_t note_time = song_start + chart[i].time_ms;

        while (HAL_GetTick() < note_time)
        {
            if ((HAL_GetTick() - song_start) >= s->duration_ms)
            {
                uart_print("SONG TIME LIMIT\r\n");
                DFPlayer_Stop();
                goto song_done;
            }
            if (PB0_WasPressed())
            {
                uart_print("PAUSED\r\n");
                DFPlayer_Pause();
                uint32_t pause_start = HAL_GetTick();
                while (!PB0_WasPressed()) HAL_Delay(10);
                uint32_t pause_duration = HAL_GetTick() - pause_start;
                uart_print("RESUMED\r\n");
                DFPlayer_Resume();
                song_start += pause_duration;
                note_time  += pause_duration;
            }
            HAL_Delay(5);
        }

        Arrow target = (Arrow)chart[i].direction;
        DrawArrow(target);
        DrawHUD();

        uint32_t win_start = HAL_GetTick();
        uint8_t  judged    = 0;

        while ((HAL_GetTick() - win_start) < window)
        {
            if ((HAL_GetTick() - song_start) >= s->duration_ms)
            {
                DFPlayer_Stop();
                goto song_done;
            }

            uint16_t up    = FSR_read(FSR_UP);
            uint16_t down  = FSR_read(FSR_DOWN);
            uint16_t left  = FSR_read(FSR_LEFT);
            uint16_t right = FSR_read(FSR_RIGHT);

            char dbg[100];
            sprintf(dbg, "UP=%u DOWN=%u LEFT=%u RIGHT=%u\r\n", up, down, left, right);
            uart_print(dbg);

            Arrow pressed = ARROW_NONE;
            if      (FSR_isPressed(FSR_UP))    pressed = ARROW_UP;
            else if (FSR_isPressed(FSR_DOWN))  pressed = ARROW_DOWN;
            else if (FSR_isPressed(FSR_LEFT))  pressed = ARROW_LEFT;
            else if (FSR_isPressed(FSR_RIGHT)) pressed = ARROW_RIGHT;

            if (pressed != ARROW_NONE)
            {
                if (pressed == target)
                {
                    /* Press in first 40% of window = PERFECT (on beat)
                     * Press in last 60% of window  = GOOD (late)        */
                    uint32_t elapsed = HAL_GetTick() - win_start;
                    if (elapsed <= (window * 4 / 10))
                    {
                        uart_print("PERFECT\r\n");
                        DrawFeedback("PERFECT", ILI9341_GREEN);
                        Score_RecordHit(1);
                        score += 5;
                    }
                    else
                    {
                        uart_print("GOOD\r\n");
                        DrawFeedback("GOOD", ILI9341_YELLOW);
                        Score_RecordHit(1);
                    }
                }
                else
                {
                    uart_print("WRONG\r\n");
                    DrawFeedback("MISS", ILI9341_RED);
                    Score_RecordHit(0);
                }

                while (FSR_isPressed(FSR_UP)    ||
                       FSR_isPressed(FSR_DOWN)  ||
                       FSR_isPressed(FSR_LEFT)  ||
                       FSR_isPressed(FSR_RIGHT))
                { HAL_Delay(10); }

                judged = 1;
                HAL_Delay(300);
                DrawFeedback("        ", ILI9341_BLACK);
                break;
            }
            HAL_Delay(5);
        }

        if (!judged)
        {
            uart_print("MISS\r\n");
            Score_RecordHit(0);
            DrawFeedback("MISS", ILI9341_RED);
            HAL_Delay(300);
            DrawFeedback("        ", ILI9341_BLACK);
        }

        if ((i + 1) % 10 == 0) Score_UpdateDifficulty();
    }

    DFPlayer_Stop();

song_done:
    sprintf(buf, "SONG DONE! SCORE=%lu\r\n", score);
    uart_print(buf);

    /* ── End screen ─────────────────────────────────────────── */
    ILI9341_FillScreen(ILI9341_BLACK);

    /* Pick message based on score */
    const char *msg;
    if      (score >= 300) msg = "AMAZING!";
    else if (score >= 200) msg = "GREAT JOB!";
    else if (score >= 100) msg = "GOOD JOB!";
    else if (score >= 50)  msg = "KEEP IT UP";
    else                   msg = "PRACTICE!";

    /* Draw message centered */
    ILI9341_DrawString(20, 60,  msg,          ILI9341_YELLOW, ILI9341_BLACK, 3);

    /* Draw score */
    char end_sc[30], end_cb[30];
    sprintf(end_sc, "SCORE: %lu", score);
    sprintf(end_cb, "COMBO: %lu", combo);
    ILI9341_DrawString(20, 130, end_sc, ILI9341_WHITE,  ILI9341_BLACK, 2);
    ILI9341_DrawString(20, 155, end_cb, ILI9341_YELLOW, ILI9341_BLACK, 2);

    /* Draw difficulty */
    char end_df[30];
    sprintf(end_df, "DIFF: %s", diff_names[difficulty]);
    ILI9341_DrawString(20, 180, end_df, ILI9341_CYAN, ILI9341_BLACK, 2);

    HAL_Delay(4000);
    ILI9341_FillScreen(ILI9341_BLACK);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    USART2_Config();
    LPUART1_Config();
    GPIO_Config();
    SPI1_Config();

    ILI9341_Init();
    ILI9341_FillScreen(ILI9341_BLACK);

    FSR_init();
    FSR_Calibrate();
    DFPlayer_Init(&huart2, &hlpuart1);

    /* TEMP TEST: pulse PLAY pin 3 times on boot to verify GPIO works */
    for (int i = 0; i < 3; i++)
    {
        GPIOB->BRR  = GPIO_PIN_1;
        HAL_Delay(200);
        GPIOB->BSRR = GPIO_PIN_1;
        HAL_Delay(500);
    }

    srand(HAL_GetTick());

    uart_print("\r\n=== MOVE LIKE JAGGER STARTED ===\r\n");
    uart_print("Press PB0 to start!\r\n");

    while (!PB0_WasPressed()) HAL_Delay(10);

    while (1)
    {
        PlaySong(current_song);
        current_song = (current_song + 1) % NUM_SONGS;
        score = combo = 0;
        history_idx = history_full = 0;
        memset(hit_history, 0, sizeof(hit_history));

        uart_print("Press PB0 to play again!\r\n");
        while (!PB0_WasPressed()) HAL_Delay(10);
    }
}

static void GPIO_Config(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOCEN |
                    RCC_AHB2ENR_GPIOGEN  | RCC_AHB2ENR_GPIOBEN;
    HAL_PWREx_EnableVddIO2();

    GPIOA->MODER &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA->MODER |=  ((2U << (2*2)) | (2U << (3*2)));
    GPIOA->OTYPER  &= ~(GPIO_PIN_2 | GPIO_PIN_3);
    GPIOA->OSPEEDR |=  ((3U << (2*2)) | (3U << (3*2)));
    GPIOA->PUPDR   &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA->AFR[0]  &= ~((0xFU << GPIO_AFRL_AFSEL2_Pos) | (0xFU << GPIO_AFRL_AFSEL3_Pos));
    GPIOA->AFR[0]  |=  ((7U << GPIO_AFRL_AFSEL2_Pos) | (7U << GPIO_AFRL_AFSEL3_Pos));

    GPIOB->MODER &= ~(3U << (0*2));
    GPIOB->PUPDR &= ~(3U << (0*2));
    GPIOB->PUPDR |=  (1U << (0*2));

    GPIOA->MODER &= ~((3U << (5*2)) | (3U << (7*2)));
    GPIOA->MODER |=  ((2U << (5*2)) | (2U << (7*2)));
    GPIOA->OTYPER  &= ~(GPIO_PIN_5 | GPIO_PIN_7);
    GPIOA->OSPEEDR |=  ((3U << (5*2)) | (3U << (7*2)));
    GPIOA->PUPDR   &= ~((3U << (5*2)) | (3U << (7*2)));
    GPIOA->AFR[0]  &= ~((0xFU << GPIO_AFRL_AFSEL5_Pos) | (0xFU << GPIO_AFRL_AFSEL7_Pos));
    GPIOA->AFR[0]  |=  ((5U << GPIO_AFRL_AFSEL5_Pos) | (5U << GPIO_AFRL_AFSEL7_Pos));

    GPIOC->MODER &= ~((3U << (3*2)) | (3U << (4*2)) | (3U << (8*2)));
    GPIOC->MODER |=  ((1U << (3*2)) | (1U << (4*2)) | (1U << (8*2)));
    GPIOC->OTYPER  &= ~(GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_8);
    GPIOC->OSPEEDR |=  ((3U << (3*2)) | (3U << (4*2)) | (3U << (8*2)));
    GPIOC->PUPDR   &= ~((3U << (3*2)) | (3U << (4*2)) | (3U << (8*2)));
    GPIOC->BSRR = GPIO_PIN_4;
    GPIOC->BSRR = GPIO_PIN_8;
    GPIOC->BRR  = GPIO_PIN_3;
}

static void SPI1_Config(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 = 0; SPI1->CR2 = 0;
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
    SPI1->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_LSBFIRST | SPI_CR1_BR);
    SPI1->CR2 &= ~SPI_CR2_DS;
    SPI1->CR2 |= (7U << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static void LPUART1_Config(void)
{
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    GPIOG->MODER &= ~((3U << (7*2)) | (3U << (8*2)));
    GPIOG->MODER |=  ((2U << (7*2)) | (2U << (8*2)));
    GPIOG->AFR[0] &= ~(0xFU << GPIO_AFRL_AFSEL7_Pos);
    GPIOG->AFR[0] |=  (8U   << GPIO_AFRL_AFSEL7_Pos);
    GPIOG->AFR[1] &= ~(0xFU << GPIO_AFRH_AFSEL8_Pos);
    GPIOG->AFR[1] |=  (8U   << GPIO_AFRH_AFSEL8_Pos);
    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&hlpuart1);
}

static void USART2_Config(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&huart2);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) Error_Handler();
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|
                                  RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

void Error_Handler(void) { __disable_irq(); while (1) {} }

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
