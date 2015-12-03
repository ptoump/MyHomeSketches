/*
  LedDimmer.h - LedDimmer library for Wiring - implementation
  Copyright (c) 2006 John Doe.  All right reserved.
*/


// include this library's description file
#include "LedDimmer.h"
#include "Arduino.h"

// Uncomment the following line to enable debug
// #define DEBUG_LedDimmer

#ifdef DEBUG_LedDimmer
#define DEBUG_LedDimmer_SERIAL(x) Serial.begin(x)
#define DEBUG_LedDimmer_PRINT(x) Serial.print(x)
#define DEBUG_LedDimmer_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_LedDimmer_SERIAL(x)
#define DEBUG_LedDimmer_PRINT(x) 
#define DEBUG_LedDimmer_PRINTLN(x) 
#endif

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

LedDimmer::LedDimmer(int pin)
{
  // initialize this instance's variables
  led = pin;
  curLevel = 0;
  toLevel = 0;
  step = 0;
  delta = 0;

  // do whatever is required to initialize the library
  pinMode(led, OUTPUT);
  analogWrite(led, curLevel);
}


// Update function.
// Must be invoked inside the loop() !!

void LedDimmer::update(void)
{
  if (curLevel != toLevel) 
  {
    curMillis = millis();
    if (curMillis > (prevMillis + step)) 
    {
      curLevel += delta;
      analogWrite(led, curLevel);
      DEBUG_LedDimmer_PRINT(curLevel);
      DEBUG_LedDimmer_PRINT(", ");
      prevMillis = curMillis;
    }
  }

}



/*******
  Function to calculate time step for smooth fading
  Take notice, that it calculates for 255 total steps
*/
int LedDimmer::calcStep(const int duration, const int to)
{
  return duration / abs( (int)( (to * 255L) / 100. ) - (int)( (curLevel * 255L) / 100. ) );
}

void LedDimmer::dim(const int step_temp, const int to_Level) 
{
  toLevel = to_Level;
  step = step_temp;
  delta = ( toLevel - curLevel ) < 0 ? -1 : 1;
  prevMillis = millis();
  DEBUG_LedDimmer_PRINTLN();
  DEBUG_LedDimmer_PRINT("curLevel : ");
  DEBUG_LedDimmer_PRINT(curLevel);
  DEBUG_LedDimmer_PRINT("  toLevel : ");
  DEBUG_LedDimmer_PRINT(toLevel);
  DEBUG_LedDimmer_PRINT("  step : ");
  DEBUG_LedDimmer_PRINT(step);
  DEBUG_LedDimmer_PRINT("  delta : ");
  DEBUG_LedDimmer_PRINTLN(delta);
}

