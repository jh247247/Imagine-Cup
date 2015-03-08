/*
 * A quick test program to test the LCD on the dev board mentioned in
 * the readme. Also blinks an LED and does some floating point calcs for
 * fun and testing :D.
 *
 * If this helps you do something cool let me know!
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_flash.h"
#include "misc.h"
#include "lcd_control.h"
#include "rfft.h"
#include "math.h"

#include "usart.h"
#include "timer.h"
#include "esp8266.h"
#include "ImagineNode.h"
#include <string.h>

void ADC_Configuration(void)
{
  ADC_InitTypeDef  ADC_InitStructure;
  /* PCLK2 is the APB2 clock */
  /* ADCCLK = PCLK2/6 = 72/6 = 12MHz*/
  RCC_ADCCLKConfig(RCC_PCLK2_Div4);

  /* Enable ADC1 clock so that we can talk to it */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  /* Put everything back to power-on defaults */
  ADC_DeInit(ADC1);

  /* ADC1 Configuration ------------------------------------------------------*/
  /* ADC1 and ADC2 operate independently */
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  /* Disable the scan conversion so we do one at a time */
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  /* Don't do contimuous conversions - do them on demand */
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  /* Start conversin by software, not an external trigger */
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  /* Conversions are 12 bit - put them in the lower 12 bits of the result */
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  /* Say how many channels would be used by the sequencer */
  ADC_InitStructure.ADC_NbrOfChannel = 1;

  /* Now do the setup */
  ADC_Init(ADC1, &ADC_InitStructure);
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibaration register */
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));
  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));

  GPIO_InitTypeDef GPIO_InitStructure;

  /* GPIOA Periph clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

  /* Configure PC12 to mode: slow rise-time, pushpull output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOB,&GPIO_InitStructure);//GPIOA init
}


unsigned int readADC1(unsigned char channel)
{
  ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_1Cycles5);
  // Start the conversion
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  // Wait until conversion completion
  while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
  // Get the conversion value
  return ADC_GetConversionValue(ADC1);
}

int max_array(float a[], int num_elements, int *index)
{
  int i;
  float max=-32000;
  for (i=0; i<num_elements; i++)
    {
      if (a[i]>max)
        {
          max=a[i];
        }
    }
  if(index != NULL) {
    *index = i;
  }
  
  return(max);
}


void clock_init(){
  /*Configure all clocks to max for best performance.
   * If there are EMI, power, or noise problems, try slowing the clocks*/

  /* First set the flash latency to work with our clock*/
  /*000 Zero wait state, if 0  MHz < SYSCLK <= 24 MHz
    001 One wait state, if  24 MHz < SYSCLK <= 48 MHz
    010 Two wait states, if 48 MHz < SYSCLK <= 72 MHz */
  FLASH_SetLatency(FLASH_Latency_2);

  /* Start with HSI clock (internal 8mhz), divide by 2 and multiply by 9 to
   * get maximum allowed frequency: 36Mhz
   * Enable PLL, wait till it's stable, then select it as system clock*/
  RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16);
  RCC_PLLCmd(ENABLE);
  while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}
  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

  /* Set HCLK, PCLK1, and PCLK2 to SCLK (these are default */
  RCC_HCLKConfig(RCC_SYSCLK_Div1);
  RCC_PCLK1Config(RCC_HCLK_Div1);
  RCC_PCLK2Config(RCC_HCLK_Div1);

  /* Set ADC clk to 9MHz (14MHz max, 18MHz default)*/
  RCC_ADCCLKConfig(RCC_PCLK2_Div4);

  SystemCoreClock = 64000000; // set the core clock since this isn't "standard".

  /*To save power, use below functions to stop the clock to ceratin
   * peripherals
   * RCC_AHBPeriphClockCmd
   */
}

void rfft(float X[],int N);

// has to be a power of 2, otherwise you get a hang.
#define FFT_LEN 512

/* TODO: whenever we take a background fft, take the maximum value of
   the range that we want and use that as the threshold for "seeing" a
   phone. Might actually need to fudge it to take into account the
   noise of a room.*/

int main(int argc, char *argv[])
{
  int i = 0;
  int maxfft = 0;

  float fft[FFT_LEN];
  //short backgroundfft[FFT_LEN]; // need to have a background fft for
  // noise reasons

  char buf[32];

  clock_init();
  ADC_Configuration();
  TIM_init();
  USART12_Init();
  ESP8266_init();

  IN_init();

  USART_resetRXBuffer(ESP8266_USART);
  USART_resetRXBuffer(HOST_USART);

  while(1) {
    IN_handleServer();

    if(g_adcFlag == 1) {
      fft[i] = readADC1(ADC_Channel_8);
      i++;
      g_adcFlag = 0;
    }

    if(i >= FFT_LEN) {
      TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
      USART_PutChar(HOST_USART, 'a');
      /* TODO: Add flags from server for taking background noise fft */
      //fix_fftr(fft,LOG2FFT+1,0);
      rfft(fft, FFT_LEN);
      USART_PutChar(HOST_USART, 'b');
      
      max_array(fft+1,FFT_LEN/2-1, &maxfft);
      memset(buf, 0, 32);
      itoa(buf,maxfft,10);
      itoa(buf+10,(int)fft[maxfft],10);
      USART_PutChar(HOST_USART, 'c');
      ESP8266_sendPacket("UDP", "192.168.1.10", "50042",
                                       buf,32);
      USART_PutChar(HOST_USART, 'd');
      USART_PutChar(HOST_USART, '\n');
      i = 0;
      
      TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    }

    /* TODO: find out if the  */

    asm("wfe");
  }
}
