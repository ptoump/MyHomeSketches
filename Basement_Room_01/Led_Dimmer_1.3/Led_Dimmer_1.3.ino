#define SN "DimLED_x2_bsm"
#define SV "1.3"

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
boolean fading[] = {false,false};


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

    if ( !fading[led2dim] ) {
    fadeToLevel( led2dim, requestedLevel );
    }
    
    
    
    
    /***  
     *   this is in case a new request is made before the old one is over 
     *   and for future compatibility with encoders
     */
     
    switch (led2dim) {
      case 0:
         if ( !fading[led2dim] ) { 
          gw.send( dim1Msg.set(currentLevel[0]) ); 
          }
      break;

      case 1:
         if ( !fading[led2dim] ) { 
          gw.send( dim1Msg.set(currentLevel[1]) ); 
          }

      break;
    } //*/
    }  // ENDIF messagetype


    
    }

/***
 *  This method provides a graceful fade up/down effect
 *  In this implementation it uses a constant fade period
 *  and 1/255 steps
 */
 
void fadeToLevel(int ledno, int toLevel ) {

  if ((toLevel != currentLevel[ledno]) && ( !fading[ledno] )) {

  fading[ledno] = true ;  
  int delta = ( toLevel - currentLevel[ledno] ) < 0 ? -1 : 1;
  int fade_target = (int)( (toLevel * 255L) / 100. );
  int fade_current = (int)( (currentLevel[ledno] * 255L) / 100. );
  /*/ Debug
  Serial.print( "delta : ");Serial.print(delta);
  Serial.print( " fade_target : ");Serial.print(fade_target);
  Serial.print( " fade_current : ");Serial.print(fade_current); */
  // Calculate fade delay
  int fade_delay = fade_period / abs( fade_target - fade_current );
  //Serial.print( " fade_delay : ");Serial.println(fade_delay);Serial.print( " fade_current : ");
  
      while (( fade_current != fade_target )) {
        fade_current += delta;
        analogWrite( ledpins[ledno], fade_current );
        // Serial.print(fade_current);Serial.print(", ");
        currentLevel[ledno] = (int)( (fade_current * 100L) / 255. );
        gw.wait( fade_delay ); 
      }
  
  // Serial.println(fade_current);
  // Serial.print( " fade_current : ");Serial.println(currentLevel[ledno]);
  fading[ledno] = false ;
      
  }

 
  
}

