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



void drawLine(int x, int y1,int y2) {
  int i;
  if(y1>240)y1=240;
  for(i = y1; i < y2; i++) {
    LCD_SetPoint(x,i,0xF800);
  }
}

void clock_init(){
  /*Configure all clocks to max for best performance.
   * If there are EMI, power, or noise problems, try slowing the clocks*/

  /* First set the flash latency to work with our clock*/
  /*000 Zero wait state, if 0  MHz < SYSCLK <= 24 MHz
    001 One wait state, if  24 MHz < SYSCLK <= 48 MHz
    010 Two wait states, if 48 MHz < SYSCLK <= 72 MHz */
  FLASH_SetLatency(FLASH_Latency_1);

  /* Start with HSI clock (internal 8mhz), divide by 2 and multiply by 9 to
   * get maximum allowed frequency: 36Mhz
   * Enable PLL, wait till it's stable, then select it as system clock*/
  RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_9);
  RCC_PLLCmd(ENABLE);
  while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}
  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

  /* Set HCLK, PCLK1, and PCLK2 to SCLK (these are default */
  RCC_HCLKConfig(RCC_SYSCLK_Div1);
  RCC_PCLK1Config(RCC_HCLK_Div1);
  RCC_PCLK2Config(RCC_HCLK_Div1);

  /* Set ADC clk to 9MHz (14MHz max, 18MHz default)*/
  RCC_ADCCLKConfig(RCC_PCLK2_Div4);

  /*To save power, use below functions to stop the clock to ceratin
   * peripherals
   * RCC_AHBPeriphClockCmd
   */
}

// has to be a power of 2, otherwise you get a hang.
#define FFT_LEN 512

#define AVG 5
int main(int argc, char *argv[])
{
  int i = 0;
  int cnt = 0;
  int max, maxi;

  float fft[FFT_LEN];
  float avgfft[FFT_LEN];
  char buf[32];

  clock_init();
  LCD_Configuration();
  LCD_Initialization();
  ADC_Configuration();
  TIM_init();
  USART12_Init();
  ESP8266_init();

  ESP8266_connect("Home&Hosed","143c91ffbf323f9b07439610a4");
  if(ESP8266_isConnected()) {
    USART2_PutString("ESP8266 Connected!\n");
  }

  while(1) {

    if(g_adcFlag == 1) {
      fft[i++] = readADC1(ADC_Channel_8);
      g_adcFlag = 0;
    }

    // FFT is full, render the screen.
    if(i == FFT_LEN) {
      TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);


      for(i=0;i< FFT_LEN; i++) {
        fft[i]/=256;
      }

      rfft(fft,FFT_LEN);
      for(i = 0; i < FFT_LEN/4; i++) {
        avgfft[i] += fft[i]/AVG;
      }
      cnt++;
      i = 0;
      TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    }

    if(cnt == AVG) {

      TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
      LCD_Clear(0);
      max = 0;
      maxi = 0;
      for(i = 0; i < FFT_LEN/2; i++) {
	if(fft[i] > max) {
	  max = fft[i]; //float to int, dont care.
	  maxi = i;
	}
        drawLine(i,0,fft[i]);
      }
      TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
      itoa(buf,maxi*(44000/FFT_LEN),10);
      ESP8266_sendPacket("UDP","192.168.1.10","40",buf, strlen(buf));

      i = 0;
      cnt = 0;
    }
  }
}
