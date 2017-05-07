/*
 * Cube_Sketch.ino
 * 
 * Disclaimer:  The implementation of the setup(), setPixel(), and ISR are our own extensions of Steve Manley's test code
 *              which can be found here https://www.dropbox.com/sh/fyvszu2xnow17fg/AACIvhJXBHD9Ufc1iOc-I8F3a/Arduino%20Sketches/Cube_Test_V0.0?dl=0
 *              The animations where inspired by a Youtube video titled "3D 8x8x8 RGB LED Cube - DEMO and BUILD [extended version]"
 *              and that can be found here https://www.youtube.com/watch?v=eO8LM893oGw
 *              Kevin Darrah's tutorial and explanation videos where also of great help. 
 *              His Youtube channel can be found here https://www.youtube.com/user/kdarrah1234
 *
 * Created: 3/21/2017 12:34:55 PM
 * Author:  Eduardo Padilla 
 * Author:  Dagan Danevic 
 */

#include "SPI.h" // SPI Library used to clock data out to the shift registers

/*******************************************************************************************
 * Constants
*******************************************************************************************/
#define SPI_SCK         13                 // SPI clock    - must be pin 13
#define SPI_MOSI        11                 // SPI data out - must be pin 11
#define SR_LATCH         3                 // shift register latch         - any non SPI pin
#define SR_BLANK         2                 // shift register output enable - any non SPI pin

#define MAX_COLOR       15                 // Max color value (2^4 - 1 = 15)
#define MAX_PANEL        7                 // Maz Z value                
#define CYCLE_ONE        8                 // BAM bit one cylcle
#define CYCLE_TWO       24                 // BAM bit two cycle
#define CYCLE_THREE     56                 // BAM bit three cycle
#define COUNTER_LIMIT  120                 // BAM bit four cycle (LIMIT in this case)

/*******************************************************************************************
 * Global Variables
*******************************************************************************************/
volatile int  BAM_Bit,    BAM_Counter = 0; // Bit Angle Modulation variables to keep track of things
volatile byte red0[64],   red1[64],   red2[64],   red3[64];    // Represents four bit resolution for RED
volatile byte green0[64], green1[64], green2[64], green3[64];  // Represents four bit resolution for GREEN
volatile byte blue0[64],  blue1[64],  blue2[64],  blue3[64];   // Represents four bit resolution for BLUE

volatile byte anodeIndex = 0;             // keeps track of which common anode is active
volatile byte anodeLevel[] = {            // used by 74HC595N shift register to control the anode multiplexing
              B11111110,                   
              B11111101,                   
              B11111011, 
              B11110111, 
              B11101111, 
              B11011111, 
              B10111111,
              B01111111}; 

/*******************************************************************************************
 * Function sets up pins for output and the Timer 2 interrupt handler
*******************************************************************************************/
void setup() {
  // set up IO pins as outputs
  pinMode(SPI_SCK,  OUTPUT);                 // Clock
  pinMode(SPI_MOSI, OUTPUT);                 // Data Out
  pinMode(SR_LATCH, OUTPUT);                 // Latch

  digitalWrite(SR_BLANK, HIGH);              // temporarily disable all shift register outputs
  digitalWrite(SR_LATCH,  LOW);              // set shift register latch to initial state
  
  // Set up Timer 2 for 8KHz (125uS) interrupt
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
  SPI.begin();                          // start up SPI

  randomSeed(analogRead(0));            // initializing pseudo-random number generator for random sequences
}

/*******************************************************************************************
 * Function that runs all the animations continuously.
 * This is where all the animations should go.
*******************************************************************************************/
void loop() {
  //test();
  //colorArray(); 
  //clearCube();
  //bamDemo(100);
  sineWave();
  snakeEffect();
  rain();
  doubleSineWave();
  cubeFill();
}

/*******************************************************************************************
 * Function lights up every LED by building cubes of different colors.
 * Cube 1: 2 x 2 x 2
 * Cube 2: 4 x 4 x 4
 * Cube 3: 6 x 6 x 6
 * Cube 4: 8 x 8 x 8
*******************************************************************************************/
void cubeFill()
{
  int red, grn, blu;
  int colorSelect = -1;
  int color[3];

  int xMin, xMax, yMin, yMax, zMin, zMax;

  int minArray[4] = {3, 2, 1, 0};
  int maxArray[4] = {4, 5, 6, 7};

  int t = 45;
  unsigned long finish = 10000;   // Animation run time
  unsigned long start = millis();   // Initiating counter
  
  while (millis() - start < finish)
  {
    for (int i = 0; i < 4; i++)
    {
      xMin = minArray[i];
      yMin = minArray[i];
      zMin = minArray[i];
  
      xMax = maxArray[i];
      yMax = maxArray[i];
      zMax = maxArray[i];
      
      for (int x = xMin; x < xMax + 1; x++) 
      {
        colorFading(colorSelect++, color);
        if (colorSelect == 47) colorSelect = -1;
        red = color[0];
        grn = color[1];
        blu = color[2];
        
        for (int y = yMin; y < yMax + 1; y++) 
        {
          for (int z = zMin; z < zMax + 1; z++) 
          {
            setPixel(x, y, z, red, grn, blu);
          }
        }
        
        delay(t);
      }
    }
  }
  clearCube();    // Clear cube to prevent "shadowed" setPixels from this animation
}

/*******************************************************************************************
 * Function runs an LED from top to bottom in a column, and another from the bottom up.
 * The idea is if there iterate fast enough, both LEDs will effectively mix to form a 
 * blended color. This is the idea behand Bit Angle Moduation
 * t -  Time used to dictate how fast the LEDs are refreshed
*******************************************************************************************/
void bamDemo(int t)
{
  for (int i = 7; i > -1; i--)
  {
    setPixel(7, i, 4, 15, 0, 0);
    setPixel(7, 7-i, 4, 0, 0, 15);
    delay(t);
    setPixel(7, i, 4, 0, 0, 0);
    setPixel(7, 7-i, 4, 0, 0, 0);
  }
}

/*******************************************************************************************
 * Function that simulates rain by creating a random opportunity for LEDs to 'appear.'
 * Everytime a rain drop reaches the base, the color changes to simulate a splash.
*******************************************************************************************/
void rain()
{
  int droplets = 64;      // Number of rain droplets to be displayed
  int colorSwitch;        // Counter for deciding when to switch color
  
  int oldX[droplets];     // Array to hold previous x values
  int oldY[droplets];     // Array to hold previous y values
  int oldZ[droplets];     // Array to hold previous z values
  int newX[droplets];     // Array to hold current x values
  int newY[droplets];     // Array to hold current y values
  int newZ[droplets];     // Array to hold current z values
  
  int i;            // for-loop index 
  
  for (i = 0; i < droplets; i++)
  {
    newX[i] = random(8);  // Random number between 0-7 for x-plane
    newY[i] = random(8);  // Random number between 0-7 for y-plane
    newZ[i] = random(8);  // Random number between 0-7 for z-plane
  }
  
  int t = 15;                     // Time to be used in delay
  unsigned long finish = 20000;   // Animation run time
  unsigned long start = millis(); // Initiating counter
  
  while (millis() - start < finish)
  {
    if (colorSwitch < 200)
    {
      for (i = 0; i < droplets; i++)
      {
        setPixel(oldX[i], oldZ[i], oldY[i], 0, 0, 0); // Clear old droplet
        
        switch (newZ[i])  // Change colors based on Z value
        {
          case 0:
              setPixel(newX[i], newZ[i], newY[i], 10, 0, 1);
              break;
          case 1:
              setPixel(newX[i], newZ[i], newY[i], 10, 0, 10);
              break;
          case 2:
              setPixel(newX[i], newZ[i], newY[i], 10, 0, 15);
              break;
          case 3:
              setPixel(newX[i], newZ[i], newY[i], 3, 0, 12);
              break;
          case 4:
              setPixel(newX[i], newZ[i], newY[i], 1, 0, 11);
              break;
          case 5:
              setPixel(newX[i], newZ[i], newY[i], 0, 0, 10);
              break;
          case 6:
              setPixel(newX[i], newZ[i], newY[i], 0, 1, 9);
              break;
          case 7:
              setPixel(newX[i], newZ[i], newY[i], 0, 5, 15);
              break;
        }
      }
    }
    
    if (colorSwitch >= 200 && colorSwitch < 300)
    {
      for (i = 0; i < droplets; i++)
      {
        setPixel(oldX[i], oldZ[i], oldY[i], 0, 0, 0); // Clear old droplet
        
        switch (newZ[i])  // Change colors based on Z value
        {
          case 0:
              setPixel(newX[i], newZ[i], newY[i], 10, 0, 0);
              break;
          case 1:
              setPixel(newX[i], newZ[i], newY[i], 12, 0, 0);
              break;
          case 2:
              setPixel(newX[i], newZ[i], newY[i], 15, 0, 0);
              break;
          case 3:
              setPixel(newX[i], newZ[i], newY[i], 15, 1, 0);
              break;
          case 4:
              setPixel(newX[i], newZ[i], newY[i], 15, 2, 0);
              break;
          case 5:
              setPixel(newX[i], newZ[i], newY[i], 15, 5, 0);
              break;
          case 6:
              setPixel(newX[i], newZ[i], newY[i], 10, 10, 0);
              break;
          case 7:
              setPixel(newX[i], newZ[i], newY[i], 15, 15, 0);
              break;
        }
      }
    }
    
    colorSwitch++;    // Incrament counter
    
    if (colorSwitch >= 300) // If the top value has been reached
    {
      colorSwitch = 0;  // Reset counter 
    }
    
    for (i = 0; i < droplets; i++)
    {
      oldX[i] = newX[i];  // Save old X value
      oldY[i] = newY[i];  // Save old Y value
      oldZ[i] = newZ[i];  // Save old Z value
      
      newZ[i] -= 1;   // Decrement current Z value
      
      if (newZ[i] < random(-100, 0))  // Produce a random opportunity for droplet locations to change
      {
        newX[i] = random(8);  // Random number between 0-7 for x-plane
        newY[i] = random(8);  // Random number between 0-7 for y-plane
        newZ[i] = 7;
      }
    }
    delay(t);
  }
  clearCube();    // Clear cube to prevent "shadowed" setPixels from this animation
}

/*******************************************************************************************
 * Function simulates an oscillating SINE wave. There are four 'frames' formed to fill in the
 * shape of the wave. Each frame is then shifted upwards - or downwards - at a different 
 * rate to get that oscillation effect.
*******************************************************************************************/
void sineWave()
{  
  int oldArray[8];
  int newArray[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  int multiplyer[8] = {1, 1, 1, 1, 1, 1, 1, 1};
  int color[3];

  int oldIndex;
  int newIndex;
  
  int red;
  int grn;
  int blu;
  
  int i;
  int low = -1;
  int top = 9;
  int lowOff = -1;
  int topOff = 8;
  int colorSelect = -1;

  int t = 34;                     // Time to be used in delay
  unsigned long finish = 15000;   // Animation run time
  unsigned long start = millis(); // Initiating counter
  
  while (millis() - start < finish)
  {
    for (i = 0; i < 8; i++)
    {
      if (newArray[i] == 7)
      {
        multiplyer[i] = -1;
      }
      if (newArray[i] == 0)
      {
        multiplyer[i] = 1;
      }
      newArray[i] += multiplyer[i];
    }
  
    colorFading(colorSelect++, color);
    if (colorSelect == 47) colorSelect = -1;
    red = color[0];
    grn = color[1];
    blu = color[2];
    
    for (int cycle = 0; cycle < 4; cycle++)
    {
      low++;  lowOff++;
      top--;  topOff--;

      for (i = low; i < top; i++)
      {
        oldIndex = oldArray[i];
        setPixel(i,       oldIndex,  lowOff,   0,    0,    0);
        setPixel(lowOff,  oldIndex,  i,        0,    0,    0);
        setPixel(7 - i,   oldIndex,  topOff,   0,    0,    0);
        setPixel(topOff,  oldIndex,  7 - i,    0,    0,    0);
        
        newIndex = newArray[i];
        setPixel(i,       newIndex,  lowOff,   red,  grn,  blu);
        setPixel(lowOff,  newIndex,  i,        red,  grn,  blu);
        setPixel(7 - i,   newIndex,  topOff,   red,  grn,  blu);
        setPixel(topOff,  newIndex,  7 - i,    red,  grn,  blu);
      }
    }
    
    for (i = 0; i < 8; i++)
    {
      oldArray[i] = newArray[i];
    }

    low = -1;
    top = 9;
    lowOff = -1;
    topOff = 8;
    
    delay(t);
  }   
  clearCube();    // Clear cube to prevent "shadowed" LEDs from this animation
}

/*******************************************************************************************
 * Function produces two oscillating sine waves that are perpendicular to eachother.
*******************************************************************************************/
void doubleSineWave()
{  
  int oldArray[8];
  int newArray[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  int multiplyer[8] = {1, 1, 1, 1, 1, 1, 1, 1};
  int color[3];

  int oldIndex;
  int newIndex;
  
  int red;
  int grn;
  int blu;
  
  int i;
  int low = -1;
  int top = 9;
  int lowOff = -1;
  int topOff = 8;
  int colorSelect = -1;

  int t = 34;                     // Time to be used in delay
  unsigned long finish = 15000;   // Animation run time
  unsigned long start = millis(); // Initiating counter
  
  while (millis() - start < finish)
  {
    for (i = 0; i < 8; i++)
    {
      if (newArray[i] == 7)
      {
        multiplyer[i] = -1;
      }
      if (newArray[i] == 0)
      {
        multiplyer[i] = 1;
      }
      newArray[i] += multiplyer[i];
    }
  
    colorFading(colorSelect++, color);
    if (colorSelect == 47) colorSelect = -1;
    red = color[0];
    grn = color[1];
    blu = color[2];
    
    for (int cycle = 0; cycle < 4; cycle++)
    {
      low++;  lowOff++;
      top--;  topOff--;

      for (i = low; i < top; i++)
      {
        oldIndex = oldArray[i];
        newIndex = newArray[i];
        
        setPixel(oldIndex,  i,         lowOff,   0,    0,    0);
        setPixel(oldIndex,  lowOff,    i,        0,    0,    0);
        setPixel(oldIndex,  7 - i,     topOff,   0,    0,    0);
        setPixel(oldIndex,  topOff,    7 - i,    0,    0,    0);
        
        setPixel(newIndex,  i,         lowOff,   red,  grn,  blu);
        setPixel(newIndex,  lowOff,    i,        red,  grn,  blu);
        setPixel(newIndex,  7 - i,     topOff,   red,  grn,  blu);
        setPixel(newIndex,  topOff,    7 - i,    red,  grn,  blu);

        setPixel(i,       oldIndex,  lowOff,   0,    0,    0);
        setPixel(lowOff,  oldIndex,  i,        0,    0,    0);
        setPixel(7 - i,   oldIndex,  topOff,   0,    0,    0);
        setPixel(topOff,  oldIndex,  7 - i,    0,    0,    0);
        
        setPixel(i,       newIndex,  lowOff,   red,  grn,  blu);
        setPixel(lowOff,  newIndex,  i,        red,  grn,  blu);
        setPixel(7 - i,   newIndex,  topOff,   red,  grn,  blu);
        setPixel(topOff,  newIndex,  7 - i,    red,  grn,  blu);
      }
    }
    
    for (i = 0; i < 8; i++)
    {
      oldArray[i] = newArray[i];
    }

    low = -1;
    top = 9;
    lowOff = -1;
    topOff = 8;
    
    delay(t);
  }   
  clearCube();    // Clear cube to prevent "shadowed" LEDs from this animation
}

/*******************************************************************************************
 * Function color fades two colors at a time and then displays 45 different colors in the 
 * first 45 LEDs.
*******************************************************************************************/
void colorArray()
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
 * Function attempts to simulate a moving snake accross all planes to get a sense of depth.
 * The snakes head will move in random direction with the same color until it hits an edge.
 * Once it hits an edge, it will change colors. A random number is produces and used to 
 * change the direction of the snake in either the X, Y, or Z direction.
*******************************************************************************************/
void snakeEffect()
{
  int snakeLength = 18;
  int direct;
  
  int i;
  int x = 1, y = 1, z = 1;
  int colorSelect = 0;
  int color[3];
  
  int oldX[snakeLength + 1];
  int oldY[snakeLength + 1];
  int oldZ[snakeLength + 1];
  int newX[snakeLength + 1];
  int newY[snakeLength + 1];
  int newZ[snakeLength + 1];
  
  int red[snakeLength + 1];
  int grn[snakeLength + 1];
  int blu[snakeLength + 1];
  
  int t = 34;                     // Time to be used in delay
  unsigned long finish = 15000;   // Animation run time
  unsigned long start = millis(); // Initiating counter
  
  while (millis() - start < finish)
  {
    direct = random(3);

    if (colorSelect >= 47) colorSelect = 0;
    colorFading(colorSelect, color);
    red[0] = color[0];
    grn[0] = color[1];
    blu[0] = color[2];
    
    for (i = 1; i < snakeLength + 1; i++)
    {
      setPixel(oldX[i], oldY[i], oldZ[i], 0, 0, 0);
      setPixel(newX[i], newY[i], newZ[i], red[i], grn[i], blu[i]);
      
      oldX[i] = newX[i];
      oldY[i] = newY[i];
      oldZ[i] = newZ[i];
    }
    
    delay(t);
    
    switch (direct)
    {
      case 0:
          newX[0] += x;
          break;
      case 1:
          newY[0] += y;
          break;
      case 2:
          newZ[0] += z;
          break;
    }
    
    if (newX[0] > 7)
    {
      x = -1;
      newX[0] = 7;
      colorSelect++;
      colorSelect++;
    }
    if (newX[0] < 0)
    {
      x = 1;
      newX[0] = 0;
      colorSelect++;
    }
    
    if (newY[0] > 7)
    {
      y = -1;
      newY[0] = 7;
      colorSelect++;
      colorSelect++;
      colorSelect++;
    }
    if (newY[0] < 0)
    {
      y = 1;
      newY[0] = 0;
      colorSelect++;
    }
    
    if (newZ[0] > 7)
    {
      z = -1;
      newZ[0] = 7;
      colorSelect++;
    }
    if (newZ[0] < 0)
    {
      z = 1;
      newZ[0] = 0;
      colorSelect++;
      colorSelect++;
    }
    
    for (i = snakeLength; i > 0; i--)
    {
      newX[i] = newX[i - 1];
      newY[i] = newY[i - 1];
      newZ[i] = newZ[i - 1];
      
      red[i] = red[i - 1];
      grn[i] = grn[i - 1];
      blu[i] = blu[i - 1];
    }
  }
  clearCube();    // Clear cube to prevent "shadowed" LEDs from this animation
}

/*******************************************************************************************
 * Function that powers of every LED
*******************************************************************************************/
void clearCube()
{
  for (int z = 0; z < 8; z++)
  {
    for (int y = 0; y < 8; y++)
    {
      for (int x = 0; x < 8; x++)
      {
         setPixel(x, y, z, 0, 0, 0);
      } 
    }
  }
}

/*******************************************************************************************
 * Function that blinks every LED, and then the whole cube all together.
*******************************************************************************************/
void test()
{
  for (int z = 0; z < 8; z++)
  {
    for (int y = 0; y < 8; y++)
    {
      for (int x = 0; x < 8; x++)
      {
        setPixel(x, y, z, 15, 0, 0);
        delay(150);
        setPixel(x, y, z, 0, 0, 0);

        setPixel(x, y, z, 0, 15, 0);
        delay(150);
        setPixel(x, y, z, 0, 0, 0);

        setPixel(x, y, z, 0, 0, 15);
        delay(150);
        setPixel(x, y, z, 0, 0, 0);
      }
    }
  }

  for (int z = 0; z < 8; z++)
  {
    for (int y = 0; y < 8; y++)
    {
      for (int x = 0; x < 8; x++)
      {
        setPixel(x, y, z, 15, 0, 0);
      }
    }
  }
  delay(500);
  clearCube();
  
  for (int z = 0; z < 8; z++)
  {
    for (int y = 0; y < 8; y++)
    {
      for (int x = 0; x < 8; x++)
      {
        setPixel(x, y, z, 0, 15, 0);
      }
    }
  }
  delay(500);
  clearCube();
  
  for (int z = 0; z < 8; z++)
  {
    for (int y = 0; y < 8; y++)
    {
      for (int x = 0; x < 8; x++)
      {
        setPixel(x, y, z, 0, 0, 15);
      }
    }
  }
  delay(500);
  clearCube();
}

/*******************************************************************************************
 * Function that fades two colors to produce a total of 46 Different variations.
 * Here, it's done in a switch statement to lesson the time impact of a function using it.
 * select -   Number used to select a specific fade.
 * color[] -  Array holding all 3 color values
*******************************************************************************************/
void colorFading(int select, int (&color)[3])
{
  switch (select)
  {
    case 1:
        color[0] = 15;
        color[1] = 0;
        color[2] = 0;
        break;
    case 2:
        color[0] = 14;
        color[1] = 1;
        color[2] = 0;
        break;
    case 3:
        color[0] = 13;
        color[1] = 2;
        color[2] = 0;
        break;
    case 4:
        color[0] = 12;
        color[1] = 3;
        color[2] = 0;
        break;
    case 5:
        color[0] = 11;
        color[1] = 4;
        color[2] = 0;
        break;
    case 6:
        color[0] = 10;
        color[1] = 5;
        color[2] = 0;
        break;
    case 7:
        color[0] = 9;
        color[1] = 6;
        color[2] = 0;
        break;
    case 8:
        color[0] = 8;
        color[1] = 7;
        color[2] = 0;
        break;
    case 9:
        color[0] = 7;
        color[1] = 8;
        color[2] = 0;
        break;
    case 10:
        color[0] = 6;
        color[1] = 9;
        color[2] = 0;
        break;
    case 11:
        color[0] = 5;
        color[1] = 10;
        color[2] = 0;
        break;
    case 12:
        color[0] = 4;
        color[1] = 11;
        color[2] = 0;
        break;
    case 13:
        color[0] = 3;
        color[1] = 12;
        color[2] = 0;
        break;
    case 14:
        color[0] = 2;
        color[1] = 13;
        color[2] = 0;
        break;
    case 15:
        color[0] = 1;
        color[1] = 14;
        color[2] = 0;
        break;
    case 16:
        color[0] = 0;
        color[1] = 15;
        color[2] = 0;
        break;
    case 17:
        color[0] = 0;
        color[1] = 14;
        color[2] = 1;
        break;
    case 18:
        color[0] = 0;
        color[1] = 13;
        color[2] = 2;
        break;
    case 19:
        color[0] = 0;
        color[1] = 12;
        color[2] = 3;
        break;
    case 20:
        color[0] = 0;
        color[1] = 11;
        color[2] = 4;
        break;
    case 21:
        color[0] = 0;
        color[1] = 10;
        color[2] = 5;
        break;
    case 22:
        color[0] = 0;
        color[1] = 9;
        color[2] = 6;
        break;
    case 23:
        color[0] = 0;
        color[1] = 8;
        color[2] = 7;
        break;
    case 24:
        color[0] = 0;
        color[1] = 7;
        color[2] = 8;
        break;
    case 25:
        color[0] = 0;
        color[1] = 6;
        color[2] = 9;
        break;
    case 26:
        color[0] = 0;
        color[1] = 5;
        color[2] = 10;
        break;
    case 27:
        color[0] = 0;
        color[1] = 4;
        color[2] = 11;
        break;
    case 28:
        color[0] = 0;
        color[1] = 3;
        color[2] = 12;
        break;
    case 29:
        color[0] = 0;
        color[1] = 2;
        color[2] = 13;
        break;
    case 30:
        color[0] = 0;
        color[1] = 1;
        color[2] = 14;
        break;
    case 31:
        color[0] = 0;
        color[1] = 0;
        color[2] = 15;
        break;
    case 32:
        color[0] = 1;
        color[1] = 0;
        color[2] = 14;
        break;
    case 33:
        color[0] = 2;
        color[1] = 0;
        color[2] = 13;
        break;
    case 34:
        color[0] = 3;
        color[1] = 0;
        color[2] = 12;
        break;
    case 35:
        color[0] = 4;
        color[1] = 0;
        color[2] = 11;
        break;
    case 36:
        color[0] = 5;
        color[1] = 0;
        color[2] = 10;
        break;
    case 37:
        color[0] = 6;
        color[1] = 0;
        color[2] = 9;
        break;
    case 38:
        color[0] = 7;
        color[1] = 0;
        color[2] = 8;
        break;
    case 39:
        color[0] = 8;
        color[1] = 0;
        color[2] = 7;
        break;
    case 40:
        color[0] = 9;
        color[1] = 0;
        color[2] = 6;
        break;
    case 41:
        color[0] = 10;
        color[1] = 0;
        color[2] = 5;
        break;
    case 42:
        color[0] = 11;
        color[1] = 0;
        color[2] = 4;
        break;
    case 43:
        color[0] = 12;
        color[1] = 0;
        color[2] = 3;
        break;
    case 44:
        color[0] = 13;
        color[1] = 0;
        color[2] = 2;
        break;
    case 45:
        color[0] = 14;
        color[1] = 0;
        color[2] = 1;
        break;
    case 46:
        color[0] = 15;
        color[1] = 0;
        color[2] = 0;
        break;
        
  }
}

/*******************************************************************************************
 * Function determines how an LED is addressed, by location and RGB value.
 * pixelX - X location
 * pixelY - Y location
 * pixelZ - Z location
 * red    - RED intensity value
 * grn    - Green intensity value
 * blu    - Blue intensity value
*******************************************************************************************/
void setPixel(byte pixelX, byte pixelY, byte pixelZ, int red, int grn, int blu) {

  // test that pixel co-ordinates and colors are within boundary
  pixelX = pixelX & MAX_PANEL;  pixelY = pixelY & MAX_PANEL;  pixelZ = pixelZ & MAX_PANEL; // Allows for rotation
  red = red & MAX_COLOR;        grn = grn & MAX_COLOR;        blu = blu & MAX_COLOR;       // Allows for rotation
  
  int SR_Byte = (8 * pixelY) + (7 - pixelZ);  // calculate the byte address in the shift registers

  bitWrite(red0[SR_Byte],   pixelX,   bitRead(red, 0));        // Set appropriate bit within appropriate red byte 
  bitWrite(red1[SR_Byte],   pixelX,   bitRead(red, 1));        // Done four times for 4-bit color resolution
  bitWrite(red2[SR_Byte],   pixelX,   bitRead(red, 2));
  bitWrite(red3[SR_Byte],   pixelX,   bitRead(red, 3));

  bitWrite(green0[SR_Byte],   pixelX,   bitRead(grn, 0));      // Set appropriate bit within appropriate grn byte
  bitWrite(green1[SR_Byte],   pixelX,   bitRead(grn, 1));      // Done four times for 4-bit color resolution
  bitWrite(green2[SR_Byte],   pixelX,   bitRead(grn, 2)); 
  bitWrite(green3[SR_Byte],   pixelX,   bitRead(grn, 3)); 
  
  bitWrite(blue0[SR_Byte],   pixelX,   bitRead(blu, 0));       // Set appropriate bit within appropriate blu byte
  bitWrite(blue1[SR_Byte],   pixelX,   bitRead(blu, 1));       // Done four times for 4-bit color resolution
  bitWrite(blue2[SR_Byte],   pixelX,   bitRead(blu, 2)); 
  bitWrite(blue3[SR_Byte],   pixelX,   bitRead(blu, 3)); 
}

/*******************************************************************************************
 * The heart of the program, this function is the background interrupt handler.
 * It determines the frequency at which the cube is refreshed. 
 * Since the handler is called every 125uS (8KHz), each plane is lit for 1/8th of the time (1KHz).
 * This means that max current draw is that of a 64 LEDs, or 33mA.
*******************************************************************************************/
ISR(TIMER2_COMPA_vect) {
  cli();  // Disable interrupts
  PORTD |= (1 << SR_BLANK);                     // temporarely disable the SR outputs

  int SR_byte;                                  // byte to be aent to the shift register

  // 4-bit Bit Angle Modulation
  switch (BAM_Counter)                          // Cycle counter
  {
    case CYCLE_ONE:                             // When a 1 is written to a colors luminasity, a level will have one cycle to light up 
            BAM_Bit++;                          // Counter tracks which bit we are modulating. Increment on each cycle
            break;
    case CYCLE_TWO:
            BAM_Bit++;
            break;
    case CYCLE_THREE:
            BAM_Bit++;
            break;
  }

  BAM_Counter++;                               // When we light up a level, increment the counter

  switch (BAM_Bit)                             // Shift out 64 bits on every cycle
  {
    case 0:                                    // Bit 0 in the BAM process
            for (int i = 0; i < 8; i++)        // Shift out 8 bytes for each color
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red0[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green0[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue0[SR_byte]);    // send the blu SR byte for the given anode level
            }
            break;
    case 1:                                    // Bit 1 in the BAM process
            for (int i = 0; i < 8; i++)        // Shift out 8 bytes for each color
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red1[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green1[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue1[SR_byte]);    // send the blu SR byte for the given anode level
            }
            break;
    case 2:                                    // Bit 2 in the BAM process
            for (int i = 0; i < 8; i++)        // Shift out 8 bytes for each color
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red2[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green2[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue2[SR_byte]);    // send the blu SR byte for the given anode level
            }
            break;
    case 3:                                    // Bit 3 in the BAM process
            for (int i = 0; i < 8; i++)        // Shift out 8 bytes for each color
            {
              SR_byte = (i + anodeIndex * 8);
              SPI.transfer(red3[SR_byte]);     // send the red SR byte for the given anode level
              SPI.transfer(green3[SR_byte]);   // send the grn SR byte for the given anode level
              SPI.transfer(blue3[SR_byte]);    // send the blu SR byte for the given anode level
            }
            if (BAM_Counter == COUNTER_LIMIT)  // Reset counters once we've reached the four bit
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

  pinMode(SR_BLANK, OUTPUT);                     // Outputs enabled

  sei();  // Renable interrupts
}


