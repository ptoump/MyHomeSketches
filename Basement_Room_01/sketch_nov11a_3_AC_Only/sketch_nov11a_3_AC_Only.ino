#include <Arduino.h>
#include <MySensor.h>  
#include <MyTransportNRF24.h>
#include <SPI.h>
#include <Timer.h>
#include <avr/pgmspace.h>
#include <PanasonicHeatpumpIR.h>


#define ACW2_CHILD 2           // Id of the AC Warm 2 hours child
#define ACW6_CHILD 3           // Id of the AC Warm 6 hours child



MyTransportNRF24 radio(7, 8, RF24_PA_MAX);

MySensor gw(radio);

MyMessage acw2Msg(ACW2_CHILD, V_LIGHT);
MyMessage acw6Msg(ACW6_CHILD, V_LIGHT);

IRSender irsender(9);
PanasonicNKEHeatpumpIR *ACIR =  new PanasonicNKEHeatpumpIR();

Timer t;

boolean ac_state = false;


void setup() {


  // Initialize library and add callback for incoming messages
  gw.begin(incomingMessage, AUTO, true);

  // Register all sensors to gw (they will be created as child devices)
  gw.present( ACW2_CHILD, S_LIGHT );
  gw.present( ACW6_CHILD, S_LIGHT );
  gw.sendSketchInfo("AC", "1");

  
}

void loop() {
  // MySensors Process
  gw.process();

  

  // Timer Process
  t.update();

}

void incomingMessage(const MyMessage &message) {

  if (message.isAck()) {
         Serial.println(F("This is an ack from gateway"));
      }
      
  if (message.type == V_LIGHT) {
    boolean state = message.getBool();
    
    if (message.sensor == ACW2_CHILD) {
      
      if(state){
        Serial.println(F("--- received ACW2 - ON"));
     // schedule_auto(6600000);
     //schedule_auto(6000);      
      }
      else
      {
        Serial.println(F("--- received ACW2 - OFF"));
        ac_off();
      }
      
    }
    
    if (message.sensor == ACW6_CHILD) {
      Serial.println(F("--- received ACW6"));
       if(state){
     schedule_quiet(21000000);      
         }
      else
      {
        ac_off();
      }
     }
    }
}


void schedule_auto(int dur) {

   Serial.println(F("--- sending auto"));
   ACIR->send(irsender, POWER_ON, MODE_HEAT, FAN_AUTO, 26, VDIR_MIDDLE, HDIR_AUTO);
   t.after(dur,ac_off);
   ac_state = true;
  
}

void schedule_quiet(int dur) {

   
   Serial.println(F("--- sending quiet"));
   ACIR->send(irsender, POWER_ON, MODE_HEAT, FAN_1, 25, VDIR_MIDDLE, HDIR_AUTO);
   t.after(dur,ac_off);
   ac_state = true;
  
}

void ac_off() {
  Serial.println(F("--- trying off"));
  if (ac_state) 
  {
    Serial.println(F("--- sending off"));
  ACIR->send(irsender, POWER_OFF, MODE_HEAT, FAN_1, 25, VDIR_MIDDLE, HDIR_AUTO);

  ac_state = false;
  gw.send(acw2Msg.set(false), true);
  gw.send(acw6Msg.set(false), true);
  }
 
}


