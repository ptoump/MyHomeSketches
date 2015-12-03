// #include <IRremote.h>
#include <Arduino.h>
#include <MySensor.h>
#include <MyTransportNRF24.h>
// #include <MyHwATMega328.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Timer.h>
#include <avr/pgmspace.h>
#include <PanasonicHeatpumpIR.h>

#define SN "AC_n_TEMP_bsm"
#define SV "1.3"

#define ONE_WIRE_BUS 5 // Pin where dallase sensor is connected, PIN 3 Is used by IRSend !!!

#define Temp_CHILD 1           // Id of the DHT-temperature child
#define ACW2_CHILD 2           // Id of the AC Warm 2 hours child
#define ACW6_CHILD 3           // Id of the AC Warm 6 hours child
#define Dbgtmp_CHILD 101           // Id of the DHT-temperature child

// Dallas Stuff
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.
DeviceAddress tempDeviceAddress;
const int  resolution = 11; // DS18B20 can set the resolution up to 12 bits

MyTransportNRF24 radio(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_MAX);

MySensor gw(radio);

MyMessage tempMsg(Temp_CHILD, V_TEMP);
MyMessage acw2Msg(ACW2_CHILD, V_LIGHT);
MyMessage acw6Msg(ACW6_CHILD, V_LIGHT);
MyMessage tempdbg(Dbgtmp_CHILD, V_TEMP);

IRSender irsender(3);
PanasonicNKEHeatpumpIR *ACIR = new PanasonicNKEHeatpumpIR();

Timer t;

boolean ac_state = false;
float lastTemp = 0;

// Timer intervals
const long temp_period = 60 * 1000L;
const long ACW2_period = 115 * 60 * 1000L;
const long ACW6_period = 355 * 60 * 1000L;

int Timer_Temp = 0;
int Timer_ACW2 = 1;
int Timer_ACW6 = 2;
int tempCount = 0;


void setup() {


  // Startup up the OneWire library
  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, resolution);

  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures(); //initial reading

  // Initialize library and add callback for incoming messages
  gw.begin(incomingMessage, AUTO, true);

  // Register all sensors to gw (they will be created as child devices)
  gw.present( Temp_CHILD, S_TEMP );
  gw.present( ACW2_CHILD, S_LIGHT );
  gw.present( ACW6_CHILD, S_LIGHT );
  gw.present( Dbgtmp_CHILD, S_TEMP ); // For debbuging temp
  gw.sendSketchInfo(SN, SV);

  // Schedule Temperature update
  read_temp();
  Timer_Temp = t.every( temp_period , read_temp);

}

void loop() {
  // MySensors Process
  gw.process();



  // Timer Process
  t.update();

}

void read_temp() {
  Serial.print(F("reading temperature from Dallas : "));
  sensors.requestTemperatures();  // Request Temperatures
  gw.wait(1000); // asynch wait for conversion
  float temp = sensors.getTempCByIndex(0);
  
  // Debug
  Serial.println(temp);
  gw.send(tempdbg.set(temp, 1)); 
  // endDebug
  
  // Check if temp was read correctly
  if ( (temp < -50) || (temp > 60) ) {  // Ignore obviously wrong readings 
    Serial.println(F("Failed to read temperature from Dallas"));
  } else if ((lastTemp != temp) || (tempCount > 20)) {  // Send an update every now and then
    gw.send(tempMsg.set(temp, 1));
    lastTemp = temp;
    tempCount = 0;
  } else { tempCount++ ; }
}

void incomingMessage(const MyMessage &message) {

  if (message.isAck()) {
    Serial.println(F("This is an ack from gateway"));
  }

  if (message.type == V_LIGHT) {
    boolean state = message.getBool();

    if (message.sensor == ACW2_CHILD) {

      if (state) {
        Serial.println(F("--- received ACW2 - ON"));
        schedule_auto(ACW2_period);
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
      if (state) {
        schedule_quiet(ACW6_period);
      }
      else
      {
        ac_off();
      }

    }
  }
}


void schedule_auto(long dur) {

  Serial.println(F("--- sending auto"));
  ACIR->send(irsender, POWER_ON, MODE_HEAT, FAN_AUTO, 26, VDIR_MIDDLE, HDIR_AUTO);
  Timer_ACW2 = t.after(dur, ac_off);
  ac_state = true;

}

void schedule_quiet(long dur) {

  Serial.println(F("--- sending quiet"));
  ACIR->send(irsender, POWER_ON, MODE_HEAT, FAN_1, 25, VDIR_MIDDLE, HDIR_AUTO);
  Timer_ACW6 = t.after(dur, ac_off);
  ac_state = true;

}

void ac_off() {
  Serial.println(F("--- trying off"));
  if (ac_state)
  {
    Serial.println(F("--- sending off"));
    ACIR->send(irsender, POWER_OFF, MODE_HEAT, FAN_1, 25, VDIR_MIDDLE, HDIR_AUTO);
    ac_state = false;
    if (Timer_Temp != Timer_ACW2) {  t.stop(Timer_ACW2);  }
    if (Timer_Temp != Timer_ACW6) {  t.stop(Timer_ACW6);  }
    gw.send(acw2Msg.set(false), true);
    gw.wait(200);
    gw.send(acw6Msg.set(false), true);
  }

}


