#include <TimerOne.h>


#define BUCKPIN 9
#define DUTYCYCLE 0.56
#define PERIOD 10 // PERIOD = 1/Frequency * 1e6

void setup() {
  
  Serial.begin(9600);
    Serial.println("Board Reset");
    
     pinMode(BUCKPIN, OUTPUT);
     Timer1.initialize(PERIOD);        // initialize timer1, and set a 1/2 second period
     Timer1.pwm(BUCKPIN, DUTYCYCLE * 1024);  
     
}

void loop() {
 
 
  
}
