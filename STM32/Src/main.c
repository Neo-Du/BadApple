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

#define IMG_WIDTH 240
#define IMG_HEIGHT 180
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
FRESULT res;

char fileName[] = { "landscape_15_fps.bin" };
//char fileName[] = {"badapple_15_fps.bin"};

uint8_t buf0_ready = 0;
uint8_t buf1_ready = 0;
volatile uint8_t line_start = 0;
volatile uint8_t dma_cplt = 0;

uint16_t pos[12][2] = { { 15, 14 }, { 265, 14 }, { 517, 14 }, { 769, 14 }, { 15, 209 }, { 265, 209 }, { 517, 209 }, { 769, 209 }, { 15, 404 }, { 265, 404 }, { 517, 404 }, { 769, 404 } };
uint16_t img_buf[43200] = { 0 };

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config (void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
    //HAL_DMA2D_Start (&hdma2d, (uint32_t) &pic_array, SDRAM_BANK_ADDR, 640, 480);

    int16_t a = 0xf800; //RED
    int16_t b = 0x07E0; //Green
    int16_t c = 0x001F; //Blue
    int16_t d = 0xF81F; //R+B

    int32_t t = 0;

    for (t = 0; t < 640 * 120; t++)
    {
	aMemory0[t] = a;
    }
    for (; t < 640 * 240; t++)
    {
	aMemory0[t] = b;
    }
    for (; t < 640 * 360; t++)
    {
	aMemory0[t] = c;
    }
    for (; t < 640 * 480; t++)
    {
	aMemory0[t] = d;
    }
}
void DMA2D_cplt (struct __DMA2D_HandleTypeDef*hdma2d)
{
    dma_cplt = 1;
}

FRESULT open_files ()
{
    FRESULT res;
    UINT dmy;
    FILINFO ino;

    res = f_open (&SDFile, fileName, FA_READ);

    f_stat (fileName, &ino);

    hdma2d.XferCpltCallback = DMA2D_cplt;
    HAL_LTDC_ProgramLineEvent (&hltdc, 0);

    while (res == FR_OK && SDFile.fptr < ino.fsize)
    {
	/* any other processes... */
	/* Fill output stream periodicaly or on-demand */

	res = f_read (&SDFile, aMemory1, VIDEO_WIDTH * VIDEO_HEIGHT * 2, &dmy);

	while (line_start != 1);
	if (HAL_DMA2D_Start_IT (&hdma2d, aMemory1, aMemory0, VIDEO_WIDTH, VIDEO_HEIGHT) != HAL_OK)
	{
	    Error_Handler ();
	}
	while (dma_cplt != 1);
	dma_cplt = 0;
	line_start = 0;
	printf ("+++++++++++++++\r\n");
    }
    f_close (&SDFile);
}

void HAL_LTDC_LineEventCallback (LTDC_HandleTypeDef*hltdc)
{
    __HAL_LTDC_ENABLE_IT(hltdc, LTDC_IT_LI);
    line_start = 1;
}

void pic_Array_test ()
{
    HAL_LTDC_SetWindowSize (&hltdc, 1024, 600, 0);
    HAL_LTDC_SetWindowPosition (&hltdc, 0, 0, 0);

    int16_t a = 0xf800; //RED
    int16_t b = 0x07E0; //Green
    int16_t c = 0x001F; //Blue
    int16_t d = 0xF81F; //R+B

    int32_t t = 0;

    for (int p = 0; p < 1024 * 600; p++)
    {
	aMemory2[p] = 0x00;
    }
    HAL_LTDC_SetAddress (&hltdc, aMemory2, 0);
    uint32_t start_time = 0;
    while (1)
    {
	start_time = HAL_GetTick ();
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	while (t++ < 43200)
	{
	    x1 = t % IMG_WIDTH;
	    y1 = t / IMG_WIDTH;
	    aMemory2[pos[0][0] + x1 + 1024 * (pos[0][1] + y1)] = pic_array[t];
	    aMemory2[pos[1][0] + x1 + 1024 * (pos[1][1] + y1)] = pic_array[t];
	    aMemory2[pos[2][0] + x1 + 1024 * (pos[2][1] + y1)] = pic_array[t];
	    aMemory2[pos[3][0] + x1 + 1024 * (pos[3][1] + y1)] = pic_array[t];
	    aMemory2[pos[4][0] + x1 + 1024 * (pos[4][1] + y1)] = pic_array[t];
	    aMemory2[pos[5][0] + x1 + 1024 * (pos[5][1] + y1)] = pic_array[t];
	    aMemory2[pos[6][0] + x1 + 1024 * (pos[6][1] + y1)] = pic_array[t];
	    aMemory2[pos[7][0] + x1 + 1024 * (pos[7][1] + y1)] = pic_array[t];
	    aMemory2[pos[8][0] + x1 + 1024 * (pos[8][1] + y1)] = pic_array[t];
	    aMemory2[pos[9][0] + x1 + 1024 * (pos[9][1] + y1)] = pic_array[t];
	    aMemory2[pos[10][0] + x1 + 1024 * (pos[10][1] + y1)] = pic_array[t];
	    aMemory2[pos[11][0] + x1 + 1024 * (pos[11][1] + y1)] = pic_array[t];
	}
	printf ("end %d\r\n", HAL_GetTick () - start_time);
	printf ("+++++++++++++\r\n");
    }
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
    //LCD_test ();
    pic_Array_test ();

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
	open_files ();

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
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 96;
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
