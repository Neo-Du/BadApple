/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "dma2d.h"
#include "fatfs.h"
#include "ltdc.h"
#include "sdio.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
//#include "RGB565_240x130_1.h"
//#include "L8_320X240.h"
#include "test_pic_1.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
    while ((USART1->SR & 0X40) == 0);
    USART1->DR = (uint8_t) ch;
    return ch;
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SDRAM_BANK_ADDR     ((uint32_t)0XC0000000)
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

#define VIDEO_ARRAY_WIDTH 240
#define VIDEO_ARRAY_HEIGHT 180

#define IMG_WIDTH 240
#define IMG_HEIGHT 180

#define LCD_WIDTH 1024
#define LCD_HEIGHT 600

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern LTDC_HandleTypeDef hltdc;
extern DMA2D_HandleTypeDef hdma2d;

extern const uint16_t pic_array[43200];

//uint32_t aMemory[262144] __attribute__((section(".ExtRAMData")));
uint16_t aMemory0[1200 * 800] __attribute__((section(".ExtRAMData"))); // 1024 * 1024 /4    //1MB / 4
uint16_t aMemory1[1200 * 800] __attribute__((section(".ExtRAMData1"))); // 1024 * 1024 /4    //1MB / 4
uint16_t aMemory2[1200 * 800] __attribute__((section(".ExtRAMData2"))); // 1024 * 1024 /4    //1MB / 4

extern FATFS SDFatFS; /* File system object for SD logical drive */
extern FIL SDFile; /* File object for SD */
extern char SDPath[4];

char video_badApple_240x180[] = { "badapple_15_fps_240_180.bin" };
char video_badApple_640x480[] = { "badapple_15_fps.bin" };
char video_badApple_800x600[] = { "badApple_15_fps_800_600.bin" };

char video_landscape_640x480[] = { "landscape_15_fps.bin" };
char video_landscape_1024x600[] = { "landscape_15_fps_1024_600.bin" };

uint8_t buf0_ready = 0;
uint8_t buf1_ready = 0;
volatile uint8_t line_start = 0;
volatile uint8_t dma_cplt = 0;

uint16_t pos[12][2] = { { 15, 14 }, { 265, 14 }, { 517, 14 }, { 769, 14 }, { 15, 209 }, { 265, 209 }, { 517, 209 }, { 769, 209 }, { 15, 404 }, { 265, 404 }, { 517, 404 }, { 769, 404 } };
uint16_t img_buf[43200] = { 0 };
LTDC_ColorTypeDef lcd_back_color;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config (void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void DMA2D_cplt (struct __DMA2D_HandleTypeDef*hdma2d)
{
    dma_cplt = 1;
}
void HAL_LTDC_LineEventCallback (LTDC_HandleTypeDef*hltdc)
{
    __HAL_LTDC_ENABLE_IT(hltdc, LTDC_IT_LI);
    line_start = 1;
}

FRESULT scan_files (char*path) /* Start node to be scanned (also used as work area) */
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    char *fn; /* This function is assuming non-Unicode cfg. */
    res = f_opendir (&dir, path); /* Open the directory */
    if (res == FR_OK)
    {
	i = strlen (path);
	for (;;)
	{
	    res = f_readdir (&dir, &fno); /* Read a directory item */
	    if (res != FR_OK)
	    {
		break; /* Break on error or end of dir */
	    }
	    else if (fno.fname[0] == 0)
	    {
		printf ("%s\r\n", path);
		break;
	    }
	    fn = fno.fname;
	    if (fno.fattrib & AM_DIR)
	    { /* It is a directory */
		sprintf (&path[i], "/%s", fn);
		res = scan_files (path);
		if (res != FR_OK)
		    break;
		path[i] = 0;
	    }
	    else
	    { /* It is a file. */
		printf ("%s/%s\r\n", path, fn);
	    }
	}
    }
    return res;
}

void LCD_test ()
{
    int32_t t = 0;

    for (t = 0; t < 640 * 120; t++)
    {
	aMemory0[t] = 0xf800; //RED
    }
    for (; t < 640 * 240; t++)
    {
	aMemory0[t] = 0x07E0; //Green
    }
    for (; t < 640 * 360; t++)
    {
	aMemory0[t] = 0x001F; //Blue
    }
    for (; t < 640 * 480; t++)
    {
	aMemory0[t] = 0xF81F; //R+B
    }
}

void play_video (char*video_name,uint16_t vidoe_width,uint16_t video_hight)
{
    FRESULT res;
    UINT dmy;
    FILINFO ino;

    HAL_LTDC_SetWindowSize (&hltdc, vidoe_width, video_hight, 0);
    HAL_LTDC_SetWindowPosition (&hltdc, (LCD_WIDTH - vidoe_width) / 2, (LCD_HEIGHT - video_hight) / 2, 0);
    HAL_LTDC_ProgramLineEvent (&hltdc, 0);

    res = f_open (&SDFile, video_name, FA_READ);

    f_stat (video_name, &ino);

    hdma2d.XferCpltCallback = DMA2D_cplt;
    uint32_t t1 = 0;
    uint32_t t2 = 0;

    while (res == FR_OK && SDFile.fptr < ino.fsize)
    {
	HAL_GPIO_TogglePin (GPIOB, GPIO_PIN_1);
	t1 = HAL_GetTick();
	res = f_read (&SDFile, aMemory1, vidoe_width * video_hight * 2, &dmy);

	t2 = HAL_GetTick();
	printf("F_read: %d \r\n",t2 - t1);

	while (line_start != 1);

	t1 = HAL_GetTick();
	printf("line_start: %d \r\n",t1 - t2);

	if (HAL_DMA2D_Start_IT (&hdma2d, aMemory1, aMemory0, vidoe_width, video_hight) != HAL_OK)
	{
	    Error_Handler ();
	}
	while (dma_cplt != 1);
	t2 = HAL_GetTick();
	printf("DMA: %d \r\n",t2 - t1);

	dma_cplt = 0;
	line_start = 0;
    }
    f_close (&SDFile);
}
void pic_Array_test ()
{
    HAL_LTDC_SetWindowSize (&hltdc, 1024, 600, 0);
    HAL_LTDC_SetWindowPosition (&hltdc, 0, 0, 0);

    uint32_t pos_x = 0;
    uint32_t pos_y = 0;
    uint32_t pix = 0;
    uint32_t a1 = 0;
    uint32_t a2 = 0;
    uint32_t a3 = 0;
    uint8_t pos_num = 0;
    int p = 0;

    for (p = 0; p < 1024 * 600; p++)
    {
	aMemory0[p] = 0x00; // black
    }
    p = 0;
    for (; p < 240 * 45; p++)
    {
	img_buf[p] = 0xf800; // red
    }
    for (; p < 240 * 90; p++)
    {
	img_buf[p] = 0x07E0; // green
    }
    for (p; p < 240 * 135; p++)
    {
	img_buf[p] = 0x001F; // blue
    }
    for (; p < 240 * 180; p++)
    {
	img_buf[p] = 0xF81F; // purple
    }

    while (pix++ < 43200)
    {
	pos_x = pix % IMG_WIDTH;
	pos_y = pix / IMG_WIDTH;

	for (pos_num = 0; pos_num < 12; pos_num++)
	{
	    a1 = pos[pos_num][0] + pos_x;
	    a2 = 1024 * pos[pos_num][1] + 1024 * pos_y;
	    a3 = a1 + a2;
	    aMemory0[a3] = img_buf[pix];
	}
    }
}

void video_Array (char*vidoe_array_name)
{
    FRESULT res;
    UINT dmy;
    FILINFO ino;

    uint32_t pos_x = 0;
    uint32_t pos_y = 0;
    uint32_t pix = 0;
    uint32_t a1 = 0;
    uint32_t a2 = 0;
    uint32_t a3 = 0;
    uint8_t pos_num = 0;

    for (int p = 0; p < 1024 * 600; p++)
    {
	aMemory1[p] = 0b1111100000000000;
    }

    res = f_open (&SDFile, vidoe_array_name, FA_READ);

    f_stat (vidoe_array_name, &ino);

    hdma2d.XferCpltCallback = DMA2D_cplt;
    HAL_LTDC_ProgramLineEvent (&hltdc, 0);
    HAL_LTDC_SetWindowSize (&hltdc, 1024, 600, 0);
    HAL_LTDC_SetWindowPosition (&hltdc, 0, 0, 0);
    dma_cplt = 1;
    while (res == FR_OK && SDFile.fptr < ino.fsize)
    {
	HAL_GPIO_TogglePin (GPIOB, GPIO_PIN_1);
	f_read (&SDFile, img_buf, VIDEO_ARRAY_WIDTH * VIDEO_ARRAY_HEIGHT * 2, &dmy);
	pix = 0;
	while (pix++ < 43200)
	{
	    pos_x = pix % IMG_WIDTH;
	    pos_y = pix / IMG_WIDTH;

	    for (pos_num = 0; pos_num < 12; pos_num++)
	    {
		a1 = pos[pos_num][0] + pos_x;
		a2 = 1024 * pos[pos_num][1] + 1024 * pos_y;
		a3 = a1 + a2;
		aMemory1[a3] = img_buf[pix];
	    }
	}

	while (line_start != 1);

	while (dma_cplt != 1);

	if (HAL_DMA2D_Start_IT (&hdma2d, aMemory1, aMemory0, 1024, 600) != HAL_OK)
	{
	    Error_Handler ();
	}
	dma_cplt = 0;
	line_start = 0;
    }
    f_close (&SDFile);
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main (void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init ();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config ();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init ();
    MX_DMA_Init ();
    MX_FMC_Init ();
    MX_LTDC_Init ();
    MX_DMA2D_Init ();
    MX_SDIO_SD_Init ();
    MX_FATFS_Init ();
    MX_USART1_UART_Init ();
    /* USER CODE BEGIN 2 */
    SDRAM_Init ();
//    LCD_test ();
//    HAL_Delay (1000);

    printf ("Start\r\n");
    /*##-2- Register the file system object to the FatFs module ##############*/
    printf ("#2  %d\r\n", HAL_GetTick ());
    if (f_mount (&SDFatFS, (TCHAR const*) SDPath, 0) != FR_OK)
    {
	/* FatFs Initialization Error */
	Error_Handler ();
    }
    else
    {
//	scan_files (SDPath);
//	pic_Array_test ();
//	HAL_Delay (1000);
//	play_video (video_badApple_240x180, 240, 180);		//12FPS
//	play_video (video_landscape_640x480, 640, 480);		//12FPS
//	play_video (video_badApple_640x480, 640, 480);		//12FPS
//	play_video (video_badApple_800x600, 800, 600);		//8FPS
	play_video (video_landscape_1024x600, 1024, 600);	//6.2FPS

//	video_Array (video_badApple_240x180);

    }
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
	printf ("hello\r\n");
	HAL_Delay (1000);
	/* USER CODE END WHILE */

	/* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config (void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 15;
    RCC_OscInitStruct.PLL.PLLN = 216;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 8;
    if (HAL_RCC_OscConfig (&RCC_OscInitStruct) != HAL_OK)
    {
	Error_Handler ();
    }
    /** Activate the Over-Drive mode
     */
    if (HAL_PWREx_EnableOverDrive () != HAL_OK)
    {
	Error_Handler ();
    }
    /** Initializes the CPU, AHB and APB busses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig (&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
	Error_Handler ();
    }
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 84;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
    if (HAL_RCCEx_PeriphCLKConfig (&PeriphClkInitStruct) != HAL_OK)
    {
	Error_Handler ();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler (void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    printf ("Error\r\n");
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
