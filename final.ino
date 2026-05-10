//CPE 301 Final Project
//Tyler Carrolls

#include <RTClib.h>

RTC_DS1307 rtc;


#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

unsigned char* ddr_b = (unsigned char*) 0x24;
unsigned char* port_b = (unsigned char*) 0x25;
unsigned char* ddr_h = (unsigned char*) 0x101;
unsigned char* port_h = (unsigned char*) 0x102;
volatile unsigned char* pin_g = (unsigned char*) 0x32;
volatile unsigned char* ddr_g = (unsigned char*) 0x33;
volatile unsigned char* port_g = (unsigned char*) 0x34;

volatile unsigned char* pin_a = (unsigned char*) 0x20;
volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* port_a = (unsigned char*) 0x22;

volatile unsigned char* pin_e = (unsigned char*) 0x2C;
volatile unsigned char* ddr_e = (unsigned char*) 0x2D;
volatile unsigned char* port_e = (unsigned char*) 0x2E;

volatile bool latch = false;

int freq[7] = {440, 494, 523, 587, 659, 698, 784};
int final_freq = 0;
unsigned int ticks = 1000;

volatile bool start = false;

unsigned int test = 0;
volatile bool testBool = false;

int answer_val;

unsigned long previousMillis = 0;
const long interval = 60000;

void setup() 
{
  rtc.begin();
  if(rtc.isrunning())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  randomSeed(0);
  // setup the UART
  U0init(9600);
  // setup the ADC
  adc_init();

//pin 13
*ddr_b |= 0x01 << 7;
//pin 12
*ddr_b |= 0x01 << 6;
//pin 11
*ddr_b |= 0x01 << 5;
  //pinMode(10, OUTPUT);
  *ddr_b |= 0x01 << 4;
  //pinMode(7, OUTPUT);
  *ddr_h |= 0x01 << 4;
  //pinMode(7, OUTPUT);
  //pinMode(8, OUTPUT);
  *ddr_h |= 0x01 << 5;
  //pinMode(9, OUTPUT);
  *ddr_h |= 0x01 << 6;
// pin 6
  *ddr_h |= 0x01 << 3;

  *ddr_a |= 0x01; //pin 22
  *ddr_a |= 0x02; //pin 23
  *ddr_a |= 0x04; //pin 24

*ddr_e &= ~(0x10);
  *port_e &= ~(0x10); // Pin 2, Start Button
  *ddr_e &= ~(0x20);
  *port_e &= ~(0x20); // Pin 3, Submit Button

  *ddr_g &= ~(0x20);
  *port_g &= ~(0x20); // Pin 4, Latch Button
  //*ddr_e &= ~(0x20);

  attachInterrupt(digitalPinToInterrupt(2),startInterrupt, RISING);
}

void loop() 
{
  unsigned int retval = adc_read('0');
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval)
  {
    display_log(retval);
    previousMillis = currentMillis;
  }
  if(start)
  {
    if(!testBool || answer_val == 1)
    {
      *port_a &= ~(0x04);
      *port_g &= ~(0x20);
      latch = false;
      getTest();
      testBool = true;
      answer_val = 0;
      
    }

    *port_a &= ~(0x02);
    
    light_logic(retval);

    if(!(*pin_e & 0x20) && latch)
    {
      while(!(*pin_e & 0x20))
      {answer_val = submit();}
      
    }
    else if(!(*pin_e & 0x20) && !latch)
    {
      ERR();
    }
  }
  else
  {
    *port_a |= 0x02;
    
  }
}

void getTest()
{
  test = random(0, 7);
  ticks = find_ticks(freq[test]);
  for(int i = 0; i < 1000; i++)
  {
    *port_h |= (0x01 << 3);
    my_delay(freq[test]);
    *port_h &= ~(0x01 << 3);
    my_delay(freq[test]);
  }
  
}

void display_log(unsigned int retval)
{
  U0putstring("Sensor Level: ", 13);
  char numbuffer[4];
    itoa(retval, numbuffer, 10);
    if(retval < 210)
    {
      U0putchar(numbuffer[0]);
    }
    if(retval > 10 && retval < 210)
    {
      U0putchar(numbuffer[1]);
    }
    if(retval > 100 && retval < 210)
    {
      U0putchar(numbuffer[2]); 
    }
    
    if(retval > 210)
    {
      U0putchar('h');
      U0putchar('i');
      U0putchar('g');
      U0putchar('h');
    }

    U0putstring(" at ", 4);
    DateTime now = rtc.now();
    int length = strlen(now.timestamp().c_str());
    U0putstring((unsigned char*)now.timestamp().c_str(), length);

    U0putchar('\n');
}

void startInterrupt()
{
  start = true;
  //U0putchar('s');
  
}

unsigned int find_ticks(unsigned int x)
{
  if(x == 0)
  {
    return 0;
  }
  float duty = (1.0/x) / 2.0;
  float clock_period = 1.0/16000000;
  return duty / clock_period;
}
//replace '??' With your value
void my_delay(unsigned int freq)
{
  double period;
  // calc period
  if(freq == 0)
  {
    period = 0;
  }
  else
  {
    period = 1.0/double(freq);
  }
   // 50% duty cycle
  double half_period = period/2;
  // clo*port_a |= 0x04;ck period def
  double clk_period = 0.0000000625;
  // calc ticks
  ticks = half_period / clk_period;
  // stop the timer
  //*myTCCR1B &= 0xF8;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks); //check slide

  * myTCCR1A = 0;
  // start the timer
  * myTCCR1B = 0x01;
  // wait for overflow
  
  while((*myTIFR1 & 0x01)==0){}; 

  // stop the timer
  myTCCR1B = 0x00;   
  // reset TOV           
  *myTIFR1 = 0x01;
}

int submit()
{
  int val = 0;
  ticks = find_ticks(freq[final_freq]);
  *port_h |= (0x01 << 3);
  my_delay(freq[final_freq]);
  *port_h &= ~(0x01 << 3);
  my_delay(freq[final_freq]);

  if(test == final_freq)
  {
    U0putchar('!');
    *port_a |= 0x04;
    val = 1;
  }
  return val;
}

void light_logic(unsigned int retval)
{
  if(!latch)
  {
    *port_a &= ~(0x01);
    if(retval >= 30)
    {
      *port_h |= (0x01 << 4);
      //digitalWrite(7, HIGH);
      final_freq = 1;
    }
    else
    {
      *port_h &= ~(0x01 << 4); 
      final_freq = 0;
    }
    if(retval >= 60)
    {
      *port_h |= (0x01 << 5);
      final_freq = 2;
    }
    else
    {
      *port_h &= ~(0x01 << 5); 
    }
    if(retval >= 90)
    {
      *port_h |= (0x01 << 6);
      final_freq = 3;
    }
    else
    {
      *port_h &= ~(0x01 << 6); 
    }
    if(retval >= 120)
    {
      *port_b |= (0x01 << 4);
      final_freq = 4;
    }
    else
    {
      *port_b &= ~(0x01 << 4); 
    }
    if(retval >= 150)
    {
      *port_b |= (0x01 << 5);
      final_freq = 5;
    }
    else
    {
      *port_b &= ~(0x01 << 5); 
    }
    if(retval >= 180)
    {
      *port_b |= (0x01 << 6);
      final_freq = 6;
    }
    else
    {
      *port_b &= ~(0x01 << 6); 
    }
    if(retval >= 210)
    {
      *port_b |= (0x01 << 7);
      latch = true;
      ERR();
    }
    else
    {
      *port_b &= ~(0x01 << 7); 
    }
    if(!(*pin_g & 0x20))
    {
      latch = true;
      while(!(*pin_g & 0x20));
      delay(10);
    }
  }
  else
  {
    *port_a |= 0x01;
    if(!(*pin_g & 0x20))
    {
      latch = false;
      while(!(*pin_g & 0x20));
      delay(10);
    }
  }
  
}

void ERR()
{
  *port_b |= (0x01 << 7);
  while(*pin_g & 0x20);
  latch = false;
  *port_b &= ~(0x01 << 7);
}

void adc_init() //write your code after each commented line and follow the instruction 
{
  // setup the A register
  // set bit 7 to 1 to enable the ADC 
  *my_ADCSRA |= 0b10000000;

  // clear bit 5 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11011111;

  // clear bit 3 to 0 to disable the ADC interrupt 
  *my_ADCSRA &= 0b11110111;

  // clear bit 0-2 to 0 to set prescaler selection to slow reading
  *my_ADCSRA &= 0b11111000;

  // setup the B register
  // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11110111;

  // clear bit 2-0 to 0 to set free running mode
  *my_ADCSRB &= 0b11111000;

  // setup the MUX Register
  // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX &= 0b01111111;

  // set bit 6 to 1 for AVCC analog reference
  *my_ADMUX |= 0b01000000;

  // clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11011111;

  // clear bit 4-0 to 0 to reset the channel and gain bits
  *my_ADMUX &= 0b11100000;

}

unsigned int adc_read(unsigned char adc_channel_num) //work with channel 0
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;

  // clear the channel selection bits (MUX 5) hint: it's not in the ADMUX register
  *my_ADCSRB &= 0b00000000;
 
  // set the channel selection bits for channel 0
  *my_ADCSRB |= 0b00000001;

  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0b01000000;

  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register and format the data based on right justification (check the lecture slide)
  
  unsigned int val = 0;

  val = (*my_ADC_DATA & 0x03FF);

  return val;
}

void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}

void U0putstring(unsigned char* str, unsigned int len)
{
  for(int i = 0; i < len; i++)
  {
    U0putchar(str[i]);
  }
}

void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}