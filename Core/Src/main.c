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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "protocol_icd.h"
#include <string.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */
ProtocolMessage_t trameAenvoyer;
ProtocolMessage_t trameRecu;

typedef enum {
    ROLE_NONCONFIG = 0,
    ROLE_MAITRE,
    ROLE_ESCLAVE
} Roles;

Roles roleActuel = ROLE_NONCONFIG;
uint32_t cleSession = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//calculer le crc pour verifier l'integrite de la trame
uint16_t calculerCRC(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

//fonction qui va masquer & demasquer le message
void appliquerMasque(uint8_t *data, size_t length, uint32_t key) {
    uint8_t cleDecoupe[4];
    memcpy(cleDecoupe, &key, 4);
    for (size_t i = 0; i < length; i++) {
        data[i] ^= cleDecoupe[i % 4];
    }
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

    //CODE QUI SERT PLUS A RIEN : ça me servait à faire des tests au début pour voir comment fonctionnait l'envoie de trames

    //trame de test
    trameAenvoyer.header.start_byte = ICD_START_BYTE;
    trameAenvoyer.header.command_type = CMD_INIT;
    trameAenvoyer.header.length = 4;
    trameAenvoyer.header.reserve = 0;
    trameAenvoyer.header.id_src = 0x12345678;

    //message de test
    trameAenvoyer.payload[0] = 'T';
    trameAenvoyer.payload[1] = 'E';
    trameAenvoyer.payload[2] = 'S';
    trameAenvoyer.payload[3] = 'T';

    //pour le test on met pas de crc
    trameAenvoyer.crc = 0x0000;
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BTN_Pin */
  GPIO_InitStruct.Pin = BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  size_t tailleDonee = sizeof(MessageHeader_t) + MAX_PAYLOAD_SIZE;
  uint32_t tmpDernierClignottement = 0;
  static uint8_t message[MAX_PAYLOAD_SIZE];
  static GPIO_PinState ancienEtatBouton = GPIO_PIN_SET;
  static uint8_t enErreur = 0;
  static uint32_t erreurDebut = 0;
  static uint8_t indexCaractereMsg = 0;
  static uint32_t tmpDernierClic = 0;
  static uint8_t nbClics = 0;

  /* Infinite loop */
  for(;;)
  {
    //raz des flags si l'uart a planté à cause d'une trame parasite
    if(huart1.ErrorCode != HAL_UART_ERROR_NONE)
    {
      __HAL_UART_CLEAR_OREFLAG(&huart1);
      __HAL_UART_CLEAR_NEFLAG(&huart1);
      __HAL_UART_CLEAR_FEFLAG(&huart1);
      huart1.ErrorCode = HAL_UART_ERROR_NONE;
    }

    //==============================GESTION BOUTON BLEU===========================
    GPIO_PinState etatActuelBouton = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
    if(nbClics > 0 && (osKernelGetTickCount() - tmpDernierClic) > 2000) 
    {
      nbClics = 0;
    }
    if(etatActuelBouton == GPIO_PIN_RESET && ancienEtatBouton == GPIO_PIN_SET)
    {
      if((osKernelGetTickCount()-tmpDernierClic)> 50) 
      {
        nbClics++;
        tmpDernierClic = osKernelGetTickCount();


        if(nbClics>=3)  //condition valide pour reset
        {
          nbClics = 0; //raz

          //envoi trame CMD_RESET
          trameAenvoyer.header.start_byte = ICD_START_BYTE;
          trameAenvoyer.header.command_type = CMD_RESET;
          trameAenvoyer.header.length = 0;
          trameAenvoyer.header.reserve = 0;
          trameAenvoyer.header.id_src = 0x00000001;
          memset(trameAenvoyer.payload, 0, MAX_PAYLOAD_SIZE);
          trameAenvoyer.crc = calculerCRC((uint8_t*)&trameAenvoyer, tailleDonee);

          HAL_UART_Transmit(&huart1, (uint8_t*)&trameAenvoyer, sizeof(ProtocolMessage_t), 100);

          //reset variables
          roleActuel = ROLE_NONCONFIG;
          cleSession = 0;
          indexCaractereMsg = 0;
          enErreur = 0;
          char logReset[] = "\r\n ================================== RESET ================================== \r\n";
          HAL_UART_Transmit(&huart2, (uint8_t*)logReset, strlen(logReset), 100);
        }
      }
    }
    ancienEtatBouton=etatActuelBouton;

    //==============================LECTURE CLAVIER============================
    
    if(roleActuel==ROLE_NONCONFIG || roleActuel==ROLE_MAITRE) //les cartes esclave ignorent le clavier
    {
      uint8_t c = 0;
      if(HAL_UART_Receive(&huart2,&c,1,1) == HAL_OK)
      {
        HAL_UART_Transmit(&huart2, &c, 1, 10);
        if (c == '\r' || c == '\n') //bouton entrée
        {
          if(indexCaractereMsg>0)
          {
            char retour[] = "\r\n";
            HAL_UART_Transmit(&huart2, (uint8_t*)retour, 2, 10);

            //si c'est le premier message : on s'initialise en maitre avant d'envoyer le message
            if (roleActuel==ROLE_NONCONFIG)
            {
                roleActuel=ROLE_MAITRE;
                cleSession=0xA5A55A5A; 

                //log
                char logMaster[100];
                snprintf(logMaster,sizeof(logMaster),"Rôle : maitre, cle: 0x%08X\r\n",(unsigned int)cleSession);
                HAL_UART_Transmit(&huart2, (uint8_t*)logMaster,strlen(logMaster),100);

                trameAenvoyer.header.start_byte = ICD_START_BYTE;
                trameAenvoyer.header.command_type = CMD_INIT;
                trameAenvoyer.header.length = 4;
                trameAenvoyer.header.reserve = 0;
                trameAenvoyer.header.id_src = 0x00000001;
                memcpy(trameAenvoyer.payload, &cleSession, sizeof(cleSession));
                trameAenvoyer.crc = calculerCRC((uint8_t*)&trameAenvoyer, tailleDonee);

                // CORRECTION : Envoi unique
                HAL_UART_Transmit(&huart1, (uint8_t*)&trameAenvoyer, sizeof(ProtocolMessage_t), 100);
                
                //pause de 20ms avant d'envoyer le msg à l'esclave
                osDelay(20);
            }

            //envoie msg
            trameAenvoyer.header.start_byte = ICD_START_BYTE;
            trameAenvoyer.header.command_type = CMD_MSG;
            trameAenvoyer.header.length = indexCaractereMsg;
            trameAenvoyer.header.reserve = 0;
            trameAenvoyer.header.id_src = 0x00000001;

            memset(trameAenvoyer.payload, 0, MAX_PAYLOAD_SIZE);
            memcpy(trameAenvoyer.payload, message, indexCaractereMsg);

            appliquerMasque(trameAenvoyer.payload, MAX_PAYLOAD_SIZE, cleSession);
            trameAenvoyer.crc = calculerCRC((uint8_t*)&trameAenvoyer, tailleDonee);

            HAL_UART_Transmit(&huart1, (uint8_t*)&trameAenvoyer, sizeof(ProtocolMessage_t), 100);

            char logEnvoie[80];
            snprintf(logEnvoie, sizeof(logEnvoie),"Maitre : message envoyé (%d octets)\r\n> ",indexCaractereMsg);
            HAL_UART_Transmit(&huart2,(uint8_t*)logEnvoie, strlen(logEnvoie), 100);

            indexCaractereMsg = 0; //vider le buffer pour prochain message
          }
        }

        //filtrer touches qui sont pas du texte
        else if (c>=32 && c<=126)
        {
          if(indexCaractereMsg<MAX_PAYLOAD_SIZE)
          {
            message[indexCaractereMsg++] = c;
          }
        }
      }
    }


    //==============================COMMUNICATION ENTRE CARTES============================
    uint8_t octetRechercheSynchro = 0;
    
    if(HAL_UART_Receive(&huart1, &octetRechercheSynchro, 1, 10) == HAL_OK)
    {
      if(octetRechercheSynchro == ICD_START_BYTE)
      {
        trameRecu.header.start_byte = octetRechercheSynchro;
        
        if (HAL_UART_Receive(&huart1, ((uint8_t*)&trameRecu) + 1, sizeof(ProtocolMessage_t) - 1, 20) == HAL_OK)
        {

          //verif intégrité
          uint16_t crcObtenu = calculerCRC((uint8_t*)&trameRecu, tailleDonee);

          if (crcObtenu == trameRecu.crc)
          {

            //Config initiale (premier message reçu)
            if(trameRecu.header.command_type == CMD_INIT && roleActuel == ROLE_NONCONFIG)
            {
              roleActuel = ROLE_ESCLAVE;
              memcpy(&cleSession, trameRecu.payload, sizeof(cleSession));

              char logEsclave[100];
              snprintf(logEsclave, sizeof(logEsclave), "\r\nRole : esclave, cle: 0x%08X\r\nJ'attend un message...\r\n", (unsigned int)cleSession);
              HAL_UART_Transmit(&huart2, (uint8_t*)logEsclave,strlen(logEsclave),100);

              HAL_UART_Transmit(&huart1, (uint8_t*)&trameRecu, sizeof(ProtocolMessage_t), 100);
            }

            //Reception message chiffré
            else if(trameRecu.header.command_type == CMD_MSG && roleActuel == ROLE_ESCLAVE)
            {
              HAL_UART_Transmit(&huart1, (uint8_t*)&trameRecu, sizeof(ProtocolMessage_t), 100);

              appliquerMasque(trameRecu.payload, MAX_PAYLOAD_SIZE, cleSession);

              char logDechiffre[100];
              snprintf(logDechiffre, sizeof(logDechiffre), "Message dechiffré : %.*s\r\n", trameRecu.header.length, trameRecu.payload);
              HAL_UART_Transmit(&huart2, (uint8_t*)logDechiffre, strlen(logDechiffre), 100);
            }

            //Ordre de reset
            else if(trameRecu.header.command_type == CMD_RESET)
            {
              HAL_UART_Transmit(&huart1, (uint8_t*)&trameRecu, sizeof(ProtocolMessage_t), 100);

              roleActuel=ROLE_NONCONFIG;
              cleSession=0;
              indexCaractereMsg=0;

              char log_r[]="\r\nRESET : chaine reset par une autre carte\r\n";
              HAL_UART_Transmit(&huart2, (uint8_t*)log_r, strlen(log_r), 100);
            }
          }
          else 
          {

            //calcul crc echec : erreur d'intégrité
            enErreur=1;
            erreurDebut=osKernelGetTickCount();
            
            char logErreur[]="\r\nERREUR, trame rejetee\r\n";
            HAL_UART_Transmit(&huart2,(uint8_t*)logErreur,strlen(logErreur),100);
          }
        }
      }
    }

    //================================GESTION LED===============================

    //Erreur : clignottement rapide
    if(enErreur)
    {
      if((osKernelGetTickCount() - tmpDernierClignottement) >= 100)
      {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        tmpDernierClignottement = osKernelGetTickCount();
      }
      
      if((osKernelGetTickCount() - erreurDebut) > 3000)
      {
        enErreur = 0;
        if (roleActuel != ROLE_NONCONFIG) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); 
      }
    }

    //En attente : clignottement
    else if (roleActuel == ROLE_NONCONFIG)
    {
      if((osKernelGetTickCount() - tmpDernierClignottement) >= 1000)
      {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        tmpDernierClignottement = osKernelGetTickCount();
      }
    }

    //Role configuré : allumée
    else
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    }

    osDelay(1); 
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
