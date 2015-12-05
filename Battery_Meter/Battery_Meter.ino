#include <Timer.h>
#include <SPI.h>

#define TIMESTEP 10         //define interval in seconds
#define RESISTOR 4.5    	  //define value of resistor used as shunt
#define ENDVOLTAGE 3000    	//define the voltage at which the test stops in mV
#define FULLVOLTAGE 4060	  //define full (starting) voltage
#define VOLTAGEPIN A1    	  //define the pin number for Analog input
#define RELAY 12           	//this is the pin number of the output to the Relay Board        

// Vcc Stuff
float voltagecc = 0;
const long scale_constant = 1100 * (4580 / 4688.) * 1023L ; // correction factor (multimeter / measured)


float capacity = 0;
long prevMillis = 0;
long currMillis = 0;
unsigned period = 0;
float voltage = 4200;
float current = voltage / RESISTOR ;
float stepCapacity = current * TIMESTEP ;
float totalCapacity = 0 ;
int timer_int = 0;
float correction = 1. ;  // Correction factor

Timer t;

void setup()
{
  Serial.begin( 115200 );
  pinMode(RELAY, OUTPUT);

  // The analogread() has as reference the Vcc which isn't always 5V.
  // We will measure the Vcc using the internal 1.1V reference
  // and use this to correct our readings

  digitalWrite(RELAY, HIGH);
  delay(500);
  Serial.print( "Initializing...  Vcc: ");
  voltagecc = readVcc();
  Serial.print( voltagecc );
  Serial.print( " correction factor : ");
  correction = (float)(voltagecc / 1024.);
  Serial.println(correction);
  analogReference(DEFAULT);
  digitalWrite(RELAY, LOW);

  Serial.print( "Please connect the battery.. ");
  for (int i = 5 ; i > 0 ; i--)
  {
    delay(1000);
    Serial.print(i);
    Serial.print(", ");
  }
  Serial.println("Starting Measurement.");
  delay(1000);
  Serial.println("Timestamp;Period;Voltage;Current;Capacity_in_Period;Total_Capacity");
  prevMillis = millis();
  digitalWrite(RELAY, HIGH);
  timer_int = t.every( TIMESTEP * 1000L , doProcess);
}

void loop()
{
  t.update();
}


void doProcess()
{
  currMillis = millis();
  period = currMillis - prevMillis ;
  voltage = (float)(analogRead(VOLTAGEPIN) * correction);
  prevMillis = currMillis;
  current = voltage / RESISTOR ;
  stepCapacity = (current * period) / (3600 * 1000.);
  totalCapacity += stepCapacity;
  Serial.print(currMillis / 1000); Serial.print(";");
  Serial.print(period / 1000); Serial.print(";");
  Serial.print(voltage); Serial.print(";");
  Serial.print(current); Serial.print(";");
  Serial.print(stepCapacity); Serial.print(";");
  Serial.println(totalCapacity);

  if (voltage < ENDVOLTAGE)
  {
    t.stop(timer_int);
    digitalWrite(RELAY, LOW);
  }


}

// Based from http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
long readVcc() {
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(500); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;
  result = scale_constant / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
