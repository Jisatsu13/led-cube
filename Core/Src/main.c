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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LAYER1_ON	LL_GPIO_SetOutputPin(LYR1_GPIO_Port, LYR1_Pin);
#define LAYER2_ON	LL_GPIO_SetOutputPin(LYR2_GPIO_Port, LYR2_Pin);
#define LAYER3_ON	LL_GPIO_SetOutputPin(LYR3_GPIO_Port, LYR3_Pin);
#define LAYER4_ON	LL_GPIO_SetOutputPin(LYR4_GPIO_Port, LYR4_Pin);
#define LAYER5_ON	LL_GPIO_SetOutputPin(LYR5_GPIO_Port, LYR5_Pin);

#define LAYER1_OFF	LL_GPIO_ResetOutputPin(LYR1_GPIO_Port, LYR1_Pin);
#define LAYER2_OFF	LL_GPIO_ResetOutputPin(LYR2_GPIO_Port, LYR2_Pin);
#define LAYER3_OFF	LL_GPIO_ResetOutputPin(LYR3_GPIO_Port, LYR3_Pin);
#define LAYER4_OFF	LL_GPIO_ResetOutputPin(LYR4_GPIO_Port, LYR4_Pin);
#define LAYER5_OFF	LL_GPIO_ResetOutputPin(LYR5_GPIO_Port, LYR5_Pin);

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t tim1_fl = 0;   //Прапорець переривання від таймеру 1
volatile uint8_t tim3_fl = 0;	//Прапорець переривання від таймеру 3
volatile uint8_t tim3_time = 0; //Рахівник часу від таймеру 3
volatile uint8_t adc_fl = 0;	//Прапорець переривання для адс
volatile uint8_t modeBtn = 0;	//Режим кубу
volatile uint32_t cube[5] = {0};//Масив світлодіодів
static uint8_t pos = 0;			//Позиція для режимів

typedef struct{
    uint8_t pos;
    int8_t layer;
    uint8_t active;
} Drop;							//Структура для режиму дощу

#define DROP_COUNT 8			//Максимальна кількість краплинок

Drop drops[DROP_COUNT];			//Масив структур

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t Conversion_ADC(ADC_TypeDef *ADCx){
	LL_ADC_REG_StartConversion(ADCx);
	while(!LL_ADC_IsActiveFlag_EOS(ADCx)){}
	LL_ADC_ClearFlag_EOS(ADCx);
	return LL_ADC_REG_ReadConversionData12(ADCx);
}


void BataryProcess(void){
	if(adc_fl){
		adc_fl = 0;
		uint16_t adc_data = Conversion_ADC(ADC1);
		float Vbat = (3.3f * adc_data* 2.0f) / 4095.0f;
		if(Vbat < 3.2f){
			while(1){}
		}
	}
}

static uint32_t rnd = 0xA5A5A5A5;

uint32_t Random32(void){
    uint32_t bit;

    bit = ((rnd >> 0) ^
           (rnd >> 2) ^
           (rnd >> 3) ^
           (rnd >> 5)) & 1;

    rnd = (rnd >> 1) | (bit << 31);

    rnd ^= TIM1->CNT;
    rnd ^= TIM3->CNT << 8;

    return rnd;
}

void LayerOff(void){
	LAYER1_OFF;
	LAYER2_OFF;
	LAYER3_OFF;
	LAYER4_OFF;
	LAYER5_OFF;
}

void LayerSwitsh(uint8_t l){
	switch(l){
	case 0:{ LAYER1_ON; }break;
	case 1:{ LAYER2_ON; }break;
	case 2:{ LAYER3_ON; }break;
	case 3:{ LAYER4_ON; }break;
	case 4:{ LAYER5_ON; }break;
	}
}

void Set_Led_On(uint8_t l,uint8_t idx){
	cube[l] |= (1UL << idx);
}

void Set_Led_Off(uint8_t l,uint8_t idx){
	cube[l] &= ~(1UL << idx);
}

void LedLayer(uint8_t l){
    uint32_t data = cube[l];

    SPI_Send16Bits(SPI1, (uint16_t)data);
    LL_GPIO_SetOutputPin(CS1_GPIO_Port, CS1_Pin);
    LL_GPIO_ResetOutputPin(CS1_GPIO_Port, CS1_Pin);

    SPI_Send8Bits(SPI2, (data>>16));
    LL_GPIO_SetOutputPin(CS2_GPIO_Port, CS2_Pin);
    LL_GPIO_ResetOutputPin(CS2_GPIO_Port, CS2_Pin);

    if(data & (1 << 24))
        LL_GPIO_SetOutputPin(LED25_GPIO_Port, LED25_Pin);
    else
        LL_GPIO_ResetOutputPin(LED25_GPIO_Port, LED25_Pin);
}


void LayerProcess(uint8_t *l){
	  if(tim1_fl){
		  tim1_fl = 0;
		  LayerOff();
		  LedLayer(*l);
		  LayerSwitsh(*l);
		  *l = (*l) + 1;
		  if(*l > 4){ *l = 0; }
	  }
}


void Mode_FillCube(void){
	if(tim3_fl){
		tim3_fl = 0;
		for(uint8_t i = 0; i< 5; i++){
			for(uint8_t j = 0; j< 25; j++){
				Set_Led_On(i, j);
			}
		}
    }
}

void Mode_Wall(void){
  if(tim3_fl){
	  tim3_fl = 0;
	  uint8_t cm[4][5] = 	  { {10,11,12,13,14},
	  							{0, 6, 12,18,24},
	  							{2, 7, 12,17,22},
	  							{4, 8, 12,16,20}};
	 for(uint8_t i = 0; i< 5; i++){
		 cube[i] = 0;
		 for(uint8_t j = 0; j< 5; j++){
			 Set_Led_On(i, cm[pos][j]);
		 }
	 }
	pos++;
	if(pos > 3)
	  pos = 0;
  }
}

void Mode_Сross(void){
	for(uint8_t i = 0; i < 25; i++){
		Set_Led_On(2, i);
	}
	for(uint8_t i = 0; i < 5; i++){
		Set_Led_On(i, 2);
		Set_Led_On(i, 7);
		Set_Led_On(i, 10);
		Set_Led_On(i, 11);
		Set_Led_On(i, 12);
		Set_Led_On(i, 13);
		Set_Led_On(i, 14);
		Set_Led_On(i, 17);
		Set_Led_On(i, 22);
	}
}

void Mode_Circuit(void){
    if(tim3_fl){
        tim3_fl = 0;
    	Set_Led_On(0, 12);
    	Set_Led_On(1, 11);
    	Set_Led_On(1, 13);
    	Set_Led_On(2, 10);
    	Set_Led_On(2, 14);
    	Set_Led_On(3, 11);
    	Set_Led_On(3, 13);
    	Set_Led_On(4, 12);

    	Set_Led_On(1, 17);
    	Set_Led_On(1, 7);
    	Set_Led_On(2, 22);
    	Set_Led_On(2, 2);
    	Set_Led_On(3, 7);
    	Set_Led_On(3, 17);

    	Set_Led_On(2, 16);
    	Set_Led_On(2, 18);
    	Set_Led_On(2, 6);
    	Set_Led_On(2, 8);
    }
}

void Mode_RandCol(void){
    if(tim3_fl){
        tim3_fl = 0;
        tim3_time++;
    }

    if(tim3_time > 2){
        tim3_time = 0;

        uint8_t columns[25] = {0};
        uint8_t count = (Random32() % 8) + 1;

        for(uint8_t i = 0; i < count; i++){
            uint8_t led = Random32() % 25;
            uint8_t height = (Random32() % 5) + 1;
            columns[led] = height;
        }

        for(uint8_t layer = 0; layer < 5; layer++)
            cube[layer] = 0;

        for(uint8_t led = 0; led < 25; led++){
            for(uint8_t layer = 0; layer < columns[led]; layer++){
                Set_Led_On(layer, led);
            }
        }
    }
}

void Mode_Rotate(void){
	  if(tim3_fl){
		  tim3_fl = 0;
		  uint8_t cm[4][4] = { {0, 4,  24, 20},
		  					   {1, 9,  23, 15},
		  					   {2, 14, 22, 10},
		  					   {3, 19, 21, 5 }};
		 for(uint8_t i = 0; i< 5; i++){
			 cube[i] = 0;
			 for(uint8_t j = 0; j< 4; j++){
				 Set_Led_On(i, cm[pos][j]);
			 }
		 }
		pos++;
		if(pos > 3)
		  pos = 0;
	  }
}

void Drop_Create(Drop *d){
    d->pos = Random32() % 25;
    d->layer = 4;
    d->active = 1;
}

void Mode_Rain(void){
    if(tim3_fl){
        tim3_fl = 0;
        for(uint8_t i = 0; i < 5; i++)
            cube[i] = 0;

        for(uint8_t i = 0; i < DROP_COUNT; i++){
            if(!drops[i].active){
                Drop_Create(&drops[i]);
            }
            Set_Led_On(drops[i].layer, drops[i].pos);
            if(drops[i].layer < 4){
                Set_Led_On(drops[i].layer + 1, drops[i].pos);
            }

            drops[i].layer--;
            if(drops[i].layer < 0){
                drops[i].active = 0;
            }
        }
    }
}


void ModeProcess(uint8_t *m){
	if(modeBtn){
		modeBtn = 0;
		*m = *m + 1;
		if(*m > 6){ *m = 0; }
    	for(uint8_t i = 0; i < 5; i++){
    		cube[i] = 0;
    	}
        for(uint8_t i = 0; i < DROP_COUNT; i++){
            drops[i].active = 0;
        }
    	pos = 0;
	}

	switch(*m){
	case 0:{
		Mode_FillCube();
	}break;
	case 1:{
		Mode_Сross();
	}break;
	case 2:{
		Mode_Circuit();
	}break;
	case 3:{
		Mode_Wall();
	}break;
	case 4:{
		Mode_RandCol();
	}break;
	case 5:{
		Mode_Rotate();
	}break;
	case 6:{
		Mode_Rain();
	}break;
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
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, 3);

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  LL_SPI_Enable(SPI1);
  LL_SPI_Enable(SPI2);

  LL_TIM_EnableCounter(TIM1);
  LL_TIM_EnableIT_UPDATE(TIM1);

  LL_TIM_EnableCounter(TIM3);
  LL_TIM_EnableIT_UPDATE(TIM3);

  LL_ADC_StartCalibration(ADC1);
  while(LL_ADC_IsCalibrationOnGoing(ADC1) != 0) {}
  LL_ADC_Enable(ADC1);

  uint8_t layerSet = 0;
  uint8_t mode = 0;
  modeBtn = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  BataryProcess();
	  LayerProcess(&layerSet);
	  ModeProcess(&mode);

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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2)
  {
  }

  /* HSI configuration and activation */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_Enable();
  LL_RCC_PLL_EnableDomain_SYS();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  }

  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Sysclk activation on the main PLL */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }

  /* Set APB1 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

  LL_Init1msTick(64000000);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(64000000);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  LL_ADC_InitTypeDef ADC_InitStruct = {0};
  LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**ADC1 GPIO Configuration
  PA0   ------> ADC1_IN0
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */

   #define ADC_CHANNEL_CONF_RDY_TIMEOUT_MS ( 1U)
   #if (USE_TIMEOUT == 1)
   uint32_t Timeout ; /* Variable used for Timeout management */
   #endif /* USE_TIMEOUT */

  ADC_InitStruct.Clock = LL_ADC_CLOCK_SYNC_PCLK_DIV2;
  ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
  ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
  LL_ADC_Init(ADC1, &ADC_InitStruct);
  LL_ADC_REG_SetSequencerConfigurable(ADC1, LL_ADC_REG_SEQ_CONFIGURABLE);

   /* Poll for ADC channel configuration ready */
   #if (USE_TIMEOUT == 1)
   Timeout = ADC_CHANNEL_CONF_RDY_TIMEOUT_MS;
   #endif /* USE_TIMEOUT */
   while (LL_ADC_IsActiveFlag_CCRDY(ADC1) == 0)
     {
   #if (USE_TIMEOUT == 1)
   /* Check Systick counter flag to decrement the time-out value */
   if (LL_SYSTICK_IsActiveCounterFlag())
     {
   if(Timeout-- == 0)
         {
   Error_Handler();
         }
     }
   #endif /* USE_TIMEOUT */
     }
   /* Clear flag ADC channel configuration ready */
   LL_ADC_ClearFlag_CCRDY(ADC1);
  ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
  ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
  ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
  ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE;
  ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
  LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
  LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
  LL_ADC_SetTriggerFrequencyMode(ADC1, LL_ADC_CLOCK_FREQ_MODE_HIGH);
  LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_COMMON_1, LL_ADC_SAMPLINGTIME_1CYCLE_5);
  LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_COMMON_2, LL_ADC_SAMPLINGTIME_1CYCLE_5);
  LL_ADC_DisableIT_EOC(ADC1);
  LL_ADC_DisableIT_EOS(ADC1);

   /* Enable ADC internal voltage regulator */
   LL_ADC_EnableInternalRegulator(ADC1);
   /* Delay for ADC internal voltage regulator stabilization. */
   /* Compute number of CPU cycles to wait for, from delay in us. */
   /* Note: Variable divided by 2 to compensate partially */
   /* CPU processing cycles (depends on compilation optimization). */
   /* Note: If system core clock frequency is below 200kHz, wait time */
   /* is only a few CPU processing cycles. */
   __IO uint32_t wait_loop_index;
   wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
   while(wait_loop_index != 0)
     {
   wait_loop_index--;
     }

  /** Configure Regular Channel
  */
  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_0);
  LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_0, LL_ADC_SAMPLINGTIME_COMMON_1);
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**SPI1 GPIO Configuration
  PA1   ------> SPI1_SCK
  PA2   ------> SPI1_MOSI
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_16BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV16;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 7;
  LL_SPI_Init(SPI1, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
  LL_SPI_EnableNSSPulseMgt(SPI1);
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**SPI2 GPIO Configuration
  PB8   ------> SPI2_SCK
  PA4   ------> SPI2_MOSI
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV16;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 7;
  LL_SPI_Init(SPI2, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI2, LL_SPI_PROTOCOL_MOTOROLA);
  LL_SPI_EnableNSSPulseMgt(SPI2);
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

  /* TIM1 interrupt Init */
  NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);
  NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  TIM_InitStruct.Prescaler = 1;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 36999;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  TIM_InitStruct.RepetitionCounter = 0;
  LL_TIM_Init(TIM1, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM1);
  LL_TIM_SetClockSource(TIM1, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_UPDATE);
  LL_TIM_SetTriggerOutput2(TIM1, LL_TIM_TRGO2_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM1);
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

  /* TIM3 interrupt Init */
  NVIC_SetPriority(TIM3_IRQn, 0);
  NVIC_EnableIRQ(TIM3_IRQn);

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  TIM_InitStruct.Prescaler = 319;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 59999;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM3, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM3);
  LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_UPDATE);
  LL_TIM_DisableMasterSlaveMode(TIM3);
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

  /**/
  LL_GPIO_ResetOutputPin(CS2_GPIO_Port, CS2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LED25_GPIO_Port, LED25_Pin);

  /**/
  LL_GPIO_ResetOutputPin(CS1_GPIO_Port, CS1_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LYR1_GPIO_Port, LYR1_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LYR2_GPIO_Port, LYR2_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LYR3_GPIO_Port, LYR3_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LYR4_GPIO_Port, LYR4_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LYR5_GPIO_Port, LYR5_Pin);

  /**/
  GPIO_InitStruct.Pin = CS2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(CS2_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LED25_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LED25_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = CS1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(CS1_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LYR1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LYR1_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LYR2_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LYR2_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LYR3_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LYR3_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LYR4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LYR4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LYR5_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LYR5_GPIO_Port, &GPIO_InitStruct);

  /**/
  LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE6);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_6;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  LL_GPIO_SetPinPull(BTNMODE_GPIO_Port, BTNMODE_Pin, LL_GPIO_PULL_NO);

  /**/
  LL_GPIO_SetPinMode(BTNMODE_GPIO_Port, BTNMODE_Pin, LL_GPIO_MODE_INPUT);

  /* EXTI interrupt init*/
  NVIC_SetPriority(EXTI4_15_IRQn, 0);
  NVIC_EnableIRQ(EXTI4_15_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void TIM1_IT(void){
	if (LL_TIM_IsActiveFlag_UPDATE(TIM1)) {
		LL_TIM_ClearFlag_UPDATE(TIM1);
		tim1_fl = 1;
	}
}

void TIM3_IT(void){
	if (LL_TIM_IsActiveFlag_UPDATE(TIM3)) {
		LL_TIM_ClearFlag_UPDATE(TIM3);
		tim3_fl = 1;
		adc_fl = 1;
	}
}

void MODE_IT(void){
	modeBtn = 1;
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
