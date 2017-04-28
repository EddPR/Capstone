#include "SPI.h" // SPI Library used to clock data out to the shift registers

/*******************************************************************************************
 * Constants
*******************************************************************************************/
#define SPI_SCK         13                 // SPI clock    - must be pin 13
#define SPI_MOSI        11                 // SPI data out - must be pin 11
#define SR_LATCH         3                 // shift register latch         - any non SPI pin
#define SR_BLANK         2                 // shift register output enable - any non SPI pin

#define MAX_COLOR       15//31
#define MAX_PANEL        7
#define COORDINATE_LEVEL 7
#define CYCLE_ONE        8
#define CYCLE_TWO       24
#define CYCLE_THREE     56 
#define CYCLE_FOUR     120
#define COUNTER_LIMIT  120//248

/*******************************************************************************************
 * Global Variables
*******************************************************************************************/
volatile int BAM_Bit, BAM_Counter=0; // Bit Angle Modulation variables to keep track of things
volatile byte red0[64],   red1[64],   red2[64],   red3[64];//,   red4[64]; 
volatile byte blue0[64],  blue1[64],  blue2[64],  blue3[64];//,  blue4[64]; 
volatile byte green0[64], green1[64], green2[64], green3[64];//,  green4[64];
volatile byte level = 0;
volatile byte anodeIndex   = 0;            // keeps track of which common anode is active
volatile byte anodeLevel[] = {             // used by 74HC595N shift register to control the anode multiplexing
              B11111110,                   // This is not an efficient use of variable memory, but what the heck!
              B11111101,                   // at least it shows how the anode multiplexing is done.
              B11111011, 
              B11110111, 
              B11101111, 
              B11011111, 
              B10111111,
              B01111111}; 

/*******************************************************************************************
 * 
*******************************************************************************************/
void setup() {
  // set up IO pins as outputs
  pinMode(SPI_SCK,  OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);

  digitalWrite(SR_BLANK, HIGH);              // temporarily disable all shift register outputs
  digitalWrite(SR_LATCH,  LOW);              // set shift register latch to initial state
  
  // Set up Timer 1 for 8KHz (125uS) interrupt
  cli();                                   // stop interrupts
  TCCR2A  =   0;                           // set entire TCCR2A register to 0
  TCCR2B  =   0;                           // same for TCCR2B
  TCNT2   =   0;                           // initialize counter value to 0
  OCR2A   = 249;                           // Set compare match register for 16KHz. Increments calculated as follows:
                                           // Arduino clk frequency 16MHz / (required frequency * Prescaler of 8) - 1, Result must be < 256.
                                           // So for this instance we calculate 16,000,000 / (8,000 * 8) - 1 = 249
  TCCR2A |= (1 << WGM21);                  // turn on CTC mode
  TCCR2B |= (1 << CS21);                   // Set CS21 bit for 8 pre-scalar
  TIMSK2 |= (1 << OCIE2A);                 // enable timer compare interrupt
  sei();                                   // allow interrupts

  // initialise SPI
  SPI.setDataMode    (SPI_MODE0);       // mode 0 rising edge of data, keep clock low
  SPI.setBitOrder    (MSBFIRST);        // most significant bit first
  SPI.setClockDivider(SPI_CLOCK_DIV2);  // run the data in at 16MHz/2 - 8MHz (max speed)
  SPI.begin(); 
}

/*******************************************************************************************
 * 
*******************************************************************************************/
void loop() {
  //setPixel(7,7,7,15,0,0);
  //setPixel(0, 0, 0, 7, 12, 4); 
  colorWheel(); 
}

void colorWheel()
{
  unsigned int color[3] = { MAX_COLOR, 0, 0 };  // Start off with RED

  int x, y = 0;
  for (int dec = 0; dec < 3; dec++)
  {
  int inc = dec == 2 ? 0 : dec + 1; // If dec is 2 set to 0, else add 1

    // cross-fade the two colours.
    for (int i = 0; i < MAX_COLOR; i++)
    {      
      color[dec] -= 1;
      color[inc] += 1;
      
      if (x < 8)
      {
        //drawRectangleFilled(0, 0, 0, 7, 7, 7, color[0], color[1], color[2]);
        delay(100);
        setPixel(x, y, 0, color[0], color[1], color[2]);
        x++;
        if (x == 8)
        {
          y++;
          x = 0;
        }
      }
    }
  }
}

/*******************************************************************************************
 * 
*******************************************************************************************/
void setPixel(byte pixelX, byte pixelY, byte pixelZ, int red, int grn, int blu) {

  // test that pixel co-ordinates and colors are within boundary
  pixelX = pixelX & MAX_PANEL;  pixelY = pixelY & MAX_PANEL;  pixelZ = pixelZ & MAX_PANEL; // Allows for rotation
  red = red & MAX_COLOR;        grn = grn & MAX_COLOR;        blu = blu & MAX_COLOR;
  
  int SR_Byte = (8 * pixelY) + (7 - pixelZ);  // calculate the byte address in the shift registers

  bitWrite(red0[SR_Byte],   pixelX,   bitRead(red, 0));        // if red is 1 set appropriate bit within appropriate red byte to 1
  bitWrite(red1[SR_Byte],   pixelX,   bitRead(red, 1));
  bitWrite(red2[SR_Byte],   pixelX,   bitRead(red, 2));
  bitWrite(red3[SR_Byte],   pixelX,   bitRead(red, 3));
  //bitWrite(red4[SR_Byte],   pixelX,   bitRead(red, 4));

  bitWrite(green0[SR_Byte],   pixelX,   bitRead(grn, 0));      // if grn is 1 set appropriate bit within appropriate grn byte to 1 
  bitWrite(green1[SR_Byte],   pixelX,   bitRead(grn, 1)); 
  bitWrite(green2[SR_Byte],   pixelX,   bitRead(grn, 2)); 
  bitWrite(green3[SR_Byte],   pixelX,   bitRead(grn, 3)); 
  //bitWrite(green4[SR_Byte],   pixelX,   bitRead(grn, 4)); 
  
  bitWrite(blue0[SR_Byte],   pixelX,   bitRead(blu, 0));      // if blu is 1 set appropriate bit within appropriate grn byte to 1 
  bitWrite(blue1[SR_Byte],   pixelX,   bitRead(blu, 1)); 
  bitWrite(blue2[SR_Byte],   pixelX,   bitRead(blu, 2)); 
  bitWrite(blue3[SR_Byte],   pixelX,   bitRead(blu, 3)); 
  //bitWrite(blue4[SR_Byte],   pixelX,   bitRead(blu, 4));
}

/*******************************************************************************************
 * 
*******************************************************************************************/
ISR(TIMER2_COMPA_vect) {
  cli();  // Disable interrupts
  PORTD |= (1 << SR_BLANK);                      // temporarely disable the SR outputs

  int SR_byte;

  switch (BAM_Counter)
  {
    case CYCLE_ONE:
            BAM_Bit++;
            break;
    case CYCLE_TWO:
            BAM_Bit++;
            break;
    case CYCLE_THREE:
            BAM_Bit++;
            break;
    /*case CYCLE_FOUR:
            BAM_Bit++;
            break;*/
  }

  BAM_Counter++;

  switch (BAM_Bit)
  {
    case 0:
            for (int i = 0; i < 8; i++)
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red0[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green0[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue0[SR_byte]);    // send the blu SR byte for the given anode level
            }
            break;
    case 1:
            for (int i = 0; i < 8; i++)
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red1[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green1[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue1[SR_byte]);    // send the blu SR byte for the given anode level
            }
            break;
    case 2:
            for (int i = 0; i < 8; i++)
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red2[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green2[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue2[SR_byte]);    // send the blu SR byte for the given anode level
            }
            break;
    case 3:
            for (int i = 0; i < 8; i++)
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red3[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green3[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue3[SR_byte]);    // send the blu SR byte for the given anode level
            }
            /*break;
    case 4:
            for (int i = 0; i < 8; i++)
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red4[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green4[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue4[SR_byte]);    // send the blu SR byte for the given anode level
            }*/
            if (BAM_Counter == COUNTER_LIMIT)
            {
              BAM_Counter = 0;
              BAM_Bit = 0;
            }
            break;
  }
 
  SPI.transfer(anodeLevel[anodeIndex]);          // send the anode multiplex byte
  anodeIndex = (anodeIndex + 1) % 8;             // incrument anode index ready for next ISR
  PORTD |=  (1 << SR_LATCH);                     // set latch pin LOW
  PORTD &= ~(1 << SR_LATCH);                     // set latch pin HIGH - SR outputs now have new data
  PORTD &= ~(1 << SR_BLANK);                     // re-enable the SR outputs

  pinMode(SR_BLANK, OUTPUT);

  sei();  // Renable interrupts
}


