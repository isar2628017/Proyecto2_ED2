/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define deviceAddress       0x32
#define WriteSlaveAddress   (deviceAddress << 1)
#define ReadSlaveAddress    (deviceAddress << 1)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//Parte 1
uint8_t datoUART;
uint8_t opcionUART;


//Parte 2
uint8_t TxData[3];
uint8_t RxData[3];

char bufferUART[30];
uint8_t indiceUART = 0;
//0 = menu principal
//1 = submenu SPI
uint8_t modoSPI = 0;

//Parte 3
uint8_t TX_Buffer[1];
uint8_t RX_Buffer[2];

uint16_t adcValue = 0;
volatile uint8_t solicitudI2C = 0;

char msg[50];

//Parte 5
volatile uint32_t tiempoEstadoMs = 500;
volatile uint32_t contadorTimerMs = 0;
volatile uint8_t senalCuadradaActiva = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C3_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

void mostrarMenu(void);

void mostrarSubmenuSPI(void);
void procesarComandoSPI(char *comando);
void enviarSPI(uint8_t led, uint16_t tiempo);
void obtenerMedicionI2C(void);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void mostrarMenu(void)	{
    char mensaje[] =
        "\r\n========================\r\n"
        "     MENU PRINCIPAL\r\n"
        "========================\r\n"
        "1. Controlar dispositivo SPI\r\n"
        "2. Obtener medicion de sensor I2C\r\n"
        "\r\nSeleccione una opcion: ";

    HAL_UART_Transmit(&huart2, (uint8_t*)mensaje,strlen(mensaje), HAL_MAX_DELAY);
}


void mostrarSubmenuSPI(void)	{
    char mensaje[] =
        "\r\n========================\r\n"
        "       CONTROL SPI\r\n"
        "========================\r\n"
        "Ingrese LED y tiempo:\r\n"
        "Formato: R/G/B,tiempo\r\n"
        "R = Rojo\r\n"
        "G = Verde\r\n"
        "B = Azul\r\n"
        "Ejemplo: R,500\r\n"
        "\r\nComando: ";

    HAL_UART_Transmit(&huart2, (uint8_t*)mensaje, strlen(mensaje), HAL_MAX_DELAY);
}


void enviarSPI(uint8_t led, uint16_t tiempo)	{
    TxData[0] = led;
    TxData[1] = (tiempo >> 8) & 0xFF;
    TxData[2] = tiempo & 0xFF;

    HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, TxData, RxData, 3, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);
}

void procesarComandoSPI(char *comando)	{
    char ledLetra;
    int tiempo;
    uint8_t led;
    char mensaje[120];
    /* Leer el comando */
    if (sscanf(comando, " %c,%d", &ledLetra, &tiempo) != 2)	{
        const char *error =
            "\r\nFormato invalido. Use R/G/B,tiempo. Ejemplo: R,500\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)error, strlen(error), HAL_MAX_DELAY);

        mostrarSubmenuSPI();
        return;
    }

    /* Validar tiempo */
    if (tiempo < 1 || tiempo > 65535)	{
        const char *error = "\r\nError: tiempo invalido (1-65535 ms).\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)error, strlen(error), HAL_MAX_DELAY);

        mostrarSubmenuSPI();
        return;
    }

    /* Convertir minusculas a mayusculas */
    if (ledLetra >= 'a' && ledLetra <= 'z')	{
        ledLetra = ledLetra - 'a' + 'A';
    }
    /* Identificar el LED */
    if (ledLetra == 'R')	{
        led = 1;
    }
    else if (ledLetra == 'G')	{
        led = 2;
    }
    else if (ledLetra == 'B')	{
        led = 3;
    }
    else	{
        const char *error =
            "\r\nError: seleccione R, G o B.\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)error,	strlen(error), HAL_MAX_DELAY);
        mostrarSubmenuSPI();
        return;
    }

    /*Parte 5 señal cuadrada cada 1ms*/
    __disable_irq();
    tiempoEstadoMs = (uint32_t)tiempo;
    contadorTimerMs = 0;
    HAL_GPIO_WritePin(LED_P5_GPIO_Port, LED_P5_Pin, GPIO_PIN_SET
    );

    senalCuadradaActiva = 1;
    __enable_irq();

    /* PARTE 2: Enviar el comando al ESP32 mediante SPI.*/
    enviarSPI(led, (uint16_t)tiempo);

    /* Confirmar acciones */
    sprintf(
        mensaje,
        "\r\nComando: LED %c, %d ms\r\n"
        "LED del ESP32 controlado por SPI.\r\n"
        "Senal cuadrada de la NUCLEO: %d ms alto y %d ms bajo.\r\n",
        ledLetra,
        tiempo,
        tiempo,
        tiempo);
    HAL_UART_Transmit(&huart2,(uint8_t *)mensaje,strlen(mensaje),HAL_MAX_DELAY);

    modoSPI = 0;
    mostrarMenu();
}


void obtenerMedicionI2C(void)	{
    uint8_t solicitud = '$';
    uint8_t datos[2] = {0, 0};
    char mensaje[50];
    HAL_StatusTypeDef estado;
    uint16_t adcRecibido;
    uint16_t adc8bits;
    uint8_t intento;
    uint8_t lecturaValida = 0;

    estado = HAL_I2C_IsDeviceReady(&hi2c3,(uint16_t)(deviceAddress << 1),10,1000);

    if (estado != HAL_OK)	{
        sprintf(mensaje, "Error de comunicacion I2C: %d\r\n", (int)estado);
        HAL_UART_Transmit(&huart2, (uint8_t *)mensaje,strlen(mensaje), HAL_MAX_DELAY);
        return;
    }
    /* Lectura inicial*/
    estado = HAL_I2C_Master_Transmit(&hi2c3,(uint16_t)(deviceAddress << 1),&solicitud, 1, 1000);

    if (estado != HAL_OK)	{
    	sprintf(mensaje, "Error al transmitir: %d\r\n", (int)estado);
        HAL_UART_Transmit(&huart2, (uint8_t *)mensaje, strlen(mensaje), HAL_MAX_DELAY);
        return;
    }

    HAL_Delay(10);
    estado = HAL_I2C_Master_Receive(&hi2c3,(uint16_t)(deviceAddress << 1), datos, 2, 1000);

    if (estado != HAL_OK)	{
        sprintf(mensaje, "Error al recibir I2C: %d\r\n", (int)estado);
        HAL_UART_Transmit(&huart2, (uint8_t *)mensaje,strlen(mensaje), HAL_MAX_DELAY);
        return;
    }

    HAL_Delay(10);
    /* Lectura valida */
    for (intento = 1; intento <= 3; intento++)	{
        datos[0] = 0;
        datos[1] = 0;
        estado = HAL_I2C_Master_Transmit(&hi2c3, (uint16_t)(deviceAddress << 1), &solicitud, 1, 1000);
        if (estado != HAL_OK)	{
            sprintf(mensaje, "Error al transmitir: %d\r\n", (int)estado);
            HAL_UART_Transmit(&huart2, (uint8_t *)mensaje, strlen(mensaje), HAL_MAX_DELAY);
            return;
        }

        HAL_Delay(10);
        estado = HAL_I2C_Master_Receive(&hi2c3,(uint16_t)(deviceAddress << 1),datos,2,1000);
        if (estado != HAL_OK)	{
            sprintf(mensaje, "Error al recibir I2C: %d\r\n", (int)estado);
            HAL_UART_Transmit(&huart2, (uint8_t *)mensaje, strlen(mensaje), HAL_MAX_DELAY);
            return;
        }

        adcRecibido = (uint16_t)datos[0] | ((uint16_t)datos[1] << 8);
        if (adcRecibido <= 4095){
            lecturaValida = 1;
            break;
        }
        HAL_Delay(10);
    }

    if (lecturaValida == 0)	{
        sprintf(mensaje, "Error: lectura ADC invalida\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)mensaje,strlen(mensaje), HAL_MAX_DELAY);
        return;
    }

    /* Convertir a escala de 0 a 255 */
    adc8bits = (adcRecibido * 255UL) / 4095UL;
    sprintf(mensaje, "ADC: %u\r\n", (unsigned int)adc8bits);
    HAL_UART_Transmit(&huart2, (uint8_t *)mensaje, strlen(mensaje), HAL_MAX_DELAY);
}




/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_I2C3_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  //Parte 5
  if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)	{
      Error_Handler();
  }


  //Parte1
  mostrarMenu();
  HAL_UART_Receive_IT(&huart2, &datoUART, 1);




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1){

	      if (solicitudI2C == 1)	{
	          solicitudI2C = 0;
	          obtenerMedicionI2C();
	          mostrarMenu();
	      }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 3999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 9;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_P5_GPIO_Port, LED_P5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ESP32_CS_GPIO_Port, ESP32_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : LED_P5_Pin */
  GPIO_InitStruct.Pin = LED_P5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_P5_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ESP32_CS_Pin */
  GPIO_InitStruct.Pin = ESP32_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ESP32_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)	{
    if (huart->Instance == USART2)	{
        // Menu principal
        if (modoSPI == 0)	{
            if (datoUART == '1')	{
                /* Entrar al submenu SPI */
                modoSPI = 1;
                indiceUART = 0;
                memset(bufferUART, 0, sizeof(bufferUART));
                mostrarSubmenuSPI();
            }
            else if (datoUART == '2')	{
                char mensaje[] =
                    "\r\nSeleccion: I2C\r\n"
                    "Obteniendo medicion del sensor I2C...\r\n";

                HAL_UART_Transmit(&huart2, (uint8_t *)mensaje,
                                  strlen(mensaje), HAL_MAX_DELAY);

                solicitudI2C = 1;
            }
            else if (datoUART == '\r' || datoUART == '\n')	{
                /* Ignorar ENTER */
            }
            else	{
                char mensaje[] = "\r\nOpcion no valida.\r\n";

                HAL_UART_Transmit(&huart2, (uint8_t *)mensaje,
                                  strlen(mensaje), HAL_MAX_DELAY);

                mostrarMenu();
            }
        }

        // Submenu SPI
        else if (modoSPI == 1)	{
            /* ENTER = terminar comando */
            if (datoUART == '\r' || datoUART == '\n')	{
                if (indiceUART > 0)	{
                    bufferUART[indiceUART] = '\0';
                    procesarComandoSPI(bufferUART);
                    indiceUART = 0;
                    memset(bufferUART, 0, sizeof(bufferUART));
                }
            }
            else	{
                /* Guardar caracter */
                if (indiceUART < sizeof(bufferUART) - 1)	{
                    bufferUART[indiceUART] = datoUART;
                    indiceUART++;
                }
                else	{
                    /* Buffer lleno */
                    char mensaje[] = "\r\nComando demasiado largo.\r\n";

                    indiceUART = 0;
                    memset(bufferUART, 0, sizeof(bufferUART));

                    HAL_UART_Transmit(&huart2, (uint8_t *)mensaje,
                                      strlen(mensaje), HAL_MAX_DELAY);
                    mostrarSubmenuSPI();
                }
            }
        }

        /* Volver a activar recepcion */
        HAL_UART_Receive_IT(&huart2, &datoUART, 1);
    }
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)	{
    if (htim->Instance == TIM6)	{
        if (senalCuadradaActiva != 0)	{
            contadorTimerMs++;
            if (contadorTimerMs >= tiempoEstadoMs)	{
                contadorTimerMs = 0;
                HAL_GPIO_TogglePin(LED_P5_GPIO_Port, LED_P5_Pin);
            }
        }
    }
}




/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
