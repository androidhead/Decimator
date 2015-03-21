// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield
int in_ADC0, in_ADC1;  //variables for 2 ADCs values (ADC0, ADC1)
int POT0, POT1, POT2, out_DAC0, out_DAC1; //variables for 3 pots (ADC8, ADC9, ADC10)
int LED = 3;
int FOOTSWITCH = 7; 
int TOGGLE = 2; 
bool ToggleIsUp = false;

void setup()
{   
  setupTimers();

  //ADC Configuration
  ADC->ADC_MR |= 0x80;   // DAC in free running mode.
  ADC->ADC_CR=2;         // Starts ADC conversion.
  ADC->ADC_CHER=0x1CC0;    // Enable ADC channels 0 and 1.  

  //DAC Configuration
  analogWrite(DAC0,0);  // Enables DAC0
  analogWrite(DAC1,0);  // Enables DAC1  
  
  
  //Pin Configuration
  pinMode(LED, OUTPUT);  
  pinMode(FOOTSWITCH, INPUT);     
  pinMode(TOGGLE, INPUT);    
  
  //setup serial port (for logging)
  Serial.begin(9600);
}

void loop()
{  
  LogMinMax();  //see "min max logging" section below  
  ToggleIsUp = !digitalRead(TOGGLE);
}

void setupTimers()
{
  //I could only get 1 timer to work at a time.  I must be doing something wrong.
  //but heres the spec just in case
  /*
  ISR/IRQ	TC       	Channel	Due pins
  TC0	        TC0	        0	2, 13
  TC1	        TC0	        1	60, 61
  TC2	        TC0	        2	58
  TC3	        TC1	        0	none  <- this line in the example above
  TC4	        TC1	        1	none
  TC5	        TC1	        2	none
  TC6	        TC2	        0	4, 5
  TC7	        TC2	        1	3, 10
  TC8	        TC2	        2	11, 12
  */
  
  /* turn on the timer clock in the power management controller */
  pmc_set_writeprotect(false);

  //TC4_Handler
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
  

  //log maximum and minumum ADC values
  RegisterMinMax(in_ADC1);


  //bit depth
  if(ToggleIsUp)
  {
    in_ADC0 = ChangeBitDepth(in_ADC0, POT1);
    in_ADC1 = ChangeBitDepth(in_ADC1, POT1);
  }
  else //alt bit depth
  {
    in_ADC0 = ChangeBitDepth3(in_ADC0, POT1);
    in_ADC1 = ChangeBitDepth3(in_ADC1, POT1);
  }
  
  
  
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
  //scale back to 4095
  input = map(input, 0, scaleTo, 0, 4095);
    
  return input;
}


//BEGIN: min max logging
int maximum = 2048;
bool logMax = false;
int minimum = 2048;
bool logMin = false;
int RegisterMinMax(int in)
{
  if(in < minimum)
  {
    minimum = in;
    logMin = true;
  }
  if(in > maximum)
  {
    maximum = in;
    logMax = true;
  }
}
void LogMinMax()
{
  if(logMax)
  {
    Serial.print("new max");
    Serial.println(maximum);
  }
  logMax = false;

  if(logMin)
  {
    Serial.print("new min");
    Serial.println(minimum);
  }
  logMin = false;
}
//END: min max logging





int ChangeBitDepth2(int input, int potPosition)
{
  //invert pot value
  potPosition = map(potPosition, 0, 4095, 4095, 0); 
  return input & potPosition;
}


int ChangeBitDepth3(int input, int potPosition)
{  
  int bitmask = 2147483647; //int max is default bit mask (aka does nothing)

  //determine severity of bit mask
  potPosition = map(potPosition, 0, 4095, 0, 12);  
   
  switch (potPosition) {
    case 1:
      //0111 1111 1111 1111 1111 1111 1111 1110
      bitmask = -2147483646;         
      break;
    case 2:
      //0111 1111 1111 1111 1111 1111 1111 1100
      bitmask = -2147483644;         
      break;
    case 3:
      //0111 1111 1111 1111 1111 1111 1111 1000
      bitmask = -2147483640;         
      break;
    case 4:
      //0111 1111 1111 1111 1111 1111 1111 0000
      bitmask = -2147483632;         
      break;
    case 5:
      //0111 1111 1111 1111 1111 1111 1110 0000
      bitmask = -2147483616;         
      break;
    case 6:
      //0111 1111 1111 1111 1111 1111 1100 0000
      bitmask = -2147483584;         
      break;
    case 7:
      //0111 1111 1111 1111 1111 1111 1000 0000
      bitmask = -2147483520;         
      break;
    case 8:
      //0111 1111 1111 1111 1111 1111 0000 0000
      bitmask = -2147483392;         
      break;
    case 9:
      //0111 1111 1111 1111 1111 1110 0000 0000
      bitmask = -2147483136;         
      break;
    case 10:
      //0111 1111 1111 1111 1111 1100 0000 0000
      bitmask = -2147482624;         
      break;
    case 11:
      //0111 1111 1111 1111 1111 1000 0000 0000
      bitmask = -2147481600;         
      break;
    case 12:
      //0111 1111 1111 1111 1111 0000 0000 0000
      bitmask = -2147479552;         
      break;
  }

      
  return input & bitmask;
}

int ChangeBitDepth4(int input, int potPosition)
{    
  //todo: this could be declared as a "const" at beginning of program.  would be more efficient
  //represents the part of the mask that doesn't matter because it's beyond the 12 bits that the converter returns
  int baseBitMask = 2147479552;
     
  int bitmask = 4095; //int max is default bit mask (aka does nothing)
  
  //determine severity of bit mask
  potPosition = map(potPosition, 0, 4095, 0, 3);  
   
  switch (potPosition) {
    case 1:
      //1010 1010 1010
      bitmask = 2730;         
      break;
    case 2:
      //0101 0101 0101
      bitmask = 1365;         
      break;
    case 3:
      //1000 1000 1000
      bitmask = 2184;         
      break;
  }
      
  return input & (bitmask + baseBitMask);
}
