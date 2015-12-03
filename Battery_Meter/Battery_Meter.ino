#include <Time.h>
#include <SPI.h>

#define TIMESTEP 10         //define interval in seconds
#define RESISTOR 4.2    	//define value of resistor used as shunt
#define ENDVOLTAGE 2.9    	//define the voltage at which the test stops
#define VOLTAGEPIN A1    		//define the pin number for input A, which will be used to read the cell positive voltage
#define RELAY 12           	//this is the pin number of the output to the transistor base or mosfet gate        

float capacity =0;
unsigned prevMillis = 0;
unsigned currMillis = 0;
unsigned period = 0;
float voltage = 4.2;
float current = voltage / RESISTOR ;
float stepCapacity = current * TIMESTEP ;
float totalCapacity = 0 ;
int timer_int = 0;

Timer t;

void setup()
{
	pinMode(RELAY, OUTPUT);  
  	Serial.begin(115200);
  	Serial.println("Timestamp;Period;Voltage;Current;Capacity_in_Period;Total_Capacity");
  	prevMillis = millis(); 	
  	digitalWrite(RELAY, HIGH);
  	timer_int = t.every( TIMESTEP * 1000L , doProcess)
}

void loop()
{
	t.update();
}


void doProcess() 
{	
	currMillis = millis();
	period = currMillis - prevMillis ;
	voltage = (float)(analogRead(VOLTAGEPIN)*5)/1024.);
	prevMillis = currMillis;
	current = voltage / RESISTOR ;
	stepCapacity = (current * period) / 1000.;
	totalCapacity += stepCapacity;
	Serial.print(currMillis);Serial.print(";");
	Serial.print(period);Serial.print(";");
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