/******************************************************************************
  Modified by Jason (a beginner) from

  owennewo’s “quaternion_yaw_pitch_roll.ino” and
  Ciara Jekel’s “An I2C based LED Stick” and
  Fischer Moseley’s “Example1_PrintButtonStatus”

  Uses a compass to control LEDs on a dance costume, without a chase effect and control conditions for psychology, which were included in a previous version

  Sometimes it freezes, and indicates the wrong direction

  A link to a YouTube video of how it can be used for recreation therapy is https://youtu.be/e3njbTbl6-I

  Date:  April 8, 2024
******************************************************************************/

#include <Arduino.h>
#define Serial SerialUSB        // Might have to be done with the microcontroller I am using.  Might cause problems with other microcontrollers
#include <SparkFun_Qwiic_Button.h>
#include <Adafruit_BNO08x.h>
#include <Wire.h>
#include "Qwiic_LED_Stick.h"    // Click here to get the library: http://librarymanager/All#SparkFun_Qwiic_LED_Stick
QwiicButton button;
LED LEDStick;                   // Create an object of the LED class
const int numLEDs = 10;         // Number of LEDs on the stick
const int d = 16;               // Dims all of the LEDs on the stick
int numClicks = 0;              // How many times the button has been pressed
int calStat = 0;                // How well it has been calibrated, up to 3
int myCal = 998;                // Used to keep track of whether a first direction has been stored.  It is reset in case 9 of the switch statement
int newCal = 998;               // Same as above, for a second direction
int myRecAdjust = 0;            // Makes the number of LEDs that are on depend on a different direction
int newRecAdjust = 0;           // Same as above, for another direction
int num = 0;                    // The number of LEDs that will be on.  It is a parameter passed to the ledController function
int calActive = 0;              // I don’t know whether this is needed to keep corrections updated
int calDone = 998;              // Used to keep track of whether it is done calibrating.  Reset in case 9 of the switch statement
int chaseDone = 998;            // Used to keep track of whether the chase effect (in case 2 of the switch statement) is done.  Reset in case 9
int randomDone = 998;           // Used to keep track of whether the random number generator is done controlling the LEDs.  Reset in case 9 of the switch statement
byte greenArray[numLEDs];       // Controls how green each of the LEDs get
byte redArray[numLEDs];         // Controls how red each of the LEDs get
byte blueArray[numLEDs];        // Controls how blue each of the LEDs get
int randNum = 0;                // Stores results from random number generator
int chaseRandomDelay = 55;      // How long LEDs are on and off during the chase effect, and when the random number generator is used to control the LEDs
const int setLEDdelay = 30;     // This might help time communication between the getNum and ledController functions

// For SPI mode, we need a CS pin
#define BNO08X_CS 10
#define BNO08X_INT 9

// #define FAST_MODE

// For SPI mode, we also need a RESET
//#define BNO08X_RESET 5
// but not for I2C or UART
#define BNO08X_RESET -1

struct euler_t {
  float yaw;
  float pitch;
  float roll;
} ypr;

Adafruit_BNO08x  bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

#ifdef FAST_MODE
// Top frequency is reported to be 1000Hz (but freq is somewhat variable)
sh2_SensorId_t reportType = SH2_GYRO_INTEGRATED_RV;
long reportIntervalUs = 2000;
#else
// Top frequency is about 250Hz but this report is more accurate
sh2_SensorId_t reportType = SH2_ARVR_STABILIZED_RV;
long reportIntervalUs = 5000;
#endif
void setReports(sh2_SensorId_t reportType, long report_interval) {
  Serial.println("Setting desired reports");
  if (! bno08x.enableReport(reportType, report_interval)) {
    Serial.println("Could not enable stabilized remote vector");
  }
}

void setup() {
  delay(4000);
  Serial.begin(115200);
  Serial.println("Qwiic button examples");
  Wire.begin(); //Join I2C bus
  //check if button will acknowledge over I2C
  if (button.begin() == false) {
    Serial.println("Device did not acknowledge! Freezing.");

    //    Might have to be made into a comment because the batteries don’t have a serial monitor
    //    while (1);
  }
  Serial.println("Button acknowledged.");

  // Might have to be made into a comment because the batteries don’t have a serial monitor
  //  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit BNO08x test!");

  // Try to initialize!
  if (!bno08x.begin_I2C()) {
    //if (!bno08x.begin_UART(&Serial1)) {  // Requires a device with > 300 byte UART buffer!
    //if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");

    /*
      Might have to be made into a comment because the batteries don’t have a serial monitor
        while (1) {
          delay(10);
        }
    */

  }
  Serial.println("BNO08x Found!");

  setReports(reportType, reportIntervalUs);

  Serial.println("Reading events");
  delay(100);

  //Start up communication with the LED Stick
  if (LEDStick.begin() == false) {

    Serial.println("Qwiic LED Stick failed to begin. Please check wiring and try again!");
    // Might have to be made into a comment because the batteries don’t have a serial monitor
    //    while (1);
  }

  Serial.println("Qwiic LED Stick ready!");
}

void quaternionToEuler(float qr, float qi, float qj, float qk, euler_t* ypr, bool degrees = false) {

  float sqr = sq(qr);
  float sqi = sq(qi);
  float sqj = sq(qj);
  float sqk = sq(qk);

  ypr->yaw = atan2(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));
  ypr->pitch = asin(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr));
  ypr->roll = atan2(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));

  if (degrees) {
    ypr->yaw *= RAD_TO_DEG;
    ypr->pitch *= RAD_TO_DEG;
    ypr->roll *= RAD_TO_DEG;
  }
}

void quaternionToEulerRV(sh2_RotationVectorWAcc_t* rotational_vector, euler_t* ypr, bool degrees = false) {
  quaternionToEuler(rotational_vector->real, rotational_vector->i, rotational_vector->j, rotational_vector->k, ypr, degrees);
}

void quaternionToEulerGI(sh2_GyroIntegratedRV_t* rotational_vector, euler_t* ypr, bool degrees = false) {
  quaternionToEuler(rotational_vector->real, rotational_vector->i, rotational_vector->j, rotational_vector->k, ypr, degrees);
}

void loop() {
  if (button.isPressed() == true && numClicks < 2) {            // Keeps track of number of button presses
    numClicks += 1;
  }
  while (button.isPressed() == true)
    delay(10);  //wait for user to stop pressing
  //  Serial.print(numClicks);  Serial.println(" clicks");
  delay(20); //Don't hammer too hard on the I2C bus

  switch (numClicks) {
    case 0:
      Serial.println("case 0 is some LEDs off and calibrate");
      for (int i = 0; i < 10; i++) {
        calActive = getNum();
        redArray[i] = 0;
        greenArray[i] = 0;
        blueArray[i] = 0;
      }
      blueArray[5] = 255;                                       // Try to stop viewers from being bored, before the neat stuff happens
      delay(140);
      LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);

      Serial.println("case 0 is also calibrate");
      if (calDone == 998) {
        for (int i = 0; i < 10; i++) {
          calActive = getNum();
          redArray[i] = 10 * 25 / d;                            // LEDs are red until calibrated, I think
        }
        LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
        while (calStat != 3) {
          calActive = getNum();
          Serial.print("calStat is ");  Serial.print(calStat);  Serial.println (" when in while loop");
        }
        for (int i = 0; i < 10; i++) {
          calActive = getNum();
          redArray[i] = 0;
          greenArray[i] = 10 * 25 / d;                          // Green LEDs indicate calibration is done
        }
        LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
        delay (2500);
        for (int i = 0; i < 10; i++) {
          calActive = getNum();
          greenArray[i] = 0;                                    // Turns green LEDs off
        }
        LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
        calDone = 999;
      }
      calActive = getNum();
      Serial.print("calStat is ");  Serial.print(calStat);  Serial.println (" when out of loop");
      break;
    /*
        case 2:
          Serial.println("case 2 is chase effect");                 // Not needed
          if (chaseDone == 998) {
            for (int i = 0; i < 2; i++) {                           // Number of times LEDs go up and down on the stick
              calActive = getNum();
              for (int j = 0; j < 10; j++) {                        // LEDs go up on stick
                calActive = getNum();
                greenArray[j] = 10 * 25 / d;
                LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
                delay(chaseRandomDelay);
                calActive = getNum();
                greenArray[j] = 0;
                LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
                delay(chaseRandomDelay);
              }
              for (int k = 8; k >= 1; k--) {                        // LEDs go down on stick
                calActive = getNum();
                greenArray[k] = 10 * 25 / d;
                LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
                delay(chaseRandomDelay);
                calActive = getNum();
                greenArray[k] = 0;
                LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
                delay(chaseRandomDelay);
              }
            }
            chaseDone = 999;
          }
          break;
    */

    case 1:
      Serial.println("case 1 is MyRec");

      /*
        If myCal equals 999, myRecAdjust has been gotten
        getNum() uses yaw at first.  180 is added to yaw to get heading
        Only a quarter of a circle makes LEDs turn on
        When in the middle of that quarter, all 10 LEDs are on
        When to about one end of that quarter, all are off.
        So, 10 possibilities are there
        When to the other end of that quarter, there are another 10 possibilities
        So in that quarter there are about 20 possibilities
        To use all 4 quarters, 4 * 20 equals 80.  4.5 is used in getNum() because 360 / 4.5 equals 80
        All 10 LEDs will be on when num equals 40
        And all 10 LEDs will be off when num equals about 30 or about 50
        That is why 30, 40, and 50 are used in the next if statement
        Notice that 50 – 30 equals 20, the number of possibilities in the quarter
        However, myRecAdjust might change what num equals, because getNum() is called once in the second if statement
        It will change it if south isn’t the direction the dance is about
      */

      if (myCal == 999) {
        num = getNum() + myRecAdjust;
        if (num < 30 || num > 50) {
          num = 0;
        } else if (num >= 30 && num <= 40) {
          num = num - 30;
        } else if (num > 40 && num <= 50) {
          num = 50 - num;
        }
        Serial.print("calStat is "); Serial.print(calStat); Serial.print(" and num is ");  Serial.println(num);
        ledController (num);
      }

      else if (myCal == 998) {
        myRecAdjust = getNum();
        myRecAdjust = 40 - myRecAdjust;
        myCal = 999;
      }
      break;

    /*
        case 4:
          Serial.println("case 4 is off");          // Not needed
          for (int i = 0; i < 10; i++) {
            calActive = getNum();
            redArray[i] = 0;
            greenArray[i] = 0;
            blueArray[i] = 0;
          }
          delay(140);
          LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
          //      delay(100);
          break;

        case 5:
          Serial.println("case 5 is NewDir");       // This is like case 3 - not needed
          if (newCal == 999) {

            num = getNum() + newRecAdjust;
            if (num < 30 || num > 50) {
              num = 0;
            } else if (num >= 30 && num <= 40) {
              num = num - 30;
            } else if (num > 40 && num <= 50) {
              num = 50 - num;
            }
            Serial.print("calStat is "); Serial.print(calStat); Serial.print(" and num is ");  Serial.println(num);
            ledController (num);
          }
          else if (newCal == 998) {
            newRecAdjust = getNum();
            newRecAdjust = 40 - newRecAdjust;
            newCal = 999;
          }
          break;
        case 6:
          Serial.println("case 6 is off");           // Not needed
          for (int i = 0; i < 10; i++) {
            calActive = getNum();
            redArray[i] = 0;
            greenArray[i] = 0;
            blueArray[i] = 0;
          }
          delay(140);
          LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
          //      delay(100);
          break;

        case 7:
          Serial.println("case 7 is random");        // Not needed
          if (randomDone == 998) {
            for (int i = 0; i < 46; i++) {
              calActive = getNum();
              randNum = random(10);       // This is the random number generator
              greenArray[randNum] = 10 * 25 / d;
              LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
              delay (chaseRandomDelay);
              calActive = getNum();
              greenArray[randNum] = 0;
              LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
              delay (chaseRandomDelay);
            }
            randomDone = 999;
          }
          break;
        case 8:
          Serial.println("case 8 is MyRec, again");       // This uses what was gotten in case 3 - not needed
          num = getNum() + myRecAdjust;
          if (num < 30 || num > 50) {
            num = 0;
          } else if (num >= 30 && num <= 40) {
            num = num - 30;
          } else if (num > 40 && num <= 50) {
            num = 50 - num;
          }
          Serial.print("calStat is "); Serial.print(calStat); Serial.print(" and num is ");  Serial.println(num);
          ledController (num);
          break;
    */
    case 2:
      Serial.println("case 9 is reset, turns LEDs blue");       // Blue LEDs indicate it got ready to start over if uncommented below
      myCal = 998;
      newCal = 998;
      calDone = 998;
      chaseDone = 998;
      randomDone = 998;
      for (int i = 0; i < 10; i++) {
        calActive = getNum();
        redArray[i] = 0;
        greenArray[i] = 0;
        blueArray[i] = 0;
        // blueArray[i] = 10 * 25 / d;
      }
      delay(140);
      LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);
      //      delay(100);
      numClicks = 0;                                              // Resets number of button presses
      break;
  }
}

int getNum () {
  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports(reportType, reportIntervalUs);
  }

  if (bno08x.getSensorEvent(&sensorValue)) {
    // in this demo only one report type will be received depending on FAST_MODE define (above)
    switch (sensorValue.sensorId) {
      case SH2_ARVR_STABILIZED_RV:
        quaternionToEulerRV(&sensorValue.un.arvrStabilizedRV, &ypr, true);
      case SH2_GYRO_INTEGRATED_RV:
        // faster (more noise?)
        quaternionToEulerGI(&sensorValue.un.gyroIntegratedRV, &ypr, true);
        break;
    }
    static long last = 0;
    long now = micros();
    //    Serial.print(now - last);             Serial.print("\t");
    last = now;
    //    Serial.print(sensorValue.status);     Serial.print("\t");  // This is accuracy in the range of 0 to 3

    calStat = sensorValue.status;                                    // See the comment before this one
    num = ypr.yaw;
    num += 180;          // Turns yaw into heading
    num /= 4.5;          // Turns heading into a number system described in a comment in case 3 of the switch statement
    //  Serial.println(angle);
    delay(setLEDdelay);
    return num;

    //    Serial.println(ypr.yaw);              Serial.print("\t");
    //    Serial.print(ypr.pitch);              Serial.print("\t");
    //    Serial.println(ypr.roll);
  }
}

void ledController (int i) {                        // i is the number of LEDs to turn on

  for (int j = 0; j < 10; j++) {
    blueArray[j] = 0;                               // turn all blue lights off
  }

  //  Serial.print("numLEDs = ");
  //  Serial.println(i);

  if (i % 2 == 0 && i != 10) {                      // All LEDs are off, or there is an even number of LEDs that are on.  LEDs that are on will be in the middle of the stick.
    // I don't know why 10 doesn't work here.
    for (int j = 0; j < 5 - i / 2; j++) {

      redArray[j] = 0;                              // Turns the LED(s) to the left of the stick off
      redArray[9 - j] = 0;                          // Turns the LED(s) to the right off
      greenArray[j] = 0;
      greenArray[9 - j] = 0;
    }
    for (int j = 5 - i / 2; j < 5 + i / 2; j++) {   // Turns the LEDs in the middle on
      redArray[j] = (10 - i) * 25 / d;              // d dims
      greenArray[j] = i * 25 / d;
    }

  } else if (i != 9 && i != 10) {                    // I don't know why 9 doesn't work here.

    /*
              Unless there is only one LED on, LEDs which are on will be together
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
      redArray[j] = (10 - i) * 25 / d;               // d dims
      greenArray[j] = i * 25 / d;
    }

  } else if (i == 9) {                                // I don't know why 9 doesn't work when i%2 == 1
    redArray[9] = 0;
    greenArray[9] = 0;
    redArray[0] = (10 - i) * 25 / d;
    greenArray[0] = i * 25 / d;
    for (int j = 1; j < 9; j++) {
      redArray[j] = (10 - i) * 25 / d;
      greenArray[j] = i * 25 / d;
      LEDStick.setLEDColor(redArray, greenArray, blueArray, numLEDs);  //  Why do I need this here also, in addition to below?
      //  delay(setLEDdelay);
    }

  } else if (i == 10) {                                // I don't know why 10 doesn't work when i%2 == 0
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
  delay(setLEDdelay);
}
