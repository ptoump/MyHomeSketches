#include <Timer.h>
#include <SPI.h>

#define TIMESTEP 10         //define interval in seconds
#define RESISTOR 4.5    	//define value of resistor used as shunt
#define ENDVOLTAGE 2900    	//define the voltage at which the test stops in mV
#define FULLVOLTAGE 4200	//define full (starting) voltage
#define VOLTAGEPIN A1    	//define the pin number for Analog input
#define RELAY 12           	//this is the pin number of the output to the Relay Board        

float capacity =0;
long prevMillis = 0;
long currMillis = 0;
unsigned period = 0;
float voltage = 4200;
float current = voltage / RESISTOR ;
float stepCapacity = current * TIMESTEP ;
float totalCapacity = 0 ;
int timer_int = 0;
float correction = ( 5 * 3810 ) / ( 1.024 * 4272. );  // Correction factor ( Multimeter / analog read )

Timer t;

void setup()
{
	pinMode(RELAY, OUTPUT);  
	digitalWrite(RELAY, HIGH);

	// Calculate the correction factor by taking a reading at NO LOAD and assuming it 
	// to be FULLVOLTAGE !!!! Warning !!!! If reset during the procedure it WILL damage the battery

	correction = (float)(( FULLVOLTAGE * 5 )  / ( 1.024 * analogRead(VOLTAGEPIN) ));
  	
  	Serial.begin( 115200 );
  	Serial.print( "Initializing...  correction factor : ");
  	Serial.println(correction);
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
	Serial.print(currMillis / 1000);Serial.print(";");
	Serial.print(period / 1000);Serial.print(";");
	Serial.print(voltage);Serial.print(";");
	Serial.print(current);Serial.print(";");
	Serial.print(stepCapacity);Serial.print(";");
	Serial.println(totalCapacity);
	
	if (voltage < ENDVOLTAGE)
	{
		t.stop(timer_int);
		digitalWrite(RELAY, LOW);
	}
		

}
