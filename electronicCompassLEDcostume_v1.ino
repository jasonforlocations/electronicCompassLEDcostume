/*
  Part of a first dance costume
  Uses digital compass
  By: Jason
  Modified from "Example2-I2C_Digital_compass, Compute magnetic heading from the MMC5983MA" (by Nathan Seidle and Ricardo Ramos)
  and from "Example03_SinglePixels2, An I2C based LED Stick" (by Ciara Jekel)
  Date: ...

  This is supposed to use the Earth's magnetic field to calculate an angle related to how much the part is pointing towards the location a dance is about.
  The magnitude of the angle is used to control the behavior of LEDs on a LED stick.

  Sometimes it doesn’t work.
*/

//  Might have to be made into a comment (using two forward slashes), or deleted, if using a different microcontroller
#define Serial SerialUSB

//  If you can't access the below two webpages, according to SparkFun webpages, you might try installing the libraries through the Arduino Library Manager tool by searching for "SparkFun MMC5983MA" and "SparkFun Qwiic LED Stick".
#include <Wire.h>
#include <SparkFun_MMC5983MA_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_MMC5983MA
#include "Qwiic_LED_Stick.h" // Click here to get the library: http://librarymanager/All#SparkFun_Qwiic_LED_Stick

SFE_MMC5983MA myMag;
int angle;             // Magnetometer results
int adjustmentAmount = 245; // Used to change those results, such as when more lights are supposed to be lit, even though the part of the dance costume is pointing less towards 180 degrees

LED LEDStick; // Create an object of the LED class
const int numLEDs = 10;  // Number of LEDs on the stick


//  The following three arrays will store how much each of the 10 LEDs are green, red, and blue.  That is why numLEDs equals 10.

/*
  The following three lines of comments (which can be skipped) contain a bit of philosophy.
  The dancers are supposed to seem more (I'm not sure of what yet) when more green LEDs are lit.
  Green (like what part of a traffic light sometimes looks like) means "go" ahead and continue pointing more towards the location.  In addition to the green lights, working harder at dancing for a location might help put the location on the map.
*/

byte greenArray[numLEDs];
byte redArray[numLEDs];      //Red (like what part of a traffic light sometimes looks like) means "stop" pointing away from the location, because the location is important.
byte blueArray[numLEDs];
const int d = 16;  // Used to dim all of the LEDs on the stick

void setup()
{
  delay (3000);  // Give me time to open the Serial Monitor, or hold the magnetometer in a certain position

  Serial.begin(115200);
  Serial.println("MMC5983MA Example");

  Wire.begin();

  if (myMag.begin() == false)  // Still need to figure out what makes it false
  {
    Serial.println("MMC5983MA did not respond - check your wiring. Freezing.");
    //    while (true);

  }

  myMag.softReset();  //used here in digital compass sketch
  Serial.println("MMC5983MA connected");

  //Start up communication with the LED Stick
  if (LEDStick.begin() == false) {  // Still need to figure out what makes it false
    Serial.println("Qwiic LED Stick failed to begin. Please check wiring and try again!");
    //    while (1);
  }

  Serial.println("Qwiic LED Stick ready!");
}

void loop()
{
  //  myMag.softReset();  //used here in test sketch, wherever that was
  uint32_t rawValueX = 0;
  uint32_t rawValueY = 0;
  uint32_t rawValueZ = 0;

  double heading = 0;

  // Read all three channels simultaneously
  myMag.getMeasurementXYZ(&rawValueX, &rawValueY, &rawValueZ);

  heading = calibration(rawValueY, rawValueX, rawValueZ);

  Serial.print("heading: ");
  Serial.println(heading, 1);

  angle = (int)heading;
  angle += adjustmentAmount;

  // Keep angles less than 181 degrees.
  // Because, for example, 181 is greater than 180, but if the location is towards 180 degrees, 181 degrees is less towards the location than 180.

  switch (angle)
  {
    case 0 ... 180:
      break;
    case 181 ... 360:
      angle = 360 - angle;
      break;
    case 361 ... 540:  // Angles can be more than 360 degrees because of the adjustmentAmount variable.
      angle = 720 - angle;
      break;
    case 541 ... 720:
      angle = 720 - angle;
      break;
    case 721 ... 901:
      angle = 1080 - angle;
      break;
  }

  ledControllerHelper (angle);
}

void ledControllerHelper (int angle) {

  switch (angle)
  {
    case 0 ... 90:
      {
        ledController (0);  // The parameter is the number of LEDs that will be lit.
        break;
      }
    case 91 ... 99:
      {
        ledController (1);
        break;
      }
    case 100 ... 108:
      {
        ledController (2);
        break;
      }
    case 109 ... 117:
      {
        ledController (3);
        break;
      }
    case 118 ... 126:
      {
        ledController (4);
        break;
      }
    case 127 ... 135:
      {
        ledController (5);
        break;
      }
    case 136 ... 144:
      {
        ledController (6);
        break;
      }
    case 145 ... 153:
      {
        ledController (7);
        break;
      }
    case 154 ... 162:
      {
        ledController (8);
        break;
      }
    case 163 ... 171:
      {
        ledController (9);
        break;
      }
    case 172 ... 181:
      {
        ledController (10);
        break;
      }
  }
}

void ledController (int i) {

  for (int j = 0; j < 10; j++) {
    blueArray[j] = 0; // turn all blue lights off
  }

  Serial.print("numLEDs = ");
  Serial.println(i);

  if (i % 2 == 0 && i != 10) { // All LEDs are off, or there is an even number of lit LEDs.  The lit LEDs will be in the middle of the stick.
    // I don't know why 10 doesn't work here.
    for (int j = 0; j < 5 - i / 2; j++) {

      redArray[j] = 0;      // Turns the LED(s) to the left of the stick off
      redArray[9 - j] = 0;  // Turns the LED(s) to the right off
      greenArray[j] = 0;
      greenArray[9 - j] = 0;
    }
    for (int j = 5 - i / 2; j < 5 + i / 2; j++) { // Turns the LEDs in the middle on
      redArray[j] = (10 - i) * 25 / d; // d dims
      greenArray[j] = i * 25 / d;
    }

  } else if (i != 9 && i != 10) { // I don't know why 9 doesn't work here.

    /*
              Unless there is only one LED lit, lit LEDs will be together
              Because there is an odd number of LEDs, more LEDs have to be off to the left or right of the stick.
              More LEDs will be off to the right of the stick
    */

    for (int j = 0; j < 4 - (i - 1) / 2; j++) {
      redArray[j] = 0;
      redArray[8 - j] = 0;
      greenArray[j] = 0;
      greenArray[8 - j] = 0;
    }

    for (int j = 4 - (i - 1) / 2; j < 5 + (i - 1) / 2; j++) {
      redArray[j] = (10 - i) * 25 / d; // d dims
      greenArray[j] = i * 25 / d;
    }

  } else if (i == 9) { // I don't know why 9 doesn't work when i%2 == 1
    redArray[9] = 0;
    greenArray[9] = 0;
    redArray[0] = (10 - i) * 25 / d;
    greenArray[0] = i * 25 / d;
    for (int j = 1; j < 9; j++) {
      redArray[j] = (10 - i) * 25 / d;
      greenArray[j] = i * 25 / d;
      LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);  //  Why do I need this here also, in addition to below?
    }

  } else if (i == 10) { // I don't know why 10 doesn't work when i%2 == 0
    for (int j = 0; j < 10; j++) {
      redArray[j] = (10 - i) * 25 / d;
      greenArray[j] = i * 25 / d;
    }
  }

  //  One way to format how it is printed
  //  Very similar to part of a sketch recommended by user build_1971 at a forum

  /*
    for (int j = 0; j < 10; j++) {
      Serial.print("Pixel: ");
      Serial.println(j);

      Serial.print("   red: ");
      Serial.print(redArray[j]);
      Serial.print(",   ");
      Serial.print("   green: ");
      Serial.println(greenArray[j]);
    }
  */

  //  Another way to format how it is printed
  /*
    Serial.println("Pixel# 1  2  3 4  5  6  7 8  9  10");
    Serial.println((String)"Red:   " + redArray[0] + " " + redArray[1] + " " + redArray[2] + " " + redArray[3] + " " + redArray[4] + " " + redArray[5] + " " + redArray[6] + " " + redArray[7] + " " + redArray[8] + " " + redArray[9]);
    Serial.println((String)"Green: " + greenArray[0] + " " + greenArray[1] + " " + greenArray[2] + " " + greenArray[3] + " " + greenArray[4] + " " + greenArray[5] + " " + greenArray[6] + " " + greenArray[7] + " " + greenArray[8] + " " + greenArray[9]);
    Serial.println(" ");
  */

  LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
  delay(100);
}

double calibration(uint32_t rawValueX, uint32_t rawValueY, uint32_t rawValueZ)

//  Get the parameters from the following, in the Example2-I2C_Digital_compass example sketch
//  myMag.getMeasurementXYZ(&rawValueX, &rawValueY, &rawValueZ);
//  I'm assuming the &s should be left out

{
  static float rawValues[3];
  rawValues[0] = (float)rawValueX;
  rawValues[1] = (float)rawValueY;
  rawValues[2] = (float)rawValueZ;

  static float hardIronCorrection[3]
  { -1.16, -16.66, -10.22};

  static float softIronCorrection[3][3]
  { {1.002,  0.007,  -0.004},
    {0.007, 1.005,  0.001},
    { -0.004,  0.001,  0.994}
  };

  static float products[3][4];  //  Used to get and add dot products

  //  Subtract hardIronCorrection from rawValues
  for (int i = 0; i < 3; i++) {
    rawValues[i] = rawValues[i] - hardIronCorrection[i];
  }

  //  Apply softIronCorrection
  for (int i = 0; i < 3; i++) {  //  rows of dot products
    for (int j = 0; j < 4; j++) {  //  3 columns of dot products, and the 4th is the sum of the dot products in a row

      if (j != 3 ) {
        products[i][j] = softIronCorrection[i][j] * rawValues[j];  //  Each element in the first column of the softIronCorrection matrix is multiplied by the element in the first row of the other matrix
        //  Each element in the second column of the softIronCorrection matrix is multiplied by the element in the second row of the other matrix
        //  Each element in the third column of the softIronCorrection matrix is multiplied by the element in the third row of the other matrix
      } else {  //There are three products in a row, and it is time to add them
        products[i][3] = products[i][0] + products[i][1] + products[i][2];
      }

    }
  }

  /*
    Serial.println("Dot products in the same row will be added together");
    for (int i = 0; i < 3; i++) {
      Serial.println("  ");
      for (int j = 0; j < 3; j++) {
        Serial.print(products[i][j]);
        Serial.print("  ");
      }
    }
    Serial.println("  ");

    Serial.println("A 3x1 matrix has a11, a21, and a31 in it");
    Serial.print("a11  ");
    Serial.println(products[0][3]);
    Serial.print("a21  ");
    Serial.println(products[1][3]);
    Serial.print("a31  ");
    Serial.println(products[2][3]);
  */

  //  Use the rest of Example2-I2C_Digital_compass here
  double scaledX = 0;
  double scaledY = 0;
  double scaledZ = 0;
  double heading = 0;

  scaledX = (double)products[0][3] - 131072.0;
  scaledX /= 131072.0;

  scaledY = (double)products[1][3] - 131072.0;
  scaledY /= 131072.0;

  scaledZ = (double)products[2][3] - 131072.0;
  scaledZ /= 131072.0;

  // Magnetic north is oriented with the Y axis
  // Convert the X and Y fields into heading using atan2 (Arc Tangent 2)
  heading = atan2(scaledX, 0 - scaledY);

  // atan2 returns a value between +PI and -PI
  // Convert to degrees
  heading /= PI;
  heading *= 180;
  heading += 180;

  return heading;
}
