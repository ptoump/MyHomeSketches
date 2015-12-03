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

#define SN "AC_n_TEMP_Attic"
#define SV "1.5"

#define ONE_WIRE_BUS 5 // Pin where dallas sensor is connected, PIN 3 Is used by IRSend !!!

// AirCon Defaults

#define ACWA_TEMP   26          // 16..30
#define ACWA_FAN    FAN_AUTO    // FAN_AUTO, FAN_1 .. FAN_5
#define ACWA_MODE   MODE_HEAT   // MODE_AUTO,MODE_COOL, MODE_HEAT, MODE_DRY, MODE_FAN, MODE_OFF
#define ACWA_VS     VDIR_MIDDLE // VDIR_AUTO, VDIR_UP, VDIR_MUP, VDIR_MIDDLE, VDIR_MDOWN, VDIR_DOWN
#define ACWA_HS     HDIR_AUTO   // HDIR_AUTO, HDIR_LEFT .. HDIR_RIGHT

#define ACWL_TEMP   26
#define ACWL_FAN    FAN_1 
#define ACWL_MODE   MODE_HEAT
#define ACWL_VS     VDIR_MIDDLE
#define ACWL_HS     HDIR_AUTO  

#define ACAA_TEMP   26
#define ACAA_FAN    FAN_AUTO 
#define ACAA_MODE   MODE_AUTO
#define ACAA_VS     VDIR_AUTO
#define ACAA_HS     HDIR_AUTO  

#define ACCA_TEMP   28
#define ACCA_FAN    FAN_AUTO 
#define ACCA_MODE   MODE_COOL
#define ACCA_VS     VDIR_UP
#define ACCA_HS     HDIR_AUTO  

#define ACCL_TEMP   28
#define ACCL_FAN    FAN_1
#define ACCL_MODE   MODE_COOL
#define ACCL_VS     VDIR_UP
#define ACCL_HS     HDIR_AUTO  

// Children Defaults

#define Temp_CHILD 1           // Id of the DHT-temperature child
#define ACWA_CHILD 2           // Id of the AC Warm 2 hours child
#define ACWL_CHILD 3           // Id of the AC Warm 6 hours child
#define ACAA_CHILD 4
#define ACCA_CHILD 5
#define ACCL_CHILD 6

// Dallas Stuff
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.
DeviceAddress tempDeviceAddress;
const int  resolution = 11; // DS18B20 can set the resolution up to 12 bits

MyTransportNRF24 radio(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_MAX);

MySensor gw(radio);

MyMessage tempMsg(Temp_CHILD, V_TEMP);
MyMessage acwaMsg(ACWA_CHILD, V_LIGHT);
MyMessage acwlMsg(ACWL_CHILD, V_LIGHT);
MyMessage acaaMsg(ACAA_CHILD, V_DIMMER);
MyMessage accaMsg(ACCA_CHILD, V_LIGHT);
MyMessage acclMsg(ACCL_CHILD, V_LIGHT);


IRSender irsender(3);
PanasonicNKEHeatpumpIR *ACIR = new PanasonicNKEHeatpumpIR();

Timer t;

boolean ac_state = false;
float lastTemp = 0;

// Timer intervals
const long temp_period = 60 * 1000L;

int Timer_Temp = 0;
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
  gw.present( ACWA_CHILD, S_LIGHT );
  gw.present( ACWL_CHILD, S_LIGHT );
  gw.present( ACAA_CHILD, S_DIMMER );
  gw.present( ACCA_CHILD, S_LIGHT );
  gw.present( ACCL_CHILD, S_LIGHT );
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

    if (!state) {
      ACIR->send(irsender, POWER_OFF, MODE_AUTO, FAN_AUTO, 26, VDIR_AUTO, HDIR_AUTO);
      ac_state = false;
      }
      else
      {
        switch (message.sensor) {
            case ACWA_CHILD :
              ACIR->send(irsender, POWER_ON, ACWA_MODE, ACWA_FAN, ACWA_TEMP, ACWA_VS, ACWA_HS);
              ac_state = true;
              break;
              case ACWL_CHILD :
              ACIR->send(irsender, POWER_ON, ACWL_MODE, ACWL_FAN, ACWL_TEMP, ACWL_VS, ACWL_HS);
              ac_state = true;
              break;
              case ACAA_CHILD :
              ACIR->send(irsender, POWER_ON, ACAA_MODE, ACAA_FAN, ACAA_TEMP, ACAA_VS, ACAA_HS);
              ac_state = true;
              break;
              case ACCA_CHILD :
              ACIR->send(irsender, POWER_ON, ACCA_MODE, ACCA_FAN, ACCA_TEMP, ACCA_VS, ACCA_HS);
              ac_state = true;
              break;
              case ACCL_CHILD :
              ACIR->send(irsender, POWER_ON, ACCL_MODE, ACCL_FAN, ACCL_TEMP, ACCL_VS, ACCL_HS);
              ac_state = true;
              break;
      }
        
      }

    }

    if (message.type == V_DIMMER) {

      int requestedLevel = atoi( message.data );

      requestedLevel = constrain(requestedLevel, 0, 100);

      if (requestedLevel = 0) {
        ACIR->send(irsender, POWER_OFF, MODE_AUTO, FAN_AUTO, 26, VDIR_AUTO, HDIR_AUTO);
        ac_state = false;
      } else {

        int tempAC = map(requestedLevel, 1, 100, 20, 30);

        ACIR->send(irsender, POWER_ON, ACAA_MODE, ACAA_FAN, tempAC, ACAA_VS, ACAA_HS);
        ac_state = true;
      }

  }
}




