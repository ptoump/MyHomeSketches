#define SN "DimLED_x2_bsm"
#define SV "1.4"

#include <MySensor.h> 
#include <SPI.h>
#include <MyTransportNRF24.h>
#include <MyHwATMega328.h>
#include <LedDimmer.h>

// Uncomment the following line to enable debug
// #define DEBUG_Local 

#ifdef DEBUG_Local
#define DEBUG_Local_SERIAL(x) Serial.begin(x)
#define DEBUG_Local_PRINT(x) Serial.print(x)
#define DEBUG_Local_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_Local_SERIAL(x)
#define DEBUG_Local_PRINT(x) 
#define DEBUG_Local_PRINTLN(x) 
#endif

#define LED1_PIN 9      // Arduino pin attached to MOSFET Gate pin
#define LED2_PIN 10      // Arduino pin attached to MOSFET Gate pin

#define LED1_CHILD 0           // Id of 1st LED strip child
#define LED2_CHILD 1           // Id of 2nd LED strip child

MyTransportNRF24 transport(7, 8, RF24_PA_MAX);
MyHwATMega328 hw;
MySensor gw(transport, hw);

const int ledpins[] = {LED1_PIN,LED2_PIN};
const int fade_period = 3 * 1000;     // fade time in ms

LedDimmer dim1(ledpins[0]);
LedDimmer dim2(ledpins[1]);

MyMessage dim1Msg(LED1_CHILD, V_DIMMER);
MyMessage dim2Msg(LED2_CHILD, V_DIMMER);


/***
 * Dimmable LED initialization method
 */
 
void setup()  
{ 
  DEBUG_Local_SERIAL(115200);    
  DEBUG_Local_PRINTLN("Serial started");
   
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
  dim1.update();
  dim2.update();
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
    
    DEBUG_Local_PRINT( "Changing led no [ ");
    DEBUG_Local_PRINT( led2dim );
    DEBUG_Local_PRINT( " ] level to " );
    DEBUG_Local_PRINTLN( requestedLevel );
    
     
    switch (led2dim) {
      case 0:
          dim1.dim(dim1.calcStep(fade_period, requestedLevel), requestedLevel); 
      break;

      case 1:
          dim2.dim(dim2.calcStep(fade_period, requestedLevel), requestedLevel);
      break;
    } //*/
  }  // ENDIF messagetype
}
 
