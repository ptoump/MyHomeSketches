#define SN "AC_TEMP_Dim_Kitchen"
#define SV "1.0"

#include <Arduino.h>
#include <MySensor.h> 
#include <SPI.h>
#include <MyTransportNRF24.h>
#include <MyHwATMega328.h>
#include <LedDimmer.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Timer.h>
#include <avr/pgmspace.h>
#include <PanasonicHeatpumpIR.h>

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

#define ONE_WIRE_BUS 5 // Pin where dallase sensor is connected, PIN 3 Is used by IRSend !!!

#define LED1_PIN 9      // Arduino pin attached to MOSFET Gate pin

#define Temp_CHILD 1    // Id of the DHT-temperature child
#define ACW2_CHILD 2    // Id of the AC Warm 2 hours child
#define ACW6_CHILD 3    // Id of the AC Warm 6 hours child
#define ACC2_CHILD 6    // Id of the AC Cold 2 hours child
#define LED1_CHILD 11   // Id of 1st LED strip child

// Take note of the CE,CS PINS !!!
MyTransportNRF24 transport(7, 8, RF24_PA_MAX);
MyHwATMega328 hw;
MySensor gw(transport, hw);

// Led dimming stuff
const int fade_period = 3 * 1000;     // fade time in ms
LedDimmer dim1(LED1_PIN);

// Dallas Stuff
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.
DeviceAddress tempDeviceAddress;
const int  resolution = 11; // DS18B20 can set the resolution up to 12 bits

MyMessage tempMsg(Temp_CHILD, V_TEMP);
MyMessage acw2Msg(ACW2_CHILD, V_LIGHT);
MyMessage acw6Msg(ACW6_CHILD, V_LIGHT);
MyMessage acc2Msg(ACC2_CHILD, V_LIGHT);
MyMessage dim1Msg(LED1_CHILD, V_DIMMER);

// HVAC Stuff
IRSender irsender(3);
PanasonicNKEHeatpumpIR *ACIR = new PanasonicNKEHeatpumpIR();

Timer t;

boolean ac_state = false;
float lastTemp = 0;

// Timer intervals
// Using timers for HVAC as a failsafe to 
// turn off the HVAC in case of network failure
const long temp_period = 60 * 1000L;
const long ACW2_period = 115 * 60 * 1000L;
const long ACW6_period = 355 * 60 * 1000L;
const long ACC2_period = 115 * 60 * 1000L;

int Timer_Temp = 0;
int Timer_ACW2 = 1;
int Timer_ACW6 = 2;
int Timer_ACC2 = 3;
int tempCount = 0;

/***
 * Dimmable LED initialization method
 */
 
void setup()  
{ 
  DEBUG_Local_SERIAL(115200);    
  DEBUG_Local_PRINTLN("Serial started");
   
  // Startup up the OneWire library
  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, resolution);

  gw.begin(incomingMessage, AUTO, true);
  gw.sendSketchInfo(SN, SV);
    
  // Register the LED Dimmable Light with the gateway
  gw.present( Temp_CHILD, S_TEMP );
  gw.present( ACW2_CHILD, S_LIGHT );
  gw.present( ACW6_CHILD, S_LIGHT );
  gw.present( ACC2_CHILD, S_LIGHT );
  gw.present( LED1_CHILD, S_DIMMER );

  // Schedule Temperature update
  read_temp();
  Timer_Temp = t.every( temp_period , read_temp);
      
  // Pull the gateway's current dim level - restore light level upon sendor node power-up
  gw.request( LED1_CHILD, V_DIMMER );
  
}

/***
 *  Dimmable LED main processing loop 
 */
void loop() 
{
  // MySensors Process
  gw.process();
  // Timer Process
  t.update();
  // Dimmer Process
  dim1.update();
}



void incomingMessage(const MyMessage &message) {
  if (message.type == V_LIGHT || message.type == V_DIMMER) {

    boolean state = message.getBool();

    if (message.sensor == ACW2_CHILD) {

      if (state) {
        DEBUG_Local_PRINTLN(F("--- received ACW2 - ON"));
        schedule_auto(ACW2_period);
        //schedule_auto(6000);
      }
      else
      {
        DEBUG_Local_PRINTLN(F("--- received ACW2 - OFF"));
        ac_off();
      }

    }

    if (message.sensor == ACW6_CHILD) {
      DEBUG_Local_PRINTLN(F("--- received ACW6"));
      if (state) {
        schedule_quiet(ACW6_period);
      }
      else
      {
        ac_off();
      }

    } // EndIf ACW2

    if (message.sensor == ACC2_CHILD) {
      DEBUG_Local_PRINTLN(F("--- received ACC2"));
      if (state) {
        schedule_cold(ACC2_period);
      }
      else
      {
        ac_off();
      }

    } // EndIf ACC2

    if (message.sensor == LED1_CHILD) {

      //  Retrieve the power or dim level from the incoming request message
      int requestedLevel = atoi( message.data );
      
      // Adjust incoming level if this is a V_LIGHT variable update [0 == off, 1 == on]
      requestedLevel *= ( message.type == V_LIGHT ? 100 : 1 );
      
      // Clip incoming level to valid range of 0 to 100
      requestedLevel = requestedLevel > 100 ? 100 : requestedLevel;
      requestedLevel = requestedLevel < 0   ? 0   : requestedLevel;
      
      DEBUG_Local_PRINT( "Changing led level to " );
      DEBUG_Local_PRINTLN( requestedLevel );
    
      dim1.dim(dim1.calcStep(fade_period, requestedLevel), requestedLevel); 
    } 
  }  // ENDIF messagetype
} 
 
void schedule_auto(long dur) {

  DEBUG_Local_PRINTLN(F("--- sending auto"));
  ACIR->send(irsender, POWER_ON, MODE_HEAT, FAN_5, 28, VDIR_UP, HDIR_AUTO);
  Timer_ACW2 = t.after(dur, ac_off);
  ac_state = true;

}

void schedule_quiet(long dur) {

  DEBUG_Local_PRINTLN(F("--- sending quiet"));
  ACIR->send(irsender, POWER_ON, MODE_HEAT, FAN_AUTO, 25, VDIR_UP, HDIR_AUTO);
  Timer_ACW6 = t.after(dur, ac_off);
  ac_state = true;

}

void schedule_cold(long dur) {

  DEBUG_Local_PRINTLN(F("--- sending quiet"));
  ACIR->send(irsender, POWER_ON, MODE_COOL, FAN_AUTO, 27, VDIR_UP, HDIR_AUTO);
  Timer_ACC2 = t.after(dur, ac_off);
  ac_state = true;

}

void ac_off() {
  DEBUG_Local_PRINTLN(F("--- trying off"));
  if (ac_state)
  {
    DEBUG_Local_PRINTLN(F("--- sending off"));
    ACIR->send(irsender, POWER_OFF, MODE_HEAT, FAN_1, 25, VDIR_UP, HDIR_AUTO);
    ac_state = false;
    if (Timer_Temp != Timer_ACW2) {  t.stop(Timer_ACW2);  }
    if (Timer_Temp != Timer_ACW6) {  t.stop(Timer_ACW6);  }
    if (Timer_Temp != Timer_ACC2) {  t.stop(Timer_ACC2);  }
    gw.send(acw2Msg.set(false), true);
    gw.send(acw6Msg.set(false), true);
    gw.send(acc2Msg.set(false), true);
  }

}

void read_temp() {
  DEBUG_Local_PRINT(F("reading temperature from Dallas : "));
  sensors.requestTemperatures();  // Request Temperatures
  gw.wait(1000); // asynch wait for conversion
  float temp = sensors.getTempCByIndex(0);
  
  DEBUG_Local_PRINTLN(temp);
  
  // Check if temp was read correctly
  if ( (temp < -50) || (temp > 60) ) {  // Ignore obviously wrong readings 
    DEBUG_Local_PRINTLN(F("Failed to read temperature from Dallas"));
  } else if ((lastTemp != temp) || (tempCount > 20)) {  // Send an update every now and then
    gw.send(tempMsg.set(temp, 1));
    lastTemp = temp;
    tempCount = 0;
  } else { tempCount++ ; }
}
