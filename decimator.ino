// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield

int in_ADC0;  //variables for ADC value from ADC0 
int POT0, POT1, POT2, out_DAC0; //variables for 3 pots (ADC8, ADC9, ADC10)
int LED = 3;
int FOOTSWITCH = 7; 
int TOGGLE = 2; 

//sample rate global vars
int sampleRateFactor = 1;
int oldSampleRateFactor = 1;
int sampleCounter = 0;
int oldSample;
 
void setup()
{
  //ADC Configuration
  ADC->ADC_MR |= 0x80;   // DAC in free running mode.
  ADC->ADC_CR=2;         // Starts ADC conversion.
  ADC->ADC_CHER=0x1CC0;  // Enable ADC channels 0,1,8,9,10).  

  //DAC Configuration
  analogWrite(DAC0,0);  // Enables DAC0
}



void loop()
{
  //Read the ADCs
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0);// wait for ADC 0, 1, 8, 9, 10 conversion complete.
  POT0=ADC->ADC_CDR[10];                 // read data from ADC8        
  POT1=ADC->ADC_CDR[11];                 // read data from ADC9   
  POT2=ADC->ADC_CDR[12];                 // read data from ADC10 
  in_ADC0 = ADC->ADC_CDR[7];              // read data from ADC0
  
  //sample rate
  in_ADC0 = ChangeSampleRate(in_ADC0);               
    
  //bit depth
  in_ADC0 = ChangeBitDepth(in_ADC0, POT2);
  
      
  //volume
  out_DAC0=map(in_ADC0,0,4095,1,POT1);
       
  //Write the DACs
  dacc_set_channel_selection(DACC_INTERFACE, 0);       //select DAC channel 0
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC0);//write on DAC
}


int ChangeSampleRate(int sample)
{
  //by default, return old sample
  int output = oldSample;

  // emulate lesser sample by only reading new sample a fraction of the time
  // translate POT to sample rate factor
  sampleRateFactor=map(POT0,0,4095,1,300);   
  if(sampleCounter % sampleRateFactor == 0)  
  {
    output = sample;
    oldSample = sample; //reset old sample
  }
  if(sampleCounter >= sampleRateFactor)
  {
    sampleCounter = 0;
  }
  else
  {       
    sampleCounter++;
  }
  
  return output;
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
