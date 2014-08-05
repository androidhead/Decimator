// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield

 int in_ADC0, in_ADC1;  //variables for 2 ADCs values (ADC0, ADC1)
 int POT0, POT1, POT2, out_DAC0, out_DAC1; //variables for 3 pots (ADC8, ADC9, ADC10)
 int LED = 3;
 int FOOTSWITCH = 7; 
 int TOGGLE = 2; 

void setup()
{
  /* turn on the timer clock in the power management controller */
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);

  /* we want wavesel 01 with RC */
  TC_Configure(/* clock */TC1,/* channel */1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC 
  | TC_CMR_TCCLKS_TIMER_CLOCK2);
  //set initial sample rate
  TC_SetRC(TC1, 1, 238);
  //TC_SetRC(TC1, 1, 109); // sets <>   96 Khz interrupt rate
  
  TC_Start(TC1, 1);
 
  // enable timer interrupts on the timer
  TC1->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;
 
  /* Enable the interrupt in the nested vector interrupt controller */
  /* TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number 
  (=(1*3)+1) for timer1 channel1 */
  NVIC_EnableIRQ(TC4_IRQn);
   
  //ADC Configuration
  ADC->ADC_MR |= 0x80;   // DAC in free running mode.
  ADC->ADC_CR=2;         // Starts ADC conversion.
  ADC->ADC_CHER=0x1CC0;    // Enable ADC channels 0 and 1.  

  //DAC Configuration
  analogWrite(DAC0,0);  // Enables DAC0
  analogWrite(DAC1,0);  // Enables DAC1  
}

void TC4_Handler()
{
  // We need to get the status to clear it and allow the interrupt to fire again
  TC_GetStatus(TC1, 1);
  
  //Read the ADCs
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0);  // wait for ADC 0, 1, 8, 9, 10 conversion complete.
  in_ADC0=ADC->ADC_CDR[7];             // read data from ADC0
  in_ADC1=ADC->ADC_CDR[6];             // read data from ADC1  
  POT0=ADC->ADC_CDR[10];                   // read data from ADC8        
  POT1=ADC->ADC_CDR[11];                   // read data from ADC9   
  POT2=ADC->ADC_CDR[12];                   // read data from ADC10    

  //bit depth
  in_ADC0 = ChangeBitDepth(in_ADC0, POT1);
  in_ADC1 = ChangeBitDepth(in_ADC1, POT1);

  //sample rate
  ChangeSampleRate(POT0);
  
  //Adjust the volume with POT2
  out_DAC0=map(in_ADC0,0,4095,1,POT2);
  out_DAC1=map(in_ADC1,0,4095,1,POT2);
    
  //Write the DACs
  dacc_set_channel_selection(DACC_INTERFACE, 0);          //select DAC channel 0
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC0);//write on DAC
  dacc_set_channel_selection(DACC_INTERFACE, 1);          //select DAC channel 1
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC1);//write on DAC
}


void ChangeSampleRate(int potPosition)
{
  int sampleSize=map(potPosition,0,4095,109,8400);   
  //10500/sampleSize = sampling rate in MHz
  TC_SetRC(TC1, 1, sampleSize); 
}

int ChangeBitDepth(int input, int potPosition)
{
  //determine scaling factor
  potPosition = map(potPosition, 0, 4095, 0, 7);  
  int scaleTo = 4095;
  switch (potPosition) {
    case 1:
      scaleTo = 1023;
      break;
    case 2:
      scaleTo = 511;
      break;
    case 3:
      scaleTo = 255;
      break;
    case 4:
      scaleTo = 63;
      break;
    case 5:
      scaleTo = 31;
      break;
    case 6:
      scaleTo = 15;
      break;
    case 7:
      scaleTo = 7;
      break;
  }

  //todo:  it'd be much cooler to use some bit-wise math here instead of map()   
  //scale to scaleTo values
  input =  map(input, 0, 4095, 0, scaleTo);
  //scale back to 4096
  input = map(input, 0, scaleTo, 0, 4095);
    
  return input;
}
