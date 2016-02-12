// #include <IRremote.h>
#include <Arduino.h>
#include <MySensor.h>
#include <MyTransportNRF24.h>
#include <MyHwATMega328.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Timer.h>
#include <avr/pgmspace.h>
#include <PanasonicHeatpumpIR.h>

#define SN "AC_n_TEMP_bsm"
#define SV "1.6"

#define ONE_WIRE_BUS 5 // Pin where dallas sensor is connected, PIN 3 Is used by IRSend !!!

// AirCon Defaults

// #define ACWA_TEMP   26          // 16..30
// #define ACWA_FAN    FAN_AUTO    // FAN_AUTO, FAN_1 .. FAN_5
// #define ACWA_MODE   MODE_HEAT   // MODE_AUTO,MODE_COOL, MODE_HEAT, MODE_DRY, MODE_FAN, MODE_OFF
// #define ACWA_VS     VDIR_MIDDLE // VDIR_AUTO, VDIR_UP, VDIR_MUP, VDIR_MIDDLE, VDIR_MDOWN, VDIR_DOWN
// #define ACWA_HS     HDIR_AUTO   // HDIR_AUTO, HDIR_LEFT .. HDIR_RIGHT

// #define ACWL_TEMP   26
// #define ACWL_FAN    FAN_1 
// #define ACWL_MODE   MODE_HEAT
// #define ACWL_VS     VDIR_MIDDLE
// #define ACWL_HS     HDIR_AUTO  

// #define ACAA_TEMP   26
// #define ACAA_FAN    FAN_AUTO 
// #define ACAA_MODE   MODE_AUTO
// #define ACAA_VS     VDIR_AUTO
// #define ACAA_HS     HDIR_AUTO  

// #define ACCA_TEMP   28
// #define ACCA_FAN    FAN_AUTO 
// #define ACCA_MODE   MODE_COOL
// #define ACCA_VS     VDIR_UP
// #define ACCA_HS     HDIR_AUTO  

// #define ACCL_TEMP   28
// #define ACCL_FAN    FAN_1
// #define ACCL_MODE   MODE_COOL
// #define ACCL_VS     VDIR_UP
// #define ACCL_HS     HDIR_AUTO  

// Children Defaults

#define Temp_CHILD 1           // Id of the DHT-temperature child
#define ACPOWER_CHILD 2           // Id of the AC Warm 2 hours child
#define ACMODE_CHILD 3           // Id of the AC Warm 6 hours child
#define ACFAN_CHILD 4
#define ACVDIR_CHILD 5
#define ACPRQT_CHILD 6
#define SHVAC_CHILD 20

// Dallas Stuff
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.
DeviceAddress tempDeviceAddress;
const int  resolution = 11; // DS18B20 can set the resolution up to 12 bits

MyTransportNRF24 radio(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_MAX);

MySensor gw(radio);

MyMessage tempMsg(Temp_CHILD, V_TEMP);
MyMessage acpowerMsg(ACPOWER_CHILD, V_LIGHT);
MyMessage acmodeMsg(ACMODE_CHILD, V_LIGHT);
MyMessage acfanMsg(ACFAN_CHILD, V_DIMMER);
MyMessage acvdirMsg(ACVDIR_CHILD, V_DIMMER);
MyMessage acprqtMsg(ACPRQT_CHILD, V_DIMMER);
MyMessage actempMsg(SHVAC_CHILD, V_HVAC_SETPOINT_HEAT);
// MyMessage hvaccMsg(SHVAC_CHILD, V_HVAC_SETPOINT_COOL);
// MyMessage hvacfsMsg(SHVAC_CHILD, V_HVAC_FLOW_STATE);
// MyMessage hvacfmMsg(SHVAC_CHILD, V_HVAC_FLOW_MODE);
// MyMessage hvacspMsg(SHVAC_CHILD, V_HVAC_SPEED);


IRSenderPWM irsender(3);
PanasonicNKEHeatpumpIR *ACIR = new PanasonicNKEHeatpumpIR();

Timer t;

boolean ac_power = false;
float ac_temp = 25.0 ;
int ac_mode = MODE_HEAT ; // 0 for heat ; 1 for cool
int ac_fan = FAN_AUTO ; // 0 for auto 1-5 for speed
int ac_vdir = VDIR_AUTO ; // 0 for auto 
boolean ac_powerfull = false ; 
boolean ac_quiet = false ;
float lastTemp = 0;
boolean setupfinished = false ;

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
  gw.begin(incomingMessage, AUTO, true); gw.wait(100);

  // Register all sensors to gw (they will be created as child devices)
  gw.present( Temp_CHILD, S_TEMP ); gw.wait(100);
  gw.present( ACPOWER_CHILD, S_LIGHT ); gw.wait(100);
  gw.present( ACMODE_CHILD, S_LIGHT ); gw.wait(100);
  gw.present( ACFAN_CHILD, S_DIMMER ); gw.wait(100);
  gw.present( ACVDIR_CHILD, S_DIMMER ); gw.wait(100);
  gw.present( ACPRQT_CHILD, S_DIMMER ); gw.wait(100);
  gw.present( SHVAC_CHILD, S_HVAC ); gw.wait(1000);
  Serial.println(F("requesting TEMP"));
  gw.request(SHVAC_CHILD, V_HVAC_SETPOINT_HEAT); gw.wait(2000);
  Serial.println(F("requesting mode"));
  gw.request(ACMODE_CHILD, V_LIGHT); gw.wait(1000);
  Serial.println(F("requesting fan"));
  gw.request(ACFAN_CHILD, V_DIMMER); gw.wait(1000);
  Serial.println(F("requesting vdir"));
  gw.request(ACVDIR_CHILD, V_DIMMER); gw.wait(1000);
  Serial.println(F("requesting power"));
  gw.request(ACPRQT_CHILD, V_DIMMER); gw.wait(2000);
  setupfinished = true ;
  gw.request(ACPOWER_CHILD, V_LIGHT); gw.wait(500);
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
    switch (message.sensor) {
          
          case ACPOWER_CHILD :
          ac_power = message.getBool();
          break;
          
          case ACMODE_CHILD :
          switch (message.getBool()) {
            case false : ac_mode = MODE_HEAT; break;
            case true : ac_mode = MODE_COOL; break;
            }
          break;
          
          case ACFAN_CHILD :
          switch (map(atoi( message.data ),0,100,0,5)) {
            case 0 : ac_fan = FAN_AUTO ; break;
            case 1 : ac_fan = FAN_1 ; break;
            case 2 : ac_fan = FAN_2 ; break;
            case 3 : ac_fan = FAN_3 ; break;
            case 4 : ac_fan = FAN_4 ; break;
            case 5 : ac_fan = FAN_5 ; break;
          }
          ac_powerfull = false;
          ac_quiet = false ;
          gw.send(acprqtMsg.set(50));
          break;
          
          case ACVDIR_CHILD :
          switch (map(atoi( message.data ),0,100,0,5)) {
            case 0 : ac_vdir = VDIR_AUTO ; break;
            case 1 : ac_vdir = VDIR_UP ; break;
            case 2 : ac_vdir = VDIR_MUP ; break;
            case 3 : ac_vdir = VDIR_MIDDLE ; break;
            case 4 : ac_vdir = VDIR_MDOWN ; break;
            case 5 : ac_vdir = VDIR_DOWN ; break;
          }
          break;
          
          case ACPRQT_CHILD :
          switch (atoi( message.data )/33) {
            // switch (map(atoi( message.data ),0,100,0,2)) {
            case 0 : ac_powerfull = false; ac_quiet = true ; ac_fan = 1 ; break;
            case 1 : ac_powerfull = false; ac_quiet = false ; ac_fan = 0 ; break;
            case 2 : ac_powerfull = true; ac_quiet = false ; ac_fan = 0 ; break;
          }
          gw.send(acfanMsg.set(map(ac_fan,0,5,0,100)));
          break;
          
          case SHVAC_CHILD :
          ac_temp = int(atof(message.data));
          gw.send(actempMsg.set(ac_temp,1));
          break;
  }

  if (setupfinished) {
    ACIR->send(irsender, ac_power, ac_mode, ac_fan, int(ac_temp), ac_vdir, HDIR_AUTO, ac_powerfull, ac_quiet);
  }
        Serial.print(F("AC: "));
        Serial.print(F("power: "));Serial.print(ac_power);
        Serial.print(F(" mode: "));Serial.print(ac_mode);
        Serial.print(F(" fan: "));Serial.print(ac_fan);
        Serial.print(F(" temp: "));Serial.print(int(ac_temp));
        Serial.print(F(" vdir: "));Serial.print(ac_vdir);
        Serial.print(F(" prfl: "));Serial.print(ac_powerfull);
        Serial.print(F(" quiet: "));Serial.println(ac_quiet);
      
  }





