#define SN "DimLED_x2_bsm"
#define SV "1.1"

#include <MySensor.h> 
#include <SPI.h>
#include <MyTransportNRF24.h>

#define LED1_PIN 9      // Arduino pin attached to MOSFET Gate pin
#define LED2_PIN 10      // Arduino pin attached to MOSFET Gate pin
// #define FADE_DELAY 30  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)

#define LED1_CHILD 0           // Id of 1st LED strip child
#define LED2_CHILD 1           // Id of 2nd LED strip child

MyTransportNRF24 transport(7, 8, RF24_PA_MAX);
MySensor gw(transport);

const int ledpins[] = {LED1_PIN,LED2_PIN};
int currentLevel[] = {0,0};           // Current dim level...
const int fade_period = 3 * 1000;     // fade time in ms

MyMessage dim1Msg(LED1_CHILD, V_DIMMER);
MyMessage dim2Msg(LED2_CHILD, V_DIMMER);


/***
 * Dimmable LED initialization method
 */
 
void setup()  
{ 
  Serial.println( SN ); 
  for (int i = 0; i < 2 ; i++) {
    pinMode(ledpins[i],OUTPUT);
    analogWrite( ledpins[i], 0 );
  }
   
  gw.begin( incomingMessage );
  gw.sendSketchInfo(SN, SV);
    
  // Register the LED Dimmable Light with the gateway
  gw.present( LED1_CHILD, S_DIMMER );
  gw.present( LED2_CHILD, S_DIMMER );
    
  // Pull the gateway's current dim level - restore light level upon sendor node power-up
  gw.request( LED1_CHILD, V_DIMMER );
  gw.request( LED2_CHILD, V_DIMMER );
}

/***
 *  Dimmable LED main processing loop 
 */
void loop() 
{
  gw.process();
}



void incomingMessage(const MyMessage &message) {
  if (message.type == V_LIGHT || message.type == V_DIMMER) {

    int led2dim = message.sensor ;
    
    //  Retrieve the power or dim level from the incoming request message
    int requestedLevel = atoi( message.data );
    
    // Adjust incoming level if this is a V_LIGHT variable update [0 == off, 1 == on]
    requestedLevel *= ( message.type == V_LIGHT ? 100 : 1 );
    
    // Clip incoming level to valid range of 0 to 100
    requestedLevel = requestedLevel > 100 ? 100 : requestedLevel;
    requestedLevel = requestedLevel < 0   ? 0   : requestedLevel;
    
    Serial.print( "Changing led no [ ");
    Serial.print( led2dim );
    Serial.print( " ] level to " );
    Serial.print( requestedLevel );
    Serial.print( ", from " ); 
    Serial.println( currentLevel[led2dim] );

    fadeToLevel( led2dim, requestedLevel );
    
    // Inform the gateway of the current DimmableLED's SwitchPower1 and LoadLevelStatus value...
    // gw.send(lightMsg.set(currentLevel > 0 ? 1 : 0));

    /***  
     *   this is in case something goes wrong
     *   and for future compatibility with encoders
     */
     
    switch (led2dim) {
      case 0:
            gw.send( dim1Msg.set(currentLevel[0]) );
      break;

      case 1:
           gw.send( dim2Msg.set(currentLevel[1]) );

      break;
    }
    }


    
    }


/***
 *  This method provides a graceful fade up/down effect
 *  In this implementation it uses a constant fade period
 */
 
void fadeToLevel(int ledno, int toLevel ) {

  int delta = ( toLevel - currentLevel[ledno] ) < 0 ? -1 : 1;

  // Calculate fade delay
  int fade_delay = fade_period / abs( toLevel - currentLevel[ledno] );
 
  while ( currentLevel[ledno] != toLevel ) {
    currentLevel[ledno] += delta;
    analogWrite( ledpins[ledno], (int)( (currentLevel[ledno] * 255L) / 100. ) );
    gw.wait( fade_delay );
  }
}

