// MASTER BRANCH - plaato_stm8_161018

#include "STM8S.h"

#define THRESHOLD_OFFSET 	40

// Function declarations
void clock_setup(void);
void GPIO_setup(void);
void TIM1_setup(void);
void TIM2_setup(void);
void AWU_setup(void);
void UART1_setup(void);
void I2C_setup(void);
void ADC1_setup(void);
void IWDG_setup(void);
void led_handler(void);
void button_handler(void);
void print(char[]);
//void printChar(char);
//void printInt(unsigned int);
void count_bubbles(void);
void wait(uint8_t);
uint32_t LSIMeasurment(void);

// TIMING GLOBALS
uint8_t led_timer = 0;
uint8_t button_timer = 0;

// LED GLOBALS
uint8_t led_mode = 0;
volatile uint8_t new_led_mode = 7; // This should be 7
volatile uint8_t led_count = 0;
volatile bool led_toggle = FALSE;
volatile bool blink_on_bubble = FALSE;
uint8_t bubble_led = 0;
uint8_t bubble_led_count = 0;
uint8_t led_value[3] = {0, 0, 0};
bool busy_on_bubble = FALSE;
bool breathing = FALSE;
uint8_t dim_count = 0;
uint8_t slow_dim_curve[30] = {0,1,2,4,7,10,14,18,22,26,30,33,36,38,40,40,40,38,36,33,30,26,22,18,14,10,7,3,1,0};

// LED Consts
static const uint8_t period = 160;
static const uint8_t slowperiod = 40;
static const uint8_t fastperiod = 10;
static const uint8_t slowflash = 10;
static const uint8_t fastflash = 2;
static const uint8_t led_max = 40;
static const uint8_t led_max_bubble = 50;
static const uint8_t dim_max = 30;

// BUTTON GLOBALS
volatile bool reset_toggle = FALSE;
volatile bool reset_flag = FALSE;
uint8_t reset_count = 0; 

// COUNTING GLOBALS
uint32_t bubble_count = 0;
uint16_t smooth = 200;
uint16_t raw = 200;
uint8_t basecount = 0;
int16_t baseline = 200;
int16_t max_baseline = 1023;
int16_t threshold = 160;
bool under_threshold = FALSE;
uint8_t wait_count = 0;	// For IR-led wait

// DEBUG - should be commented out for production
//extern uint8_t last_event;
//extern uint8_t i2c_interrupts;


void main(void) {
	unsigned char i = 0; 
	GPIO_setup();
	clock_setup();
	TIM2_setup();
	ADC1_setup();
	I2C_setup();
	AWU_setup();
	IWDG_setup();
	//ITC_SetSoftwarePriority(ITC_IRQ_I2C, ITC_PRIORITYLEVEL_3); // Set I2C to top priority
	enableInterrupts(); 	// Enable general interrupts
	TIM2_SetCompare1(0); 	// Turn off LED 1
	TIM2_SetCompare2(0);	// Turn off LED 2
	TIM2_SetCompare3(0);	// Turn off LED 3
	GPIO_WriteHigh(GPIOC, ((GPIO_Pin_TypeDef)GPIO_PIN_5 | GPIO_PIN_6)); // Turn on IR-LED
	
	//UART1_setup(); // DEBUG
	
	while(TRUE) {
		//CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE); // START HSI
		led_timer++;
		button_timer++;
		
		// Feed the dog
		IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
		IWDG_ReloadCounter();
		IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
		
		// COUNT BUBBLES
		wait(255); // Wait about 300us for the IR-sensor LED to be fully turned on. 
		wait(255);
		wait(255);
		wait(255);
		count_bubbles();										// Takes ADC sample and runs counting algorithm
		
		// LED HANDLER
		if (led_timer == 9) {
			led_timer = 0;
			//GPIO_WriteHigh(GPIOD, GPIO_PIN_2); 	// DEBUG
			led_handler();
			//GPIO_WriteLow(GPIOD, GPIO_PIN_2); 	// DEBUG
		}
		
		// BUTTON HANDLER - Pulls state of a reed-switch and sets flag if it is activated for a few seconds.
		if (button_timer == 50) {
			button_timer = 0;
			if(GPIO_ReadInputPin(GPIOC, GPIO_PIN_7) == FALSE) {
				if (reset_toggle == FALSE) {
					reset_toggle = TRUE;
					new_led_mode = 22; // BOTONBUBBLE
				}
				reset_count++;
				if (reset_count > 7) {
					new_led_mode = 8;	// ALLON
					reset_flag = TRUE;
				}
				
			}
			if (GPIO_ReadInputPin(GPIOC, GPIO_PIN_7) > 0 && reset_toggle == TRUE) {
				reset_toggle = FALSE;
				reset_count = 0;
			}
		}
		
	
		CFG->GCR |= CFG_GCR_AL;								// Set AL bit to interrupt only so that I2C interrupt will not resume main program. 
		wfi(); 																// Enter wait-mode.
	};
}

void clock_setup(void) {
      CLK_DeInit();
                
      CLK_HSECmd(DISABLE);
      CLK_LSICmd(ENABLE);
      CLK_HSICmd(ENABLE);
      while(CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == FALSE);
                
      CLK_ClockSwitchCmd(ENABLE);
      CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
      CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
                
      CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE);
                
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, ENABLE);
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_AWU, ENABLE);
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, DISABLE);
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, DISABLE);
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
      CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);
}

 
void GPIO_setup(void) {
      GPIO_DeInit(GPIOA);
      GPIO_Init(GPIOA, GPIO_PIN_3, GPIO_MODE_OUT_PP_HIGH_FAST); // PA3 - LED 3 (top)
			
			GPIO_DeInit(GPIOB);
			GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUT_OD_HIZ_FAST);	// I2C - SCL
			GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_OD_HIZ_FAST);	// I2C - SDA
			
			GPIO_DeInit(GPIOC);
			GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT); // PC4 - ADC
			GPIO_Init(GPIOC, GPIO_PIN_7, GPIO_MODE_IN_FL_NO_IT); // Reset switch button
			GPIO_Init(GPIOC, ((GPIO_Pin_TypeDef)GPIO_PIN_5 | GPIO_PIN_6), GPIO_MODE_OUT_PP_HIGH_FAST); 	// IR-LED pin PC5 and PC6
                
      GPIO_DeInit(GPIOD);
      GPIO_Init(GPIOD, ((GPIO_Pin_TypeDef)GPIO_PIN_3 | GPIO_PIN_4), // PD3 and PD4 - LED 2 (mid) and LED 1 (bot)
                GPIO_MODE_OUT_PP_HIGH_FAST);
			//GPIO_Init(GPIOD, GPIO_PIN_5, GPIO_MODE_OUT_PP_HIGH_FAST); // D5 - UART TX DEBUG
			
			//GPIO_Init(GPIOD, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_FAST); // debug pin
}

void IWDG_setup(void) {
     IWDG_Enable();
     IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
     IWDG_SetPrescaler(IWDG_Prescaler_256);
     IWDG_SetReload(0xE);
     IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
}

void ADC1_setup(void) {
   ADC1_DeInit();         
                
   ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, 
             ADC1_CHANNEL_2,
             ADC1_PRESSEL_FCPU_D18, 
             ADC1_EXTTRIG_GPIO, 
             DISABLE, 
             ADC1_ALIGN_RIGHT, 
             ADC1_SCHMITTTRIG_CHANNEL0, 
             DISABLE);
	ADC1_Cmd(ENABLE);
}

/*
void UART1_setup(void) { // DEBUG
     UART1_DeInit();
                
     UART1_Init(115200, 
                UART1_WORDLENGTH_8D, 
                UART1_STOPBITS_1, 
                UART1_PARITY_NO, 
                UART1_SYNCMODE_CLOCK_DISABLE,
                UART1_MODE_TX_ENABLE);
                
     UART1_Cmd(ENABLE);
}
*/

void I2C_setup(void) {
    I2C_DeInit();
		/* Initialize I2C peripheral */
		I2C_Init(100000, 0x3<<1, I2C_DUTYCYCLE_2, I2C_ACK_CURR, I2C_ADDMODE_7BIT, CLK_GetClockFreq()/1000000);           

		/* Enable Error Interrupt*/
		I2C_ITConfig((I2C_IT_TypeDef)(I2C_IT_ERR | I2C_IT_EVT | I2C_IT_BUF), ENABLE);

}

void TIM1_setup(void) {               
     TIM1_DeInit();
     TIM1_TimeBaseInit(256, TIM1_COUNTERMODE_UP, 128, 1);     
     TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
     TIM1_Cmd(ENABLE);
}

void TIM2_setup(void) {
      TIM2_DeInit();
      TIM2_TimeBaseInit(TIM2_PRESCALER_32, 1000);
      TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 1000, 
                   TIM2_OCPOLARITY_HIGH);
      TIM2_OC2Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 1000, 
                   TIM2_OCPOLARITY_HIGH);
      TIM2_OC3Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, 1000, 
                   TIM2_OCPOLARITY_HIGH);
      TIM2_Cmd(ENABLE);
}

void AWU_setup(void) {
			//AWU_IdleModeEnable();
			//AWU_DeInit();
			AWU_LSICalibrationConfig(128000);
			AWU_Init(AWU_TIMEBASE_8MS);
			//AWU_Cmd(ENABLE);
			CLK_FastHaltWakeUpCmd(ENABLE); 			// Enables fast wake up from active halt using HSI
			CFG->GCR |= CFG_GCR_AL;								// Set AL bit to interrupt only
}

void count_bubbles(void) {

	// LOWPASS
	smooth += raw;
	smooth >>= 1;
	
	// UPDATE BASELINE
	if (smooth >= baseline) {
		if (smooth > max_baseline) {
			// Potentially fake baseline! Do not update. Just slowly increase
			baseline += 2;
			// Do something to potentially change max baseline
		} else {
			baseline = smooth;
			basecount = 0;
		}
		max_baseline = baseline + 5;
		
	} else { // Linear fall to ajdust to lower baseline
		basecount++;
		if (basecount >= 3) {
			basecount = 0;
			baseline -= 1;
		}
	}
	
	// UPDATE THRESHOLD
	threshold = baseline - THRESHOLD_OFFSET;
	
	// COUNT BUBBLES
	if ( (int16_t)smooth <= threshold && !under_threshold) {
		if (blink_on_bubble) {
			// Flash on TOP LED
			TIM2_SetCompare1(led_max_bubble);
			busy_on_bubble = TRUE;
			bubble_led_count = 0;
		}
		under_threshold = TRUE;
		disableInterrupts(); // Disable interrupt to increment 32bit shared variable
		bubble_count++;
		enableInterrupts();
	} else if ( (int16_t)smooth > threshold && under_threshold) {
		if (blink_on_bubble) {
			TIM2_SetCompare1(led_value[0]);
		}
		under_threshold = FALSE;
		busy_on_bubble = FALSE;
	}
	
	// MEASURE
	ADC1_StartConversion();
	while(ADC1_GetFlagStatus(ADC1_FLAG_EOC) == FALSE);
	raw = ADC1_GetConversionValue();
	ADC1_ClearFlag(ADC1_FLAG_EOC);
	GPIO_WriteLow(GPIOC, ((GPIO_Pin_TypeDef)GPIO_PIN_5 | GPIO_PIN_6)); // Turn off IR-LED
	
	// PRINTING
	//printInt(smooth); print("\n");// printInt(raw); print("\t"); printInt(threshold); print("\n");
}

void led_handler(void) {
	
	// Update led_mode
	if (led_mode != new_led_mode) {
		led_mode = new_led_mode;
		led_count = 0;
		led_toggle = FALSE;
		blink_on_bubble = FALSE;
	}
	
	switch (led_mode)
  {
		case 1: // COUNTING
			// Setup 
			if (led_toggle == FALSE) { 
				led_toggle = TRUE;
				blink_on_bubble = TRUE;
				breathing = TRUE;
			}
			
			if (!breathing) {
				break;
			}
			
			// Overflow led_count early
			if (led_count >= period) {
				led_count = 0;
			}
			
			// Update values
			if (led_count < period/12) {
				led_value[0] = led_count;
				led_value[1] = 0;
				led_value[2] = 0;
			} else if (led_count <= (period/12)*2) {
				led_value[0] = led_count;
				led_value[1] = led_count - (period/12);   
				led_value[2] = 0;
			} else if (led_count <= (period/12)*4) {
				led_value[0] = led_count;
				led_value[1] = led_count - (period/12);
				led_value[2] = led_count - (period/12)*2;
			} else  {
				led_value[0] = 0;
				led_value[1] = 0;
				led_value[2] = 0;
				breathing = FALSE; 
			}
	
			// Saturation
			if (led_value[0] > dim_max) {
				led_value[0] = dim_max;
			}
			if (led_value[1] > dim_max) {
				led_value[1] = dim_max;
			}
			if (led_value[2] > dim_max) {
				led_value[2] = dim_max;
			}
			
			// Set PWM if led is not blinking on bubble
			TIM2_SetCompare3(led_value[0]);
			TIM2_SetCompare2(led_value[1]);
			if (busy_on_bubble == FALSE) {
				TIM2_SetCompare1(led_value[2]);
			}
			
			// Special led_count incrementor to increase led strenght on breathing
			led_count++;
			
      break;
		
		case 2: // COUNTING 2
      if (led_toggle == FALSE) {
				TIM2_SetCompare3(dim_max);
				TIM2_SetCompare2(0);
				TIM2_SetCompare1(0);
				led_toggle = TRUE;
				blink_on_bubble = TRUE;
				led_value[0] = 0; 				// Set breathing value for led to 0
			}
			break;
			
		case 3: // SLOWBREATHING

			// Overflow led_count early
			if (led_count >= 30) {
				led_count = 0;
			}
			
			// Set PWM
			TIM2_SetCompare3(slow_dim_curve[led_count]);
			TIM2_SetCompare2(slow_dim_curve[led_count]);
			TIM2_SetCompare1(slow_dim_curve[led_count]);
			break;
	
		case 6: // FASTFLASH
			// Overflow led_count early
			if (led_count > fastflash) {
				led_count = 0;
				if (led_toggle == FALSE) {
					TIM2_SetCompare3(led_max);
					TIM2_SetCompare2(led_max);
					TIM2_SetCompare1(led_max);
					led_toggle = TRUE;
				} else {
					TIM2_SetCompare3(0);
					TIM2_SetCompare2(0);
					TIM2_SetCompare1(0);
					led_toggle = FALSE;
				}
			}
      break;
		
		case 7: // ALLOFF
      if (led_toggle == FALSE) {
				TIM2_SetCompare3(0);
				TIM2_SetCompare2(0);
				TIM2_SetCompare1(0);
				led_toggle = TRUE;
			}
			break;
    
		case 8: // ALLON
			if (led_toggle == FALSE) {
				TIM2_SetCompare3(led_max);
				TIM2_SetCompare2(led_max);
				TIM2_SetCompare1(led_max);
				led_toggle = TRUE;
			}
      break;
		
		case 9: // BOT
			if (led_toggle == FALSE) {
				TIM2_SetCompare3(led_max);
				TIM2_SetCompare2(0);
				TIM2_SetCompare1(0);
				led_toggle = TRUE;
			}
      break;
		
		case 10: // MID
			if (led_toggle == FALSE) {
				TIM2_SetCompare3(0);
				TIM2_SetCompare2(led_max);
				TIM2_SetCompare1(0);
				led_toggle = TRUE;
			}
      break;
		
		case 11: // TOP
			if (led_toggle == FALSE) {
				TIM2_SetCompare3(0);
				TIM2_SetCompare2(0);
				TIM2_SetCompare1(led_max);
				led_toggle = TRUE;
			}
      break;
		
		case 13: // FASTDOWN

			// Overflow led_count early
			if (led_count >= fastperiod) {
				led_count = 0;
			}
			
			// Update 
			if (led_count < fastperiod/3) {
				TIM2_SetCompare3(0);
				TIM2_SetCompare2(0);
				TIM2_SetCompare1(led_max);
			} else if (led_count < fastperiod*2/3) {
				TIM2_SetCompare3(0);
				TIM2_SetCompare2(led_max);
				TIM2_SetCompare1(0);
			} else {
				TIM2_SetCompare3(led_max);
				TIM2_SetCompare2(0);
				TIM2_SetCompare1(0);
			}
			break;
		
		case 19: // MIDFASTFLASH
			// Overflow led_count early
			if (led_count >= fastflash) {
				led_count = 0;
				if (led_toggle == FALSE) {
					TIM2_SetCompare3(0);
					TIM2_SetCompare2(led_max);
					TIM2_SetCompare1(0);
					led_toggle = TRUE;
				} else {
					TIM2_SetCompare3(0);
					TIM2_SetCompare2(0);
					TIM2_SetCompare1(0);
					led_toggle = FALSE;
				}
			}
      break;
			
		case 22: // BOTONBUBBLE
      if (led_toggle == FALSE) {
				TIM2_SetCompare3(0);
				TIM2_SetCompare2(0);
				TIM2_SetCompare1(0);
				led_toggle = TRUE;
				blink_on_bubble = TRUE;
				led_value[0] = 0; 				// Set breathing value for led to 0
			}
			break;
		
    default:
			new_led_mode = 9; // set to error mode
      break;
  }
	led_count++;
	bubble_led_count++;
	
	// Timeout used by blink_on_bubble
	if (blink_on_bubble == TRUE) {
		if (bubble_led_count >= 100) {
			bubble_led_count = 0;
			breathing = TRUE;
			led_count = 0;
		}
	}
}

void wait(uint8_t n) {
	for (wait_count = 0; wait_count < n; wait_count++) {
		nop();
	}
}

/*
////////// UART PRINT FUNCTIONS FOR DEBUGGING ////////////
void print(char msg[]) {
	// TODO: Add timestamp
	int i = 0;
	while (msg[i] != '\0') {
		while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
		// wait for serial
		}
		UART1_SendData8(msg[i]);
		i++;
	}
}

void printChar(char c) {
	char first = c / 100;
	char second = (c % 100) / 10;
	char third = c % 10;
	bool firstFlag = FALSE;
	if (first > 0) {
		while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
		// wait for serial
		}
		UART1_SendData8('0' + first);
		firstFlag = TRUE;
	}
	if (second > 0 || firstFlag) {
		while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
		// wait for serial
		}
		UART1_SendData8('0' + second);
	}
	while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
		// wait for serial
	}
	UART1_SendData8('0' + third);
}

void printInt(unsigned int c) {
	char first = c / 10000;
	char second = (c % 10000) / 1000;
	char third = (c % 1000) / 100;
	char forth = (c % 100) / 10;
	char fifth = c % 10;
	while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
	// wait for serial
	}
	UART1_SendData8('0' + first);
	while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
	// wait for serial
	}
	UART1_SendData8('0' + second);
	while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
		// wait for serial
	}
	UART1_SendData8('0' + third);
	while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
		// wait for serial
	}
	UART1_SendData8('0' + forth);
	while(UART1_GetFlagStatus(UART1_FLAG_TXE) == FALSE){
		// wait for serial
	}
	UART1_SendData8('0' + fifth);
}
*/