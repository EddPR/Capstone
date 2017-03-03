int RED [3] = { 13, 10, 7 };
int GREEN [3] = { 12, 9, 6 };
int BLUE [3] = { 11, 8, 5 };
int GROUND [3] = { 4, 3, 2 };
int wait = 5;
int centerRed = 0, centerGreen = 0, centerBlue = 0;

void setup() {
}

void loop() {
  unsigned int color[3] = { 255, 0, 0 };  // Start off with RED

  // Choose the colors to increment and decrement
  for (int dec = 0; dec < 3; dec++)
  {
    int inc = dec == 2 ? 0 : dec + 1; // If dec is 2 set to 0, else add 1

    // cross-fade the two colours.
    for (int i = 0; i < 255; i++)
    {
      //int col = col == 2 ? 0 : col + 1;
      //int row = row == 2 ? 0 : row + 1;
      
      
      color[dec] -= 1;
      color[inc] += 1;
      
      /* setColor(color[0], color[1], color[2], row, col);
      delay(wait);
      turnOff(row, col);*/
      center(color[0], color[1], color[2]);
      //snakePattern(color[0], color[1], color[2]);
      /*square(color[0], color[1], color[2]);
      if (color[0] == 255) centerRed = 255, centerGreen = 0, centerBlue = 0;
      else if (color[1] == 255) centerRed = 0, centerGreen = 255, centerBlue = 0;
      else if (color[2] == 255) centerRed = 0, centerGreen = 0, centerBlue = 255;*/
    }
  }
}

void setColor(int red, int green, int blue, int row, int col)
{
  red = 255 - red;
  green = 255 - green;
  blue = 255 - blue;

  pinMode(RED[col], OUTPUT);
  pinMode(GREEN[col], OUTPUT);
  pinMode(BLUE[col], OUTPUT);
  pinMode(GROUND[row], OUTPUT);

  analogWrite(RED[col], red);
  analogWrite(GREEN[col], green);
  analogWrite(BLUE[col], blue);
  digitalWrite(GROUND[row], HIGH);
}

void turnOff(int row, int col)
{
  analogWrite(RED[col], 255);
  analogWrite(GREEN[col], 255);
  analogWrite(BLUE[col], 255);
  digitalWrite(GROUND[row], LOW);
}

void snakePattern(int red, int green, int blue)
{
  for (int col = 0; col < 3; col++)
  {
    int row = 0;
    setColor(red, green, blue, row, col);
    delay(wait);
    turnOff(row, col);
  }
  for (int col = 2; col > -1; col--)
  {
    int row = 1;
    setColor(red, green, blue, row, col);
    delay(wait);
    turnOff(row, col);
  }
  for (int col = 0; col < 3; col++)
  {
    int row = 2;
    setColor(red, green, blue, row, col);
    delay(wait);
    turnOff(row, col);
  }
  for (int col = 2; col > -1; col--)
  {
    int row = 1;
    setColor(red, green, blue, row, col);
    delay(wait);
    turnOff(row, col);
  }
}

void square(int red, int green, int blue)
{
  for (int col = 0; col < 3; col++)
  {
    int row = 0;
    setColor(red, green, blue, row, col);
    delay(wait);
    turnOff(row, col);
  }
  setColor(red, green, blue, 1, 2);
  delay(wait);
  turnOff(1, 2);
  for (int col = 2; col > -1; col--)
  {
    int row = 2;
    setColor(red, green, blue, row, col);
    delay(wait);
    turnOff(row, col);
  }
  setColor(red, green, blue, 1, 0);
  delay(wait);
  turnOff(1, 2);

  setColor(centerRed, centerGreen, centerBlue, 1, 1);
  delay(wait);
  turnOff(1,1);
}

void center(int red, int green, int blue)
{
  setColor(red, green, blue, 1, 1);
  delay(wait);
}

